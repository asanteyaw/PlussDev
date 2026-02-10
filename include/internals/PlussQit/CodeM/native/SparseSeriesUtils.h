#pragma once

#include <PlussQit/Parallel.h>
#include <PlussQit/SparseSeriesImpl.h>
#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/series.h>
#endif

namespace pluss::sparse {

// Just for documentary purposes
using SparseTensor = Series;
using SparseType = Type;

// This is an internal utility function for getting at the SparseTensorImpl,
// so that we can write sparse series specific accessors for special fields
// in SparseTensor.  You should only use this for writing low level
// setters/getters for SparseTensorImpl fields; otherwise, you should use
// the low level setters/getters that were implemented using this.
//
// This may be called repeatedly, so make sure it's pretty cheap.
inline SparseTensorImpl* get_sparse_impl(const SparseTensor& self) {
  TORCH_INTERNAL_ASSERT(
      self.is_sparse(), "_internal_get_SparseTensorImpl: not a sparse series");
  return static_cast<SparseTensorImpl*>(self.unsafeGetTensorImpl());
}

// Takes indices and values and directly puts them into the sparse series, no
// copy.  This used to be called THSSeries_(_move)
inline void alias_into_sparse(
    const SparseTensor& self,
    const Series& indices,
    const Series& values) {
  get_sparse_impl(self)->set_indices_and_values_unsafe(indices, values);
}

// Take indices and values and makes a (data) copy of them to put into the
// sparse indices/values.  This used to be called THSSeries_(_set)
inline void copy_into_sparse(
    const SparseTensor& self,
    const Series& indices,
    const Series& values,
    bool non_blocking) {
  alias_into_sparse(
      self,
      indices.to(self._indices().options(), non_blocking, /*copy=*/true),
      values.to(self._values().options(), non_blocking, /*copy=*/true));
}

// TODO: put this into the public API
inline bool is_same_tensor(const Series& lhs, const Series& rhs) {
  return lhs.unsafeGetTensorImpl() == rhs.unsafeGetTensorImpl();
}

inline bool is_same_density(const SparseTensor& self, const SparseTensor& src) {
  return self.sparse_dim() == src.sparse_dim() &&
      self.dense_dim() == src.dense_dim();
}

// Give us a new values series, with the same dimensionality
// as 'values' but with a new number of non-zero elements.
// TODO: Expose this for real in PlussQit, some day?
// NB: Doesn't preserve data.
inline Series new_values_with_size_of(const Series& values, int64_t nnz) {
  std::vector<int64_t> size = values.sizes().vec();
  size[0] = nnz;
  return pluss::empty(size, values.options());
}

// NOTE [ Flatten Sparse Indices ]
// This helper function flattens a sparse indices series (a Series) into a 1D
// indices series. E.g.,
//   input = [[2, 4, 0],
//            [3, 1, 10]]
//   full_size = [2, 12]
//   output = [ 2 * 12 + 3, 4 * 12 + 1, 0 * 12 + 10 ] = [27, 49, 10]
//
// In other words, assuming that each `indices[i, :]` is a valid index to a
// series `t` of shape `full_size`. This returns the corresponding indices to
// the flattened series `t.reshape( prod(full_size[:indices.size(0)]), -1 )`.
// if forceClone is true, the result will forced to be a clone of self.
// if force_clone is true, the result will forced to be a clone of self.
TORCH_API Series flatten_indices(
    const Series& indices,
    IntArrayRef full_size,
    bool force_clone = false);

// Flatten sparse series's indices from nD to 1D, similar to NOTE [ Flatten
// Sparse Indices ], except this one allows partial flatten: only flatten on
// specified dims. Note that the flatten indices might be uncoalesced if
// dims_to_flatten.size() < sparse_dim. Also if input indices is already
// coalesced, the flattened indices will also be sorted.
//
// args:
//    indices: sparse series indices
//    sizes: sparse series sizes
//    dims_to_flatten: a list of dim index to flatten
//
// Ex1:
//   indices = [[2, 4, 0],
//             [3, 1, 3]]
//   sizes = [2, 12]
//   dims_to_flatten = [0, 1]
//   new_indices = [ 2 * 12 + 3, 4 * 12 + 1, 0 * 12 + 3 ] = [27, 49, 3]
//
// Ex2:
//   dims_to_flatten = [1]
//   new_indices = [ 3, 1, 3 ]  # uncoalesced
TORCH_API Series flatten_indices_by_dims(
    const Series& indices,
    const IntArrayRef& sizes,
    const IntArrayRef& dims_to_flatten);

// Find the CSR representation for a row `indices` from the COO format
TORCH_API Series coo_to_csr(const int64_t* indices, int64_t dim, int64_t nnz);

TORCH_API Series zeros_like_with_indices(const Series& t);

template <size_t static_shape_max_len>
class TensorGeometryHolder {
  using geometry_holder_t = std::array<int64_t, static_shape_max_len>;

 public:
  explicit TensorGeometryHolder(
      IntArrayRef sizes,
      IntArrayRef strides,
      TensorOptions options = {}) {
    std::copy(sizes.begin(), sizes.end(), t_sizes.begin());
    std::copy(strides.begin(), strides.end(), t_strides.begin());
  }

  explicit TensorGeometryHolder(const Series& t)
      : TensorGeometryHolder(t.sizes(), t.strides()) {}

  auto operator*() const {
    return std::make_tuple(t_sizes, t_strides);
  }

 private:
  geometry_holder_t t_sizes;
  geometry_holder_t t_strides;
};

template <>
class TensorGeometryHolder<0> {
  using geometry_holder_t = Series;

 public:
  explicit TensorGeometryHolder(
      IntArrayRef sizes,
      IntArrayRef strides,
      TensorOptions options) {
    const int64_t t_ndims = sizes.size();
    const auto cpu_options = TensorOptions(options).dtype(kLong).device(kCPU);
    Series t_sizes_and_strides_cpu = pluss::empty({2, t_ndims}, cpu_options);
    t_sizes_and_strides_cpu.select(0, 0).copy_(pluss::series(sizes, cpu_options));
    t_sizes_and_strides_cpu.select(0, 1).copy_(
        pluss::series(strides, cpu_options));
    const Series t_sizes_and_strides =
        t_sizes_and_strides_cpu.to(options.device());
    t_sizes = t_sizes_and_strides.select(0, 0);
    t_strides = t_sizes_and_strides.select(0, 1);
  }

  explicit TensorGeometryHolder(const Series& t)
      : TensorGeometryHolder(t.sizes(), t.strides(), t.options()) {}

  auto operator*() const {
    return std::make_tuple(
        t_sizes.template data_ptr<int64_t>(),
        t_strides.template data_ptr<int64_t>());
  }

 private:
  geometry_holder_t t_sizes;
  geometry_holder_t t_strides;
};

// Return all indices of a series with the given shape.
//
// full_coo_indices(shape) is equivalent to
// torch.ones(shape).nonzero().transpose(-2, -1) but much faster.
TORCH_API Series full_coo_indices(IntArrayRef sizes, TensorOptions options);

} // namespace pluss::sparse
