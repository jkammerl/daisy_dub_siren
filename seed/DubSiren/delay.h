#pragma once
#ifndef DELAY_H
#define DELAY_H

#include <string>
#include <vector>

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t kMaxDelay = static_cast<size_t>(48000 * 1.f);

DelayLine<float, kMaxDelay> DSY_SDRAM_BSS g_memory;

class Delay {
 public:
  void Init(float sample_rate) {
    g_memory.Init();
    echo_filter_.Init(sample_rate);
    echo_filter_.SetRes(0.05f);
  }

  void SetTargetDelay(float delay) { delay_target_ = delay * kMaxDelay; }

  void SetFeedback(float feedback) { feedback_ = feedback; }

  float Process(float in) {
    // set delay times
    fonepole(current_delay_, delay_target_, .0002f);
    g_memory.SetDelay(current_delay_);

    float read = g_memory.Read();
    ApplyFilterEcho(read);
    g_memory.Write((feedback_ * read) + in);
    return read;
  }

  void SetFrequency(float knob_value) {
    constexpr float cutoff_div_3 = 48000. / 3.;
    constexpr float kWiggle = 0.15;
    float frequency;
    if (fabs(knob_value - 0.5) < kWiggle) {
      echo_filter_.SetFreq(cutoff_div_3);
      high_low_pass_ = false;
      return;
    }
    high_low_pass_ = knob_value > 0.5;
    if (high_low_pass_) {
      // Highpass
      frequency = knob_value - kWiggle - 0.5f;
    } else {
      frequency = knob_value;
    }
    const float cutoff_final = frequency * cutoff_div_3;
    echo_filter_.SetFreq(cutoff_final);
  }

 private:
  void ApplyFilterEcho(float& echo) {
    echo_filter_.Process(echo);
    if (high_low_pass_) {
      echo = echo_filter_.High();
    } else {
      echo = echo_filter_.Low();
    }
  }

  float current_delay_ = kMaxDelay / 2;
  float delay_target_ = kMaxDelay / 2;
  float feedback_ = 0.0f;

  bool high_low_pass_ = false;

  daisysp::Svf echo_filter_;
};

#endif  // DELAY_H