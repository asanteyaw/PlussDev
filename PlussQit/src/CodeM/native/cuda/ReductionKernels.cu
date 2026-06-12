#include "CodeM/native/cuda/ReductionOps.h"

#include "CodeM/core/Storage.h"

#include <cuda_runtime_api.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace pluss::codem::native::cuda {
namespace {

constexpr int kBlockSize = 256;

enum class ReduceOp : int {
  Sum,
  Prod,
  Mean,
  AMax,
  AMin,
};

template <typename T>
__device__ T init_value_(ReduceOp op) {
  switch (op) {
    case ReduceOp::Prod:
      return T(1);
    case ReduceOp::AMax:
      return -static_cast<T>(INFINITY);
    case ReduceOp::AMin:
      return static_cast<T>(INFINITY);
    case ReduceOp::Sum:
    case ReduceOp::Mean:
    default:
      return T(0);
  }
}

template <typename T>
__device__ T combine_(ReduceOp op, T acc, T x) {
  switch (op) {
    case ReduceOp::Prod:
      return acc * x;
    case ReduceOp::AMax:
      return x > acc ? x : acc;
    case ReduceOp::AMin:
      return x < acc ? x : acc;
    case ReduceOp::Sum:
    case ReduceOp::Mean:
    default:
      return acc + x;
  }
}

template <typename T>
__global__ void reduce_dim_kernel_(
    ReduceOp op,
    const T* in,
    T* out,
    std::int64_t outer,
    std::int64_t reduce,
    std::int64_t inner) {
  const std::int64_t out_idx = static_cast<std::int64_t>(blockIdx.x);
  if (out_idx >= outer * inner) return;
  const std::int64_t outer_idx = out_idx / inner;
  const std::int64_t inner_idx = out_idx - outer_idx * inner;
  const std::int64_t base = outer_idx * reduce * inner + inner_idx;

  __shared__ T scratch[kBlockSize];
  T acc = init_value_<T>(op);
  for (std::int64_t r = threadIdx.x; r < reduce; r += blockDim.x) {
    acc = combine_(op, acc, in[base + r * inner]);
  }
  scratch[threadIdx.x] = acc;
  __syncthreads();

  for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
    if (threadIdx.x < stride) {
      scratch[threadIdx.x] = combine_(op, scratch[threadIdx.x], scratch[threadIdx.x + stride]);
    }
    __syncthreads();
  }
  if (threadIdx.x == 0) {
    T v = scratch[0];
    if (op == ReduceOp::Mean) {
      v = v / static_cast<T>(reduce);
    }
    out[out_idx] = v;
  }
}

__global__ void logical_dim_kernel_(
    bool all_op,
    const std::uint8_t* in,
    std::uint8_t* out,
    std::int64_t outer,
    std::int64_t reduce,
    std::int64_t inner) {
  const std::int64_t out_idx = static_cast<std::int64_t>(blockIdx.x);
  if (out_idx >= outer * inner) return;
  const std::int64_t outer_idx = out_idx / inner;
  const std::int64_t inner_idx = out_idx - outer_idx * inner;
  const std::int64_t base = outer_idx * reduce * inner + inner_idx;

  __shared__ std::uint8_t scratch[kBlockSize];
  std::uint8_t acc = all_op ? std::uint8_t{1} : std::uint8_t{0};
  for (std::int64_t r = threadIdx.x; r < reduce; r += blockDim.x) {
    const std::uint8_t v = in[base + r * inner] ? std::uint8_t{1} : std::uint8_t{0};
    acc = all_op ? static_cast<std::uint8_t>(acc & v) : static_cast<std::uint8_t>(acc | v);
  }
  scratch[threadIdx.x] = acc;
  __syncthreads();

  for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
    if (threadIdx.x < stride) {
      scratch[threadIdx.x] = all_op
          ? static_cast<std::uint8_t>(scratch[threadIdx.x] & scratch[threadIdx.x + stride])
          : static_cast<std::uint8_t>(scratch[threadIdx.x] | scratch[threadIdx.x + stride]);
    }
    __syncthreads();
  }
  if (threadIdx.x == 0) {
    out[out_idx] = scratch[0] ? std::uint8_t{1} : std::uint8_t{0};
  }
}

