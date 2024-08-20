#pragma once
#ifndef CONSTANTS_H_
#define CONSTANTS_H_

static constexpr int kSampleRate = 48000;
static constexpr float kNyquist = kSampleRate / 2.0;

static constexpr int kAnalysisSize = 1024;
static constexpr int kPowerSpecSize = kAnalysisSize / 2;
static constexpr int kOverlap = 4;

// Must be power of 2
static constexpr int kNumMelBands = 16;

static constexpr float kMelLowHz = 20.0;
static constexpr float kMelHighHz = 12000.0;

#endif  // CONSTANTS_H_