#pragma once
#include <Arduino.h>

#define A 10
#define B 13
#define C 11
#define D 12

#define LED05 14
#define LED10 15
#define LED15 9
#define LED20 8
#define LED25 5
#define LED30 4
#define LED35 7

#define RELAIS 16
#define ACCECK 0
#define PWM 1
#define CPRead 2
#define PPRead 3

void pininit(void) {
    pinMode(A, INPUT_PULLUP);    
    pinMode(B, INPUT_PULLUP);
    pinMode(C, INPUT_PULLUP);    
    pinMode(D, INPUT_PULLUP);

    pinMode(LED05, OUTPUT);
    pinMode(LED10, OUTPUT);
    pinMode(LED15, OUTPUT);
    pinMode(LED20, OUTPUT);
    pinMode(LED25, OUTPUT);
    pinMode(LED30, OUTPUT);
    // pinMode(LED35, OUTPUT);    

    pinMode(RELAIS, OUTPUT);
    pinMode(ACCECK, INPUT);
    pinMode(PWM, OUTPUT);
    pinMode(CPRead, INPUT);
    pinMode(PPRead, INPUT);
}

byte LEDs[] = {LED05, LED10, LED15, LED20, LED25, LED30};
uint16_t ladeleistungen[] = {500, 1000, 1500, 2000, 2500, 3000};

void toggle_LED(byte index) {
    for(byte i = 0; i < sizeof(LEDs); i++) digitalWrite(LEDs[i], LOW);
    digitalWrite(LEDs[index], HIGH);
}