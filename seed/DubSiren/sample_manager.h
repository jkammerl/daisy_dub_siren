#pragma once
#ifndef SAMPLE_MANAGER_H_
#define SAMPLE_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "sample_player.h"
#include "wav_file.h"

constexpr int kMaxSimultaneousSample = 2;

class SampleManager {
 public:
  SampleManager() = default;
  SampleManager(const SampleManager&) = delete;

  int Init(daisy::DaisySeed* seed);

  int TriggerSample(const SampleInfo& sample_info, bool retrigger);

  float GetSample();

  int SdCardLoading();

 private:
  void ComputeCloudCoordinate(std::shared_ptr<SamplePlayer> sample_player,
                              float* x, float* y);

  using SamplePlayerList = std::list<std::shared_ptr<SamplePlayer>>;

  std::vector<std::string> files_;
  std::shared_ptr<SamplePlayerList> sample_player_;
};

#endif  // SDCARD_H