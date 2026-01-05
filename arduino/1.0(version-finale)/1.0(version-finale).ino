#include "M5TimerCAM.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h> 
#include <Preferences.h> 

#define PIR_PIN GPIO_NUM_13
#define LED_PIN GPIO_NUM_4
#define ALIM_PIN GPIO_NUM_33

// Valeur par défaut (écraser lors de l'initialisation)
char mqtt_serveur[40] = "172.20.10.3";
char mqtt_port[6]    = "1883";

// --- TEMPS ---
#define DUREE_HIBERNATION_SEC  10  // 10 minutes
#define DUREE_SURVEILLANCE_SEC 30   // 30 secondes
#define CYCLES_POUR_24H        144  // 144 * 10min = 24h

// --- ETATS DU SYSTEME ---
enum NichoirState {
  ETAT_INITIATION = 0,
  ETAT_APRES_HIBERNATION,
  ETAT_APRES_DEEPSLEEP
};

WiFiClient espClient;
PubSubClient client(espClient);
Preferences preferences; // Mémoire de stockage
WiFiManager wm;          // Gestionnaire WiFi

// Variable pour savoir si on doit sauvegarder les params après config
bool sauvegarderConfig = false;

void saveConfigCallback();
void setupWiFiManager();
bool connexionRapide();
void envoyerPhotoEtBatterie();
void envoyerBatterieSeule();
void programmerHibernation(int secondes);
void programmerSurveillance();


void setup() {
    Serial.begin(115200);
    
    // 1. MAINTIEN ALIMENTATION
    pinMode(ALIM_PIN, OUTPUT);
    digitalWrite(ALIM_PIN, HIGH);
    gpio_hold_en((gpio_num_t)ALIM_PIN);
    gpio_deep_sleep_hold_en();

    // 2. INIT MATERIEL
    TimerCAM.begin(true); 

    if (!TimerCAM.Camera.begin()) {
        Serial.println("ERREUR: Init Caméra échouée !");
        // Si la caméra ne démarre pas, on tente de dormir pour réessayer plus tard
        programmerHibernation(10); 
    }

    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_VGA);
    TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
    TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);

    pinMode(PIR_PIN, INPUT_PULLDOWN);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 3. LECTURE MEMOIRE (PREFERENCES)
    preferences.begin("nichoir", false); // false = mode lecture/écriture - true = lecture seule
    
    int etatActuel = preferences.getInt("etat", ETAT_INITIATION);
    int compteur24h = preferences.getInt("cycle", 0);
    
    // On charge aussi la config MQTT sauvegardée
    String sauvegarde_mqtt = preferences.getString("mqtt_serveur", mqtt_serveur); 
    String sauvegarde_port = preferences.getString("mqtt_port", mqtt_port);
    
    // On convertit les String en char array pour les utiliser plus tard
    sauvegarde_mqtt.toCharArray(mqtt_serveur, 40);
    sauvegarde_port.toCharArray(mqtt_port, 6);
    
    Serial.printf(">>> DEMARRAGE. Etat: %d, Serveur: %s\n", etatActuel, mqtt_serveur);
    Serial.println("=====================================");



    // ------------------------------------------------------
    // LOGIQUE PRINCIPALE
    // ------------------------------------------------------



    // --- CAS 0 : PREMIER DEMARRAGE (ou après un reset) ---
    if (etatActuel == ETAT_INITIATION) {
        Serial.println(">>> MODE CONFIGURATION (AP) <<<");
        setupWiFiManager(); // Ouvre le portail WiFi + Config MQTT
        
        // Une fois configuré, on prépare le cycle
        preferences.putInt("etat", ETAT_APRES_HIBERNATION); 
        preferences.putInt("cycle", 0);
        
        // Si une config a été changée, on la sauvegarde
        if (sauvegarderConfig) {
            preferences.putString("mqtt_serveur", mqtt_serveur);
            preferences.putString("mqtt_port", mqtt_port);
            Serial.println("Configuration MQTT sauvegardée !");
        }
        
        // On fait un petit dodo test de 10s pour vérifier que le réveil marche
        programmerHibernation(10); 
    }

    // --- CAS 1 : ON VIENT DE L'HIBERNATION ---
    else if (etatActuel == ETAT_APRES_HIBERNATION) {
        compteur24h++;
        preferences.putInt("cycle", compteur24h);

        if (compteur24h >= CYCLES_POUR_24H) {
             Serial.println("Cycle 24h : Envoi rapport batterie...");
             envoyerBatterieSeule(); 
             preferences.putInt("cycle", 0); 
        }

        Serial.println("Passage en SURVEILLANCE (30s)...");
        preferences.putInt("etat", ETAT_APRES_DEEPSLEEP);
        preferences.end();
        programmerSurveillance();
    }

    // --- CAS 2 : ON VIENT DE LA SURVEILLANCE (deepsleep) ---
    else if (etatActuel == ETAT_APRES_DEEPSLEEP) {
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

        if (cause == ESP_SLEEP_WAKEUP_EXT0) {
            delay(100);

            // On vérifie qu'il ne s'agit bien d'un oiseau et non une perturbation électrique
            if (digitalRead(PIR_PIN) == HIGH) {
                Serial.println("!!! OISEAU CONFIRMÉ !!!");
                envoyerPhotoEtBatterie();
            } else {
                Serial.println("Fausse alerte (Pic électrique au réveil).");
            }
        } else {
            Serial.println("Rien à signaler.");
        }
        
        preferences.putInt("etat", ETAT_APRES_HIBERNATION);
        programmerHibernation(DUREE_HIBERNATION_SEC);
    }
}

