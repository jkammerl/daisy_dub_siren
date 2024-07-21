#include "sample_player.h"

int SamplePlayer::Prepare() {
  if (!fill_next_buffer_) {
    return 0;
  }
  const int read_pos_bytes =
      sample_info_.samples_first_byte +
      read_pos_samples_ * static_cast<int>(sizeof(int16_t));
  if (sdfile_->Seek(read_pos_bytes) != FR_OK) {
    init_ = false;
    return -1;
  }

  const int samples_left = sample_info_.num_samples - read_pos_samples_;
  const int num_read_samples = std::min<int>(samples_left, kBufferSize);
  const int num_read_bytes = num_read_samples * sizeof(int16_t);

  size_t bytes_actually_read = 0;
  const int result = sdfile_->Read(&next_buffer_->data[0], num_read_bytes,
                                   &bytes_actually_read);
  if (result != FR_OK) {
    init_ = false;
    return -1;
  }
  next_buffer_->size = bytes_actually_read / sizeof(int16_t);
  read_pos_samples_ += next_buffer_->size;
  if (num_read_samples < kBufferSize ||
      static_cast<int>(bytes_actually_read) < num_read_bytes ||
      sdfile_->IsEof()) {
    read_pos_samples_ = 0;
    next_buffer_->is_final_buffer_reached = true;
  } else {
    next_buffer_->is_final_buffer_reached = false;
  }
  fill_next_buffer_ = false;
  return 0;
}