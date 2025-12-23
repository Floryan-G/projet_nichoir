#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>   // Pour stocker SSID et mot de passe
#include <DNSServer.h>     // Pour gérer le DNS

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;

String chipID;

// ----------- PAGE HTML DE CONFIGURATION AP -------------
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Configuration WiFi</title>
    <style>
        body {
            font-family: Arial;
            background: #f2f2f2;
            padding: 20px;
        }
        .box {
            background: white;
            padding: 20px;
            border-radius: 10px;
            width: 300px;
            margin: auto;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        input, select, button {
            width: 100%;
            padding: 10px;
            margin-top: 10px;
            border-radius: 5px;
            font-size: 14px;
        }
        .row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 10px;
        }
        .row button {
            width: 40px; /* Bouton plus petit */
            font-size: 16px;
            border: 2px solid #ccc; /* Bordure discrète */
            background-color: transparent;
        }
        button:hover {
            background-color: #f0f0f0; /* Légère ombre au survol */
            cursor: pointer;
        }
        .password-toggle {
            cursor: pointer;
            margin-top: 10px;
            color: #007bff;
        }
    </style>
</head>
<body>
    <div class="box">
        <h2>Configurer le WiFi</h2>
        <form method="POST" action="/save">
            <label for="ssid">Sélectionner un réseau :</label>
            <div class="row">
                <select name="ssid" id="ssid" required>
                    <option value="">Choisir un réseau</option>
                    %SSIDS%
                </select>
                <!-- Bouton rafraîchir -->
                <button type="button" class="refresh-btn" onclick="refreshWifi()">🔄</button>
            </div><br>

            <div class="row">
                <input name="password" id="password" placeholder="Mot de passe" required type="password">
                <button type="button" class="password-toggle" onclick="togglePassword()">👁️</button>
            </div><br>

            <button type="submit">Enregistrer</button>
        </form>
    </div>

    <script>
        function togglePassword() {
            var passField = document.getElementById("password");
            var toggleText = document.querySelector(".password-toggle");
        }

        function refreshWifi() {
            // Rafraîchir uniquement la liste des réseaux sans recharger la page
            fetch('/scan_wifi')
                .then(response => response.json())
                .then(data => {
                    let ssidSelect = document.getElementById("ssid");
                    ssidSelect.innerHTML = "<option value=''>Choisir un réseau</option>"; // Réinitialiser la liste

                    data.forEach(ssid => {
                        let option = document.createElement("option");
                        option.value = ssid;
                        option.textContent = ssid;
                        ssidSelect.appendChild(option);
                    });
                });
        }
    </script>
</body>
</html>
)rawliteral";

// Fonction pour générer le HTML avec les SSID disponibles
String generateHTMLPage() {
    String html = htmlPage;

    // Scanner les réseaux Wi-Fi
    int n = WiFi.scanNetworks();
    String ssidOptions = "";

    if (n == 0) {
        ssidOptions = "<option>Aucun réseau trouvé</option>";
    } else {
        for (int i = 0; i < n; ++i) {
            ssidOptions += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
        }
    }

    // Remplacer le placeholder %SSIDS% par la liste des SSID
    html.replace("%SSIDS%", ssidOptions);
    
    return html;
}

// -----------------------------------------------------------------------------
// LANCER LE MODE ACCESS POINT (CONFIGURATION)
// -----------------------------------------------------------------------------
void startAccessPoint() {
    Serial.println("=== MODE CONFIG (AP) ===");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("NichoirGP4", "12345678");

    Serial.println("AP actif : NichoirGP4");
    Serial.println("IP : " + WiFi.softAPIP().toString());

    // Lancer le serveur web
    server.on("/", []() {
        server.send(200, "text/html", generateHTMLPage());
    });

    // Réception des infos Wi-Fi
    server.on("/save", []() {
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        Serial.println("→ SSID reçu : " + ssid);
        Serial.println("→ MDP reçu : " + password);

        // Enregistrer dans NVS
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);

        server.send(200, "text/html", "<h3>Configuration enregistrée !<br>Redémarrage...</h3>");

        delay(1000);
        ESP.restart();
    });

    // Route pour rafraîchir la liste des réseaux Wi-Fi
    server.on("/scan_wifi", []() {
        int n = WiFi.scanNetworks();
        String jsonResponse = "[";

        for (int i = 0; i < n; i++) {
            jsonResponse += "\"" + WiFi.SSID(i) + "\"";
            if (i < n - 1) jsonResponse += ", ";
        }

        jsonResponse += "]";
        server.send(200, "application/json", jsonResponse);
    });

    // Activer le DNS Server pour rediriger toute requête vers l'ESP (portail captif)
    dnsServer.start(53, "*", WiFi.softAPIP());

    server.onNotFound([]() {
        // Rediriger toutes les requêtes vers la page d'accueil
        server.sendHeader("Location", "/", true);  // Redirection vers la page de configuration
        server.send(302, "text/plain", "");  // HTTP code 302 (redirection)
    });

    server.begin();
}

// -----------------------------------------------------------------------------
// MODE NORMAL : TENTER LA CONNEXION WIFI
// Retourne true = succès / false = échec
// Si échec → efface la config et repasse en AP automatiquement
// -----------------------------------------------------------------------------
bool connectToWiFi() {
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");

    if (ssid == "") return false;  // Pas encore configuré

    Serial.println("Tentative de connexion au WiFi : " + ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    const unsigned long TIMEOUT = 30000;  // 30 secondes

    while (millis() - startAttempt < TIMEOUT) {

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connecté ! IP : " + WiFi.localIP().toString());
            return true;  // Succès
        }

        Serial.print(".");
        delay(500);
    }

    // --------------------------------------------------------
    // ÉCHEC APRÈS 30 SECONDES → on efface la config Wi-Fi
    // --------------------------------------------------------
    Serial.println("\nÉchec de connexion WiFi → Suppression des données WiFi !");
    prefs.clear();  // Efface SSID + mot de passe

    delay(500);

    return false;
}

bool APMode = true;

// ----------------- SETUP --------------------
void setup() {
    Serial.begin(115200);
    prefs.begin("wifi", false);

    chipID = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX)
           + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.println("Chip ID : " + chipID);

    // Si connexion Wi-Fi impossible → repasse en AP
    if (!connectToWiFi()) {
        Serial.println("Échec de la connexion Wi-Fi, démarrage en mode AP");
        startAccessPoint();
        APMode = true;
    } else {
        Serial.println("Connecté au Wi-Fi, démarrage en mode STA");
        APMode = false;
    }
}

// --------------- LOOP ------------------------
void loop() {
    if (APMode) {
        dnsServer.processNextRequest();  // DNS redirection
        server.handleClient();           // Gère les requêtes HTTP du formulaire
    } else {
        // reste du code
    }
}
