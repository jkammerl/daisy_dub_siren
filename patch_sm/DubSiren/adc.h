#pragma once
#ifndef ADC_H
#define ADC_H

#include "daisy_seed.h"
#include "daisysp.h"

constexpr const int kNumKnobs = 7;
constexpr const float kKnobAlpha = 0.01f;
constexpr const float kInvKnobAlpha = 1.0f - kKnobAlpha;

class FilteredAdc {
  public:
    void Init(daisy::DaisySeed* seed);

    void UpdateValues();

    float GetValue(int i) {
        return values[i];
    }

  private:
    daisy::DaisySeed* seed_;

    float values[kNumKnobs];
};


#endif // ADC_H