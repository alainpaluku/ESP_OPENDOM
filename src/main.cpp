#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>
#include "Config.h"
#include "Sensor.h"
#include "Actuator.h"
#include "StatusLED.h"

// Global objects
WebServer server(80);
DNSServer dnsServer;
Config config;

// Device containers
std::vector<BaseSensor*> sensors;
std::vector<BaseActuator*> actuators;

// Status LED (pins RGB)
StatusLED statusLED(25, 26, 27); // Rouge=25, Vert=26, Bleu=27

// System state
String authToken = "";
String currentUser = "";
unsigned long tokenTimestamp = 0;
const unsigned long TOKEN_TIMEOUT_MS = 24 * 3600 * 1000; // 24 hours
unsigned long lastSensorRead = 0;
const unsigned long sensorReadInterval = 1000;
unsigned long lastDiagnostics = 0;
const unsigned long diagnosticsInterval = 30000; // 30 secondes

// Time synchronization
unsigned long timeOffset = 0; // Offset pour synchroniser avec l'heure client
bool timeSet = false;

// Function prototypes
void initWiFi();
void initSPIFFS();
void initDevices();
void initWebServer();
void handleRoot();
void handleLogin();
void handleAPI();
void handleDevices();
void handleSensorData();
void handleActuatorControl();
void handleConfig();
void handleSystemStats();
void handleTimeSync();
void handleNotFound();
void updateSensors();
void processRules();
void updateStatusLED();
void evaluateRule(const RuleConfig& rule);
bool evaluateConditions(const std::vector<Condition>& conditions, const std::map<String, SensorReading>& readings);
bool evaluateSchedule(const Schedule& schedule);
void executeActions(const std::vector<Action>& actions);
String getContentType(String filename);
bool checkAuthentication();
BaseActuator* findActuatorById(const String& id);
String getActuatorType(const String& id);
void printDiagnostics();
unsigned long getCurrentTime();

// Sensor readings storage
std::map<String, SensorReading> latestReadings;

void setup() {
  Serial.begin(115200);
  Serial.println("OPENDOM System Starting...");
  
  // Initialize SPIFFS
  initSPIFFS();
  
  // Load configuration
  if (!config.loadFromFile("/configuration.json")) {
    Serial.println("Failed to load configuration!");
    return;
  }
  config.printConfig();
  
  // Initialize devices
  initDevices();
  
  // Initialize status LED
  statusLED.init();
  
  // Initialize WiFi
  initWiFi();
  
  // Initialize web server
  initWebServer();
  
  Serial.println("OPENDOM System Ready!");
  Serial.println("Connect to WiFi: " + config.system.wifi.ssid);
  Serial.println("Password: " + config.system.wifi.password);
  Serial.println("Access interface at: http://192.168.4.1");
}

void loop() {
  // Handle DNS requests (captive portal)
  dnsServer.processNextRequest();
  
  // Handle web server requests
  server.handleClient();
  
  // Update sensors
  updateSensors();
  
  // Process automation rules
  processRules();
  
  // Update status LED
  updateStatusLED();
  statusLED.update();
  
  // Print diagnostics periodically
  if (millis() - lastDiagnostics >= diagnosticsInterval) {
    printDiagnostics();
    lastDiagnostics = millis();
  }
  
  // Update actuators (for timed operations)
  for (auto* actuator : actuators) {
    String actuatorType = getActuatorType(actuator->getId());
    
    if (actuatorType == "RELAY") {
      RelayActuator* relay = static_cast<RelayActuator*>(actuator);
      relay->update();
    } else if (actuatorType == "BUZZER") {
      BuzzerActuator* buzzer = static_cast<BuzzerActuator*>(actuator);
      buzzer->update();
    }
  }
  
  delay(10);
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }
  Serial.println("SPIFFS initialized successfully");
}

void initWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config.system.wifi.ssid.c_str(), config.system.wifi.password.c_str());
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("WiFi AP started");
  Serial.print("IP address: ");
  Serial.println(IP);
  
  // Start DNS server for captive portal
  if (config.system.captivePortal) {
    dnsServer.start(53, "*", IP);
    Serial.println("Captive portal DNS started");
  }
}

