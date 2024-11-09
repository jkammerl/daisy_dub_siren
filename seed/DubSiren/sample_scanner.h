#pragma once
#ifndef SAMPLE_SCANNER_H_
#define SAMPLE_SCANNER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "feature_generator.h"
#include "sample_player.h"
#include "sample_search.h"
#include "wav_file.h"

class SampleScanner {
 public:
  SampleScanner() = default;
  SampleScanner(const SampleScanner&) = delete;

  int Init(daisy::DaisySeed* seed);

  void PopulateSamples(SampleSearch* sample_search, daisy::DaisySeed* seed);

 private:
  int AnalyzeFile(SdFile* sdfile, float* x, float* y, daisy::DaisySeed* seed);

  void NormalizeCordMap();

  FeatureGenerator feature_generator_;
  std::unordered_map<long, SdFile*> file_map_;
};

#endif  // SAMPLE_SCANNER_H_