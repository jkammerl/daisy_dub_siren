// #pragma GCC push_options
// #pragma GCC optimize ("O0")

#include <cmath>
#include <list>
#include <memory>
#include <vector>

#include "adc.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "delay.h"
#include "low_high_pass.h"
#include "sample_manager.h"
#include "switches.h"

using namespace daisy;
using namespace daisysp;

constexpr int kSirenPitchKnob = 0;

constexpr int kLfoSpeedKnob = 2;

constexpr int kSampleSelect = 1;

// constexpr int kDelayLengthKnob = 2;

// DaisyPatchSM hw;
DaisySeed hw;
TimerHandle timer;
Switches switches;

Oscillator osc;
Adsr adsr;
Adsr extern_echo_adsr;

Oscillator lfo_osc;
Delay delay;
Limiter limiter_left;
Limiter limiter_right;

FilteredAdc fadc;

SampleManager sample_manager;

GPIO my_led;

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

  float freq_knob = fadc.GetValue(kSirenPitchKnob);
  float lfo_knob = fadc.GetValue(kLfoSpeedKnob);
  float lfo_depth_knob = 0.9f;

  float delay_mix = switches.Pressed(8) ? 0.0f : 0.7f;
  float delay_length = 0.3f;  // switches.Pressed(8) ? 0.3f : 0.8f;
  float delay_feedback = switches.Pressed(7) ? 0.3f : 0.7f;

  constexpr std::array<int, 3> kLfoWaveforms = {
      Oscillator::WAVE_RAMP, Oscillator::WAVE_POLYBLEP_SAW,
      Oscillator::WAVE_POLYBLEP_SQUARE};
  int lfo_wave_idx = 0;
  bool pitch_up_mod = false;
  bool pitch_down_mod = false;
  static bool pitch_mod_start = false;
  if (switches.Pressed(0)) {
    pitch_down_mod = true;
    lfo_wave_idx = 1;
  } else if (switches.Pressed(1)) {
    lfo_wave_idx = 2;
  } else if (switches.Pressed(2)) {
    pitch_up_mod = true;
    lfo_wave_idx = 2;
  } else if (switches.Pressed(3)) {
    pitch_up_mod = true;
    lfo_wave_idx = 1;
  }
  if (!pitch_mod_start && lfo_wave_idx != 0) {
    g_pitch_add = 0;
  }
  pitch_mod_start = lfo_wave_idx != 0;

  static bool siren_active = false;
  if (switches.Pressed(5)) {
    if (!siren_active) {
      adsr.Retrigger(/*hard=*/false);
      lfo_osc.Reset();
      g_pitch_add = 0.0;
    }
    siren_active = true;
  } else {
    siren_active = false;
  }

  if (pitch_up_mod || pitch_down_mod) {
    const float pitch_knob_delta = (1.0f - g_pitch_add) * g_pitch_up_factor;
    g_pitch_add += pitch_knob_delta;
  }

  if (switches.Clicked(4)) {
    float sample_val = fmap(fadc.GetValue(kSampleSelect), 0.0, 0.99);
    int num_samples = sample_manager.GetNumSamples();
    sample_triggered = sample_val * num_samples;
  }

  delay.SetFeedback(delay_feedback);
  delay.SetTargetDelay(delay_length);

  float lfo_freq = fmap(lfo_knob, 0.1f, 25.f);
  float lfo_amp = fmap(lfo_depth_knob, 0.3f, 1.f);

  lfo_osc.SetAmp(lfo_amp);

  lfo_osc.SetWaveform(kLfoWaveforms[lfo_wave_idx]);
  lfo_osc.SetFreq(lfo_freq);

  for (size_t i = 0; i < size; i++) {
    float adsr_vol = adsr.Process(siren_active);
    osc.SetAmp(adsr_vol * 0.5f);

    float lfo = 1.0 + lfo_osc.Process();
    float freq_knob_mod = freq_knob;
    if (pitch_up_mod) {
      freq_knob_mod = freq_knob + (1.0f - freq_knob) * g_pitch_add;
    } else if (pitch_down_mod) {
      freq_knob_mod = freq_knob - freq_knob * g_pitch_add;
    }

    float freq_hz = fmap(LinToLog(freq_knob_mod), 30.f, 4000.f);
    float freq_lfo = std::min(std::max(freq_hz * lfo, 15.0f), 9000.f);
    osc.SetFreq(freq_lfo);

    const float sample = sample_manager.GetSample();
    float sig = osc.Process() + sample;
    sig *= 0.8;
    float delay_sig = delay.Process(sig) * delay_mix;

    float out_left = sig + delay_sig;
    float out_right = sig + delay_sig;

    float outl = out_left;
    float outr = out_right;

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

  my_led.Init(seed::D11, GPIO::Mode::OUTPUT);

  sample_manager.Init(&hw);
  hw.PrintLine("Num samples found: %d", sample_manager.GetNumSamples());

  fadc.Init(&hw);
  switches.Init(&hw, hw.AudioCallbackRate());

  delay.Init();
  limiter_left.Init();
  limiter_right.Init();

  // hw.PrintLine("System sample rate: %f", hw.AudioSampleRate());

  g_pitch_up_factor = 1.0f / (hw.AudioSampleRate() * 3);

  osc.Init(hw.AudioSampleRate());
  osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
  osc.SetFreq(440);
  adsr.Init(hw.AudioSampleRate());
  // Set envelope parameters
  adsr.SetTime(ADSR_SEG_ATTACK, .01);
  adsr.SetTime(ADSR_SEG_DECAY, .1);
  adsr.SetTime(ADSR_SEG_RELEASE, .01);
  adsr.SetTime(ADSR_SEG_RELEASE, 0.5f);

  adsr.SetSustainLevel(1.0);

  lfo_osc.Init(hw.AudioSampleRate());

  extern_echo_adsr.Init(hw.AudioCallbackRate());
  extern_echo_adsr.SetTime(ADENV_SEG_ATTACK, .05);
  extern_echo_adsr.SetTime(ADENV_SEG_DECAY, .05);
  extern_echo_adsr.SetTime(ADSR_SEG_RELEASE, .05);
  extern_echo_adsr.SetTime(ADSR_SEG_RELEASE, .05);
  extern_echo_adsr.SetSustainLevel(1.0);

  hw.StartAudio(AudioCallback);

  uint32_t freq = timer.GetFreq();
  uint32_t lastTime = timer.GetTick();
  bool led = false;
  while (1) {
    uint32_t newTick = timer.GetTick();
    float interval_sec = (float)(newTick - lastTime) / (float)freq;
    if (interval_sec > 1.f) {
      OutputStats();
      lastTime = newTick;
      my_led.Write(led);
      led = !led;
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
