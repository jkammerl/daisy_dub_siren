#pragma once
#ifndef PLAY_HEAD_H_
#define PLAY_HEAD_H_

#include <memory>
#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "sample_player_common.h"
#include "sdcard.h"

class SamplePlayer {
 public:
  SamplePlayer(const SampleInfo& wav_file_info, SdFile* sdfile,
               Buffer& init_buffer)
      : sample_info_(wav_file_info),
        sdfile_(sdfile),
        current_buffer_(&init_buffer),
        next_buffer_(&buffer_[1]),
        is_playing_(true) {
    read_pos_samples_ = init_buffer.size;
  }

  SamplePlayer(const SamplePlayer&) = delete;

  void Play() { is_playing_ = true; }

  bool IsPlaying() const { return is_playing_; }

  int GetNumUnderuns() const { return buffer_underruns_; }

  SdFile* GetSdFile() const { return sdfile_; }

  int16_t GetSample() {
    if (!is_playing_) {
      return 0;
    }
    if (current_buffer_->size == 0) {
      is_playing_ = false;
      return 0;
    }

    // When starting playback of a new buffer, signal the filling of a new one.
    bool should_trigger_fill_next_buffer = play_pos_idx_ == 0;
    const int16_t samp = current_buffer_->data[play_pos_idx_];

    ++play_pos_idx_;
    if ((play_pos_idx_ >= current_buffer_->size)) {
      // Has the last buffer been reached?
      is_playing_ = !current_buffer_->is_final_buffer_reached;
      if (!is_playing_) {
        // In case final buffer has been reached, fill beginning.
        should_trigger_fill_next_buffer = true;
      }
      current_buffer_ = next_buffer_;
      next_buffer_ = (next_buffer_ == &buffer_[0]) ? &buffer_[1] : &buffer_[0];
      play_pos_idx_ = 0;
      if (fill_next_buffer_) {
        ++buffer_underruns_;
      }
    }
    if (should_trigger_fill_next_buffer) {
      fill_next_buffer_ = true;
    }
    return samp;
  }

  int Prepare();

 private:
  const SampleInfo& sample_info_;
  SdFile* const sdfile_;

  Buffer* current_buffer_;
  Buffer* next_buffer_;

  // Double buffering scheme.
  Buffer buffer_[2];

  bool is_playing_ = false;
  int play_pos_idx_ = 0;
  bool fill_next_buffer_ = false;

  bool init_ = false;
  int read_pos_samples_ = 0;

  int buffer_underruns_ = 0;
};

#endif  // PLAY_HEAD_H_