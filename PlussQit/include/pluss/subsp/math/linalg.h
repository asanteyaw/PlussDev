#pragma once

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#endif

namespace pluss::subsp::math::linalg {

struct QRResult {
  Series Q;
  Series R;
};

struct SVDResult {
  Series U;
  Series S;
  Series Vh;
};

namespace detail {

inline Config cpu_f64_cfg_() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

inline void require_2d_(const Series& a, const char* op) {
  if (!a.defined() || a.dim() != 2) {
    throw std::runtime_error(std::string(op) + ": expected a defined 2D Series");
  }
}

inline void require_square_(const Series& a, const char* op) {
  require_2d_(a, op);
  if (a.sizes()[0] != a.sizes()[1]) {
    throw std::runtime_error(std::string(op) + ": expected square matrix");
  }
}

inline std::vector<double> to_row_major_f64_(const Series& a) {
  auto cpu = a.to(pluss::pCPU, ScalarType::Float64).contiguous();
  return cpu.to_vec<double>();
}

inline Series matrix_from_row_major_(const std::vector<double>& row, std::int64_t m, std::int64_t n) {
  return pluss::make_series(row, {m, n}, cpu_f64_cfg_());
}

inline Series vector_from_values_(const std::vector<double>& vals) {
  return pluss::make_series(vals, {static_cast<std::int64_t>(vals.size())}, cpu_f64_cfg_());
}

inline Series scalar_from_value_(double v) {
  return pluss::make_series(v, cpu_f64_cfg_());
}

inline std::vector<double> row_to_col_major_(const std::vector<double>& row, std::int64_t m, std::int64_t n) {
  std::vector<double> col(static_cast<std::size_t>(m * n));
  for (std::int64_t r = 0; r < m; ++r) {
    for (std::int64_t c = 0; c < n; ++c) {
      col[static_cast<std::size_t>(c * m + r)] = row[static_cast<std::size_t>(r * n + c)];
    }
  }
  return col;
}

inline std::vector<double> col_to_row_major_(const std::vector<double>& col, std::int64_t m, std::int64_t n) {
  std::vector<double> row(static_cast<std::size_t>(m * n));
  for (std::int64_t r = 0; r < m; ++r) {
    for (std::int64_t c = 0; c < n; ++c) {
      row[static_cast<std::size_t>(r * n + c)] = col[static_cast<std::size_t>(c * m + r)];
    }
  }
  return row;
}

inline std::vector<double> identity_(std::int64_t n) {
  std::vector<double> out(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) out[static_cast<std::size_t>(i * n + i)] = 1.0;
  return out;
}

inline std::vector<double> transpose_(const std::vector<double>& a, std::int64_t m, std::int64_t n) {
  std::vector<double> out(static_cast<std::size_t>(m * n), 0.0);
  for (std::int64_t r = 0; r < m; ++r) {
    for (std::int64_t c = 0; c < n; ++c) {
      out[static_cast<std::size_t>(c * m + r)] = a[static_cast<std::size_t>(r * n + c)];
    }
  }
  return out;
}

inline std::vector<double> matmul_(const std::vector<double>& a, std::int64_t m, std::int64_t k,
                                   const std::vector<double>& b, std::int64_t n) {
  std::vector<double> out(static_cast<std::size_t>(m * n), 0.0);
  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t p = 0; p < k; ++p) {
      const double av = a[static_cast<std::size_t>(i * k + p)];
      const std::size_t bbase = static_cast<std::size_t>(p * n);
      const std::size_t obase = static_cast<std::size_t>(i * n);
      for (std::int64_t j = 0; j < n; ++j) {
        out[obase + static_cast<std::size_t>(j)] += av * b[bbase + static_cast<std::size_t>(j)];
      }
    }
  }
  return out;
}