void loop() {
}



// ------------------------------------------------------
// GESTION DU PORTAIL WIFI & CONFIG
// ------------------------------------------------------



// Callback appelé quand on clique sur "Save" dans le portail
void saveConfigCallback () {
  Serial.println("Bouton Sauvegarder cliqué !");
  sauvegarderConfig = true;
}

void setupWiFiManager() {
    // 1. PARAMETRES PERSO
    WiFiManagerParameter custom_mqtt_serveur("serveur", "Adresse IP Raspberry/MQTT", mqtt_serveur, 40);
    WiFiManagerParameter custom_mqtt_port("port", "Port (ex: 1883)", mqtt_port, 6);

    wm.addParameter(&custom_mqtt_serveur);
    wm.addParameter(&custom_mqtt_port);
    wm.setSaveConfigCallback(saveConfigCallback);

    // 2. NETTOYAGE DU MENU
    std::vector<const char *> menu = {"wifi"};
    wm.setMenu(menu);

    // 3. STYLE & SCRIPTS (CSS + JS)
    const char* customHead = 
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<style>"
        // --- STYLE CSS (Centrage et Mobile) ---
        "body { background-color: #f2f2f2; font-family: Helvetica, sans-serif; }"
        ".wrap { "
        "   max-width: 360px; margin: 30px auto; padding: 20px; "
        "   background: white; border-radius: 10px; "
        "   box-shadow: 0 4px 15px rgba(0,0,0,0.1); text-align: center;"
        "   display: block !important; visibility: visible !important;" 
        "}"
        "input { "
        "   width: 100%; box-sizing: border-box; padding: 10px; "
        "   margin: 5px 0 15px 0; border: 1px solid #ddd; border-radius: 5px;"
        "}"
        "button { "
        "   width: 100%; padding: 12px; border: none; border-radius: 5px; "
        "   background-color: #4CAF50; color: white; font-size: 16px; font-weight: bold; cursor: pointer;"
        "}"
        "button:hover { background-color: #45a049; }"
        ".msg { display: none !important; }"
        "</style>"

        // --- JAVASCRIPT ---
        "<script>"
        "window.addEventListener('load', function() {"
        "  console.log('Script chargé');"

        // --- PARTIE 1 : TRADUCTION ---
        "  function trad(selector, oldTxt, newTxt) {"
        "    var els = document.querySelectorAll(selector);"
        "    for (var i=0; i < els.length; i++) {"
        "      if (els[i].innerText.trim() === oldTxt) els[i].innerText = newTxt;"
        "    }"
        "  }"
        
        // Traduction des Boutons
        "  trad('button', 'Configure WiFi', 'Configurer le WiFi');"
        "  trad('button', 'Refresh', 'Rafraichir');"
        "  trad('button', 'Save', 'Enregistrer');"
        
        // Traduction des Labels
        "  var labels = document.getElementsByTagName('label');"
        "  for(var i=0; i<labels.length; i++) {"
        "     if(labels[i].innerHTML.indexOf('SSID') !== -1) labels[i].innerHTML = 'Nom du WiFi';"
        "     if(labels[i].innerHTML.indexOf('Password') !== -1) labels[i].innerHTML = 'Mot de passe';"
        "  }"

        // --- PARTIE 2 : VALIDATION (Empêcher le vide) ---
        // On cible le formulaire directement
        "  var form = document.querySelector('form');"
        "  if (form) {"
        "      form.addEventListener('submit', function(e) {"
        
        // Récupération des champs par leur ID (défini dans le C++)
        // 's' est l'ID par défaut du SSID dans WiFiManager
        "          var el_ssid = document.getElementById('s');" 
        "          var el_serveur = document.getElementById('serveur');"
        "          var el_port = document.getElementById('port');"
        
        // On vérifie s'ils existent (pour ne pas bugger sur la page d'accueil) et s'ils sont vides
        "          if (el_ssid && el_ssid.value.trim() === '') {"
        "              alert('Il faut choisir un réseau WiFi !');"
        "              e.preventDefault(); return false;"
        "          }"
        "          if (el_serveur && el_serveur.value.trim() === '') {"
        "              alert('L\\'adresse IP du serveur est vide !');"
        "              e.preventDefault(); return false;"
        "          }"
        "          if (el_port && el_port.value.trim() === '') {"
        "              alert('Le port est vide !');"
        "              e.preventDefault(); return false;"
        "          }"
        
        // Si tout est bon
        "          var btn = document.querySelector('button[type=\"submit\"]');"
        "          if(btn) btn.innerText = 'Sauvegarde en cours...';"
        "      });"
        "  }"

        "});"
        "</script>";
    
    wm.setCustomHeadElement(customHead);
    
    // 4. CONFIG GENERALE
    wm.setTitle("Nichoir - Configuration");
    wm.setMinimumSignalQuality(30); 
    wm.setConfigPortalTimeout(180); 

    // 5. LANCEMENT
    if (!wm.autoConnect("Nichoir-Setup")) {
        Serial.println("Timeout ou échec...");
        programmerHibernation(DUREE_HIBERNATION_SEC);
    }

    Serial.println("Connecté !");
    strcpy(mqtt_serveur, custom_mqtt_serveur.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
}

// ------------------------------------------------------
// FONCTIONS RESEAU
// ------------------------------------------------------

bool connexionRapide() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(); 
    
    int t = 0;
    while (WiFi.status() != WL_CONNECTED && t < 40) { 
        delay(500);
        t++;
    }
    
    // --- GESTION INTELLIGENTE DES ECHECS ---
    
    if (WiFi.status() == WL_CONNECTED) {
        // SUCCES : On remet le compteur d'échecs à 0
        preferences.putInt("echecs", 0); 

        int port = atoi(mqtt_port);
        client.setServer(mqtt_serveur, port);
        return true;
    } else {
        // ECHEC : On augmente le compteur
        int nbEchecs = preferences.getInt("echecs", 0) + 1;
        Serial.printf("[WIFI] Echec n°%d\n", nbEchecs);
        preferences.putInt("echecs", nbEchecs);

        // Si on a échoué 3 fois de suite
        if (nbEchecs >= 3) {
            Serial.println("[CRITIQUE] Trop d'échecs ! Forçage du mode AP pour le prochain réveil.");
            preferences.putInt("etat", ETAT_INITIATION);
            preferences.putInt("echecs", 0);
        }
        
        return false;
    }
}

