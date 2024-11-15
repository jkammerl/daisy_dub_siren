#pragma once
#ifndef KD_TREE_
#define KD_TREE_

#include <memory>

#include "common.h"

class SdFile;
class SampleSearchImpl;

class SampleSearch {
 public:
  SampleSearch();
  ~SampleSearch();

  void Init();

  const SampleInfo& Lookup(float x, float y);

 private:
  std::unique_ptr<SampleSearchImpl> sample_search_impl_;
};

#endif  // KD_TREE_