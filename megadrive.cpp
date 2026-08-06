#include <Arduino.h>
#include "pad_state.h"
#include "mapping_manager.h"
#include "megadrive.h"

namespace MegaDrive {

#define MD_UP_PIN     2
#define MD_DOWN_PIN   3
#define MD_LEFT_PIN   4
#define MD_RIGHT_PIN  5
#define MD_6_PIN      6   // DB9 pin 6 : B / A
#define MD_9_PIN      7   // DB9 pin 9 : C / START
#define MD_TH_PIN     A0  // DB9 pin 7 : TH depuis console

PadMode mdPadMode = MD_6_BUTTONS;

enum MdCycle : uint8_t {
  MD_CYCLE_TH_HIGH = 0, // BlueRetro buttons[0]
  MD_CYCLE_TH_LOW  = 1, // BlueRetro buttons[1]
  MD_CYCLE_6BTNS   = 2  // BlueRetro buttons[2]
};

static bool lastTH = HIGH;
static uint8_t thEdges = 0;
static unsigned long lastThChange = 0;

static const unsigned long MD_TIMEOUT_US = 2500;

static inline bool pressed(int btn) {
  return MappingManager::pressed(btn);
}

static inline bool turboPressed(bool normalButton, bool turboButton) {
  return normalButton || (turboButton && turboState);
}

static void resetProtocol(bool th, unsigned long now) {
  lastTH = th;
  thEdges = 0;
  lastThChange = now;
}

static void updateProtocol(bool th, unsigned long now) {
  if ((unsigned long)(now - lastThChange) > MD_TIMEOUT_US) {
    resetProtocol(th, now);
    return;
  }

  if (th != lastTH) {
    lastTH = th;
    lastThChange = now;

    if (thEdges < 7) {
      thEdges++;
    }
  }
}

static MdCycle getCycle(bool th) {
  if (mdPadMode == MD_3_BUTTONS) {
    return th ? MD_CYCLE_TH_HIGH : MD_CYCLE_TH_LOW;
  }

  // En 6 boutons :
  // TH HIGH normal avant la séquence complète
  // TH LOW normal avant ID
  // puis cycle 6BTNS après les bascules rapides TH.
  if (thEdges >= 6 && th == HIGH) {
    return MD_CYCLE_6BTNS;
  }

  return th ? MD_CYCLE_TH_HIGH : MD_CYCLE_TH_LOW;
}

void init6() {
  pinMode(MD_TH_PIN, INPUT);

  setLine(MD_UP_PIN, false);
  setLine(MD_DOWN_PIN, false);
  setLine(MD_LEFT_PIN, false);
  setLine(MD_RIGHT_PIN, false);
  setLine(MD_6_PIN, false);
  setLine(MD_9_PIN, false);

  resetProtocol(digitalRead(MD_TH_PIN), micros());
}

void nextMode() {
  mdPadMode = (mdPadMode == MD_6_BUTTONS) ? MD_3_BUTTONS : MD_6_BUTTONS;

  resetProtocol(digitalRead(MD_TH_PIN), micros());

  Serial.print("[MD] mode = ");
  Serial.println(modeName());
}

const char* modeName() {
  return mdPadMode == MD_6_BUTTONS ? "6 buttons" : "3 buttons";
}

void apply6() {
  const bool th = digitalRead(MD_TH_PIN);
  const unsigned long now = micros();

  updateProtocol(th, now);

  bool mdA;
  bool mdB;
  bool mdC;

  if (mdPadMode == MD_3_BUTTONS) {
    mdA = turboPressed(pressed(PAD_L_1), pressed(PAD_H_1));
    mdB = turboPressed(pressed(PAD_L_2), pressed(PAD_H_2));
    mdC = turboPressed(pressed(PAD_L_3), pressed(PAD_H_3));
  } else {
    mdA = pressed(PAD_L_1);
    mdB = pressed(PAD_L_2);
    mdC = pressed(PAD_L_3);
  }

  const bool mdStart = pressed(PAD_START);
  const bool mdX = pressed(PAD_H_1);
  const bool mdY = pressed(PAD_H_2);
  const bool mdZ = pressed(PAD_H_3);
  const bool mdMode = pressed(PAD_SELECT);

  switch (getCycle(th)) {
    case MD_CYCLE_TH_HIGH:
      // BlueRetro cycle 0 : TH HIGH
      // U D L R B C
      setLine(MD_UP_PIN, pressed(PAD_UP));
      setLine(MD_DOWN_PIN, pressed(PAD_DOWN));
      setLine(MD_LEFT_PIN, pressed(PAD_LEFT));
      setLine(MD_RIGHT_PIN, pressed(PAD_RIGHT));
      setLine(MD_6_PIN, mdB);
      setLine(MD_9_PIN, mdC);
      break;

    case MD_CYCLE_TH_LOW:
      if (mdPadMode == MD_6_BUTTONS && thEdges >= 5) {
        // ID 6 boutons : 0 0 0 0 A START
        setLine(MD_UP_PIN, true);
        setLine(MD_DOWN_PIN, true);
        setLine(MD_LEFT_PIN, true);
        setLine(MD_RIGHT_PIN, true);
      } else {
        // Normal : U D 0 0 A START
        setLine(MD_UP_PIN, pressed(PAD_UP));
        setLine(MD_DOWN_PIN, pressed(PAD_DOWN));
        setLine(MD_LEFT_PIN, true);
        setLine(MD_RIGHT_PIN, true);
      }

      setLine(MD_6_PIN, mdA);
      setLine(MD_9_PIN, mdStart);
      break;

    case MD_CYCLE_6BTNS:
      // BlueRetro cycle 2 : 6BTNS
      // Z Y X MODE 1 1
      setLine(MD_UP_PIN, mdZ);
      setLine(MD_DOWN_PIN, mdY);
      setLine(MD_LEFT_PIN, mdX);
      setLine(MD_RIGHT_PIN, mdMode);
      setLine(MD_6_PIN, false);
      setLine(MD_9_PIN, false);
      break;
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