#pragma once
#ifndef SAMPLE_ANALYZER_H_
#define SAMPLE_ANALYZER_H_

#include "fft.h"
#include "sample_manager.h"
#include "windower.h"

static constexpr int kAnalysisSize = 1024;
static constexpr int kOverlap = 4;

class SampleAnalyzer {
 public:
  SampleAnalyzer() { fft_.Init(); }

  void ComputeMfccs(std::array<float, kAnalysisSize>* buffer) {
    fft_.ComputeSpectrum(buffer);
  }

  void AnalyzeSamples(SampleManager& sample_manager) {
    Windower<kAnalysisSize, kOverlap> windower;
    std::array<float, kAnalysisSize> buffer;

    const int num_samples = sample_manager.GetNumSamples();
    for (int sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
      if (sample_manager.TriggerSample(num_samples, /*retrigger=*/true) != 0) {
        float sample = 0.0f;
        while (sample_manager.GetSample(&sample)) {
          if (windower.AddSample(sample)) {
            windower.GetWindowedBuffer(&buffer);
            ComputeMfccs(&buffer);
          }
        }
        windower.Flush();
        ComputeMfccs(&buffer);
      }
    }
  }

 private:
  FFT<kAnalysisSize> fft_;
};

#endif  // SAMPLE_ANALYZER_H_