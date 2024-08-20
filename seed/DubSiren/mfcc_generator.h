#pragma once
#ifndef MEL_FILTER_BANK_H_
#define MEL_FILTER_BANK_H_

#include <array>
#include <cmath>
#include <numeric>
#include <vector>

#include "arm_math.h"
#include "constants.h"
#include "transforms.h"

class MfccGenerator {
 public:
  void Init() {
    filters_ = ComputeMelFilters(kNyquist, kPowerSpecSize);
    temp_products_.resize(kPowerSpecSize);
    fft_.Init();
    mel_dct_.Init();
  }

  std::array<float, kNumMelBands> ComputeMfccs(
      const std::array<float, kAnalysisSize>& buffer) {
    fft_.ComputeSpectrum(buffer, &power_spec_);
    ComputeLogMelEnergy(power_spec_, &log_mel_energy_);
    mel_dct_.Transform(&log_mel_energy_);
    std::array<float, kNumMelBands> result;
    FastFloatArrayCopy(log_mel_energy_.data(), result.data(),
                       log_mel_energy_.size());
    return result;
  }

 private:
  struct TriangleMelFilter {
    int start_bin;
    std::vector<float> coefs;
  };

  static inline float Hz2Mel(float hz) {
    return 2595.0 * log10(1.0 + hz / 700.0);
  }

  static inline float Mel2Hz(float mel) {
    return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);
  }

  float DotProduct(const float* vec_a, const float* vec_b, int size) {
    arm_mult_f32(const_cast<float*>(vec_a), const_cast<float*>(vec_b),
                 temp_products_.data(), size);
    float result = 0.0f;
    for (int i = 0; i < size; i++) {
      arm_add_f32(&result, &temp_products_[i], &result, 1);
    }
    return result;
  }

  void ComputeLogMelEnergy(
      const std::array<float, kPowerSpecSize>& power_spectrum,
      std::array<float, kNumMelBands>* log_mel_energy) {
    for (int i = 0; i < kNumMelBands; ++i) {
      const TriangleMelFilter& filter = filters_[i];
      (*log_mel_energy)[i] =
          log(std::max(DotProduct(&power_spectrum[filter.start_bin],
                                  filter.coefs.data(), filter.coefs.size()),
                       std::numeric_limits<float>::epsilon()));
    }
  }

  static std::array<TriangleMelFilter, kNumMelBands> ComputeMelFilters(
      float nyquist_hz, int num_bins) {
    const float low_mel = Hz2Mel(kMelLowHz);
    const float high_mel = Hz2Mel(kMelHighHz);
    const float mel_step = (high_mel - low_mel) / float(kNumMelBands + 1);
    std::vector<int> bin_points(kNumMelBands + 2);
    // compute points evenly spaced in mels
    for (int i = 0; i < (int)bin_points.size(); i++) {
      const float mel_center = low_mel + i * mel_step;
      bin_points[i] =
          static_cast<int>(Mel2Hz(mel_center) / nyquist_hz * num_bins);
    }

    std::array<TriangleMelFilter, kNumMelBands> triangle_filterbank;
    for (int i = 0; i < kNumMelBands; ++i) {
      TriangleMelFilter& filter = triangle_filterbank[i];
      filter.start_bin = bin_points[i];
      const int end_bin = bin_points[i + 2];

      const int first_triangle_width = bin_points[i + 1] - bin_points[i];
      const int second_triangle_width = bin_points[i + 2] - bin_points[i + 1];

      // Ensure the range is valid
      if (end_bin <= filter.start_bin || first_triangle_width <= 0 ||
          second_triangle_width <= 0) {
        filter.coefs.clear();  // Or handle error
        continue;
      }

      filter.coefs.resize(end_bin - filter.start_bin);
      // first half of triangle
      const float first_triangle_width_float =
          static_cast<float>(first_triangle_width);
      for (int j = 0; j < first_triangle_width; ++j) {
        filter.coefs[j] = static_cast<float>(j) / first_triangle_width_float;
      }
      const float second_triangle_width_float =
          static_cast<float>(second_triangle_width);
      for (int j = 0; j < second_triangle_width; ++j) {
        filter.coefs[first_triangle_width + j] =
            static_cast<float>(second_triangle_width - j) /
            second_triangle_width_float;
      }
    }
    return triangle_filterbank;
  }

  FFT<kAnalysisSize> fft_;
  std::array<float, kPowerSpecSize> power_spec_;
  std::array<float, kNumMelBands> log_mel_energy_;

  std::array<TriangleMelFilter, kNumMelBands> filters_;
  std::vector<float> temp_products_;

  DCT<kNumMelBands> mel_dct_;
};

#endif  // MEL_FILTER_BANK_H_