#include <Arduino.h>

#include "functions.h"

pegel high;
pegel low;

ISR(TCA0_LCMP0_vect) {
  delayMicroseconds(50);
  low.messwerte[low.index++] = analogRead(CPRead);
  if(low.index >= 10) low.index = 0;
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00010000);
}
ISR(TCA0_LUNF_vect) {
  delayMicroseconds(50);
  high.messwerte[high.index++] = analogRead(CPRead);
  if(high.index >= 10) high.index = 0;
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00000010);
}

#define INTERRUPTS_ON TCA0_SPLIT_INTCTRL = 0b00010001
#define INTERRUPTS_OFF TCA0_SPLIT_INTCTRL = 0b00000000

void setup() {

  Serial.begin(9600);
  delay(10);
  pin_init();
  TCA0_SPLIT_LPER = PWM_LPER; 
  ADC0_CTRLC = 0b01010000;
  TCA0_SPLIT_INTCTRL = 0b00010001;
  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], HIGH);
    delay(60);
  }
  delay(200);
  for(byte i = 0; i < sizeof(LEDs); i++) {
    digitalWrite(LEDs[i], LOW);
    delay(60);
  }
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
  delay(300);
  standby.set();
}

void loop() {
  // put your main code here, to run repeatedly:
  INTERRUPTS_ON;
  delay(50);
  INTERRUPTS_OFF;

  read_serial();
  machine_state->run();

  if(machine_state == &error) return;
  // only executed if state is not error
  if(high.error_counter > 5 || low.error_counter > 5) {
    Serial.print("Messfehler: ");
    if(high.error_counter > 5) Serial.println("High");
    if(low.error_counter > 5) Serial.println("Low");
    error.set();
  }

  if(digitalRead(RESET) == 0) {
    Serial.println("Manual error injecton");
    byte i = 0;
    delay(50);
    while(digitalRead(RESET) == 0) {
      delay(50);
      i++;
      if(i > 40) resetFunc();
    }
    error.set();
  }
}