inline std::vector<double> det_lu_and_sign_(const std::vector<double>& in, std::int64_t n, int* sign_out) {
  std::vector<double> m = in;
  int sign = 1;
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t piv = k;
    double maxv = std::abs(m[static_cast<std::size_t>(k * n + k)]);
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double v = std::abs(m[static_cast<std::size_t>(r * n + k)]);
      if (v > maxv) {
        maxv = v;
        piv = r;
      }
    }
    if (maxv < 1e-18) {
      if (sign_out) *sign_out = 0;
      return m;
    }
    if (piv != k) {
      for (std::int64_t c = k; c < n; ++c) {
        std::swap(m[static_cast<std::size_t>(k * n + c)], m[static_cast<std::size_t>(piv * n + c)]);
      }
      sign = -sign;
    }
    const double akk = m[static_cast<std::size_t>(k * n + k)];
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double f = m[static_cast<std::size_t>(r * n + k)] / akk;
      m[static_cast<std::size_t>(r * n + k)] = 0.0;
      for (std::int64_t c = k + 1; c < n; ++c) {
        m[static_cast<std::size_t>(r * n + c)] -= f * m[static_cast<std::size_t>(k * n + c)];
      }
    }
  }
  if (sign_out) *sign_out = sign;
  return m;
}

inline double det_gauss_(const std::vector<double>& in, std::int64_t n) {
  int sign = 1;
  auto m = det_lu_and_sign_(in, n, &sign);
  if (sign == 0) return 0.0;
  double d = static_cast<double>(sign);
  for (std::int64_t i = 0; i < n; ++i) {
    d *= m[static_cast<std::size_t>(i * n + i)];
  }
  return d;
}

inline std::vector<double> inverse_gauss_(const std::vector<double>& in, std::int64_t n) {
  std::vector<double> aug(static_cast<std::size_t>(n * 2 * n), 0.0);
  for (std::int64_t r = 0; r < n; ++r) {
    for (std::int64_t c = 0; c < n; ++c) aug[static_cast<std::size_t>(r * (2 * n) + c)] = in[static_cast<std::size_t>(r * n + c)];
    aug[static_cast<std::size_t>(r * (2 * n) + (n + r))] = 1.0;
  }
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t piv = k;
    double maxv = std::abs(aug[static_cast<std::size_t>(k * (2 * n) + k)]);
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double v = std::abs(aug[static_cast<std::size_t>(r * (2 * n) + k)]);
      if (v > maxv) {
        maxv = v;
        piv = r;
      }
    }
    if (maxv <= 1e-18) {
      throw std::runtime_error("subsp::math::linalg::inverse: singular matrix");
    }
    if (piv != k) {
      for (std::int64_t c = 0; c < 2 * n; ++c) {
        std::swap(aug[static_cast<std::size_t>(k * (2 * n) + c)], aug[static_cast<std::size_t>(piv * (2 * n) + c)]);
      }
    }
    const double diag = aug[static_cast<std::size_t>(k * (2 * n) + k)];
    for (std::int64_t c = 0; c < 2 * n; ++c) aug[static_cast<std::size_t>(k * (2 * n) + c)] /= diag;
    for (std::int64_t r = 0; r < n; ++r) {
      if (r == k) continue;
      const double f = aug[static_cast<std::size_t>(r * (2 * n) + k)];
      if (f == 0.0) continue;
      for (std::int64_t c = 0; c < 2 * n; ++c) aug[static_cast<std::size_t>(r * (2 * n) + c)] -= f * aug[static_cast<std::size_t>(k * (2 * n) + c)];
    }
  }
  std::vector<double> inv(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t r = 0; r < n; ++r) {
    for (std::int64_t c = 0; c < n; ++c) inv[static_cast<std::size_t>(r * n + c)] = aug[static_cast<std::size_t>(r * (2 * n) + (n + c))];
  }
  return inv;
}

inline std::vector<double> cholesky_lower_(const std::vector<double>& in, std::int64_t n) {
  std::vector<double> out(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    for (std::int64_t j = 0; j <= i; ++j) {
      double s = in[static_cast<std::size_t>(i * n + j)];
      for (std::int64_t k = 0; k < j; ++k) {
        s -= out[static_cast<std::size_t>(i * n + k)] * out[static_cast<std::size_t>(j * n + k)];
      }
      if (i == j) {
        if (s <= 0.0) throw std::runtime_error("subsp::math::linalg::cholesky: matrix is not SPD");
        out[static_cast<std::size_t>(i * n + j)] = std::sqrt(s);
      } else {
        out[static_cast<std::size_t>(i * n + j)] = s / out[static_cast<std::size_t>(j * n + j)];
      }
    }
  }
  return out;
}

