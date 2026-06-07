#pragma once
#include <Arduino.h>
#include "pad_state.h"

namespace MappingManager {

void init();
void update();

// Return the current remapped state for a logical output button.
// Backends should use this instead of reading wanted[] directly.
bool pressed(uint8_t logicalButton);

// Raw debounced state, without remapping.
// Useful for shortcuts, remap learning, and debug.
bool rawPressed(uint8_t physicalButton);

const char* padName(uint8_t pad);
const char* outputName(uint8_t outputButton);
void printCurrentMapping();

}
