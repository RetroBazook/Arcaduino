#include <SPI.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

#include "hidjoystickrptparser.h"
#include "pad_state.h"
#include "neogeo.h"
#include "megadrive.h"
#include "snes.h"
#include "pce.h"
#include "ps2.h"
#include "gamecube.h"
#include "hackpad_generic.h"
#include "mapping_manager.h"

USB Usb;
USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);

JoystickEvents JoyEvents;
JoystickReportParser Joy(&JoyEvents);

OutputMode outputMode = MODE_NEOGEO;
//OutputMode outputMode = MODE_MD6;
//OutputMode outputMode = MODE_NES;
//OutputMode outputMode = MODE_SNES;
//OutputMode outputMode = MODE_PCE;
//OutputMode outputMode = MODE_PS2;
//OutputMode outputMode = MODE_GAMECUBE;
//OutputMode outputMode = MODE_HACK_GENERIC;

OutputMode getOutputMode() {
  return outputMode;
}

const char* buttonName(int index) {
  if (outputMode == MODE_MD6) return MegaDrive::buttonName(index);
  if (outputMode == MODE_PCE) return PCE::buttonName(index);
  if (outputMode == MODE_NES || outputMode == MODE_SNES) return SNES::buttonName(index);
  if (outputMode == MODE_HACK_GENERIC) return HackPadGeneric::buttonName(index);
  return NeoGeo::buttonName(index);
}

void initOutput() {
  if (outputMode == MODE_HACK_GENERIC) {
    HackPadGeneric::init();
    Serial.println("Output mode: Hackpad");
  } else if (outputMode == MODE_NEOGEO) {
    NeoGeo::init();
    Serial.println("Output mode: NeoGeo");
  } else if (outputMode == MODE_MD6) {
    MegaDrive::init6();
    Serial.println("Output mode: Mega Drive 6 buttons");
  } else if (outputMode == MODE_NES) {
    SNES::init(false);
    Serial.println("Output mode: NES");
  } else if (outputMode == MODE_SNES) {
    SNES::init(true);
    Serial.println("Output mode: SNES");
  } else if (outputMode == MODE_PCE) {
    PCE::init();
    Serial.println("Output mode: PC Engine");
  } else if (outputMode == MODE_PS2) {
    PS2Pad::init();
    Serial.println("Output mode: PS2 experimental");
  } else if (outputMode == MODE_GAMECUBE) {
    GameCube::init();
    Serial.println("Output mode: GameCube skeleton");
  } else {
    NeoGeo::init();
    Serial.println("Output mode: fallback NeoGeo");
  }
}

void applyOutput() {
  if (outputMode == MODE_HACK_GENERIC) {
    HackPadGeneric::apply();
  } else if (outputMode == MODE_NEOGEO) {
    NeoGeo::apply();
  } else if (outputMode == MODE_MD6) {
    MegaDrive::apply6();
  } else if (outputMode == MODE_NES || outputMode == MODE_SNES) {
    SNES::apply();
  } else if (outputMode == MODE_PCE) {
    PCE::apply();
  } else if (outputMode == MODE_PS2) {
    PS2Pad::apply();
  } else if (outputMode == MODE_GAMECUBE) {
    GameCube::apply();
  }
}

void sendObsState() {
  static bool initialized = false;
  static bool lastState[12];

  bool state[12] = {
    wanted[PAD_UP],
    wanted[PAD_DOWN],
    wanted[PAD_LEFT],
    wanted[PAD_RIGHT],

    wanted[PAD_L_1],
    wanted[PAD_L_2],
    wanted[PAD_L_3],

    wanted[PAD_H_1],
    wanted[PAD_H_2],
    wanted[PAD_H_3],

    wanted[PAD_START],
    wanted[PAD_SELECT]
  };

  bool changed = !initialized;

  if (initialized) {
    for (uint8_t i = 0; i < 12; i++) {
      if (state[i] != lastState[i]) {
        changed = true;
        break;
      }
    }
  }

  if (!changed) return;

  initialized = true;

  for (uint8_t i = 0; i < 12; i++) {
    lastState[i] = state[i];
  }

  Serial.print("{");

  Serial.print("\"up\":");     Serial.print(state[0]);
  Serial.print(",\"down\":");  Serial.print(state[1]);
  Serial.print(",\"left\":");  Serial.print(state[2]);
  Serial.print(",\"right\":"); Serial.print(state[3]);

  Serial.print(",\"b1\":"); Serial.print(state[4]);
  Serial.print(",\"b2\":"); Serial.print(state[5]);
  Serial.print(",\"b3\":"); Serial.print(state[6]);

  Serial.print(",\"b4\":"); Serial.print(state[7]);
  Serial.print(",\"b5\":"); Serial.print(state[8]);
  Serial.print(",\"b6\":"); Serial.print(state[9]);

  Serial.print(",\"start\":");  Serial.print(state[10]);
  Serial.print(",\"select\":"); Serial.print(state[11]);

  Serial.println("}");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Start");

  MappingManager::init();

  initOutput();

  if (Usb.Init() == -1) {
    Serial.println("USB Host Shield KO / OSC did not start.");
    while (1);
  }

  Serial.println("USB Host Shield OK");
  delay(200);

  if (!Hid.SetReportParser(0, &Joy)) {
    ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
  }

  sendObsState();
}

void loop() {
  Usb.Task();

  MappingManager::update();

  updateDebounce();
  updateTurbo();

  if (!MappingManager::isRemapping()) {
    applyOutput();
  }

  sendObsState();
}