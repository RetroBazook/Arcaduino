#pragma once
namespace MegaDrive {
  enum PadMode {
    MD_3_BUTTONS,
    MD_6_BUTTONS
  };

  void init6();
  void apply6();
  void nextMode();
  const char* modeName();
  const char* buttonName(int index);
}
