#pragma once
#ifndef SAMPLE_SCANNER_H_
#define SAMPLE_SCANNER_H_

#include <list>
#include <memory>
#include <string>

#include "common.h"
#include "feature_generator.h"
#include "sample_player.h"
#include "sample_search.h"
#include "wav_file.h"

struct SdFileWithHash;

class SampleScanner {
 public:
  SampleScanner() = default;
  SampleScanner(const SampleScanner&) = delete;

  int Init(daisy::DaisySeed* seed);

 private:
  int AnalyzeFile(SdFileWithHash* sdfile_with_hash, SampleInfo* sample_info,
                  daisy::DaisySeed* seed);

  void NormalizeCordMap(int num_sample_infos);

  FeatureGenerator feature_generator_;
};

#endif  // SAMPLE_SCANNER_H_