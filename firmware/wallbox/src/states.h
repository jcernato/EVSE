#pragma once

#include <Arduino.h>
#include "functions.h"

// ======================================== //
// ============== STATE CLASS ============= //
// ======================================== //
class state {
public:
  char *name;
  byte statusbezeichnung;
  virtual void set_individual() = 0;
  virtual void run() = 0;
  void set(void);
  byte counter = 0;
};

// ======================================== //
// ============= STATES =================== //
// ======================================== //
class _off: public state {
  public:
  _off(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set_individual();
};
class _standby: public state {
  public:
  _standby(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set_individual();
};
class _detected: public state {
  public:
  _detected(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set_individual();
};
class _charging: public state {
  public:
  _charging(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set_individual();
};
// class _ventilation: public state {
//   public:
//   _ventilation(char *n, char g) { name = n; statusbezeichnung = g; }
//   void run();
//   void set_individual();
// };
// class _no_power: public state {
//   public:
//   _no_power(char *n, char g) { name = n; statusbezeichnung = g; }
//   void run();
//   void set_individual();
// };
class _error: public state {
  public:
  _error(char *n, char g) { name = n; statusbezeichnung = g; }
  void run();
  void set_individual();
};

extern state *machine_state;
static _off off("Off", '0');
static _standby standby("Standby", 'A');
static _detected detected("Detected", 'B');
static _charging charging("Charging", 'C');
// static _ventilation ventilation("Ventilation", 'D');
// static _no_power no_power("No-Power", 'E');
static _error error("Error", 'F');