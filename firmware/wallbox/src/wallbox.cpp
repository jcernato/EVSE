#include <Arduino.h>
#include <avr/wdt.h>
#include "functions.h"


void setup() {
  Serial.begin(9600);
  delay(10);
  pin_init();
  // set PWM frequency
    TCA0_SPLIT_LPER = PWM_LPER; 
  // configure ADC (faster reading)
    ADC0_CTRLC = 0b01010000;

  init_splash();
  delay(300);
  off.set();
  wdt_enable(9); //  2s
}

void loop() {
  wdt_reset();
  while(digitalRead(RESET) == 0) delay(50);

  // Messen
  if(pwm_active) {
    INTERRUPTS_ON;
    delay(50);
    INTERRUPTS_OFF;
  } else {
    delay(50);
  }

  read_serial();
  machine_state->run();

  if(digitalRead(RESET) == 0) {
    dbgln("Manual error injecton");
    error.set();
    while(digitalRead(RESET) == 0) delay(50);
  }
}