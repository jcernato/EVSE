#pragma once
#include "functions.h"

void(* resetFunc) (void) = 0;

state *machine_state;
_standby standby("Standby", 'A');
_detected detected("Detected", 'B');
_charging charging("Charging", 'C');
// _ventilation ventilation("Ventilation", 'D');
// _no_power no_power("No-Power", 'E');
_error error("Error", 'F');

byte LEDs[] = {LED12, LED16, LED20, LED24, LED28, LED32, LED36};
uint16_t ladeleistungen[] = {0, 1200, 1600, 2000, 2400, 2800, 3200, 3400};

char input_string[RX_BUFFSIZE];
_serial_input serial_input;
bool automatik = false;

void pin_init(void) {
    pinMode(A, INPUT_PULLUP);    
    pinMode(B, INPUT_PULLUP);
    pinMode(C, INPUT_PULLUP);    
    pinMode(D, INPUT_PULLUP);

    pinMode(LED12, OUTPUT);
    pinMode(LED16, OUTPUT);
    pinMode(LED20, OUTPUT);
    pinMode(LED24, OUTPUT);
    pinMode(LED28, OUTPUT);
    pinMode(LED32, OUTPUT);
    pinMode(LED36, OUTPUT);    

    pinMode(RELAIS, OUTPUT);
    pinMode(PWM, OUTPUT);
    pinMode(CPRead, INPUT);
    pinMode(AUTOMATIK, INPUT);
}

void toggle_LED(byte index) {
    if(index < 0 || index > 7) {
        Serial.print("Encoder ERROR");
        error.set();
        return;
    }
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], LOW);
    if(index > 0) digitalWrite(LEDs[index-1], HIGH);
}

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

void send_status() {
  char msg[10];
  char magic = 'S';
  char autom, cs;
  if(automatik) autom = 'A';
  else autom = 'M';
  msg[0] = magic;
  msg[1] = machine_state->statusbezeichnung;
  msg[2] = autom;
  msg[3] = ladeleistung >> 8;
  msg[4] = ladeleistung & 0xff;
  cs = (msg[0] + msg[1] + msg[2] + msg[3] + msg[4]) & 0xff;
  msg[5] = cs;
  msg[6] = '\0';
  for(byte i = 0; i < 6; i++) Serial.print(msg[i]);
  delay(50);
}

void send_verbose() {
  // TODO: Implement!
    delay(50);
}

void read_serial() {
  byte index = 0;
  if(!Serial.available()) {
    return;
  }
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
  byte cmd = input_string[0];
  switch(cmd) {
    case 'L': {
      serial_input.force_auto = false;
      serial_input.timestamp = millis();
      break;
    }
    case 'F': {
      serial_input.force_auto = true;
      serial_input.timestamp = millis();
      break;
    }
    case 'S': {
      send_status();
      return;
    }
    case 'V': {
      send_verbose();
      return;
    }
    case 'R': {
      delay(2500); //resetFun();
    }
    default: {
      serial_input.timestamp = 0;
      return;
    }
  }
  byte wert1 = input_string[1];
  byte wert0 = input_string[2];
  serial_input.wert = (wert1 << 8) + wert0;
  if((serial_input.wert < 1000 && serial_input.wert != 0) || serial_input.wert > 3600) {
    Serial.println("Out of range [1000 - 3600]");
    serial_input.timestamp = 0;
  }
}

// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if(leistung == 0) {
    analogWrite(PWM, PWM_LPER+1);
    digitalWrite(PWM, LOW);
    return;
  }
  if (leistung < 1000) {
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

byte state::update() {
  pinMode(AUTOMATIK, INPUT);
  automatik = false;
  delay(50);
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
    if(serial_input.force_auto == true) {
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

  hvolts = high.spannung();
  lvolts = low.spannung();

  if(hvolts >= 0 && lvolts < 0) {
    return 1;
  } else {
    return 0;
  }
}


#define STANDBY 12
#define DETECTED 9
#define CHARGING 6
#define VENT 3
#define HYST 1.0

bool check_CP(float messwert, float checkwert) {
  if((messwert > checkwert - HYST) && messwert < checkwert + HYST) {
    return true;
  } else {
    return false;
  }
}

bool diode_fail(float messwert) {
  if(messwert > -4) {
    Serial.println("Diode check failed");
    return true;
  } else {
    return false;
  }
}

#define DIODE_CHECK if(diode_fail(lvolts)) { error.set(); return; }
#define PWM_LOW digitalWrite(PWM, HIGH)

// ################## STANDBY ####################
void _standby::run() {
  // nur hvolts werden gemessen
  low.error_counter = 0;
  update();

  toggle_LED(enc);
  // if(enc == 0) return;

  if(check_CP(hvolts, STANDBY)) {
    return;
  } else if(check_CP(hvolts, DETECTED)) {
    detected.set();
  } else if(check_CP(hvolts, CHARGING)) {
    detected.set();
  } else {
    high.error_counter++;
  }
}

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

// ################# DETECTED #######################
void _detected::set() {
  machine_state = &detected;
  digitalWrite(RELAIS, LOW);
  set_pwm(1234);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
}

void _detected::run() {
  if(!update()) {
    return;
  }

  DIODE_CHECK
  toggle_LED(enc);

  if(check_CP(hvolts, DETECTED)) {
    return;
  } else if(check_CP(hvolts, STANDBY)) {
    standby.set();
  } else if(check_CP(hvolts, CHARGING)) {
    if(enc != 0) {
      charging.set();
    } else {
      return;
    }
  } else {
    high.error_counter++;
  }
}

// ################## CHARGING #####################
void _charging::set() {
  machine_state = &charging;
  // Serial.println("Charging");
  set_pwm(ladeleistung);
  digitalWrite(RELAIS, HIGH);
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
}

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
  if(check_CP(hvolts, CHARGING)) {
    return;
  } else if(check_CP(hvolts, STANDBY)) {
    standby.set();
  } else if(check_CP(hvolts, DETECTED)) {
    detected.set();
  } else if(check_CP(hvolts, VENT)) {
    // FIXME: Ventilation status not implemented. Wenn dem Auto zu warm wird muss es selber damit klarkommen (runterregeln)
    return; 
  } else {
    high.error_counter++;
  }
}

// ################### ERRROR #######################
void _error::set() {
  machine_state = &error;
  Serial.println("Error");
  digitalWrite(RELAIS, LOW);
  PWM_LOW;
  for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], HIGH);
  while(digitalRead(RESET) == 0) delay(50);
  
}
void _error::run() {
  if(digitalRead(RESET) == 0) {
    Serial.println("Error cleared");
    high.error_counter = 0;
    low.error_counter = 0;
    standby.set();
  }
}
