#pragma once
#ifndef FFT_H_
#define FFT_H_

#include <array>

#include "arm_math.h"

template <int FftSize>
class FFT {
 public:
  void Init() { arm_rfft_fast_init_f32(&fft_instance_, FftSize); }

  void ComputeSpectrum(std::array<float, FftSize>* buffer) {
    arm_rfft_fast_f32(&fft_instance_, buffer->data(), freq_domain_.data(),
                      /*ifftFlag=*/0);
    arm_cmplx_mag_f32(freq_domain_.data(), freq_power_.data(),
                      freq_power_.size());
    // arm_rfft_fast_f32 writes DC and higest bin into index 0 & 1, ignore it.
    freq_power_[0] = 0.0f;
  }

 private:
  static constexpr int kNumBins = FftSize / 2;
  arm_rfft_fast_instance_f32 fft_instance_;
  std::array<float, FftSize> freq_domain_;
  std::array<float, kNumBins> freq_power_;
};

#endif  // FFT_H_