#include <Arduino.h>

#include "pinconfig.h"
#define PWM_LPER 158 // set counter reset to 158 => change PWM Frequency from 617 to 1000 Hz


byte read_encoder() {
  byte a = digitalRead(A);
  byte b = digitalRead(B);
  byte c = digitalRead(C);
  byte d = digitalRead(D);

  byte wert = (a << 3) + (b << 2) + (c << 1) + d;
  if (wert == 0b1111) return 8;
  else if (wert == 0b1011) return 0;
  else if (wert == 0b0011) return 1;
  else if (wert == 0b0001) return 2;
  else if (wert == 0b1001) return 3;
  else if (wert == 0b1000) return 4;
  else if (wert == 0b1100) return 5;
  else if (wert == 0b1101) return 6;
  else return 8;
}

// input: leistung in W (max 3680) = 16 A * 230 V
void set_pwm(uint16_t leistung) {
  // Duty cycle (in%) = Verfügbare Stromstärke (in A) ÷ 0,6 A   16 A entsprechen 27% duty
  // Gültiger Bereich: 10 % - 85 %
  //                   => Mindeststrom = 6 A (manchmal auch 4.8 A?)
  //                      1200 W ... 5.2 A mal schauen ob da eGolf damit ladet ...
  // https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/
  if (leistung < 1200) return;
  byte value = map(leistung, 0, 3600, 0, int(PWM_LPER*0.27));
  if (leistung > 3680) return;
  analogWrite(PWM, value);  
}

uint16_t highval[10], lowval[10];
byte hindex = 0, lindex = 0;
ISR(TCA0_LCMP0_vect) {
  highval[hindex++] = analogRead(CPRead);
  if(hindex == 10) hindex = 0;
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00010000);
}
ISR(TCA0_LUNF_vect) {
  lowval[lindex++] = analogRead(CPRead);
  if(lindex == 10) lindex = 0;
  TCA0_SPLIT_INTFLAGS = TCA0_SPLIT_INTFLAGS | (0b00000010);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);
  pin_init();
  TCA0_SPLIT_LPER = PWM_LPER; 
  ADC0_CTRLC = 0b01010000;
  TCA0_SPLIT_INTCTRL = 0b00010001;
}

byte i = 0;
char buff[40];
char hbuf[8], lbuf[8];
void loop() {
  // put your main code here, to run repeatedly:
  byte enc = read_encoder();

  if (enc < 7) {
    toggle_LED(enc);
    set_pwm(ladeleistungen[enc]);

    int16_t high=0, low=0;
    for(byte j = 0; j < 10; j++) {
      high = high + highval[j];
      low = low + lowval[j];
    }
    float hvolts = ((high / 10) - 468) / 45.6;
    float lvolts = ((low / 10) - 468) / 45.6;

    dtostrf(hvolts, 4, 1, hbuf);
    dtostrf(lvolts, 4, 1, lbuf);
    
    sprintf(buff, "H: %s V\t L: %s V\n", hbuf, lbuf);
    Serial.print(buff);

    delay(100);
  }

  else {
    digitalWrite(PWM, HIGH);

    toggle_LED(i++);
    if(i == sizeof(LEDs)) i = 0;
    delay(50);
  }

}