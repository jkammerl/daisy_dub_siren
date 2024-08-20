// #pragma GCC push_options
// #pragma GCC optimize ("O0")

#include <cmath>
#include <list>
#include <memory>
#include <vector>

#include "adc.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "sample_analyzer.h"
#include "sample_manager.h"
#include "switches.h"

using namespace daisy;
using namespace daisysp;

constexpr int kSirenPitchKnob = 9;
constexpr int kLfoWave = 8;
constexpr int kLfoDepthKnob = 7;
constexpr int kLfoSpeedKnob = 6;

constexpr int kDecay = 5;
constexpr int kSampleSelect = 4;

constexpr int kDelayLengthKnob = 3;
constexpr int kDelayFeedbackKnob = 2;
constexpr int kDelayMixKnob = 1;
constexpr int kFilterKnob = 0;

// constexpr int kReverbFeedback = 11;
constexpr int kVolumeKnob = 10;
constexpr int kExternVolume = 11;

// DaisyPatchSM hw;
DaisySeed hw;
TimerHandle timer;
Switches switches;

Switch button, toggle;
Limiter limiter_left;
Limiter limiter_right;

SampleAnalyzer sample_analyzer;

// ReverbSc verb;

FilteredAdc fadc;

SampleManager sample_manager;

float g_pitch_up_factor = 1.0f;
float g_pitch_add = 0.0f;

int sample_triggered = -1;

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

  float amp = LinToLog(fadc.GetValue(kVolumeKnob));
  float extern_volume = LinToLog(fadc.GetValue(kExternVolume));
  if (extern_volume < 0.01f) {
    extern_volume = 0.0f;
  }

  if (switches.Clicked(1)) {
    float sample_val = fmap(fadc.GetValue(kSampleSelect), 0.0, 0.99);
    int num_samples = sample_manager.GetNumSamples();
    sample_triggered = sample_val * num_samples;
  }

  for (size_t i = 0; i < size; i++) {
    float in_left = IN_L[i];
    float in_right = IN_R[i];

    float sample;
    sample_manager.GetSample(&sample);

    float outl = sample + in_left * extern_volume;
    float outr = sample + in_right * extern_volume;

    OUT_L[i] = outl;
    OUT_R[i] = outr;
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
  hw.SetAudioBlockSize(1);

  TimerHandle::Config config;
  config.dir = TimerHandle::Config::CounterDir::UP;
  config.periph = TimerHandle::Config::Peripheral::TIM_2;
  timer.Init(config);
  timer.Start();
  hw.StartLog(false);
  hw.PrintLine("Daisy Patch SM started.");

  sample_manager.Init(&hw);
  hw.PrintLine("Num samples found: %d", sample_manager.GetNumSamples());

  fadc.Init(&hw);
  switches.Init(&hw, hw.AudioCallbackRate());

  limiter_left.Init();
  limiter_right.Init();

  hw.PrintLine("System sample rate: %f", hw.AudioSampleRate());

  g_pitch_up_factor = 1.0f / (hw.AudioSampleRate() * 3);

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
    if (sample_triggered >= 0) {
      sample_manager.TriggerSample(sample_triggered, /*retrigger=*/true);
      hw.PrintLine("Trigger: %d, %d", sample_triggered,
                   fadc.GetInt(kSampleSelect));
      sample_triggered = -1;
    }

    if (int err = sample_manager.SdCardLoading()) {
      hw.PrintLine("SdCard loading error: %d", err);
    }
  }
}
