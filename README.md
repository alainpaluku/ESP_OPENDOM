<<<<<<< HEAD
# opendom-esp32
esp32, iot, home-automation, smart-home, pwa, embedded-systems, sensors, actuators, platformio, arduino, academic-project, domotique, offline-first, web-interface, real-time, automation, french
=======
# OPENDOM - Système Domotique Embarqué ESP32

**Description GitHub**: Système domotique autonome ESP32 avec PWA hors ligne, capteurs IoT, LED RGB signalisation, moteur de règles automatiques et interface web moderne. Parfait pour projets académiques et prototypage IoT.

## 🏠 Description

OPENDOM est un système domotique complet et autonome basé sur ESP32, conçu pour fonctionner entièrement hors ligne. Il intègre une Progressive Web App (PWA) moderne avec interface mobile-first et un moteur de règles avancé pour l'automatisation intelligente.

## ✨ Caractéristiques principales

### 🔧 Hardware
- **ESP32** avec serveur web embarqué et point d'accès WiFi
- **LED RGB de signalisation** (rouge=alarme, vert=actionneur actif, bleu=veille)
- **Capteurs supportés**: DHT11, MQ2, ASC, LDR, PIR, Bouton poussoir
- **Actionneurs supportés**: Relais, Buzzer
- **Détection automatique** de déconnexion des capteurs

### 💻 Software
- **Interface PWA** mobile-first avec design Apple Vision
- **Système de règles** automatiques configurable via interface web
- **Authentification** sécurisée avec rôles utilisateur
- **API REST** complète pour contrôle à distance
- **Configuration JSON** centralisée et modifiable
- **Fonctionnement 100% offline** sans dépendance cloud

### 🛡️ Sécurité et fiabilité
- **Triple vérification** des lectures capteurs
- **Validation stabilité** des mesures avec seuils adaptatifs
- **Aucune valeur négative** ou aléatoire des capteurs
- **Gestion robuste** des déconnexions de capteurs
- **Isolation réseau** via point d'accès dédié

## 🎯 Signalisation LED intelligente

- **🔴 Rouge**: Mode alarme (gaz critique, bouton urgence)
- **🟢 Vert**: Actionneur actif (relais en fonctionnement)
- **🔵 Bleu**: Système en veille (fonctionnement normal)
- **⚪ Éteint**: Erreur système ou démarrage

## 📋 Configuration par défaut

- **SSID WiFi**: OPENDOM
- **Mot de passe WiFi**: opendom2025
- **Utilisateur standard**: astron / astron
- **Utilisateur root**: astron / astronome
- **URL d'accès**: http://192.168.4.1

## 🏗️ Structure du projet

```
OPENDOM/
├── platformio.ini              # Configuration PlatformIO
├── src/                        # Code source ESP32
│   ├── main.cpp               # Programme principal
│   ├── Config.cpp             # Gestion configuration
│   ├── Sensor.cpp             # Classes capteurs avec validation
│   ├── Actuator.cpp           # Classes actionneurs
│   └── StatusLED.cpp          # Contrôle LED RGB signalisation
├── include/                    # Headers C++
│   ├── Config.h
│   ├── Sensor.h
│   ├── Actuator.h
│   └── StatusLED.h
├── data/                       # Interface web PWA
│   ├── index.html             # Interface principale
│   ├── style.css              # Styles CSS modernes
│   ├── app.js                 # Application JavaScript
│   ├── manifest.json          # Manifest PWA
│   ├── sw.js                  # Service Worker offline
│   └── configuration.json     # Configuration système
└── README.md
```

## 🚀 Installation rapide

