#include <Arduino.h>

#include "pinconfig.h"
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


void setup() {

  Serial.begin(9600);
  delay(10);
  pin_init();
  TCA0_SPLIT_LPER = PWM_LPER; 
  ADC0_CTRLC = 0b01010000;
  TCA0_SPLIT_INTCTRL = 0b00010001;
  delay(200);
  standby.set();
}

byte lauf = 0;
void loop() {
  // put your main code here, to run repeatedly:

  delay(50);
  machine_state->run();

  if(lauf++ > 20) {
    lauf = 0;
    char buffer[30];
    sprintf(buffer, "%s: H: %s V\tL: %s V", machine_state->name, high.floatbuf, low.floatbuf);
    Serial.println(buffer);
  }
  // Serial.available();
  // Serial.read();
  if(machine_state == &error) return;
  // only executed if state is not error
  if(high.error_counter > 5 || low.error_counter > 5) {
    Serial.println("Messfehler");
    error.set();
  }

  if(digitalRead(RESET) == 0) {
    Serial.println("Manual error injecton");
    error.set();
  }
}