#include <Arduino.h>

#include "pinconfig.h"
#define PWM_HPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz


byte read_encoder() {
  byte a = digitalRead(A);
  byte b = digitalRead(B);
  byte c = digitalRead(C);
  byte d = digitalRead(D);

  byte wert = (a << 3) + (b << 2) + (c << 1) + d;
  if (wert == 0b1111) return 0;
  else if (wert == 0b1011) return 1;
  else if (wert == 0b0011) return 2;
  else if (wert == 0b0001) return 3;
  else if (wert == 0b1001) return 4;
  else if (wert == 0b1000) return 5;
  else if (wert == 0b1100) return 6;
  else if (wert == 0b1101) return 7;
  else return 8;
}

// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if (leistung < 0) return;
  byte value = map(leistung, 0, 3600, 0, int(PWM_HPER*0.27));
  if (leistung > 3680) return;
  analogWrite(PWM, value);  
}


void setup() {
  // put your setup code here, to run once:
  pininit();
  TCA0_SPLIT_HPER = PWM_HPER;
  Serial.begin(9600);
  delay(1500);
  Serial.print(" B: ");
  Serial.print(ADC0_CTRLB, BIN);
  Serial.print(" C: ");
  Serial.print(ADC0_CTRLC, BIN);
  Serial.print(" D: ");
  Serial.print(ADC0_CTRLD, BIN);
  Serial.print(" |");
  // ADC0_CTRLC = 0b01010000;
}
byte i = 0, j=0;
int dataHIGH[10];
int dataLOW[10];

void loop() {
  // // put your main code here, to run repeatedly:
  byte enc = read_encoder();

  if (enc < 7) {
    toggle_LED(enc);
    set_pwm(ladeleistungen[enc]);

    // unsigned long timestamp = micros();
    // if (digitalRead(PWM)) dataHIGH[i] == analogRead(CPRead);
    // else dataLOW[i] == analogRead(CPRead);
    // i++;
    // // delayMicroseconds();
    // unsigned long now = micros();
    // Serial.print(now-timestamp);
    while(1) {
      if(digitalRead(PWM)) {
        dataHIGH[i++ % 10] = analogRead(CPRead);
      }
      else {
        dataLOW[j++] = analogRead(CPRead); 
        if (j == 10) break;
      }
    }
    Serial.print(i);
    Serial.print((' '));
    Serial.print(j);
    Serial.println();
    for(byte i = 0; i < 10; i++) {
      Serial.print(dataHIGH[i]);
      Serial.print(' ');
    }
    Serial.println();
    for(byte i = 0; i < 10; i++) {
      Serial.print(dataLOW[i]);
      Serial.print(' ');
    }
    Serial.println();
    i=0;
    j=0;
    delay(1000);
  }

  else {
    digitalWrite(PWM, HIGH);

    for(byte i = 0; i < sizeof(LEDs); i++) {
      toggle_LED(i);
      delay(50);
    }
  }

}