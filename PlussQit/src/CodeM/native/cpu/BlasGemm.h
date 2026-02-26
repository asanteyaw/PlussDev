#pragma once

#include <cstdint>

namespace pluss::codem::native::cpu::blas {

enum class Provider : std::uint8_t {
  None = 0,
  Accelerate = 1,
  OpenBLAS = 2,
  OneMKL = 3,
  CBLAS = 4,
};

Provider provider() noexcept;
const char* provider_name() noexcept;
bool available() noexcept;

bool gemm_f32(const float* a, const float* b, float* c,
              std::int64_t m, std::int64_t n, std::int64_t k) noexcept;

bool gemm_f64(const double* a, const double* b, double* c,
              std::int64_t m, std::int64_t n, std::int64_t k) noexcept;

} // namespace pluss::codem::native::cpu::blas

