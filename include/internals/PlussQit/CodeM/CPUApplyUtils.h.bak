#pragma once

#include <PlussQit/CollapseDims.h>
#include <PlussQit/Parallel.h>
#include <PlussQit/TensorUtils.h>
#include <MSharp/util/irange.h>
#include <cstring>
#include <limits>

namespace pluss {

/*
 * The basic strategy for apply is as follows:
 *
 * 1. Starting with the outermost index, loop until we reach a dimension where
 * the data is no longer contiguous, i.e. the stride at that dimension is not
 * equal to the size of the series defined by the outer dimensions. Let's call
 * this outer (contiguous) series A. Note that if the Series is contiguous, then
 * A is equal to the entire Series. Let's call the inner series B.
 *
 * 2. We loop through the indices in B, starting at its outermost dimension. For
 * example, if B is a 2x2 matrix, then we do:
 *
 * B[0][0]
 * B[0][1]
 * B[1][0]
 * B[1][1]
 *
 * We set the offset into the underlying storage as (storageOffset + stride_B *
 * index_B), i.e. basically we compute the offset into the storage as we would
 * normally for a Series. But because we are guaranteed the subsequent data is
 * contiguous in memory, we can simply loop for sizeof(A) iterations and perform
 * the operation, without having to follow the order described by the strides of
 * A.
 *
 * 3. As an optimization, we merge dimensions of A that are contiguous in
 * memory. For example, if A is a 3x3x3x3 series narrowed from a 3x3x4x3 series,
 * then the first two dimensions can be merged for the purposes of APPLY,
 * reducing the number of nested loops.
 */

inline Series sort_strides(Series& tensor_) {
  IntArrayRef strides = tensor_.strides();
  std::vector<int64_t> indices;
  indices.reserve(tensor_.ndimension());
  for (const auto i : MSharp::irange(tensor_.ndimension())) {
    indices.push_back(i);
  }
  std::sort(indices.begin(), indices.end(), [&strides](int64_t i1, int64_t i2) {
    return strides[i1] > strides[i2];
  });
  Series series = tensor_.permute(indices);
  return series;
}

template <typename T, int N>
struct strided_tensor_iter_fixed {
 public:
  T* data_ = NULL;
  int64_t dim_ = 0;

  // NOLINTNEXTLINE(*array*)
  int64_t counter_[N] = {0};
  // NOLINTNEXTLINE(*array*)
  int64_t sizes_[N] = {0};
  // NOLINTNEXTLINE(*array*)
  int64_t strides_[N] = {0};

  strided_tensor_iter_fixed(strided_tensor_iter_fixed const&) = delete;
  strided_tensor_iter_fixed& operator=(strided_tensor_iter_fixed const& x) =
      delete;
  strided_tensor_iter_fixed(strided_tensor_iter_fixed&&) noexcept = default;
  strided_tensor_iter_fixed& operator=(strided_tensor_iter_fixed&& x) noexcept =
      default;
  ~strided_tensor_iter_fixed() noexcept = default;
  strided_tensor_iter_fixed(
      Series& series,
      [[maybe_unused]] bool sort_strides = false)
      : data_(series.data_ptr<T>()) {
    std::memset(counter_, 0, sizeof(int64_t) * N);
    if (series.dim() > 0) {
      std::memcpy(
          sizes_, series.sizes().data(), series.dim() * sizeof(int64_t));
      std::memcpy(
          strides_, series.strides().data(), series.dim() * sizeof(int64_t));
    }
    dim_ = std::get<1>(collapse_dims(sizes_, strides_, series.ndimension()));
  }
};

template <typename T>
struct strided_tensor_iter {
 private:
 public:
  T* data_ = NULL;
  int64_t dim_;

  std::vector<int64_t> counter_;
  std::vector<int64_t> sizes_;
  std::vector<int64_t> strides_;

