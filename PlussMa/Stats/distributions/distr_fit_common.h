#pragma once

#include "bessel_k.h"
#include "pluss/tils/aux/scalar_cast.h"
#include "distr_object_detail.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::stats::distr::fit_detail {

inline std::vector<double> data_f64(const Series& x, std::string_view op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": x must be defined");
  const auto values = x.to(ScalarType::Float64).host<double>();
  PLUSS_CHECK(!values.empty(), std::string(op) + ": x must contain at least one observation");
  PLUSS_CHECK(std::all_of(values.begin(), values.end(), [](double v) { return std::isfinite(v); }),
              std::string(op) + ": x must contain finite values only");
  return values;
}

inline double wald_z(double level, std::string_view op) {
  PLUSS_CHECK(level > 0.0 && level < 1.0, std::string(op) + ": level must be in (0, 1)");
  constexpr double kEps = 1e-15;
  const double alpha = 1.0 - level;
  const double p = std::clamp(1.0 - 0.5 * alpha, kEps, 1.0 - kEps);
  return pluss::tils::to_scalar<double>(detail::scalar_f64(2.0 * p).erfcinv().mul(-std::sqrt(2.0)));
}

inline double fd_step(double x) {
  return std::max(1e-6, 1e-5 * std::max(1.0, std::abs(x)));
}

template <class Fn>
std::vector<double> fd_grad(const std::vector<double>& params, Fn&& fn) {
  std::vector<double> grad(params.size(), 0.0);
  std::vector<double> plus = params;
  std::vector<double> minus = params;
  for (std::size_t i = 0; i < params.size(); ++i) {
    const double h = fd_step(params[i]);
    plus[i] = params[i] + h;
    minus[i] = params[i] - h;
    const double fp = fn(plus);
    const double fm = fn(minus);
    grad[i] = (fp - fm) / (2.0 * h);
    plus[i] = params[i];
    minus[i] = params[i];
  }
  return grad;
}

template <class Fn>
std::vector<double> fd_hess_diag(const std::vector<double>& params, Fn&& fn) {
  std::vector<double> diag(params.size(), 0.0);
  std::vector<double> plus = params;
  std::vector<double> minus = params;
  const double f0 = fn(params);
  for (std::size_t i = 0; i < params.size(); ++i) {
    const double h = fd_step(params[i]);
    plus[i] = params[i] + h;
    minus[i] = params[i] - h;
    const double fp = fn(plus);
    const double fm = fn(minus);
    diag[i] = (fp - 2.0 * f0 + fm) / (h * h);
    plus[i] = params[i];
    minus[i] = params[i];
  }
  return diag;
}

inline Series vec_series(const std::vector<double>& values, std::span<const std::int64_t> shape, const Series& like) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline Series vec_series(const std::vector<double>& values, std::initializer_list<std::int64_t> shape, const Series& like) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline Series vec_series_cpu(const std::vector<double>& values, std::initializer_list<std::int64_t> shape) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   shape,
                                   Config{}.with_dtype(ScalarType::Float64));
}

inline Series scalar_series(double value, const Series& like) {
  return detail::scalar_f64(value).to(like.platform(), ScalarType::Float64);
}

