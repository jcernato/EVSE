#pragma once
#include "functions.h"

void(* resetFunc) (void) = 0;

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


// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if(leistung == 0) {
    digitalWrite(PWM, HIGH); // inverted by driver stage (cmos inverter)
    pwm_active = false;
  } else if(leistung == 1) {
    digitalWrite(PWM, LOW);
    pwm_active = false;
  } else if (leistung < 1000 || leistung > 3600) {
    Serial.println("PWM value error!");
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
  if(low.spannung() > -4) {
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

  Serial.println("Wallbox:");
  Serial.println("Befehlsstruktur: MagicNumber (Byte0), Payload(Byte 1&2), Checksum(Byte3)");
  Serial.println("Checksum: (byte0 + byte1 + byte2) & 0xff");
  Serial.println("Mögliche Befehle:");
  Serial.println("'L': setze Ladeleistung für Automatik");
  Serial.println("'F': Force -> erzwinge Automatik modus ");
  Serial.println("'S': Statusanforderung");
  Serial.println("     Antwort: 4 bytes:");
  Serial.println("     Byte 0: 'S'");
  Serial.println("     Byte 1: Wallboxstatus nach SAE_J1772 (A-E)");
  Serial.println("     Byte 2: Modus (M...Manell, A...Automatik)");
  Serial.println("     Byte 3 + 4: Ladeleistung");
  Serial.println("     Byte 5: Checksum");
  Serial.println("'V': Verbose - Statusname + gemessene Spannungen (CP)");
  Serial.println("'R': Reset");

  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], LOW);
    delay(60);
  }
}