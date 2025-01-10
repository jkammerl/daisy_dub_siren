#include "feature_generator.h"

#include "constants.h"
#include "daisy_seed.h"

std::array<std::array<float, kNumMelBands>, kMaxMfccs> DSY_SDRAM_BSS g_mfccs;

std::array<float, kNumMelBands>* GetMfccBuffer(int i) { return &g_mfccs[i]; }
