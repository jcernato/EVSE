#pragma once
#include "functions.hpp"

void(* resetFunc) (void) = 0;
bool DEBUG = false;
byte enc = 0;
uint16_t ladeleistung = 0;
bool automatik = 0;
bool pwm_active = 0;
pegel high, low;
byte error_code = 0;

ISR(TCA0_LCMP0_vect) {
  delayMicroseconds(50); // Warte bis Spannung stabil (hohe input Widerstände + Inputkapazität -> Ladekurve)
  low.messwerte[low.index++] = analogRead(CPRead);
  if(low.index >= BUFFSIZE) {
    low.index = 0;
  }
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00010000);
}
ISR(TCA0_LUNF_vect) {
  delayMicroseconds(50); // Warte bis Spannung stabil (hohe input Widerstände + Inputkapazität -> Ladekurve)
  high.messwerte[high.index++] = analogRead(CPRead);
  if(high.index >= BUFFSIZE) {
    high.index = 0;
  }
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00000010);
}

void dbg(const char *text) {
  if(DEBUG) Serial.print(text);
}
void dbgln(const char *text) {
  if(DEBUG) Serial.println(text);
}
void dbg(const uint16_t value) {
  if(DEBUG) Serial.print(value);
}
void dbgln(const uint16_t value) {
  if(DEBUG) Serial.println(value);
}

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

void ALL_LEDs_ON(void) {
  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], HIGH);
  }
}
void ALL_LEDs_OFF(void) {
  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], LOW);
  }
}
void toggle_LED(byte index) {
    if(index < 0 || index > 7) {
        Serial.print("Encoder ERROR");
        error_code = 4;
        error.set();
        return;
    }
    ALL_LEDs_OFF();
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

byte calc_encoder(uint16_t wert) {
  if((wert >= 1000) && (wert < (ladeleistungen[1] + ladeleistungen[2]) / 2)) {
    return 1;
  } else if((wert >= (ladeleistungen[1] + ladeleistungen[2]) / 2) && (wert < (ladeleistungen[2] + ladeleistungen[3]) / 2)) {
    return 2;
  } else if((wert >= (ladeleistungen[2] + ladeleistungen[3]) / 2) && (wert < (ladeleistungen[3] + ladeleistungen[4]) / 2)) {
    return 3;
  } else if((wert >= (ladeleistungen[3] + ladeleistungen[4]) / 2) && (wert < (ladeleistungen[4] + ladeleistungen[5]) / 2)) {
    return 4;
  } else if((wert >= (ladeleistungen[4] + ladeleistungen[5]) / 2) && (wert < (ladeleistungen[5] + ladeleistungen[6]) / 2)) {
    return 5;
  } else if((wert >= (ladeleistungen[5] + ladeleistungen[6]) / 2) && (wert < (ladeleistungen[6] + ladeleistungen[7]) / 2)) {
    return 6;
  } else if((wert >= (ladeleistungen[6] + ladeleistungen[7]) / 2) && (wert <= ladeleistungen[7])) {
    return 7;
  } else {
    return 0;
  }
}

void set_ladeleistung(uint16_t leistung) {
  if(leistung != ladeleistung) {
    dbg("Ladeleistung: ");
    dbgln(leistung);
  }
  ladeleistung = leistung;
}

// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  if(leistung != ladeleistung) {
    dbg("Set PWM: ");
    dbgln(leistung);
  }
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if(leistung == 0) {
    digitalWrite(PWM, HIGH); // inverted by driver stage (cmos inverter)
    pwm_active = false;
    return;
  } else if(leistung == 1) {
    digitalWrite(PWM, LOW);  // inverted by driver stage (cmos inverter)
    pwm_active = false;
    return;
  } else if (leistung < 1000 || leistung > 3600) {
    Serial.println("PWM value error!");
    error_code = 5;
    error.set();
    return;
  }

  byte value = map(leistung, 0, 3600, 0, int(PWM_LPER*0.27));
  analogWrite(PWM, PWM_LPER - value);  
  pwm_active = true;
}


bool check_CP(float checkwert) {
  float messwert = high.spannung();
  if((messwert > (checkwert - HYST)) && (messwert < (checkwert + HYST))) {
    return true;
  } else {
    return false;
  }
}

bool diode_fail() {
  if(low.spannung() > -3.2) {
    Serial.println("Diode check failed");
    return true;
  } else {
    return false;
  }
}


#define PWM_LOW digitalWrite(PWM, HIGH)
#define PWM_HIGH digitalWrite(PWM, LOW)


void init_splash() {
  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], HIGH);
    delay(60);
  }
  delay(100);

  if(digitalRead(RESET) == 0) DEBUG = true;

  dbgln("DEBUG MODE");
  dbgln("Wallbox:");
  dbgln("Befehlsstruktur: MagicNumber/Command (Byte0), Payload(Byte 1&2), Checksum(Byte3)");
  dbgln("Checksum: (byte0 + byte1 + byte2) & 0xff");
  dbgln("Mögliche Befehle:");
  dbgln("'L': setze Ladeleistung für Automatik");
  dbgln("'F': Force -> erzwinge Automatik modus ");
  dbgln("'S': Statusanforderung");
  dbgln("     Antwort: 14 bytes:");
  dbgln("     Byte 0: 'S'");
  dbgln("     Byte 1: Wallboxstatus nach SAE_J1772 (A-E)");
  dbgln("     Byte 2: Modus (M...Manell, A...Automatik)");
  dbgln("     Byte 3: pwm-active");
  dbgln("     Byte 4 + 5: Ladeleistung");
  dbgln("     Byte 6 - 9: Spannung level high (Control Pilot)");
  dbgln("     Byte 10 - 13: Spannung level high (Control Pilot)");
  dbgln("     Byte 14: Checksum");
  dbgln("     Byte 15: Endmarker '#'");
  dbgln("'R': Reset");
  dbgln("Error codes:");
  dbgln("0: no error");
  dbgln("1: manual error injection");
  dbgln("2: Diode fail");
  dbgln("3: Measurement error");
  dbgln("4: Encoder error");
  dbgln("5: Set-PWM value error");

  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], LOW);
    delay(60);
  }
}

void update() {
  // set automatik pin to input and read state
  pinMode(AUTOMATIK, INPUT);
  delay(10);
  if(digitalRead(AUTOMATIK) == 0) {
    automatik = true;
    serial_input.force_auto = false;
  } else {
    automatik = false;
  }

  // serial data is valid
  if(millis() - serial_input.timestamp < 300000 && serial_input.timestamp != 0) {
    if(serial_input.force_auto) automatik = true;
    if(automatik) {
      ALL_LEDs_OFF();
      delay(100);
      pinMode(AUTOMATIK, OUTPUT);
      digitalWrite(AUTOMATIK, LOW);
      set_ladeleistung(serial_input.wert);
      enc = calc_encoder(ladeleistung);
      return;
    }
  }

  enc = read_encoder();
  set_ladeleistung(ladeleistungen[enc]);
  return;
}


int16_t pegel::mittelwert() {
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
      dbgln("Error measurement error_count");
      error_code = 3;
      error.set();
      return 0;
    }
  } 
  
  error_counter = 0;
  old_value = (summe - min - max) / (BUFFSIZE-2); // entferne min und max (Ausreißer)
  return old_value;
}

float adc2float(uint16_t val) {
  int a = val;
  return float((a - 350) / 50.0);
}

float pegel::spannung() { 
  uint16_t wert = 0;
  if(pwm_active) {
    wert = mittelwert();
  } else {
    wert = analogRead(CPRead);
  }

  return adc2float(wert);
}