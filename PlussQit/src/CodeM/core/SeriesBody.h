#pragma once

#include "CodeM/core/Series.h"
#include "CodeM/core/factories/ValueInitList.h"
#include "CodeM/core/Storage.h"
#include "CodeM/core/Errors.h"

// Generated ops headers (produced into build dir)
#include "CodeM/ops/add_ops.h"
#include "CodeM/ops/sub_ops.h"
#include "CodeM/ops/mul_ops.h"
#include "CodeM/ops/div_ops.h"
#include "CodeM/ops/neg_ops.h"
#include "CodeM/ops/abs_ops.h"
#include "CodeM/ops/exp_ops.h"
#include "CodeM/ops/log_ops.h"
#include "CodeM/ops/sqrt_ops.h"
#include "CodeM/ops/sin_ops.h"
#include "CodeM/ops/cos_ops.h"
#include "CodeM/ops/tanh_ops.h"
#include "CodeM/ops/sum_ops.h"
#include "CodeM/ops/mean_ops.h"
#include "CodeM/ops/amax_ops.h"
#include "CodeM/ops/amin_ops.h"
#include "CodeM/ops/argmax_ops.h"
#include "CodeM/ops/argmin_ops.h"
#include "CodeM/ops/index_select_ops.h"
#include "CodeM/ops/masked_select_ops.h"
#include "CodeM/ops/matmul_ops.h"
#include "CodeM/ops/gather_ops.h"
#include "CodeM/ops/scatter_ops.h"
#include "CodeM/ops/scatter_add_ops.h"
#include "CodeM/ops/where_ops.h"
#include "CodeM/ops/masked_fill_ops.h"
#include "CodeM/ops/nonzero_ops.h"
#include "CodeM/ops/take_ops.h"
#include "CodeM/ops/put_ops.h"
#include "CodeM/ops/index_ops.h"
#include "CodeM/ops/index_put_ops.h"
#include "CodeM/ops/cat_ops.h"
#include "CodeM/ops/stack_ops.h"
#include "CodeM/ops/eq_ops.h"
#include "CodeM/ops/ne_ops.h"
#include "CodeM/ops/lt_ops.h"
#include "CodeM/ops/le_ops.h"
#include "CodeM/ops/gt_ops.h"
#include "CodeM/ops/ge_ops.h"
#include "CodeM/ops/minimum_ops.h"
#include "CodeM/ops/maximum_ops.h"
#include "CodeM/ops/clamp_ops.h"