inline Series diag_matrix_series(const std::vector<double>& diag, const Series& like) {
  const std::size_t n = diag.size();
  std::vector<double> data(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    data[i * n + i] = diag[i];
  }
  return pluss::codem::make_series(std::span<const double>(data.data(), data.size()),
                                   {static_cast<std::int64_t>(n), static_cast<std::int64_t>(n)},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline std::vector<double> diag_inverse(const std::vector<double>& diag, std::string_view op) {
  std::vector<double> inv(diag.size(), 0.0);
  std::transform(diag.begin(), diag.end(), inv.begin(), [op](double v) {
    PLUSS_CHECK(v > 0.0 && std::isfinite(v), std::string(op) + ": expected positive finite diagonal info");
    return 1.0 / v;
  });
  return inv;
}


// ---------------------------------------------------------------------------------------------------------------------
// Shared maximum-likelihood machinery: special functions, a full numerical Hessian, and the statistics of a fit that
// follow from it (observed information, covariance, standard errors, Wald intervals).
// ---------------------------------------------------------------------------------------------------------------------

inline double digamma(double x) {
  double r = 0.0;
  while (x < 6.0) {
    r -= 1.0 / x;
    x += 1.0;
  }
  const double f = 1.0 / (x * x);
  return r + std::log(x) - 0.5 / x - f * (1.0 / 12.0 - f * (1.0 / 120.0 - f * (1.0 / 252.0 - f * (1.0 / 240.0 - f / 132.0))));
}

inline double trigamma(double x) {
  double r = 0.0;
  while (x < 6.0) {
    r += 1.0 / (x * x);
    x += 1.0;
  }
  const double f = 1.0 / (x * x);
  return r + 1.0 / x + 0.5 * f + (1.0 / x) * f * (1.0 / 6.0 - f * (1.0 / 30.0 - f * (1.0 / 42.0 - f / 30.0)));
}

using LogLikFn = std::function<double(const std::vector<double>&)>;
constexpr double kPenalty = -1e300;

// Statistics of a fitted model from its log-likelihood: `k` parameters, of which some are fixed (known, not estimated);
// the matrices are k x k row-major with zero rows/columns for the fixed parameters.
struct FitStats {
  std::size_t k = 0;
  std::vector<double> info;
  std::vector<double> vcov;
  std::vector<double> se;
};

// Inverse of a small symmetric positive definite matrix by Gauss-Jordan elimination with partial pivoting; false if singular.
inline bool invert_matrix(std::vector<double> a, std::size_t n, std::vector<double>& inv) {
  inv.assign(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i) inv[i * n + i] = 1.0;
  for (std::size_t col = 0; col < n; ++col) {
    std::size_t piv = col;
    for (std::size_t r = col + 1; r < n; ++r) {
      if (std::abs(a[r * n + col]) > std::abs(a[piv * n + col])) piv = r;
    }
    if (!(std::abs(a[piv * n + col]) > 1e-300)) return false;
    if (piv != col) {
      for (std::size_t c = 0; c < n; ++c) {
        std::swap(a[piv * n + c], a[col * n + c]);
        std::swap(inv[piv * n + c], inv[col * n + c]);
      }
    }
    const double d = 1.0 / a[col * n + col];
    for (std::size_t c = 0; c < n; ++c) {
      a[col * n + c] *= d;
      inv[col * n + c] *= d;
    }
    for (std::size_t r = 0; r < n; ++r) {
      if (r == col) continue;
      const double f = a[r * n + col];
      if (f == 0.0) continue;
      for (std::size_t c = 0; c < n; ++c) {
        a[r * n + c] -= f * a[col * n + c];
        inv[r * n + c] -= f * inv[col * n + c];
      }
    }
  }
  return true;
}

inline double hess_step(double v) { return 1e-4 * std::max(std::abs(v), 1e-2); }

// Observed information, covariance and standard errors over the free parameters, from central differences of the log-likelihood.
inline FitStats fit_stats(const std::vector<double>& params, const std::vector<bool>& is_free, const LogLikFn& ll) {
  const std::size_t k = params.size();
  std::vector<std::size_t> free_idx;
  for (std::size_t i = 0; i < k; ++i) {
    if (is_free[i]) free_idx.push_back(i);
  }
  const std::size_t m = free_idx.size();
  FitStats out;
  out.k = k;
  out.info.assign(k * k, 0.0);
  out.vcov.assign(k * k, 0.0);
  out.se.assign(k, 0.0);
  if (m == 0) return out;

  const auto at = [&](std::size_t i, double di, std::size_t j, double dj) {
    std::vector<double> p = params;
    p[free_idx[i]] += di;
    p[free_idx[j]] += dj;
    return ll(p);
  };
  const double f0 = ll(params);
  std::vector<double> h(m);
  for (std::size_t i = 0; i < m; ++i) h[i] = hess_step(params[free_idx[i]]);
  std::vector<double> info(m * m, 0.0);
  for (std::size_t i = 0; i < m; ++i) {
    info[i * m + i] = -(at(i, h[i], i, 0.0) - 2.0 * f0 + at(i, -h[i], i, 0.0)) / (h[i] * h[i]);
    for (std::size_t j = i + 1; j < m; ++j) {
      const double v = -(at(i, h[i], j, h[j]) - at(i, h[i], j, -h[j]) - at(i, -h[i], j, h[j]) + at(i, -h[i], j, -h[j])) /
                       (4.0 * h[i] * h[j]);
      info[i * m + j] = info[j * m + i] = v;
    }
  }
  std::vector<double> cov;
  const bool ok = invert_matrix(info, m, cov);
  for (std::size_t i = 0; i < m; ++i) {
    for (std::size_t j = 0; j < m; ++j) {
      out.info[free_idx[i] * k + free_idx[j]] = info[i * m + j];
      out.vcov[free_idx[i] * k + free_idx[j]] = ok ? cov[i * m + j] : std::numeric_limits<double>::quiet_NaN();
    }
    const double var = ok ? cov[i * m + i] : std::numeric_limits<double>::quiet_NaN();
    out.se[free_idx[i]] = (var >= 0.0) ? std::sqrt(var) : std::numeric_limits<double>::quiet_NaN();
  }
  return out;
}

// Central-difference gradient of the log-likelihood over the free parameters (zero for the fixed ones).
inline std::vector<double> fit_score(const std::vector<double>& params, const std::vector<bool>& is_free, const LogLikFn& ll) {
  std::vector<double> g(params.size(), 0.0);
  for (std::size_t i = 0; i < params.size(); ++i) {
    if (!is_free[i]) continue;
    const double h = hess_step(params[i]);
    std::vector<double> a = params, b = params;
    a[i] += h;
    b[i] -= h;
    g[i] = (ll(a) - ll(b)) / (2.0 * h);
  }
  return g;
}

inline Series matrix_series(const std::vector<double>& values, std::size_t k, const Series& like) {
  return pluss::codem::make_series(std::span<const double>(values.data(), values.size()),
                                   {static_cast<std::int64_t>(k), static_cast<std::int64_t>(k)},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

inline Series interval_series(const std::vector<double>& params, const std::vector<double>& se, double z, const Series& like) {
  std::vector<double> out(params.size() * 2);
  for (std::size_t i = 0; i < params.size(); ++i) {
    out[2 * i] = params[i] - z * se[i];
    out[2 * i + 1] = params[i] + z * se[i];
  }
  return pluss::codem::make_series(std::span<const double>(out.data(), out.size()),
                                   {static_cast<std::int64_t>(params.size()), 2},
                                   Config{}.with_dtype(ScalarType::Float64))
      .to(like.platform(), ScalarType::Float64);
}

// Nelder-Mead simplex maximisation of `f` (returns the maximiser); a plain, dependency-free implementation used to fit
// models without closed-form estimators. Restarts from the best point until the value stops improving.
inline std::vector<double> maximize_simplex(const LogLikFn& f, std::vector<double> start, double step, int max_iter = 4000, int restarts = 4) {
  const std::size_t n = start.size();
  std::vector<double> best = start;
  double best_val = f(best);
  for (int round = 0; round < restarts; ++round) {
    std::vector<std::vector<double>> simplex(n + 1, best);
    std::vector<double> val(n + 1, best_val);
    for (std::size_t i = 0; i < n; ++i) {
      simplex[i + 1][i] += (std::abs(best[i]) > 1e-8 ? step * std::abs(best[i]) : step);
      val[i + 1] = f(simplex[i + 1]);
    }
    for (int it = 0; it < max_iter; ++it) {
      std::vector<std::size_t> order(n + 1);
      std::iota(order.begin(), order.end(), 0);
      std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return val[a] > val[b]; });
      std::vector<std::vector<double>> s2(n + 1);
      std::vector<double> v2(n + 1);
      for (std::size_t i = 0; i <= n; ++i) {
        s2[i] = simplex[order[i]];
        v2[i] = val[order[i]];
      }
      simplex.swap(s2);
      val.swap(v2);
      if (std::abs(val[0] - val[n]) < 1e-13 * (1.0 + std::abs(val[0]))) break;
      std::vector<double> centroid(n, 0.0);
      for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) centroid[j] += simplex[i][j] / static_cast<double>(n);
      }
      const auto along = [&](double t) {
        std::vector<double> p(n);
        for (std::size_t j = 0; j < n; ++j) p[j] = centroid[j] + t * (simplex[n][j] - centroid[j]);
        return p;
      };
      const auto reflected = along(-1.0);
      const double fr = f(reflected);
      if (fr > val[0]) {
        const auto expanded = along(-2.0);
        const double fe = f(expanded);
        if (fe > fr) { simplex[n] = expanded; val[n] = fe; } else { simplex[n] = reflected; val[n] = fr; }
      } else if (fr > val[n - 1]) {
        simplex[n] = reflected;
        val[n] = fr;
      } else {
        const auto contracted = along(fr > val[n] ? -0.5 : 0.5);
        const double fc = f(contracted);
        if (fc > std::max(fr, val[n])) {
          simplex[n] = contracted;
          val[n] = fc;
        } else {
          for (std::size_t i = 1; i <= n; ++i) {
            for (std::size_t j = 0; j < n; ++j) simplex[i][j] = simplex[0][j] + 0.5 * (simplex[i][j] - simplex[0][j]);
            val[i] = f(simplex[i]);
          }
        }
      }
    }
    std::size_t arg = 0;
    for (std::size_t i = 1; i <= n; ++i) {
      if (val[i] > val[arg]) arg = i;
    }
    if (val[arg] > best_val + 1e-12 * (1.0 + std::abs(best_val))) {
      best = simplex[arg];
      best_val = val[arg];
    } else {
      if (val[arg] > best_val) best = simplex[arg];
      break;
    }
  }
  return best;
}


