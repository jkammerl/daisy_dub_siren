#pragma once
#ifndef FFT_H_
#define FFT_H_

#include <array>
#include <cmath>
#include <complex>

#include "fft.h"

// void FastFloatArrayCopy(float* source, float* destination, int size) {
//   arm_copy_f32(source, destination, size);
// }

// void FastFloatFill(float value, float* destination, int size) {
//   arm_fill_f32(value, destination, size);
// }

template <int Size>
class PowerSpectrum {
 public:
  void Init() {
    fft_.Init();
    window_ = GeneratePeriodicHannWindow();
  }

  // Modifies input buffer
  void ComputeSpectrum(const std::array<float, Size>& buffer,
                       std::array<float, Size / 2 + 1>* power_spec) {
    for (int i = 0; i < Size; ++i) {
      input_[i] = buffer[i] * window_[i];
    }
    fft_.Direct(input_.data(), freq_domain_.data());

    // Handle DC component (bin 0)
    (*power_spec)[0] = freq_domain_[0] * freq_domain_[0];

    // Handle the general case (bins 1 to Size/2 - 1)
    const float* real_ptr = &freq_domain_[1];
    const float* imag_ptr = &freq_domain_[Size / 2 + 1];
    for (int i = 1; i < Size / 2; ++i, ++real_ptr, ++imag_ptr) {
      (*power_spec)[i] = std::norm(std::complex<float>(*real_ptr, *imag_ptr));
    }
    // Handle the Nyquist frequency (bin Size/2)
    (*power_spec)[Size / 2] = freq_domain_[Size / 2] * freq_domain_[Size / 2];
  }

 private:
  // Periodic hanning computes the Haning window of size N+1 and returns the
  // first N weights.
  static std::array<float, Size> GeneratePeriodicHannWindow() {
    std::array<float, Size> window;
    for (int i = 0; i < Size; i++) {
      window[i] = 0.5 * (1 - cos(2.0 * M_PI * i / (Size)));
    }
    return window;
  }

  stmlib::ShyFFT<float, Size> fft_;
  std::array<float, Size> input_;
  std::array<float, Size> window_;
  std::array<float, Size> freq_domain_;
};

template <int Size>
class DCT2 {
 public:
  void Init() { fft_.Init(); }

  void Transform(const std::array<float, Size>& input,
                 std::array<float, Size>* output, bool ortho_norm = true) {
    // Zero-padding and flipping the input
    for (int i = 0; i < Size; ++i) {
      extended_[i] = input[i];
    }
    for (int i = 0; i < Size; ++i) {
      extended_[Size + i] = 0.0f;
    }

    fft_.Direct(extended_.data(), freq_domain_.data());

    // Compute the DCT-II coefficients
    for (int k = 0; k < Size; ++k) {
      std::complex<float> freq_bin;
      if (k == 0) {
        // Bin 0 (real part only)
        freq_bin = std::complex<float>(freq_domain_[0], 0.0f);
      } else if (k == Size / 2) {
        // Bin N/2 (real part only)
        freq_bin = std::complex<float>(freq_domain_[Size / 2], 0.0f);
      } else {
        // Bins 1 to N/2-1
        double real_part = freq_domain_[k];
        double imag_part = freq_domain_[Size + k] *
                           -1;  // ShyFFT provides negative imaginary values.
        freq_bin = std::complex<float>(real_part, imag_part);
      }

      // Compute the multiplier for DCT-II
      std::complex<float> multiplier =
          std::polar<float>(2.0f, -M_PI * k / (2.0f * Size));

      (*output)[k] = (freq_bin * multiplier).real();
      // Compute and normalize DCT-II coefficient
      if (k == 0) {
        (*output)[k] =
            (freq_bin * multiplier).real();  //* std::sqrt(1.0f / Size);
      } else {
        (*output)[k] =
            (freq_bin * multiplier).real();  // * std::sqrt(2.0f / Size);
      }
    }
    if (ortho_norm) {
      (*output)[0] *= std::sqrt(1.0f / (4.0f * Size));
      const float norm_fac = std::sqrt(1.0f / (2.0f * Size));
      for (int k = 1; k < Size; ++k) {
        (*output)[k] *= norm_fac;
      }
    }
  }

 private:
  std::array<float, Size * 2> extended_;
  std::array<float, Size * 2> freq_domain_;
  stmlib::ShyFFT<float, Size * 2> fft_;
};

#endif  // FFT_H_