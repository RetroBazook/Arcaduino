#include <Arduino.h>
#include "pad_state.h"
#include "megadrive.h"

namespace MegaDrive {

// Mega Drive / Genesis controller port logic:
// Console drives TH. Controller returns active-low lines.
//
// 3-button-compatible phases:
//   TH HIGH: UP, DOWN, LEFT, RIGHT, B, C
//   TH LOW : UP, DOWN, ID LOW, ID LOW, A, START
//
// 6-button mode adds extra phases after several TH transitions:
//   extra TH HIGH: MODE, X, Z, Y exposed on LEFT, RIGHT, pin 6, pin 9.
//
// Panel mapping for Mega Drive:
//   3 buttons:
//     PAD_L_1 -> A, PAD_L_2 -> B, PAD_L_3 -> C
//     PAD_H_1 -> turbo A, PAD_H_2 -> turbo B, PAD_H_3 -> turbo C
//
//   6 buttons:
//     PAD_H_1 -> X, PAD_H_2 -> Y, PAD_H_3 -> Z
//     PAD_L_1 -> A, PAD_L_2 -> B, PAD_L_3 -> C
//     PAD_SELECT -> MODE

#define MD_UP_PIN     2
#define MD_DOWN_PIN   3
#define MD_LEFT_PIN   4
#define MD_RIGHT_PIN  5
#define MD_6_PIN      6   // DB9 pin 6 : B / A / Z
#define MD_9_PIN      7   // DB9 pin 9 : C / START / Y
#define MD_TH_PIN     A0  // DB9 pin 7 : TH depuis console

PadMode mdPadMode = MD_6_BUTTONS;

bool lastTH = HIGH;
uint8_t thStep = 0;
unsigned long lastThChange = 0;

void init6() {
  pinMode(MD_TH_PIN, INPUT);

  setLine(MD_UP_PIN, false);
  setLine(MD_DOWN_PIN, false);
  setLine(MD_LEFT_PIN, false);
  setLine(MD_RIGHT_PIN, false);
  setLine(MD_6_PIN, false);
  setLine(MD_9_PIN, false);

  lastTH = digitalRead(MD_TH_PIN);
  thStep = 0;
  lastThChange = micros();
}

void nextMode() {
  mdPadMode = (mdPadMode == MD_6_BUTTONS) ? MD_3_BUTTONS : MD_6_BUTTONS;

  thStep = 0;
  lastTH = digitalRead(MD_TH_PIN);
  lastThChange = micros();

  Serial.print("[MD] mode = ");
  Serial.println(modeName());
}

const char* modeName() {
  return mdPadMode == MD_6_BUTTONS ? "6 buttons" : "3 buttons";
}

bool turboPressed(bool normalButton, bool turboButton) {
  return normalButton || (turboButton && turboState);
}

void apply6() {
  bool th = digitalRead(MD_TH_PIN);
  unsigned long now = micros();

  if (th != lastTH) {
    lastTH = th;
    thStep++;
    lastThChange = now;
  }

  // If the console stops toggling TH, resync to the normal 3-button phase.
  if (now - lastThChange > 2500) {
    thStep = 0;
  }

  bool mdA;
  bool mdB;
  bool mdC;

  if (mdPadMode == MD_3_BUTTONS) {
    // Top row = turbo equivalent of the bottom row.
    mdA = turboPressed(wanted[PAD_L_1], wanted[PAD_H_1]);
    mdB = turboPressed(wanted[PAD_L_2], wanted[PAD_H_2]);
    mdC = turboPressed(wanted[PAD_L_3], wanted[PAD_H_3]);
  } else {
    mdA = wanted[PAD_L_1];
    mdB = wanted[PAD_L_2];
    mdC = wanted[PAD_L_3];
  }

  bool mdX = wanted[PAD_H_1];
  bool mdY = wanted[PAD_H_2];
  bool mdZ = wanted[PAD_H_3];
  bool mdStart = wanted[PAD_START];
  bool mdMode = wanted[PAD_SELECT];

  bool sixButtonPhase = (mdPadMode == MD_6_BUTTONS && thStep >= 5);

  if (th == HIGH) {
    if (!sixButtonPhase) {
      // Normal phase: U D L R B C
      setLine(MD_UP_PIN, wanted[PAD_UP]);
      setLine(MD_DOWN_PIN, wanted[PAD_DOWN]);
      setLine(MD_LEFT_PIN, wanted[PAD_LEFT]);
      setLine(MD_RIGHT_PIN, wanted[PAD_RIGHT]);
      setLine(MD_6_PIN, mdB);
      setLine(MD_9_PIN, mdC);
    } else {
      // 6-button extra phase: MODE X Z Y
      setLine(MD_UP_PIN, true);    // ID for 6-button phase
      setLine(MD_DOWN_PIN, true);  // ID for 6-button phase
      setLine(MD_LEFT_PIN, mdMode);
      setLine(MD_RIGHT_PIN, mdX);
      setLine(MD_6_PIN, mdZ);
      setLine(MD_9_PIN, mdY);
    }
  } else {
    if (!sixButtonPhase) {
      // Normal phase: U D 0 0 A START
      setLine(MD_UP_PIN, wanted[PAD_UP]);
      setLine(MD_DOWN_PIN, wanted[PAD_DOWN]);
      setLine(MD_LEFT_PIN, true);
      setLine(MD_RIGHT_PIN, true);
      setLine(MD_6_PIN, mdA);
      setLine(MD_9_PIN, mdStart);
    } else {
      // 6-button ID phase
      setLine(MD_UP_PIN, true);
      setLine(MD_DOWN_PIN, true);
      setLine(MD_LEFT_PIN, true);
      setLine(MD_RIGHT_PIN, true);
      setLine(MD_6_PIN, false);
      setLine(MD_9_PIN, false);
    }
  }
}

const char* buttonName(int index) {
  switch (index) {
    case PAD_UP: return "MD_UP";
    case PAD_DOWN: return "MD_DOWN";
    case PAD_LEFT: return "MD_LEFT";
    case PAD_RIGHT: return "MD_RIGHT";

    case PAD_H_1: return mdPadMode == MD_6_BUTTONS ? "MD_X" : "MD_TURBO_A";
    case PAD_H_2: return mdPadMode == MD_6_BUTTONS ? "MD_Y" : "MD_TURBO_B";
    case PAD_H_3: return mdPadMode == MD_6_BUTTONS ? "MD_Z" : "MD_TURBO_C";

    case PAD_L_1: return "MD_A";
    case PAD_L_2: return "MD_B";
    case PAD_L_3: return "MD_C";

    case PAD_START: return "MD_START";
    case PAD_SELECT: return "MD_MODE";
    case PAD_MENU: return "MD_MODE_SWITCH";
    default: return "UNKNOWN";
  }
}

}