// Closed-form log-likelihoods of the families fitted with the shared machinery above (parameter vectors in declaration order).
inline LogLikFn gamma_loglik(const std::vector<double>& v) {  // {shape, scale, location}
  return [&v](const std::vector<double>& p) -> double {
    if (p.size() != 3 || !(p[0] > 0.0) || !(p[1] > 0.0)) return kPenalty;
    double sum_log = 0.0, sum = 0.0;
    for (double x : v) {
      const double d = x - p[2];
      if (!(d > 0.0)) return kPenalty;
      sum_log += std::log(d);
      sum += d;
    }
    const double n = static_cast<double>(v.size());
    return -n * std::lgamma(p[0]) - n * p[0] * std::log(p[1]) + (p[0] - 1.0) * sum_log - sum / p[1];
  };
}

inline LogLikFn beta_loglik(const std::vector<double>& v) {  // {low, high, alpha, beta}
  return [&v](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[1] > p[0]) || !(p[2] > 0.0) || !(p[3] > 0.0)) return kPenalty;
    double s1 = 0.0, s2 = 0.0;
    for (double x : v) {
      if (!(x > p[0] && x < p[1])) return kPenalty;
      s1 += std::log(x - p[0]);
      s2 += std::log(p[1] - x);
    }
    const double n = static_cast<double>(v.size());
    const double log_b = std::lgamma(p[2]) + std::lgamma(p[3]) - std::lgamma(p[2] + p[3]);
    return (p[2] - 1.0) * s1 + (p[3] - 1.0) * s2 - n * log_b - n * (p[2] + p[3] - 1.0) * std::log(p[1] - p[0]);
  };
}

