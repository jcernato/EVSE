#include "serial.h"

static char input_string[RX_BUFFSIZE];

void send_status() {
  char msg[10];
  char magic = 'S';
  char autom, cs;
  if(automatik) autom = 'A';
  else autom = 'M';
  msg[0] = magic;
  msg[1] = machine_state->statusbezeichnung;
  msg[2] = autom;
  msg[3] = ladeleistung >> 8;
  msg[4] = ladeleistung & 0xff;
  cs = (msg[0] + msg[1] + msg[2] + msg[3] + msg[4]) & 0xff;
  msg[5] = cs;
  msg[6] = '\0';
  for(byte i = 0; i < 6; i++) Serial.print(msg[i]);
  Serial.flush();
  delay(50);
}

void send_verbose() {
  // TODO: Implement!
    delay(50);
}

void read_serial() {
  byte index = 0;
  if(!Serial.available()) {
    return;
  }
  while(Serial.available()) {
    input_string[index++] = Serial.read();
    if(index >= RX_BUFFSIZE) {
      Serial.println("RX-Buffer full");
      index = RX_BUFFSIZE - 1;
    }
  }

  byte sum_msg = (input_string[0] + input_string[1] + input_string[2]) & 0xff;
  byte cs = input_string[3];
  if(sum_msg != cs) {
    Serial.println("Checksum failed");
    return;
  }
  byte cmd = input_string[0];
  switch(cmd) {
    case 'L': {
      serial_input.force_auto = false;
      serial_input.timestamp = millis();
      break;
    }
    case 'F': {
      serial_input.force_auto = true;
      serial_input.timestamp = millis();
      break;
    }
    case 'S': {
      send_status();
      return;
    }
    case 'V': {
      send_verbose();
      return;
    }
    case 'R': {
      delay(2500); //resetFun();
    }
    default: {
      serial_input.timestamp = 0;
      return;
    }
  }
  byte wert1 = input_string[1];
  byte wert0 = input_string[2];
  serial_input.wert = (wert1 << 8) + wert0;
  if((serial_input.wert < 1000 && serial_input.wert != 0) || serial_input.wert > 3600) {
    Serial.println("Out of range [1000 - 3600]");
    serial_input.timestamp = 0;
  }
}