#include <Arduino.h>
#include "pad_state.h"
#include "mapping_manager.h"
#include "snes.h"

namespace SNES {

#define SNES_LATCH_PIN 2
#define SNES_CLOCK_PIN 3
#define SNES_DATA_PIN  4

// D4 = DATA
static constexpr uint8_t DATA_MASK = _BV(PD4);

static constexpr uint8_t SNES_REPORT_BITS = 16;
static constexpr uint8_t NES_REPORT_BITS  = 8;

volatile bool     isSnes        = true;
volatile uint16_t latchedReport = 0xFFFF;
volatile uint16_t cachedReport  = 0xFFFF;
volatile uint8_t  Si            = 0;
volatile uint8_t  reportBits    = SNES_REPORT_BITS;

static bool oldPhysicalL = false;
static bool oldPhysicalR = false;
static unsigned long pulseLUntil = 0;
static unsigned long pulseRUntil = 0;

static inline void dataLow() {
  PORTD &= ~DATA_MASK;
  DDRD  |=  DATA_MASK;   // sortie LOW
}

static inline void dataHigh() {
  PORTD &= ~DATA_MASK;   // pas de pull-up interne
  DDRD  &= ~DATA_MASK;   // entrée = ligne relâchée
}

static inline void writeDataBit(bool b) {
  if (b) dataHigh();
  else dataLow();
}

static uint16_t buildReport(bool oneShotLActive, bool oneShotRActive) {
  uint16_t r = 0xFFFF;

  if (isSnes) {
    if (MappingManager::pressed(PAD_L_1))    r &= ~(1 << 0);   // B
    if (MappingManager::pressed(PAD_H_2))    r &= ~(1 << 1);   // Y
    if (MappingManager::pressed(PAD_SELECT)) r &= ~(1 << 2);   // Select
    if (MappingManager::pressed(PAD_START))  r &= ~(1 << 3);   // Start
    if (MappingManager::pressed(PAD_UP))     r &= ~(1 << 4);   // Up
    if (MappingManager::pressed(PAD_DOWN))   r &= ~(1 << 5);   // Down
    if (MappingManager::pressed(PAD_LEFT))   r &= ~(1 << 6);   // Left
    if (MappingManager::pressed(PAD_RIGHT))  r &= ~(1 << 7);   // Right
    if (MappingManager::pressed(PAD_L_2))    r &= ~(1 << 8);   // A
    if (MappingManager::pressed(PAD_H_1))    r &= ~(1 << 9);   // X
    if (oneShotLActive)                      r &= ~(1 << 10);  // L
    if (oneShotRActive)                      r &= ~(1 << 11);  // R
  } else {
    if (MappingManager::pressed(PAD_L_2))    r &= ~(1 << 0);   // A
    if (MappingManager::pressed(PAD_L_1))    r &= ~(1 << 1);   // B
    if (MappingManager::pressed(PAD_SELECT)) r &= ~(1 << 2);
    if (MappingManager::pressed(PAD_START))  r &= ~(1 << 3);
    if (MappingManager::pressed(PAD_UP))     r &= ~(1 << 4);
    if (MappingManager::pressed(PAD_DOWN))   r &= ~(1 << 5);
    if (MappingManager::pressed(PAD_LEFT))   r &= ~(1 << 6);
    if (MappingManager::pressed(PAD_RIGHT))  r &= ~(1 << 7);
  }

  return r;
}

// LATCH FALLING : on fige le report et on présente B immédiatement
void onLatch() {
  latchedReport = cachedReport;
  Si = 0;
  writeDataBit(latchedReport & 1);
}

// CLOCK RISING : on prépare le bit suivant
void onClock() {
  Si++;

  if (Si < reportBits) {
    writeDataBit((latchedReport >> Si) & 1);
  } else {
    dataHigh();
  }
}

void init(bool snesMode) {
  isSnes = snesMode;
  reportBits = isSnes ? SNES_REPORT_BITS : NES_REPORT_BITS;

  pinMode(SNES_LATCH_PIN, INPUT);
  pinMode(SNES_CLOCK_PIN, INPUT);

  dataHigh();

  cachedReport  = 0xFFFF;
  latchedReport = 0xFFFF;
  Si = 0;

  oldPhysicalL = false;
  oldPhysicalR = false;
  pulseLUntil = 0;
  pulseRUntil = 0;

  detachInterrupt(digitalPinToInterrupt(SNES_LATCH_PIN));
  detachInterrupt(digitalPinToInterrupt(SNES_CLOCK_PIN));

  attachInterrupt(digitalPinToInterrupt(SNES_LATCH_PIN), onLatch, FALLING);
  attachInterrupt(digitalPinToInterrupt(SNES_CLOCK_PIN), onClock, RISING);
}

void apply() {
  unsigned long now = millis();

  bool physL = MappingManager::pressed(PAD_H_3);
  if (physL && !oldPhysicalL) {
    pulseLUntil = now + 40;
  }
  oldPhysicalL = physL;

  bool physR = MappingManager::pressed(PAD_L_3);
  if (physR && !oldPhysicalR) {
    pulseRUntil = now + 40;
  }
  oldPhysicalR = physR;

  bool oneShotLActive = now < pulseLUntil;
  bool oneShotRActive = now < pulseRUntil;

  uint16_t report = buildReport(oneShotLActive, oneShotRActive);

  noInterrupts();
  cachedReport = report;
  interrupts();
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