inline LogLikFn negbin_loglik(const std::vector<double>& v) {  // {r, p}
  return [&v](const std::vector<double>& p) -> double {
    if (p.size() != 2 || !(p[0] > 0.0) || !(p[1] > 0.0) || !(p[1] < 1.0)) return kPenalty;
    double acc = 0.0;
    for (double x : v) acc += std::lgamma(x + p[0]) - std::lgamma(p[0]) - std::lgamma(x + 1.0) + x * std::log1p(-p[1]);
    return acc + static_cast<double>(v.size()) * p[0] * std::log(p[1]);
  };
}

inline LogLikFn nig_loglik(const std::vector<double>& v) {  // {alpha, beta, location, scale}
  return [&v](const std::vector<double>& p) -> double {
    if (p.size() != 4 || !(p[0] > std::abs(p[1])) || !(p[3] > 0.0)) return kPenalty;
    const double g = std::sqrt(p[0] * p[0] - p[1] * p[1]);
    double acc = 0.0;
    for (double x : v) {
      const double d = x - p[2];
      const double q = std::sqrt(p[3] * p[3] + d * d);
      acc += p[3] * g + p[1] * d + besselk::log_k(1.0, p[0] * q) - std::log(q);
    }
    const double n = static_cast<double>(v.size());
    return acc + n * std::log(p[0] * p[3] / std::numbers::pi_v<double>);
  };
}

