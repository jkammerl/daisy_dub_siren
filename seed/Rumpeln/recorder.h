#pragma once
#ifndef RECORDER_H
#define RECORDER_H

#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

template <typename T, int max_size>
class Recorder {
 public:
  void Init() {
    for (size_t i = 0; i < max_size; i++) {
      line_[i] = T(0);
    }
    Reset();
  }
  /** clears buffer, sets write ptr to 0, and delay to 1 sample.
   */
  void Reset() {
    write_ptr_ = 0;
    read_ptr_ = 0.0f;
    is_recording_ = false;
  }

  bool HasRecording() const { return write_ptr_ > 0; }

  /** writes the sample of type T to the delay line, and advances the write ptr
   */
  inline void Write(const T sample) {
    if (write_ptr_ == max_size) {
      return;
    }
    is_recording_ = true;
    line_[write_ptr_] = sample;
    ++write_ptr_;
  }

  static constexpr int kFadeWinNumSamples = 1024;

  inline void Finish() {
    const int fade_win_samples = std::min(kFadeWinNumSamples, write_ptr_ / 2);
    if (fade_win_samples == 0) {
      return;
    }
    for (int i = 0; i < fade_win_samples; ++i) {
      const float fade_in =
          static_cast<float>(i) / static_cast<float>(fade_win_samples);
      const float fade_out = 1.0f - fade_in;
      line_[i] = line_[i] * fade_in +
                 line_[write_ptr_ - fade_win_samples + i] * fade_out;
    }
    write_ptr_ -= fade_win_samples;
  }

  /** returns the next sample of type T in the delay line, interpolated if
   * necessary.
   */
  inline const T Read(float speed) {
    if (is_recording_) {
      Finish();
      is_recording_ = false;
    }
    if (write_ptr_ == 0) {
      return T(0);
    }
    const size_t read_idx = read_ptr_;
    const float fraction = fmod(read_ptr_, 1.0f);

    read_ptr_ += speed;
    while (read_ptr_ < 0) {
      read_ptr_ += write_ptr_;
    }
    while (read_ptr_ >= write_ptr_) {
      read_ptr_ -= write_ptr_;
    }

    T a = line_[read_idx % write_ptr_];
    T b = line_[(read_idx + 1) % write_ptr_];
    return a + (b - a) * fraction;
  }

 private:
  int write_ptr_;
  mutable double read_ptr_;
  bool is_recording_;
  T line_[max_size];
};

#endif  // RECORDER_H