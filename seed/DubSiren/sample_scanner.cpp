#include "sample_scanner.h"

#include "common.h"
#include "daisy_seed.h"
#include "fatfs.h"
#include "feature_generator.h"
#include "sample_info_storage.h"
#include "sdcard.h"
#include "wav_file.h"

using namespace daisy;

struct SdFileWithHash {
  SdFile sdfile;
  unsigned long hash;
};

DSY_SDRAM_BSS SdFileWithHash sdfiles_with_hash[kMaxWavFiles];
int num_sdfiles_with_hash = 0;

unsigned long SimpleHash(const char* str) {
  const unsigned char* ustr = reinterpret_cast<const unsigned char*>(str);
  unsigned long hash = 5381;
  int c;
  while (c = *ustr++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

int SampleScanner::Init(daisy::DaisySeed* seed) {
  InitSdCard();
  feature_generator_.Init();

  auto callback = [&](const std::string& filename) -> bool {
    if (num_sdfiles_with_hash >= kMaxWavFiles) {
      return false;
    }
    auto& sdfile_with_hash = sdfiles_with_hash[num_sdfiles_with_hash];
    sdfile_with_hash.sdfile.Open(filename.c_str());

    sdfile_with_hash.sdfile.Close();
    sdfile_with_hash.hash = SimpleHash(filename.c_str());
    ++num_sdfiles_with_hash;
    return true;
  };
  seed->PrintLine("Scanning SD card for wav files");
  // TODO exclude .trash
  ReadDir(callback, {".wav", ".WAV", ".Wav"}, true);
  seed->PrintLine("Found %d wav files", num_sdfiles_with_hash);

  std::sort(std::begin(sdfiles_with_hash),
            std::begin(sdfiles_with_hash) + num_sdfiles_with_hash,
            [](const SdFileWithHash& lhs, const SdFileWithHash& rhs) {
              return lhs.hash < rhs.hash;
            });

  int g_num_sample_infos = 0;
  if (ReadMapFile(kCoordinatesMapFilename, GetMutableSampleInfo(0),
                  &g_num_sample_infos, kMaxWavFiles) != 0) {
    seed->PrintLine("Failed to read coordinate hash list from %s",
                    kCoordinatesMapFilename);
    g_num_sample_infos = 0;
  } else {
    seed->PrintLine("Successfully read %d coordinate hash list from %s",
                    g_num_sample_infos, kCoordinatesMapFilename);
  }

  bool rescan_needed = true;  // g_num_sample_infos == 0;

  if (rescan_needed) {
    g_num_sample_infos = 0;
    for (int i = 0; i < num_sdfiles_with_hash; ++i) {
      auto& sdfile_with_hash = sdfiles_with_hash[g_num_sample_infos];
      seed->PrintLine("Processing file %d", i);
      SampleInfo* sample_info = GetMutableSampleInfo(g_num_sample_infos);
      if (AnalyzeFile(&sdfile_with_hash, sample_info, seed) != 0) {
        seed->PrintLine("Can't read/analyze file");
        continue;
      }
      seed->PrintLine("Coordinate x: %d, y: %d", (int)(sample_info->x * 100),
                      (int)(sample_info->y * 100));
      ++g_num_sample_infos;
    }
    NormalizeCordMap(g_num_sample_infos);

    seed->PrintLine("Analysis done");
    if (WriteMapFile(kCoordinatesMapFilename, GetSampleInfo(0),
                     g_num_sample_infos) != 0) {
      seed->PrintLine("Error writing: %s", kCoordinatesMapFilename);
    }
    seed->PrintLine("Successfully wrote coordinate hash list to %s",
                    kCoordinatesMapFilename);
  }
  SetNumSampleInfos(g_num_sample_infos);

  seed->PrintLine("Populating samples");
  for (int i = 0; i < g_num_sample_infos; ++i) {
    SampleInfo* sample_info = GetMutableSampleInfo(i);
    auto itr = std::lower_bound(
        std::begin(sdfiles_with_hash),
        std::begin(sdfiles_with_hash) + num_sdfiles_with_hash,
        sample_info->hash,
        [](const SdFileWithHash& sdfile_with_hash, unsigned long hash) {
          return sdfile_with_hash.hash < hash;
        });

    if (itr == std::begin(sdfiles_with_hash) + num_sdfiles_with_hash ||
        itr->hash != sample_info->hash) {
      seed->PrintLine("Could not find file matching hash");
      sample_info->sdfile = nullptr;
      continue;
    }
    sample_info->sdfile = &itr->sdfile;
  }
  seed->PrintLine("Populating samples done");

  return 0;
}

void SampleScanner::NormalizeCordMap(int num_sample_infos) {
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float max_y = std::numeric_limits<float>::min();
  for (int i = 0; i < num_sample_infos; ++i) {
    const SampleInfo& sample_info = GetSampleInfo(i);
    const float x = sample_info.x;
    const float y = sample_info.y;
    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
  }
  const float x_scale = 1.0f / (max_x - min_x);
  const float y_scale = 1.0f / (max_y - min_y);
  for (int i = 0; i < num_sample_infos; ++i) {
    SampleInfo* sample_info = GetMutableSampleInfo(i);
    float& x = sample_info->x;
    float& y = sample_info->y;
    x = (x - min_x) * x_scale;
    y = (y - min_y) * y_scale;
  }
}

int SampleScanner::AnalyzeFile(SdFileWithHash* sdfile_with_hash,
                               SampleInfo* sample_info,
                               daisy::DaisySeed* seed) {
  WavFile new_streamer;
  int err = new_streamer.Init(&sdfile_with_hash->sdfile);
  if (err != 0 || !new_streamer.IsInitialized()) {
    return -1;
  }
  err = new_streamer.PopulateSampleInfo(sample_info);
  if (err != 0) {
    return -1;
  }

  sample_info->sdfile = &sdfile_with_hash->sdfile;
  sample_info->hash = sdfile_with_hash->hash;

  SamplePlayer sample_player(sample_info);
  sample_player.Play();
  int num_samples = 0;
  while (sample_player.IsPlaying()) {
    sample_player.Prepare();
    ++num_samples;
    feature_generator_.AddSample(sample_player.GetSample() / 32768.0f);
  }
  seed->PrintLine("Computing coordinates");
  feature_generator_.GetCloudCoordinates(&sample_info->x, &sample_info->y);
  seed->PrintLine("Computing coordinates done");
  return 0;
}