inline LogLikFn gh_loglik(const std::vector<double>& v) {  // {lambda, alpha, beta, location, scale}
  return [&v](const std::vector<double>& p) -> double {
    if (p.size() != 5 || !(p[1] > std::abs(p[2])) || !(p[4] > 0.0)) return kPenalty;
    const double lam = p[0], g = std::sqrt(p[1] * p[1] - p[2] * p[2]);
    double acc = 0.0;
    for (double x : v) {
      const double d = x - p[3];
      const double q = std::sqrt(p[4] * p[4] + d * d);
      acc += besselk::log_k(lam - 0.5, p[1] * q) - (0.5 - lam) * (std::log(q) - std::log(p[1])) + p[2] * d;
    }
    const double n = static_cast<double>(v.size());
    return acc + n * (lam * (std::log(g) - std::log(p[4])) - 0.5 * std::log(2.0 * std::numbers::pi_v<double>) - besselk::log_k(lam, p[4] * g));
  };
}

// Defines the statistics methods of a fit type from its log-likelihood: params_expr is the parameter vector (all parameters,
// in declaration order), free_expr a std::vector<bool> marking those that were estimated, and make_ll(values) the log-likelihood
// as a function of the full parameter vector (kPenalty outside the parameter space).
#define PLUSS_DISTR_FIT_STATS(FitType, params_expr, free_expr, make_ll)                                                     \
  Series FitType::score(const Series& x) const {                                                                            \
    const auto values = fit_detail::data_f64(x, #FitType "::score");                                                        \
    return fit_detail::vec_series(fit_detail::fit_score(params_expr, free_expr, make_ll(values)), {static_cast<std::int64_t>((params_expr).size())}, x); \
  }                                                                                                                         \
  Series FitType::observed_inf(const Series& x) const {                                                                     \
    const auto values = fit_detail::data_f64(x, #FitType "::observed_inf");                                                 \
    const auto st = fit_detail::fit_stats(params_expr, free_expr, make_ll(values));                                         \
    return fit_detail::matrix_series(st.info, st.k, x);                                                                     \
  }                                                                                                                         \
  Series FitType::fisher_inf(const Series& x) const { return observed_inf(x); }                                             \
  Series FitType::vcov(const Series& x) const {                                                                             \
    const auto values = fit_detail::data_f64(x, #FitType "::vcov");                                                         \
    const auto st = fit_detail::fit_stats(params_expr, free_expr, make_ll(values));                                         \
    return fit_detail::matrix_series(st.vcov, st.k, x);                                                                     \
  }                                                                                                                         \
  Series FitType::stderr(const Series& x) const {                                                                           \
    const auto values = fit_detail::data_f64(x, #FitType "::stderr");                                                       \
    const auto st = fit_detail::fit_stats(params_expr, free_expr, make_ll(values));                                         \
    return fit_detail::vec_series(st.se, {static_cast<std::int64_t>(st.k)}, x);                                             \
  }                                                                                                                         \
  Series FitType::confint(const Series& x, double level) const {                                                            \
    const auto values = fit_detail::data_f64(x, #FitType "::confint");                                                      \
    const auto st = fit_detail::fit_stats(params_expr, free_expr, make_ll(values));                                         \
    return fit_detail::interval_series(params_expr, st.se, fit_detail::wald_z(level, #FitType "::confint"), x);             \
  }

} // namespace pluss::subsp::stats::distr::fit_detail
