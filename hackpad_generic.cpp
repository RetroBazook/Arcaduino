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

enum HackMappingMode {
  HACK_MAPPING_DEFAULT,
  HACK_MAPPING_CUSTOM
};

HackMappingMode currentMode = HACK_MAPPING_DEFAULT;

uint8_t outputMap[6] = {
  PAD_H_1,
  PAD_H_2,
  PAD_H_3,
  PAD_L_1,
  PAD_L_2,
  PAD_L_3
};

const uint8_t remapOrder[6] = {
  0, // B1
  1, // B2
  3, // B4
  4, // B5
  2, // B3
  5  // B6
};

const uint8_t remappableButtons[] = {
  PAD_H_1,
  PAD_H_2,
  PAD_H_3,
  PAD_L_1,
  PAD_L_2,
  PAD_L_3
};

bool comboLatch = false;

bool remapMode = false;
uint8_t remapStep = 0;
bool usedButtons[PAD_BUTTON_COUNT];

bool remapWaitRelease = false;
unsigned long remapBlinkLast = 0;
bool remapBlinkState = false;
const unsigned long remapBlinkInterval = 300;

bool resetBlinkActive = false;
bool resetBlinkState = false;
uint8_t resetBlinkCount = 0;
unsigned long resetBlinkLast = 0;
const unsigned long resetBlinkInterval = 100;

void setSwitch(uint8_t pin, bool pressed)
{
  digitalWrite(pin, pressed ? HIGH : LOW);
}

bool isPressed(uint8_t pad)
{
  return wanted[pad];
}

const char* padName(uint8_t pad)
{
  switch (pad) {
    case PAD_UP: return "PAD_UP";
    case PAD_DOWN: return "PAD_DOWN";
    case PAD_LEFT: return "PAD_LEFT";
    case PAD_RIGHT: return "PAD_RIGHT";

    case PAD_H_1: return "PAD_H_1";
    case PAD_H_2: return "PAD_H_2";
    case PAD_H_3: return "PAD_H_3";

    case PAD_L_1: return "PAD_L_1";
    case PAD_L_2: return "PAD_L_2";
    case PAD_L_3: return "PAD_L_3";

    case PAD_START: return "PAD_START";
    case PAD_SELECT: return "PAD_SELECT";
    case PAD_MENU: return "PAD_MENU";

    default: return "UNKNOWN";
  }
}

const char* outputName(uint8_t outputIndex)
{
  switch (outputIndex) {
    case 0: return "B1";
    case 1: return "B2";
    case 2: return "B3";
    case 3: return "B4";
    case 4: return "B5";
    case 5: return "B6";
    default: return "UNKNOWN_OUTPUT";
  }
}

void printCurrentMapping()
{
  Serial.println("[HackPad] current mapping:");

  for (uint8_t i = 0; i < 6; i++) {
    Serial.print("  ");
    Serial.print(outputName(i));
    Serial.print(" <- ");
    Serial.println(padName(outputMap[i]));
  }
}

void startResetBlink()
{
  resetBlinkActive = true;
  resetBlinkState = false;
  resetBlinkCount = 0;
  resetBlinkLast = millis();
}

void setDefaultMapping()
{
  currentMode = HACK_MAPPING_DEFAULT;

  outputMap[0] = PAD_H_1; // B1
  outputMap[1] = PAD_H_2; // B2
  outputMap[2] = PAD_H_3; // B3

  outputMap[3] = PAD_L_1; // B4
  outputMap[4] = PAD_L_2; // B5
  outputMap[5] = PAD_L_3; // B6

  remapMode = false;
  remapStep = 0;
  remapWaitRelease = false;
  remapBlinkState = false;

  Serial.println("[HackPad] mode: DEFAULT");
  printCurrentMapping();

  startResetBlink();
}

void startCustomRemap()
{
  currentMode = HACK_MAPPING_CUSTOM;

  remapMode = true;
  remapStep = 0;
  remapWaitRelease = true;

  remapBlinkLast = millis();
  remapBlinkState = false;

  resetBlinkActive = false;
  resetBlinkState = false;

  for (uint8_t i = 0; i < PAD_BUTTON_COUNT; i++) {
    usedButtons[i] = false;
  }

  Serial.println("[HackPad] custom remap started");
  Serial.println("[HackPad] release combo buttons");
}

bool comboCustomRemap()
{
  return
    isPressed(PAD_START) &&
    isPressed(PAD_SELECT) &&
    isPressed(PAD_H_1) &&
    isPressed(PAD_H_2) &&
    isPressed(PAD_H_3) &&
    !isPressed(PAD_DOWN);
}

bool comboDefault()
{
  return
    isPressed(PAD_START) &&
    isPressed(PAD_SELECT) &&
    isPressed(PAD_DOWN) &&
    isPressed(PAD_H_1) &&
    isPressed(PAD_H_2) &&
    isPressed(PAD_H_3);
}

