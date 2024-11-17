#include "sample_search.h"

#include <memory>
#include <vector>

#define NANOFLANN_NO_THREADS 1
#include "3p/nanoflann/include/nanoflann.hpp"
#include "common.h"
#include "daisy_seed.h"
#include "sample_info_storage.h"

struct PointCloud {
  PointCloud(int num_samples) : num_samples_(num_samples) {}
  using coord_t = float;  //!< The type of each coordinate

  // Must return the number of data points
  inline size_t kdtree_get_point_count() const { return num_samples_; }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate
  // value, the
  //  "if/else's" are actually solved at compile time.
  inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
    auto& hw = DaisyHw::Get();
    hw.PrintLine("kdtree_get_pt idx: %d", idx);
    const SampleInfo& sample_info = GetSampleInfo(idx);
    hw.PrintLine("sample_info: %d, %d", (int)(sample_info.x * 100),
                 (int)(sample_info.y * 100));
    if (dim == 0) {
      return sample_info.x;
    }
    return sample_info.y;
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

  int num_samples_;
};

// construct a kd-tree index:
using KdTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 2 /* dim */
    >;

class SampleSearchImpl {
 public:
  void Init() {
    cloud_ = std::unique_ptr<PointCloud>(new PointCloud(GetNumSampleInfos()));
    kd_tree_ = std::unique_ptr<KdTree>(
        new KdTree(2 /*dim*/, *cloud_, {6 /* max leaf */}));
    result_set_ =
        std::make_unique<nanoflann::KNNResultSet<float>>(kKdTreeMaxResults);
    result_set_->init(return_indices_.data(), return_sqr_distances_.data());
  }

  const SampleInfo& Lookup(float x, float y) {
    float query_pt[2] = {x, y};
    kd_tree_->findNeighbors(*result_set_, &query_pt[0]);
    return GetSampleInfo(return_indices_[0]);
  }

 private:
  std::unique_ptr<KdTree> kd_tree_;
  std::unique_ptr<nanoflann::KNNResultSet<float>> result_set_;
  std::unique_ptr<PointCloud> cloud_;

  std::array<size_t, kKdTreeMaxResults> return_indices_;
  std::array<float, kKdTreeMaxResults> return_sqr_distances_;
};

SampleSearch::SampleSearch() : sample_search_impl_(new SampleSearchImpl) {}
SampleSearch::~SampleSearch() = default;

void SampleSearch::Init() { sample_search_impl_->Init(); }

const SampleInfo& SampleSearch::Lookup(float x, float y) {
  return sample_search_impl_->Lookup(x, y);
}