template <typename T>
__global__ void arg_dim_kernel_(
    bool max_op,
    const T* in,
    std::int64_t* out,
    std::int64_t outer,
    std::int64_t reduce,
    std::int64_t inner) {
  const std::int64_t out_idx = static_cast<std::int64_t>(blockIdx.x);
  if (out_idx >= outer * inner) return;
  const std::int64_t outer_idx = out_idx / inner;
  const std::int64_t inner_idx = out_idx - outer_idx * inner;
  const std::int64_t base = outer_idx * reduce * inner + inner_idx;

  __shared__ T vals[kBlockSize];
  __shared__ std::int64_t idxs[kBlockSize];
  T best = in[base];
  std::int64_t best_idx = 0;
  for (std::int64_t r = threadIdx.x; r < reduce; r += blockDim.x) {
    const T v = in[base + r * inner];
    const bool better = max_op ? (v > best) : (v < best);
    if (better || (v == best && r < best_idx)) {
      best = v;
      best_idx = r;
    }
  }
  vals[threadIdx.x] = best;
  idxs[threadIdx.x] = best_idx;
  __syncthreads();

  for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
    if (threadIdx.x < stride) {
      const T rhs_v = vals[threadIdx.x + stride];
      const std::int64_t rhs_i = idxs[threadIdx.x + stride];
      const bool better = max_op ? (rhs_v > vals[threadIdx.x]) : (rhs_v < vals[threadIdx.x]);
      if (better || (rhs_v == vals[threadIdx.x] && rhs_i < idxs[threadIdx.x])) {
        vals[threadIdx.x] = rhs_v;
        idxs[threadIdx.x] = rhs_i;
      }
    }
    __syncthreads();
  }
  if (threadIdx.x == 0) {
    out[out_idx] = idxs[0];
  }
}

bool finish_kernel_() noexcept {
  return cudaPeekAtLastError() == cudaSuccess;
}

bool valid_in_(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      a.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice;
}

bool valid_out_(const Series& out) noexcept {
  return out.platform() == Platform::CUDA &&
      out.is_contiguous() &&
      out.storage().location() == StorageLocation::CudaDevice;
}

bool dims_(const Series& a, std::int64_t dim, std::int64_t& outer, std::int64_t& reduce, std::int64_t& inner) noexcept {
  const std::int64_t rank = static_cast<std::int64_t>(a.dim());
  if (rank <= 0) return false;
  if (dim < 0) dim += rank;
  if (dim < 0 || dim >= rank) return false;
  outer = 1;
  for (std::int64_t i = 0; i < dim; ++i) outer *= a.sizes()[static_cast<std::size_t>(i)];
  reduce = a.sizes()[static_cast<std::size_t>(dim)];
  inner = 1;
  for (std::int64_t i = dim + 1; i < rank; ++i) inner *= a.sizes()[static_cast<std::size_t>(i)];
  return reduce > 0;
}

bool copy_scalar_(const Series& a, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || a.scalar_type() != out.scalar_type() || a.numel() != 1 || out.numel() != 1) {
    return false;
  }
  return cudaMemcpy(out.raw_mutable_data(), a.raw_data(), static_cast<std::size_t>(a.dtype().itemsize), cudaMemcpyDeviceToDevice) ==
      cudaSuccess;
}

bool run_float_reduce_(ReduceOp op, const Series& a, std::int64_t dim, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || out.scalar_type() != a.scalar_type()) return false;
  std::int64_t outer = 0;
  std::int64_t reduce = 0;
  std::int64_t inner = 0;
  if (!dims_(a, dim, outer, reduce, inner) || out.numel() != outer * inner) return false;
  if (out.numel() == 0) return true;
  const int grid = static_cast<int>(out.numel());
  switch (a.scalar_type()) {
    case ScalarType::Float32:
      reduce_dim_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const float*>(a.raw_data()),
          reinterpret_cast<float*>(out.raw_mutable_data()),
          outer,
          reduce,
          inner);
      return finish_kernel_();
    case ScalarType::Float64:
      reduce_dim_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const double*>(a.raw_data()),
          reinterpret_cast<double*>(out.raw_mutable_data()),
          outer,
          reduce,
          inner);
      return finish_kernel_();
    default:
      return false;
  }
}

bool run_float_reduce_all_(ReduceOp op, const Series& a, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || out.scalar_type() != a.scalar_type() || out.numel() != 1) return false;
  if (a.numel() == 0) return false;
  if (a.dim() == 0) return copy_scalar_(a, out);
  Series cur = a;
  Series tmp;
  for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
    const bool final = d == 0;
    if (final) {
      if (!run_float_reduce_(op, cur, d, out)) return false;
    } else {
      auto sizes = std::vector<std::int64_t>(cur.sizes().begin(), cur.sizes().end());
      sizes.erase(sizes.begin() + d);
      tmp = Series::empty(sizes, cur.scalar_type(), Platform::CUDA);
      if (!run_float_reduce_(op, cur, d, tmp)) return false;
      cur = tmp;
    }
  }
  return true;
}

