from flask import Flask, render_template, send_from_directory, request, jsonify, redirect, url_for
import pymysql
import os

# --- CONFIG ---
PHOTOS_DIR = "/home/noefu/nichoire/photos"
app = Flask(__name__)

# --- BDD ---
def get_db():
    return pymysql.connect(
        host="localhost",
        user="nichoir_user",
        password="88N88a88",
        database="nichoir",
        cursorclass=pymysql.cursors.DictCursor # Transforme les données de tupple en dictionnaire
    )

# --- ACCUEIL ---
@app.route("/")
def index():
    db = get_db()
    cursor = db.cursor()

    # 1. DERNIERE BATTERIE
    cursor.execute("SELECT niveau_batterie FROM donnees ORDER BY date_heure DESC LIMIT 1")
    last_bat_data = cursor.fetchone()
    derniere_batterie = last_bat_data['niveau_batterie'] if last_bat_data else "N/A"

    # 2. RECUPERATION DES FILTRES
    f_appareil = request.args.get('appareil')
    f_mode_date = request.args.get('mode_date') # 'periode' ou 'exact'
    f_date_exacte = request.args.get('date_exacte')
    f_date_min = request.args.get('date_min')
    f_date_max = request.args.get('date_max')
    f_favoris  = request.args.get('favoris')

    # 3. CONSTRUCTION REQUETE GALERIE
    # On récupère a.nom ET a.emplacement
    sql = """
        SELECT d.id, d.date_heure, d.niveau_batterie, d.chemin_photo, d.favori,
               a.nom, a.emplacement
        FROM donnees d
        JOIN appareils a ON d.appareil_id = a.id
        WHERE d.chemin_photo IS NOT NULL AND d.chemin_photo != ''
    """
    params = []

    # Filtre Appareil
    if f_appareil:
        sql += " AND d.appareil_id = %s"
        params.append(f_appareil)

    # Filtre Date (Logique Intelligente)
    if f_mode_date == 'exact' and f_date_exacte:
        # Cas : Date précise uniquement
        sql += " AND DATE(d.date_heure) = %s"
        params.append(f_date_exacte)
    else:
        # Cas : Période (Avant, Après ou Entre)
        if f_date_min:
            sql += " AND DATE(d.date_heure) >= %s"
            params.append(f_date_min)
        if f_date_max:
            sql += " AND DATE(d.date_heure) <= %s"
            params.append(f_date_max)

    # Filtre Favoris
    if f_favoris:
        sql += " AND d.favori = 1"

    sql += " ORDER BY d.date_heure DESC"

    cursor.execute(sql, params)
    donnees = cursor.fetchall()

    # 4. LISTE APPAREILS (Avec le NOM cette fois)
    cursor.execute("SELECT id, nom, emplacement FROM appareils")
    appareils = cursor.fetchall()

    db.close()

    return render_template("index.html",
                           donnees=donnees,
                           appareils=appareils,
                           derniere_batterie=derniere_batterie)

# --- CONFIGURATION ---
@app.route("/config", methods=['GET', 'POST'])
def config():
    db = get_db()
    cursor = db.cursor()

    if request.method == 'POST':
        id_app = request.form.get('id')
        nom = request.form.get('nom')
        emplacement = request.form.get('emplacement')
        desc = request.form.get('description')
        en_service = 1 if request.form.get('en_service') else 0

        sql = "UPDATE appareils SET nom=%s, emplacement=%s, description=%s, en_service=%s WHERE id=%s"
        cursor.execute(sql, (nom, emplacement, desc, en_service, id_app))
        db.commit()
        return redirect(url_for('config'))

    cursor.execute("SELECT * FROM appareils")
    appareils = cursor.fetchall()
    db.close()
    return render_template("config.html", appareils=appareils)

# --- DETAIL PHOTO ---
@app.route("/detail/<int:id_photo>")
def detail(id_photo):
    db = get_db()
    cursor = db.cursor()
    sql = """
        SELECT d.*, a.nom, a.emplacement, a.description, a.en_service
        FROM donnees d
        JOIN appareils a ON d.appareil_id = a.id
        WHERE d.id = %s
    """
    cursor.execute(sql, (id_photo,))
    data = cursor.fetchone()
    db.close()
    return render_template("photo.html", data=data)

# --- API & ROUTES STANDARDS ---
@app.route("/api/favori/<int:id_photo>", methods=['POST'])
def toggle_favori_api(id_photo):
    db = get_db()
    cursor = db.cursor()
    cursor.execute("UPDATE donnees SET favori = NOT favori WHERE id = %s", (id_photo,))
    db.commit()
    cursor.execute("SELECT favori FROM donnees WHERE id = %s", (id_photo,))
    result = cursor.fetchone()
    db.close()
    return jsonify({"success": True, "nouvel_etat": result['favori']})

@app.route("/photo/<filename>")
def serve_photo(filename):
    return send_from_directory(PHOTOS_DIR, filename)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)