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
  reading.isValid = false; // Default invalid
  
  if (_dht) {
    // Wait at least 2 seconds between DHT11 readings
    static unsigned long lastDHTRead = 0;
    if (millis() - lastDHTRead < 2000) {
      reading.isValid = false;
      return reading;
    }
    
    // First read attempt
    float temp1 = _dht->readTemperature();
    float hum1 = _dht->readHumidity();
    
    if (isnan(temp1) || isnan(hum1)) {
      Serial.println("DHT11 Sensor " + _id + ": First read failed - trying recovery");
      
      // Delay and second attempt
      delay(250);
      float temp2 = _dht->readTemperature();
      float hum2 = _dht->readHumidity();
      
      if (isnan(temp2) || isnan(hum2)) {
        Serial.println("DHT11 Sensor " + _id + ": Sensor disconnected or failing");
        reading.isValid = false;
        lastDHTRead = millis();
        return reading;
      } else {
        reading.temperature = temp2;
        reading.humidity = hum2;
        Serial.println("DHT11 Sensor " + _id + ": Recovery successful");
      }
    } else {
      reading.temperature = temp1;
      reading.humidity = hum1;
    }
    
    // Validation of realistic ranges
    if (reading.temperature >= -40 && reading.temperature <= 80 &&
        reading.humidity >= 0 && reading.humidity <= 100) {
      reading.isValid = true;
    } else {
      reading.isValid = false;
    }
    
    lastDHTRead = millis();
  } else {
    Serial.println("DHT11 Sensor " + _id + ": Uninitialized");
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
  
  // Multiple readings for stability (5 samples)
  int readings[5];
  for (int i = 0; i < 5; i++) {
    readings[i] = analogRead(_pin);
    delay(1);
  }
  
  // Calculate average excluding extreme values
  int sum = 0;
  int minVal = readings[0], maxVal = readings[0];
  for (int i = 0; i < 5; i++) {
    if (readings[i] < minVal) minVal = readings[i];
    if (readings[i] > maxVal) maxVal = readings[i];
    sum += readings[i];
  }
  int avgValue = (sum - minVal - maxVal) / 3;
  
  // Check if sensor is connected
  bool isConnected = (avgValue > 50) &&          // Minimum threshold for MQ2 gas sensor
                     (avgValue < 4080) &&        // Not saturated
                     ((maxVal - minVal) < 100);  // Stability check
  
  if (!isConnected) {
    Serial.println("MQ2 Sensor " + _id + ": Sensor disconnected or unstable");
    reading.isValid = false;
  } else {
    // Approximate conversion to ppm
    reading.gas = map(avgValue, 50, 4095, 0, 1000);
    
    // Clamp values
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
  
  // Multiple readings for stability
  int rawValue1 = analogRead(_pin);
  delay(2);
  int rawValue2 = analogRead(_pin);
  delay(2);
  int rawValue3 = analogRead(_pin);
  
  // Check stability
  int maxDiff = max(max(abs(rawValue1 - rawValue2), abs(rawValue2 - rawValue3)), abs(rawValue1 - rawValue3));
  
  // Check if sensor is connected
  bool isConnected = (maxDiff < 50) &&           // Stable readings
                     (rawValue1 > 10) &&         // Not zero
                     (rawValue1 < 4080) &&       // Not saturated
                     (rawValue2 > 10) && 
                     (rawValue2 < 4080);
  
  if (!isConnected) {
    Serial.println("ASC Sensor " + _id + ": Sensor disconnected - no data");
    reading.isValid = false;
  } else {
    // Average of 3 readings
    int avgValue = (rawValue1 + rawValue2 + rawValue3) / 3;
    float voltage = (avgValue / 4095.0) * _voltage;
    
    // Calculate current with negative value protection
    float currentCalc = (voltage - (_voltage / 2.0)) / _sensitivity;
    
    // Prevent negative readings
    reading.current = (currentCalc < 0.0) ? 0.0 : currentCalc;
    
    // Cap at maximum expected current (e.g. 30A)
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
  
  // Multiple readings for stability
  int rawValue1 = analogRead(_pin);
  delay(2);
  int rawValue2 = analogRead(_pin);
  delay(2);
  int rawValue3 = analogRead(_pin);
  
  // Check stability
  int maxDiff = max(max(abs(rawValue1 - rawValue2), abs(rawValue2 - rawValue3)), abs(rawValue1 - rawValue3));
  
  // Check if LDR sensor is connected
  bool isConnected = (maxDiff < 100) &&          // Reasonably stable
                     (rawValue1 >= 0) &&         // Acceptable min
                     (rawValue1 <= 4095) &&      // Within ADC range
                     (rawValue2 >= 0) && 
                     (rawValue2 <= 4095) &&
                     (rawValue3 >= 0) && 
                     (rawValue3 <= 4095) &&
                     ((rawValue1 + rawValue2 + rawValue3) > 0);
  
  if (!isConnected) {
    Serial.println("LDR Sensor " + _id + ": Sensor disconnected - no data");
    reading.isValid = false;
  } else {
    // Average of 3 readings
    int avgValue = (rawValue1 + rawValue2 + rawValue3) / 3;
    
    // Approximate lux conversion for pull-up LDR
    reading.light = 1000.0 - (float(avgValue) / 4095.0) * 1000.0;
    
    // Clamp within bounds
    if (reading.light < 0) reading.light = 0;
    if (reading.light > 1000) reading.light = 1000;
    
    reading.isValid = true;
    
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
  reading.isValid = true; // Digital sensors are always valid
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
    reading.pressed = !currentState; // Inverted due to INPUT_PULLUP
    _lastState = currentState;
  } else {
    reading.pressed = false;
  }
  
  reading.isValid = true; // Digital sensors are always valid
  
  _lastRead = millis();
  return reading;
}
