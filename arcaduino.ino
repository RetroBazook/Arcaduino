#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include <SPI.h>

#include "hidjoystickrptparser.h"
#include "pad_state.h"
#include "neogeo.h"
#include "megadrive.h"
#include "snes.h"
#include "pce.h"
#include "ps2.h"
#include "gamecube.h"

USB Usb;
USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);
JoystickEvents JoyEvents;
JoystickReportParser Joy(&JoyEvents);

OutputMode outputMode = MODE_NEOGEO;
// OutputMode outputMode = MODE_MD6;
// OutputMode outputMode = MODE_NES;
// OutputMode outputMode = MODE_SNES;
// OutputMode outputMode = MODE_PCE;
// OutputMode outputMode = MODE_PS2;
// OutputMode outputMode = MODE_GAMECUBE;

OutputMode getOutputMode() {
  return outputMode;
}

const char* buttonName(int index) {
  if (outputMode == MODE_MD6) return MegaDrive::buttonName(index);
  if (outputMode == MODE_PCE) return PCE::buttonName(index);
  if (outputMode == MODE_NES || outputMode == MODE_SNES) return SNES::buttonName(index);
  return NeoGeo::buttonName(index);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Start");

  if (outputMode == MODE_NEOGEO) {
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
  }

  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
    while (1);
  }

  Serial.println("USB Host Shield OK");
  delay(200);

  if (!Hid.SetReportParser(0, &Joy)) {
    ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
  }
}

void loop() {
  Usb.Task();

  updateDebounce();
  updateTurbo();

  if (outputMode == MODE_NEOGEO) {
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
