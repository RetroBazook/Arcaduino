Retro USB adapter Arduino project

Files:
- testusb.ino: main Arduino sketch, USB Host setup and mode selection
- pad_state.h/.cpp: shared button state, debounce, turbo, open-drain output helper
- neogeo.h/.cpp: NeoGeo DB15 output backend
- megadrive.h/.cpp: Mega Drive 6-button output backend
- snes.h/.cpp: NES/SNES serial controller output backend
- hidjoystickrptparser.h/.cpp: USB zero-delay HID parser

Select output mode in testusb.ino:
  OutputMode outputMode = MODE_NEOGEO;
  // OutputMode outputMode = MODE_MD6;
  // OutputMode outputMode = MODE_NES;
  // OutputMode outputMode = MODE_SNES;

Use only one active output mode line.

NES/SNES suggested wiring:
  Arduino D2  -> DATA
  Arduino D3  <- CLOCK
  Arduino D4  <- LATCH
  Arduino GND -> console GND
  Do not connect console +5V if Arduino is externally powered.

SNES mapping:
  PAD_H_1 -> X
  PAD_H_2 -> Y
  PAD_H_3 -> L
  PAD_L_1 -> B
  PAD_L_2 -> A
  PAD_L_3 -> R

NES mapping:
  PAD_L_1 -> B
  PAD_L_2 -> A
  PAD_START -> START
  PAD_SELECT -> SELECT
