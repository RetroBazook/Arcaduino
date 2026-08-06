#include <Arduino.h>
#include <util/atomic.h>

#include "pad_state.h"
#include "mapping_manager.h"
#include "snes.h"

namespace SNES {

#define SNES_LATCH_PIN 2
#define SNES_CLOCK_PIN 3
#define SNES_DATA_PIN  4

// Arduino UNO/Nano : D4 = PD4
static constexpr uint8_t DATA_MASK = _BV(PD4);

static constexpr uint8_t SNES_REPORT_BITS = 16;
static constexpr uint8_t NES_REPORT_BITS  = 8;

volatile bool     isSnes       = true;
volatile uint8_t  reportBits   = SNES_REPORT_BITS;
volatile uint16_t cachedReport = 0xFFFF;
volatile uint16_t shiftReport  = 0xFFFF;
volatile uint8_t  shiftIndex   = 0;

// Ordre SNES / NPISO :
// B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R, puis 1,1,1,1
enum NpisoBit : uint8_t {
  NPISO_B      = 0,
  NPISO_Y      = 1,
  NPISO_SELECT = 2,
  NPISO_START  = 3,
  NPISO_UP     = 4,
  NPISO_DOWN   = 5,
  NPISO_LEFT   = 6,
  NPISO_RIGHT  = 7,
  NPISO_A      = 8,
  NPISO_X      = 9,
  NPISO_L      = 10,
  NPISO_R      = 11
};

static inline void dataLow() {
  PORTD &= ~DATA_MASK;
  DDRD  |=  DATA_MASK;   // sortie LOW
}

static inline void dataHigh() {
  PORTD &= ~DATA_MASK;   // pull-up interne OFF
  DDRD  &= ~DATA_MASK;   // entrée = ligne relâchée
}

static inline void writeCurrentBit() {
  if (shiftIndex >= reportBits) {
    dataHigh();
    return;
  }

  if (shiftReport & (1 << shiftIndex)) {
    dataHigh();
  } else {
    dataLow();
  }
}

static inline void press(uint16_t &report, uint8_t bit, bool isPressed) {
  if (isPressed) {
    report &= ~(1 << bit); // actif-bas
  }
}

static uint16_t buildReport() {
  uint16_t r = 0xFFFF;

  if (isSnes) {
    press(r, NPISO_B,      MappingManager::pressed(PAD_L_1));
    press(r, NPISO_Y,      MappingManager::pressed(PAD_H_2));
    press(r, NPISO_SELECT, MappingManager::pressed(PAD_SELECT));
    press(r, NPISO_START,  MappingManager::pressed(PAD_START));
    press(r, NPISO_UP,     MappingManager::pressed(PAD_UP));
    press(r, NPISO_DOWN,   MappingManager::pressed(PAD_DOWN));
    press(r, NPISO_LEFT,   MappingManager::pressed(PAD_LEFT));
    press(r, NPISO_RIGHT,  MappingManager::pressed(PAD_RIGHT));
    press(r, NPISO_A,      MappingManager::pressed(PAD_L_2));
    press(r, NPISO_X,      MappingManager::pressed(PAD_H_1));
    press(r, NPISO_L,      MappingManager::pressed(PAD_H_3));
    press(r, NPISO_R,      MappingManager::pressed(PAD_L_3));
  } else {
    // NES : A, B, Select, Start, Up, Down, Left, Right.
    // On réutilise les bits 0..7 du registre série.
    press(r, 0, MappingManager::pressed(PAD_L_2)); // NES A
    press(r, 1, MappingManager::pressed(PAD_L_1)); // NES B
    press(r, 2, MappingManager::pressed(PAD_SELECT));
    press(r, 3, MappingManager::pressed(PAD_START));
    press(r, 4, MappingManager::pressed(PAD_UP));
    press(r, 5, MappingManager::pressed(PAD_DOWN));
    press(r, 6, MappingManager::pressed(PAD_LEFT));
    press(r, 7, MappingManager::pressed(PAD_RIGHT));
  }

  return r;
}

// Équivalent Arduino du backend BlueRetro :
// LATCH montant = chargement parallèle du registre.
void onLatch() {
  shiftReport = cachedReport;
  shiftIndex = 0;
  writeCurrentBit();
}

// CLOCK descendant = bit suivant.
// BlueRetro utilise le SPI matériel pour ça ; ici on l'émule avec une ISR.
void onClock() {
  if (shiftIndex < 0xFF) {
    shiftIndex++;
  }

  writeCurrentBit();
}

void init(bool snesMode) {
  isSnes = snesMode;
  reportBits = isSnes ? SNES_REPORT_BITS : NES_REPORT_BITS;

  pinMode(SNES_LATCH_PIN, INPUT_PULLUP);
  pinMode(SNES_CLOCK_PIN, INPUT_PULLUP);

  dataHigh();

  cachedReport = 0xFFFF;
  shiftReport  = 0xFFFF;
  shiftIndex   = 0;

  detachInterrupt(digitalPinToInterrupt(SNES_LATCH_PIN));
  detachInterrupt(digitalPinToInterrupt(SNES_CLOCK_PIN));

  // Conforme au comportement SNES/4021 et proche BlueRetro :
  // latch sur front montant.
  attachInterrupt(digitalPinToInterrupt(SNES_LATCH_PIN), onLatch, RISING);
  attachInterrupt(digitalPinToInterrupt(SNES_CLOCK_PIN), onClock, FALLING);
}

void apply() {
  const uint16_t r = buildReport();

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    cachedReport = r;
  }
}

const char* buttonName(int index) {
  switch (index) {
    case PAD_UP:     return isSnes ? "SNES_UP"     : "NES_UP";
    case PAD_DOWN:   return isSnes ? "SNES_DOWN"   : "NES_DOWN";
    case PAD_LEFT:   return isSnes ? "SNES_LEFT"   : "NES_LEFT";
    case PAD_RIGHT:  return isSnes ? "SNES_RIGHT"  : "NES_RIGHT";

    case PAD_H_1:    return isSnes ? "SNES_X"      : "NES_UNUSED";
    case PAD_H_2:    return isSnes ? "SNES_Y"      : "NES_UNUSED";
    case PAD_H_3:    return isSnes ? "SNES_L"      : "NES_UNUSED";

    case PAD_L_1:    return isSnes ? "SNES_B"      : "NES_B";
    case PAD_L_2:    return isSnes ? "SNES_A"      : "NES_A";
    case PAD_L_3:    return isSnes ? "SNES_R"      : "NES_UNUSED";

    case PAD_START:  return isSnes ? "SNES_START"  : "NES_START";
    case PAD_SELECT: return isSnes ? "SNES_SELECT" : "NES_SELECT";
    default:         return "UNKNOWN";
  }
}

}