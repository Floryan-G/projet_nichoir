import paho.mqtt.client as mqtt
import pymysql
import os
from datetime import datetime

PHOTOS_DIR = "/home/noefu/nichoire/photos"
DB_HOST = "localhost"
DB_USER = "nichoir_user"
DB_PASS = "88N88a88"
DB_NAME = "nichoir"

MQTT_BROKER = "localhost"
MQTT_TOPIC = "test/photo"

# --- CONNEXION BASE DE DONNEES ---
def get_db_connection():
    return pymysql.connect(
        host=DB_HOST,
        user=DB_USER,
        password=DB_PASS,
        database=DB_NAME,
        autocommit=True
    )

# --- RECEPTION MQTT ---
def on_connect(client, userdata, flags, rc):
    print(f"Connecté au MQTT (Code: {rc})")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    print("------------------------------------------------")
    print(f"Photo reçue ! Taille: {len(msg.payload)} bytes")

    # 1. GENERER LE NOM DE FICHIER
    # Format : photo_YYYYMMDD_HHMMSS.jpg
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"photo_{timestamp}.jpg"
    full_path = os.path.join(PHOTOS_DIR, filename)

    # 2. SAUVEGARDER L'IMAGE SUR LE DISQUE
    try:
        with open(full_path, "wb") as f:
            f.write(msg.payload)
        print(f"-> Image enregistrée : {filename}")
    except Exception as e:
        print(f"ERREUR écriture fichier : {e}")
        return

    # 3. ENREGISTRER DANS LA BASE DE DONNEES
    try:
        connection = get_db_connection()
        with connection.cursor() as cursor:
            # On insère une nouvelle ligne dans la table 'donnees'
            # appareil_id = 1 (config plus tard)
            # 100% de batterie (config plus tard)
            sql = """
                INSERT INTO donnees (date_heure, niveau_batterie, chemin_photo, appareil_id, favori)
                VALUES (%s, %s, %s, %s, %s)
            """

            cursor.execute(sql, (datetime.now(), 100, full_path, 1, 0))

        print("-> Base de données mise à jour !")
        connection.close()

    except Exception as e:
        print(f"ERREUR SQL : {e}")

# --- LANCEMENT ---
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("Démarrage du listener MQTT...")
print(f"Ecoute sur le topic : {MQTT_TOPIC}")
print(f"Stockage dans : {PHOTOS_DIR}")

try:
    client.connect(MQTT_BROKER, 1883, 60)
    client.loop_forever() # Boucle infinie qui bloque le script ici
except KeyboardInterrupt:
    print("Arrêt du script.")