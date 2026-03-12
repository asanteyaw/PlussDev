#include "CodeM/native/cuda/LinalgOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstdlib>

namespace pluss::codem::native::cuda {
namespace {

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* name, bool default_value) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return default_value;
  }
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) {
    return true;
  }
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) {
    return false;
  }
  return default_value;
}

bool supports_linalg_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
      return true;
    default:
      return false;
  }
}

bool linalg_baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_linalg_enabled();
}

bool is_cuda_matrix_like_(const Series& a) noexcept {
  return a.platform() == Platform::CUDA && a.dim() >= 2;
}

std::int64_t tail_rows_(const Series& a) noexcept {
  return a.sizes()[static_cast<std::size_t>(a.dim() - 2)];
}

std::int64_t tail_cols_(const Series& a) noexcept {
  return a.sizes()[static_cast<std::size_t>(a.dim() - 1)];
}

bool is_square_cuda_matrix_like_(const Series& a) noexcept {
  return is_cuda_matrix_like_(a) && tail_rows_(a) == tail_cols_(a);
}

bool is_cuda_rhs_for_square_(const Series& b, std::int64_t n) noexcept {
  if (b.platform() != Platform::CUDA || b.dim() < 1) {
    return false;
  }
  if (b.dim() == 1) {
    return b.sizes()[0] == n;
  }
  return b.sizes()[static_cast<std::size_t>(b.dim() - 2)] == n;
}

} // namespace

bool native_linalg_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_LINALG", false);
}

bool native_linalg_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_LINALG_STRICT", false);
}

bool supports_det_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_inverse_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_cholesky_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_logdet_baseline(ScalarType dtype) noexcept {
  return supports_det_baseline(dtype);
}

bool supports_slogdet_baseline(ScalarType dtype) noexcept {
  return supports_det_baseline(dtype);
}

bool supports_pinverse_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_triangular_solve_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept {
  return linalg_baseline_ready_() &&
      supports_linalg_dtype_(a_dtype) &&
      supports_linalg_dtype_(b_dtype) &&
      a_dtype == b_dtype;
}

bool supports_cholesky_solve_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept {
  return supports_triangular_solve_baseline(factor_dtype, b_dtype);
}

bool supports_cholesky_inverse_baseline(ScalarType factor_dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(factor_dtype);
}

bool supports_lu_solve_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept {
  return supports_triangular_solve_baseline(lu_dtype, b_dtype);
}

bool supports_qr_baseline(ScalarType dtype, bool reduced) noexcept {
  return reduced && linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_svd_baseline(ScalarType dtype, bool full_matrices) noexcept {
  return !full_matrices && linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_matrix_power_baseline(ScalarType dtype) noexcept {
  return supports_matmul_lane(dtype, dtype);
}

bool supports_matrix_exp_baseline(ScalarType dtype) noexcept {
  return supports_matmul_lane(dtype, dtype);
}

bool supports_det(const Series& a) noexcept {
  return supports_det_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_inverse(const Series& a) noexcept {
  return supports_inverse_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_cholesky(const Series& a) noexcept {
  return supports_cholesky_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_logdet(const Series& a) noexcept {
  return supports_logdet_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_slogdet(const Series& a) noexcept {
  return supports_slogdet_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_pinverse(const Series& a) noexcept {
  return supports_pinverse_baseline(a.scalar_type()) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_triangular_solve(const Series& a, const Series& b) noexcept {
  return supports_triangular_solve_baseline(a.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(a) &&
      is_cuda_rhs_for_square_(b, tail_rows_(a));
}

bool supports_cholesky_solve(const Series& factor, const Series& b) noexcept {
  return supports_cholesky_solve_baseline(factor.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(factor) &&
      is_cuda_rhs_for_square_(b, tail_rows_(factor));
}

bool supports_cholesky_inverse(const Series& factor) noexcept {
  return supports_cholesky_inverse_baseline(factor.scalar_type()) && is_square_cuda_matrix_like_(factor);
}

bool supports_lu_solve(const Series& lu, const Series& b) noexcept {
  return supports_lu_solve_baseline(lu.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(lu) &&
      is_cuda_rhs_for_square_(b, tail_rows_(lu));
}

bool supports_qr(const Series& a, bool reduced) noexcept {
  return supports_qr_baseline(a.scalar_type(), reduced) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_svd(const Series& a, bool full_matrices) noexcept {
  return supports_svd_baseline(a.scalar_type(), full_matrices) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_matrix_power(const Series& a) noexcept {
  return supports_matrix_power_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_matrix_exp(const Series& a) noexcept {
  return supports_matrix_exp_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

} // namespace pluss::codem::native::cuda
