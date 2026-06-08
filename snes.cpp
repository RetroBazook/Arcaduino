#include <Arduino.h>
#include "pad_state.h"
#include "mapping_manager.h"
#include "snes.h"

namespace SNES {

#define SNES_LATCH_PIN 2
#define SNES_CLOCK_PIN 3
#define SNES_DATA_PIN  4

static constexpr uint8_t REPORT_BITS = 16;

bool isSnes = true;
uint16_t shiftReg = 0xFFFF;
uint8_t bitIndex = 0;

bool lastLatch = LOW;
bool lastClock = HIGH;

void writeDataBit(bool releasedHigh) {
  // true  = released / HIGH / Hi-Z
  // false = pressed / LOW
  setLine(SNES_DATA_PIN, !releasedHigh);
}

uint16_t buildReport() {
  uint16_t r = 0xFFFF;

  if (isSnes) {
    // SNES: B,Y,Select,Start,Up,Down,Left,Right,A,X,L,R,1,1,1,1
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
    // NES: A,B,Select,Start,Up,Down,Left,Right,1,1,1,1,1,1,1,1
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

void init(bool snesMode) {
  isSnes = snesMode;

  pinMode(SNES_LATCH_PIN, INPUT);
  pinMode(SNES_CLOCK_PIN, INPUT);

  // DATA released/high before first poll
  writeDataBit(true);

  shiftReg = 0xFFFF;
  bitIndex = 0;

  lastLatch = digitalRead(SNES_LATCH_PIN);
  lastClock = digitalRead(SNES_CLOCK_PIN);
}

void apply() {
  const bool latch = digitalRead(SNES_LATCH_PIN);
  const bool clock = digitalRead(SNES_CLOCK_PIN);

  // LATCH falling edge => first bit
  if (!latch && lastLatch) {
    shiftReg = buildReport();
    bitIndex = 0;
    writeDataBit((shiftReg >> bitIndex) & 1);
  }

  // CLOCK rising edge => next bit
  if (!latch && lastClock == LOW && clock == HIGH) {
    bitIndex++;

    if (bitIndex < REPORT_BITS) {
      writeDataBit((shiftReg >> bitIndex) & 1);
    } else {
      writeDataBit(false); // DATA LOW après 16 bits
    }
  }

  lastLatch = latch;
  lastClock = clock;
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