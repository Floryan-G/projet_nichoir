# 🐦 Nichoir Connecté – Projet SmartCities & IoT

![Logo](graphics/Logo.png)

## 📖 Description générale

Le **Nichoir Connecté** est un système **IoT autonome à très basse consommation** destiné à surveiller l’activité d’un nichoir pour oiseaux.  
Il s’inscrit dans un contexte **SmartCities / objets connectés**, où l’optimisation énergétique, la fiabilité et la collecte intelligente de données sont essentielles.

Le système repose sur un **ESP32 M5Stack TimerCam**, associé à un capteur PIR, une LED infrarouge et une batterie.  
Il capture des images uniquement lorsqu’un événement pertinent est détecté et transmet les données via **MQTT** vers une passerelle **Raspberry Pi**, où elles sont stockées dans une base **MariaDB** et consultables via une **interface web**.

---

## 🎯 Objectifs du projet

- Surveiller automatiquement l’activité d’un nichoir
- Minimiser la consommation énergétique grâce à l’hibernation
- Capturer et transmettre des images uniquement lors d’événements utiles
- Centraliser et historiser les données
- Offrir une interface web simple pour la consultation
- Garantir une autonomie de plusieurs mois sur batterie

---

## 🧠 Architecture globale

Le système est composé de trois grandes briques :

1. **Nœud embarqué (Nichoir)**
   - ESP32 M5Stack TimerCam
   - Capteur PIR
   - LED infrarouge
   - Batterie LiPo
   - Gestion avancée du sommeil (Deep Sleep / Hibernation)

2. **Passerelle**
   - Raspberry Pi
   - Broker MQTT
   - Traitement des messages entrants

3. **Stockage & Interface**
   - Base de données MariaDB
   - Interface web de visualisation

---

## ⚙️ Fonctionnalités principales

- 📷 Capture d’images déclenchée par détection de mouvement (PIR)
- 🔋 Surveillance du niveau de batterie avec envoi périodique (1 fois par jour)
- 📡 Transmission des données via le protocole MQTT
- 🗄️ Stockage structuré des images et métadonnées dans MariaDB
- 🌐 Interface web pour la consultation des données
- 😴 Gestion énergétique avancée :
  - Deep Sleep avec réveil par interruption externe
  - Activation du Wi-Fi uniquement lorsque nécessaire
  - Hibernation globale du système

---

## 🔋 Gestion de l’énergie (point clé du projet)

Le Nichoir Connecté adopte un fonctionnement **événementiel** :

- **Hibernation / Deep Sleep**  
  - État principal du système  
  - ESP32 arrêté, RTC actif  
  - Consommation mesurée : **≈ 20–30 µA**

- **Réveil par interruption PIR**  
  - Capture d’image
  - Transmission des données
  - Retour immédiat en hibernation

- **Réveil périodique (1× / jour)**  
  - Mesure et envoi du niveau de batterie
  - Impact énergétique négligeable

Cette stratégie permet une **autonomie de plusieurs mois**, malgré des phases ponctuelles très consommatrices (Wi-Fi).

---

## 🧱 Boîtier

Le boîtier a été conçu spécifiquement pour une utilisation extérieure :

- Impression 3D en **PETG** (résistant aux UV et à l’humidité)
- Intégration compacte de l’électronique
- Positionnement optimisé :
  - Caméra orientée vers l’entrée du nichoir
  - PIR placé pour limiter les faux positifs

![BoxV2Design](graphics/BoxV2Design.png)

---

## 🔌 Schéma électrique

Le schéma de câblage présente :
- l’ESP32 TimerCam
- le capteur PIR
- la LED infrarouge pilotée
- la gestion de l’alimentation

![PCBSchematic](graphics/PCBSchematic.png)

---

## 🧩 PCB

Un PCB dédié a été développé afin d’améliorer :
- la fiabilité électrique
- la stabilité de la détection PIR
- la reproductibilité du système

![PCBTop](graphics/PCBTop.png)
![PCBBottom](graphics/PCBBottom.png)

---

## 🔑 Première connexion (configuration Wi-Fi)

Lors de la première mise sous tension :

1. L’ESP32 démarre en **mode configuration**
2. Création d’un point d’accès Wi-Fi : `Nichoir-Config`
3. Connexion depuis un smartphone ou un PC
4. Accès à une page web de configuration
5. Saisie du SSID et du mot de passe Wi-Fi
6. Sauvegarde en mémoire non volatile
7. Redémarrage automatique du système

Ce mode n’est utilisé **qu’une seule fois**, lors de l’installation.

---

## 🌐 Interface Web

L’interface web permet de :
- visualiser les images capturées
- consulter l’historique des événements
- afficher le niveau de batterie
- analyser l’activité du nichoir dans le temps

---

## 🚀 Améliorations envisagées

- Support de **plusieurs nichoirs**
- Réseau maillé (communication sans Wi-Fi permanent)
- Ajout d’un capteur environnemental (température, humidité, CO₂)
- Alimentation par panneau solaire
- Méthodes de configuration alternatives (QR Code / NFC)

---

## 📚 Contexte académique

Ce projet a été réalisé dans le cadre du cours **SmartCities & IoT**  
et vise à valider les compétences liées à :
- la conception de systèmes embarqués
- la gestion énergétique
- l’intégration hardware / software
- la communication IoT
- la fiabilité et la documentation technique

---