bool anyComboPressed()
{
  return comboCustomRemap() || comboDefault();
}

void updatePresetCombos()
{
  if (!anyComboPressed()) {
    comboLatch = false;
    return;
  }

  if (comboLatch) {
    return;
  }

  comboLatch = true;

  if (comboDefault()) {
    setDefaultMapping();
    return;
  }

  if (comboCustomRemap()) {
    startCustomRemap();
    return;
  }
}

int8_t findNewRemapPress()
{
  for (uint8_t i = 0; i < sizeof(remappableButtons) / sizeof(remappableButtons[0]); i++) {
    uint8_t pad = remappableButtons[i];

    if (wanted[pad] && !usedButtons[pad]) {
      return pad;
    }
  }

  return -1;
}

void updateRemapBlink()
{
  if (!remapMode) {
    return;
  }

  if (millis() - remapBlinkLast >= remapBlinkInterval) {
    remapBlinkLast = millis();
    remapBlinkState = !remapBlinkState;
  }
}

void updateResetBlink()
{
  if (!resetBlinkActive) {
    return;
  }

  if (millis() - resetBlinkLast < resetBlinkInterval) {
    return;
  }

  resetBlinkLast = millis();
  resetBlinkState = !resetBlinkState;
  resetBlinkCount++;

  if (resetBlinkCount >= 12) {
    resetBlinkActive = false;
    resetBlinkState = false;
  }
}

void updateCustomRemap()
{
  if (!remapMode) {
    return;
  }

  updateRemapBlink();

  if (remapWaitRelease) {
    if (!wanted[PAD_START] &&
        !wanted[PAD_SELECT] &&
        !wanted[PAD_H_1] &&
        !wanted[PAD_H_2] &&
        !wanted[PAD_H_3]) {
      remapWaitRelease = false;

      Serial.print("[HackPad] press input for ");
      Serial.println(outputName(remapOrder[remapStep]));
    }

    return;
  }

  int8_t pad = findNewRemapPress();

  if (pad < 0) {
    return;
  }

  uint8_t outputIndex = remapOrder[remapStep];

  outputMap[outputIndex] = pad;
  usedButtons[pad] = true;

  Serial.print("[HackPad] ");
  Serial.print(outputName(outputIndex));
  Serial.print(" <- ");
  Serial.println(padName(pad));

  remapStep++;

  if (remapStep >= 6) {
    remapMode = false;
    remapBlinkState = false;
    remapWaitRelease = false;

    Serial.println("[HackPad] custom remap complete");
    printCurrentMapping();
    return;
  }

  Serial.print("[HackPad] press input for ");
  Serial.println(outputName(remapOrder[remapStep]));
}

void updateMappingControl()
{
  if (remapMode) {
    updateCustomRemap();
    return;
  }

  updatePresetCombos();
}

const char* buttonName(int index)
{
  switch (index) {
    case PAD_UP: return "PAD_UP (D2)";
    case PAD_DOWN: return "PAD_DOWN (D3)";
    case PAD_LEFT: return "PAD_LEFT (D4)";
    case PAD_RIGHT: return "PAD_RIGHT (D5)";

    case PAD_H_1: return "PAD_H_1";
    case PAD_H_2: return "PAD_H_2";
    case PAD_H_3: return "PAD_H_3";

    case PAD_L_1: return "PAD_L_1";
    case PAD_L_2: return "PAD_L_2";
    case PAD_L_3: return "PAD_L_3";

    case PAD_START: return "PAD_START";
    case PAD_SELECT: return "PAD_SELECT";
    case PAD_MENU: return "PAD_MENU";

    default: return "UNKNOWN";
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

  for (uint8_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }

  setDefaultMapping();

  // Pas de blink au démarrage.
  resetBlinkActive = false;
  resetBlinkState = false;
}

void apply()
{
  updateMappingControl();
  updateResetBlink();

  setSwitch(PIN_UP, wanted[PAD_UP]);
  setSwitch(PIN_DOWN, wanted[PAD_DOWN]);
  setSwitch(PIN_LEFT, wanted[PAD_LEFT]);
  setSwitch(PIN_RIGHT, wanted[PAD_RIGHT]);

  setSwitch(PIN_B1, wanted[outputMap[0]]);
  setSwitch(PIN_B2, wanted[outputMap[1]]);
  setSwitch(PIN_B3, wanted[outputMap[2]]);
  setSwitch(PIN_B4, wanted[outputMap[3]]);
  setSwitch(PIN_B5, wanted[outputMap[4]]);
  setSwitch(PIN_B6, wanted[outputMap[5]]);

  bool startOutput = wanted[PAD_START];

  if (remapMode) {
    startOutput = remapBlinkState;
  } else if (resetBlinkActive) {
    startOutput = resetBlinkState;
  }

  setSwitch(PIN_START, startOutput);
  setSwitch(PIN_SELECT, wanted[PAD_SELECT]);
}

}