#include "states.h"

static state *machine_state;

void state::set(void) {
  machine_state = this;
  dbg("Set state: ");
  dbgln(name);
  set_individual();
}



// ######################################## //
// ################## OFF ################# //
// ################## SET ################# //
void _off::set_individual() {
  digitalWrite(RELAIS, LOW);
  set_pwm(0);
  ALL_LEDs_ON();
}

// ################## OFF ################# //
// ################## RUN ################# //
void _off::run() {
   update();
    toggle_LED(enc);
    if(enc == 0) {
      return;
    } else {
      standby.set();
    }
}

// ######################################## //
// ################ STANDBY ############### //
// ################## SET ################# //
void _standby::set_individual() {
  digitalWrite(RELAIS, LOW);
  set_pwm(1);
  low.clear();
  if(digitalRead(RESET) == 0) {
    ALL_LEDs_OFF();
    while(digitalRead(RESET) == 0) delay(50);
  } else {
    ALL_LEDs_ON();
  }
}

// ################ STANDBY ############### //
// ################## RUN ################# //
void _standby::run() {
  // nur hvolts werden gemessen
  update();

  toggle_LED(enc);
  if(enc == 0) {
    off.set();
    return;
  }

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
void _detected::set_individual() {
  digitalWrite(RELAIS, LOW);
  set_pwm(2345);
  ALL_LEDs_ON();
}

// ################ DETECTED ############## //
// ################## RUN ################# //
void _detected::run() {
  update();

  DIODE_CHECK
  toggle_LED(enc);
  if(enc == 0) {
    off.set();
    return;
  }

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
void _charging::set_individual() {
  set_pwm(ladeleistung);
  digitalWrite(RELAIS, HIGH);
  ALL_LEDs_ON();
}
// ############### CHARGING ############### //
// ################## RUN ################# //
void _charging::run() {
  update();

  DIODE_CHECK 
  toggle_LED(enc);
  if(enc == 0) {
    off.set();
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
void _error::set_individual() {
  digitalWrite(RELAIS, LOW);
  set_pwm(0);
  high.error_counter = 0;
  low.error_counter = 0;
  ALL_LEDs_ON();
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