bool run_logical_reduce_(bool all_op, const Series& a, std::int64_t dim, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool) {
    return false;
  }
  std::int64_t outer = 0;
  std::int64_t reduce = 0;
  std::int64_t inner = 0;
  if (!dims_(a, dim, outer, reduce, inner) || out.numel() != outer * inner) return false;
  if (out.numel() == 0) return true;
  const int grid = static_cast<int>(out.numel());
  logical_dim_kernel_<<<grid, kBlockSize>>>(
      all_op,
      reinterpret_cast<const std::uint8_t*>(a.raw_data()),
      reinterpret_cast<std::uint8_t*>(out.raw_mutable_data()),
      outer,
      reduce,
      inner);
  return finish_kernel_();
}

bool run_logical_reduce_all_(bool all_op, const Series& a, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool ||
      out.numel() != 1 || a.numel() == 0) {
    return false;
  }
  if (a.dim() == 0) return copy_scalar_(a, out);
  Series cur = a;
  Series tmp;
  for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
    const bool final = d == 0;
    if (final) {
      if (!run_logical_reduce_(all_op, cur, d, out)) return false;
    } else {
      auto sizes = std::vector<std::int64_t>(cur.sizes().begin(), cur.sizes().end());
      sizes.erase(sizes.begin() + d);
      tmp = Series::empty(sizes, ScalarType::Bool, Platform::CUDA);
      if (!run_logical_reduce_(all_op, cur, d, tmp)) return false;
      cur = tmp;
    }
  }
  return true;
}

bool run_arg_reduce_(bool max_op, const Series& a, std::int64_t dim, Series& out) noexcept {
  if (!valid_in_(a) || !valid_out_(out) || out.scalar_type() != ScalarType::Int64) return false;
  std::int64_t outer = 0;
  std::int64_t reduce = 0;
  std::int64_t inner = 0;
  if (!dims_(a, dim, outer, reduce, inner) || out.numel() != outer * inner) return false;
  if (out.numel() == 0) return true;
  const int grid = static_cast<int>(out.numel());
  switch (a.scalar_type()) {
    case ScalarType::Float32:
      arg_dim_kernel_<<<grid, kBlockSize>>>(
          max_op,
          reinterpret_cast<const float*>(a.raw_data()),
          reinterpret_cast<std::int64_t*>(out.raw_mutable_data()),
          outer,
          reduce,
          inner);
      return finish_kernel_();
    case ScalarType::Float64:
      arg_dim_kernel_<<<grid, kBlockSize>>>(
          max_op,
          reinterpret_cast<const double*>(a.raw_data()),
          reinterpret_cast<std::int64_t*>(out.raw_mutable_data()),
          outer,
          reduce,
          inner);
      return finish_kernel_();
    default:
      return false;
  }
}

} // namespace

bool run_sum_reduction(const Series& a, Series& out) noexcept { return run_float_reduce_all_(ReduceOp::Sum, a, out); }
bool run_sum_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_float_reduce_(ReduceOp::Sum, a, dim, out); }
bool run_prod_reduction(const Series& a, Series& out) noexcept { return run_float_reduce_all_(ReduceOp::Prod, a, out); }
bool run_prod_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_float_reduce_(ReduceOp::Prod, a, dim, out); }
bool run_mean_reduction(const Series& a, Series& out) noexcept { return run_float_reduce_all_(ReduceOp::Mean, a, out); }
bool run_mean_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_float_reduce_(ReduceOp::Mean, a, dim, out); }
bool run_amax_reduction(const Series& a, Series& out) noexcept { return run_float_reduce_all_(ReduceOp::AMax, a, out); }
bool run_amax_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_float_reduce_(ReduceOp::AMax, a, dim, out); }
bool run_amin_reduction(const Series& a, Series& out) noexcept { return run_float_reduce_all_(ReduceOp::AMin, a, out); }
bool run_amin_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_float_reduce_(ReduceOp::AMin, a, dim, out); }
bool run_all_reduction(const Series& a, Series& out) noexcept { return run_logical_reduce_all_(true, a, out); }
bool run_all_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_logical_reduce_(true, a, dim, out); }
bool run_any_reduction(const Series& a, Series& out) noexcept { return run_logical_reduce_all_(false, a, out); }
bool run_any_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_logical_reduce_(false, a, dim, out); }
bool run_argmax_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_arg_reduce_(true, a, dim, out); }
bool run_argmin_reduction(const Series& a, std::int64_t dim, Series& out) noexcept { return run_arg_reduce_(false, a, dim, out); }

} // namespace pluss::codem::native::cuda
