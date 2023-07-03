#pragma once

#include <Arduino.h>
#include "pinconfig.h"
#include "serial.h"
#include "states.h"

#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz
// default voltage levels
#define STANDBY 12
#define DETECTED 9
#define CHARGING 6
#define VENT 3
#define HYST 1.0

static byte enc = 0;
static uint16_t ladeleistung = 0;
static bool automatik = 0;
static  bool pwm_active = 0;
static uint16_t ladeleistungen[8] = {0, 1200, 1600, 2000, 2400, 2800, 3200, 3400};
static byte LEDs[] = {LED12, LED16, LED20, LED24, LED28, LED32, LED36};

void pin_init();
byte read_encoder();
void set_pwm(uint16_t leistung);
void toggle_LED(byte index);
bool check_CP(float checkwert);
bool diode_fail();
void init_splash();

#define DIODE_CHECK if(diode_fail()) { error.set(); return; }

#define INTERRUPTS_ON TCA0_SPLIT_INTCTRL = 0b00010001
#define INTERRUPTS_OFF TCA0_SPLIT_INTCTRL = 0b00000000

// ========================================= //
// ================ PEGEL ================== //
// ========================================= //
#define TOLERANZ 25
#define BUFFSIZE 10

class pegel {
private:
  float old_value;
public:
  byte error_counter = 0;
  int16_t messwerte[BUFFSIZE];
  byte index;

  int16_t mittelwert()  {
    int16_t summe = 0;
    int16_t min = 1023;
    int16_t max = 0;
    for(byte i = 0; i < BUFFSIZE; i++) {
      summe = summe + messwerte[i];
      if (messwerte[i] > max) max = messwerte[i];
      if (messwerte[i] < min) min = messwerte[i];
    }

    if(abs(max - min) > TOLERANZ || summe < 10) { // Entweder zu hohe Spreizung der Messwerte (hohes Rauschen oder in Flanke gemessen), oder Wert zu gering bzw keine Messung erfolgt (kein pwm)
      error_counter++;
      if(error_counter < 5) {
        return old_value;
      } else {
        return 0;
      }
    } 
    
    error_counter = 0;
    old_value = (summe - min - max) / 8;
    return old_value;
  
  }

  float spannung() { 
    float wert = 0;
    if(pwm_active) {
      wert = mittelwert();
    } else {
      wert = analogRead(CPRead);
    }

    if(wert == 0) {
      return 0;
    }
    return (wert - 350) / 50;
  }

  void clear() {
    for(byte i = 0; i < BUFFSIZE; i++) messwerte[i] = 0;
  }
};

static pegel high, low;