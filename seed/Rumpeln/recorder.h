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
    start_ptr_ = 0;
    len_samples_ = 0;
    write_ptr_ = 0;
    read_ptr_ = 0.0f;
  }

  bool HasRecording() const { return len_samples_ > 0; }

  /** writes the sample of type T to the delay line, and advances the write ptr
   */
  inline void Write(const T sample) {
    line_[write_ptr_] = sample;
    ++write_ptr_;
    write_ptr_ = write_ptr_ % max_size;

    if (len_samples_ < max_size) {
      ++len_samples_;
    } else {
      start_ptr_ = write_ptr_;
    }
  }

  /** returns the next sample of type T in the delay line, interpolated if
   * necessary.
   */
  inline const T Read(float speed) const {
    if (len_samples_ == 0) {
      return T(0);
    }
    const size_t read_idx = read_ptr_;
    const float fraction = read_ptr_ - read_idx;
    const size_t read_pos = start_ptr_ + read_idx;

    read_ptr_ += speed;
    if (read_ptr_ < 0) {
      read_ptr_ += len_samples_;
    }

    T a = line_[read_pos % len_samples_];
    T b = line_[(read_pos + 1) % len_samples_];
    return a + (b - a) * fraction;
  }

 private:
  int start_ptr_;
  int len_samples_;
  int write_ptr_;
  mutable float read_ptr_;
  T line_[max_size];
};

#endif  // RECORDER_H