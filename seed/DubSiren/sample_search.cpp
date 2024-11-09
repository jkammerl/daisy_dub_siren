#include "sample_search.h"

#include <memory>
#include <vector>

#define NANOFLANN_NO_THREADS 1
#include "3p/nanoflann/include/nanoflann.hpp"
#include "common.h"
#include "daisy_seed.h"

struct CoordinatesToSdFile {
  float x, y;
  SdFile* sdfile;
};

DSY_SDRAM_BSS CoordinatesToSdFile coordinates_to_sdfile[kMaxWavFiles];
int num_coordinates_to_sdfile = 0;

struct PointCloud {
  using coord_t = float;  //!< The type of each coordinate

  // Must return the number of data points
  inline size_t kdtree_get_point_count() const {
    return num_coordinates_to_sdfile;
  }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate
  // value, the
  //  "if/else's" are actually solved at compile time.
  inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
    if (dim == 0) {
      return coordinates_to_sdfile[idx].x;
    }
    return coordinates_to_sdfile[idx].y;
  }

  // Optional bounding-box computation: return false to default to a standard
  // bbox computation loop.
  //   Return true if the BBOX was already computed by the class and returned
  //   in "bb" so it can be avoided to redo it again. Look at bb.size() to
  //   find out the expected dimensionality (e.g. 2 or 3 for point clouds)
  template <class BBOX>
  bool kdtree_get_bbox(BBOX& /* bb */) const {
    return false;
  }
};

// construct a kd-tree index:
using KdTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 2 /* dim */
    >;

class SampleSearchImpl {
 public:
  void AddSample(float x, float y, SdFile* wav_file) {
    CoordinatesToSdFile& entry =
        coordinates_to_sdfile[num_coordinates_to_sdfile];
    entry.x = x;
    entry.y = y;
    entry.sdfile = wav_file;
    ++num_coordinates_to_sdfile;
  }

  void ComputeKdTree() {
    kd_tree_ = std::unique_ptr<KdTree>(
        new KdTree(2 /*dim*/, cloud_, {10 /* max leaf */}));
    result_set_ =
        std::make_unique<nanoflann::KNNResultSet<float>>(kKdTreeMaxResults);
    result_set_->init(return_indices_.data(), return_sqr_distances_.data());
  }

  SdFile* Lookup(float x, float y) {
    float query_pt[2] = {x, y};
    kd_tree_->findNeighbors(*result_set_, &query_pt[0]);
    return coordinates_to_sdfile[return_indices_[0]].sdfile;
  }

 private:
  std::unique_ptr<KdTree> kd_tree_;
  std::unique_ptr<nanoflann::KNNResultSet<float>> result_set_;
  PointCloud cloud_;

  std::array<size_t, kKdTreeMaxResults> return_indices_;
  std::array<float, kKdTreeMaxResults> return_sqr_distances_;
};

SampleSearch::SampleSearch() : sample_search_impl_(new SampleSearchImpl) {}
SampleSearch::~SampleSearch() = default;

void SampleSearch::AddSample(float x, float y, SdFile* wav_file) {
  sample_search_impl_->AddSample(x, y, wav_file);
}

void SampleSearch::ComputeKdTree() { sample_search_impl_->ComputeKdTree(); }

SdFile* SampleSearch::Lookup(float x, float y) {
  return sample_search_impl_->Lookup(x, y);
}
