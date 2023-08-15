#pragma once

#include <Arduino.h>
#include "states.h"
#include "pinconfig.h"
#include "serial.h"

#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz
// default voltage levels
#define STANDBY 12
#define DETECTED 9
#define CHARGING 6
#define VENT 3
#define HYST 1.25

extern bool DEBUG;
extern byte enc;
extern uint16_t ladeleistung;
extern bool automatik;
extern bool pwm_active;
extern byte error_code;
static uint16_t ladeleistungen[8] = {0, 1200, 1600, 2000, 2400, 2800, 3200, 3400};
static byte LEDs[] = {LED12, LED16, LED20, LED24, LED28, LED32, LED36};

void dbg(const char *string);
void dbgln(const char *string);
void pin_init();
byte read_encoder();
byte calc_encoder(uint16_t wert);
void set_pwm(uint16_t leistung);
void toggle_LED(byte index);
bool check_CP(float checkwert);
bool diode_fail();
void init_splash();
void update();
void ALL_LEDs_ON(void);
void ALL_LEDs_OFF(void);
float adc2float(uint16_t val);
void set_ladeleistung(uint16_t val);

#define DIODE_CHECK if(diode_fail()) { error_code = 2; error.set(); return; }

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

  int16_t mittelwert();
  float spannung();
  void clear() {
    for(byte i = 0; i < BUFFSIZE; i++) messwerte[i] = 0;
  }
};

extern pegel high, low;