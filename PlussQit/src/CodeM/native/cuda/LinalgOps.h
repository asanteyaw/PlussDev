#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_linalg_enabled() noexcept;
bool native_linalg_strict() noexcept;

bool supports_det_baseline(ScalarType dtype) noexcept;
bool supports_inverse_baseline(ScalarType dtype) noexcept;
bool supports_cholesky_baseline(ScalarType dtype) noexcept;
bool supports_logdet_baseline(ScalarType dtype) noexcept;
bool supports_slogdet_baseline(ScalarType dtype) noexcept;
bool supports_pinverse_baseline(ScalarType dtype) noexcept;
bool supports_triangular_solve_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept;
bool supports_cholesky_solve_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept;
bool supports_cholesky_inverse_baseline(ScalarType factor_dtype) noexcept;
bool supports_lu_solve_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept;
bool supports_qr_baseline(ScalarType dtype, bool reduced) noexcept;
bool supports_svd_baseline(ScalarType dtype, bool full_matrices) noexcept;
bool supports_matrix_power_baseline(ScalarType dtype) noexcept;
bool supports_matrix_exp_baseline(ScalarType dtype) noexcept;

bool supports_det(const Series& a) noexcept;
bool supports_inverse(const Series& a) noexcept;
bool supports_cholesky(const Series& a) noexcept;
bool supports_logdet(const Series& a) noexcept;
bool supports_slogdet(const Series& a) noexcept;
bool supports_pinverse(const Series& a) noexcept;
bool supports_triangular_solve(const Series& a, const Series& b) noexcept;
bool supports_cholesky_solve(const Series& factor, const Series& b) noexcept;
bool supports_cholesky_inverse(const Series& factor) noexcept;
bool supports_lu_solve(const Series& lu, const Series& b) noexcept;
bool supports_qr(const Series& a, bool reduced) noexcept;
bool supports_svd(const Series& a, bool full_matrices) noexcept;
bool supports_matrix_power(const Series& a) noexcept;
bool supports_matrix_exp(const Series& a) noexcept;

} // namespace pluss::codem::native::cuda
