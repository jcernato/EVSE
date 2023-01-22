#pragma once
#include <Arduino.h>

#define A 10
#define B 13
#define C 11
#define D 12

#define LED12 14
#define LED16 15
#define LED20 1
#define LED24 8
#define LED28 5
#define LED32 4
#define LED36 6

#define RELAIS 16
#define ACCECK 0
#define PWM 9
#define CPRead 2
#define PPRead 3

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
    pinMode(ACCECK, INPUT);
    pinMode(PWM, OUTPUT);
    pinMode(CPRead, INPUT);
    pinMode(PPRead, INPUT);
}

byte LEDs[] = {LED12, LED16, LED20, LED24, LED28, LED32, LED36};
uint16_t ladeleistungen[] = {1200, 1600, 2000, 2400, 2800, 3200, 3600};

void toggle_LED(byte index) {
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], LOW);
    digitalWrite(LEDs[index], HIGH);
}