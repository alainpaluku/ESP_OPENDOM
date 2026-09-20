#include "StatusLED.h"

StatusLED::StatusLED(int redPin, int greenPin, int bluePin) 
  : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin),
    _currentStatus(LEDStatus::SYSTEM_NORMAL_IDLE), _blinkState(false), 
    _lastBlink(0), _blinkInterval(500) {}

void StatusLED::init() {
  pinMode(_redPin, OUTPUT);
  pinMode(_greenPin, OUTPUT);
  pinMode(_bluePin, OUTPUT);
  
  // Power-on self-test sequence
  testSequence();
  
  Serial.println("Status LED initialized on pins R:" + String(_redPin) + 
                 " G:" + String(_greenPin) + " B:" + String(_bluePin));
}

void StatusLED::setStatus(LEDStatus status) {
  if (_currentStatus != status) {
    _currentStatus = status;
    
    switch (status) {
      case LEDStatus::SYSTEM_NORMAL_IDLE:
        Serial.println("Status LED: BLUE - System normal, no actuators active");
        setBlue();
        break;
        
      case LEDStatus::SYSTEM_NORMAL_ACTIVE:
        Serial.println("Status LED: GREEN - System normal, actuator(s) active");
        setGreen();
        break;
        
      case LEDStatus::ALARM_ACTIVE:
        Serial.println("Status LED: RED - ALARM ACTIVE");
        setRed();
        break;
    }
  }
}

void StatusLED::update() {
  // Blinking effect for alarm
  if (_currentStatus == LEDStatus::ALARM_ACTIVE) {
    if (millis() - _lastBlink >= _blinkInterval) {
      _blinkState = !_blinkState;
      _lastBlink = millis();
      
      if (_blinkState) {
        setRed();
      } else {
        setOff();
      }
    }
  }
}

void StatusLED::turnOff() {
  setOff();
}

void StatusLED::setColor(int red, int green, int blue) {
  // PWM output for color intensity
  analogWrite(_redPin, red);
  analogWrite(_greenPin, green);
  analogWrite(_bluePin, blue);
}

void StatusLED::blink(int red, int green, int blue, unsigned long interval) {
  _blinkInterval = interval;
  
  if (millis() - _lastBlink >= _blinkInterval) {
    _blinkState = !_blinkState;
    _lastBlink = millis();
    
    if (_blinkState) {
      setColor(red, green, blue);
    } else {
      setOff();
    }
  }
}

void StatusLED::setRed() {
  setColor(255, 0, 0);
}

void StatusLED::setGreen() {
  setColor(0, 255, 0);
}

void StatusLED::setBlue() {
  setColor(0, 0, 255);
}

void StatusLED::setOff() {
  setColor(0, 0, 0);
}

void StatusLED::testSequence() {
  // Test sequence: Red -> Green -> Blue -> White -> Initial Blue (Idle)
  Serial.println("Status LED: Test sequence starting...");
  
  setRed();
  delay(200);
  setGreen();
  delay(200);
  setBlue();
  delay(200);
  setColor(255, 255, 255); // White
  delay(200);
  setBlue(); // Idle state
  
  Serial.println("Status LED: Test sequence completed");
}
