#pragma once
#ifndef BUFFER_CACHE_
#define BUFFER_CACHE_

#include <list>
#include <memory>

#include "common.h"
#include "constants.h"

const SampleInfo& GetSampleInfo(int id);

SampleInfo* GetMutableSampleInfo(int id);

void SetNumSampleInfos(int num_sample_infos);

int GetNumSampleInfos();

#endif  // BUFFER_CACHE_