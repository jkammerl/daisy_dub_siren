#include "wav_file.h"

int WavFile::SkipToChunk(size_t* file_pos_bytes, const char* chunk_name,
                         int32_t* chunk_size) {
  char buffer[4];
  int chunk_count = 0;
  size_t bytesread = 0;
  while (true) {
    if (file_->Read((void*)&buffer, sizeof(buffer), &bytesread) != FR_OK ||
        bytesread != sizeof(buffer)) {
      return -1;
    }
    *file_pos_bytes += sizeof(buffer);
    if (file_->Read((void*)chunk_size, sizeof(int32_t), &bytesread) == FR_OK &&
        bytesread == sizeof(int32_t) && (memcmp(buffer, chunk_name, 4) == 0)) {
      *file_pos_bytes += sizeof(int32_t);
      return 0;
    }
    *file_pos_bytes += *chunk_size + sizeof(int32_t);
    if (++chunk_count == 10) {
      return -1;
    };
    // Skip chunk.
    if (file_->Seek(*file_pos_bytes) != FR_OK) {
      return -1;
    }
  }
  return 0;
}

int WavFile::ParseWavHeader() {
  size_t bytesread = 0;
  size_t file_pos_bytes = 0;
  char buffer[4];

  // Look for RIFF chunk.
  if (file_->Read((void*)&buffer, sizeof(buffer), &bytesread) != FR_OK ||
      bytesread != sizeof(buffer) || memcmp(buffer, "RIFF", 4) != 0) {
    return -1;
  }
  file_pos_bytes += 4;

  // Read RIFF chunk size.
  int32_t riff_size;
  if (file_->Read((void*)&riff_size, sizeof(riff_size), &bytesread) != FR_OK ||
      bytesread != sizeof(riff_size)) {
    return -1;
  }
  file_pos_bytes += sizeof(riff_size);

  // Read wav format.
  int32_t wave_format;
  if (file_->Read((void*)&wave_format, sizeof(wave_format), &bytesread) !=
          FR_OK ||
      bytesread != sizeof(wave_format)) {
    return -1;
  }
  file_pos_bytes += sizeof(wave_format);

  // Look for fmt chunk.
  int32_t chunk_size = 0;
  if (SkipToChunk(&file_pos_bytes, "fmt ", &chunk_size)) {
    return -1;
  }

  // Read Wav header.
  if (file_->Read((void*)&wav_header_, sizeof(WavHeader), &bytesread) !=
      FR_OK) {
    return -1;
  }

  // Seek to next chunk.
  file_pos_bytes += chunk_size;
  if (file_->Seek(file_pos_bytes) != FR_OK) {
    return -1;
  }

  // Look for data chunk.
  if (SkipToChunk(&file_pos_bytes, "data", &chunk_size)) {
    return -1;
  }

  // Obtain number of samples.
  sample_info_.num_samples =
      wav_header_.SubCHunk2Size / (wav_header_.BitPerSample / 8);
  if (sample_info_.num_samples == 0 && file_pos_bytes <= chunk_size + 8) {
    // Compute num samples from chunk size
    sample_info_.num_samples = chunk_size / (wav_header_.BitPerSample / 8);
  }
  sample_info_.samples_first_byte = file_pos_bytes;
  return 0;
}

int WavFile::Init(SdFile* sdfile, SampleInfo* sample_info) {
  file_ = sdfile;
  file_->ReOpen();

  if (ParseWavHeader()) {
    return -1;
  }

  if (wav_header_.SampleRate != 48000 || wav_header_.BitPerSample != 16 ||
      wav_header_.NbrChannels != 1) {
    return -1;
  }

  if (file_->Seek(sample_info_.samples_first_byte) != FR_OK) {
    return -1;
  }

  sample_info->num_samples = sample_info_.num_samples;
  sample_info->samples_first_byte = sample_info_.samples_first_byte;
  sample_info->sdfile = file_;

  AudioBuffer* buffer = &sample_info->first_buffer;

  const int num_request_samples =
      std::min<int>(sample_info_.num_samples, buffer->samples.size());
  const size_t num_request_bytes = num_request_samples * sizeof(int16_t);
  size_t bytesread = 0;
  const int result =
      file_->Read(buffer->samples.data(), num_request_bytes, &bytesread);
  if (result != FR_OK) {
    return -1;
  }
  buffer->eob_reached = num_request_bytes < kBufferSize ||
                        bytesread < num_request_bytes || file_->IsEof();
  buffer->num_samples = bytesread / sizeof(int16_t);

  init_ = true;

  return 0;
}

void WavFile::Close() {
  if (init_) {
    file_->Close();
    init_ = false;
  }
}