#pragma once

#include <Arduino.h>
#include "pinconfig.h"


byte read_encoder() {
  byte a = digitalRead(A);
  byte b = digitalRead(B);
  byte c = digitalRead(C);
  byte d = digitalRead(D);

  byte wert = (a << 3) + (b << 2) + (c << 1) + d;
  switch(wert) {
    case 0b1011: return 0;
    case 0b0011: return 1;
    case 0b0001: return 2;
    case 0b1001: return 3;
    case 0b1000: return 4;
    case 0b1100: return 5;
    case 0b1101: return 6;
    case 0b1111: return 7;
    default: return 8;
  }
}


// ================ PEGEL ==================
#define TOLERANZ 25
#define BUFFSIZE 10
#define INTERRUPTS_ON TCA0_SPLIT_INTCTRL = 0b00010001
#define INTERRUPTS_OFF TCA0_SPLIT_INTCTRL = 0b00000000

class pegel {
public:
  int16_t messwerte[BUFFSIZE];
  byte index;
  byte error_counter = 0;
  int16_t mittelwert()  {
    INTERRUPTS_OFF;
    int16_t summe = 0;
    int16_t min = 1023;
    int16_t max = 0;
    for(byte i = 0; i < BUFFSIZE; i++) {
      summe = summe + messwerte[i];
      if (messwerte[i] > max) max = messwerte[i];
      if (messwerte[i] < min) min = messwerte[i];
    }
    INTERRUPTS_ON;
    if(abs(max - min) > TOLERANZ || summe < 10) { // Entweder zu hohe Spreizung der Messwerte (hohes Rauschen oder in Flanke gemessen), oder Wert zu gering bzw keine Messung erfolgt (kein pwm)
      error_counter++;
      return 0;
    }
    else {
      error_counter = 0;
      return (summe - min - max) / 8;
    }
  }
  char floatbuf[6];
  float spannung() { 
    float wert = mittelwert();
    if(wert == 0) return 0;
    float volts = (wert - 350) / 50;
    dtostrf(volts, 4, 1, floatbuf);
    return volts;
  }
  void clear() {
    for(byte i = 0; i < BUFFSIZE; i++) messwerte[i] = 0;
    dtostrf(0, 4, 0, floatbuf);
  }
};

// ============== STATES =============
extern pegel high, low;

class state {
public:
  char *name;
  virtual void set() = 0;
  virtual void run() = 0;

  byte enc = 0;
  float hvolts = 0;
  float lvolts = 0;
  byte counter = 0;

  byte update() {
    enc = read_encoder();
    hvolts = high.spannung();
    lvolts = low.spannung();
    if(hvolts >= 0 && lvolts < 0) return 1;
    else return 0;
  }

};


class _standby: public state {
  public:
  _standby(char *n) { name = n; }
  void run();
  void set();
};
class _detected: public state {
  public:
  _detected(char *n) { name = n; }
  void run();
  void set();
};
class _charging: public state {
  public:
  _charging(char *n) { name = n; }
  void run();
  void set();
};
class _ventilation: public state {
  public:
  _ventilation(char *n) { name = n; }
  void run();
  void set();
};
class _no_power: public state {
  public:
  _no_power(char *n) { name = n; }
  void run();
  void set();
};
class _error: public state {
  public:
  _error(char *n) { name = n; }
  void run();
  void set();
};
class _automatic: public state {
  public:
  _automatic(char *n) { name = n; }
  void run();
  void set();
};

state *machine_state;
_standby standby("Standby");
_detected detected("Detected");
_charging charging("Charging");
// _ventilation ventilation("Ventilation");
// _no_power no_power("No-Power");
_error error("Error");
_automatic automatic("Automatik");