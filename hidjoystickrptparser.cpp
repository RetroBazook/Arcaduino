#include "hidjoystickrptparser.h"

JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
joyEvents(evt),
oldHat(0xDE),
oldButtons(0)
{
  for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++) oldPad[i] = 0xD;
}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
  bool match = true;

  for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++) {
    if (buf[i] != oldPad[i]) {
      match = false;
      break;
    }
  }

  if (!match && joyEvents) {
    joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
    for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++) oldPad[i] = buf[i];
  }

  uint8_t hat = buf[5] & 0x0F;

  if (hat != oldHat && joyEvents) {
    joyEvents->OnHatSwitch(hat);
    oldHat = hat;
  }

  uint16_t buttons = buf[6];
  buttons <<= 4;
  buttons |= (buf[5] >> 4);
  uint16_t changes = buttons ^ oldButtons;

  if (changes) {
    for (uint8_t i = 0; i < 12; i++) {
      uint16_t mask = 1 << i;
      if ((changes & mask) && joyEvents) {
        if (buttons & mask) joyEvents->OnButtonDn(i + 1);
        else joyEvents->OnButtonUp(i + 1);
      }
    }
    oldButtons = buttons;
  }
}

uint8_t oldXButtons = 0;
uint8_t oldYButtons = 0;
uint8_t oldZ2 = 0xFF;
uint8_t oldRz = 0xFF;

const int8_t x1Map[9] = {
  -1,
  PAD_H_2,   // X1 bit 1
  PAD_L_2,   // X1 bit 2
  PAD_L_1,   // X1 bit 3
  PAD_H_1,   // X1 bit 4
  PAD_H_3,   // X1 bit 5
  PAD_L_3,   // X1 bit 6
  -1,        // X1 bit 7
  PAD_MENU   // X1 bit 8
};

const int8_t y1Map[9] = {
  -1,
  PAD_START,   // Y1 bit 1
  PAD_SELECT,  // Y1 bit 2
  -1,
  -1,
  -1,
  -1,
  -1,
  -1
};

void handleBitmaskButtons(uint8_t value, uint8_t *oldValue, const int8_t *map)
{
  uint8_t changes = value ^ *oldValue;

  for (uint8_t bit = 1; bit <= 8; bit++) {
    uint8_t mask = 1 << (bit - 1);
    if (changes & mask) {
      if (map[bit] != -1) {
        bool isDown = value & mask;
        pushButton(map[bit], isDown);
      }
    }
  }
  *oldValue = value;
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{
  handleBitmaskButtons(evt->X, &oldXButtons, x1Map);
  handleBitmaskButtons(evt->Y, &oldYButtons, y1Map);

  if (evt->Z2 != oldZ2) {
    if (evt->Z2 == 0x00) {
      pushButton(PAD_LEFT, true);
      pushButton(PAD_RIGHT, false);
    } else if (evt->Z2 == 0xFF) {
      pushButton(PAD_LEFT, false);
      pushButton(PAD_RIGHT, true);
    } else if (evt->Z2 == 0x80) {
      pushButton(PAD_LEFT, false);
      pushButton(PAD_RIGHT, false);
    }
    oldZ2 = evt->Z2;
  }

  if (evt->Rz != oldRz) {
    if (evt->Rz == 0x00) {
      pushButton(PAD_UP, true);
      pushButton(PAD_DOWN, false);
    } else if (evt->Rz == 0xFF) {
      pushButton(PAD_UP, false);
      pushButton(PAD_DOWN, true);
    } else if (evt->Rz == 0x80) {
      pushButton(PAD_UP, false);
      pushButton(PAD_DOWN, false);
    }
    oldRz = evt->Rz;
  }
}

void JoystickEvents::OnHatSwitch(uint8_t hat) { }
void JoystickEvents::OnButtonUp(uint8_t but_id) { }
void JoystickEvents::OnButtonDn(uint8_t but_id) { }
