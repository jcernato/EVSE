#include "states.h"

byte state::update() {
  pinMode(AUTOMATIK, INPUT);
  automatik = false;
  delay(10);
  if(digitalRead(AUTOMATIK) == 0) {
    automatik = true;
    serial_input.force_auto = false;
  }
  if(millis() - serial_input.timestamp < 300000 && serial_input.timestamp > 0) {
    ladeleistung = serial_input.wert;
    if((serial_input.wert >= 1000) && (serial_input.wert < (ladeleistungen[1] + ladeleistungen[2]) / 2)) {
      enc = 1;
    } else if((serial_input.wert >= (ladeleistungen[1] + ladeleistungen[2]) / 2) && (serial_input.wert < (ladeleistungen[2] + ladeleistungen[3]) / 2)) {
      enc = 2;
    } else if((serial_input.wert >= (ladeleistungen[2] + ladeleistungen[3]) / 2) && (serial_input.wert < (ladeleistungen[3] + ladeleistungen[4]) / 2)) {
      enc = 3;
    } else if((serial_input.wert >= (ladeleistungen[3] + ladeleistungen[4]) / 2) && (serial_input.wert < (ladeleistungen[4] + ladeleistungen[5]) / 2)) {
      enc = 4;
    } else if((serial_input.wert >= (ladeleistungen[4] + ladeleistungen[5]) / 2) && (serial_input.wert < (ladeleistungen[5] + ladeleistungen[6]) / 2)) {
      enc = 5;
    } else if((serial_input.wert >= (ladeleistungen[5] + ladeleistungen[6]) / 2) && (serial_input.wert < (ladeleistungen[6] + ladeleistungen[7]) / 2)) {
      enc = 6;
    } else if((serial_input.wert >= (ladeleistungen[6] + ladeleistungen[7]) / 2) && (serial_input.wert <= ladeleistungen[7])) {
      enc = 7;
    } else {
      enc = 0;
    }
    if(serial_input.force_auto) {
      automatik = true;
      pinMode(AUTOMATIK, OUTPUT);
      digitalWrite(AUTOMATIK, LOW);
    }
  } else if (serial_input.force_auto) {
    if(digitalRead(AUTOMATIK) == 0) {
      serial_input.force_auto = false;
    } else {
      pinMode(AUTOMATIK, OUTPUT);
      digitalWrite(AUTOMATIK, LOW);
      automatik = true;
      enc = 0;
      delay(200);
    }

  } else {
    enc = 0;    // zurücksetzen
  }

  if(!automatik) { 
    enc = read_encoder();
    ladeleistung = ladeleistungen[enc];
  }

  if(high.spannung() >= 0 && low.spannung() < 0) {
    return 1;
  } else {
    return 0;
  }
}

// ######################################## //
// ################## OFF ################# //
// ################## SET ################# //
void _off::set() {
  machine_state = &off;
  digitalWrite(RELAIS, LOW);
  set_pwm(0);
}

// ################## OFF ################# //
// ################## RUN ################# //
void _off::run() {
    update();
    if(enc == 0) return;
    DIODE_CHECK
    return;
}

// ######################################## //
// ################ STANDBY ############### //
// ################## SET ################# //
void _standby::set() {
  machine_state = &standby;
  digitalWrite(RELAIS, LOW);
  set_pwm(0);
  low.clear();
  if(digitalRead(RESET) == 0) {
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], LOW);
    while(digitalRead(RESET) == 0) delay(50);
  } else {
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
  }
}

// ################ STANDBY ############### //
// ################## RUN ################# //
void _standby::run() {
  // nur hvolts werden gemessen
  update();

  toggle_LED(enc);
  // if(enc == 0) return;

  if(check_CP(STANDBY)) {
    return;
  } else if(check_CP(DETECTED)) {
    detected.set();
  } else if(check_CP(CHARGING)) {
    detected.set();
  }
}
// ######################################## //
// ################ DETECTED ############## //
// ################## SET ################# //
void _detected::set() {
  machine_state = &detected;
  digitalWrite(RELAIS, LOW);
  set_pwm(2345);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
}

// ################ DETECTED ############## //
// ################## RUN ################# //
void _detected::run() {
  if(!update()) {
    return;
  }

  DIODE_CHECK
  toggle_LED(enc);

  if(check_CP(DETECTED)) {
    return;
  } else if(check_CP(STANDBY)) {
    standby.set();
  } else if(check_CP(CHARGING)) {
    if(enc != 0) {
      charging.set();
    } else {
      return;
    }
  }
}

// ######################################## //
// ############### CHARGING ############### //
// ################## SET ################# //
void _charging::set() {
  machine_state = &charging;
  // Serial.println("Charging");
  set_pwm(ladeleistung);
  digitalWrite(RELAIS, HIGH);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
}
// ############### CHARGING ############### //
// ################## RUN ################# //
void _charging::run() {
  if(!update()) {
    return;
  }

  DIODE_CHECK 
  toggle_LED(enc);

  if(enc == 0) {
    detected.set();
    return;
  }

  set_pwm(ladeleistung);
  if(check_CP(CHARGING)) {
    return;
  } else if(check_CP(STANDBY)) {
    standby.set();
  } else if(check_CP(DETECTED)) {
    detected.set();
  } else if(check_CP(VENT)) {
    // FIXME: Ventilation status not implemented. Wenn dem Auto zu warm wird muss es selber damit klarkommen (runterregeln)
    Serial.println("Ventilation not implemented!");
    return; 
  }
}

// ######################################## //
// ################# ERRROR ############### //
// ################## SET ################# //
void _error::set() {
  machine_state = &error;
  Serial.println("Error");
  digitalWrite(RELAIS, LOW);
  set_pwm(0);
  high.error_counter = 0;
  low.error_counter = 0;
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
  while(digitalRead(RESET) == 0) delay(50);
  
}
// ################# ERRROR ############### //
// ################## RUN ################# //
void _error::run() {
  if(digitalRead(RESET) == 0) {
    Serial.println("Error cleared");
    standby.set();
  }
}
