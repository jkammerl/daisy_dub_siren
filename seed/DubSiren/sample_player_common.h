#pragma once
#ifndef SAMPLE_PLAYER_COMMON_H
#define SAMPLE_PLAYER_COMMON_H

#include <string>

static constexpr int kBufferSize = 4096 / 4;

struct WavHeader {
  uint16_t AudioFormat;   /**< & */
  uint16_t NbrChannels;   /**< & */
  uint32_t SampleRate;    /**< & */
  uint32_t ByteRate;      /**< & */
  uint16_t BlockAlign;    /**< & */
  uint16_t BitPerSample;  /**< & */
  uint32_t SubChunk2ID;   /**< & */
  uint32_t SubCHunk2Size; /**< & */
};

struct SampleInfo {
  WavHeader wav_header;
  int samples_first_byte = 0;
  int num_samples = 0;
};

struct Buffer {
  int16_t data[kBufferSize];
  int size = 0;
  bool is_final_buffer_reached = false;
};

#endif  // SAMPLE_PLAYER_COMMON_H