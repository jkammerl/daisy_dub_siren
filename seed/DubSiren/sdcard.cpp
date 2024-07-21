#include "sdcard.h"

#include "daisy_patch.h"
#include "daisysp.h"
#include "fatfs.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t kMaxReadBytes = 256;

static SdmmcHandler sdcard;
FatFSInterface fsi;

// Init the hardware
SdmmcHandler::Config sd_cfg;

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

SdFile::SdFile() { memset(&fil_, 0, sizeof(FIL)); }

int SdFile::GetFileSize(const char* filename, int* file_size) {
  FILINFO file_info;
  int result = f_stat(filename, &file_info);
  *file_size = file_info.fsize;
  return result;
}

int SdFile::Open(const char* filename) {
  return f_open(&fil_, filename, (FA_OPEN_EXISTING | FA_READ));
}

int SdFile::Seek(size_t pos_bytes) { return f_lseek(&fil_, pos_bytes); }

int SdFile::Read(void* buffer, size_t num_bytes, size_t* bytes_read) {
  // return f_read(&fil_, buffer, num_bytes, bytes_read);
  *bytes_read = 0;
  int result = FR_OK;
  char* buffer_ptr = reinterpret_cast<char*>(buffer);
  UINT chunk_read_bytes = -1;
  while ((*bytes_read) < num_bytes && result == FR_OK &&
         chunk_read_bytes != 0) {
    const size_t bytes_left = num_bytes - *bytes_read;
    const size_t bytes_to_read = std::min(bytes_left, kMaxReadBytes);
    result = f_read(&fil_, reinterpret_cast<void*>(buffer_ptr), bytes_to_read,
                    &chunk_read_bytes);
    *bytes_read += chunk_read_bytes;
    buffer_ptr += chunk_read_bytes;
  }
  return result;
}

bool SdFile::IsEof() const { return f_eof(&fil_); }

void SdFile::Close() { f_close(&fil_); }
