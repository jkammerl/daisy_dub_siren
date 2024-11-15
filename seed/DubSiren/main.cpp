// #pragma GCC push_options
// #pragma GCC optimize ("O0")

#include <cmath>
#include <list>
#include <memory>
#include <vector>

#include "adc.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "sample_manager.h"
#include "sample_scanner.h"
#include "sample_search.h"
#include "switches.h"

using namespace daisy;
using namespace daisysp;

constexpr int kJoystickX = 0;
constexpr int kJoystickY = 1;
float joystick_x = 0.0f;
float joystick_y = 0.0f;
bool sample_triggered = false;

// DaisyPatchSM hw;
DaisySeed hw;
TimerHandle timer;
Switches switches;

Limiter limiter_left;
Limiter limiter_right;
FilteredAdc fadc;

SampleScanner sample_scanner;
SampleSearch sample_search;

SampleManager sample_manager;

// float LinToLog(float input) {
//   if (input < 0.5) {
//     return input * 0.2;
//   }
//   return input * 1.8 - 0.8;
// }
float LinToLog(float input) {
  constexpr const float m = 0.1f;
  constexpr const float b = (1.0f / m - 1.0f) * (1.0f / m - 1.0f);
  constexpr const float a = 1.0f / (b - 1.0f);
  return a * powf(b, input) - a;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  switches.Update();

  fadc.UpdateValues();

  joystick_x = fadc.GetValue(kJoystickX);
  joystick_y = fadc.GetValue(kJoystickY);

  if (switches.Clicked(1)) {
    sample_triggered = true;
  }

  for (size_t i = 0; i < size; i++) {
    const float sample = sample_manager.GetSample();
    OUT_L[i] = sample;
    OUT_R[i] = sample;
  }

  limiter_left.ProcessBlock(OUT_L, size, /*pre_gain=*/1.0f);
  limiter_right.ProcessBlock(OUT_R, size, /*pre_gain=*/1.0f);
}

void OutputStats() {
  for (int i = 0; i < 12; ++i) {
    hw.PrintLine("Knob: %d, %d", i, fadc.GetInt(i));
  }
  for (int i = 0; i < 11; ++i) {
    hw.PrintLine("Switch: %i %s", i, switches.Pressed(i) ? "yes" : "no");
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(16);

  TimerHandle::Config config;
  config.dir = TimerHandle::Config::CounterDir::UP;
  config.periph = TimerHandle::Config::Peripheral::TIM_2;
  timer.Init(config);
  timer.Start();
  hw.StartLog(true);
  hw.PrintLine("Daisy Patch SM started.");

  sample_scanner.Init(&hw);
  sample_search.Init();
  hw.PrintLine("Search is ready.");

  sample_manager.Init(&hw);

  fadc.Init(&hw);
  switches.Init(&hw, hw.AudioCallbackRate());
  limiter_left.Init();
  limiter_right.Init();

  hw.PrintLine("System sample rate: %f", hw.AudioSampleRate());

  hw.StartAudio(AudioCallback);

  uint32_t freq = timer.GetFreq();
  uint32_t lastTime = timer.GetTick();
  while (1) {
    uint32_t newTick = timer.GetTick();
    float interval_sec = (float)(newTick - lastTime) / (float)freq;
    if (interval_sec > 1.f) {
      OutputStats();
      lastTime = newTick;
    }

    hw.DelayMs(1);
    if (sample_triggered) {
      const SampleInfo& sample_info =
          sample_search.Lookup(joystick_x, joystick_y);
      if (sample_manager.TriggerSample(sample_info, true) != 0) {
        hw.PrintLine("Error triggering sample");
      }
      sample_triggered = false;
    }

    if (int err = sample_manager.SdCardLoading()) {
      hw.PrintLine("SdCard loading error: %d", err);
    }
  }
}
