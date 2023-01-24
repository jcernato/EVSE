#include <Arduino.h>

#include "pinconfig.h"
#include "functions.h"

pegel high;
pegel low;

ISR(TCA0_LCMP0_vect) {
  high.messwerte[high.index++] = analogRead(CPRead);
  if(high.index == 10) high.index = 0;
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00010000);
}
ISR(TCA0_LUNF_vect) {
  low.messwerte[low.index++] = analogRead(CPRead);
  if(low.index == 10) low.index = 0;
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
  standby.transition();
  digitalWrite(PWM, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  machine_state->run();
  delay(50);
}