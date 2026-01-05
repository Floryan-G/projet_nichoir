#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"

// --- CONFIGURATION UTILISATEUR ---
const char* ssid = "iPhone de Flo";          // <--- A MODIFIER
const char* password = "123456789";  // <--- A MODIFIER
const char* mqtt_server = "172.20.10.3"; // <--- IP DE LA RASPBERRY

/// --- PINS (Selon tes infos : PIR=13, LED=4) ---
#define PIR_PIN GPIO_NUM_13  
#define LED_PIN GPIO_NUM_4 
#define POWER_HOLD_PIN GPIO_NUM_33

// --- TIMINGS (En Microsecondes) ---
#define TIME_COOLDOWN  30000000ULL  // 30 secondes (Après une photo)
#define TIME_ROUTINE   60000000ULL  // 60 secondes (Si calme plat)

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(115200);
    
    // 1. INIT PINS
    pinMode(PIR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);

    gpio_hold_en(POWER_HOLD_PIN);
    gpio_deep_sleep_hold_en();

    // 2. INIT CAMERA
    TimerCAM.begin();
    if (!TimerCAM.Camera.begin()) {
        Serial.println("Camera Init Fail");
        return;
    }
    // Qualité SVGA (800x600)
    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_SVGA);
    TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
    TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);

    // 3. ANALYSE DU RÉVEIL
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        // --- CAS 1 : C'EST UN OISEAU ! (PIR) ---
        Serial.println(">>> OISEAU DÉTECTÉ !");
        
        connectWifi();
        connectMqtt();
        
        // Flash ON -> Photo -> Flash OFF
        digitalWrite(LED_PIN, HIGH);
        delay(100); 
        prendreEtEnvoyerPhoto();
        digitalWrite(LED_PIN, LOW);

        // PREPARATION DU DODO "ANTI-SPAM" (30s)
        // On n'active PAS le PIR. On force une pause de 30s.
        Serial.println("Pause de 30s (Cooldown)...");
        esp_sleep_enable_timer_wakeup(TIME_COOLDOWN);
    } 
    else {
        // --- CAS 2 : C'EST LE TIMER (Fin de cooldown OU Routine check) ---
        // (Ou premier démarrage)
        Serial.println(">>> TIMER / ROUTINE");
        
        connectWifi();
        connectMqtt();
        
        // On envoie le statut batterie
        // (Note: La fonction getBatteryVoltage est dispo sur certaines libs M5, 
        // sinon on envoie juste le signal de vie)
        client.publish("nichoir/status/1", "Batterie OK - En attente");
        Serial.println("Statut envoyé.");
        
        // PREPARATION DU DODO "SURVEILLANCE" (60s)
        // On réactive le PIR (Pin 13) pour guetter les oiseaux
        // On met un timer de 60s : si personne ne vient, on se réveillera pour dire "Batterie OK"
        Serial.println("PIR Activé. Dodo pour max 60s...");
        esp_sleep_enable_ext0_wakeup(PIR_PIN, 1); 
        esp_sleep_enable_timer_wakeup(TIME_ROUTINE);
    }

    // 4. NETTOYAGE ET SOMMEIL
    TimerCAM.Camera.free(); 
    client.disconnect();
    WiFi.disconnect();
    
    Serial.flush(); 
    esp_deep_sleep_start();
}

void loop() {
    // Vide (Deep Sleep)
}

// --- FONCTIONS ---
void connectWifi() {
    WiFi.begin(ssid, password);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 20) {
        delay(500);
        i++;
    }
}

void connectMqtt() {
    client.setServer(mqtt_server, 1883);
    client.setBufferSize(4096);
    client.connect("M5TimerCam");
}

void prendreEtEnvoyerPhoto() {
    if (TimerCAM.Camera.get()) {
        uint8_t *buffer = TimerCAM.Camera.fb->buf;
        size_t remaining = TimerCAM.Camera.fb->len;
        size_t chunkSize = 1024;

        if (client.beginPublish("test/photo", remaining, false)) {
            while (remaining > 0) {
                size_t toWrite = (remaining > chunkSize) ? chunkSize : remaining;
                client.write(buffer, toWrite);
                buffer += toWrite;
                remaining -= toWrite;
            }
            client.endPublish();
        }
        TimerCAM.Camera.free();
    }
}

void printLocalTime() {
  struct tm timeinfo;
  // On essaye de récupérer l'heure (timeout 5000ms)
  if(!getLocalTime(&timeinfo)){
    Serial.println("Echec lecture heure");
    return;
  }
  Serial.println(&timeinfo, "Heure actuelle : %H:%M:%S");
}