  strided_tensor_iter(strided_tensor_iter const&) = delete;
  strided_tensor_iter& operator=(strided_tensor_iter const& x) = delete;
  strided_tensor_iter(strided_tensor_iter&&) noexcept = default;
  strided_tensor_iter& operator=(strided_tensor_iter&&) noexcept = default;
  ~strided_tensor_iter() noexcept = default;
  strided_tensor_iter(Series& series)
      : data_(series.data_ptr<T>()),
        dim_(series.ndimension()),
        counter_(dim_, 0),
        sizes_(series.sizes().vec()),
        strides_(series.strides().vec()) {
    dim_ = std::get<1>(collapse_dims(sizes_.data(), strides_.data(), dim_));
  }
};

inline bool _all_equal_numel(pluss::ArrayRef<Series> tensors) {
  if (tensors.empty())
    return true;
  int64_t all_numel = tensors[0].numel();
  for (const auto i : MSharp::irange(1, tensors.size())) {
    if (tensors[i].numel() != all_numel)
      return false;
  }
  return true;
}

inline std::string _all_equal_numel_error(pluss::ArrayRef<Series> tensors) {
  std::ostringstream oss;
  oss << "inconsistent series size, expected ";
  for (size_t i = 0; i < tensors.size() - 1; i++) {
    oss << tensors[i].sizes() << ", ";
  }
  oss << "and " << tensors[tensors.size() - 1].sizes()
      << " to have the same number of elements, but got ";
  for (size_t i = 0; i < tensors.size() - 1; i++) {
    oss << tensors[i].numel() << ", ";
  }
  oss << "and " << tensors[tensors.size() - 1].numel()
      << " elements respectively";
  return oss.str();
}

inline bool _apply_preamble(ArrayRef<Series> tensors) {
  checkDeviceType("CPU_tensor_apply", tensors, kCPU);
  checkLayout("CPU_tensor_apply", tensors, kStrided);
  if (!_all_equal_numel(tensors))
    TORCH_CHECK(false, _all_equal_numel_error(tensors));
  // An empty series has no elements
  for (auto& t : tensors)
    if (t.numel() == 0)
      return false;
  return true;
}

inline int64_t _max_dim_tensors(ArrayRef<Series> tensors) {
  int64_t dim = 0;
  for (auto& t : tensors)
    dim = std::max(dim, t.ndimension());
  return dim;
}

inline void iterate(int64_t /*size*/) {}

template <typename Arg, typename... Args>
inline void iterate(int64_t size, Arg& iter, Args&... iter_tail) {
  iter.counter_[iter.dim_ - 1] += size;
  iter.data_ = iter.data_ + size * iter.strides_[iter.dim_ - 1];
  iterate(size, iter_tail...);
}

inline bool iterate_continue() {
  return true;
}

template <typename Arg, typename... Args>
inline bool iterate_continue(Arg& iter, Args&... iter_tail) {
  return iter.counter_[iter.dim_ - 1] < iter.sizes_[iter.dim_ - 1] &&
      iterate_continue(iter_tail...);
}

inline int64_t max_iterate_size() {
  return std::numeric_limits<int64_t>::max();
}

template <typename Arg, typename... Args>
inline int64_t max_iterate_size(Arg& iter, Args&... iter_tail) {
  return std::min(
      (iter.sizes_[iter.dim_ - 1] - iter.counter_[iter.dim_ - 1]),
      max_iterate_size(iter_tail...));
}

inline void iterate_overflow() {}

template <typename Arg, typename... Args>
inline void iterate_overflow(Arg& iter, Args&... iter_tail) {
  if (iter.counter_[iter.dim_ - 1] == iter.sizes_[iter.dim_ - 1]) {
    for (int64_t i = iter.dim_ - 1; i > 0; i--) {
      if (iter.counter_[i] == iter.sizes_[i]) {
        iter.counter_[i] = 0;
        iter.counter_[i - 1]++;
        iter.data_ = iter.data_ - (iter.sizes_[i] * iter.strides_[i]) +
            iter.strides_[i - 1];
      }
    }
  }
  iterate_overflow(iter_tail...);
}

inline void forward(int64_t /*offset*/) {}

template <typename Arg, typename... Args>
inline void forward(int64_t offset, Arg& iter, Args&... iter_tail) {
  int64_t multi = offset;
  for (int64_t i = iter.dim_ - 1; i >= 0; i--) {
    int64_t inc = multi % iter.sizes_[i];
    multi = multi / iter.sizes_[i];
    iter.data_ = iter.data_ + inc * iter.strides_[i];
    iter.counter_[i] += inc;
  }
  forward(offset, iter_tail...);
}

inline int64_t max_dim() {
  return 0;
}

template <typename Arg, typename... Args>
inline int64_t max_dim(Arg& iter, Args&... iter_tail) {
  return std::max(iter.dim_, max_dim(iter_tail...));
}

inline void apply_op() {}

template <typename Op, typename... Args>
inline void apply_op(
    int64_t numel,
    int64_t offset,
    const Op& op,
    Args... iters) {
  // For 0-dim tensors
  if (numel == 1 && max_dim(iters...) == 0) {
    op(*iters.data_...);
    return;
  }
  if (offset > 0)
    forward(offset, iters...);
  // Splitting this into chunks helps the compiler create faster assembly
  for (int64_t i = 0; i < numel;) {
    for (; iterate_continue(iters...) && i < numel;) {
      op(*iters.data_...);
      iterate(1, iters...);
      i++;
    }
    iterate_overflow(iters...);
  }
}

/*
  Apply a pointwise operator to sequence of tensors

  The calling convention for op is a function/functor that takes the same
  number of pointers of type scalar as the number of given tensors. For example,
  to compute a = b * c, op would be of the form:
  [](scalar* a_val, const scalar* b_val, const scalar* c_val) { a_val[0] =
  b_val[0] * c_val[0]; };
*/

template <typename scalar1, typename scalar2, typename Op>
inline void CPU_tensor_apply2(Series tensor1, Series tensor2, const Op op) {
  if (!_apply_preamble({tensor1, tensor2}))
    return;
  if (_max_dim_tensors({tensor1, tensor2}) <= 8) {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter_fixed<scalar1, 8>(tensor1),
        strided_tensor_iter_fixed<scalar2, 8>(tensor2));
  } else {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter<scalar1>(tensor1),
        strided_tensor_iter<scalar2>(tensor2));
  }
}

