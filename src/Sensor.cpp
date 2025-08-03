#include "Sensor.h"

// BaseSensor Implementation
BaseSensor::BaseSensor(String id, String name, int pin) 
  : _id(id), _name(name), _pin(pin), _lastRead(0), _readInterval(1000) {}

bool BaseSensor::isReady() {
  return (millis() - _lastRead) >= _readInterval;
}

// DHT11Sensor Implementation
DHT11Sensor::DHT11Sensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin), _dht(nullptr) {}

void DHT11Sensor::init() {
  _dht = new DHT(_pin, DHT11);
  _dht->begin();
  Serial.println("DHT11 sensor initialized on pin " + String(_pin));
}

SensorReading DHT11Sensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "DHT11";
  reading.timestamp = millis();
  reading.isValid = false; // Par défaut invalide
  
  if (_dht) {
    // Attendre au moins 2 secondes entre les lectures pour DHT11
    static unsigned long lastDHTRead = 0;
    if (millis() - lastDHTRead < 2000) {
      reading.isValid = false;
      return reading;
    }
    
    // Première tentative de lecture
    float temp1 = _dht->readTemperature();
    float hum1 = _dht->readHumidity();
    
    if (isnan(temp1) || isnan(hum1)) {
      Serial.println("DHT11 Sensor " + _id + ": First read failed - trying recovery");
      
      // Délai et seconde tentative
      delay(250);
      float temp2 = _dht->readTemperature();
      float hum2 = _dht->readHumidity();
      
      if (isnan(temp2) || isnan(hum2)) {
        Serial.println("DHT11 Sensor " + _id + ": Capteur déconnecté ou défaillant");
        reading.isValid = false;
        lastDHTRead = millis();
        return reading;
      } else {
        reading.temperature = temp2;
        reading.humidity = hum2;
        Serial.println("DHT11 Sensor " + _id + ": Récupération réussie");
      }
    } else {
      reading.temperature = temp1;
      reading.humidity = hum1;
    }
    
    // Validation des plages réalistes
    if (reading.temperature >= -40 && reading.temperature <= 80 &&
        reading.humidity >= 0 && reading.humidity <= 100) {
      reading.isValid = true;
    } else {
      reading.isValid = false;
    }
    
    lastDHTRead = millis();
  } else {
    Serial.println("DHT11 Sensor " + _id + ": Non initialisé");
    reading.isValid = false;
  }
  
  _lastRead = millis();
  return reading;
}

// MQ2Sensor Implementation
MQ2Sensor::MQ2Sensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin) {}

void MQ2Sensor::init() {
  pinMode(_pin, INPUT);
  Serial.println("MQ2 sensor initialized on pin " + String(_pin));
}

SensorReading MQ2Sensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "MQ2";
  reading.timestamp = millis();
  
  // Lectures multiples pour stabilité (5 lectures)
  int readings[5];
  for (int i = 0; i < 5; i++) {
    readings[i] = analogRead(_pin);
    delay(1);
  }
  
  // Calcul de la moyenne en excluant les valeurs extrêmes
  int sum = 0;
  int minVal = readings[0], maxVal = readings[0];
  for (int i = 0; i < 5; i++) {
    if (readings[i] < minVal) minVal = readings[i];
    if (readings[i] > maxVal) maxVal = readings[i];
    sum += readings[i];
  }
  int avgValue = (sum - minVal - maxVal) / 3;
  
  // Vérification si le capteur est connecté
  bool isConnected = (avgValue > 50) &&          // Seuil minimum pour MQ2 (capteur de gaz)
                     (avgValue < 4080) &&        // Pas saturé
                     ((maxVal - minVal) < 100);  // Stabilité des lectures
  
  if (!isConnected) {
    Serial.println("MQ2 Sensor " + _id + ": Capteur déconnecté ou instable");
    reading.isValid = false;
  } else {
    // Conversion en ppm (approximative)
    reading.gas = map(avgValue, 50, 4095, 0, 1000);
    
    // Assurer que la valeur est dans les limites
    if (reading.gas < 0) reading.gas = 0;
    if (reading.gas > 1000) reading.gas = 1000;
    
    reading.isValid = true;
  }
  
  _lastRead = millis();
  return reading;
}

// ASCSensor Implementation
ASCSensor::ASCSensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin), _sensitivity(0.1), _voltage(3.3) {}

void ASCSensor::init() {
  pinMode(_pin, INPUT);
  Serial.println("ASC sensor initialized on pin " + String(_pin));
}

