#include "sdcard.h"

#include <list>

#include "common.h"
#include "daisy_patch.h"
#include "daisysp.h"
#include "fatfs.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t kMaxReadBytes = 256;

DSY_TEXT SdmmcHandler sdcard;
DSY_TEXT FatFSInterface fsi;

// Init the hardware
DSY_TEXT SdmmcHandler::Config sd_cfg;

void CopyFil(const FIL& from, FIL* to) { memcpy(to, &from, sizeof(FIL)); }

constexpr int kNumMaxOpenFiles = 128;  // 64;
// FIL can't be on the stack, see
// https://forum.electro-smith.com/t/fatfs-f-read-returns-fr-disk-err/2883
DSY_TEXT FIL g_fils[kNumMaxOpenFiles];

class FilManager {
 public:
  static FilManager* Get() {
    static FilManager* filmanager = []() { return new FilManager(); }();
    return filmanager;
  }

  int RegisterFil() {
    int fil_idx = unused_fils_.back();
    unused_fils_.pop_back();
    return fil_idx;
  }

  void ReturnFil(int fil_idx) { unused_fils_.push_back(fil_idx); }

  FIL* GetFil(int fil_idx) { return &g_fils[fil_idx]; }

 private:
  FilManager() {
    memset(g_fils, 0, sizeof(FIL) * kNumMaxOpenFiles);
    for (int i = 0; i < kNumMaxOpenFiles; ++i) {
      unused_fils_.push_back(i);
    }
  }

  std::list<int> unused_fils_;
};

