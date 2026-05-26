#pragma once
#include <Arduino.h>

enum OutputMode {
  MODE_NEOGEO,
  MODE_MD6,
  MODE_NES,
  MODE_SNES,
  MODE_PCE,
  MODE_PS2,
  MODE_GAMECUBE
};

enum PadButton {
  PAD_UP = 0,
  PAD_DOWN,
  PAD_LEFT,
  PAD_RIGHT,

  PAD_H_1,
  PAD_H_2,
  PAD_H_3,

  PAD_L_1,
  PAD_L_2,
  PAD_L_3,

  PAD_START,
  PAD_SELECT,

  PAD_MENU,

  PAD_BUTTON_COUNT
};

extern bool rawWanted[PAD_BUTTON_COUNT];
extern bool wanted[PAD_BUTTON_COUNT];
extern bool menuWanted[PAD_BUTTON_COUNT];
extern bool turboState;

OutputMode getOutputMode();
const char* buttonName(int index);
void pushButton(int index, bool isDown);
void updateDebounce();
void updateTurbo();
void setLine(uint8_t pin, bool isDown);
