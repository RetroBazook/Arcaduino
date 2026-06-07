#include <Arduino.h>
#include "pad_state.h"
#include "mapping_manager.h"
#include "pce.h"

namespace PCE {

// PC Engine controller port logic:
// Console drives SEL and CLR. Controller returns 4 active-low lines D0..D3.
//
// 2-button protocol:
//   SEL HIGH: D3 LEFT, D2 DOWN, D1 RIGHT, D0 UP
//   SEL LOW : D3 RUN,  D2 SELECT, D1 II,    D0 I
//
// 6-button protocol alternates every CLR scan:
//   scan 0: same as 2-button
//   scan 1: SEL HIGH returns 0000,
//           SEL LOW returns VI,V,IV,III on D3..D0.
//
// --------------------------------------------------
// Panel mapping for PCE:
//
// 2 buttons:
//   PAD_L_2 -> II
//   PAD_L_3 -> I
//
//   PAD_H_2 -> turbo II
//   PAD_H_3 -> turbo I
//
// 3 buttons:
//   PAD_L_1 -> III
//   PAD_L_2 -> II
//   PAD_L_3 -> I
//
//   PAD_H_1 -> turbo III
//   PAD_H_2 -> turbo II
//   PAD_H_3 -> turbo I
//
// 6 buttons:
//   PAD_L_1 -> III
//   PAD_L_2 -> II
//   PAD_L_3 -> I
//
//   PAD_H_1 -> IV
//   PAD_H_2 -> V
//   PAD_H_3 -> VI
// --------------------------------------------------

#define PCE_D0_PIN   2
#define PCE_D1_PIN   3
#define PCE_D2_PIN   4
#define PCE_D3_PIN   5
#define PCE_SEL_PIN  A0
#define PCE_CLR_PIN  A1

PadMode pceMode = PCE_2_BUTTONS;

bool lastCLR = LOW;
uint8_t scanParity = 0; // 0 = normal scan, 1 = extra III/IV/V/VI scan
unsigned long lastClrRise = 0;

void setNibble(bool d0Down, bool d1Down, bool d2Down, bool d3Down) {
  setLine(PCE_D0_PIN, d0Down);
  setLine(PCE_D1_PIN, d1Down);
  setLine(PCE_D2_PIN, d2Down);
  setLine(PCE_D3_PIN, d3Down);
}

void allReleased() {
  setNibble(false, false, false, false);
}

void allLow() {
  setNibble(true, true, true, true);
}

bool turboPressed(bool normalButton, bool turboButton) {
  return normalButton || (turboButton && turboState);
}

void init() {
  pinMode(PCE_SEL_PIN, INPUT);
  pinMode(PCE_CLR_PIN, INPUT);

  allReleased();

  lastCLR = digitalRead(PCE_CLR_PIN);
  scanParity = 0;
  lastClrRise = micros();
}

void nextMode() {
  if (pceMode == PCE_2_BUTTONS) {
    pceMode = PCE_3_BUTTONS;
  } else if (pceMode == PCE_3_BUTTONS) {
    pceMode = PCE_6_BUTTONS;
  } else {
    pceMode = PCE_2_BUTTONS;
  }

  scanParity = 0;
  lastClrRise = micros();

  Serial.print("[PCE] mode = ");
  Serial.println(modeName());
}

const char* modeName() {
  switch (pceMode) {
    case PCE_2_BUTTONS: return "2 buttons";
    case PCE_3_BUTTONS: return "3 buttons";
    case PCE_6_BUTTONS: return "6 buttons";
    default: return "UNKNOWN";
  }
}

void updateScanState(bool clr) {
  unsigned long now = micros();

  // CLR rising edge begins a new controller scan. For 6-button mode,
  // alternate normal and extra-button scan on each CLR pulse.
  if (clr && !lastCLR) {
    if (pceMode == PCE_6_BUTTONS) {
      scanParity ^= 1;
    } else {
      scanParity = 0;
    }

    lastClrRise = now;
  }

  // If CLR is idle for a long time, resync to normal scan.
  if (now - lastClrRise > 20000) {
    scanParity = 0;
  }

  lastCLR = clr;
}

void apply() {
  bool sel = digitalRead(PCE_SEL_PIN);
  bool clr = digitalRead(PCE_CLR_PIN);

  updateScanState(clr);

  // Most simple PCE controllers output 0000 while CLR is HIGH.
  if (clr) {
    allLow();
    return;
  }

  // 6-button extra scan
  if (pceMode == PCE_6_BUTTONS && scanParity == 1) {
    if (sel == HIGH) {
      // Impossible direction state used by games to identify 6-button scan.
      allLow();
    } else {
      // Extra buttons on SEL LOW:
      // D3 VI, D2 V, D1 IV, D0 III.
      bool btnIII = MappingManager::pressed(PAD_L_1);
      bool btnIV  = MappingManager::pressed(PAD_H_1);
      bool btnV   = MappingManager::pressed(PAD_H_2);
      bool btnVI  = MappingManager::pressed(PAD_H_3);

      setNibble(btnIII, btnIV, btnV, btnVI);
    }

    return;
  }

  if (sel == HIGH) {
    // Directions:
    // D3 LEFT, D2 DOWN, D1 RIGHT, D0 UP.
    setNibble(
      MappingManager::pressed(PAD_UP),
      MappingManager::pressed(PAD_RIGHT),
      MappingManager::pressed(PAD_DOWN),
      MappingManager::pressed(PAD_LEFT)
    );
  } else {
    // Normal buttons:
    // D3 RUN, D2 SELECT, D1 II, D0 I.
    bool btnI;
    bool btnII;
    bool btnRun = MappingManager::pressed(PAD_START);
    bool btnSelect = MappingManager::pressed(PAD_SELECT);

    if (pceMode == PCE_6_BUTTONS) {
      // In 6-button mode:
      // PAD_L_3 -> I
      // PAD_L_2 -> II
      // PAD_L_1 -> III in extra scan
      btnI = MappingManager::pressed(PAD_L_3);
      btnII = MappingManager::pressed(PAD_L_2);
    } else {
      // In 2/3-button mode:
      // bottom row = normal buttons
      // top row = turbo equivalent.
      btnI = turboPressed(
        MappingManager::pressed(PAD_L_3),
        MappingManager::pressed(PAD_H_3)
      );

      btnII = turboPressed(
        MappingManager::pressed(PAD_L_2),
        MappingManager::pressed(PAD_H_2)
      );

      if (pceMode == PCE_3_BUTTONS) {
        // III mapped on SELECT line.
        btnSelect = btnSelect || turboPressed(
          MappingManager::pressed(PAD_L_1),
          MappingManager::pressed(PAD_H_1)
        );
      }
    }

    setNibble(btnI, btnII, btnSelect, btnRun);
  }
}

const char* buttonName(int index) {
  switch (index) {
    case PAD_UP: return "PCE_UP";
    case PAD_DOWN: return "PCE_DOWN";
    case PAD_LEFT: return "PCE_LEFT";
    case PAD_RIGHT: return "PCE_RIGHT";

    case PAD_L_3: return "PCE_I";
    case PAD_L_2: return "PCE_II";
    case PAD_L_1: return "PCE_III";

    case PAD_H_3:
      return pceMode == PCE_6_BUTTONS
        ? "PCE_VI"
        : "PCE_TURBO_I";

    case PAD_H_2:
      return pceMode == PCE_6_BUTTONS
        ? "PCE_V"
        : "PCE_TURBO_II";

    case PAD_H_1:
      return pceMode == PCE_6_BUTTONS
        ? "PCE_IV"
        : "PCE_TURBO_III";

    case PAD_START: return "PCE_RUN";
    case PAD_SELECT: return "PCE_SELECT";
    case PAD_MENU: return "PCE_MODE_SWITCH";

    default: return "UNKNOWN";
  }
}

}