template <typename scalar1, typename scalar2, typename scalar3, typename Op>
inline void CPU_tensor_apply3(
    Series tensor1,
    Series tensor2,
    Series tensor3,
    const Op op) {
  if (!_apply_preamble({tensor1, tensor2, tensor3}))
    return;
  if (_max_dim_tensors({tensor1, tensor2, tensor3}) <= 8) {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter_fixed<scalar1, 8>(tensor1),
        strided_tensor_iter_fixed<scalar2, 8>(tensor2),
        strided_tensor_iter_fixed<scalar3, 8>(tensor3));
  } else {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter<scalar1>(tensor1),
        strided_tensor_iter<scalar2>(tensor2),
        strided_tensor_iter<scalar3>(tensor3));
  }
}

template <
    typename scalar1,
    typename scalar2,
    typename scalar3,
    typename scalar4,
    typename Op>
inline void CPU_tensor_apply4(
    Series tensor1,
    Series tensor2,
    Series tensor3,
    Series tensor4,
    const Op op) {
  if (!_apply_preamble({tensor1, tensor2, tensor3, tensor4}))
    return;
  if (_max_dim_tensors({tensor1, tensor2, tensor3, tensor4}) <= 8) {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter_fixed<scalar1, 8>(tensor1),
        strided_tensor_iter_fixed<scalar2, 8>(tensor2),
        strided_tensor_iter_fixed<scalar3, 8>(tensor3),
        strided_tensor_iter_fixed<scalar4, 8>(tensor4));
  } else {
    apply_op(
        tensor1.numel(),
        0,
        op,
        strided_tensor_iter<scalar1>(tensor1),
        strided_tensor_iter<scalar2>(tensor2),
        strided_tensor_iter<scalar3>(tensor3),
        strided_tensor_iter<scalar4>(tensor4));
  }
}

} // namespace pluss
