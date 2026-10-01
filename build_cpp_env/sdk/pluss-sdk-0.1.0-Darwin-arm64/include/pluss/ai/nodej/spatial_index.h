#pragma once

#include "pluss/ai/nodej/dataset.h"

#include <cstdint>
#include <vector>

namespace pluss::ai::nodej {

struct KNearestResult {
  Series indices = pluss::series::undefined();     // [n_queries, k], int64 -- row indices into the built index
  Series distances2 = pluss::series::undefined();  // [n_queries, k], float64 -- squared distance, ascending per row
};

// Exact k-nearest-neighbor index over a fixed point set, backed by a classic
// median-split KD-tree (nth_element build, bounded max-heap query with
// hyperplane-distance pruning). Query results are always exact -- same
// k-nearest set brute force would return, just found faster on average.
//
// CPU-only by construction: the tree is a real host-memory recursive
// structure, not a Series-vectorized computation, so it cannot run on GPU.
// This is meant to replace the CPU host-loop brute-force k-NN already
// present in a few places (family_classifier.cpp's knn_probability_cpu,
// family_regressor.cpp's knn_predict_cpu, balance.cpp's BlendNeighbors
// self-query) -- it is NOT a replacement for the separate
// pairwise_distance2-based Series/matmul path those same files already keep
// as their GPU fallback; that path stays exactly as-is.
//
// No separate high-dimensionality brute-force fallback: a KD-tree's pruning
// weakens as dimensionality grows (the "curse of dimensionality"), so above
// roughly a few dozen dimensions this degrades toward brute-force *cost*,
// but it never degrades *correctness* -- results stay exact regardless of
// dimensionality. Adding a second, separately-tuned code path for that case
// was judged premature without profiling data motivating a specific
// threshold; this can be revisited if it turns out to matter in practice.
class SpatialIndex {
 public:
  SpatialIndex() = default;

  // rows: [n, dim] Series. Copies into host memory once; the index owns its
  // own copy and does not alias `rows` afterward.
  [[nodiscard]] static StatusOr<SpatialIndex> build(const Series& rows);

  // query_rows: [n_queries, dim], must match the index's dimensionality. k is
  // clamped to the index's point count if larger.
  [[nodiscard]] StatusOr<KNearestResult> query_k(const Series& query_rows, std::int64_t k) const;

  // Self-vs-self k-NN: for each of the n indexed points, its k nearest
  // *other* points within the same index -- each point's own zero-distance
  // match to itself is excluded outright, not just deprioritized by
  // distance (so it can't crowd out a real neighbor, and duplicate points
  // at distance 0 are still returned correctly). k is clamped to n-1.
  [[nodiscard]] StatusOr<KNearestResult> query_self_k(std::int64_t k) const;

  [[nodiscard]] std::int64_t size() const noexcept { return static_cast<std::int64_t>(points_.size() / static_cast<std::size_t>(dim_ > 0 ? dim_ : 1)); }
  [[nodiscard]] std::int64_t dim() const noexcept { return dim_; }

 private:
  struct Node {
    std::int32_t point_index = -1;
    std::int32_t axis = -1;
    std::int32_t left = -1;
    std::int32_t right = -1;
  };

  std::vector<double> points_{};  // n * dim_, row-major host copy
  std::vector<Node> nodes_{};
  std::int64_t dim_ = 0;
  std::int32_t root_ = -1;
  Platform platform_ = pluss::pCPU;

  [[nodiscard]] const double* point_(std::int32_t index) const noexcept {
    return points_.data() + static_cast<std::size_t>(index) * static_cast<std::size_t>(dim_);
  }

  [[nodiscard]] std::int32_t build_recursive_(std::vector<std::int32_t>& indices,
                                              std::int64_t lo,
                                              std::int64_t hi,
                                              std::int64_t depth);

  void search_k_recursive_(std::int32_t node_index,
                           const double* query,
                           std::int64_t k,
                           std::int32_t exclude_point_index,
                           std::vector<std::pair<double, std::int32_t>>& heap) const;

  [[nodiscard]] StatusOr<KNearestResult> query_k_impl_(const double* query_data,
                                                        std::int64_t n_queries,
                                                        std::int64_t k,
                                                        bool self_query) const;
};

} // namespace pluss::ai::nodej
