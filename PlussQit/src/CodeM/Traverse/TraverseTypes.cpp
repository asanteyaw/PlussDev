#include "CodeM/Traverse/TraverseTypes.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "CodeM/core/Errors.h"
#include "pluss/codem/stack_array.h"

namespace pluss::codem {

TraverseMatch::TraverseMatch(
    Series coords,
    Series values,
    std::vector<std::int64_t> source_shape,
    std::int64_t indexed_rank)
    : coords_(std::move(coords)),
      values_(std::move(values)),
      source_shape_(std::move(source_shape)),
      indexed_rank_(indexed_rank) {
  PLUSS_CHECK(indexed_rank_ >= 0, "TraverseMatch: indexed_rank must be non-negative");
  PLUSS_CHECK(coords_.defined(), "TraverseMatch: coords must be defined");
  PLUSS_CHECK(coords_.dim() == 2, "TraverseMatch: coords must be rank-2 [K, R]");
  if (coords_.scalar_type() != ScalarType::Int64) {
    coords_ = coords_.to(coords_.platform(), ScalarType::Int64);
  }
  PLUSS_CHECK(coords_.size(1) == indexed_rank_, "TraverseMatch: coords second dimension must match indexed_rank");
  PLUSS_CHECK(
      static_cast<std::int64_t>(source_shape_.size()) == indexed_rank_,
      "TraverseMatch: source_shape rank must match indexed_rank");
}

TraverseMatch::TraverseMatch(Series coords, std::vector<std::int64_t> source_shape, std::int64_t indexed_rank)
    : TraverseMatch(std::move(coords), pluss::series::undefined(), std::move(source_shape), indexed_rank) {}

bool TraverseMatch::defined() const noexcept {
  return coords_.defined();
}

std::int64_t TraverseMatch::count() const {
  if (!coords_.defined()) {
    return 0;
  }
  PLUSS_CHECK(coords_.dim() == 2, "TraverseMatch::count: coords must be rank-2 [K, R]");
  return coords_.size(0);
}

std::int64_t TraverseMatch::rank() const noexcept {
  return indexed_rank_;
}

std::int64_t TraverseMatch::indexed_rank() const noexcept {
  return indexed_rank_;
}

std::span<const std::int64_t> TraverseMatch::source_shape() const noexcept {
  return std::span<const std::int64_t>(source_shape_.data(), source_shape_.size());
}

bool TraverseMatch::has_values() const noexcept {
  return values_.defined();
}

const Series& TraverseMatch::coords() const noexcept {
  return coords_;
}

const Series& TraverseMatch::values() const noexcept {
  return values_;
}

::pluss::StackArray<Series> TraverseMatch::split_indices() const {
  PLUSS_CHECK(defined(), "TraverseMatch::split_indices: undefined match");
  PLUSS_CHECK(coords_.dim() == 2, "TraverseMatch::split_indices: coords must be rank-2 [K, R]");
  if (indexed_rank_ == 0) {
    return ::pluss::StackArray<Series>{};
  }

  std::vector<Series> out;
  out.reserve(static_cast<std::size_t>(indexed_rank_));
  for (std::int64_t d = 0; d < indexed_rank_; ++d) {
    Series col = coords_.select(1, d);
    if (col.scalar_type() != ScalarType::Int64) {
      col = col.to(col.platform(), ScalarType::Int64);
    }
    out.push_back(std::move(col));
  }
  return ::pluss::StackArray<Series>(std::move(out));
}

Series TraverseMatch::linear_indices() const {
  PLUSS_CHECK(defined(), "TraverseMatch::linear_indices: undefined match");
  PLUSS_CHECK(coords_.dim() == 2, "TraverseMatch::linear_indices: coords must be rank-2 [K, R]");
  PLUSS_CHECK(
      static_cast<std::int64_t>(source_shape_.size()) == indexed_rank_,
      "TraverseMatch::linear_indices: source_shape rank must match indexed_rank");

  const std::int64_t k = count();
  if (k == 0) {
    return Series::empty({0}, ScalarType::Int64, coords_.platform());
  }
  if (indexed_rank_ == 0) {
    return Series::zeros({k}, ScalarType::Int64).to(coords_.platform(), ScalarType::Int64);
  }

  std::vector<std::int64_t> strides(static_cast<std::size_t>(indexed_rank_), 1);
  for (std::int64_t d = indexed_rank_ - 2; d >= 0; --d) {
    strides[static_cast<std::size_t>(d)] =
        strides[static_cast<std::size_t>(d + 1)] * source_shape_[static_cast<std::size_t>(d + 1)];
  }

  Series linear = Series::zeros({k}, ScalarType::Int64).to(coords_.platform(), ScalarType::Int64);
  for (std::int64_t d = 0; d < indexed_rank_; ++d) {
    Series col = coords_.select(1, d).to(coords_.platform(), ScalarType::Int64);
    Series term = col.mul(static_cast<double>(strides[static_cast<std::size_t>(d)]))
                      .to(coords_.platform(), ScalarType::Int64);
    linear = linear.add(term).to(coords_.platform(), ScalarType::Int64);
  }
  return linear;
}

Series TraverseMatch::select_from(const Series& other) const {
  PLUSS_CHECK(defined(), "TraverseMatch::select_from: undefined match");
  PLUSS_CHECK(indexed_rank_ <= other.dim(), "TraverseMatch::select_from: indexed rank exceeds target rank");

  if (static_cast<std::int64_t>(source_shape_.size()) == indexed_rank_) {
    for (std::int64_t d = 0; d < indexed_rank_; ++d) {
      PLUSS_CHECK(
          other.size(d) == source_shape_[static_cast<std::size_t>(d)],
      "TraverseMatch::select_from: selector/source shape mismatch at indexed dimension");
    }
  }

  if (count() == 0) {
    std::vector<std::int64_t> empty_shape{0};
    for (std::int64_t d = indexed_rank_; d < other.dim(); ++d) {
      empty_shape.push_back(other.size(d));
    }
    return Series::empty(empty_shape, other.scalar_type(), other.platform());
  }

  if (indexed_rank_ == 0) {
    PLUSS_CHECK(count() == 1, "TraverseMatch::select_from: scalar selector must have count 0 or 1");
    return other.clone();
  }

  const auto idx = split_indices();
  std::vector<ExtractSpec> specs;
  specs.reserve(static_cast<std::size_t>(other.dim()));

  for (std::int64_t d = 0; d < indexed_rank_; ++d) {
    Series id = idx[static_cast<std::size_t>(d)];
    if (id.scalar_type() != ScalarType::Int64 || id.platform() != other.platform()) {
      id = id.to(other.platform(), ScalarType::Int64);
    }
    specs.emplace_back(id);
  }
  for (std::int64_t d = indexed_rank_; d < other.dim(); ++d) {
    specs.emplace_back(pluss::Slicing::SliceAll());
  }

  return other.index(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
}

} // namespace pluss::codem