inline std::vector<double> triangular_solve_(const std::vector<double>& b, const std::vector<double>& a,
                                             std::int64_t n, std::int64_t nrhs, bool upper, bool unitriangular) {
  std::vector<double> x = b;
  if (!upper) {
    for (std::int64_t c = 0; c < nrhs; ++c) {
      for (std::int64_t i = 0; i < n; ++i) {
        double s = x[static_cast<std::size_t>(i * nrhs + c)];
        for (std::int64_t k = 0; k < i; ++k) s -= a[static_cast<std::size_t>(i * n + k)] * x[static_cast<std::size_t>(k * nrhs + c)];
        const double d = unitriangular ? 1.0 : a[static_cast<std::size_t>(i * n + i)];
        x[static_cast<std::size_t>(i * nrhs + c)] = s / d;
      }
    }
    return x;
  }
  for (std::int64_t c = 0; c < nrhs; ++c) {
    for (std::int64_t i = n - 1; i >= 0; --i) {
      double s = x[static_cast<std::size_t>(i * nrhs + c)];
      for (std::int64_t k = i + 1; k < n; ++k) s -= a[static_cast<std::size_t>(i * n + k)] * x[static_cast<std::size_t>(k * nrhs + c)];
      const double d = unitriangular ? 1.0 : a[static_cast<std::size_t>(i * n + i)];
      x[static_cast<std::size_t>(i * nrhs + c)] = s / d;
    }
  }
  return x;
}

inline std::vector<double> matrix_power_(const std::vector<double>& in, std::int64_t n, std::int64_t e) {
  if (e == 0) return identity_(n);
  std::vector<double> base = in;
  std::uint64_t p = static_cast<std::uint64_t>(e < 0 ? -e : e);
  if (e < 0) base = inverse_gauss_(base, n);
  std::vector<double> out = identity_(n);
  while (p > 0) {
    if (p & 1ULL) out = matmul_(out, n, n, base, n);
    p >>= 1ULL;
    if (p) base = matmul_(base, n, n, base, n);
  }
  return out;
}

inline std::vector<double> matrix_exp_(const std::vector<double>& in, std::int64_t n) {
  std::vector<double> out = identity_(n);
  std::vector<double> term = identity_(n);
  for (std::int64_t k = 1; k <= 18; ++k) {
    term = matmul_(term, n, n, in, n);
    const double invk = 1.0 / static_cast<double>(k);
    for (double& v : term) v *= invk;
    for (std::size_t i = 0; i < out.size(); ++i) out[i] += term[i];
  }
  return out;
}

} // namespace detail

inline Series det(const Series& a) {
  detail::require_square_(a, "subsp::math::linalg::det");
  const auto n64 = a.sizes()[0];
#if defined(__APPLE__)
  int n = static_cast<int>(n64);
  auto row = detail::to_row_major_f64_(a);
  auto col = detail::row_to_col_major_(row, n, n);
  std::vector<int> ipiv(static_cast<std::size_t>(n));
  int info = 0;
  dgetrf_(&n, &n, col.data(), &n, ipiv.data(), &info);
  if (info < 0) throw std::runtime_error("subsp::math::linalg::det: invalid LAPACK arg");
  if (info > 0) return detail::scalar_from_value_(0.0);
  double d = 1.0;
  int sign = 1;
  for (int i = 0; i < n; ++i) {
    d *= col[static_cast<std::size_t>(i * n + i)];
    if (ipiv[static_cast<std::size_t>(i)] != (i + 1)) sign = -sign;
  }
  return detail::scalar_from_value_(sign * d);
#else
  const auto row = detail::to_row_major_f64_(a);
  return detail::scalar_from_value_(detail::det_gauss_(row, n64));
#endif
}

