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

class WavFile {
 public:
  WavFile() : file_(std::make_unique<SdFile>()) {}
  WavFile(WavFile&& other) {
    file_ = std::move(other.file_);
    sample_info_ = std::move(other.sample_info_);
    init_buffer_ = std::move(other.init_buffer_);
    init_ = other.init_;
  };
  WavFile(const WavFile&) = delete;

  int Init(const std::string& filename);

  std::shared_ptr<SamplePlayer> GetSamplePlayer(int sample_idx) {
    if (!init_) {
      return nullptr;
    }
    return std::shared_ptr<SamplePlayer>(
        new SamplePlayer(sample_info_, sample_idx, file_.get(), init_buffer_));
  }

  bool IsInitialized() const { return init_; }

 private:
  int ParseWavHeader(const char* filename);
  int SkipToChunk(size_t* bytepos, const char* chunk_name, int32_t* chunk_size);
  int LoadInitBuffer();
  void Close();

  std::unique_ptr<SdFile> file_;
  SampleInfo sample_info_;
  Buffer init_buffer_;
  bool init_ = false;
};

#endif  // SAMPLE_STREAMER_H_