### Prérequis
- [PlatformIO](https://platformio.org/) installé
- ESP32 DevKit ou compatible
- Capteurs et actionneurs selon configuration

### Étapes d'installation

1. **Cloner le projet**
```bash
git clone https://github.com/palukuba/opendom-esp32
cd opendom-esp32
```

2. **Compiler et flasher**
```bash
# Compiler le firmware
pio run

# Uploader le système de fichiers (interface web)
pio run -t uploadfs

# Uploader le firmware
pio run -t upload

# Monitorer les logs
pio device monitor
```

3. **Première connexion**
- Connectez-vous au WiFi "OPENDOM" (mot de passe: opendom2025)
- Ouvrez http://192.168.4.1
- Connectez-vous avec astron/astron

## 📱 Interface utilisateur

### Navigation principale
- **🏠 Accueil**: Tableau de bord temps réel avec statut LED
- **🔍 Surveillance**: Alertes et historique des événements
- **⚙️ Appareils**: Configuration des dispositifs (accès root)
- **⚙️ Paramètres**: Préférences système et thèmes

### Fonctionnalités PWA
- **Installation** comme application native
- **Mode hors ligne** complet avec Service Worker
- **Thème sombre/clair** avec sauvegarde préférences
- **Notifications push** pour alertes critiques
- **Interface responsive** optimisée mobile/desktop

## 🔧 Configuration des dispositifs

### Capteurs par défaut
```json
{
  "DHT11": { "pin": 4, "type": "temperature_humidity" },
  "MQ2": { "pin": 35, "type": "gas_sensor" },
  "ASC": { "pin": 34, "type": "current_sensor" },
  "LDR": { "pin": 33, "type": "light_sensor" },
  "PIR": { "pin": 2, "type": "motion_sensor" },
  "Button": { "pin": 0, "type": "digital_input" }
}
```

### Actionneurs par défaut
```json
{
  "Relay1": { "pin": 5, "type": "relay" },
  "Relay2": { "pin": 18, "type": "relay" },
  "Relay3": { "pin": 19, "type": "relay" },
  "Buzzer": { "pin": 21, "type": "buzzer" }
}
```

## 🎮 Exemples de règles automatiques

1. **Ventilation intelligente**
   - Déclencheur: Température > 30°C OU Gaz > 300ppm
   - Action: Activer Relay1 (ventilateur)

2. **Éclairage extérieur**
   - Déclencheur: Luminosité < 400 lux ET Mouvement détecté
   - Action: Activer Relay2 (éclairage)

3. **Prise programmée**
   - Déclencheur: Horaire 13h00-16h00
   - Action: Activer Relay3 (prise télé)

4. **Alarme gaz critique**
   - Déclencheur: Gaz > 400ppm OU Bouton urgence
   - Action: Buzzer + LED rouge

## 🔌 API REST

| Endpoint | Méthode | Description |
|----------|---------|-------------|
| `/login` | POST | Authentification utilisateur |
| `/api/sensors` | GET | Données capteurs temps réel |
| `/api/actuators` | POST | Contrôle actionneurs |
| `/api/status` | GET | État LED et système |
| `/api/config` | GET/POST | Configuration (root requis) |
| `/api/rules` | GET/POST | Gestion règles automatiques |

## 🛠️ Développement

### Ajouter un nouveau capteur

1. **Créer la classe** dans `include/Sensor.h` et `src/Sensor.cpp`
2. **Ajouter l'initialisation** dans `initDevices()` 
3. **Mettre à jour** `configuration.json`
4. **Ajouter l'icône** dans `getDeviceIcon()` de `app.js`

### Personnaliser la signalisation LED

Modifier la logique dans `src/StatusLED.cpp`:
```cpp
void StatusLED::updateStatus(SystemStatus status) {
    switch(status) {
        case ALARM: setColor(255, 0, 0); break;     // Rouge
        case ACTIVE: setColor(0, 255, 0); break;    // Vert
        case IDLE: setColor(0, 0, 255); break;      // Bleu
        case OFF: setColor(0, 0, 0); break;         // Éteint
    }
}
```

## 🐛 Dépannage

### Problèmes courants

**Capteurs donnent des valeurs incorrectes**
- Vérifier les connexions hardware
- Consulter les logs série pour voir la validation triple
- S'assurer que les seuils de stabilité sont adaptés

**LED RGB ne fonctionne pas**
- Vérifier les pins de connexion (R, G, B)
- Tester avec `StatusLED::testSequence()`
- Contrôler l'alimentation de la LED

**Interface web inaccessible**
- Vérifier la connexion WiFi OPENDOM
- Re-uploader le système de fichiers: `pio run -t uploadfs`
- Contrôler les logs avec `pio device monitor`

## 📄 Licence

Ce projet est développé par **Paluku B** dans le cadre d'un travail académique. 

Le code source est libre et distribué sous licence MIT. Vous êtes libre de l'utiliser, le modifier et le distribuer pour des projets académiques, personnels ou commerciaux.

### Conditions d'utilisation
- ✅ Usage commercial autorisé
- ✅ Modification autorisée  
- ✅ Distribution autorisée
- ✅ Usage privé autorisé
- ⚠️ Attribution requise

## 🎓 Contexte académique

OPENDOM a été développé comme projet de fin d'études en systèmes embarqués et IoT. Il démontre:

- **Intégration hardware/software** complète
- **Programmation ESP32** avancée avec FreeRTOS
- **Interface web moderne** avec PWA
- **Gestion temps réel** des capteurs et actionneurs
- **Architecture modulaire** et extensible
- **Validation rigoureuse** des données capteurs

## 🤝 Contribution

Les contributions sont encouragées! Pour contribuer:

1. **Fork** le projet
2. **Créer** une branche feature (`git checkout -b feature/AmazingFeature`)
3. **Commiter** les changements (`git commit -m 'Add AmazingFeature'`)
4. **Push** vers la branche (`git push origin feature/AmazingFeature`)
5. **Ouvrir** une Pull Request

## 📞 Support

- 📧 **Email**: [palukuba@proton.me](mailto:palukuba@proton.me)
- 🐛 **Issues**: [GitHub Issues](https://github.com/palukuba/opendom-esp32/issues)
- 📖 **Documentation**: Voir ce README et les commentaires dans le code
- 💬 **Discussions**: [GitHub Discussions](https://github.com/palukuba/opendom-esp32/discussions)

---

⭐ **N'hésitez pas à donner une étoile si ce projet vous aide dans vos travaux académiques ou projets IoT!**
>>>>>>> f4568a2 (Premier commit)
