#include "CodeM/native/cpu/BlasGemm.h"

#include <limits>

#ifndef PLUSSQIT_CPU_BLAS_PROVIDER
#define PLUSSQIT_CPU_BLAS_PROVIDER 0
#endif

#define PLUSSQIT_CPU_BLAS_PROVIDER_NONE 0
#define PLUSSQIT_CPU_BLAS_PROVIDER_ACCELERATE 1
#define PLUSSQIT_CPU_BLAS_PROVIDER_OPENBLAS 2
#define PLUSSQIT_CPU_BLAS_PROVIDER_ONEMKL 3
#define PLUSSQIT_CPU_BLAS_PROVIDER_CBLAS 4

#if (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_ACCELERATE)
  #include <Accelerate/Accelerate.h>
  #define PLUSSQIT_HAS_CBLAS_BACKEND 1
#elif (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_ONEMKL)
  #if __has_include(<mkl.h>)
    #include <mkl.h>
    #define PLUSSQIT_HAS_CBLAS_BACKEND 1
  #elif __has_include(<mkl_cblas.h>)
    #include <mkl_cblas.h>
    #define PLUSSQIT_HAS_CBLAS_BACKEND 1
  #elif __has_include(<cblas.h>)
    #include <cblas.h>
    #define PLUSSQIT_HAS_CBLAS_BACKEND 1
  #else
    #define PLUSSQIT_HAS_CBLAS_BACKEND 0
  #endif
#elif (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_OPENBLAS) || \
      (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_CBLAS)
  #if __has_include(<cblas.h>)
    #include <cblas.h>
    #define PLUSSQIT_HAS_CBLAS_BACKEND 1
  #else
    #define PLUSSQIT_HAS_CBLAS_BACKEND 0
  #endif
#else
  #define PLUSSQIT_HAS_CBLAS_BACKEND 0
#endif

namespace pluss::codem::native::cpu::blas {
namespace {

constexpr bool in_int_range_(std::int64_t v) noexcept {
  return (v >= 0) && (v <= static_cast<std::int64_t>(std::numeric_limits<int>::max()));
}

} // namespace

Provider provider() noexcept {
#if PLUSSQIT_HAS_CBLAS_BACKEND
  #if (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_ACCELERATE)
  return Provider::Accelerate;
  #elif (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_OPENBLAS)
  return Provider::OpenBLAS;
  #elif (PLUSSQIT_CPU_BLAS_PROVIDER == PLUSSQIT_CPU_BLAS_PROVIDER_ONEMKL)
  return Provider::OneMKL;
  #else
  return Provider::CBLAS;
  #endif
#else
  return Provider::None;
#endif
}

const char* provider_name() noexcept {
  switch (provider()) {
    case Provider::Accelerate: return "Accelerate";
    case Provider::OpenBLAS: return "OpenBLAS";
    case Provider::OneMKL: return "oneMKL";
    case Provider::CBLAS: return "CBLAS";
    case Provider::None:
    default: return "None";
  }
}

bool available() noexcept {
  return provider() != Provider::None;
}

bool gemm_f32(const float* a, const float* b, float* c,
              std::int64_t m, std::int64_t n, std::int64_t k) noexcept {
#if PLUSSQIT_HAS_CBLAS_BACKEND
  if (!in_int_range_(m) || !in_int_range_(n) || !in_int_range_(k)) return false;
  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
              static_cast<int>(m), static_cast<int>(n), static_cast<int>(k),
              1.0f, a, static_cast<int>(k), b, static_cast<int>(n),
              0.0f, c, static_cast<int>(n));
  return true;
#else
  (void)a;
  (void)b;
  (void)c;
  (void)m;
  (void)n;
  (void)k;
  return false;
#endif
}

bool gemm_f64(const double* a, const double* b, double* c,
              std::int64_t m, std::int64_t n, std::int64_t k) noexcept {
#if PLUSSQIT_HAS_CBLAS_BACKEND
  if (!in_int_range_(m) || !in_int_range_(n) || !in_int_range_(k)) return false;
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
              static_cast<int>(m), static_cast<int>(n), static_cast<int>(k),
              1.0, a, static_cast<int>(k), b, static_cast<int>(n),
              0.0, c, static_cast<int>(n));
  return true;
#else
  (void)a;
  (void)b;
  (void)c;
  (void)m;
  (void)n;
  (void)k;
  return false;
#endif
}

} // namespace pluss::codem::native::cpu::blas