bool endsWith(const char* str, const char* suffix) {
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

void InitSdCard() {
  sd_cfg.Defaults();
  sd_cfg.width = daisy::SdmmcHandler::BusWidth::BITS_4;
  sd_cfg.speed = daisy::SdmmcHandler::Speed::FAST;
  sdcard.Init(sd_cfg);

  // Link hardware and FatFS
  fsi.Init(FatFSInterface::Config::MEDIA_SD);

  // Mount SD Card
  f_mount(&fsi.GetSDFileSystem(), "/", 1);
}

void ReadDir(std::vector<std::string>& filenames,
             const std::vector<std::string>& filetypes, bool recursively,
             std::string path) {
  DIR dir;
  FILINFO fi;
  f_opendir(&dir, fsi.GetSDPath());

  FRESULT fr = f_findfirst(&dir, &fi, path.c_str(), "*");

  while (fr == FR_OK && fi.fname[0]) {
    if ((fi.fattrib & AM_DIR) && recursively) {
      ReadDir(filenames, filetypes, recursively, path + "/" + fi.fname);
    } else {
      for (const auto& file_type : filetypes) {
        if (endsWith(fi.fname, file_type.c_str())) {
          filenames.push_back(path + "/" + fi.fname);
          break;
        }
      }
    }
    fr = f_findnext(&dir, &fi);
  }
  f_closedir(&dir);
}

bool ReadDir(FileCallback file_callback,
             const std::vector<std::string>& filetypes, bool recursively,
             std::string path) {
  DIR dir;
  FILINFO fi;
  f_opendir(&dir, fsi.GetSDPath());
  bool cont = true;

  FRESULT fr = f_findfirst(&dir, &fi, path.c_str(), "*");

  while (fr == FR_OK && fi.fname[0] && cont) {
    if ((fi.fattrib & AM_DIR) && recursively) {
      cont &=
          ReadDir(file_callback, filetypes, recursively, path + "/" + fi.fname);
    } else {
      for (const auto& file_type : filetypes) {
        if (endsWith(fi.fname, file_type.c_str())) {
          cont &= file_callback(path + "/" + fi.fname);
          break;
        }
      }
    }
    fr = f_findnext(&dir, &fi);
  }
  f_closedir(&dir);
  return cont;
}

int WriteMapFile(const char* filename, const CoordinateToHash* coordinate_map,
                 int size) {
  int fil_idx = FilManager::Get()->RegisterFil();
  FIL* fil = FilManager::Get()->GetFil(fil_idx);
  FRESULT res = f_open(fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK) {
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  unsigned int bytes_written;

  res = f_write(fil, reinterpret_cast<const char*>(&size), sizeof(size),
                &bytes_written);
  if (res != FR_OK || bytes_written != sizeof(size)) {
    f_close(fil);
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  res = f_write(fil, reinterpret_cast<const char*>(coordinate_map),
                sizeof(CoordinateToHash) * size, &bytes_written);
  if (res != FR_OK || bytes_written != sizeof(CoordinateToHash) * size) {
    f_close(fil);
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  res = f_close(fil);
  if (res != FR_OK) {
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  FilManager::Get()->ReturnFil(fil_idx);
  return FR_OK;
}

int ReadMapFile(const char* filename, CoordinateToHash* coordinate_map,
                int* size, int max_size) {
  int fil_idx = FilManager::Get()->RegisterFil();
  FIL* fil = FilManager::Get()->GetFil(fil_idx);
  FRESULT res = f_open(fil, filename, FA_READ);
  if (res != FR_OK) {
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  unsigned int bytes_read;

  res = f_read(fil, reinterpret_cast<char*>(size), sizeof(size), &bytes_read);
  if (res != FR_OK || bytes_read != sizeof(size)) {
    f_close(fil);
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  if (*size > max_size) {
    return -1;
  }
  res = f_read(fil, reinterpret_cast<char*>(coordinate_map),
               sizeof(CoordinateToHash) * (*size), &bytes_read);
  if (res != FR_OK || bytes_read != sizeof(CoordinateToHash) * (*size)) {
    f_close(fil);
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  res = f_close(fil);
  if (res != FR_OK) {
    FilManager::Get()->ReturnFil(fil_idx);
    return res;
  }
  FilManager::Get()->ReturnFil(fil_idx);
  return FR_OK;
}

SdFile::~SdFile() { Close(); }

int SdFile::ReadFileSize(const char* filename, int* file_size) {
  FILINFO file_info;
  int result = f_stat(filename, &file_info);
  *file_size = file_info.fsize;
  return result;
}

int SdFile::Open(const char* filename) {
  fil_idx_ = FilManager::Get()->RegisterFil();
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  if (f_open(fil, filename, (FA_OPEN_EXISTING | FA_READ)) != FR_OK) {
    return -1;
  }
  memcpy(&fil_copy_, fil, sizeof(FIL));

  // const int full_filename_len = strlen(filename);
  // if (full_filename_len > 4) {
  //   const int kFileExtensionSize = 4;
  //   int chars_to_copy = std::min<int>(full_filename_len - kFileExtensionSize,
  //                                     kFileNameSize - 1);
  //   strncpy(filename_,
  //           filename + full_filename_len - kFileExtensionSize -
  //           chars_to_copy, chars_to_copy);
  //   filename_[chars_to_copy] = 0;
  // } else {
  //   strcpy(filename_, "weird");
  // }
  return 0;
}

int SdFile::ReOpen() {
  fil_idx_ = FilManager::Get()->RegisterFil();
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  memcpy(fil, &fil_copy_, sizeof(FIL));
  return 0;
}

int SdFile::Seek(size_t pos_bytes) {
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  if (f_lseek(fil, pos_bytes) != FR_OK) {
    return -1;
  }
  return 0;
}

int SdFile::GetReadPos() const {
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  return fil->fptr;
}

int SdFile::Read(void* buffer, size_t num_bytes, size_t* bytes_read) {
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  // return f_read(&fil_, buffer, num_bytes, bytes_read);
  *bytes_read = 0;
  int result = FR_OK;
  char* buffer_ptr = reinterpret_cast<char*>(buffer);
  UINT chunk_read_bytes = -1;
  while ((*bytes_read) < num_bytes && result == FR_OK &&
         chunk_read_bytes != 0) {
    const size_t bytes_left = num_bytes - *bytes_read;
    const size_t bytes_to_read = std::min(bytes_left, kMaxReadBytes);
    result = f_read(fil, reinterpret_cast<void*>(buffer_ptr), bytes_to_read,
                    &chunk_read_bytes);
    *bytes_read += chunk_read_bytes;
    buffer_ptr += chunk_read_bytes;
  }
  return result;
}

bool SdFile::IsEof() const {
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  return f_eof(fil);
}

void SdFile::Close() {
  FIL* fil = FilManager::Get()->GetFil(fil_idx_);
  f_close(fil);
  FilManager::Get()->ReturnFil(fil_idx_);
}
