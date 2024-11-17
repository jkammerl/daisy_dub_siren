#pragma once
#ifndef SAMPLE_STREAMER_H_
#define SAMPLE_STREAMER_H_

#include <memory>
#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "sample_player.h"
#include "sample_player_common.h"
#include "sdcard.h"

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

class WavFile {
 public:
  WavFile() = default;
  WavFile(const WavFile&) = delete;

  ~WavFile() { Close(); }

  int Init(SdFile* sdfile, SampleInfo* sample_info);

  SdFile* GetSdFile() const { return file_; }

  bool IsInitialized() const { return init_; }

 private:
  int ParseWavHeader();
  int SkipToChunk(size_t* bytepos, const char* chunk_name, int32_t* chunk_size);
  void Close();

  SdFile* file_;
  SampleInfo sample_info_;
  WavHeader wav_header_;
  bool init_ = false;
};

#endif  // SAMPLE_STREAMER_H_