namespace pluss::codem {

// Scalar constants / factories (CPU-only).
inline Series nan(ScalarType dtype=ScalarType::Float32) {
  const double v = std::numeric_limits<double>::quiet_NaN();
  return Series::full({}, v, dtype);
}
inline Series inf(ScalarType dtype=ScalarType::Float32) {
  const double v = std::numeric_limits<double>::infinity();
  return Series::full({}, v, dtype);
}
inline Series neg_inf(ScalarType dtype=ScalarType::Float32) {
  const double v = -std::numeric_limits<double>::infinity();
  return Series::full({}, v, dtype);
}

// Create a scalar Series (rvalue-style), similar to torch::tensor(scalar).
inline Series value(double v, ScalarType dtype = ScalarType::Float32) {
  return Series::full({}, v, dtype);
}


// Scalar overloads: integral defaults to Int64; bool defaults to Bool.
template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
inline Series value(T v, ScalarType dtype = ScalarType::Int64) {
  return Series::full({}, static_cast<double>(v), dtype);
}
inline Series value(bool v, ScalarType dtype = ScalarType::Bool) {
  return Series::full({}, v ? 1.0 : 0.0, dtype);
}

// Nested initializer_list: infer shape and flatten. Supports up to 7D (enforced by depth check).
template <class T>
inline Series value(std::initializer_list<T> list, ScalarType dtype = ScalarType::Float32) {
  static_assert(pluss::codem::factories::depth<std::initializer_list<T>>::value <= 7,
                "pluss::value: supports up to 7D nested initializer_list");
  auto inf = pluss::codem::factories::infer(list);
  return Series::make_from_doubles(inf.data, inf.sizes, dtype);
}

// Create a Series by copying from an lvalue buffer (e.g. std::vector / span).
// Overload that deduces T from std::vector<T>.
template <class T>
inline Series make_series(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
  return make_series<T>(std::span<const T>(data.data(), data.size()), sizes, cfg);
}

// Overload that deduces T from std::span<T> + initializer_list sizes.
template <class T>
inline Series make_series(std::span<const T> data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
  return make_series<T>(data, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
}

template <class T>
inline Series make_series(std::span<const T> data, std::span<const std::int64_t> sizes, Config cfg = {}) {
  // Infer dtype from T if user didn't override explicitly.
  ScalarType dt = cfg.dtype;
  if (dt == ScalarType::Float32) {
    if constexpr (std::is_same_v<T,double>) dt = ScalarType::Float64;
    else if constexpr (std::is_same_v<T,float>) dt = ScalarType::Float32;
    else if constexpr (std::is_same_v<T,std::int64_t>) dt = ScalarType::Int64;
    else if constexpr (std::is_same_v<T,std::int32_t>) dt = ScalarType::Int32;
    else if constexpr (std::is_same_v<T,std::int16_t>) dt = ScalarType::Int16;
    else if constexpr (std::is_same_v<T,std::int8_t>) dt = ScalarType::Int8;
    else if constexpr (std::is_same_v<T,std::uint8_t>) dt = ScalarType::UInt8;
    else if constexpr (std::is_same_v<T,bool>) dt = ScalarType::Bool;
  }
  Series out = Series::empty(sizes, dt).contiguous();
  const std::size_t n = (std::size_t)out.numel();
  PLUSS_CHECK(data.size() == n, "make_series: data size mismatch with sizes");
  std::memcpy(const_cast<std::byte*>(out.raw_data()), data.data(), n * out.itemsize());
  return out;
}

// Expose external memory without taking ownership (like torch::from_blob).
// `deleter` will be invoked when the last Series view is destroyed.
using BlobDeleter = std::function<void(void*)>;

inline Series from_blob(
    void* data,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    BlobDeleter deleter,
    ScalarType dtype = ScalarType::Float32) {

  PLUSS_CHECK(data != nullptr, "from_blob: null data");
  PLUSS_CHECK(sizes.size() == strides.size(), "from_blob: sizes/strides rank mismatch");
  const auto item = dtype_of(dtype).itemsize;

  // conservative byte-size: max offset + 1 element
  std::int64_t max_off = 0;
  for (std::size_t i=0;i<sizes.size();++i) {
    if (sizes[i] == 0) continue;
    max_off += (sizes[i]-1) * strides[i];
  }
  const std::size_t nbytes = (std::size_t)(max_off + 1) * (std::size_t)item;

  auto sp = std::shared_ptr<std::byte>((std::byte*)data, [deleter](std::byte* p){
    if (deleter) deleter((void*)p);
  });
  Storage stor(sp, nbytes, 1);
  return Series::from_storage(stor,
      std::vector<std::int64_t>(sizes.begin(), sizes.end()),
      std::vector<std::int64_t>(strides.begin(), strides.end()),
      dtype);
}

inline Series arange(double start, double end, double step=1.0, ScalarType dtype=ScalarType::Int64) { return Series::arange(start,end,step,dtype); }
inline Series linspace(double start, double end, std::int64_t steps, ScalarType dtype=ScalarType::Float32) { return Series::linspace(start,end,steps,dtype); }
inline Series eye(std::int64_t n, std::int64_t m=-1, ScalarType dtype=ScalarType::Float32) { return Series::eye(n, (m<0)?n:m, dtype); }
inline Series rand(std::span<const std::int64_t> sizes, ScalarType dtype=ScalarType::Float32, std::uint64_t seed=0) { return Series::rand(sizes,dtype,seed); }
inline Series randn(std::span<const std::int64_t> sizes, ScalarType dtype=ScalarType::Float32, std::uint64_t seed=0) { return Series::randn(sizes,dtype,seed); }

} // namespace pluss::codem
