#pragma once
#ifndef LOUDNESS_NORMALIZER_H_
#define LOUDNESS_NORMALIZER_H_

#include <algorithm>
#include <cmath>

// Dynamic loudness normalizer for individual WAV sample playback.
//
// RMS-based AGC that brings each playing sample to a consistent output level,
// making quiet and loud WAV files equally perceived. Common settings for
// sample-playback loudness control:
//   target  -15 dBFS RMS  (moderate level, headroom for transients)
//   attack   50 ms        (fast gain reduction — tames loud transients)
//   release 500 ms        (slow recovery — avoids pumping between notes)
//   max gain +18 dB / min gain -24 dB
class LoudnessNormalizer {
 public:
  static constexpr float kTargetRms  = 0.211f;   // -13.5 dBFS
  static constexpr float kMaxGain    = 8.0f;     // +18 dB
  static constexpr float kMinGain    = 0.0625f;  // -24 dB

  void Init(float sample_rate) {
    const float sr = (sample_rate > 0.0f) ? sample_rate : 48000.0f;
    rms_coeff_     = expf(-1.0f / (sr * 0.100f));  // 100 ms RMS integration
    attack_coeff_  = expf(-1.0f / (sr * 0.050f));  //  50 ms gain attack
    release_coeff_ = expf(-1.0f / (sr * 0.500f));  // 500 ms gain release
    mean_sq_ = kTargetRms * kTargetRms;             // start at target level
    gain_    = 1.0f;
  }

  // Process one sample [-1, 1]. Returns gain-adjusted output.
  float Process(float input) {
    // Running mean-square estimator.
    mean_sq_ = rms_coeff_ * mean_sq_ + (1.0f - rms_coeff_) * (input * input);
    const float rms = sqrtf(mean_sq_);

    // Desired gain to reach target RMS.
    float target_gain = (rms > 1e-6f) ? (kTargetRms / rms) : kMaxGain;
    target_gain = std::min(target_gain, kMaxGain);
    target_gain = std::max(target_gain, kMinGain);

    // Asymmetric smoothing: fast when reducing gain, slow when boosting.
    const float coeff = (target_gain < gain_) ? attack_coeff_ : release_coeff_;
    gain_ = coeff * gain_ + (1.0f - coeff) * target_gain;

    return input * gain_;
  }

 private:
  float rms_coeff_     = 0.0f;
  float attack_coeff_  = 0.0f;
  float release_coeff_ = 0.0f;
  float mean_sq_       = 0.0445f;  // kTargetRms²
  float gain_          = 1.0f;
};

#endif  // LOUDNESS_NORMALIZER_H_
