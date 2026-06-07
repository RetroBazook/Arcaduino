#include "pad_state.h"
#include "pce.h"
#include "megadrive.h"

bool wanted[PAD_BUTTON_COUNT] = {false};
static bool rawWanted[PAD_BUTTON_COUNT] = {false};
static bool menuWanted[PAD_BUTTON_COUNT] = {false};

unsigned long downStartTime[PAD_BUTTON_COUNT] = {0};
const unsigned long minPressMs = 15;

unsigned long lastTurboToggle = 0;
bool turboState = false;
const unsigned long turboInterval = 50;

bool menuScreenLatch = false;

void setLine(uint8_t pin, bool isDown) {
  if (isDown) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    pinMode(pin, INPUT);
  }
}

void handleMenuScreen(bool isDown) {
  if (!isDown) {
    menuScreenLatch = false;

    for (uint8_t i = 0; i < PAD_BUTTON_COUNT; i++) {
      menuWanted[i] = false;
    }
    return;
  }

  if (menuScreenLatch) return;
  menuScreenLatch = true;

  if (getOutputMode() == MODE_PCE) {
    PCE::nextMode();
    return;
  }

  if (getOutputMode() == MODE_MD6) {
    MegaDrive::nextMode();
    return;
  }

  // NeoGeo menu = A+B+C+START, mapped through generic panel positions.
  menuWanted[PAD_H_1] = true;
  menuWanted[PAD_L_1] = true;
  menuWanted[PAD_H_2] = true;
  menuWanted[PAD_START] = true;
}

void pushButton(int index, bool isDown) {
  if (index == PAD_MENU) {
    handleMenuScreen(isDown);
    return;
  }

  if (index < 0 || index >= PAD_BUTTON_COUNT) return;

  if (isDown) {
    if (!rawWanted[index]) {
      rawWanted[index] = true;
      downStartTime[index] = millis();
    }
  } else {
    rawWanted[index] = false;
    wanted[index] = false;
  }
}

void updateDebounce() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < PAD_BUTTON_COUNT; i++) {
    if (rawWanted[i] && !wanted[i]) {
      if (now - downStartTime[i] >= minPressMs) {
        wanted[i] = true;
      }
    }
  }
}

void updateTurbo() {
  if (millis() - lastTurboToggle >= turboInterval) {
    lastTurboToggle = millis();
    turboState = !turboState;
  }
}
