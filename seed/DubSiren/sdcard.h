#pragma once
#ifndef SDCARD_H
#define SDCARD_H

#include <functional>
#include <string>
#include <vector>

#include "common.h"
#include "fatfs.h"

#define DSY_TEXT __attribute__((section(".text")))

constexpr size_t kFileNameSize = 16;

void InitSdCard();

using FileCallback = std::function<bool(const std::string& filename)>;

bool ReadDir(FileCallback file_callback,
             const std::vector<std::string>& filetypes, bool recursively,
             std::string path = "");

void ReadDir(std::vector<std::string>& filenames,
             const std::vector<std::string>& filetypes, bool recursively,
             std::string path = "");

int WriteMapFile(const char* filename, const CoordinateToHash* coordinate_map,
                 int size);

int ReadMapFile(const char* filename, CoordinateToHash* coordinate_map,
                int* size, int max_size);

class SdFile {
 public:
  SdFile(const SdFile&) = delete;
  SdFile(SdFile&&) = default;
  SdFile& operator=(SdFile&&) = default;

  SdFile() = default;
  ~SdFile();

  int Open(const char* filename);
  int ReOpen();
  int Seek(size_t pos_bytes);
  int GetReadPos() const;
  int Read(void* buffer, size_t num_bytes, size_t* bytes_read);
  bool IsEof() const;
  void Close();
  // const char* GetFilename() const { return filename_; }

 private:
  int ReadFileSize(const char* filename, int* file_size);

  int fil_idx_;
  FIL fil_copy_;

  // char filename_[kFileNameSize];
};

#endif  // SDCARD_H