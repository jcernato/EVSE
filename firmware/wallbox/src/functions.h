#pragma once

#include <Arduino.h>
#include "pinconfig.h"

#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz


byte read_encoder() {
  byte a = digitalRead(A);
  byte b = digitalRead(B);
  byte c = digitalRead(C);
  byte d = digitalRead(D);

  byte wert = (a << 3) + (b << 2) + (c << 1) + d;
  if (wert == 0b1111) return 7;
  else if (wert == 0b1011) return 0;
  else if (wert == 0b0011) return 1;
  else if (wert == 0b0001) return 2;
  else if (wert == 0b1001) return 3;
  else if (wert == 0b1000) return 4;
  else if (wert == 0b1100) return 5;
  else if (wert == 0b1101) return 6;
  else return 8;
}

class pegel {
public:
  int16_t messwerte[10];
  byte index;
  int16_t mittelwert()  {
    int16_t summe = 0;
    int16_t min = 1023;
    int16_t max = 0;
    for(byte i = 0; i < 10; i++) {
      summe = summe + messwerte[i];
      if (messwerte[i] > max) max = messwerte[i];
      if (messwerte[i] < min) min = messwerte[i];
    }
    return (summe - min - max) / 8;
  }
  char floatbuf[6];
  float spannung() { 
    float volts = (mittelwert() - 468) / 45.6;
    dtostrf(volts, 4, 1, floatbuf);
    return volts;
  }
};


// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if (leistung < 1200) return;
  byte value = map(leistung, 0, 3600, 0, int(PWM_LPER*0.27));
  if (leistung > 3680) return;
  analogWrite(PWM, value);  
}

extern pegel high, low;

class state {
public:
  char name;
  virtual void transition() = 0;
  virtual void run() = 0;

  void update() {
    enc = read_encoder();
    hvolts = high.spannung();
    lvolts = low.spannung();
  }

  byte enc = 0;
  float hvolts = 0;
  float lvolts = 0;
  byte counter = 0;
};


class _standby: public state {
  public:
  void run();
  void transition();
};
class _detected: public state {
  public:
  void run();
  void transition();
};
class _charging: public state {
  public:
  void run();
  void transition();
};
class _ventilation: public state {
  public:
  void run();
  void transition();
};
class _no_power: public state {
  public:
  void run();
  void transition();
};
class _error: public state {
  public:
  void run();
  void transition();
};
class _automatic: public state {
  public:
  void run();
  void transition();
};

state *machine_state;
_standby standby;
_detected detected;
_charging charging;
// _ventilation ventilation;
// _no_power no_power;
_error error;
_automatic automatic;
#define MARGIN 1 // +- margin for analogRead

// ################## STANDBY ####################
  void _standby::run() {
    // nur lvolts werden gemessen
    update();

    char buffer[30];
    sprintf(buffer, "CPilot: %s V\n", low.floatbuf);
    Serial.print(buffer);

    if(enc == 7) automatic.transition();
    else toggle_LED(enc);

    if(digitalRead(RESET) == 0) {
      Serial.println("Manual error injecton");
      error.transition();
      return;
    }

    if(lvolts > 9 - MARGIN && lvolts < 9 + MARGIN) detected.transition();
  }
  void _standby::transition() {
    machine_state = &standby;
    Serial.println("Standby");
    digitalWrite(RELAIS, LOW);
    digitalWrite(PWM, HIGH);
    toggle_LED(read_encoder());
    while(digitalRead(RESET) == 0) delay(50);
  }

// ################# DETECTED #######################
void _detected::transition() {
  machine_state = &detected;
  Serial.println("Detected");
  enc = read_encoder();
  set_pwm(ladeleistungen[enc]);
  digitalWrite(RELAIS, LOW);
  delay(100);
}
void _detected::run() {
  update();

  char buffer[30];
  sprintf(buffer, "CPilot: H: %s V\tL: %s V\t%d\n", high.floatbuf, low.floatbuf, digitalRead(ACCECK));
  Serial.print(buffer);

  if(lvolts > -4) {
    Serial.println("Diode check failed");
    error.transition();
    return;
  }
  toggle_LED(enc);
  set_pwm(ladeleistungen[enc]);
  if(hvolts > 12 - MARGIN) standby.transition();
  else if(hvolts > 6 - MARGIN && hvolts < 6 + MARGIN) charging.transition();
  else if(hvolts > 9 - MARGIN && hvolts < 9 + MARGIN) ;
  else {
    // FIXME: durch 10 sample average kann update() wärend einem Spannungssprung stattfinden. Dadurch entsteht ein Messfehler
    //        entweder nochmal scannen oder 1x ignorieren ?!
    Serial.println("Bogus voltage level hvolts");
    error.transition();
    return;
  }
}

// ################## CHARGING #####################
void _charging::transition() {
  machine_state = &charging;
  Serial.println("Charging");
  digitalWrite(RELAIS, HIGH);
  delay(100);
}
void _charging::run() {
  update();

  char buffer[30];
  sprintf(buffer, "CPilot: H: %s V\tL: %s V\t%d\n", high.floatbuf, low.floatbuf, digitalRead(ACCECK));
  Serial.print(buffer);

  if(lvolts > -4) {
    Serial.println("Diode check failed");
    error.transition();
    return;
  }
  if(hvolts > 12 - MARGIN) standby.transition();
  else if(hvolts > 6 - MARGIN && hvolts < 6 + MARGIN) ;
  else if(hvolts > 9 - MARGIN && hvolts < 9 + MARGIN) detected.transition();
}

// ################### ERRROR #######################
  void _error::transition() {
    machine_state = &error;
    Serial.println("Error");
    digitalWrite(RELAIS, LOW);
    digitalWrite(PWM, LOW);
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
    while(digitalRead(RESET) == 0) delay(50);
  }
  void _error::run() {
    if(digitalRead(RESET) == 0) standby.transition();
  }

// ################ AUTO no input ###################
  void _automatic::transition() {
    machine_state = &automatic;
    counter = 0;
    Serial.println("Automatic mode, no input");
    digitalWrite(RELAIS, LOW);
    digitalWrite(PWM, HIGH);
  }
   void _automatic::run() {
    byte enc = read_encoder();
    if(enc < 7) standby.transition();
    toggle_LED(counter++);
      if(counter == sizeof(LEDs)) counter = 0;
  }

