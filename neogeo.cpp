#include <Arduino.h>
#include "pad_state.h"
#include "neogeo.h"

namespace NeoGeo {

const uint8_t pins[10] = {
  2,   // UP    -> DB15 pin 15
  3,   // DOWN  -> DB15 pin 7
  4,   // LEFT  -> DB15 pin 14
  5,   // RIGHT -> DB15 pin 6
  6,   // A     -> DB15 pin 13
  7,   // B     -> DB15 pin 5
  8,   // C     -> DB15 pin 12
  9,   // D     -> DB15 pin 4
  A0,  // START -> DB15 pin 11
  A1   // SELECT-> DB15 pin 3
};

void init() {
  for (uint8_t i = 0; i < 10; i++) setLine(pins[i], false);
}

void apply() {
  bool ngA = wanted[PAD_H_1] || menuWanted[PAD_H_1];
  bool ngB = wanted[PAD_L_1] || menuWanted[PAD_L_1];
  bool ngC = wanted[PAD_H_2] || menuWanted[PAD_H_2];
  bool ngD = wanted[PAD_L_2] || menuWanted[PAD_L_2];

  if (wanted[PAD_H_3]) ngA = ngA || turboState;
  if (wanted[PAD_L_3]) ngB = ngB || turboState;

  setLine(pins[0], wanted[PAD_UP] || menuWanted[PAD_UP]);
  setLine(pins[1], wanted[PAD_DOWN] || menuWanted[PAD_DOWN]);
  setLine(pins[2], wanted[PAD_LEFT] || menuWanted[PAD_LEFT]);
  setLine(pins[3], wanted[PAD_RIGHT] || menuWanted[PAD_RIGHT]);
  setLine(pins[4], ngA);
  setLine(pins[5], ngB);
  setLine(pins[6], ngC);
  setLine(pins[7], ngD);
  setLine(pins[8], wanted[PAD_START] || menuWanted[PAD_START]);
  setLine(pins[9], wanted[PAD_SELECT] || menuWanted[PAD_SELECT]);
}

const char* buttonName(int index) {
  switch (index) {
    case PAD_UP: return "NG_UP";
    case PAD_DOWN: return "NG_DOWN";
    case PAD_LEFT: return "NG_LEFT";
    case PAD_RIGHT: return "NG_RIGHT";
    case PAD_H_1: return "NG_A";
    case PAD_H_2: return "NG_C";
    case PAD_H_3: return "NG_TURBO_A";
    case PAD_L_1: return "NG_B";
    case PAD_L_2: return "NG_D";
    case PAD_L_3: return "NG_TURBO_B";
    case PAD_START: return "NG_START";
    case PAD_SELECT: return "NG_SELECT";
    case PAD_MENU: return "MENU_SCREEN";
    default: return "UNKNOWN";
  }
}

}
