#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"

#if defined(__APPLE__)
  #include <Accelerate/Accelerate.h>
  #define PLUSS_HAS_ACCELERATE 1
#else
  #define PLUSS_HAS_ACCELERATE 0
#endif

namespace pluss::codem::native::cpu {

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

Series matmul(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("matmul: undefined");
  if (a.dim() != 2 || b.dim() != 2) throw_error("matmul: only 2D supported in v0");
  if (a.scalar_type() != b.scalar_type()) throw_error("matmul: dtype mismatch (v0)");
  if (!a.is_contiguous() || !b.is_contiguous()) {
    return matmul(a.contiguous(), b.contiguous());
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
#if PLUSS_HAS_ACCELERATE
    // cblas uses column-major by default; use row-major interface
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                (int)M, (int)N, (int)K,
                1.0, A, (int)K, B, (int)N, 0.0, C, (int)N);
#else
    naive_matmul_f64(A, B, C, M, N, K);
#endif
    return out;
  }

  if (a.scalar_type() == ScalarType::Float32) {
    const auto* A = reinterpret_cast<const float*>(a.raw_data());
    const auto* B = reinterpret_cast<const float*>(b.raw_data());
    auto* C = reinterpret_cast<float*>(out.raw_data());
#if PLUSS_HAS_ACCELERATE
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                (int)M, (int)N, (int)K,
                1.0f, A, (int)K, B, (int)N, 0.0f, C, (int)N);
#else
    naive_matmul_f32(A, B, C, M, N, K);
#endif
    return out;
  }

  throw_error("matmul: dtype not implemented (only float32/float64)");
}

} // namespace pluss::codem::native::cpu
