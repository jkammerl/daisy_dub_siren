#include "switches.h"

#include <array>

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

constexpr std::array<Pin, kNumSwitches> kSwitchPins = {
    seed::D26, seed::D27, seed::D7,  seed::D8,  seed::D9,  seed::D10,
    seed::D11, seed::D12, seed::D13, seed::D14, seed::D15, seed::A10};

void Switches::Init(daisy::DaisySeed* seed, float audio_callback_rate) {
  seed_ = seed;
  switches_.resize(kNumSwitches);
  for (int i = 0; i < kNumSwitches; ++i) {
    switches_[i].Init(kSwitchPins[i], audio_callback_rate);
    last_switch_state_[i] = false;
    switch_down_event_[i] = false;
  }
}

void Switches::Update() {
  for (int i = 0; i < kNumSwitches; ++i) {
    switches_[i].Debounce();
    const bool switch_state = switches_[i].Pressed();
    switch_down_event_[i] = switch_state && !last_switch_state_[i];
    last_switch_state_[i] = switch_state;
  }
}

bool Switches::Pressed(int switch_idx) const {
  return switches_[switch_idx].Pressed();
}

bool Switches::Clicked(int switch_idx) const {
  return switch_down_event_[switch_idx];
}
