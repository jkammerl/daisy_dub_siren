#include "adc.h"

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

void FilteredAdc::Init(daisy::DaisySeed* seed) {
    seed_ = seed;
    AdcChannelConfig adc[7];  
    adc[0].InitSingle(seed::A0);
    adc[1].InitSingle(seed::A1);
    adc[2].InitSingle(seed::A2);
    adc[3].InitSingle(seed::A3);
    adc[4].InitSingle(seed::A4);
    adc[5].InitSingle(seed::A5);
    adc[6].InitSingle(seed::A6);
    seed_->adc.Init(adc, 7);  
    seed_->adc.Start();   

    for (int i=0; i<kNumKnobs; ++i) {
        values[i] = 0.0f;
    } 
}

void FilteredAdc::UpdateValues() {
    for (int i=0; i<kNumKnobs; ++i) {
        const float reading = seed_->adc.GetFloat(i);
        values[i] = values[i] * (kInvKnobAlpha) + kKnobAlpha * reading;
    }    
}
