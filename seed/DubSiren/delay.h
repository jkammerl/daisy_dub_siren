#pragma once
#ifndef DELAY_H
#define DELAY_H

#include <string>
#include <vector>

#include "daisy_seed.h"
#include "daisysp.h"
#include "low_high_pass.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t kMaxDelay = static_cast<size_t>(48000 * 1.f);

DelayLine<float, kMaxDelay> DSY_SDRAM_BSS g_memory;

class Delay {
 public:
  void Init(float sample_rate) {
    g_memory.Init();
    echo_filter_.Init(0.9);
  }

  void SetTargetDelay(float delay) { delay_target_ = delay * kMaxDelay; }

  void SetFeedback(float feedback) { feedback_ = feedback; }

  float Process(float in) {
    // set delay times
    fonepole(current_delay_, delay_target_, .0002f);
    g_memory.SetDelay(current_delay_);

    float read = g_memory.Read();
    echo_filter_.ApplyFilter(read);
    g_memory.Write((feedback_ * read) + in);
    return read;
  }

  void SetFrequency(float knob_value) { echo_filter_.SetFrequency(knob_value); }

 private:
  float current_delay_ = kMaxDelay / 2;
  float delay_target_ = kMaxDelay / 2;
  float feedback_ = 0.0f;

  bool high_low_pass_ = false;

  LowHighPass echo_filter_;
};

#endif  // DELAY_H