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

std::array<float, kNumMelBands>* GetMfccBuffer(int i);

class FeatureGenerator {
 public:
  void Init() {
    mfcc_.Init();
    num_mfccs_ = 0;
  }

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
    num_mfccs_ = 0;
  }

 private:
  void ComputeMfccs() {
    std::cout << "ComputeMfccs\n";
    windower_.GetBuffer(&analysis_buffer_);
    mfcc_.ComputeMfccs(analysis_buffer_, GetMfccBuffer(num_mfccs_));
    ++num_mfccs_;
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

    if (num_mfccs_ == 0) {
      std::cout << "mfcc_vec_ is empty";
      return;
    }
    std::array<float, kNumMelBands> mel_mean_features;
    std::array<float, kNumMelBands> mel_std_features;
    std::array<float, kNumMelBands> mel_avg_mean_features;

    std::vector<float> mel_over_time;
    mel_over_time.resize(num_mfccs_);
    for (int m = 0; m < kNumMelBands; ++m) {
      for (int t = 0; t < num_mfccs_; ++t) {
        const auto* mfcc = GetMfccBuffer(t);
        mel_over_time[t] = (*mfcc)[m];
      }
      std::pair<float, float> result = ComputeMeanAndStd(mel_over_time);
      mel_mean_features[m] = result.first;
      mel_std_features[m] = result.second;

      double avg_sum = 0.0f;
      if (num_mfccs_ > 2) {
        for (int t = 0; t < num_mfccs_ - 2; t += 2) {
          const auto& mfcc_t = *GetMfccBuffer(t);
          const auto& mfcc_t_plus_one = *GetMfccBuffer(t + 1);
          avg_sum += mfcc_t[m] - mfcc_t_plus_one[m];
        }
        avg_sum /= num_mfccs_ - 2;
      }
      mel_avg_mean_features[m] = avg_sum;
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
  int num_mfccs_;
  std::array<float, kAnalysisSize> analysis_buffer_;
  Windower<kAnalysisSize, kOverlap> windower_;
  // std::vector<std::array<float, kNumMelBands>> mfcc_vec_;
  std::array<float, kFeatureVecLen> feature_vec_;
};

#endif  // FEATURE_GENERATOR_H_