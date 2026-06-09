#include <Arduino.h>
#include "pad_state.h"
#include "mapping_manager.h"
#include "snes.h"

namespace SNES {

#define SNES_LATCH_PIN 2
#define SNES_CLOCK_PIN 3
#define SNES_DATA_PIN  4

static constexpr uint8_t REPORT_BITS = 16;
static constexpr uint8_t DATA_MASK = _BV(PD4); // D4 = PD4 sur ATmega328P

volatile bool isSnes = true;
volatile uint16_t shiftReg = 0xFFFF;
volatile uint16_t cachedReport = 0xFFFF;
volatile uint8_t bitIndex = 0;

static inline void dataLow() {
  PORTD &= ~DATA_MASK;
  DDRD  |= DATA_MASK;
}

static inline void dataRelease() {
  DDRD &= ~DATA_MASK;
}

static inline void writeDataBitFast(bool releasedHigh) {
  if (releasedHigh) dataRelease();
  else dataLow();
}

uint16_t buildReport() {
  uint16_t r = 0xFFFF;

  if (isSnes) {
    if (MappingManager::pressed(PAD_L_1))    r &= ~(1 << 0);   // B
    if (MappingManager::pressed(PAD_H_2))    r &= ~(1 << 1);   // Y
    if (MappingManager::pressed(PAD_SELECT)) r &= ~(1 << 2);
    if (MappingManager::pressed(PAD_START))  r &= ~(1 << 3);
    if (MappingManager::pressed(PAD_UP))     r &= ~(1 << 4);
    if (MappingManager::pressed(PAD_DOWN))   r &= ~(1 << 5);
    if (MappingManager::pressed(PAD_LEFT))   r &= ~(1 << 6);
    if (MappingManager::pressed(PAD_RIGHT))  r &= ~(1 << 7);
    if (MappingManager::pressed(PAD_L_2))    r &= ~(1 << 8);   // A
    if (MappingManager::pressed(PAD_H_1))    r &= ~(1 << 9);   // X
    if (MappingManager::pressed(PAD_H_3))    r &= ~(1 << 10);  // L
    if (MappingManager::pressed(PAD_L_3))    r &= ~(1 << 11);  // R
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

// LATCH falling edge => first bit
void onLatchFall() {
  shiftReg = cachedReport;
  bitIndex = 0;
  writeDataBitFast(shiftReg & 1);
}

// CLOCK rising edge => next bit
void onClockRise() {
  bitIndex++;

  if (bitIndex < REPORT_BITS) {
    writeDataBitFast((shiftReg >> bitIndex) & 1);
  } else {
    writeDataBitFast(false); // DATA LOW après 16 bits
  }
}

void init(bool snesMode) {
  isSnes = snesMode;

  pinMode(SNES_LATCH_PIN, INPUT);
  pinMode(SNES_CLOCK_PIN, INPUT);

  dataRelease();

  shiftReg = 0xFFFF;
  cachedReport = 0xFFFF;
  bitIndex = 0;

  attachInterrupt(digitalPinToInterrupt(SNES_LATCH_PIN), onLatchFall, FALLING);
  attachInterrupt(digitalPinToInterrupt(SNES_CLOCK_PIN), onClockRise, RISING);
}

void apply() {
  uint16_t report = buildReport();

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