
#pragma once
#ifndef WINDOWER_H_
#define WINDOWER_H_

#include <array>

template <int Size, int Overlap>
class Windower {
 public:
  static constexpr int WindowStep = Size / Overlap;
  Windower() : window_(GenerateHannWindow()), write_pos_(0) {};
  // returns true when window available.
  bool AddSample(float sample) {
    buffer_[write_pos_] = sample;
    write_pos_ = (++write_pos_) % Size;
    const bool window_available = (write_pos_ % WindowStep) == 0;
    return window_available;
  }

  void Flush() {
    while (!AddSample(0.0f)) {
    }
  }

  void GetWindowedBuffer(std::array<float, Size>* buffer) {
    int read_pos = write_pos_;
    auto it = buffer->begin();
    for (int i = 0; i < Size; ++i, ++it) {
      *it = buffer_[read_pos] * window_[i];
      read_pos = (++read_pos) % Size;
    }
  }

 private:
  static std::array<float, Size> GenerateHannWindow() {
    std::array<float, Size> window;
    for (int i = 0; i < Size; i++) {
      window[i] = 0.5 * (1 - cos(2.0 * M_PI * i / (Size - 1)));
    }
    return window;
  }

  std::array<float, Size> buffer_;
  std::array<float, Size> window_;
  int write_pos_ = 0;
};

#endif  // WINDOWER_H_
