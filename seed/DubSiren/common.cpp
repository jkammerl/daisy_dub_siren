#include "common.h"

#include "daisy_seed.h"

DaisySeed& DaisyHw::Get() {
  static DaisySeed* hw = new DaisySeed();
  return *hw;
}
