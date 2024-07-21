#pragma once
#ifndef LOW_HIGH_PASS_
#define LOW_HIGH_PASS_

#include "daisy_seed.h"
#include "daisysp.h"

class LowHighPass {
 public:
  void Init(float sample_rate) {
    left_filter_.Init(sample_rate);
    right_filter_.Init(sample_rate);
  }

  void SetFrequency(float knob_value) {
    constexpr float cutoff_div_3 = 48000. / 3.;
    constexpr float kWiggle = 0.15;
    float frequency;
    if (fabs(knob_value - 0.5) < kWiggle) {
      left_filter_.SetFreq(cutoff_div_3);
      right_filter_.SetFreq(cutoff_div_3);
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
    left_filter_.SetFreq(frequency * cutoff_div_3);
    right_filter_.SetFreq(frequency * cutoff_div_3);
  }

  void ApplyFilter(float& left, float& right) {
    left_filter_.Process(left);
    right_filter_.Process(right);

    if (high_low_pass_) {
      left = left_filter_.High();
      right = right_filter_.High();
    } else {
      left = left_filter_.Low();
      right = right_filter_.Low();
    }
  }

 private:
  bool high_low_pass_ = false;
  daisysp::Svf left_filter_;
  daisysp::Svf right_filter_;
};

#endif  // LOW_HIGH_PASS_