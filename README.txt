Arcaduino

Multi-console USB controller adapter for retro gaming

Arcaduino is an open-source Arduino project that allows modern USB arcade encoders and gamepads (Zero Delay, generic HID controllers, arcade sticks, etc.) to be used on multiple retro gaming consoles.

The project currently supports:

Neo Geo AES / MVS / CD
Sega Mega Drive / Genesis
NEC PC Engine / TurboGrafx-16
NES
SNES

Experimental / work-in-progress support:

PlayStation / PS2
Nintendo GameCube
Features
USB HID controller support through USB Host Shield
Multi-console architecture
Open-drain output simulation for safe retro interfacing
Turbo buttons
Runtime controller mode switching
Mega Drive 3-button / 6-button switching
PC Engine 2-button / 3-button / 6-button switching
Clean modular architecture (*.cpp / *.h)
Arcade panel-oriented button mapping
Lightweight and low-latency design
Hardware Requirements
Mandatory
Arduino Uno / Nano
USB Host Shield MAX3421E
USB arcade encoder or USB gamepad
Retro console cable / connector
Recommended
External 5V power supply for Arduino + USB controller
Shared ground with console
Powered USB hub (optional but recommended)
Important Electrical Notes
Open-drain outputs

Arcaduino simulates open-drain outputs using:

OUTPUT LOW = button pressed
INPUT       = button released

This is important because most retro consoles already include pull-up resistors internally.

This design:

prevents bus conflicts,
improves compatibility with multitaps,
protects console hardware.
Powering

Recommended setup:

External 5V PSU
 ├── Arduino
 └── USB encoder

Only connect:

GND Arduino <-> GND console

Do NOT power the Arduino directly from the console controller port unless you know the console regulator can handle it.

Supported Consoles
Neo Geo

Supported systems:

AES
MVS
Neo Geo CD

Features:

A/B/C/D
Turbo A / Turbo B
START / SELECT
MENU combo

Button layout:

H1 = A
H2 = C
H3 = Turbo A

L1 = B
L2 = D
L3 = Turbo B
Mega Drive / Genesis

Supported modes:

3-button
6-button

PAD_MENU toggles:

MD 3-button mode
MD 6-button mode
3-button mode

Top row becomes turbo buttons:

H1 = Turbo A
H2 = Turbo B
H3 = Turbo C

L1 = A
L2 = B
L3 = C
6-button mode
H1 = X
H2 = Y
H3 = Z

L1 = A
L2 = B
L3 = C
PC Engine / TurboGrafx-16

Supported modes:

2-button
3-button
6-button

PAD_MENU cycles modes.

2-button mode
H2 = Turbo II
H3 = Turbo I

L2 = II
L3 = I
3-button mode
H1 = Turbo III
H2 = Turbo II
H3 = Turbo I

L1 = III
L2 = II
L3 = I
6-button mode
H1 = IV
H2 = V
H3 = VI

L1 = III
L2 = II
L3 = I

The implementation uses proper PC Engine multiplexing with SEL and CLR.

NES / SNES

Supported through serial protocol emulation:

LATCH
CLOCK
DATA

The same implementation supports:

NES
SNES

SNES mapping:

H1 = X
H2 = Y
H3 = L

L1 = B
L2 = A
L3 = R
Experimental Support
PS2

Experimental.

The PlayStation controller protocol is timing-sensitive and may not be fully reliable on Arduino Uno + USB Host Shield.

A RP2040 or dedicated MCU is recommended for serious PS2 support.

GameCube

Experimental.

The GameCube protocol requires sub-microsecond timing and is not guaranteed to work reliably on AVR hardware.

Project Structure
arcaduino/
├── arcaduino.ino
├── pad_state.cpp
├── pad_state.h
├── hidjoystickrptparser.cpp
├── hidjoystickrptparser.h
├── neogeo.cpp
├── neogeo.h
├── megadrive.cpp
├── megadrive.h
├── pce.cpp
├── pce.h
├── snes.cpp
├── snes.h
├── ps2.cpp
├── ps2.h
├── gamecube.cpp
└── gamecube.h
USB Controller Mapping

The project uses a generic arcade-panel abstraction:

PAD_H_1
PAD_H_2
PAD_H_3

PAD_L_1
PAD_L_2
PAD_L_3

This allows consistent mappings across all supported consoles.

Dependencies

Install:

USB Host Shield Library 2.0

Arduino Library Manager:

USB Host Shield Library 2.0

Repository:

https://github.com/felis/USB_Host_Shield_2.0
Compiling

Tested with:

Arduino Uno
Arduino Nano

Board settings:

Board: Arduino Uno
Processor: ATmega328P
Known Limitations
PS2 support is experimental
GameCube support is experimental
Mega Drive 6-button mode may be incompatible with some old games
USB polling depends on USB Host Shield timing
Future Ideas
Saturn support
Dreamcast support
N64 support
OLED configuration menu
EEPROM configuration saving
Auto console detection
RP2040 version
Bluetooth controller support
Credits

Inspired by:

original retro controller protocols,
USB Host Shield community projects,
arcade DIY communities,
retro hardware documentation.

Special thanks:

NeoGeoDev Wiki
GameSX
Sega Retro
PCE development documentation
License

MIT License

Disclaimer

Use at your own risk.

Always verify:

voltage levels,
grounds,
controller pinouts,
console compatibility

before connecting hardware.