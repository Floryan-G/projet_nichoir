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
MQTT_TOPIC_PHOTO = "photo"
MQTT_TOPIC_BATTERIE = "batterie" # 24h seulement

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
    print(f"Connecté au Broker (Code: {rc})")
    client.subscribe([(MQTT_TOPIC_PHOTO, 0), (MQTT_TOPIC_BATTERIE, 0)])

def on_message(client, userdata, msg):

    # --- Batterie 24H ---
    if msg.topic == MQTT_TOPIC_BATTERIE:
        print("------------------------------------------------")

        try:
            batterie = int(msg.payload.decode("utf-8"))
            print(f"Rapport 24h : {batterie}%")

            connection = get_db_connection()
            with connection.cursor() as cursor:
                sql = """
                    INSERT INTO donnees (date_heure, niveau_batterie, chemin_photo, appareil_id, favori)
                    VALUES (%s, %s, %s, %s, %s)
                """

                cursor.execute(sql, (datetime.now(), batterie, None, 1, 0))
            connection.close()

        except Exception as e:
            print(f"Erreur lecture rapport 24h: {e}")

    # --- Photo + batterie ---
    elif msg.topic == MQTT_TOPIC_PHOTO:
        print("------------------------------------------------")

        try:
            # --- DECODAGE DU PAQUET ---
            # Octet 0 contient la batterie
            batterie_recue = msg.payload[0]

            # Les octets de 1 à la fin contiennent l'image
            image_data = msg.payload[1:]

            print(f"   Pourcentage batterie : {batterie_recue}%)")
            print(f"   Taille image : {len(image_data)} octets")

            # 1. Sauvegarde Fichier
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"photo_{timestamp}.jpg"
            full_path = os.path.join(PHOTOS_DIR, filename)

            with open(full_path, "wb") as f:
                f.write(image_data)
            print(f"   -> Image enregistrée : {filename}")

            # 2. Enregistrement BDD
            connection = get_db_connection()
            with connection.cursor() as cursor:
                sql = """
                    INSERT INTO donnees (date_heure, niveau_batterie, chemin_photo, appareil_id, favori)
                    VALUES (%s, %s, %s, %s, %s)
                """
                cursor.execute(sql, (datetime.now(), batterie_recue, full_path, 1, 0))
            connection.close()
            print("   -> BDD mise à jour.")

        except Exception as e:
            print(f"Erreur traitement photo : {e}")

# --- LANCEMENT ---
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("Démarrage du listener MQTT...")
print(f"Ecoute sur les topics : {MQTT_TOPIC_PHOTO} & {MQTT_TOPIC_BATTERIE}")
print(f"Stockage dans : {PHOTOS_DIR}")

try:
    client.connect(MQTT_BROKER, 1883, 60)
    client.loop_forever() # Boucle infinie qui bloque le script ici
except KeyboardInterrupt:
    print("Arrêt du script.")