SensorReading ASCSensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "ASC";
  reading.timestamp = millis();
  
  // Lectures multiples pour stabilité
  int rawValue1 = analogRead(_pin);
  delay(2);
  int rawValue2 = analogRead(_pin);
  delay(2);
  int rawValue3 = analogRead(_pin);
  
  // Vérification de stabilité des lectures
  int maxDiff = max(max(abs(rawValue1 - rawValue2), abs(rawValue2 - rawValue3)), abs(rawValue1 - rawValue3));
  
  // Vérification si le capteur est connecté
  bool isConnected = (maxDiff < 50) &&           // Valeurs stables
                     (rawValue1 > 10) &&         // Pas à zéro
                     (rawValue1 < 4080) &&       // Pas saturé
                     (rawValue2 > 10) && 
                     (rawValue2 < 4080);
  
  if (!isConnected) {
    Serial.println("ASC Sensor " + _id + ": Capteur déconnecté - pas de données");
    reading.isValid = false;
  } else {
    // Utiliser la moyenne des 3 lectures
    int avgValue = (rawValue1 + rawValue2 + rawValue3) / 3;
    float voltage = (avgValue / 4095.0) * _voltage;
    
    // Calcul du courant avec protection contre les valeurs négatives
    float currentCalc = (voltage - (_voltage / 2.0)) / _sensitivity;
    
    // JAMAIS de valeur négative - forcer à zéro minimum
    reading.current = (currentCalc < 0.0) ? 0.0 : currentCalc;
    
    // Limiter à une valeur maximale raisonnable (ex: 30A)
    if (reading.current > 30.0) reading.current = 30.0;
    
    reading.isValid = true;
  }
  
  _lastRead = millis();
  return reading;
}

// LDRSensor Implementation
LDRSensor::LDRSensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin) {}

void LDRSensor::init() {
  pinMode(_pin, INPUT);
  Serial.println("LDR sensor initialized on pin " + String(_pin));
}

SensorReading LDRSensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "LDR";
  reading.timestamp = millis();
  
  // Lectures multiples pour stabilité
  int rawValue1 = analogRead(_pin);
  delay(2);
  int rawValue2 = analogRead(_pin);
  delay(2);
  int rawValue3 = analogRead(_pin);
  
  // Vérification de stabilité des lectures
  int maxDiff = max(max(abs(rawValue1 - rawValue2), abs(rawValue2 - rawValue3)), abs(rawValue1 - rawValue3));
  
  // Vérification si le capteur LDR est connecté
  // Pour un LDR, les valeurs peuvent aller de très bas (obscurité) à très haut (forte lumière)
  bool isConnected = (maxDiff < 100) &&          // Valeurs relativement stables
                     (rawValue1 >= 0) &&         // Valeur minimum acceptable
                     (rawValue1 <= 4095) &&      // Dans la plage ADC
                     (rawValue2 >= 0) && 
                     (rawValue2 <= 4095) &&
                     (rawValue3 >= 0) && 
                     (rawValue3 <= 4095) &&
                     // Au moins une lecture non nulle (éviter les courts-circuits)
                     ((rawValue1 + rawValue2 + rawValue3) > 0);
  
  if (!isConnected) {
    Serial.println("LDR Sensor " + _id + ": Capteur déconnecté - pas de données");
    reading.isValid = false;
  } else {
    // Utiliser la moyenne des 3 lectures
    int avgValue = (rawValue1 + rawValue2 + rawValue3) / 3;
    
    // Conversion en lux approximative pour un LDR en pull-up
    // Plus la valeur ADC est faible, plus il y a de lumière (logique inversée)
    // Formule : lux = 1000 - (ADC / 4095) * 1000 (approximation 0-1000 lux)
    reading.light = 1000.0 - (float(avgValue) / 4095.0) * 1000.0;
    
    // Assurer que la valeur est dans la plage correcte
    if (reading.light < 0) reading.light = 0;
    if (reading.light > 1000) reading.light = 1000;
    
    reading.isValid = true;
    
    // Log de debug pour LDR (une fois par seconde max)
    static unsigned long lastLDRLog = 0;
    if (millis() - lastLDRLog > 1000) {
      Serial.println("LDR " + _id + ": " + String(reading.light) + " lux (ADC: " + String(avgValue) + ")");
      lastLDRLog = millis();
    }
  }
  
  _lastRead = millis();
  return reading;
}

// PIRSensor Implementation
PIRSensor::PIRSensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin), _lastState(false) {}

void PIRSensor::init() {
  pinMode(_pin, INPUT);
  _lastState = digitalRead(_pin);
  Serial.println("PIR sensor initialized on pin " + String(_pin));
}

SensorReading PIRSensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "PIR";
  reading.timestamp = millis();
  
  bool currentState = digitalRead(_pin);
  reading.motion = currentState;
  reading.isValid = true; // Les capteurs digitaux sont toujours valides
  _lastState = currentState;
  
  _lastRead = millis();
  return reading;
}

// ButtonSensor Implementation
ButtonSensor::ButtonSensor(String id, String name, int pin) 
  : BaseSensor(id, name, pin), _lastState(true), _debounceTime(50), _lastDebounceTime(0) {}

void ButtonSensor::init() {
  pinMode(_pin, INPUT_PULLUP);
  _lastState = digitalRead(_pin);
  Serial.println("Button sensor initialized on pin " + String(_pin));
}

SensorReading ButtonSensor::read() {
  SensorReading reading;
  reading.sensorId = _id;
  reading.type = "BUTTON";
  reading.timestamp = millis();
  
  bool currentState = digitalRead(_pin);
  
  if (currentState != _lastState) {
    _lastDebounceTime = millis();
  }
  
  if ((millis() - _lastDebounceTime) > _debounceTime) {
    reading.pressed = !currentState; // Inverted because of INPUT_PULLUP
    _lastState = currentState;
  } else {
    reading.pressed = false;
  }
  
  reading.isValid = true; // Les capteurs digitaux sont toujours valides
  
  _lastRead = millis();
  return reading;
}
