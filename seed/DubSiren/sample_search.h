#pragma once
#ifndef KD_TREE_
#define KD_TREE_

#include <memory>

class SdFile;
class SampleSearchImpl;

class SampleSearch {
 public:
  SampleSearch();
  ~SampleSearch();

  void AddSample(float x, float y, SdFile* wav_file);

  void ComputeKdTree();

  SdFile* Lookup(float x, float y);

 private:
  std::unique_ptr<SampleSearchImpl> sample_search_impl_;
};

#endif  // KD_TREE_