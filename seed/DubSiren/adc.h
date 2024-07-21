#pragma once
#ifndef ADC_H
#define ADC_H

#include "daisy_seed.h"
#include "daisysp.h"

constexpr const int kNumKnobs = 12;

// Reads Knob position from AD converters.
class FilteredAdc {
 public:
  void Init(daisy::DaisySeed* seed);

  // Reads all knob values and stores they state in values member.
  void UpdateValues();

  float GetValue(int i) { return values[i]; }

  int GetInt(int idx) { return seed_->adc.Get(idx); };

 private:
  daisy::DaisySeed* seed_;

  float values[kNumKnobs];
};

#endif  // ADC_H