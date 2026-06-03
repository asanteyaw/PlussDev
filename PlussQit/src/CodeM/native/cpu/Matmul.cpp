#include "CodeM/core/Errors.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/native/cpu/BlasGemm.h"

#include <functional>
#include <numeric>
#include <vector>

namespace pluss::codem::native::cpu {
namespace {

static void naive_matmul_f64(const double* A, const double* B, double* C,
                             std::int64_t M, std::int64_t N, std::int64_t K) {
  for (std::int64_t i=0;i<M;++i) {
    for (std::int64_t j=0;j<N;++j) {
      double acc = 0.0;
      for (std::int64_t k=0;k<K;++k) acc += A[i*K + k] * B[k*N + j];
      C[i*N + j] = acc;
    }
  }
}

static void naive_matmul_f32(const float* A, const float* B, float* C,
                             std::int64_t M, std::int64_t N, std::int64_t K) {
  for (std::int64_t i=0;i<M;++i) {
    for (std::int64_t j=0;j<N;++j) {
      float acc = 0.0f;
      for (std::int64_t k=0;k<K;++k) acc += A[i*K + k] * B[k*N + j];
      C[i*N + j] = acc;
    }
  }
}

static void naive_matmul_i32(const std::int32_t* A, const std::int32_t* B, std::int32_t* C,
                             std::int64_t M, std::int64_t N, std::int64_t K) {
  for (std::int64_t i = 0; i < M; ++i) {
    for (std::int64_t j = 0; j < N; ++j) {
      std::int64_t acc = 0;
      for (std::int64_t k = 0; k < K; ++k) {
        acc += static_cast<std::int64_t>(A[i * K + k]) * static_cast<std::int64_t>(B[k * N + j]);
      }
      C[i * N + j] = static_cast<std::int32_t>(acc);
    }
  }
}

template <class T>
static void naive_matmul_complex(const T* A, const T* B, T* C,
                                 std::int64_t M, std::int64_t N, std::int64_t K) {
  for (std::int64_t i = 0; i < M; ++i) {
    for (std::int64_t j = 0; j < N; ++j) {
      T acc{};
      for (std::int64_t k = 0; k < K; ++k) {
        acc += A[i * K + k] * B[k * N + j];
      }
      C[i * N + j] = acc;
    }
  }
}

std::int64_t product_(std::span<const std::int64_t> sizes) {
  return std::accumulate(sizes.begin(), sizes.end(), std::int64_t{1}, std::multiplies<std::int64_t>{});
}

Series matmul_2d_core_(const Series& a, const Series& b) {
  PLUSS_CHECK(a.dim() == 2 && b.dim() == 2, "matmul: internal 2D path expected 2D tensors");
  if (a.scalar_type() != b.scalar_type()) throw_error("matmul: dtype mismatch");

  if (a.scalar_type() == ScalarType::Float16 || a.scalar_type() == ScalarType::BFloat16) {
    return matmul_2d_core_(a.to(ScalarType::Float32), b.to(ScalarType::Float32)).to(a.scalar_type());
  }

  if (!a.is_contiguous() || !b.is_contiguous()) {
    return matmul_2d_core_(a.contiguous(), b.contiguous());
  }

  const auto M = a.sizes()[0];
  const auto K = a.sizes()[1];
  const auto K2 = b.sizes()[0];
  const auto N = b.sizes()[1];
  if (K != K2) throw_error("matmul: shape mismatch");

  Series out = Series::empty({M, N}, a.scalar_type());

  if (a.scalar_type() == ScalarType::Float64) {
    const auto* A = reinterpret_cast<const double*>(a.raw_data());
    const auto* B = reinterpret_cast<const double*>(b.raw_data());
    auto* C = reinterpret_cast<double*>(out.raw_data());
    if (!blas::gemm_f64(A, B, C, M, N, K)) {
      naive_matmul_f64(A, B, C, M, N, K);
    }
    return out;
  }

  if (a.scalar_type() == ScalarType::Float32) {
    const auto* A = reinterpret_cast<const float*>(a.raw_data());
    const auto* B = reinterpret_cast<const float*>(b.raw_data());
    auto* C = reinterpret_cast<float*>(out.raw_data());
    if (!blas::gemm_f32(A, B, C, M, N, K)) {
      naive_matmul_f32(A, B, C, M, N, K);
    }
    return out;
  }

  if (a.scalar_type() == ScalarType::Int32) {
    const auto* A = reinterpret_cast<const std::int32_t*>(a.raw_data());
    const auto* B = reinterpret_cast<const std::int32_t*>(b.raw_data());
    auto* C = reinterpret_cast<std::int32_t*>(out.raw_data());
    naive_matmul_i32(A, B, C, M, N, K);
    return out;
  }

  if (a.scalar_type() == ScalarType::Complex64) {
    const auto* A = reinterpret_cast<const Complex64*>(a.raw_data());
    const auto* B = reinterpret_cast<const Complex64*>(b.raw_data());
    auto* C = reinterpret_cast<Complex64*>(out.raw_data());
    naive_matmul_complex(A, B, C, M, N, K);
    return out;
  }

  if (a.scalar_type() == ScalarType::Complex128) {
    const auto* A = reinterpret_cast<const Complex128*>(a.raw_data());
    const auto* B = reinterpret_cast<const Complex128*>(b.raw_data());
    auto* C = reinterpret_cast<Complex128*>(out.raw_data());
    naive_matmul_complex(A, B, C, M, N, K);
    return out;
  }

  throw_error("matmul: dtype not implemented (only float16/bfloat16/float32/float64/int32/complex64/complex128)");
}

std::vector<std::int64_t> prefix_batch_shape_(const Series& x) {
  if (x.dim() <= 2) {
    return {};
  }
  const auto& sz = x.sizes();
  return std::vector<std::int64_t>(sz.begin(), sz.end() - 2);
}

std::vector<std::int64_t> decode_batch_index_(
    std::int64_t linear_idx,
    std::span<const std::int64_t> batch_shape) {
  std::vector<std::int64_t> out(batch_shape.size(), 0);
  for (std::int64_t i = static_cast<std::int64_t>(batch_shape.size()) - 1; i >= 0; --i) {
    const std::int64_t dim = batch_shape[static_cast<std::size_t>(i)];
    if (dim == 0) {
      out[static_cast<std::size_t>(i)] = 0;
    } else {
      out[static_cast<std::size_t>(i)] = linear_idx % dim;
      linear_idx /= dim;
    }
  }
  return out;
}

std::vector<std::int64_t> project_batch_index_(
    std::span<const std::int64_t> out_index,
    std::span<const std::int64_t> out_batch_shape,
    std::span<const std::int64_t> src_batch_shape) {
  if (src_batch_shape.empty()) {
    return {};
  }
  const std::size_t out_rank = out_batch_shape.size();
  const std::size_t src_rank = src_batch_shape.size();
  std::vector<std::int64_t> src_index(src_rank, 0);
  for (std::size_t i = 0; i < src_rank; ++i) {
    const std::size_t out_pos = out_rank - src_rank + i;
    const std::int64_t coord = out_index[out_pos];
    src_index[i] = (src_batch_shape[i] == 1) ? 0 : coord;
  }
  return src_index;
}

Series select_batch_matrix_(const Series& x, std::span<const std::int64_t> batch_index) {
  Series out = x;
  for (const std::int64_t idx : batch_index) {
    out = out.select(0, idx);
  }
  return out;
}

Series batched_matmul_(const Series& a_raw, const Series& b_raw) {
  const Series a = (a_raw.dim() == 1) ? a_raw.unsqueeze(0) : a_raw;
  const Series b = (b_raw.dim() == 1) ? b_raw.unsqueeze(1) : b_raw;
  PLUSS_CHECK(a.dim() >= 2 && b.dim() >= 2, "matmul: internal batched path requires rank >= 2");

  const std::int64_t n = a.sizes()[a.dim() - 2];
  const std::int64_t k_left = a.sizes().back();
  const std::int64_t k_right = b.sizes()[b.dim() - 2];
  const std::int64_t p = b.sizes().back();
  if (k_left != k_right) {
    throw_error("matmul: shape mismatch");
  }

  const auto batch_a = prefix_batch_shape_(a);
  const auto batch_b = prefix_batch_shape_(b);
  const auto batch_shape = SeriesIterator::broadcast_shape(batch_a, batch_b);

  const std::int64_t batch_product = product_(batch_shape);

  std::vector<std::int64_t> matrix_output_shape = batch_shape;
  matrix_output_shape.push_back(n);
  matrix_output_shape.push_back(p);
  if (batch_product == 0) {
    return Series::empty(matrix_output_shape, a.scalar_type());
  }

  std::vector<Series> pages;
  pages.reserve(static_cast<std::size_t>(batch_product));
  for (std::int64_t i = 0; i < batch_product; ++i) {
    const auto out_index = decode_batch_index_(i, batch_shape);
    const auto a_batch_index = project_batch_index_(out_index, batch_shape, batch_a);
    const auto b_batch_index = project_batch_index_(out_index, batch_shape, batch_b);
    pages.push_back(
        matmul_2d_core_(
            select_batch_matrix_(a, a_batch_index),
            select_batch_matrix_(b, b_batch_index)));
  }
  std::vector<Series> tails;
  tails.reserve(pages.size() > 0 ? pages.size() - 1 : 0);
  for (std::size_t i = 1; i < pages.size(); ++i) {
    tails.push_back(pages[i]);
  }
  return pages.front().pile(tails, 0).reshape(matrix_output_shape);
}

} // namespace

Series matmul(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("matmul: undefined");
  if (a.dim() == 0 || b.dim() == 0) throw_error("matmul: both arguments must be at least 1D");
  if (a.scalar_type() != b.scalar_type()) throw_error("matmul: dtype mismatch");

  if (a.dim() == 2 && b.dim() == 2) {
    return matmul_2d_core_(a, b);
  }
  if (a.dim() == 1 && b.dim() == 1) {
    const std::int64_t k = a.sizes()[0];
    if (k != b.sizes()[0]) {
      throw_error("matmul: shape mismatch");
    }
    return matmul_2d_core_(a.reshape({1, k}), b.reshape({k, 1})).reshape({1});
  }

  Series out = batched_matmul_(a, b);
  if (a.dim() == 1) {
    out = out.squeeze(-2);
  }
  if (b.dim() == 1) {
    out = out.squeeze(-1);
  }
  return out;
}

} // namespace pluss::codem::native::cpu
