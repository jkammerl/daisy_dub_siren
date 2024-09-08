#pragma once
#ifndef LOW_HIGH_PASS_
#define LOW_HIGH_PASS_

#include "filter.h"

class LowHighPass {
 public:
  void Init(float q) {
    q_ = q;
    Reset();
  }

  void Reset() {
    low_filter_.Init();
    high_filter_.Init();
    SetFrequency(0.5f);
  }

  void SetFrequency(float knob_value) {
    constexpr float kWiggle = 0.15;
    float frequency;
    if (fabs(knob_value - 0.5) < kWiggle) {
      low_filter_.set_f_q<stmlib::FREQUENCY_FAST>(1.0f, q_);
      high_filter_.set_f_q<stmlib::FREQUENCY_FAST>(1.0f, q_);
      high_low_pass_ = false;
      return;
    }
    high_low_pass_ = knob_value > 0.5;
    if (high_low_pass_) {
      // Highpass
      frequency = (knob_value * 0.8 - kWiggle - 0.5f);
    } else {
      frequency = knob_value;
    }
    // frequency = fmap(frequency * 2, 0.1, 0.3);
    frequency = std::min(0.99f, std::max(0.01f, frequency));
    low_filter_.set_f_q<stmlib::FREQUENCY_FAST>(frequency, q_);
    high_filter_.set_f_q<stmlib::FREQUENCY_FAST>(frequency, q_);
  }

  void ApplyFilter(float& sig) {
    float low = low_filter_.Process<stmlib::FILTER_MODE_LOW_PASS>(sig);
    float high = high_filter_.Process<stmlib::FILTER_MODE_HIGH_PASS>(sig);

    if (!high_low_pass_) {
      sig = low;
    } else {
      sig = high;
    }
    if ((sig != sig)) {
      sig = 0.0f;
    }
  }

 private:
  bool high_low_pass_ = false;

  stmlib::Svf low_filter_;
  stmlib::Svf high_filter_;
  float q_ = 0.9f;
};

#endif  // LOW_HIGH_PASS_