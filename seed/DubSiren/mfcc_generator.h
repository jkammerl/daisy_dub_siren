#pragma once
#ifndef MEL_FILTER_BANK_H_
#define MEL_FILTER_BANK_H_

#include <array>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "constants.h"
#include "transforms.h"

class MfccGenerator {
 public:
  void Init() {
    filters_ = ComputeMelFilters<kNumMelBands>(kNyquist, kPowerSpecSize);
    temp_products_.resize(kPowerSpecSize);
    ps_.Init();
    mel_dct_.Init();
  }

  std::array<float, kNumMelBands> ComputeMfccs(
      const std::array<float, kAnalysisSize>& buffer) {
    ps_.ComputeSpectrum(buffer, &power_spec_);
    ComputeLogMelEnergy(power_spec_, &log_mel_energy_);
    std::array<float, kNumMelBands> result;
    mel_dct_.Transform(log_mel_energy_, &result);
    return result;
  }

 private:
  struct TriangleMelFilter {
    int start_bin;
    std::vector<float> coefs;
  };

  static inline float PowerToDb(float magnitude) {
    constexpr float kAmin = 1e-10f;
    return 10.0f * std::log10(std::max(kAmin, magnitude));
  }

  static inline float Hz2Mel(float hz, bool htk = false) {
    if (htk) {
      return 2595.0 * log10(1.0 + hz / 700.0);
    }
    // Slaney formula
    constexpr float kFSp = 200.0 / 3.;
    constexpr float kMinLogHz = 1000.0;  // beginning of log region(Hz)
    constexpr float kMinLogMel = kMinLogHz / kFSp;            // same (Mels)
    constexpr float kLogstep = 0.06875177742094911749945930;  //  std::log(6.4)
                                                              //  / 27.0
                                                              //  step
                                                              //  size
                                                              //  for log
                                                              //  region

    if (hz >= kMinLogHz) {
      return kMinLogMel + std::log(hz / kMinLogHz) / kLogstep;
    }
    return hz / kFSp;
  }

  static inline float Mel2Hz(float mel, bool htk = false) {
    if (htk) {
      return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);
    }
    // Slaney formula
    constexpr float kFSp = 200.0 / 3.;
    constexpr float kMinLogHz = 1000.0;  // beginning of log region(Hz)
    constexpr float kMinLogMel = kMinLogHz / kFSp;  // same (Mels)
    constexpr float kLogstep =
        0.06875177742094911749945930;  //  std::log(6.4) / 27.0 step size for
                                       //  log region
    if (mel >= kMinLogMel) {
      return kMinLogHz * std::exp(kLogstep * (mel - kMinLogMel));
    }
    return kFSp * mel;
  }

  float DotProduct(const float* vec_a, const float* vec_b, int size) {
    float result = 0.0f;
    for (int i = 0; i < size; i++) {
      result += vec_a[i] * vec_b[i];
    }
    return result;
  }

  void ComputeLogMelEnergy(
      const std::array<float, kPowerSpecSize>& power_spectrum,
      std::array<float, kNumMelBands>* log_mel_energy) {
    float max_db = std::numeric_limits<float>::min();
    for (int i = 0; i < kNumMelBands; ++i) {
      const TriangleMelFilter& filter = filters_[i];
      const float power_db = PowerToDb(
          std::max(DotProduct(&power_spectrum[filter.start_bin],
                              filter.coefs.data(), filter.coefs.size()),
                   std::numeric_limits<float>::epsilon()));
      max_db = std::max(max_db, power_db);
      (*log_mel_energy)[i] = power_db;
    }
    // Set kTopDb range.
    const float min_db = max_db - kTopDb;
    for (int i = 0; i < kNumMelBands; ++i) {
      (*log_mel_energy)[i] = std::max((*log_mel_energy)[i], min_db);
    }
  }

  template <int kNumMelBands>
  std::array<TriangleMelFilter, kNumMelBands> ComputeMelFilters(
      float nyquist_hz, int num_freq_bins) {
    const float low_mel = Hz2Mel(0);
    const float high_mel = Hz2Mel(nyquist_hz);
    const double mel_step = (high_mel - low_mel) / float(kNumMelBands + 1);
    std::vector<float> mel_center_hz(kNumMelBands + 2);
    // compute points evenly spaced in mels
    for (int i = 0; i < (int)mel_center_hz.size(); i++) {
      const float mel_center = low_mel + i * mel_step;
      mel_center_hz[i] = Mel2Hz(mel_center);
    }

    std::vector<float> ramp(mel_center_hz.size() * num_freq_bins);
    const float bin_step_hz = nyquist_hz / (num_freq_bins - 1);
    for (int c = 0; c < num_freq_bins; ++c) {
      const float bin_center_hz = c * bin_step_hz;
      for (int m = 0; m < static_cast<int>(mel_center_hz.size()); ++m) {
        ramp[m * num_freq_bins + c] = mel_center_hz[m] - bin_center_hz;
      }
    }

    std::vector<float> weights(kNumMelBands * num_freq_bins);
    for (int m = 0; m < kNumMelBands; ++m) {
      const float lower_diff = mel_center_hz[m + 1] - mel_center_hz[m];
      const float upper_diff = mel_center_hz[m + 2] - mel_center_hz[m + 1];

      // Slaney-style mel is scaled to be approx constant energy per channel
      const float enorm = 2.0 / (mel_center_hz[2 + m] - mel_center_hz[m]);
      for (int c = 0; c < num_freq_bins; ++c) {
        const float lower = -ramp[m * num_freq_bins + c] / lower_diff;
        const float upper = ramp[(m + 2) * num_freq_bins + c] / upper_diff;
        weights[m * num_freq_bins + c] =
            std::max(0.0f, std::min(lower, upper)) * enorm;
      }
    }

    std::array<TriangleMelFilter, kNumMelBands> triangle_filterbank;
    for (int m = 0; m < static_cast<int>(triangle_filterbank.size()); ++m) {
      int first_non_zero_coef_idx = -1;
      int num_non_zero_coefs = 0;
      for (int c = 0; c < num_freq_bins; ++c) {
        const float coef = weights[m * num_freq_bins + c];
        if (coef > 0) {
          if (first_non_zero_coef_idx < 0) {
            first_non_zero_coef_idx = c;
          }
          ++num_non_zero_coefs;
        }
      }
      auto& filterbank = triangle_filterbank[m];
      filterbank.start_bin = first_non_zero_coef_idx;
      const float* first_coef =
          &weights[m * num_freq_bins + first_non_zero_coef_idx];
      filterbank.coefs =
          std::vector<float>(first_coef, first_coef + num_non_zero_coefs);
    }
    return triangle_filterbank;
  }

  PowerSpectrum<kAnalysisSize> ps_;
  std::array<float, kPowerSpecSize> power_spec_;
  std::array<float, kNumMelBands> log_mel_energy_;

  std::array<TriangleMelFilter, kNumMelBands> filters_;
  std::vector<float> temp_products_;

  DCT2<kNumMelBands> mel_dct_;
};

#endif  // MEL_FILTER_BANK_H_