void envoyerPhotoEtBatterie() {

    // --- CHECK 1 : WIFI ---
    if (!connexionRapide()) {
        Serial.println("[ERREUR] Echec connexion WiFi");
        return; 
    }

    // --- ACTION LED ---
    digitalWrite(LED_PIN, HIGH);

    // --- CHECK 2 : CAMERA DISPO ? ---
    // On vérifie si le pointeur caméra existe
    if (TimerCAM.Camera.fb == NULL) {
        Serial.println("[INFO] Framebuffer vide (Normal avant prise)");
    }

    // --- ACTION PHOTO ---
    TimerCAM.Camera.free();
    bool succes = TimerCAM.Camera.get();

    if (!succes) {
        Serial.println("[ERREUR FATALE] Camera.get() a renvoyé FALSE !");
        digitalWrite(LED_PIN, LOW);
        return;
    }
    
    // --- CHECK 4 : DONNÉES PHOTO VALIDES ? ---
    if (TimerCAM.Camera.fb == NULL) {
        Serial.println("[ERREUR FATALE] Le pointeur fb est NULL malgré le succès !");
        digitalWrite(LED_PIN, LOW);
        return;
    }
    
    if (TimerCAM.Camera.fb->len == 0 || TimerCAM.Camera.fb->buf == NULL) {
        Serial.println("[ERREUR FATALE] Photo vide (len=0) ou buffer vide !");
        digitalWrite(LED_PIN, LOW);
        return;
    }

    Serial.printf("[DEBUG] Photo valide ! Taille : %d octets\n", TimerCAM.Camera.fb->len);

    // --- ACTION ENVOI MQTT ---
    if (client.connect("NichoirCam")) {
         
         size_t len = TimerCAM.Camera.fb->len;
         uint8_t *buf = TimerCAM.Camera.fb->buf;
         uint8_t niveauBatterie = (uint8_t)TimerCAM.Power.getBatteryLevel();

         client.beginPublish("photo", len + 1, false);
         client.write(&niveauBatterie, 1);
         client.write(buf, len); 
         client.endPublish();
         
         client.disconnect();
    } else {
         Serial.print("[ERREUR] Echec connexion MQTT, State: ");
         Serial.println(client.state());
    }
    
    // Libération mémoire
    TimerCAM.Camera.free();
    digitalWrite(LED_PIN, LOW); 
    Serial.println("[DEBUG] Photo envoyé.");
}

