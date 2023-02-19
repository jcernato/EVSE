#pragma once

#include <Arduino.h>
#include "pinconfig.h"
#include "custom_classes.h"

#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz

void toggle_LED(byte index) {
    if(index < 0 || index > 7) {
        Serial.print("Encoder ERROR");
        error.set();
        return;
    }
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], LOW);
    if(index > 0) digitalWrite(LEDs[index-1], HIGH);
}


// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if(leistung == 0) return;
  if (leistung < 1200) {
    Serial.println("PWM value error!");
    error.set();
    return;
  }
  byte value = map(leistung, 0, 3600, 0, int(PWM_LPER*0.27));
  if (leistung > 3680) {
    Serial.println("PWM value error!");
    error.set();
    return;
  }
  analogWrite(PWM, PWM_LPER - value);  
}

#define STANDBY 12
#define DETECTED 9
#define CHARGING 6
#define VENT 3

bool check_CP(float messwert, float checkwert) {
  #define HYST 1.0
  if((messwert > checkwert - HYST) && messwert < checkwert + HYST) return true;
  else return false;
}
bool diode_fail(float messwert) {
  if(messwert > -4) {
    Serial.println("Diode check failed");
    return true;
  }
  else return false;
}
#define DIODE_CHECK if(diode_fail(lvolts)) { error.set(); return; }
#define BOGUS { high.error_counter++; }
#define PWM_HIGH digitalWrite(PWM, LOW);
#define PWM_LOW digitalWrite(PWM, HIGH);
byte lauf = 0;

// ################## STANDBY ####################
  void _standby::run() {
    // nur hvolts werden gemessen
    low.error_counter = 0;
    update();

    toggle_LED(enc);
    if(enc == 0) return;

    if(check_CP(hvolts, STANDBY));
    else if(hvolts == -1) return;
    else if(check_CP(hvolts, DETECTED)) detected.set();
    else BOGUS;
  }

  void _standby::set() {
    machine_state = &standby;
    Serial.println("Standby");
    digitalWrite(RELAIS, LOW);
    PWM_HIGH
    low.clear();
    while(digitalRead(RESET) == 0) delay(50);
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
    lauf = 0;
  }

// ################# DETECTED #######################
void _detected::set() {
  machine_state = &detected;
  Serial.println("Detected");
  digitalWrite(RELAIS, LOW);
  if(enc != 0) set_pwm(ladeleistungen[enc]);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
  lauf = 0;
}

void _detected::run() {
  if(!update()) return;

  DIODE_CHECK

  if(enc == 0) { standby.set(); return; }
  toggle_LED(enc);
  set_pwm(ladeleistungen[enc]);
  
  if(check_CP(hvolts, DETECTED)) return;
  else if(check_CP(hvolts, STANDBY)) standby.set();
  else if(check_CP(hvolts, CHARGING)) charging.set();
  else BOGUS
}

// ################## CHARGING #####################
void _charging::set() {
  machine_state = &charging;
  Serial.println("Charging");
  digitalWrite(RELAIS, HIGH);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
  lauf = 0;
}

void _charging::run() {
  if(!update()) return;

  DIODE_CHECK

  if(enc == 0) { standby.set(); return; }
  toggle_LED(enc);
  set_pwm(ladeleistungen[enc]);
  
  if(check_CP(hvolts, CHARGING)) return;
  else if(check_CP(hvolts, STANDBY)) standby.set();
  else if(check_CP(hvolts, DETECTED)) detected.set();
  else BOGUS
}

// ################### ERRROR #######################
  void _error::set() {
    machine_state = &error;
    Serial.println("Error");
    digitalWrite(RELAIS, LOW);
    PWM_LOW
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
    while(digitalRead(RESET) == 0) delay(50);
    lauf = 0;
    
  }
  void _error::run() {
    if(digitalRead(RESET) == 0) {
      Serial.println("Error cleared");
      high.error_counter = 0;
      low.error_counter = 0;
      standby.set();
    }
  }