inline Series inverse(const Series& a) {
  detail::require_square_(a, "subsp::math::linalg::inverse");
  const auto n64 = a.sizes()[0];
#if defined(__APPLE__)
  int n = static_cast<int>(n64);
  auto row = detail::to_row_major_f64_(a);
  auto col = detail::row_to_col_major_(row, n, n);
  std::vector<int> ipiv(static_cast<std::size_t>(n));
  int info = 0;
  dgetrf_(&n, &n, col.data(), &n, ipiv.data(), &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::inverse: LU factorization failed");
  int lwork = -1;
  double wkopt = 0.0;
  dgetri_(&n, col.data(), &n, ipiv.data(), &wkopt, &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::inverse: LAPACK workspace query failed");
  lwork = std::max(1, static_cast<int>(wkopt));
  std::vector<double> work(static_cast<std::size_t>(lwork), 0.0);
  dgetri_(&n, col.data(), &n, ipiv.data(), work.data(), &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::inverse: singular matrix");
  return detail::matrix_from_row_major_(detail::col_to_row_major_(col, n, n), n, n);
#else
  const auto row = detail::to_row_major_f64_(a);
  return detail::matrix_from_row_major_(detail::inverse_gauss_(row, n64), n64, n64);
#endif
}

inline Series cholesky(const Series& a, bool upper = false) {
  detail::require_square_(a, "subsp::math::linalg::cholesky");
  const auto n64 = a.sizes()[0];
#if defined(__APPLE__)
  int n = static_cast<int>(n64);
  auto row = detail::to_row_major_f64_(a);
  auto col = detail::row_to_col_major_(row, n, n);
  char uplo = upper ? 'U' : 'L';
  int info = 0;
  dpotrf_(&uplo, &n, col.data(), &n, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::cholesky: matrix is not SPD");
  for (int c = 0; c < n; ++c) {
    for (int r = 0; r < n; ++r) {
      const bool keep = upper ? (r <= c) : (r >= c);
      if (!keep) col[static_cast<std::size_t>(c * n + r)] = 0.0;
    }
  }
  return detail::matrix_from_row_major_(detail::col_to_row_major_(col, n, n), n, n);
#else
  auto lower = detail::cholesky_lower_(detail::to_row_major_f64_(a), n64);
  if (upper) lower = detail::transpose_(lower, n64, n64);
  return detail::matrix_from_row_major_(lower, n64, n64);
#endif
}

inline Series logdet(const Series& a) {
  auto d = det(a).to_vec<double>();
  const double v = d.empty() ? 0.0 : d[0];
  return detail::scalar_from_value_(std::log(std::abs(v)));
}

inline Series slogdet(const Series& a) {
  auto d = det(a).to_vec<double>();
  const double v = d.empty() ? 0.0 : d[0];
  const double s = (v > 0.0) ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
  return detail::vector_from_values_({s, std::log(std::abs(v))});
}

inline Series matrix_power(const Series& a, std::int64_t n) {
  detail::require_square_(a, "subsp::math::linalg::matrix_power");
  const auto nn = a.sizes()[0];
  return detail::matrix_from_row_major_(detail::matrix_power_(detail::to_row_major_f64_(a), nn, n), nn, nn);
}

inline Series matrix_exp(const Series& a) {
  detail::require_square_(a, "subsp::math::linalg::matrix_exp");
  const auto n = a.sizes()[0];
  return detail::matrix_from_row_major_(detail::matrix_exp_(detail::to_row_major_f64_(a), n), n, n);
}

inline Series pinverse(const Series& a, double rcond = 1e-12) {
  detail::require_2d_(a, "subsp::math::linalg::pinverse");
  const auto m = a.sizes()[0];
  const auto n = a.sizes()[1];
  const auto ar = detail::to_row_major_f64_(a);
  if (m == n) return inverse(a);
  if (m >= n) {
    auto at = detail::transpose_(ar, m, n);
    auto gram = detail::matmul_(at, n, m, ar, n);
    for (std::int64_t i = 0; i < n; ++i) gram[static_cast<std::size_t>(i * n + i)] += rcond;
    auto inv = detail::inverse_gauss_(gram, n);
    return detail::matrix_from_row_major_(detail::matmul_(inv, n, n, at, m), n, m);
  }
  auto at = detail::transpose_(ar, m, n);
  auto gram = detail::matmul_(ar, m, n, at, m);
  for (std::int64_t i = 0; i < m; ++i) gram[static_cast<std::size_t>(i * m + i)] += rcond;
  auto inv = detail::inverse_gauss_(gram, m);
  return detail::matrix_from_row_major_(detail::matmul_(at, n, m, inv, m), n, m);
}

inline Series triangular_solve(const Series& b, const Series& a, bool upper = true, bool unitriangular = false) {
  detail::require_square_(a, "subsp::math::linalg::triangular_solve");
  auto aa = a.to(pluss::pCPU, ScalarType::Float64).contiguous();
  auto bb = b.to(pluss::pCPU, ScalarType::Float64).contiguous();
  const auto n = aa.sizes()[0];
  std::int64_t nrhs = 1;
  if (bb.dim() == 1) {
    if (bb.sizes()[0] != n) throw std::runtime_error("subsp::math::linalg::triangular_solve: rhs mismatch");
    const std::vector<std::int64_t> shape{n, 1};
    bb = bb.reshape(shape);
  } else {
    detail::require_2d_(bb, "subsp::math::linalg::triangular_solve");
    if (bb.sizes()[0] != n) throw std::runtime_error("subsp::math::linalg::triangular_solve: rhs mismatch");
    nrhs = bb.sizes()[1];
  }
#if defined(__APPLE__)
  int ni = static_cast<int>(n);
  int nrhsi = static_cast<int>(nrhs);
  auto a_col = detail::row_to_col_major_(aa.to_vec<double>(), n, n);
  auto b_col = detail::row_to_col_major_(bb.to_vec<double>(), n, nrhs);
  char uplo = upper ? 'U' : 'L';
  char trans = 'N';
  char diag = unitriangular ? 'U' : 'N';
  int info = 0;
  dtrtrs_(&uplo, &trans, &diag, &ni, &nrhsi, a_col.data(), &ni, b_col.data(), &ni, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::triangular_solve: LAPACK dtrtrs failed");
  return detail::matrix_from_row_major_(detail::col_to_row_major_(b_col, n, nrhs), n, nrhs);
#else
  const auto xr = detail::triangular_solve_(bb.to_vec<double>(), aa.to_vec<double>(), n, nrhs, upper, unitriangular);
  return detail::matrix_from_row_major_(xr, n, nrhs);
#endif
}

inline Series cholesky_solve(const Series& b, const Series& factor, bool upper = false) {
  detail::require_square_(factor, "subsp::math::linalg::cholesky_solve");
  auto l = factor.to(pluss::pCPU, ScalarType::Float64).contiguous();
  auto rhs = b.to(pluss::pCPU, ScalarType::Float64).contiguous();
  const auto n = l.sizes()[0];
  if (rhs.dim() != 2 || rhs.sizes()[0] != n) {
    throw std::runtime_error("subsp::math::linalg::cholesky_solve: rhs must be 2D with matching first dim");
  }
  const auto nrhs = rhs.sizes()[1];
#if defined(__APPLE__)
  int ni = static_cast<int>(n);
  int nrhsi = static_cast<int>(nrhs);
  auto l_col = detail::row_to_col_major_(l.to_vec<double>(), n, n);
  auto rhs_col = detail::row_to_col_major_(rhs.to_vec<double>(), n, nrhs);
  char uplo = upper ? 'U' : 'L';
  int info = 0;
  dpotrs_(&uplo, &ni, &nrhsi, l_col.data(), &ni, rhs_col.data(), &ni, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::cholesky_solve: LAPACK dpotrs failed");
  return detail::matrix_from_row_major_(detail::col_to_row_major_(rhs_col, n, nrhs), n, nrhs);
#else
  if (upper) l = l.transpose(0, 1).contiguous();
  const auto lrow = l.to_vec<double>();
  auto y = detail::triangular_solve_(rhs.to_vec<double>(), lrow, n, nrhs, /*upper=*/false, /*unitriangular=*/false);
  auto x = detail::triangular_solve_(y, lrow, n, nrhs, /*upper=*/true, /*unitriangular=*/false);
  return detail::matrix_from_row_major_(x, n, nrhs);
#endif
}

inline Series cholesky_inverse(const Series& factor, bool upper = false) {
  detail::require_square_(factor, "subsp::math::linalg::cholesky_inverse");
  auto l = factor.to(pluss::pCPU, ScalarType::Float64).contiguous();
  const auto n = l.sizes()[0];
#if defined(__APPLE__)
  int ni = static_cast<int>(n);
  auto l_col = detail::row_to_col_major_(l.to_vec<double>(), n, n);
  char uplo = upper ? 'U' : 'L';
  int info = 0;
  dpotri_(&uplo, &ni, l_col.data(), &ni, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::cholesky_inverse: LAPACK dpotri failed");
  for (std::int64_t c = 0; c < n; ++c) {
    for (std::int64_t r = 0; r < n; ++r) {
      if (upper) {
        if (r > c) {
          l_col[static_cast<std::size_t>(c * n + r)] =
              l_col[static_cast<std::size_t>(r * n + c)];
        }
      } else if (r < c) {
        l_col[static_cast<std::size_t>(c * n + r)] =
            l_col[static_cast<std::size_t>(r * n + c)];
      }
    }
  }
  return detail::matrix_from_row_major_(detail::col_to_row_major_(l_col, n, n), n, n);
#else
  if (upper) l = l.transpose(0, 1).contiguous();
  const auto lr = l.to_vec<double>();
  auto a = detail::matmul_(lr, n, n, detail::transpose_(lr, n, n), n);
  return detail::matrix_from_row_major_(detail::inverse_gauss_(a, n), n, n);
#endif
}

inline Series lu_solve(const Series& b, const Series& lu) {
  detail::require_square_(lu, "subsp::math::linalg::lu_solve");
  auto a = lu.to(pluss::pCPU, ScalarType::Float64).contiguous();
  const auto n = a.sizes()[0];
  auto rhs = b.to(pluss::pCPU, ScalarType::Float64).contiguous();
  if (rhs.dim() == 1) {
    const std::vector<std::int64_t> shape{lu.sizes()[0], 1};
    rhs = rhs.reshape(shape);
  }
  detail::require_2d_(rhs, "subsp::math::linalg::lu_solve");
  if (rhs.sizes()[0] != lu.sizes()[0]) throw std::runtime_error("subsp::math::linalg::lu_solve: rhs mismatch");
#if defined(__APPLE__)
  const auto nrhs = rhs.sizes()[1];
  int ni = static_cast<int>(n);
  int nrhsi = static_cast<int>(nrhs);
  auto a_col = detail::row_to_col_major_(a.to_vec<double>(), n, n);
  auto b_col = detail::row_to_col_major_(rhs.to_vec<double>(), n, nrhs);
  std::vector<int> ipiv(static_cast<std::size_t>(n), 0);
  int info = 0;
  dgesv_(&ni, &nrhsi, a_col.data(), &ni, ipiv.data(), b_col.data(), &ni, &info);
  if (info < 0) throw std::runtime_error("subsp::math::linalg::lu_solve: invalid LAPACK arg");
  if (info > 0) throw std::runtime_error("subsp::math::linalg::lu_solve: singular matrix");
  return detail::matrix_from_row_major_(detail::col_to_row_major_(b_col, n, nrhs), n, nrhs);
#else
  const auto rr = rhs.to_vec<double>();
  auto inv = inverse(lu);
  auto invr = inv.to_vec<double>();
  const auto out = detail::matmul_(invr, n, n, rr, rhs.sizes()[1]);
  return detail::matrix_from_row_major_(out, lu.sizes()[0], rhs.sizes()[1]);
#endif
}

inline QRResult qr(const Series& a, bool reduced = true) {
  detail::require_2d_(a, "subsp::math::linalg::qr");
#if defined(__APPLE__)
  int m = static_cast<int>(a.sizes()[0]);
  int n = static_cast<int>(a.sizes()[1]);
  int k = std::min(m, n);

  auto row = detail::to_row_major_f64_(a);
  auto col = detail::row_to_col_major_(row, m, n);
  std::vector<double> tau(static_cast<std::size_t>(k));
  int info = 0;

  int lwork = -1;
  double wkopt = 0.0;
  dgeqrf_(&m, &n, col.data(), &m, tau.data(), &wkopt, &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::qr: geqrf workspace query failed");
  lwork = std::max(1, static_cast<int>(wkopt));
  std::vector<double> work(static_cast<std::size_t>(lwork), 0.0);
  dgeqrf_(&m, &n, col.data(), &m, tau.data(), work.data(), &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::qr: geqrf failed");

  int q_cols = reduced ? k : m;
  std::vector<double> qcol(static_cast<std::size_t>(m * q_cols), 0.0);
  for (int c = 0; c < q_cols; ++c) {
    for (int r = 0; r < m; ++r) {
      if (c < n) qcol[static_cast<std::size_t>(c * m + r)] = col[static_cast<std::size_t>(c * m + r)];
      else qcol[static_cast<std::size_t>(c * m + r)] = (r == c) ? 1.0 : 0.0;
    }
  }
  lwork = -1;
  wkopt = 0.0;
  dorgqr_(&m, &q_cols, &k, qcol.data(), &m, tau.data(), &wkopt, &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::qr: orgqr workspace query failed");
  lwork = std::max(1, static_cast<int>(wkopt));
  work.assign(static_cast<std::size_t>(lwork), 0.0);
  dorgqr_(&m, &q_cols, &k, qcol.data(), &m, tau.data(), work.data(), &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::qr: orgqr failed");

  int r_rows = reduced ? k : m;
  std::vector<double> rrow(static_cast<std::size_t>(r_rows * n), 0.0);
  for (int r = 0; r < r_rows; ++r) {
    for (int c = r; c < n; ++c) {
      rrow[static_cast<std::size_t>(r * n + c)] = col[static_cast<std::size_t>(c * m + r)];
    }
  }
  return QRResult{
      detail::matrix_from_row_major_(detail::col_to_row_major_(qcol, m, q_cols), m, q_cols),
      detail::matrix_from_row_major_(rrow, r_rows, n)};
#else
  throw std::runtime_error("subsp::math::linalg::qr: LAPACK path unavailable on this platform");
#endif
}

inline SVDResult svd(const Series& a, bool full_matrices = true) {
  detail::require_2d_(a, "subsp::math::linalg::svd");
#if defined(__APPLE__)
  int m = static_cast<int>(a.sizes()[0]);
  int n = static_cast<int>(a.sizes()[1]);
  int k = std::min(m, n);
  char job = full_matrices ? 'A' : 'S';

  auto row = detail::to_row_major_f64_(a);
  auto col = detail::row_to_col_major_(row, m, n);

  std::vector<double> s(static_cast<std::size_t>(k), 0.0);
  int u_cols = (job == 'A') ? m : k;
  int vt_rows = (job == 'A') ? n : k;
  int ldvt = (job == 'A') ? n : vt_rows;
  std::vector<double> ucol(static_cast<std::size_t>(m * u_cols), 0.0);
  std::vector<double> vtcol(static_cast<std::size_t>(ldvt * n), 0.0);
  int info = 0;

  int lwork = -1;
  double wkopt = 0.0;
  dgesvd_(
      &job, &job, &m, &n, col.data(), &m, s.data(), ucol.data(), &m, vtcol.data(), &ldvt, &wkopt, &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::svd: workspace query failed");
  lwork = std::max(1, static_cast<int>(wkopt));
  std::vector<double> work(static_cast<std::size_t>(lwork), 0.0);
  dgesvd_(
      &job, &job, &m, &n, col.data(), &m, s.data(), ucol.data(), &m, vtcol.data(), &ldvt, work.data(), &lwork, &info);
  if (info != 0) throw std::runtime_error("subsp::math::linalg::svd: dgesvd failed");

  return SVDResult{
      detail::matrix_from_row_major_(detail::col_to_row_major_(ucol, m, u_cols), m, u_cols),
      detail::vector_from_values_(s),
      detail::matrix_from_row_major_(detail::col_to_row_major_(vtcol, vt_rows, n), vt_rows, n)};
#else
  throw std::runtime_error("subsp::math::linalg::svd: LAPACK path unavailable on this platform");
#endif
}

} // namespace pluss::subsp::math::linalg