void envoyerBatterieSeule() {
    if (connexionRapide()) {
        if (client.connect("NichoirCam")) {
            int niveauBatterie = TimerCAM.Power.getBatteryLevel();
            client.publish("batterie", String(niveauBatterie).c_str());
            client.disconnect();
        }
    }
}



// ------------------------------------------------------
// GESTION SOMMEIL
// ------------------------------------------------------



void programmerHibernation(int secondes) {
    Serial.printf(">>> Hibernation (OFF) pour %d sec\n", secondes);
    preferences.end();
    
    // 1. ARRÊT TOTAL CAMÉRA
    esp_camera_deinit(); 
    delay(100);

    // 2. FORCAGE DU BUS I2C
    // On redémarre le bus à zéro pour être sûr qu'il est propre (problème avec la fonction de la librairie)
    Wire.end();
    Wire.begin(12, 14);
    Wire.setClock(100000); // Vitesse 100kHz (plus stable que les 400kHz de l'ESP de base)
    delay(100);

    Serial.println(">>> Envoi commande manuelle au BM8563...");

    // 3. PROGRAMMATION MANUELLE DU TIMER (Puce BM8563 - Adresse 0x51)    
    Wire.beginTransmission(0x51);
    Wire.write(0x0E); // Registre Timer Control
    Wire.write(0x82); // 0x82 = Enable Timer + 1Hz Clock (1000 0010) --> 7 = active timer / 1 = 1Hz
    Wire.write(secondes); // Valeur du timer (secondes)
    int erreur = Wire.endTransmission();

    if (erreur == 0) {
        Serial.println(">>> Commande reçue par la puce batterie !");
    } else {
        Serial.printf(">>> ERREUR I2C: Code %d (Le bus est bloqué)\n", erreur);
    }
    
    // 4. COUPURE DU MAINTIEN
    Serial.println(">>> Coupure maintien...");
    Serial.flush();
    
    gpio_hold_dis((gpio_num_t)ALIM_PIN);
    
    // 5. FALLBACK USB
    Serial.println("!!! ECHEC COUPURE (USB ?) -> Deep Sleep Logiciel");
    delay(100);
    esp_sleep_enable_timer_wakeup(secondes * 1000000ULL);
    esp_deep_sleep_start();
}

void programmerSurveillance() {
    Serial.println(">>> Deep Sleep (PIR ON)");
    preferences.end();
    esp_sleep_enable_ext0_wakeup(PIR_PIN, 1); 
    esp_sleep_enable_timer_wakeup(DUREE_SURVEILLANCE_SEC * 1000000ULL);
    esp_deep_sleep_start();
}