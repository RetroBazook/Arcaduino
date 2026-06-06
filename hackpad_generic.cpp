#include <Arduino.h>
#include "pad_state.h"
#include "hackpad_generic.h"

namespace HackPadGeneric {

#define PIN_UP      2
#define PIN_DOWN    3
#define PIN_LEFT    4
#define PIN_RIGHT   5

#define PIN_B1      6
#define PIN_B2      7
#define PIN_B3      8
#define PIN_B4      A0
#define PIN_B5      A1
#define PIN_B6      A2

#define PIN_START   A3
#define PIN_SELECT  A4

void setSwitch(uint8_t pin, bool pressed)
{
  digitalWrite(pin, pressed ? HIGH : LOW);
}

const char* buttonName(int index)
{
  switch (index) {
    case PAD_UP:     return "PAD_UP";
    case PAD_DOWN:   return "PAD_DOWN";
    case PAD_LEFT:   return "PAD_LEFT";
    case PAD_RIGHT:  return "PAD_RIGHT";

    case PAD_L_1:    return "PAD_L_1";
    case PAD_L_2:    return "PAD_L_2";
    case PAD_L_3:    return "PAD_L_3";

    case PAD_H_1:    return "PAD_H_1";
    case PAD_H_2:    return "PAD_H_2";
    case PAD_H_3:    return "PAD_H_3";

    case PAD_START:  return "PAD_START";
    case PAD_SELECT: return "PAD_SELECT";
    case PAD_MENU:   return "PAD_MENU";

    default:         return "UNKNOWN";
  }
}

void init()
{
  const uint8_t pins[] = {
    PIN_UP,
    PIN_DOWN,
    PIN_LEFT,
    PIN_RIGHT,
    PIN_B1,
    PIN_B2,
    PIN_B3,
    PIN_B4,
    PIN_B5,
    PIN_B6,
    PIN_START,
    PIN_SELECT
  };

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
}

void apply()
{
  setSwitch(PIN_UP, wanted[PAD_UP]);
  setSwitch(PIN_DOWN, wanted[PAD_DOWN]);
  setSwitch(PIN_LEFT, wanted[PAD_LEFT]);
  setSwitch(PIN_RIGHT, wanted[PAD_RIGHT]);

  setSwitch(PIN_B1, wanted[PAD_H_1]);
  setSwitch(PIN_B2, wanted[PAD_H_2]);
  setSwitch(PIN_B3, wanted[PAD_H_3]);

  setSwitch(PIN_B4, wanted[PAD_L_1]);
  setSwitch(PIN_B5, wanted[PAD_L_2]);
  setSwitch(PIN_B6, wanted[PAD_L_3]);

  setSwitch(PIN_START, wanted[PAD_START]);
  setSwitch(PIN_SELECT, wanted[PAD_SELECT]);
}

}