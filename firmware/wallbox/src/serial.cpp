#include "serial.hpp"

static char input_string[RX_BUFFSIZE];
_serial_input serial_input;

void send_status() {
  char msg[20];
  char magic = '$';
  char autom, cs;
  if(automatik) autom = 'A';
  else autom = 'M';
  bool pwm = pwm_active;
  union {
    float wert;
    byte bytes[4];
  } fl;
  msg[0] = magic;
  msg[1] = machine_state->statusbezeichnung;
  msg[2] = autom;
  msg[3] = pwm;
  msg[4] = ladeleistung >> 8;
  msg[5] = ladeleistung & 0xff;
  fl.wert = high.spannung();
  for(byte i = 0; i < 4; i++) msg[i+6] = fl.bytes[i];
  fl.wert = low.spannung();
  for(byte i = 0; i < 4; i++) msg[i+10] = fl.bytes[i];
  msg[14] = error_code;
  unsigned int sum = 0;
  for(byte i = 0; i < 15; i++) sum = sum + msg[i];
  cs = sum & 0xff;
  msg[15] = cs;
  msg[16] = '#';
  for(byte i = 0; i < 17; i++) Serial.print(msg[i]);
  Serial.flush();
  delay(50);
}

void read_serial() {
  byte index = 0;
  if(!Serial.available()) {
    return;
  }
  delay(50);
  while(Serial.available()) {
    char c = Serial.read();
    input_string[index++] = c;
    if(index >= RX_BUFFSIZE) {
      index = RX_BUFFSIZE - 1;
    }
  }

  byte sum_msg = (input_string[0] + input_string[1] + input_string[2]) & 0xff;
  byte cs = input_string[3];
  dbg("Received data: ");
  if(sum_msg != cs) {
    Serial.println("WB: Checksum failed");
    return;
  }
  byte cmd = input_string[0];
  switch(cmd) {
    case 'L': {
      dbgln("Set Ladeleistung");
      serial_input.force_auto = false;
      serial_input.timestamp = millis();
      break;
    }
    case 'F': {
      dbgln("Force Ladeleistung");
      serial_input.force_auto = true;
      serial_input.timestamp = millis();
      break;
    }
    case 'S': {
      dbgln("Request Status");
      send_status();
      return;
    }
    case 'R': {
      Serial.println("OK");
      dbgln("Reset");
      delay(2500); //resetFun();
    }
    default: {
      dbgln("Unknown");
      serial_input.timestamp = 0;
      return;
    }
  }
  byte wert1 = input_string[1];
  byte wert0 = input_string[2];
  serial_input.wert = (wert1 << 8) + wert0;
  if((serial_input.wert < 1000 && serial_input.wert != 0) || serial_input.wert > 3600) {
    Serial.println("WB: Out of range [1000 - 3600]");
    serial_input.timestamp = 0;
  }
  Serial.println("OK");
}