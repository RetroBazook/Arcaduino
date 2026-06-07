#include <Arduino.h>
#include "mapping_manager.h"

namespace MappingManager {

enum MappingMode {
  MAPPING_DEFAULT,
  MAPPING_CUSTOM
};

// outputToInput[logical output] = physical input.
// Example: if outputToInput[PAD_H_1] = PAD_L_2,
// then pressed(PAD_H_1) returns wanted[PAD_L_2].
uint8_t outputToInput[PAD_BUTTON_COUNT];

MappingMode currentMode = MAPPING_DEFAULT;

const uint8_t remappableButtons[] = {
  PAD_H_1,
  PAD_H_2,
  PAD_H_3,
  PAD_L_1,
  PAD_L_2,
  PAD_L_3
};

// Requested learning order:
// B1, B2, B4, B5, B3, B6
//
// With the generic hack output convention:
// B1 = PAD_H_1
// B2 = PAD_H_2
// B3 = PAD_H_3
// B4 = PAD_L_1
// B5 = PAD_L_2
// B6 = PAD_L_3
const uint8_t remapOrder[] = {
  PAD_H_1, // B1
  PAD_H_2, // B2
  PAD_L_1, // B4
  PAD_L_2, // B5
  PAD_H_3, // B3
  PAD_L_3  // B6
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

bool rawPressed(uint8_t physicalButton)
{
  if (physicalButton >= PAD_BUTTON_COUNT) {
    return false;
  }

  return wanted[physicalButton];
}

bool pressed(uint8_t logicalButton)
{
  if (logicalButton >= PAD_BUTTON_COUNT) {
    return false;
  }

  // During custom remap, make START blink slowly for visual feedback.
  if (logicalButton == PAD_START && remapMode) {
    return remapBlinkState;
  }

  // After reset to default, make START blink quickly 6 times.
  if (logicalButton == PAD_START && resetBlinkActive) {
    return resetBlinkState;
  }

  uint8_t physicalButton = outputToInput[logicalButton];

  if (physicalButton >= PAD_BUTTON_COUNT) {
    return false;
  }

  return wanted[physicalButton];
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

const char* outputName(uint8_t outputButton)
{
  switch (outputButton) {
    case PAD_H_1: return "B1";
    case PAD_H_2: return "B2";
    case PAD_H_3: return "B3";
    case PAD_L_1: return "B4";
    case PAD_L_2: return "B5";
    case PAD_L_3: return "B6";
    default: return padName(outputButton);
  }
}

void printCurrentMapping()
{
  Serial.println("[Mapping] current mapping:");

  for (uint8_t i = 0; i < sizeof(remappableButtons) / sizeof(remappableButtons[0]); i++) {
    uint8_t outputButton = remappableButtons[i];

    Serial.print("  ");
    Serial.print(outputName(outputButton));
    Serial.print(" / ");
    Serial.print(padName(outputButton));
    Serial.print(" <- ");
    Serial.println(padName(outputToInput[outputButton]));
  }
}

void startResetBlink()
{
  resetBlinkActive = true;
  resetBlinkState = false;
  resetBlinkCount = 0;
  resetBlinkLast = millis();
}

void setDefaultMapping(bool withFeedback)
{
  currentMode = MAPPING_DEFAULT;

  for (uint8_t i = 0; i < PAD_BUTTON_COUNT; i++) {
    outputToInput[i] = i;
  }

  remapMode = false;
  remapStep = 0;
  remapWaitRelease = false;
  remapBlinkState = false;

  Serial.println("[Mapping] mode: DEFAULT");
  printCurrentMapping();

  if (withFeedback) {
    startResetBlink();
  } else {
    resetBlinkActive = false;
    resetBlinkState = false;
  }
}

void startCustomRemap()
{
  currentMode = MAPPING_CUSTOM;

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

  Serial.println("[Mapping] custom remap started");
  Serial.println("[Mapping] release combo buttons");
}

bool comboCustomRemap()
{
  return
    rawPressed(PAD_START) &&
    rawPressed(PAD_SELECT) &&
    rawPressed(PAD_H_1) &&
    rawPressed(PAD_H_2) &&
    rawPressed(PAD_H_3) &&
    !rawPressed(PAD_DOWN);
}

bool comboDefault()
{
  return
    rawPressed(PAD_START) &&
    rawPressed(PAD_SELECT) &&
    rawPressed(PAD_DOWN) &&
    rawPressed(PAD_H_1) &&
    rawPressed(PAD_H_2) &&
    rawPressed(PAD_H_3);
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

  // Reset first because it is the custom combo + DOWN.
  if (comboDefault()) {
    setDefaultMapping(true);
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

    if (rawPressed(pad) && !usedButtons[pad]) {
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

  // Wait for the launch combo to be fully released.
  // Otherwise H1/H2/H3 could be learned accidentally.
  if (remapWaitRelease) {
    if (!rawPressed(PAD_START) &&
        !rawPressed(PAD_SELECT) &&
        !rawPressed(PAD_H_1) &&
        !rawPressed(PAD_H_2) &&
        !rawPressed(PAD_H_3)) {
      remapWaitRelease = false;

      Serial.print("[Mapping] press input for ");
      Serial.print(outputName(remapOrder[remapStep]));
      Serial.print(" / ");
      Serial.println(padName(remapOrder[remapStep]));
    }

    return;
  }

  int8_t pad = findNewRemapPress();

  if (pad < 0) {
    return;
  }

  uint8_t outputButton = remapOrder[remapStep];

  outputToInput[outputButton] = pad;
  usedButtons[pad] = true;

  Serial.print("[Mapping] ");
  Serial.print(outputName(outputButton));
  Serial.print(" / ");
  Serial.print(padName(outputButton));
  Serial.print(" <- ");
  Serial.println(padName(pad));

  remapStep++;

  if (remapStep >= sizeof(remapOrder) / sizeof(remapOrder[0])) {
    remapMode = false;
    remapBlinkState = false;
    remapWaitRelease = false;

    Serial.println("[Mapping] custom remap complete");
    printCurrentMapping();
    return;
  }

  Serial.print("[Mapping] press input for ");
  Serial.print(outputName(remapOrder[remapStep]));
  Serial.print(" / ");
  Serial.println(padName(remapOrder[remapStep]));
}

void init()
{
  setDefaultMapping(false);
}

void update()
{
  if (remapMode) {
    updateCustomRemap();
  } else {
    updatePresetCombos();
  }

  updateResetBlink();
}

}