void printDiagnostics() {
  Serial.println("\n=== OPENDOM DIAGNOSTICS ===");
  Serial.println("System Status:");
  Serial.println("- Uptime: " + String(millis() / 1000) + " seconds");
  Serial.println("- Free heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("- WiFi clients: " + String(WiFi.softAPgetStationNum()));
  
  Serial.println("\nSensor Status:");
  for (auto* sensor : sensors) {
    Serial.println("- " + sensor->getName() + " (Pin " + String(sensor->getPin()) + "):");
    auto it = latestReadings.find(sensor->getId());
    if (it != latestReadings.end()) {
      const SensorReading& reading = it->second;
      Serial.println("  Connected: " + String(reading.isValid ? "YES" : "NO"));
      Serial.println("  Last reading: " + String((millis() - reading.timestamp) / 1000) + "s ago");
    } else {
      Serial.println("  Status: No data available");
    }
  }
  
  Serial.println("\nActuator Status:");
  for (auto* actuator : actuators) {
    Serial.println("- " + actuator->getName() + " (Pin " + String(actuator->getPin()) + "):");
    Serial.println("  State: " + String(actuator->getState() ? "ON" : "OFF"));
  }
  Serial.println("==========================\n");
}

void clearDevices() {
  for (auto* sensor : sensors) {
    delete sensor;
  }
  sensors.clear();

  for (auto* actuator : actuators) {
    delete actuator;
  }
  actuators.clear();
  latestReadings.clear();
}

void initDevices() {
  Serial.println("Initializing devices...");
  clearDevices();
  
  // Initialize sensors
  for (const auto& deviceConfig : config.devices) {
    if (deviceConfig.type == "sensor" && deviceConfig.enabled) {
      BaseSensor* sensor = nullptr;
      
      if (deviceConfig.sensorType == "DHT11") {
        sensor = new DHT11Sensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.sensorType == "MQ2") {
        sensor = new MQ2Sensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.sensorType == "ASC") {
        sensor = new ASCSensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.sensorType == "LDR") {
        sensor = new LDRSensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.sensorType == "PIR") {
        sensor = new PIRSensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.sensorType == "BUTTON") {
        sensor = new ButtonSensor(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      }
      
      if (sensor) {
        sensor->setReadInterval(deviceConfig.readInterval);
        sensor->init();
        sensors.push_back(sensor);
      }
    }
    
    // Initialize actuators
    if (deviceConfig.type == "actuator" && deviceConfig.enabled) {
      BaseActuator* actuator = nullptr;
      
      if (deviceConfig.actuatorType == "RELAY") {
        actuator = new RelayActuator(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      } else if (deviceConfig.actuatorType == "BUZZER") {
        actuator = new BuzzerActuator(deviceConfig.id, deviceConfig.name, deviceConfig.pin);
      }
      
      if (actuator) {
        actuator->init();
        actuator->setState(deviceConfig.state);
        actuators.push_back(actuator);
      }
    }
  }
  
  Serial.println("Devices initialized: " + String(sensors.size()) + " sensors, " + String(actuators.size()) + " actuators");
}

void initWebServer() {
  const char * headerkeys[] = {"X-Auth-Token", "X-Root-Password"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  // Serve static files
  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/api/sensors", HTTP_GET, handleSensorData);
  server.on("/api/actuators", HTTP_POST, handleActuatorControl);
  server.on("/api/config", HTTP_GET, handleConfig);
  server.on("/api/config", HTTP_POST, handleConfig);
  server.on("/api/system", HTTP_GET, handleSystemStats);
  server.on("/api/time", HTTP_POST, handleTimeSync);
  
  // Serve static files
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleLogin() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String username = server.arg("username");
    String password = server.arg("password");
    
    if (username == config.system.auth.username && password == config.system.auth.password) {
      // Generate pseudo-random session token
      authToken = String(random(100000, 999999)) + String(micros()) + String(ESP.getFreeHeap());
      currentUser = username;
      tokenTimestamp = millis();

      server.send(200, "application/json", "{\"success\":true,\"user\":\"" + username + "\",\"token\":\"" + authToken + "\"}");
    } else {
      server.send(401, "application/json", "{\"success\":false,\"error\":\"Invalid credentials\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing credentials\"}");
  }
}

void handleSensorData() {
  if (!checkAuthentication()) return;
  
  JsonDocument doc;
  JsonArray sensorsArray = doc["sensors"].to<JsonArray>();
  
  // Parcourir tous les capteurs configurés
  for (const auto& deviceConfig : config.devices) {
    if (deviceConfig.type == "sensor" && deviceConfig.enabled) {
      JsonObject sensorObj = sensorsArray.add<JsonObject>();
      sensorObj["id"] = deviceConfig.id;
      sensorObj["name"] = deviceConfig.name;
      sensorObj["type"] = deviceConfig.sensorType;
      sensorObj["pin"] = deviceConfig.pin;
      sensorObj["enabled"] = deviceConfig.enabled;
      
      // Chercher les dernières lectures
      auto it = latestReadings.find(deviceConfig.id);
      if (it != latestReadings.end()) {
        const SensorReading& reading = it->second;
        
        sensorObj["connected"] = reading.isValid;
        sensorObj["timestamp"] = reading.timestamp;
        
        // Ajouter les valeurs selon le type de capteur
        if (deviceConfig.sensorType == "DHT11") {
          sensorObj["temperature"] = reading.temperature;
          sensorObj["humidity"] = reading.humidity;
        } else if (deviceConfig.sensorType == "MQ2") {
          sensorObj["gas"] = reading.gas;
        } else if (deviceConfig.sensorType == "ASC") {
          sensorObj["current"] = reading.current;
        } else if (deviceConfig.sensorType == "LDR") {
          sensorObj["light"] = reading.light;
        } else if (deviceConfig.sensorType == "PIR") {
          sensorObj["motion"] = reading.motion;
        } else if (deviceConfig.sensorType == "BUTTON") {
          sensorObj["pressed"] = reading.pressed;
        }
      } else {
        // Pas de données disponibles
        sensorObj["connected"] = false;
        sensorObj["timestamp"] = 0;
        sensorObj["error"] = "No data available";
      }
    }
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleActuatorControl() {
  if (!checkAuthentication()) return;
  
  if (server.hasArg("id") && server.hasArg("action")) {
    String actuatorId = server.arg("id");
    String action = server.arg("action");
    
    for (auto* actuator : actuators) {
      if (actuator->getId() == actuatorId) {
        if (action == "turn_on") {
          actuator->turnOn();
        } else if (action == "turn_off") {
          actuator->turnOff();
        } else if (action == "toggle") {
          actuator->toggle();
        }
        
        server.send(200, "application/json", "{\"success\":true,\"state\":" + String(actuator->getState() ? "true" : "false") + "}");
        return;
      }
    }
    
    server.send(404, "application/json", "{\"success\":false,\"error\":\"Actuator not found\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
  }
}

void handleConfig() {
  if (!checkAuthentication()) return;
  
  if (server.method() == HTTP_GET) {
    // Return current configuration
    File file = SPIFFS.open("/configuration.json", "r");
    if (file) {
      server.streamFile(file, "application/json");
      file.close();
    } else {
      server.send(404, "application/json", "{\"error\":\"Configuration file not found\"}");
    }
  } else if (server.method() == HTTP_POST) {
    // Update configuration (requires root password)
    String rootPassword = "";
    if (server.hasHeader("X-Root-Password")) {
      rootPassword = server.header("X-Root-Password");
    } else if (server.hasArg("root_password")) {
      rootPassword = server.arg("root_password");
    }

    if (rootPassword.isEmpty() || rootPassword != config.system.auth.rootPassword) {
      server.send(401, "application/json", "{\"error\":\"Invalid or missing root password\"}");
      return;
    }
    
    // Save new configuration
    String body = server.arg("plain");
    if (body.isEmpty() || body.length() > 16384) {
      server.send(400, "application/json", "{\"error\":\"Invalid or oversized configuration body\"}");
      return;
    }
    JsonDocument checkDoc;
    DeserializationError err = deserializeJson(checkDoc, body);
    if (err) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
      return;
    }

    File file = SPIFFS.open("/configuration.json", "w");
    if (file) {
      file.print(body);
      file.close();
      
      // Reload configuration
      config.loadFromFile("/configuration.json");
      
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
    }
  }
}



void handleNotFound() {
  String path = server.uri();
  
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    String contentType = getContentType(path);
    server.streamFile(file, contentType);
    file.close();
  } else {
    // Captive portal redirect
    if (config.system.captivePortal) {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "File not found");
    }
  }
}



void updateStatusLED() {
  bool alarmActive = false;
  bool anyActuatorActive = false;
  int activeRelays = 0;
  int activeBuzzers = 0;
  
  // Parcourir tous les actionneurs pour déterminer l'état
  for (auto* actuator : actuators) {
    if (!actuator) continue; // Vérification de sécurité
    
    String actuatorType = getActuatorType(actuator->getId());
    bool isActive = actuator->getState();
    
    if (actuatorType == "BUZZER" && isActive) {
      alarmActive = true;
      activeBuzzers++;
    } else if (actuatorType == "RELAY" && isActive) {
      anyActuatorActive = true;
      activeRelays++;
    }
  }
  
  // Log de debug pour le statut (max une fois par 5 secondes)
  static unsigned long lastStatusLog = 0;
  if (millis() - lastStatusLog > 5000) {
    Serial.println("Status LED: Relays=" + String(activeRelays) + 
                   ", Buzzers=" + String(activeBuzzers) + 
                   ", Alarm=" + String(alarmActive ? "YES" : "NO"));
    lastStatusLog = millis();
  }
  
  // Priorité : Alarme > Actionneur actif > Veille
  if (alarmActive) {
    statusLED.setStatus(LEDStatus::ALARM_ACTIVE);
  } else if (anyActuatorActive) {
    statusLED.setStatus(LEDStatus::SYSTEM_NORMAL_ACTIVE);
  } else {
    statusLED.setStatus(LEDStatus::SYSTEM_NORMAL_IDLE);
  }
}

void processRules() {
  for (const auto& rule : config.rules) {
    if (rule.enabled) {
      evaluateRule(rule);
    }
  }
}

void evaluateRule(const RuleConfig& rule) {
  bool shouldActivate = false;
  
  if (rule.triggerType == "sensor_threshold" || 
      rule.triggerType == "sensor_combination" || 
      rule.triggerType == "critical_event") {
    shouldActivate = evaluateConditions(rule.conditions, latestReadings);
  } else if (rule.triggerType == "schedule") {
    shouldActivate = evaluateSchedule(rule.schedule);
  }
  
  // Debug log pour voir les déclenchements
  static unsigned long lastRuleLog = 0;
  if (millis() - lastRuleLog > 5000) { // Log toutes les 5 secondes
    Serial.println("Rule: " + rule.name + " -> " + (shouldActivate ? "ACTIVE" : "inactive"));
    lastRuleLog = millis();
  }
  
  if (shouldActivate) {
    Serial.println("EXECUTING RULE: " + rule.name);
    executeActions(rule.actions);
  } else if (!rule.deactivationConditions.empty()) {
    bool shouldDeactivate = evaluateConditions(rule.deactivationConditions, latestReadings);
    if (shouldDeactivate) {
      Serial.println("DEACTIVATING RULE: " + rule.name);
      for (const auto& action : rule.actions) {
        for (auto* actuator : actuators) {
          if (actuator->getId() == action.actuatorId) {
            actuator->turnOff();
          }
        }
      }
    }
  }
}

bool evaluateConditions(const std::vector<Condition>& conditions, const std::map<String, SensorReading>& readings) {
  if (conditions.empty()) return false;
  
  bool result = false; // Commencer par false, pas true
  String lastLogic = "AND";
  bool firstCondition = true;
  bool hasValidCondition = false; // Track si on a au moins une condition valide
  
  for (const auto& condition : conditions) {
    auto it = readings.find(condition.sensorId);
    if (it == readings.end() || !it->second.isValid) {
      Serial.println("Condition SKIP: " + condition.sensorId + " (no valid data)");
      continue;
    }
    hasValidCondition = true;
    
    const SensorReading& reading = it->second;
    bool conditionResult = false;
    
    float sensorValue = 0;
    if (condition.parameter == "temperature") sensorValue = reading.temperature;
    else if (condition.parameter == "humidity") sensorValue = reading.humidity;
    else if (condition.parameter == "gas") sensorValue = reading.gas;
    else if (condition.parameter == "current") sensorValue = reading.current;
    else if (condition.parameter == "light") sensorValue = reading.light;
    else if (condition.parameter == "motion") sensorValue = reading.motion ? 1 : 0;
    else if (condition.parameter == "pressed") sensorValue = reading.pressed ? 1 : 0;
    
   
    // Les capteurs déconnectés retournent des valeurs par défaut sécurisées
    
    // Debug log pour voir les valeurs des capteurs
    Serial.println("Condition: " + condition.sensorId + "." + condition.parameter + 
                   " " + condition.operator_ + " " + String(condition.value) + 
                   " (current: " + String(sensorValue) + ")");
    
    if (condition.operator_ == ">") conditionResult = sensorValue > condition.value;
    else if (condition.operator_ == "<") conditionResult = sensorValue < condition.value;
    else if (condition.operator_ == "==") {
      // Pour les capteurs booléens (motion, pressed), comparaison exacte
      if (condition.parameter == "motion" || condition.parameter == "pressed") {
        conditionResult = (sensorValue == condition.value);
      } else {
        // Pour les autres capteurs, tolérance de 10%
        float tolerance = max(10.0f, condition.value * 0.1f);
        conditionResult = abs(sensorValue - condition.value) < tolerance;
      }
    }
    else if (condition.operator_ == ">=") conditionResult = sensorValue >= condition.value;
    else if (condition.operator_ == "<=") conditionResult = sensorValue <= condition.value;
    

    
    if (firstCondition) {
      result = conditionResult;
      firstCondition = false;
    } else {
      if (lastLogic == "AND") {
        result = result && conditionResult;
      } else if (lastLogic == "OR") {
        result = result || conditionResult;
      }
    }
    
    lastLogic = condition.logic.isEmpty() ? "AND" : condition.logic;
  }
  
  // Si aucune condition valide, retourner false
  if (!hasValidCondition) {
    Serial.println("No valid conditions found - returning false");
    return false;
  }
  
  return result;
}

bool evaluateSchedule(const Schedule& schedule) {
  if (!timeSet || schedule.startTime.isEmpty() || schedule.endTime.isEmpty()) {
    return false;
  }
  
  unsigned long currentTimeMs = getCurrentTime();
  unsigned long currentSeconds = (currentTimeMs / 1000) % 86400; // Secondes dans la journée
  
  // Parser start_time et end_time (format HH:MM)
  int startHour = schedule.startTime.substring(0, 2).toInt();
  int startMin = schedule.startTime.substring(3, 5).toInt();
  int endHour = schedule.endTime.substring(0, 2).toInt();
  int endMin = schedule.endTime.substring(3, 5).toInt();
  
  unsigned long startSeconds = startHour * 3600 + startMin * 60;
  unsigned long endSeconds = endHour * 3600 + endMin * 60;
  
  return (currentSeconds >= startSeconds && currentSeconds <= endSeconds);
}

void executeActions(const std::vector<Action>& actions) {
  static unsigned long lastActionTime = 0;
  const unsigned long actionCooldown = 2000; // 2 secondes entre les actions
  
  // Anti-rebond pour éviter les actions répétées
  if (millis() - lastActionTime < actionCooldown) {
    return;
  }
  
  for (const auto& action : actions) {
    for (auto* actuator : actuators) {
      if (actuator->getId() == action.actuatorId) {
        if (action.action == "turn_on") {
          Serial.println("TURNING ON: " + actuator->getName());
          actuator->turnOn();
          lastActionTime = millis();
          
          if (action.duration > 0) {
            String actuatorType = getActuatorType(actuator->getId());
            if (actuatorType == "RELAY") {
              RelayActuator* relay = static_cast<RelayActuator*>(actuator);
              relay->setDuration(action.duration);
            }
          }
          
          if (!action.pattern.isEmpty()) {
            String actuatorType = getActuatorType(actuator->getId());
            if (actuatorType == "BUZZER") {
              BuzzerActuator* buzzer = static_cast<BuzzerActuator*>(actuator);
              buzzer->setPattern(action.pattern);
              if (action.duration > 0) {
                buzzer->setDuration(action.duration);
              }
            }
          }
        } else if (action.action == "turn_off") {
          actuator->turnOff();
        } else if (action.action == "toggle") {
          actuator->toggle();
        }
        break;
      }
    }
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

bool checkAuthentication() {
  String reqToken = "";
  if (server.hasHeader("X-Auth-Token")) {
    reqToken = server.header("X-Auth-Token");
  } else if (server.hasArg("token")) {
    reqToken = server.arg("token");
  }

  if (authToken.isEmpty() || reqToken.isEmpty() || reqToken != authToken) {
    server.send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return false;
  }

  // Token timeout check
  if (millis() - tokenTimestamp > TOKEN_TIMEOUT_MS) {
    authToken = "";
    server.send(401, "application/json", "{\"error\":\"Session expired\"}");
    return false;
  }

  return true;
}

void updateSensors() {
  if (millis() - lastSensorRead < sensorReadInterval) {
    return;
  }
  
  for (auto* sensor : sensors) {
    if (sensor->isReady()) {
      SensorReading reading = sensor->read();
      
      if (reading.isValid) {
        latestReadings[sensor->getId()] = reading;
      } else {
        if (latestReadings.find(sensor->getId()) == latestReadings.end()) {
          SensorReading defaultReading;
          defaultReading.sensorId = sensor->getId();
          defaultReading.type = reading.type;
          defaultReading.temperature = 20.0;
          defaultReading.humidity = 50.0;
          defaultReading.gas = 0.0;
          defaultReading.current = 0.0;
          defaultReading.light = 500.0;
          defaultReading.motion = false;
          defaultReading.pressed = false;
          defaultReading.isValid = true; // Permettre l'utilisation des valeurs par défaut
          defaultReading.timestamp = millis();
          
          latestReadings[sensor->getId()] = defaultReading;
        }
      }
    }
  }
  
  lastSensorRead = millis();
}

BaseActuator* findActuatorById(const String& id) {
  for (auto* actuator : actuators) {
    if (actuator->getId() == id) {
      return actuator;
    }
  }
  return nullptr;
}

String getActuatorType(const String& id) {
  for (const auto& deviceConfig : config.devices) {
    if (deviceConfig.id == id && deviceConfig.type == "actuator") {
      return deviceConfig.actuatorType;
    }
  }
  return "";
}

void handleTimeSync() {
  if (!checkAuthentication()) return;
  
  if (server.hasArg("timestamp")) {
    unsigned long clientTime = server.arg("timestamp").toInt();
    timeOffset = clientTime - (millis() / 1000);
    timeSet = true;
    
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Time synchronized\"}");
    Serial.println("Time synchronized with client. Offset: " + String(timeOffset));
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing timestamp\"}");
  }
}

void handleSystemStats() {
  if (!checkAuthentication()) return;
  
  JsonDocument doc;
  doc["freeHeap"] = ESP.getFreeHeap();

  doc["wifiClients"] = WiFi.softAPgetStationNum();
  doc["timeSet"] = timeSet;
  doc["cpuTemp"] = temperatureRead(); // Température CPU de l'ESP32
  doc["totalHeap"] = ESP.getHeapSize();
  doc["minFreeHeap"] = ESP.getMinFreeHeap();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["chipModel"] = ESP.getChipModel();
  doc["flashSize"] = ESP.getFlashChipSize();
  
  if (timeSet) {
    doc["currentTime"] = getCurrentTime();
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

unsigned long getCurrentTime() {
  if (!timeSet) return 0;
  return (millis() / 1000) + timeOffset;
}
