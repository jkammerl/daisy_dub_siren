// #pragma GCC push_options
// #pragma GCC optimize ("O0")

#include <array>
#include <cmath>
#include <list>
#include <memory>
#include <vector>

#include "adc.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "recorder.h"
#include "switches.h"

using namespace daisy;
using namespace daisysp;

// DaisyPatchSM hw;
DaisySeed hw;
TimerHandle timer;
Switches switches;

constexpr int kRecLenSamples = static_cast<int>(96000 * 20.f);
constexpr int kNumRecorder = 3;

Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderl0;
Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderr0;
Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderl1;
Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderr1;
Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderl2;
Recorder<float, kRecLenSamples> DSY_SDRAM_BSS g_recorderr2;
std::array<Recorder<float, kRecLenSamples>*, kNumRecorder> g_recorder_l = {
    &g_recorderl0, &g_recorderl1, &g_recorderl2};
std::array<Recorder<float, kRecLenSamples>*, kNumRecorder> g_recorder_r = {
    &g_recorderr0, &g_recorderr1, &g_recorderr2};

Limiter limiter_left;
Limiter limiter_right;

FilteredAdc fadc;

Oscillator test_sound;
float g_speed;

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

  for (int i = 0; i < kNumRecorder; ++i) {
    if (switches.Clicked(i)) {
      g_recorder_l[i]->Reset();
      g_recorder_r[i]->Reset();
    }
    if (switches.Clicked(i + kNumRecorder)) {
      g_recorder_l[i]->Reset();
      g_recorder_r[i]->Reset();
    }
  }

  for (size_t i = 0; i < size; i++) {
    float in_left = IN_L[i];
    float in_right = IN_R[i];
    // float mono = (in_left + in_right) / 2.0f;

    float out_sample_l = 0.0f;
    float out_sample_r = 0.0f;

    // float osc = test_sound.Process();
    // in_left = osc;
    // in_right = osc;

    for (int i = 0; i < kNumRecorder; ++i) {
      if (switches.Pressed(i)) {
        g_recorder_l[i]->Write(in_left);
        g_recorder_r[i]->Write(in_right);
        // g_recorder[i]->Write(mono);
      } else {
        const float volume = LinToLog(1.0f - fadc.GetValue(i * 2 + 0));
        float speed = 0.0;
        if (i == 2) {
          speed = (1.0f - fadc.GetValue(i * 2 + 1) - 0.5f) * 6.0f;
        } else {
          speed = (1.0f - fadc.GetValue(i * 2 + 1) - 0.5f) * 3.0f;
        }
        g_speed = speed;
        out_sample_l += g_recorder_l[i]->Read(speed) * volume;
        out_sample_r += g_recorder_r[i]->Read(speed) * volume;
      }
    }
    const float in_volume = LinToLog(1.0f - fadc.GetValue(kNumRecorder * 2));

    OUT_L[i] = out_sample_l;
    OUT_R[i] = out_sample_r;

    OUT_L[i] += in_left * in_volume;
    OUT_R[i] += in_right * in_volume;
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
  hw.SetAudioBlockSize(64);
  hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_96KHZ);

  TimerHandle::Config config;
  config.dir = TimerHandle::Config::CounterDir::UP;
  config.periph = TimerHandle::Config::Peripheral::TIM_2;
  timer.Init(config);
  timer.Start();
  hw.StartLog(false);
  hw.PrintLine("Daisy Patch SM started.");

  fadc.Init(&hw);
  switches.Init(&hw, hw.AudioCallbackRate());

  test_sound.Init(hw.AudioSampleRate());
  test_sound.SetWaveform(Oscillator::WAVE_SIN);
  // test_sound.SetFreq(440);
  // test_sound.SetAmp(1.0);

  for (int i = 0; i < kNumRecorder; ++i) {
    g_recorder_l[i]->Init();
    g_recorder_r[i]->Init();
  }
  limiter_left.Init();
  limiter_right.Init();

  hw.PrintLine("System sample rate: %d", (int)hw.AudioSampleRate());

  hw.StartAudio(AudioCallback);

  uint32_t freq = timer.GetFreq();
  uint32_t lastTime = timer.GetTick();
  while (1) {
    uint32_t newTick = timer.GetTick();
    float interval_sec = (float)(newTick - lastTime) / (float)freq;
    if (interval_sec > 1.f) {
      OutputStats();
      lastTime = newTick;
      hw.PrintLine("g_speed: %d", (int)(g_speed * 10000.));
    }

    hw.DelayMs(1);
  }
}
