#pragma once
#ifndef SAMPLE_MANAGER_H_
#define SAMPLE_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "feature_generator.h"
#include "sample_player.h"
#include "wav_file.h"

constexpr int kMaxSimultaneousSample = 4;

class SampleManager {
 public:
  SampleManager() = default;
  SampleManager(const SampleManager&) = delete;

  int Init(daisy::DaisySeed* seed);

  int GetNumSamples() const { return wav_files_.size(); }

  int TriggerSample(int sample_idx, bool retrigger);

  float GetSample();

  int SdCardLoading();

 private:
  void ComputeCloudCoordinate(std::shared_ptr<SamplePlayer> sample_player,
                              float* x, float* y);

  using SamplePlayerList = std::list<std::shared_ptr<SamplePlayer>>;

  std::vector<std::string> files_;
  std::shared_ptr<SamplePlayerList> sample_player_;
  std::vector<WavFile> wav_files_;

  FeatureGenerator feature_generator_;
};

#endif  // SDCARD_H