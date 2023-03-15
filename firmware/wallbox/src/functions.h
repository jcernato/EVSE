#pragma once

#include <Arduino.h>
#include "pinconfig.h"



#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz
#define RX_BUFFSIZE 5

void read_serial();
void pin_init();

struct _serial_input {
  bool force_auto = false;
  uint16_t wert = 0;
  unsigned long timestamp = 0;
};

// ================ PEGEL ==================
#define TOLERANZ 25
#define BUFFSIZE 10

class pegel {
public:
  int16_t messwerte[BUFFSIZE];
  byte index;
  byte error_counter = 0;

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
      return 0;
    } else {
      error_counter = 0;
      return (summe - min - max) / 8;
    }
  }

  float spannung() { 
    float wert = mittelwert();
    if(wert == 0) {
      return 0;
    }
    float volts = (wert - 350) / 50;
    return volts;
  }

  void clear() {
    for(byte i = 0; i < BUFFSIZE; i++) messwerte[i] = 0;
  }
};

// ============== STATES =============
extern pegel high, low;
static byte enc = 0;
static uint16_t ladeleistung = 0;

class state {
public:
  char *name;
  byte statusbezeichnung;
  virtual void set() = 0;
  virtual void run() = 0;

  byte counter = 0;

  byte update();

};


class _standby: public state {
  public:
  _standby(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set();
};
class _detected: public state {
  public:
  _detected(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set();
};
class _charging: public state {
  public:
  _charging(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set();
};
// class _ventilation: public state {
//   public:
//   _ventilation(char *n, char g) { name = n; statusbezeichnung = g; }
//   void run();
//   void set();
// };
// class _no_power: public state {
//   public:
//   _no_power(char *n, char g) { name = n; statusbezeichnung = g; }
//   void run();
//   void set();
// };
class _error: public state {
  public:
  _error(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set();
};

extern state *machine_state;
extern _standby standby;
extern _detected detected;
extern _charging charging;
extern _error error;

extern byte LEDs[7];