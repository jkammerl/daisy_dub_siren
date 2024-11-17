#pragma once
#ifndef COMMON_H_
#define COMMON_H_

#include <array>

#include "constants.h"
#include "daisy_seed.h"

using namespace daisy;

class DaisyHw {
 public:
  static DaisySeed& Get();
};

class SdFile;

struct AudioBuffer {
  std::array<int16_t, kBufferSize> samples;
  int num_samples;
  bool eob_reached;
};

struct SampleInfo {
  unsigned long hash;
  mutable SdFile* sdfile;
  AudioBuffer first_buffer;
  int samples_first_byte;
  int num_samples;
  float x;
  float y;

  SampleInfo() = default;
  SampleInfo(const SampleInfo&) = delete;
  SampleInfo& operator=(const SampleInfo&) = delete;
};

#endif  // COMMON_H_