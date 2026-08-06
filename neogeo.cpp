#include <Arduino.h>
#include "pad_state.h"
#include "neogeo.h"
#include "mapping_manager.h"

namespace NeoGeo {

#define NG_UP_PIN      2
#define NG_DOWN_PIN    3
#define NG_LEFT_PIN    4
#define NG_RIGHT_PIN   5

#define NG_A_PIN       6
#define NG_B_PIN       7
#define NG_C_PIN       8
#define NG_D_PIN       9

#define NG_START_PIN   A0
#define NG_SELECT_PIN  A1

void init()
{
  setLine(NG_UP_PIN, false);
  setLine(NG_DOWN_PIN, false);
  setLine(NG_LEFT_PIN, false);
  setLine(NG_RIGHT_PIN, false);

  setLine(NG_A_PIN, false);
  setLine(NG_B_PIN, false);
  setLine(NG_C_PIN, false);
  setLine(NG_D_PIN, false);

  setLine(NG_START_PIN, false);
  setLine(NG_SELECT_PIN, false);
}

void apply()
{
  setLine(NG_UP_PIN, MappingManager::pressed(PAD_UP));
  setLine(NG_DOWN_PIN, MappingManager::pressed(PAD_DOWN));
  // Diagnostic: the field "b2" printed by sendObsState() is PAD_L_2.
  // Make that exact physical input activate LEFT as well.
  setLine(NG_LEFT_PIN, MappingManager::pressed(PAD_LEFT) ||
                       MappingManager::rawPressed(PAD_L_2));
  setLine(NG_RIGHT_PIN, MappingManager::pressed(PAD_RIGHT));

  setLine(NG_A_PIN, MappingManager::pressed(PAD_H_1));
  setLine(NG_B_PIN, MappingManager::pressed(PAD_L_1));
  setLine(NG_C_PIN, MappingManager::pressed(PAD_H_2));
  setLine(NG_D_PIN, MappingManager::pressed(PAD_L_2));

  setLine(NG_START_PIN, MappingManager::pressed(PAD_START));
  setLine(NG_SELECT_PIN, MappingManager::pressed(PAD_SELECT));
}

const char* buttonName(int index)
{
  switch (index) {
    case PAD_UP: return "NG_UP";
    case PAD_DOWN: return "NG_DOWN";
    case PAD_LEFT: return "NG_LEFT";
    case PAD_RIGHT: return "NG_RIGHT";

    case PAD_H_1: return "NG_A";
    case PAD_L_1: return "NG_B";
    case PAD_H_2: return "NG_C";
    case PAD_L_2: return "NG_D";

    case PAD_START: return "NG_START";
    case PAD_SELECT: return "NG_SELECT";
    case PAD_MENU: return "NG_MENU";

    default: return "UNKNOWN";
  }
}

}