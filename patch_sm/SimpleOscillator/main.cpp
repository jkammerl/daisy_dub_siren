// #pragma GCC push_options
// #pragma GCC optimize ("O0")

#include <list>
#include <memory>
#include <vector>

#include "adc.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "delay.h"
#include "sample_manager.h"

using namespace daisy;
using namespace daisysp;

#define MAX_DELAY static_cast<size_t>(48000 * 5.f)

// DaisyPatchSM patch;
DaisySeed patch;
Oscillator osc;
Oscillator lfo_osc;
AdEnv siren_env;
Switch button, toggle;
Delay delay;
Limiter limiter_left;
Limiter limiter_right;

Switch s0, s1, s2;

FilteredAdc fadc;

SampleManager sample_manager_;

bool sample_triggered = false;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  button.Debounce();
  toggle.Debounce();

  s0.Debounce();
  s1.Debounce();
  s2.Debounce();

  fadc.UpdateValues();

  uint8_t waveform = 0;
  uint8_t lfo_waveform = 0;

  bool siren_active = false;

  if (s0.Pressed()) {
    waveform = Oscillator::WAVE_TRI;
    lfo_waveform = Oscillator::WAVE_SAW;
    siren_active = true;
  } else if (s1.Pressed()) {
    waveform = Oscillator::WAVE_POLYBLEP_SAW;
    lfo_waveform = Oscillator::WAVE_RAMP;
    siren_active = true;
  }
  static bool last_s2_pressed = false;
  bool s2_pressed = s2.Pressed();
  if (!last_s2_pressed && s2_pressed) {
    sample_triggered = true;
  }
  last_s2_pressed = s2_pressed;

  float amp = fadc.GetValue(0);
  float freq_knob = fadc.GetValue(1);
  float lfo_knob = fadc.GetValue(2);
  float lfo_amp = fadc.GetValue(3);
  float delay_mix = fadc.GetValue(4);
  float delay_length = 0.3f;    // fadc.GetValue(4);
  float delay_feedback = 0.4f;  // fadc.GetValue(5);

  delay.SetFeedback(delay_feedback);
  delay.SetTargetDelay(delay_length);

  float freq = fmap(freq_knob, 30.f, 8000.f);
  float lfo_freq = fmap(lfo_knob, 00.f, 30.f);

  lfo_osc.SetAmp(lfo_amp);
  lfo_osc.SetWaveform(lfo_waveform);
  lfo_osc.SetFreq(lfo_freq);
  osc.SetWaveform(waveform);

  for (size_t i = 0; i < size; i++) {
    float in_mono = (IN_L[i] + IN_R[i]) / 2.0f;

    float trigger_vol = siren_active ? 1.0f : 0.0f;
    osc.SetAmp(amp * trigger_vol);

    float lfo = 1.0 + lfo_osc.Process();
    float final_freq = std::min(std::max(freq * lfo, 30.0f), 9000.f);
    osc.SetFreq(final_freq);

    const float sample = sample_manager_.GetSample();
    float sig = osc.Process() + sample;
    float delay_sig = delay.Process(sig + in_mono);

    float mix = (1.0f - delay_mix) * sig + delay_mix * delay_sig;

    OUT_L[i] = mix + IN_L[i];
    OUT_R[i] = mix + IN_R[i];
  }

  limiter_left.ProcessBlock(OUT_L, size, /*pre_gain=*/1.0f);
  limiter_right.ProcessBlock(OUT_R, size, /*pre_gain=*/1.0f);
}

int main(void) {
  patch.Init();
  patch.StartLog(false);
  patch.PrintLine("Daisy Patch SM started.");

  sample_manager_.Init();

  fadc.Init(&patch);

  delay.Init();
  limiter_left.Init();
  limiter_right.Init();

  patch.PrintLine("System sample rate: %f", patch.AudioSampleRate());

  osc.Init(patch.AudioSampleRate());
  osc.SetAmp(1.0f);
  osc.SetFreq(440);
  lfo_osc.Init(patch.AudioSampleRate());

  s0.Init(seed::D25, patch.AudioCallbackRate());
  s1.Init(seed::D26, patch.AudioCallbackRate());
  s2.Init(seed::D27, patch.AudioCallbackRate());

  siren_env.Init(patch.AudioCallbackRate());
  siren_env.SetTime(ADENV_SEG_ATTACK, .01);
  siren_env.SetTime(ADENV_SEG_DECAY, .2);
  siren_env.SetMax(1);
  siren_env.SetMin(0);

  patch.StartAudio(AudioCallback);
  while (1) {
    if (sample_triggered) {
      sample_triggered = false;
      sample_manager_.TriggerSample(0, /*retrigger=*/false);
      patch.PrintLine("Trigger: %d", 0);
    }

    const int err = sample_manager_.SdCardLoading();
    if (err) {
      patch.PrintLine("SdCard loading error: %d", err);
    }
  }
}

// #pragma GCC pop_options