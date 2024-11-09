#include "sample_scanner.h"

#include "common.h"
#include "daisy_seed.h"
#include "fatfs.h"
#include "feature_generator.h"
#include "sdcard.h"
#include "wav_file.h"

using namespace daisy;

struct SdFileWithHash {
  SdFile sdfile;
  unsigned long hash;
};
DSY_SDRAM_BSS SdFileWithHash sdfiles_with_hash[kMaxWavFiles];
int num_sdfiles_with_hash = 0;

DSY_SDRAM_BSS CoordinateToHash cord_to_hash_list[kMaxWavFiles];
int cord_to_hash_list_size = 0;

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

  if (ReadMapFile(kCoordinatesMapFilename, cord_to_hash_list,
                  &cord_to_hash_list_size, kMaxWavFiles) != 0) {
    seed->PrintLine("Failed to read coordinate hash list from %s",
                    kCoordinatesMapFilename);
    cord_to_hash_list_size = 0;
  } else {
    seed->PrintLine("Successfully read %d coordinate hash list from %s",
                    cord_to_hash_list_size, kCoordinatesMapFilename);
  }

  bool rescan_needed = cord_to_hash_list_size == 0;
  //                      cord_to_hash_list_size != num_sdfiles_with_hash;
  // if (!rescan_needed) {
  //   for (int i = 0; i < num_sdfiles_with_hash; ++i) {
  //     if (sdfiles_with_hash[i].hash != cord_to_hash_list[i].hash) {
  //       rescan_needed = true;
  //       break;
  //     }
  //   }
  // }
  // seed->PrintLine("No rescanning needed!");

  if (rescan_needed) {
    for (int i = 0; i < num_sdfiles_with_hash; ++i) {
      auto& sdfile_with_hash = sdfiles_with_hash[cord_to_hash_list_size];
      seed->PrintLine("Processing file %d", i);
      CoordinateToHash& cord_to_hash = cord_to_hash_list[i];
      cord_to_hash.hash = sdfile_with_hash.hash;
      if (AnalyzeFile(&sdfile_with_hash.sdfile, &cord_to_hash.x,
                      &cord_to_hash.y, seed) != 0) {
        seed->PrintLine("Can't read file");
      }
      seed->PrintLine("Coordinate x: %d, y: %d", (int)(cord_to_hash.x * 100),
                      (int)(cord_to_hash.y * 100));
      ++cord_to_hash_list_size;
    }
    NormalizeCordMap();

    seed->PrintLine("Analysis done");
    if (WriteMapFile(kCoordinatesMapFilename, cord_to_hash_list,
                     cord_to_hash_list_size) != 0) {
      seed->PrintLine("Error writing: %s", kCoordinatesMapFilename);
    }
    seed->PrintLine("Successfully wrote coordinate hash list to %s",
                    kCoordinatesMapFilename);
  }
  return 0;
}

void SampleScanner::PopulateSamples(SampleSearch* sample_search,
                                    daisy::DaisySeed* seed) {
  seed->PrintLine("Populating samples");
  for (int i = 0; i < cord_to_hash_list_size; ++i) {
    auto& cord_to_hash = cord_to_hash_list[i];
    auto itr = std::lower_bound(
        std::begin(sdfiles_with_hash),
        std::begin(sdfiles_with_hash) + num_sdfiles_with_hash,
        cord_to_hash.hash,
        [](const SdFileWithHash& sdfile_with_hash, unsigned long hash) {
          return sdfile_with_hash.hash < hash;
        });

    if (itr == std::begin(sdfiles_with_hash) + num_sdfiles_with_hash ||
        itr->hash != cord_to_hash.hash) {
      seed->PrintLine("Could not find file matching hash");
      continue;
    }
    sample_search->AddSample(cord_to_hash.x, cord_to_hash.y, &itr->sdfile);
  }
  seed->PrintLine("Done");
}

void SampleScanner::NormalizeCordMap() {
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float max_y = std::numeric_limits<float>::min();
  for (int i = 0; i < cord_to_hash_list_size; ++i) {
    const float x = cord_to_hash_list[i].x;
    const float y = cord_to_hash_list[i].y;
    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
  }
  const float x_scale = 1.0f / (max_x - min_x);
  const float y_scale = 1.0f / (max_y - min_y);
  for (int i = 0; i < cord_to_hash_list_size; ++i) {
    float& x = cord_to_hash_list[i].x;
    float& y = cord_to_hash_list[i].y;
    x = (x - min_x) * x_scale;
    y = (y - min_y) * y_scale;
  }
}

int SampleScanner::AnalyzeFile(SdFile* sdfile, float* x, float* y,
                               daisy::DaisySeed* seed) {
  WavFile new_streamer;
  int err = new_streamer.Init(sdfile);
  if (err != 0 || !new_streamer.IsInitialized()) {
    return -1;
  }

  auto player = new_streamer.GetSamplePlayer();
  player->Play();
  int num_samples = 0;
  while (player->IsPlaying()) {
    player->Prepare();
    ++num_samples;
    feature_generator_.AddSample(player->GetSample() / 32768.0f);
  }
  seed->PrintLine("Computing coordinates");
  feature_generator_.GetCloudCoordinates(x, y);
  seed->PrintLine("Computing coordinates done");
  return 0;
}
