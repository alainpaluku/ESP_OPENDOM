#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

enum class LEDStatus {
  SYSTEM_NORMAL_IDLE,    // Blue - System normal, no active actuators
  SYSTEM_NORMAL_ACTIVE,  // Green - System normal, actuator(s) active
  ALARM_ACTIVE          // Red - Alarm active
};

class StatusLED {
public:
  StatusLED(int redPin, int greenPin, int bluePin);
  
  void init();
  void setStatus(LEDStatus status);
  void update(); // Handle blink effects
  void turnOff();
  
  // Utility methods
  void setColor(int red, int green, int blue);
  void blink(int red, int green, int blue, unsigned long interval = 500);
  void testSequence(); // Power-on self-test sequence
  
private:
  int _redPin;
  int _greenPin;
  int _bluePin;
  
  LEDStatus _currentStatus;
  bool _blinkState;
  unsigned long _lastBlink;
  unsigned long _blinkInterval;
  
  // Predefined colors
  void setRed();
  void setGreen(); 
  void setBlue();
  void setOff();
};

#endif
