#pragma once
#ifndef DELAY_H
#define DELAY_H

#include <vector>
#include <string>

#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t kMaxDelay = static_cast<size_t>(48000 * 2.f);

DelayLine<float, kMaxDelay> DSY_SDRAM_BSS g_memory;

class Delay {
    public:
    void Init() {
        g_memory.Init();
    }

    void SetTargetDelay(float delay) {
        delay_target_ = delay * kMaxDelay;
    }

   void SetFeedback(float feedback) {
        feedback_ = feedback;
    }

    float Process(float in) {
        //set delay times
        fonepole(current_delay_, delay_target_, .0002f);
        g_memory.SetDelay(current_delay_);

        float read = g_memory.Read();
        g_memory.Write((feedback_ * read) + in);
        return read;
    }

private:

    float                        current_delay_ = kMaxDelay / 2;
    float                        delay_target_ = kMaxDelay / 2;
    float                        feedback_ = 0.0f;
};

#endif // DELAY_H