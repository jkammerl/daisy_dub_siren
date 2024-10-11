
#pragma once
#ifndef WINDOWER_H_
#define WINDOWER_H_

#include <algorithm>
#include <array>
#include <vector>

class PreemphasisFilter {
 public:
  explicit PreemphasisFilter(float coefficient = 0.97f) : alpha_(coefficient) {}
  // Applies the pre-emphasis filter to the input signal.
  float Apply(float signal) {
    const float filtered_signal = signal - alpha_ * prev_signal_;
    prev_signal_ = signal;
    return filtered_signal;
  }

 private:
  float alpha_;        // Pre-emphasis coefficient
  float prev_signal_;  // Store the previous signal value
};

template <int Size, int Overlap>
class Windower {
 public:
  static constexpr int WindowStep = Size / Overlap;
  Windower() : write_pos_(0) {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
  };
  // returns true when window available.
  bool AddSample(float sample) {
    if (num_samples_to_first_frame < Size) {
      ++num_samples_to_first_frame;
    }
    buffer_[write_pos_] = sample;  // preemphasis_.Apply(sample);
    write_pos_ = (++write_pos_) % Size;
    const bool window_available = (write_pos_ % WindowStep) == 0;
    const bool first_frame_available = num_samples_to_first_frame == Size;
    return window_available && first_frame_available;
  }

  bool Flush() {
    const bool window_available = (write_pos_ % WindowStep) == 0;
    if (window_available) {
      return false;
    }
    while (!AddSample(0.0f)) {
    }
    return true;
  }

  void GetBuffer(std::array<float, Size>* buffer) {
    int read_pos = write_pos_;
    auto it = buffer->begin();
    for (int i = 0; i < Size; ++i, ++it) {
      *it = buffer_[read_pos];
      read_pos = (++read_pos) % Size;
    }
  }

 private:
  // PreemphasisFilter preemphasis_;
  std::array<float, Size> buffer_;
  int write_pos_ = 0;
  int num_samples_to_first_frame = 0;
};

#endif  // WINDOWER_H_
