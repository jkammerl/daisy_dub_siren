#include "wav_file.h"

int WavFile::SkipToChunk(size_t* file_pos_bytes, const char* chunk_name,
                         int32_t* chunk_size) {
  char id[4];
  size_t bytesread = 0;
  for (int i = 0; i < 32; ++i) {
    // Read 4-byte chunk ID.
    if (file_->Read(id, sizeof(id), &bytesread) != FR_OK ||
        bytesread != sizeof(id)) {
      return -1;
    }
    *file_pos_bytes += sizeof(id);

    // Read 4-byte chunk size.
    int32_t size = 0;
    if (file_->Read(&size, sizeof(size), &bytesread) != FR_OK ||
        bytesread != sizeof(size)) {
      return -1;
    }
    *file_pos_bytes += sizeof(size);

    if (memcmp(id, chunk_name, 4) == 0) {
      *chunk_size = size;
      return 0;
    }

    // Skip past this chunk's data. RIFF requires even-byte alignment, so
    // odd-sized chunks are followed by a 0x00 pad byte.
    const int32_t skip = size + (size & 1);
    *file_pos_bytes += skip;
    if (file_->Seek(*file_pos_bytes) != FR_OK) {
      return -1;
    }
  }
  return -1;
}

int WavFile::ParseWavHeader(const char* filename) {
  size_t bytesread = 0;
  size_t file_pos_bytes = 0;
  char buffer[4];

  // Validate RIFF marker.
  if (file_->Read(buffer, 4, &bytesread) != FR_OK || bytesread != 4 ||
      memcmp(buffer, "RIFF", 4) != 0) {
    return -1;
  }
  file_pos_bytes += 4;

  // Skip RIFF chunk size (unused).
  int32_t riff_size;
  if (file_->Read(&riff_size, sizeof(riff_size), &bytesread) != FR_OK ||
      bytesread != sizeof(riff_size)) {
    return -1;
  }
  file_pos_bytes += sizeof(riff_size);

  // Validate WAVE format marker.
  if (file_->Read(buffer, 4, &bytesread) != FR_OK || bytesread != 4 ||
      memcmp(buffer, "WAVE", 4) != 0) {
    return -1;
  }
  file_pos_bytes += 4;

  // Find "fmt " chunk (may be preceded by JUNK, bext, etc.).
  int32_t fmt_size = 0;
  if (SkipToChunk(&file_pos_bytes, "fmt ", &fmt_size)) {
    return -1;
  }

  // fmt must be at least 16 bytes (standard PCM header size).
  if (fmt_size < static_cast<int32_t>(sizeof(WavHeader))) {
    return -1;
  }

  // Read the first sizeof(WavHeader) bytes of the fmt chunk.
  // Extended formats (18-byte non-PCM, 40-byte extensible) have extra fields
  // after byte 16 that we don't need.
  if (file_->Read(&sample_info_.wav_header, sizeof(WavHeader), &bytesread) !=
          FR_OK ||
      bytesread != sizeof(WavHeader)) {
    return -1;
  }

  // Seek past the complete fmt chunk (accounting for odd-size alignment padding).
  file_pos_bytes += fmt_size + (fmt_size & 1);
  if (file_->Seek(file_pos_bytes) != FR_OK) {
    return -1;
  }

  // Find "data" chunk (may be preceded by fact, LIST, id3, etc.).
  int32_t data_size = 0;
  if (SkipToChunk(&file_pos_bytes, "data", &data_size)) {
    return -1;
  }

  sample_info_.samples_first_byte = file_pos_bytes;
  // Use the data chunk size directly — more reliable than reading it from
  // within the fmt chunk at the wrong offset.
  sample_info_.num_samples =
      data_size / (sample_info_.wav_header.BitPerSample / 8);
  return 0;
}

int WavFile::Init(const std::string& filename) {
  int file_size = 0;
  if (file_->GetFileSize(filename.c_str(), &file_size) != FR_OK) {
    return -1;
  }

  if (file_->Open(filename.c_str()) != FR_OK) {
    return -1;
  }

  if (ParseWavHeader(filename.c_str())) {
    return -1;
  }

  // const int bytes_per_sample = sample_info_.wav_header.BitPerSample / 8;
  // if (sample_info_.samples_first_byte +
  //         sample_info_.num_samples * bytes_per_sample !=
  //     file_size) {
  //   return -1;
  // }
  if (sample_info_.wav_header.AudioFormat != 1 ||
      sample_info_.wav_header.SampleRate != 48000 ||
      sample_info_.wav_header.BitPerSample != 16 ||
      sample_info_.wav_header.NbrChannels != 1) {
    return -1;
  }

  if (file_->Seek(sample_info_.samples_first_byte) != FR_OK) {
    return -1;
  }

  int err = LoadInitBuffer();
  init_ = true;

  return err;
}

int WavFile::LoadInitBuffer() {
  if (file_->Seek(sample_info_.samples_first_byte) != FR_OK) {
    return -1;
  }
  const int num_request_samples =
      std::min<int>(sample_info_.num_samples, kBufferSize);
  const size_t num_request_bytes = num_request_samples * sizeof(int16_t);
  size_t bytesread = 0;
  const int result =
      file_->Read(&init_buffer_.data[0], num_request_bytes, &bytesread);
  if (result != FR_OK) {
    return -1;
  }
  init_buffer_.is_final_buffer_reached = num_request_samples < kBufferSize ||
                                         bytesread < num_request_bytes ||
                                         file_->IsEof();
  init_buffer_.size = bytesread / sizeof(int16_t);
  return 0;
}

void WavFile::Close() {
  if (init_) {
    file_->Close();
    init_ = false;
  }
}