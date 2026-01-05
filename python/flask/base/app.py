from flask import Flask, render_template, send_from_directory
import pymysql
import os

# --- CONFIG ---
PHOTOS_DIR = "/home/noefu/nichoire/photos"

# --- Flask ---
app = Flask(__name__)

# --- Connexion MariaDB ---
def get_db():
    return pymysql.connect(
        host="localhost",
        user="nichoir_user",        # ton user MariaDB
        password="88N88a88",        # ton mot de passe
        database="nichoir",         # ta base
        cursorclass=pymysql.cursors.DictCursor  # important → retourne des dicts
    )

# --- Page d'accueil : galerie ---
@app.route("/")
def index():
    db = get_db()
    cursor = db.cursor()

    query = """
        SELECT d.id, d.date_heure, d.niveau_batterie, d.chemin_photo,
               a.emplacement, a.description, a.en_service
        FROM donnees d
        JOIN appareils a ON d.appareil_id = a.id
        ORDER BY d.date_heure DESC;
    """
    cursor.execute(query)
    rows = cursor.fetchall()

    return render_template("index.html", donnees=rows)

# --- Route pour servir les images ---
@app.route("/photo/<filename>")
def serve_photo(filename):
    return send_from_directory(PHOTOS_DIR, filename)

# --- Page détail d'une photo ---
@app.route("/detail/<int:id_photo>")
def detail(id_photo):
    db = get_db()
    cursor = db.cursor()

    query = """
        SELECT d.*, a.emplacement, a.description, a.en_service
        FROM donnees d
        JOIN appareils a ON d.appareil_id = a.id
        WHERE d.id = %s;
    """
    cursor.execute(query, (id_photo,))
    data = cursor.fetchone()

    return render_template("photo.html", data=data)

# --- Lancer le serveur ---
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
