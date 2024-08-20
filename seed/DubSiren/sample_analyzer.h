#pragma once
#ifndef SAMPLE_ANALYZER_H_
#define SAMPLE_ANALYZER_H_

#include "constants.h"
#include "mfcc_generator.h"
#include "sample_manager.h"
#include "windower.h"

class SampleAnalyzer {
 public:
  SampleAnalyzer() { mfcc_.Init(); }

  void AnalyzeSamples(SampleManager& sample_manager) {
    Windower<kAnalysisSize, kOverlap> windower;
    std::array<float, kAnalysisSize> buffer;

    std::list<std::array<float, kNumMelBands>> mfccs;

    const int num_samples = sample_manager.GetNumSamples();
    for (int sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
      if (sample_manager.TriggerSample(num_samples, /*retrigger=*/true) != 0) {
        float sample = 0.0f;
        while (sample_manager.GetSample(&sample)) {
          if (windower.AddSample(sample)) {
            windower.GetWindowedBuffer(&buffer);
            mfccs.push_back(std::move(mfcc_.ComputeMfccs(buffer)));
          }
        }
        windower.Flush();
        mfccs.push_back(std::move(mfcc_.ComputeMfccs(buffer)));
      }
    }
  }

  MfccGenerator mfcc_;
};

#endif  // SAMPLE_ANALYZER_H_