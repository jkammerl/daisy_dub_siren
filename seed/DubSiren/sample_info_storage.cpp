#include "sample_info_storage.h"

#include "daisy_seed.h"

using namespace daisy;

SampleInfo DSY_SDRAM_BSS g_memory[kMaxWavFiles];
int g_num_sample_infos;

const SampleInfo& GetSampleInfo(int id) {
  assert(id >= 0 && id < kMaxWavFiles);
  return g_memory[id];
}

SampleInfo* GetMutableSampleInfo(int id) {
  assert(id >= 0 && id < kMaxWavFiles);
  return &g_memory[id];
}
void SetNumSampleInfos(int num_sample_infos) {
  g_num_sample_infos = num_sample_infos;
}

int GetNumSampleInfos() { return g_num_sample_infos; }
