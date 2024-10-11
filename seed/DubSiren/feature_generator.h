#pragma once
#ifndef FEATURE_GENERATOR_H_
#define FEATURE_GENERATOR_H_

#include <array>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "mfcc_generator.h"
#include "windower.h"

class FeatureGenerator {
 public:
  void Init() { mfcc_.Init(); }

  void AddSample(float input) {
    if (windower_.AddSample(input)) {
      ComputeMfccs();
    }
  }

  void AddSamples(const std::vector<float>& input) {
    for (float sample : input) {
      AddSample(sample);
    }
  }

  void GetCloudCoordinates(float* x, float* y) {
    GetFeatures(&feature_vec_);
    ComputeCoordinates(feature_vec_, x, y);
  }

 private:
  void ComputeMfccs() {
    std::cout << "ComputeMfccs\n";
    windower_.GetBuffer(&analysis_buffer_);
    auto mfccs = mfcc_.ComputeMfccs(analysis_buffer_);
    mfcc_vec_.push_back(std::move(mfccs));
  }

  std::pair<float, float> ComputeMeanAndStd(const std::vector<float>& data) {
    if (data.empty()) {
      return {
          0.0f,
          0.0f,
      };  // Handle empty vector case
    }

    // Calculate mean
    double mean = 0.0f;
    for (float value : data) {
      mean += value;
    }
    mean /= data.size();

    // Calculate variance
    double variance = 0.0f;
    for (float value : data) {
      variance += (value - mean) * (value - mean);
    }
    variance /= data.size();

    // Calculate standard deviation
    return {mean, std::sqrt(variance)};
  }

  void GetFeatures(std::array<float, kFeatureVecLen>* feature_vec) {
    if (windower_.Flush()) {
      ComputeMfccs();
    }

    if (mfcc_vec_.empty()) {
      std::cout << "mfcc_vec_ is empty";
      return;
    }
    std::array<float, kNumMelBands> mel_mean_features;
    std::array<float, kNumMelBands> mel_std_features;
    std::array<float, kNumMelBands> mel_avg_mean_features;

    std::vector<float> mel_over_time;
    mel_over_time.resize(mfcc_vec_.size());
    for (int m = 0; m < kNumMelBands; ++m) {
      for (int t = 0; t < static_cast<int>(mfcc_vec_.size()); ++t) {
        mel_over_time[t] = mfcc_vec_[t][m];
      }
      std::pair<float, float> result = ComputeMeanAndStd(mel_over_time);
      mel_mean_features[m] = result.first;
      mel_std_features[m] = result.second;

      double avg_sum = 0.0f;
      for (int t = 0; t < static_cast<int>(mfcc_vec_.size()) - 2; t += 2) {
        avg_sum += mfcc_vec_[t][m] - mfcc_vec_[t + 1][m];
      }

      mel_avg_mean_features[m] = avg_sum / (mfcc_vec_.size() - 2);
    }
    std::copy(mel_mean_features.begin(), mel_mean_features.end(),
              feature_vec->begin() + kNumMelBands * 0);
    std::copy(mel_std_features.begin(), mel_std_features.end(),
              feature_vec->begin() + kNumMelBands * 1);
    std::copy(mel_avg_mean_features.begin(), mel_avg_mean_features.end(),
              feature_vec->begin() + kNumMelBands * 2);
  }

  void ComputeCoordinates(const std::array<float, kFeatureVecLen>& features,
                          float* x, float* y) {
    *x = 0.0f;
    *y = 0.0f;
    for (int i = 0; i < static_cast<int>(features.size()); ++i) {
      const float feature = features[i];
      *x += kPcaCoeffs[0][i] * feature;
      *y += kPcaCoeffs[1][i] * feature;
    }
  }

  MfccGenerator mfcc_;
  std::array<float, kAnalysisSize> analysis_buffer_;
  Windower<kAnalysisSize, kOverlap> windower_;
  std::vector<std::array<float, kNumMelBands>> mfcc_vec_;
  std::array<float, kFeatureVecLen> feature_vec_;
};

#endif  // FEATURE_GENERATOR_H_