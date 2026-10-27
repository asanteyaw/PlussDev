#include "pluss/ai/nodej/spatial_index.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <utility>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Config i64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

} // namespace

std::int32_t SpatialIndex::build_recursive_(std::vector<std::int32_t>& indices,
                                            std::int64_t lo,
                                            std::int64_t hi,
                                            std::int64_t depth) {
  if (lo >= hi) return -1;
  const std::int64_t axis = depth % dim_;
  const std::int64_t mid = lo + (hi - lo) / 2;

  std::nth_element(indices.begin() + lo,
                   indices.begin() + mid,
                   indices.begin() + hi,
                   [&](std::int32_t a, std::int32_t b) { return point_(a)[axis] < point_(b)[axis]; });

  // Reserve this node's slot before recursing so left/right children (built
  // afterward) can be linked back to it -- nodes_ may reallocate as children
  // are appended, so every subsequent write goes through node_index (a plain
  // integer, stable across reallocation), never a held Node&/pointer.
  const auto node_index = static_cast<std::int32_t>(nodes_.size());
  nodes_.push_back(Node{});
  const std::int32_t point_idx = indices[static_cast<std::size_t>(mid)];
  const std::int32_t left = build_recursive_(indices, lo, mid, depth + 1);
  const std::int32_t right = build_recursive_(indices, mid + 1, hi, depth + 1);
  nodes_[static_cast<std::size_t>(node_index)] = Node{
      .point_index = point_idx,
      .axis = static_cast<std::int32_t>(axis),
      .left = left,
      .right = right,
  };
  return node_index;
}

void SpatialIndex::search_k_recursive_(std::int32_t node_index,
                                       const double* query,
                                       std::int64_t k,
                                       std::int32_t exclude_point_index,
                                       std::vector<std::pair<double, std::int32_t>>& heap) const {
  if (node_index < 0) return;
  const Node& node = nodes_[static_cast<std::size_t>(node_index)];
  const double* p = point_(node.point_index);

  if (node.point_index != exclude_point_index) {
    double d2 = 0.0;
    for (std::int64_t f = 0; f < dim_; ++f) {
      const double dv = query[f] - p[f];
      d2 += dv * dv;
    }
    if (static_cast<std::int64_t>(heap.size()) < k) {
      heap.emplace_back(d2, node.point_index);
      if (static_cast<std::int64_t>(heap.size()) == k) {
        std::make_heap(heap.begin(), heap.end());
      }
    } else if (d2 < heap.front().first) {
      std::pop_heap(heap.begin(), heap.end());
      heap.back() = std::pair<double, std::int32_t>{d2, node.point_index};
      std::push_heap(heap.begin(), heap.end());
    }
  }

  const double diff = query[node.axis] - p[node.axis];
  const std::int32_t near_child = diff < 0.0 ? node.left : node.right;
  const std::int32_t far_child = diff < 0.0 ? node.right : node.left;

  search_k_recursive_(near_child, query, k, exclude_point_index, heap);

  const double worst = (static_cast<std::int64_t>(heap.size()) < k)
                           ? std::numeric_limits<double>::infinity()
                           : heap.front().first;
  // Only descend into the far side if its splitting hyperplane is closer
  // than the current worst kept neighbor -- the actual pruning step that
  // makes this faster than brute force on average.
  if (diff * diff < worst) {
    search_k_recursive_(far_child, query, k, exclude_point_index, heap);
  }
}

