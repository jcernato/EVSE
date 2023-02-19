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
    case 0b1011: return 1;
    case 0b0011: return 2;
    case 0b0001: return 3;
    case 0b1001: return 4;
    case 0b1000: return 5;
    case 0b1100: return 6;
    case 0b1101: return 7;
    case 0b1111: return 0;
    default: return 0;
  }
}

#define RX_BUFFSIZE 5
char input_string[RX_BUFFSIZE];
struct _serial_input {
  bool force_auto = false;
  uint16_t wert = 0;
  unsigned long timestamp = 0;
} serial_input;

void read_serial() {
  byte index = 0;
  if(!Serial.available()) return;
  while(Serial.available()) {
    input_string[index++] = Serial.read();
    if(index >= RX_BUFFSIZE) {
      Serial.println("RX-Buffer full");
      index = RX_BUFFSIZE - 1;
    }
  }

  byte sum_msg = (input_string[0] + input_string[1] + input_string[2]) & 0xff;
  byte cs = input_string[3];
  if(sum_msg != cs) {
    Serial.println("Checksum failed");
    return;
  }

  serial_input.timestamp = 0;
  serial_input.force_auto = false;
  byte cmd = input_string[0];
  switch(cmd) {
    case 0xb4: break;
    case 0xb5: serial_input.force_auto = true; break;
    case 0xb6: Serial.println("Clear Error, not implemented"); break;
    default: return;
  }
  byte wert1 = input_string[1];
  byte wert0 = input_string[2];
  serial_input.wert = (wert1 << 8) + wert0;
  if(serial_input.wert < 1000 || serial_input.wert > 3600) Serial.println("Out of range [1000 - 3600]");
  else { Serial.println("OK"); serial_input.timestamp = millis(); }
  return;
}

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
static byte enc = 0;

class state {
public:
  char *name;
  virtual void set() = 0;
  virtual void run() = 0;

  float hvolts = 0;
  float lvolts = 0;
  byte counter = 0;

  byte update() {
    pinMode(AUTOMATIK, INPUT);
    delay(50);
    bool automatik = false;
    if(digitalRead(AUTOMATIK) == 0) automatik = true;
    if(millis() - serial_input.timestamp < 300000 && serial_input.timestamp > 0) {
      if((serial_input.wert >= 1000) && (serial_input.wert < (ladeleistungen[1] + ladeleistungen[2]) / 2)) enc = 1;
      else if((serial_input.wert >= (ladeleistungen[1] + ladeleistungen[2]) / 2) && (serial_input.wert < (ladeleistungen[2] + ladeleistungen[3]) / 2)) enc = 2;
      else if((serial_input.wert >= (ladeleistungen[2] + ladeleistungen[3]) / 2) && (serial_input.wert < (ladeleistungen[3] + ladeleistungen[4]) / 2)) enc = 3;
      else if((serial_input.wert >= (ladeleistungen[3] + ladeleistungen[4]) / 2) && (serial_input.wert < (ladeleistungen[4] + ladeleistungen[5]) / 2)) enc = 4;
      else if((serial_input.wert >= (ladeleistungen[4] + ladeleistungen[5]) / 2) && (serial_input.wert < (ladeleistungen[5] + ladeleistungen[6]) / 2)) enc = 5;
      else if((serial_input.wert >= (ladeleistungen[5] + ladeleistungen[6]) / 2) && (serial_input.wert < (ladeleistungen[6] + ladeleistungen[7]) / 2)) enc = 6;
      else if((serial_input.wert >= (ladeleistungen[6] + ladeleistungen[7]) / 2) && (serial_input.wert <= ladeleistungen[7])) enc = 7;
      else enc = 0;
      if(serial_input.force_auto == true) {
        automatik = true;
        pinMode(AUTOMATIK, OUTPUT);
        digitalWrite(AUTOMATIK, LOW);
      }
    }
    else enc = 0;
    if(!automatik) enc = read_encoder();

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
// _automatic automatic("Automatik");