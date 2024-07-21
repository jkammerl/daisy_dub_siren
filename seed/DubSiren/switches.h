#pragma once
#ifndef BUTTONS_H
#define BUTTONS_H

#include "daisy_seed.h"
#include "daisysp.h"

constexpr const int kNumSwitches = 11;

class Switches {
 public:
  void Init(daisy::DaisySeed* seed, float audio_callback_rate);

  void Update();

  bool Pressed(int switch_idx) const;

  bool Clicked(int switch_idx) const;

 private:
  daisy::DaisySeed* seed_;

  std::vector<daisy::Switch> switches_;

  std::array<bool, kNumSwitches> last_switch_state_;
  std::array<bool, kNumSwitches> switch_down_event_;
};

#endif  // BUTTONS_H