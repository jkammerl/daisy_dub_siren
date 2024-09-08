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

constexpr int kSampleSelect = 1;

constexpr int kSirenPitchKnob = 2;
constexpr int kLfoWave = 3;
// constexpr int kLfoDepthKnob = 7;
constexpr int kLfoSpeedKnob = 4;

// constexpr int kDecay = 5;
// constexpr int kSampleSelect = 4;

constexpr int kDelayLengthKnob = 6;
constexpr int kDelayFeedbackKnob = 7;
constexpr int kDelayMixKnob = 9;
constexpr int kFilterKnob = 8;

constexpr int kVolumeKnob = 5;
constexpr int kExternVolume = 11;

// DaisyPatchSM hw;
DaisySeed hw;
TimerHandle timer;
Switches switches;

// OnePole freq_filter;

Oscillator osc;
Adsr adsr;
Adsr extern_echo_adsr;

Oscillator lfo_osc;
Switch button, toggle;
Delay delay;
Limiter limiter_left;
Limiter limiter_right;

stmlib::Svf siren_high_filter_;

Limiter input_limiter_left;
Limiter input_limiter_right;

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

int GetSampleBank(int int_val) {
  static constexpr std::array<int, 6> kPos = {0,     12994,  26071,
                                              39056, 52359, 64680};
  int index = 0;
  int smallest_diff = std::numeric_limits<int>::max();
  for (int i = 0; i < kPos.size(); ++i) {
    const int pos_diff = abs(kPos[i] - int_val);
    if (pos_diff < smallest_diff) {
      smallest_diff = pos_diff;
      index = i;
    }
  }
  return index;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  switches.Update();

  fadc.UpdateValues();

  float amp = LinToLog(fadc.GetValue(kVolumeKnob));
  float freq_knob = fadc.GetValue(kSirenPitchKnob);
  float lfo_knob = fadc.GetValue(kLfoSpeedKnob);
  float lfo_depth_knob = 0.9;  // fadc.GetValue(kLfoDepthKnob);
  float decay_knob = 0.2;      // fadc.GetValue(kDecay);

  float lfo_waveform = fmap(fadc.GetValue(kLfoWave), 0., 0.99);
  float sample_volume = LinToLog(fadc.GetValue(kExternVolume));
  if (sample_volume < 0.01f) {
    sample_volume = 0.0f;
  }
  float delay_mix = LinToLog(fadc.GetValue(kDelayMixKnob));
  float delay_length = LinToLog(fadc.GetValue(kDelayLengthKnob));
  float delay_feedback = fmap(fadc.GetValue(kDelayFeedbackKnob), 0.1, 0.92);
  float filter_val = fmap(fadc.GetValue(kFilterKnob), 0.001, 0.999);

  static bool siren_active = false;
  if (switches.Pressed(9)) {
    if (!siren_active) {
      adsr.Retrigger(/*hard=*/false);
      lfo_osc.Reset();
      g_pitch_add = 0.0;
    }
    siren_active = true;
  } else {
    siren_active = false;
  }

  static bool extern_delay_in = false;
  if (switches.Pressed(11)) {
    if (!extern_delay_in) {
      extern_echo_adsr.Retrigger(/*hard=*/false);
    }
    extern_delay_in = true;
  } else {
    extern_delay_in = false;
  }

  // if (switches.Pressed(3)) {
  //   const float pitch_knob_delta = (1.0f - g_pitch_add) * g_pitch_up_factor;
  //   g_pitch_add += pitch_knob_delta;
  // }

  int sample_bank = GetSampleBank(fadc.GetInt(kSampleSelect));
  int sample_select = -1;
  if (switches.Clicked(5)) {
    sample_select = 0;
  } else if (switches.Clicked(6)) {
    sample_select = 1;
  } else if (switches.Clicked(7)) {
    sample_select = 2;
  } else if (switches.Clicked(8)) {
    sample_select = 3;
  }

  if (sample_select >= 0) {
    sample_triggered = sample_bank * 4 + sample_select;
  }

  delay.SetFrequency(filter_val);

  delay.SetFeedback(delay_feedback);
  delay.SetTargetDelay(delay_length);

  float lfo_freq = fmap(lfo_knob, 0.1f, 25.f);
  float lfo_amp = fmap(lfo_depth_knob, 0.3f, 1.f);

  adsr.SetTime(ADSR_SEG_RELEASE, fmap(decay_knob, 0.01f, 1.f));

  lfo_osc.SetAmp(lfo_amp);

  constexpr std::array<int, 3> kLfoWaveforms = {
      Oscillator::WAVE_RAMP, Oscillator::WAVE_POLYBLEP_SAW,
      Oscillator::WAVE_POLYBLEP_SQUARE};
  int lfo_wave_idx = static_cast<int>(lfo_waveform * 3.0f);
  if (lfo_wave_idx == 2) {
    lfo_amp = 0.2f;
  }

  lfo_osc.SetWaveform(kLfoWaveforms[lfo_wave_idx]);
  lfo_osc.SetFreq(lfo_freq);

  for (size_t i = 0; i < size; i++) {
    // float in_left = 0.9 * ((rand() % 2 == 0) ? 0.99f : -0.99f);   // IN_L[i];
    // float in_right = 0.9 * ((rand() % 2 == 0) ? 0.99f : -0.99f);  // IN_R[i];
    float in_left = IN_L[i];
    float in_right = IN_R[i];

    float adsr_vol = adsr.Process(siren_active);
    float extern_echo_adsr_value = extern_echo_adsr.Process(extern_delay_in);
    float echo_in_mono = (in_left + in_right) / 2.0f * extern_echo_adsr_value;
    osc.SetAmp(adsr_vol);

    float lfo = 1.0 + lfo_osc.Process();
    // float freq_knob_mod;
    // if (freq_knob < 0.5) {
    //   freq_knob_mod = freq_knob + (1.0f - freq_knob) * g_pitch_add;
    // } else {
    //   freq_knob_mod = freq_knob - freq_knob * g_pitch_add;
    // }

    float freq_hz = fmap(LinToLog(freq_knob), 30.f, 4000.f);
    float freq_lfo = std::min(std::max(freq_hz * lfo, 15.0f), 9000.f);
    osc.SetFreq(freq_lfo);

    const float sample = sample_manager.GetSample() * sample_volume;
    // float osc_out =
    //     siren_high_filter_.Process<stmlib::FILTER_MODE_LOW_PASS>(osc.Process());
    float osc_out = osc.Process();
    float sig = osc_out * adsr_vol * amp + sample;
    ;

    float delay_input = sig + echo_in_mono;
    float delay_sig = (delay.Process(delay_input)) * delay_mix;

    float out_left = sig + delay_sig;
    float out_right = sig + delay_sig;

    OUT_L[i] = out_left;
    OUT_R[i] = out_right;
  }

  limiter_left.ProcessBlock(OUT_L, size, /*pre_gain=*/1.0f);
  limiter_right.ProcessBlock(OUT_R, size, /*pre_gain=*/1.0f);
}

