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

void(* resetFunc) (void) = 0;
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

  if(lauf++ == 0) {
    char buffer[40];
    sprintf(buffer, "%s: H: %s V\tL: %s V", machine_state->name, high.floatbuf, low.floatbuf);
    Serial.println(buffer);
    delay(50);
  }
  if(lauf >= 50) lauf = 0;

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