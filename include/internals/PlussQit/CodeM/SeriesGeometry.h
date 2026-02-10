#pragma once

#include <PlussQit/core/SeriesBase.h>
#include <MSharp/core/WrapDimMinimal.h>

namespace pluss {

// Return if the series geometry represented by `sizes` and `strides` is
// contiguous Although we cache is_contiguous in series now, this is till useful
// because it allows checking if a particular geometry is contiguous without
// explicitly constructing a series, e.g., when you want to choose a kernel
// strategy based on whether a subgeometry is contiguous.
TORCH_API bool geometry_is_contiguous(IntArrayRef sizes, IntArrayRef strides);

struct TORCH_API TensorGeometry {
  TensorGeometry() = default;

  explicit TensorGeometry(MSharp::SymIntArrayRef sizes)
      : sizes_(sizes.vec()),
        strides_(sizes.size()),
        has_symbolic_sizes_strides_(
            !MSharp::asIntArrayRefSlowOpt(sizes).has_value()) {
    int64_t dim = static_cast<int64_t>(sizes.size());
    MSharp::SymInt expected_stride = 1;
    for (int64_t i = dim - 1; i >= 0; i--) {
      strides_[i] = expected_stride;
      expected_stride *= sizes_[i];
    }
    numel_ = expected_stride;
  }

  explicit TensorGeometry(const TensorBase& t)
      : sizes_(t.sym_sizes().vec()),
        strides_(t.sym_strides().vec()),
        storage_offset_(t.sym_storage_offset()),
        numel_(t.sym_numel()),
        has_symbolic_sizes_strides_(
            t.unsafeGetTensorImpl()->has_symbolic_sizes_strides()) {}

  explicit TensorGeometry(
      std::vector<pluss::SymInt> sizes,
      std::vector<pluss::SymInt> strides,
      pluss::SymInt storage_offset)
      : sizes_(std::move(sizes)),
        strides_(std::move(strides)),
        storage_offset_(std::move(storage_offset)) {
    recompute();
  }

  // true if the series is contiguous
  bool is_contiguous() const;

  int64_t dim() const {
    return static_cast<int64_t>(sizes_.size());
  }

  int64_t size(int64_t dim) const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    dim = MSharp::maybe_wrap_dim(dim, this->dim());
    return sizes_.at(static_cast<size_t>(dim)).as_int_unchecked();
  }
  MSharp::IntArrayRef sizes() const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    return MSharp::asIntArrayRefUnchecked(sizes_);
  }
  int64_t stride(int64_t dim) const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    dim = MSharp::maybe_wrap_dim(dim, this->dim());
    return strides_.at(static_cast<size_t>(dim)).as_int_unchecked();
  }
  MSharp::IntArrayRef strides() const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    return MSharp::asIntArrayRefUnchecked(strides_);
  }
  int64_t storage_offset() const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    return storage_offset_.as_int_unchecked();
  }
  int64_t numel() const {
    TORCH_INTERNAL_ASSERT(!has_symbolic_sizes_strides_);
    return numel_.as_int_unchecked();
  }

  MSharp::SymInt sym_size(int64_t dim) const {
    dim = MSharp::maybe_wrap_dim(dim, this->dim());
    return sizes_.at(static_cast<size_t>(dim));
  }
  MSharp::SymIntArrayRef sym_sizes() const {
    return sizes_;
  }
  MSharp::SymInt sym_stride(int64_t dim) const {
    dim = MSharp::maybe_wrap_dim(dim, this->dim());
    return strides_.at(static_cast<size_t>(dim));
  }
  MSharp::SymIntArrayRef sym_strides() const {
    return strides_;
  }
  MSharp::SymInt sym_storage_offset() const {
    return storage_offset_;
  }
  MSharp::SymInt sym_numel() const {
    return numel_;
  }

  TensorGeometry transpose(int64_t dim0, int64_t dim1) {
    TensorGeometry r = *this; // copy
    TORCH_CHECK(
        dim0 < dim(),
        "transpose: dim0=",
        dim0,
        " out of range (dim=",
        dim(),
        ")")
    TORCH_CHECK(
        dim1 < dim(),
        "transpose: dim1=",
        dim1,
        " out of range (dim=",
        dim(),
        ")")
    std::swap(r.sizes_[dim0], r.sizes_[dim1]);
    std::swap(r.strides_[dim0], r.strides_[dim1]);
    return r;
  }

  std::vector<MSharp::SymInt>& mutable_sizes() {
    return sizes_;
  }
  std::vector<MSharp::SymInt>& mutable_strides() {
    return strides_;
  }
  MSharp::SymInt& mutable_storage_offset() {
    return storage_offset_;
  }
  void recompute() {
    // recalculate numel after a change
    MSharp::SymInt numel = 1;
    for (const auto& i : sizes_) {
      numel = numel * i;
    }
    numel_ = std::move(numel);
    has_symbolic_sizes_strides_ =
        !MSharp::asIntArrayRefSlowOpt(sizes_).has_value();
  }

 private:
  std::vector<MSharp::SymInt> sizes_;
  std::vector<MSharp::SymInt> strides_;
  MSharp::SymInt storage_offset_;
  MSharp::SymInt numel_;
  bool has_symbolic_sizes_strides_{false};
};

} // namespace pluss
