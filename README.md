# projet_nichoir
# Nichoir Connecté

## Description

Le projet **NichoirGPT4** est un dispositif IoT destiné à surveiller l'activité d'un nichoir pour oiseaux. Ce système utilise un **M5StackTimerCam** pour capturer des images et envoyer ces données à un serveur via le protocole **MQTT**. Les images et les données sont stockées dans une base de données **MariaDB** et peuvent être consultées via une interface web dédiée.

### Objectif

L'objectif principal du projet est de fournir un moyen simple et fiable de suivre les activités des oiseaux dans un nichoir connecté. Le système permet également une gestion de la batterie, avec la possibilité de récupérer des informations sur l'état de la connexion sans fil et l'historique des captures photo.

## Fonctionnalités

- **Capture d'images** .
- **Surveillance de la batterie** et des événements via le **PIR sensor** (détection de présence).
- **Envoi des données** (images, niveau de batterie, signal, etc.) via **MQTT**.
- **Stockage des données** dans une **base de données MariaDB**.
- **Interface web** pour consulter les images et leurs informations associées.
- **Gestion de l'activation** du système en fonction de la détection de mouvement ou à des intervalles réguliers.

---

## Sections

### Fonctionnalités à venir

Ajoute une description des futures améliorations ou fonctionnalités prévues ici, par exemple :

- **Support pour plusieurs nichoirs** : la gestion d'un réseau de nichoirs connectés.
- **Notifications push** pour alerter l'utilisateur en cas d'activité dans le nichoir.

---

## Boîtier

Le boîtier du **Nichoir Connecté** a été conçu pour être résistant aux intempéries et permettre un montage facile sur un arbre ou dans un environnement extérieur. 

- **Matériau** : Boîtier en PETG résistant aux UV et aux intempéries.
- **Emplacement des composants** : L'ESP32 TimerCam, la batterie et le capteur PIR sont intégrés à l'intérieur du boîtier pour une protection optimale.

*Image ou Schéma du boîtier à insérer ici.*

---

## Schéma

Le schéma de câblage montre comment les composants du **Nichoir Connecté** sont connectés, y compris l'ESP32 TimerCam, le capteur PIR, et les autres modules électroniques.

*Image du schéma de câblage à insérer ici.*

---

## PCB

Le PCB (circuit imprimé) a été conçu pour assurer une connectivité stable et une gestion efficace de l'alimentation. Ce circuit imprime permet de connecter tous les composants électroniques du nichoir de manière compacte et organisée.

*Image du PCB à insérer ici.*

---


## Première Connexion

Lors de la première connexion du **Nichoir Connecté** à votre réseau, le système crée un point d'accès Wi-Fi pour la configuration. Suivez ces étapes :

1. **Allumez le Nichoir** : Lorsque vous allumez le système pour la première fois, l'ESP32 se met en mode **Access Point (AP)** et crée un réseau Wi-Fi nommé "Nichoir-Config".
2. **Connectez-vous à ce réseau Wi-Fi** : Sur votre ordinateur ou smartphone, connectez-vous au réseau **"NichoirGPT4"**.
3. **Accédez à la page de configuration** : Une fois connecté au réseau, une page devrait s'ouvrir
4. **Configurez les paramètres Wi-Fi** : Dans l'interface web, entrez les informations de votre réseau Wi-Fi (SSID et mot de passe).
5. **Redémarrez le système** : Une fois la configuration terminée, cliquez sur le bouton "Sauvegarder" et redémarrez le dispositif. Il se connectera automatiquement à votre réseau Wi-Fi et au broker MQTT.

---

## Site Internet

Le projet inclut une interface web qui vous permet de visualiser les photos capturées et d'accéder aux données stockées dans la base de données MariaDB. Voici comment configurer et utiliser le site internet :