void OutputStats() {
  for (int i = 0; i < 12; ++i) {
    hw.PrintLine("Knob: %d, %d", i, fadc.GetInt(i));
  }
  for (int i = 0; i < 12; ++i) {
    hw.PrintLine("Switch: %i %s", i, switches.Pressed(i) ? "yes" : "no");
  }
  hw.PrintLine("SB: %i", GetSampleBank(fadc.GetInt(kSampleSelect)));
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(16);

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

  delay.Init(hw.AudioSampleRate());
  limiter_left.Init();
  limiter_right.Init();
  input_limiter_left.Init();
  input_limiter_right.Init();

  siren_high_filter_.Init();
  siren_high_filter_.set_f_q<stmlib::FREQUENCY_ACCURATE>(0.1, 1.0);

  hw.PrintLine("System sample rate: %d", (int)hw.AudioSampleRate());

  g_pitch_up_factor = 1.0f / (hw.AudioSampleRate() * 3);

  // b

  osc.Init(hw.AudioSampleRate());
  osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
  osc.SetFreq(440);
  adsr.Init(hw.AudioSampleRate());
  // Set envelope parameters
  adsr.SetTime(ADSR_SEG_ATTACK, .01);
  adsr.SetTime(ADSR_SEG_DECAY, .1);
  adsr.SetTime(ADSR_SEG_RELEASE, .01);
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
