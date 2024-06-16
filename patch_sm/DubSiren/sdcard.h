#pragma once
#ifndef SDCARD_H
#define SDCARD_H

#include <string>
#include <vector>

#include "fatfs.h"

void InitSdCard();

void ReadDir(std::vector<std::string>& filenames,
             const std::vector<std::string>& filetypes, bool recursively,
             std::string path = "");

class SdFile {
 public:
  SdFile(const SdFile&) = delete;
  SdFile(SdFile&&) = default;

  SdFile();
  int GetFileSize(const char* filename, int* file_size);
  int Open(const char* filename);
  int Seek(size_t pos_bytes);
  int GetReadPos() const { return fil_.fptr; }
  int Read(void* buffer, size_t num_bytes, size_t* bytes_read);
  bool IsEof() const;
  void Close();

 private:
  // FIL can't be on the stack, see
  // https://forum.electro-smith.com/t/fatfs-f-read-returns-fr-disk-err/2883
  FIL fil_;
};

#endif  // SDCARD_H