StatusOr<KNearestResult> SpatialIndex::query_k_impl_(const double* query_data,
                                                      std::int64_t n_queries,
                                                      std::int64_t k,
                                                      bool self_query) const {
  const std::int64_t n_points = size();
  if (n_points <= 0) {
    return StatusOr<KNearestResult>(Status::invalid_argument("SpatialIndex::query_k: index is empty."));
  }
  if (k <= 0) {
    return StatusOr<KNearestResult>(Status::invalid_argument("SpatialIndex::query_k: k must be positive."));
  }
  const std::int64_t available = self_query ? n_points - 1 : n_points;
  const std::int64_t effective_k = std::min(k, available);
  if (effective_k <= 0) {
    return StatusOr<KNearestResult>(
        Status::invalid_argument("SpatialIndex::query_k: not enough other points in the index to satisfy k."));
  }

  std::vector<double> out_dist2(static_cast<std::size_t>(n_queries * effective_k));
  std::vector<std::int64_t> out_idx(static_cast<std::size_t>(n_queries * effective_k));
  std::vector<std::pair<double, std::int32_t>> heap{};
  heap.reserve(static_cast<std::size_t>(effective_k));

  for (std::int64_t qi = 0; qi < n_queries; ++qi) {
    heap.clear();
    const double* q = query_data + qi * dim_;
    const std::int32_t exclude = self_query ? static_cast<std::int32_t>(qi) : -1;
    search_k_recursive_(root_, q, effective_k, exclude, heap);
    std::sort_heap(heap.begin(), heap.end());
    for (std::int64_t j = 0; j < effective_k; ++j) {
      out_dist2[static_cast<std::size_t>(qi * effective_k + j)] = heap[static_cast<std::size_t>(j)].first;
      out_idx[static_cast<std::size_t>(qi * effective_k + j)] = heap[static_cast<std::size_t>(j)].second;
    }
  }

  KNearestResult result{};
  result.indices = pluss::make_series(std::move(out_idx), {n_queries, effective_k}, i64_on(platform_));
  result.distances2 = pluss::make_series(std::move(out_dist2), {n_queries, effective_k}, f64_on(platform_));
  return StatusOr<KNearestResult>(std::move(result));
}

StatusOr<SpatialIndex> SpatialIndex::build(const Series& rows) {
  if (!rows.defined()) {
    return StatusOr<SpatialIndex>(Status::invalid_argument("SpatialIndex::build requires a defined Series."));
  }
  if (rows.dim() != 2) {
    return StatusOr<SpatialIndex>(Status::shape_mismatch("SpatialIndex::build requires a rank-2 [n, dim] Series."));
  }
  const std::int64_t n = rows.sizes()[0];
  const std::int64_t dim = rows.sizes()[1];
  if (n <= 0 || dim <= 0) {
    return StatusOr<SpatialIndex>(Status::shape_mismatch("SpatialIndex::build requires n > 0 and dim > 0."));
  }

  SpatialIndex index{};
  index.dim_ = dim;
  index.platform_ = rows.platform();
  index.points_ =
      rows.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({n * dim}).host<double>();

  std::vector<std::int32_t> indices(static_cast<std::size_t>(n));
  std::iota(indices.begin(), indices.end(), 0);
  index.nodes_.reserve(static_cast<std::size_t>(n));
  index.root_ = index.build_recursive_(indices, 0, n, 0);
  return StatusOr<SpatialIndex>(std::move(index));
}

StatusOr<KNearestResult> SpatialIndex::query_k(const Series& query_rows, std::int64_t k) const {
  if (!query_rows.defined()) {
    return StatusOr<KNearestResult>(Status::invalid_argument("SpatialIndex::query_k requires a defined Series."));
  }
  if (query_rows.dim() != 2 || query_rows.sizes()[1] != dim_) {
    return StatusOr<KNearestResult>(Status::shape_mismatch(
        "SpatialIndex::query_k requires a rank-2 [n_queries, dim] Series matching the index's dimensionality."));
  }
  const std::int64_t n_queries = query_rows.sizes()[0];
  const auto data =
      query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({n_queries * dim_}).host<double>();
  return query_k_impl_(data.data(), n_queries, k, /*self_query=*/false);
}

StatusOr<KNearestResult> SpatialIndex::query_self_k(std::int64_t k) const {
  return query_k_impl_(points_.data(), size(), k, /*self_query=*/true);
}

} // namespace pluss::ai::nodej
