#pragma once

#include <Arduino.h>
#include "functions.h"
#include "states.h"

#define RX_BUFFSIZE 5
void read_serial();

struct _serial_input {
  bool force_auto = false;
  uint16_t wert = 0;
  unsigned long timestamp = 0;
};
extern _serial_input serial_input;