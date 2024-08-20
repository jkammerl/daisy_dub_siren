#pragma once
#ifndef FFT_H_
#define FFT_H_

#include <array>

#include "arm_math.h"

void FastFloatArrayCopy(float* source, float* destination, int size) {
  arm_copy_f32(source, destination, size);
}

void FastFloatFill(float value, float* destination, int size) {
  arm_fill_f32(value, destination, size);
}

template <int Size>
class FFT {
 public:
  void Init() { arm_rfft_fast_init_f32(&fft_instance_, Size); }

  void ComputeSpectrum(const std::array<float, Size>& buffer,
                       std::array<float, Size / 2>* power_spec) {
    arm_rfft_fast_f32(&fft_instance_, const_cast<float*>(buffer.data()),
                      freq_domain_.data(),
                      /*ifftFlag=*/0);
    arm_cmplx_mag_f32(freq_domain_.data(), power_spec->data(),
                      power_spec->size());
    // arm_rfft_fast_f32 writes DC and higest bin into index 0 & 1, ignore it.
    (*power_spec)[0] = 0.0f;
  }

 private:
  arm_rfft_fast_instance_f32 fft_instance_;
  std::array<float, Size> freq_domain_;
};

template <int Size>
class DCT {
 public:
  void Init() {
    // Initialize the DCT4 structure
    arm_dct4_init_f32(
        &dct4_instance_,  // DCT4 instance
        &rfft_instance_,  // RFFT instance (used internally)
        &cfft_instance_,  // CFFT instance (used internally)
        Size,             // DCT length
        Size / 2,         // RFFT length (half of DCT_SIZE)
        1.0f / sqrtf(static_cast<float>(Size))  // Normalization factor
    );
  }

  void Transform(std::array<float, Size>* in_out) {
    arm_dct4_f32(&dct4_instance_, temp_state_.data(), in_out->data());
  }

 private:
  // Initialize DCT4/IDCT4 instance
  arm_dct4_instance_f32 dct4_instance_;
  arm_rfft_instance_f32 rfft_instance_;
  arm_cfft_radix4_instance_f32 cfft_instance_;
  std::array<float, Size> temp_state_;
};

#endif  // FFT_H_