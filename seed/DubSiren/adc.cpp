#include "adc.h"

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Filter intensity:
constexpr const float kKnobAlpha = 0.01f;
constexpr const float kInvKnobAlpha = 1.0f - kKnobAlpha;

void FilteredAdc::Init(daisy::DaisySeed* seed) {
  seed_ = seed;
  AdcChannelConfig adc[kNumKnobs];
  adc[0].InitSingle(seed::A0);
  adc[1].InitSingle(seed::A1);
  adc[2].InitSingle(seed::A2);
  adc[3].InitSingle(seed::A3);
  adc[4].InitSingle(seed::A4);
  adc[5].InitSingle(seed::A5);
  adc[6].InitSingle(seed::A6);
  adc[7].InitSingle(seed::A7);
  adc[8].InitSingle(seed::A8);
  adc[9].InitSingle(seed::A9);
  adc[10].InitSingle(seed::A10);
  adc[11].InitSingle(seed::A11);
  seed_->adc.Init(adc, kNumKnobs);
  seed_->adc.Start();

  for (int i = 0; i < kNumKnobs; ++i) {
    values[i] = 0.0f;
  }
}

void FilteredAdc::UpdateValues() {
  for (int i = 0; i < kNumKnobs; ++i) {
    const float reading = seed_->adc.GetFloat(i);
    values[i] = values[i] * (kInvKnobAlpha) + kKnobAlpha * reading;
  }
}
