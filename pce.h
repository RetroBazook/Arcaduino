#pragma once
namespace PCE {
  enum PadMode {
    PCE_2_BUTTONS,
    PCE_3_BUTTONS,
    PCE_6_BUTTONS
  };

  void init();
  void apply();
  void nextMode();
  const char* modeName();
  const char* buttonName(int index);
}
