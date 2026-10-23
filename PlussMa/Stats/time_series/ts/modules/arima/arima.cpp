#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "../../../../../Math/Numerical/optimization/nelder_mead_minimizer.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kLog2Pi = 1.83787706640934533908;
constexpr double kHuge = 1e200;

std::vector<double> flat_f64_values_(const Series& x) {
  const ScalarType work_dtype = (x.platform() == pluss::Platform::MPS) ? ScalarType::Float32 : ScalarType::Float64;
  const Series flat = x.to(work_dtype).reshape({x.numel()}).contiguous();
  std::vector<double> out(static_cast<std::size_t>(flat.numel()), 0.0);
  if (flat.scalar_type() == ScalarType::Float32) {
    const auto* src = reinterpret_cast<const float*>(flat.raw_data());
    std::transform(src, src + flat.numel(), out.begin(), [](float v) { return static_cast<double>(v); });
  } else {
    const auto* src = reinterpret_cast<const double*>(flat.raw_data());
    std::copy(src, src + flat.numel(), out.begin());
  }
  return out;
}

struct ArimaParams {
  double intercept = 0.0;
  std::vector<double> ar{};
  std::vector<double> ma{};
  std::vector<double> sar{};
  std::vector<double> sma{};
  std::vector<double> beta{};
};

struct DiffPipeline {
  std::vector<std::vector<double>> seasonal_levels{};
  std::vector<std::vector<double>> regular_levels{};
  std::vector<double> transformed{};
  std::int64_t lost = 0;
};

struct ArmaEval {
  std::vector<double> eps{};
  std::vector<double> ll_per_obs{};
  double sigma2 = 1.0;
  std::int64_t burn = 0;
};

struct ExogMatrix {
  std::vector<double> values{};
  std::int64_t n = 0;
  std::int64_t k = 0;

  [[nodiscard]] bool empty() const noexcept { return k == 0; }
  [[nodiscard]] double at(std::int64_t row, std::int64_t col) const {
    return values[static_cast<std::size_t>(row * k + col)];
  }
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::stats::ts::") + name + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

Series as_matrix_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0], 1});
  }
  PLUSS_CHECK(sizes.size() == 2,
              std::string("subsp::stats::ts::") + name + ": expected vector or matrix");
  return x;
}

ExogMatrix as_exog_(const Series& x, std::int64_t expected_rows, const char* name, Config cfg) {
  if (!x.defined()) {
    return {};
  }
  const auto out_cfg = f64_cfg_(cfg);
  const Series xm = as_matrix_(x, name).to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = xm.sizes();
  PLUSS_CHECK(static_cast<std::int64_t>(sizes[0]) == expected_rows,
              std::string("subsp::stats::ts::") + name + ": expected row count to match y");
  const auto flat = flat_f64_values_(xm);
  return ExogMatrix{
      .values = flat,
      .n = static_cast<std::int64_t>(sizes[0]),
      .k = static_cast<std::int64_t>(sizes[1]),
  };
}

ExogMatrix exog_tail_(const ExogMatrix& x, std::int64_t keep_rows) {
  PLUSS_CHECK(keep_rows >= 0 && keep_rows <= x.n,
              "subsp::stats::ts::Arima: invalid exog tail rows");
  if (x.empty()) {
    return {};
  }
  const std::int64_t start = x.n - keep_rows;
  std::vector<double> out(static_cast<std::size_t>(keep_rows * x.k), 0.0);
  for (std::int64_t r = 0; r < keep_rows; ++r) {
    const auto src = x.values.begin() + (start + r) * x.k;
    const auto dst = out.begin() + r * x.k;
    std::copy(src, src + x.k, dst);
  }
  return ExogMatrix{.values = std::move(out), .n = keep_rows, .k = x.k};
}

double exog_dot_(const ExogMatrix* x, const std::vector<double>& beta, std::int64_t t) {
  if (x == nullptr || x->empty() || beta.empty()) {
    return 0.0;
  }
  double out = 0.0;
  for (std::int64_t j = 0; j < x->k; ++j) {
    out += beta[static_cast<std::size_t>(j)] * x->at(t, j);
  }
  return out;
}

std::pair<std::vector<double>, double> centered_(const Series& y, bool demean) {
  auto values = flat_f64_values_(y);
  PLUSS_CHECK(!values.empty(), "subsp::stats::ts::Arima: expected non-empty response series");
  const double mean = demean ? std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size()) : 0.0;
  if (demean) {
    std::transform(values.begin(), values.end(), values.begin(), [mean](const double v) { return v - mean; });
  }
  return {std::move(values), mean};
}

std::vector<double> centered_with_mean_(const Series& y, double mean) {
  auto values = flat_f64_values_(y);
  std::transform(values.begin(), values.end(), values.begin(), [mean](const double v) { return v - mean; });
  return values;
}

std::vector<double> diff_regular_once_(const std::vector<double>& x) {
  PLUSS_CHECK(x.size() >= 2, "subsp::stats::ts::Arima: regular differencing requires at least 2 observations");
  std::vector<double> out(x.size() - 1, 0.0);
  for (std::size_t i = 1; i < x.size(); ++i) {
    out[i - 1] = x[i] - x[i - 1];
  }
  return out;
}

std::vector<double> diff_seasonal_once_(const std::vector<double>& x, std::int64_t s) {
  PLUSS_CHECK(s > 0, "subsp::stats::ts::Arima: seasonal_period must be > 0 for seasonal differencing");
  PLUSS_CHECK(static_cast<std::int64_t>(x.size()) > s,
              "subsp::stats::ts::Arima: seasonal differencing requires observations > seasonal_period");
  std::vector<double> out(x.size() - static_cast<std::size_t>(s), 0.0);
  for (std::size_t i = static_cast<std::size_t>(s); i < x.size(); ++i) {
    out[i - static_cast<std::size_t>(s)] = x[i] - x[i - static_cast<std::size_t>(s)];
  }
  return out;
}

DiffPipeline build_pipeline_(const std::vector<double>& centered,
                             std::int64_t diff_order,
                             std::int64_t seasonal_diff_order,
                             std::int64_t seasonal_period) {
  PLUSS_CHECK(diff_order >= 0, "subsp::stats::ts::Arima: diff_order must be >= 0");
  PLUSS_CHECK(seasonal_diff_order >= 0, "subsp::stats::ts::Arima: seasonal_diff_order must be >= 0");
  if (seasonal_diff_order > 0) {
    PLUSS_CHECK(seasonal_period > 1,
                "subsp::stats::ts::Arima: seasonal_period must be > 1 when seasonal_diff_order > 0");
  }

  DiffPipeline pipe;
  pipe.seasonal_levels.push_back(centered);
  for (std::int64_t k = 0; k < seasonal_diff_order; ++k) {
    pipe.seasonal_levels.push_back(diff_seasonal_once_(pipe.seasonal_levels.back(), seasonal_period));
  }

  pipe.regular_levels.push_back(pipe.seasonal_levels.back());
  for (std::int64_t k = 0; k < diff_order; ++k) {
    pipe.regular_levels.push_back(diff_regular_once_(pipe.regular_levels.back()));
  }

  pipe.transformed = pipe.regular_levels.back();
  pipe.lost = static_cast<std::int64_t>(centered.size()) - static_cast<std::int64_t>(pipe.transformed.size());
  PLUSS_CHECK(!pipe.transformed.empty(), "subsp::stats::ts::Arima: transformed series is empty after differencing");
  return pipe;
}

std::vector<double> inv_regular_once_(double last_value, const std::vector<double>& diff_future) {
  std::vector<double> out(diff_future.size(), 0.0);
  for (std::size_t i = 0; i < diff_future.size(); ++i) {
    const double base = (i == 0) ? last_value : out[i - 1];
    out[i] = base + diff_future[i];
  }
  return out;
}

std::vector<double> tail_values_(const std::vector<double>& x, std::int64_t count) {
  PLUSS_CHECK(count >= 0, "subsp::stats::ts::Arima: tail count must be >= 0");
  PLUSS_CHECK(static_cast<std::int64_t>(x.size()) >= count,
              "subsp::stats::ts::Arima: tail extraction exceeds available history");
  return std::vector<double>(x.end() - count, x.end());
}

std::vector<double> inv_seasonal_once_(const std::vector<double>& last_s,
                                       const std::vector<double>& diff_future,
                                       std::int64_t s) {
  PLUSS_CHECK(s > 0, "subsp::stats::ts::Arima: seasonal_period must be > 0 for seasonal inversion");
  PLUSS_CHECK(static_cast<std::int64_t>(last_s.size()) == s,
              "subsp::stats::ts::Arima: last seasonal state size mismatch");
  std::vector<double> out(diff_future.size(), 0.0);
  for (std::size_t i = 0; i < diff_future.size(); ++i) {
    const double base = (static_cast<std::int64_t>(i) < s) ? last_s[i] : out[i - static_cast<std::size_t>(s)];
    out[i] = base + diff_future[i];
  }
  return out;
}

std::vector<double> invert_forecast_(const DiffPipeline& pipe,
                                     std::vector<double> transformed_forecast,
                                     std::int64_t diff_order,
                                     std::int64_t seasonal_diff_order,
                                     std::int64_t seasonal_period) {
  for (std::int64_t r = diff_order; r >= 1; --r) {
    transformed_forecast = inv_regular_once_(pipe.regular_levels[static_cast<std::size_t>(r - 1)].back(), transformed_forecast);
  }

  for (std::int64_t d = seasonal_diff_order; d >= 1; --d) {
    const auto last_s = tail_values_(pipe.seasonal_levels[static_cast<std::size_t>(d - 1)], seasonal_period);
    transformed_forecast = inv_seasonal_once_(last_s, transformed_forecast, seasonal_period);
  }

  return transformed_forecast;
}

std::vector<double> invert_zero_base_(std::vector<double> transformed,
                                      std::int64_t diff_order,
                                      std::int64_t seasonal_diff_order,
                                      std::int64_t seasonal_period) {
  for (std::int64_t r = diff_order; r >= 1; --r) {
    transformed = inv_regular_once_(0.0, transformed);
  }
  for (std::int64_t d = seasonal_diff_order; d >= 1; --d) {
    transformed = inv_seasonal_once_(std::vector<double>(static_cast<std::size_t>(seasonal_period), 0.0),
                                     transformed,
                                     seasonal_period);
  }
  return transformed;
}

// Autoregressive (and, with the sign flipped, moving-average) coefficients are tied to the partial autocorrelations: a polynomial
// 1 - phi_1 z - ... - phi_k z^k has all its roots outside the unit circle exactly when its k partial autocorrelations lie in (-1, 1)
// (Barndorff-Nielsen and Schou 1973; Monahan 1984). Parametrising by the partial autocorrelations therefore covers the whole stationary
// (invertible) region, not a subset, and the zero polynomial is an ordinary interior point.
std::vector<double> coefficients_from_pacf_(const std::vector<double>& r) {
  std::vector<double> phi;
  phi.reserve(r.size());
  for (std::size_t k = 0; k < r.size(); ++k) {
    std::vector<double> next(k + 1);
    for (std::size_t j = 0; j < k; ++j) {
      next[j] = phi[j] - r[k] * phi[k - 1 - j];
    }
    next[k] = r[k];
    phi = std::move(next);
  }
  return phi;
}

std::vector<double> pacf_from_coefficients_(std::vector<double> phi) {
  const std::size_t n = phi.size();
  std::vector<double> r(n, 0.0);
  for (std::size_t k = n; k >= 1; --k) {
    const double rk = phi[k - 1];
    r[k - 1] = rk;
    const double denom = 1.0 - rk * rk;
    if (!(std::abs(denom) > 1e-12)) {
      // on or outside the boundary: the lower-order coefficients are not recoverable, treat them as zero
      std::fill(phi.begin(), phi.end(), 0.0);
      continue;
    }
    std::vector<double> lower(k - 1);
    for (std::size_t j = 0; j + 1 < k; ++j) {
      lower[j] = (phi[j] + rk * phi[k - 2 - j]) / denom;
    }
    phi.assign(lower.begin(), lower.end());
    phi.resize(k - 1);
  }
  return r;
}

// Pull a coefficient vector inside the stationary region by clamping its partial autocorrelations to (-cap, cap); a vector that is
// already inside is returned unchanged.
std::vector<double> stabilize_coefficients_(const std::vector<double>& phi, double cap) {
  if (phi.empty()) {
    return phi;
  }
  auto r = pacf_from_coefficients_(phi);
  bool changed = false;
  for (double& v : r) {
    if (!std::isfinite(v)) {
      v = 0.0;
      changed = true;
    } else if (std::abs(v) > cap) {
      v = (v > 0.0 ? cap : -cap);
      changed = true;
    }
  }
  return changed ? coefficients_from_pacf_(r) : phi;
}

ArimaParams stabilize_params_(ArimaParams p, double stationarity_margin) {
  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  p.ar = stabilize_coefficients_(p.ar, cap);
  p.sar = stabilize_coefficients_(p.sar, cap);
  // moving-average polynomials 1 + theta z + ... are invertible exactly when the autoregressive polynomial with phi = -theta is stationary
  auto negate = [](std::vector<double> v) {
    for (double& x : v) x = -x;
    return v;
  };
  p.ma = negate(stabilize_coefficients_(negate(p.ma), cap));
  p.sma = negate(stabilize_coefficients_(negate(p.sma), cap));
  if (!std::isfinite(p.intercept)) {
    p.intercept = 0.0;
  }
  return p;
}

// Layout of the unconstrained vector: intercept, then one partial-autocorrelation block (atanh scale) for each of ar, sar, ma, sma,
// then the regression coefficients.
ArimaParams decode_theta_(const std::vector<double>& theta,
                          std::int64_t p,
                          std::int64_t q,
                          std::int64_t P,
                          std::int64_t Q,
                          std::int64_t k_beta,
                          double stationarity_margin) {
  const std::int64_t expected = 1 + p + P + q + Q + k_beta;
  PLUSS_CHECK(static_cast<std::int64_t>(theta.size()) == expected,
              "subsp::stats::ts::Arima: internal theta size mismatch");

  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  std::size_t idx = 0;
  ArimaParams out;
  out.intercept = theta[idx++];
  auto block = [&](std::int64_t count, bool negate) {
    std::vector<double> r(static_cast<std::size_t>(count));
    for (auto& v : r) {
      v = cap * std::tanh(theta[idx++]);
    }
    auto coeffs = coefficients_from_pacf_(r);
    if (negate) {
      for (double& c : coeffs) c = -c;
    }
    return coeffs;
  };
  out.ar = block(p, false);
  out.sar = block(P, false);
  out.ma = block(q, true);
  out.sma = block(Q, true);
  out.beta.resize(static_cast<std::size_t>(k_beta));
  for (auto& b : out.beta) {
    b = theta[idx++];
  }
  return out;
}

std::vector<double> encode_theta_(ArimaParams p, double stationarity_margin) {
  p = stabilize_params_(std::move(p), stationarity_margin);
  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  std::vector<double> out;
  out.push_back(p.intercept);
  auto block = [&](std::vector<double> coeffs, bool negate) {
    if (negate) {
      for (double& c : coeffs) c = -c;
    }
    for (double r : pacf_from_coefficients_(coeffs)) {
      out.push_back(std::atanh(std::clamp(r / cap, -0.999999, 0.999999)));
    }
  };
  block(p.ar, false);
  block(p.sar, false);
  block(p.ma, true);
  block(p.sma, true);
  out.insert(out.end(), p.beta.begin(), p.beta.end());
  return out;
}

// Full autoregressive and moving-average coefficients of the multiplicative model
//   (1 - phi_1 B - ...)(1 - Phi_1 B^s - ...) (x_t - mu) = (1 + theta_1 B + ...)(1 + Theta_1 B^s + ...) e_t,
// as the coefficient lists a (x_t = a_1 x_{t-1} + ... ) and b (... + b_1 e_{t-1} + ...). The seasonal and non-seasonal factors multiply, so
// the cross terms (lag s + 1 and so on) are present, as in the standard seasonal ARIMA model.
struct FullPolynomials {
  std::vector<double> a{};
  std::vector<double> b{};
};

std::vector<double> multiply_polynomials_(const std::vector<double>& u, const std::vector<double>& v) {
  std::vector<double> out(u.size() + v.size() - 1, 0.0);
  for (std::size_t i = 0; i < u.size(); ++i) {
    for (std::size_t j = 0; j < v.size(); ++j) {
      out[i + j] += u[i] * v[j];
    }
  }
  return out;
}

FullPolynomials expand_polynomials_(const std::vector<double>& ar,
                                    const std::vector<double>& ma,
                                    const std::vector<double>& sar,
                                    const std::vector<double>& sma,
                                    std::int64_t seasonal_period) {
  auto seasonal_poly = [&](const std::vector<double>& c, double sign) {
    std::vector<double> poly(1 + c.size() * static_cast<std::size_t>(std::max<std::int64_t>(seasonal_period, 1)), 0.0);
    poly[0] = 1.0;
    for (std::size_t j = 0; j < c.size(); ++j) {
      poly[(j + 1) * static_cast<std::size_t>(std::max<std::int64_t>(seasonal_period, 1))] = sign * c[j];
    }
    return poly;
  };
  auto plain_poly = [&](const std::vector<double>& c, double sign) {
    std::vector<double> poly(1 + c.size(), 0.0);
    poly[0] = 1.0;
    for (std::size_t j = 0; j < c.size(); ++j) {
      poly[j + 1] = sign * c[j];
    }
    return poly;
  };
  const auto ar_poly = multiply_polynomials_(plain_poly(ar, -1.0), seasonal_poly(sar, -1.0));
  const auto ma_poly = multiply_polynomials_(plain_poly(ma, 1.0), seasonal_poly(sma, 1.0));
  FullPolynomials out;
  for (std::size_t i = 1; i < ar_poly.size(); ++i) out.a.push_back(-ar_poly[i]);
  for (std::size_t i = 1; i < ma_poly.size(); ++i) out.b.push_back(ma_poly[i]);
  while (!out.a.empty() && out.a.back() == 0.0) out.a.pop_back();
  while (!out.b.empty() && out.b.back() == 0.0) out.b.pop_back();
  return out;
}

ArmaEval arma_eval_(const std::vector<double>& x,
                    const ArimaParams& p,
                    const ExogMatrix* exog,
                    std::int64_t seasonal_period,
                    double variance_floor) {
  const std::int64_t n = static_cast<std::int64_t>(x.size());
  if (exog != nullptr && !exog->empty()) {
    PLUSS_CHECK(exog->n == n, "subsp::stats::ts::Arima: exog rows must match transformed y length");
    PLUSS_CHECK(exog->k == static_cast<std::int64_t>(p.beta.size()),
                "subsp::stats::ts::Arima: exog columns must match beta size");
  } else {
    PLUSS_CHECK(p.beta.empty(), "subsp::stats::ts::Arima: beta provided but exog matrix is missing");
  }
  ArmaEval out;
  out.eps.assign(static_cast<std::size_t>(n), 0.0);
  out.ll_per_obs.assign(static_cast<std::size_t>(n), std::numeric_limits<double>::quiet_NaN());

  const auto poly = expand_polynomials_(p.ar, p.ma, p.sar, p.sma, seasonal_period);
  const std::int64_t na = static_cast<std::int64_t>(poly.a.size());
  const std::int64_t nb = static_cast<std::int64_t>(poly.b.size());
  out.burn = std::max<std::int64_t>({0, na, nb});
  PLUSS_CHECK(n > out.burn + 2,
              "subsp::stats::ts::Arima: effective sample is too short for requested ARIMA/SARIMA lags");

  for (std::int64_t t = 0; t < n; ++t) {
    double pred = p.intercept + exog_dot_(exog, p.beta, t);
    for (std::int64_t i = 1; i <= na; ++i) {
      if (t - i >= 0) pred += poly.a[static_cast<std::size_t>(i - 1)] * x[static_cast<std::size_t>(t - i)];
    }
    for (std::int64_t i = 1; i <= nb; ++i) {
      if (t - i >= 0) pred += poly.b[static_cast<std::size_t>(i - 1)] * out.eps[static_cast<std::size_t>(t - i)];
    }

    out.eps[static_cast<std::size_t>(t)] = x[static_cast<std::size_t>(t)] - pred;
  }

  double ss = 0.0;
  for (std::int64_t t = out.burn; t < n; ++t) {
    const double e = out.eps[static_cast<std::size_t>(t)];
    ss += e * e;
  }
  out.sigma2 = std::max(variance_floor, ss / static_cast<double>(n - out.burn));

  for (std::int64_t t = out.burn; t < n; ++t) {
    const double e = out.eps[static_cast<std::size_t>(t)];
    const double ll = -0.5 * (kLog2Pi + std::log(out.sigma2) + (e * e) / out.sigma2);
    out.ll_per_obs[static_cast<std::size_t>(t)] = ll;
  }
  return out;
}

// Exact Gaussian likelihood of the stationary ARMA process (x_t - mu) = sum a_i (x_{t-i} - mu) + e_t + sum b_i e_{t-i}, mu = c / (1 - sum a),
// by the Kalman filter on Harvey's state-space form, started from the stationary state covariance (obtained by the doubling iteration
// P <- P + M P M', M <- M M for the discrete Lyapunov equation P = T P T' + R R'). The innovations variance is concentrated out:
// sigma^2 = mean(v_t^2 / F_t). Returns innovations v_t as `eps`, the per-observation log-likelihood and sigma^2.
ArmaEval arma_exact_eval_(const std::vector<double>& x,
                          const ArimaParams& p,
                          std::int64_t seasonal_period,
                          double variance_floor) {
  const std::int64_t n = static_cast<std::int64_t>(x.size());
  ArmaEval out;
  out.eps.assign(static_cast<std::size_t>(n), 0.0);
  out.ll_per_obs.assign(static_cast<std::size_t>(n), std::numeric_limits<double>::quiet_NaN());
  out.burn = 0;
  const auto poly = expand_polynomials_(p.ar, p.ma, p.sar, p.sma, seasonal_period);
  const std::size_t na = poly.a.size();
  const std::size_t nb = poly.b.size();
  const std::size_t r = std::max<std::size_t>(1, std::max(na, nb + 1));
  PLUSS_CHECK(n > 2, "subsp::stats::ts::Arima: effective sample is too short");

  double sum_a = 0.0;
  for (double v : poly.a) sum_a += v;
  const double denom = 1.0 - sum_a;
  const double mu = (std::abs(denom) > 1e-10) ? p.intercept / denom : 0.0;

  std::vector<double> A(r, 0.0), R(r, 0.0);
  for (std::size_t i = 0; i < na; ++i) A[i] = poly.a[i];
  R[0] = 1.0;
  for (std::size_t i = 0; i < nb; ++i) R[i + 1] = poly.b[i];

  // stationary covariance by doubling
  auto matmul = [r](const std::vector<double>& X, const std::vector<double>& Y) {
    std::vector<double> Z(r * r, 0.0);
    for (std::size_t i = 0; i < r; ++i)
      for (std::size_t k = 0; k < r; ++k) {
        const double xik = X[i * r + k];
        if (xik == 0.0) continue;
        for (std::size_t j = 0; j < r; ++j) Z[i * r + j] += xik * Y[k * r + j];
      }
    return Z;
  };
  auto transpose = [r](const std::vector<double>& X) {
    std::vector<double> Z(r * r);
    for (std::size_t i = 0; i < r; ++i)
      for (std::size_t j = 0; j < r; ++j) Z[j * r + i] = X[i * r + j];
    return Z;
  };
  std::vector<double> P(r * r), M(r * r, 0.0);
  for (std::size_t i = 0; i < r; ++i)
    for (std::size_t j = 0; j < r; ++j) P[i * r + j] = R[i] * R[j];
  for (std::size_t i = 0; i < r; ++i) {
    M[i * r] = A[i];
    if (i + 1 < r) M[i * r + i + 1] = 1.0;
  }
  for (int iter = 0; iter < 80; ++iter) {
    const auto MPMt = matmul(matmul(M, P), transpose(M));
    double change = 0.0, scale = 0.0;
    for (std::size_t k = 0; k < r * r; ++k) {
      P[k] += MPMt[k];
      change = std::max(change, std::abs(MPMt[k]));
      scale = std::max(scale, std::abs(P[k]));
    }
    M = matmul(M, M);
    if (!(change <= 1e-15 * std::max(1.0, scale))) continue;
    break;
  }

  std::vector<double> a(r, 0.0);  // predicted state mean
  std::vector<double> Pa = P;     // predicted state covariance
  std::vector<double> TP(r * r), Pn(r * r), col(r);
  double sum_scaled = 0.0;
  std::vector<double> innovations_variance(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 0; t < n; ++t) {
    const double v = x[static_cast<std::size_t>(t)] - mu - a[0];
    const double F = Pa[0];
    if (!(F > 1e-300) || !std::isfinite(F) || !std::isfinite(v)) {
      out.sigma2 = variance_floor;
      return out;  // ll_per_obs left NaN -> treated as an invalid evaluation
    }
    out.eps[static_cast<std::size_t>(t)] = v;
    innovations_variance[static_cast<std::size_t>(t)] = F;
    sum_scaled += v * v / F;
    // filter update: state a + P[:,0] v / F, covariance P - P[:,0] P[0,:] / F
    for (std::size_t i = 0; i < r; ++i) col[i] = Pa[i * r];
    std::vector<double> a_f(r);
    for (std::size_t i = 0; i < r; ++i) a_f[i] = a[i] + col[i] * v / F;
    std::vector<double> P_f(r * r);
    for (std::size_t i = 0; i < r; ++i)
      for (std::size_t j = 0; j < r; ++j) P_f[i * r + j] = Pa[i * r + j] - col[i] * col[j] / F;
    // prediction: a <- T a_f, P <- T P_f T' + R R' with T = A e_0' + shift
    for (std::size_t i = 0; i < r; ++i) a[i] = A[i] * a_f[0] + ((i + 1 < r) ? a_f[i + 1] : 0.0);
    for (std::size_t i = 0; i < r; ++i)
      for (std::size_t j = 0; j < r; ++j) TP[i * r + j] = A[i] * P_f[j] + ((i + 1 < r) ? P_f[(i + 1) * r + j] : 0.0);
    for (std::size_t i = 0; i < r; ++i)
      for (std::size_t j = 0; j < r; ++j) Pn[i * r + j] = TP[i * r] * A[j] + ((j + 1 < r) ? TP[i * r + j + 1] : 0.0) + R[i] * R[j];
    Pa = Pn;
  }
  const double nd = static_cast<double>(n);
  out.sigma2 = std::max(variance_floor, sum_scaled / nd);
  // per-observation terms use the same concentrated sigma^2, so they add up to the profile log-likelihood
  const double ls2 = std::log(out.sigma2);
  for (std::int64_t t = 0; t < n; ++t) {
    const double F = innovations_variance[static_cast<std::size_t>(t)];
    const double v = out.eps[static_cast<std::size_t>(t)];
    out.ll_per_obs[static_cast<std::size_t>(t)] = -0.5 * (kLog2Pi + ls2 + std::log(F) + (v * v) / (out.sigma2 * F));
  }
  return out;
}

ArmaEval eval_dispatch_(const std::vector<double>& x,
                        const ArimaParams& p,
                        const ExogMatrix* exog,
                        std::int64_t seasonal_period,
                        double variance_floor,
                        bool exact) {
  if (exact && (exog == nullptr || exog->empty())) {
    return arma_exact_eval_(x, p, seasonal_period, variance_floor);
  }
  return arma_eval_(x, p, exog, seasonal_period, variance_floor);
}

double loglik_from_eval_(const ArmaEval& eval) {
  double ll = 0.0;
  for (double v : eval.ll_per_obs) {
    if (std::isfinite(v)) ll += v;
  }
  return ll;
}

double loglik_transformed_(const std::vector<double>& x,
                           const ExogMatrix* exog,
                           const ArimaParams& p,
                           std::int64_t seasonal_period,
                           double variance_floor,
                           bool exact) {
  const auto eval = eval_dispatch_(x, p, exog, seasonal_period, variance_floor, exact);
  return loglik_from_eval_(eval);
}

double nll_transformed_(const std::vector<double>& x,
                        const ExogMatrix* exog,
                        const ArimaParams& p,
                        std::int64_t seasonal_period,
                        double variance_floor,
                        bool exact) {
  const double ll = loglik_transformed_(x, exog, p, seasonal_period, variance_floor, exact);
  if (!std::isfinite(ll)) {
    return kHuge;
  }
  return -ll;
}

std::vector<double> normalize_init_(const std::vector<double>& values,
                                    std::int64_t order,
                                    const char* name) {
  PLUSS_CHECK(order >= 0, std::string("subsp::stats::ts::Arima::fit_mle: ") + name + " order must be >= 0");
  if (order == 0) {
    return {};
  }
  if (values.empty()) {
    return std::vector<double>(static_cast<std::size_t>(order), 0.0);
  }
  if (static_cast<std::int64_t>(values.size()) == 1) {
    return std::vector<double>(static_cast<std::size_t>(order), values.front());
  }
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == order,
              std::string("subsp::stats::ts::Arima::fit_mle: ") + name +
                  " initial size must be 1 or equal to its order");
  return values;
}

ArimaFit fit_arima_(const Series& y, const Series* x_opt, const ArimaOptions& options, Config cfg) {
  PLUSS_CHECK(options.ar_order >= 0, "subsp::stats::ts::Arima::fit_mle: ar_order must be >= 0");
  PLUSS_CHECK(options.ma_order >= 0, "subsp::stats::ts::Arima::fit_mle: ma_order must be >= 0");
  PLUSS_CHECK(options.seasonal_ar_order >= 0, "subsp::stats::ts::Arima::fit_mle: seasonal_ar_order must be >= 0");
  PLUSS_CHECK(options.seasonal_ma_order >= 0, "subsp::stats::ts::Arima::fit_mle: seasonal_ma_order must be >= 0");
  PLUSS_CHECK(options.diff_order >= 0, "subsp::stats::ts::Arima::fit_mle: diff_order must be >= 0");
  PLUSS_CHECK(options.seasonal_diff_order >= 0,
              "subsp::stats::ts::Arima::fit_mle: seasonal_diff_order must be >= 0");
  PLUSS_CHECK(options.variance_floor > 0.0, "subsp::stats::ts::Arima::fit_mle: variance_floor must be > 0");
  PLUSS_CHECK(options.stationarity_margin > 0.0 && options.stationarity_margin < 1.0,
              "subsp::stats::ts::Arima::fit_mle: stationarity_margin must be in (0,1)");
  PLUSS_CHECK(options.finite_diff_eps > 0.0, "subsp::stats::ts::Arima::fit_mle: finite_diff_eps must be > 0");
  PLUSS_CHECK(options.max_iterations > 0, "subsp::stats::ts::Arima::fit_mle: max_iterations must be > 0");
  PLUSS_CHECK(options.max_evaluations > 0, "subsp::stats::ts::Arima::fit_mle: max_evaluations must be > 0");
  if (options.seasonal_ar_order > 0 || options.seasonal_ma_order > 0 || options.seasonal_diff_order > 0) {
    PLUSS_CHECK(options.seasonal_period > 1,
                "subsp::stats::ts::Arima::fit_mle: seasonal_period must be > 1 for SARIMA terms");
  }

  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "Arima::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const ExogMatrix x_all = (x_opt != nullptr) ? as_exog_(*x_opt, yv.numel(), "Arima::fit_mle", out_cfg) : ExogMatrix{};
  const auto [centered, mean] = centered_(yv, options.demean);
  const auto pipe = build_pipeline_(centered,
                                    options.diff_order,
                                    options.seasonal_diff_order,
                                    options.seasonal_period);
  const ExogMatrix x_trans = x_all.empty() ? ExogMatrix{} : exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));

  ArimaParams init;
  init.intercept = options.intercept0;
  init.ar = normalize_init_(options.ar0, options.ar_order, "ar");
  init.ma = normalize_init_(options.ma0, options.ma_order, "ma");
  init.sar = normalize_init_(options.sar0, options.seasonal_ar_order, "sar");
  init.sma = normalize_init_(options.sma0, options.seasonal_ma_order, "sma");
  init.beta = normalize_init_(options.beta0, x_trans.k, "beta");
  init = stabilize_params_(std::move(init), options.stationarity_margin);

  const auto theta0 = encode_theta_(init, options.stationarity_margin);
  const Series initial = pluss::make_series(theta0, {static_cast<std::int64_t>(theta0.size())}, out_cfg);

  pluss::subsp::math::optimization::NelderMeadConfig nm;
  nm.max_iterations = options.max_iterations;
  nm.max_evaluations = options.max_evaluations;
  nm.simplex_delta = options.simplex_delta;
  nm.position_tolerance = 1e-7;
  nm.value_tolerance = 1e-7;

  const auto objective = [&](const Series& theta) -> double {
    const auto tv = flat_f64_values_(theta);
    const auto p = decode_theta_(tv,
                                 options.ar_order,
                                 options.ma_order,
                                 options.seasonal_ar_order,
                                 options.seasonal_ma_order,
                                 x_trans.k,
                                 options.stationarity_margin);
    return nll_transformed_(pipe.transformed, &x_trans, p, options.seasonal_period, options.variance_floor, options.exact_likelihood);
  };

  const auto out = pluss::subsp::math::optimization::nelder_mead_minimizer(objective, initial, nm);
  const auto best = flat_f64_values_(out.minimizer);
  auto best_p = decode_theta_(best,
                              options.ar_order,
                              options.ma_order,
                              options.seasonal_ar_order,
                              options.seasonal_ma_order,
                              x_trans.k,
                              options.stationarity_margin);
  best_p = stabilize_params_(std::move(best_p), options.stationarity_margin);

  const auto eval = eval_dispatch_(pipe.transformed, best_p, &x_trans, options.seasonal_period, options.variance_floor, options.exact_likelihood);

  return ArimaFit{
      .mean = mean,
      .intercept = best_p.intercept,
      .sigma = std::sqrt(eval.sigma2),
      .ar = best_p.ar,
      .ma = best_p.ma,
      .sar = best_p.sar,
      .sma = best_p.sma,
      .beta = best_p.beta,
      .diff_order = options.diff_order,
      .seasonal_diff_order = options.seasonal_diff_order,
      .seasonal_period = options.seasonal_period,
      .variance_floor = options.variance_floor,
      .stationarity_margin = options.stationarity_margin,
      .finite_diff_eps = options.finite_diff_eps,
      .exact_likelihood = options.exact_likelihood,
  };
}

ArimaParams to_params_(const ArimaFit& fit) {
  return stabilize_params_(ArimaParams{
                              .intercept = fit.intercept,
                              .ar = fit.ar,
                              .ma = fit.ma,
                              .sar = fit.sar,
                              .sma = fit.sma,
                              .beta = fit.beta,
                          },
                          fit.stationarity_margin);
}

std::vector<double> natural_params_(const ArimaFit& fit) {
  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(1 + fit.ar.size() + fit.ma.size() + fit.sar.size() + fit.sma.size() + fit.beta.size()));
  out.push_back(fit.intercept);
  out.insert(out.end(), fit.ar.begin(), fit.ar.end());
  out.insert(out.end(), fit.ma.begin(), fit.ma.end());
  out.insert(out.end(), fit.sar.begin(), fit.sar.end());
  out.insert(out.end(), fit.sma.begin(), fit.sma.end());
  out.insert(out.end(), fit.beta.begin(), fit.beta.end());
  return out;
}

ArimaParams perturb_natural_(ArimaParams p,
                             std::int64_t idx,
                             double delta,
                             double stationarity_margin) {
  if (idx == 0) {
    p.intercept += delta;
    return stabilize_params_(std::move(p), stationarity_margin);
  }

  std::int64_t k = idx - 1;
  auto apply = [&](std::vector<double>& v) -> bool {
    if (k < static_cast<std::int64_t>(v.size())) {
      v[static_cast<std::size_t>(k)] += delta;
      return true;
    }
    k -= static_cast<std::int64_t>(v.size());
    return false;
  };

  if (apply(p.ar) || apply(p.ma) || apply(p.sar) || apply(p.sma) || apply(p.beta)) {
    return stabilize_params_(std::move(p), stationarity_margin);
  }
  return stabilize_params_(std::move(p), stationarity_margin);
}

std::vector<double> score_numeric_(const std::vector<double>& x,
                                   const ExogMatrix* exog,
                                   const ArimaFit& fit,
                                   double finite_diff_eps) {
  const auto base = natural_params_(fit);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> g(static_cast<std::size_t>(d), 0.0);

  const auto p0 = to_params_(fit);
  for (std::int64_t i = 0; i < d; ++i) {
    const double h = std::max(finite_diff_eps, finite_diff_eps * std::abs(base[static_cast<std::size_t>(i)]));
    const auto p_plus = perturb_natural_(p0, i, h, fit.stationarity_margin);
    const auto p_minus = perturb_natural_(p0, i, -h, fit.stationarity_margin);
    const double f_plus = loglik_transformed_(x, exog, p_plus, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
    const double f_minus = loglik_transformed_(x, exog, p_minus, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
    const double grad = (f_plus - f_minus) / (2.0 * h);
    g[static_cast<std::size_t>(i)] = std::isfinite(grad) ? grad : 0.0;
  }
  return g;
}

std::vector<double> observed_inf_numeric_(const std::vector<double>& x,
                                          const ExogMatrix* exog,
                                          const ArimaFit& fit,
                                          double finite_diff_eps) {
  const auto base = natural_params_(fit);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> out(static_cast<std::size_t>(d * d), 0.0);

  const auto p0 = to_params_(fit);
  const auto step = [finite_diff_eps](double v) {
    return std::max(finite_diff_eps, finite_diff_eps * std::abs(v));
  };

  for (std::int64_t i = 0; i < d; ++i) {
    const double hi = step(base[static_cast<std::size_t>(i)]);
    for (std::int64_t j = 0; j < d; ++j) {
      const double hj = step(base[static_cast<std::size_t>(j)]);

      const auto pp = perturb_natural_(perturb_natural_(p0, i, hi, fit.stationarity_margin),
                                       j,
                                       hj,
                                       fit.stationarity_margin);
      const auto pm = perturb_natural_(perturb_natural_(p0, i, hi, fit.stationarity_margin),
                                       j,
                                       -hj,
                                       fit.stationarity_margin);
      const auto mp = perturb_natural_(perturb_natural_(p0, i, -hi, fit.stationarity_margin),
                                       j,
                                       hj,
                                       fit.stationarity_margin);
      const auto mm = perturb_natural_(perturb_natural_(p0, i, -hi, fit.stationarity_margin),
                                       j,
                                       -hj,
                                       fit.stationarity_margin);

      const double fpp = loglik_transformed_(x, exog, pp, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
      const double fpm = loglik_transformed_(x, exog, pm, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
      const double fmp = loglik_transformed_(x, exog, mp, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
      const double fmm = loglik_transformed_(x, exog, mm, fit.seasonal_period, fit.variance_floor, fit.exact_likelihood);
      const double h = (fpp - fpm - fmp + fmm) / (4.0 * hi * hj);
      out[static_cast<std::size_t>(i * d + j)] = std::isfinite(h) ? -h : 0.0;
    }
  }
  return out;
}

std::vector<double> arma_forecast_transformed_(std::int64_t horizon,
                                               const std::vector<double>& x,
                                               const std::vector<double>& eps,
                                               const ExogMatrix* x_future,
                                               const ArimaFit& fit) {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::ArimaFit::forecast: horizon must be > 0");
  if (x_future != nullptr && !x_future->empty()) {
    PLUSS_CHECK(x_future->n == horizon, "subsp::stats::ts::ArimaFit::forecast: x_future rows must match horizon");
    PLUSS_CHECK(x_future->k == static_cast<std::int64_t>(fit.beta.size()),
                "subsp::stats::ts::ArimaFit::forecast: x_future columns must match beta size");
  } else {
    PLUSS_CHECK(fit.beta.empty(), "subsp::stats::ts::ArimaFit::forecast: exogenous coefficients require x_future");
  }
  std::vector<double> x_ext = x;
  std::vector<double> e_ext = eps;
  const auto poly = expand_polynomials_(fit.ar, fit.ma, fit.sar, fit.sma, fit.seasonal_period);

  const std::int64_t n = static_cast<std::int64_t>(x.size());
  std::vector<double> out(static_cast<std::size_t>(horizon), 0.0);

  for (std::int64_t h = 0; h < horizon; ++h) {
    const std::int64_t t = n + h;
    double pred = fit.intercept + exog_dot_(x_future, fit.beta, h);
    for (std::int64_t i = 1; i <= static_cast<std::int64_t>(poly.a.size()); ++i) {
      if (t - i >= 0) pred += poly.a[static_cast<std::size_t>(i - 1)] * x_ext[static_cast<std::size_t>(t - i)];
    }
    for (std::int64_t i = 1; i <= static_cast<std::int64_t>(poly.b.size()); ++i) {
      if (t - i >= 0) pred += poly.b[static_cast<std::size_t>(i - 1)] * e_ext[static_cast<std::size_t>(t - i)];
    }

    out[static_cast<std::size_t>(h)] = pred;
    x_ext.push_back(pred);
    e_ext.push_back(0.0);
  }

  return out;
}

std::vector<double> arma_simulate_transformed_(std::int64_t n,
                                               const ArimaFit& fit,
                                               const ExogMatrix* exog,
                                               Config cfg,
                                               std::uint64_t seed) {
  PLUSS_CHECK(n > 0, "subsp::stats::ts::ArimaFit::simulate: num_observations must be > 0");
  if (exog != nullptr && !exog->empty()) {
    PLUSS_CHECK(exog->n == n, "subsp::stats::ts::ArimaFit::simulate: x rows must match num_observations");
    PLUSS_CHECK(exog->k == static_cast<std::int64_t>(fit.beta.size()),
                "subsp::stats::ts::ArimaFit::simulate: x columns must match beta size");
  } else {
    PLUSS_CHECK(fit.beta.empty(), "subsp::stats::ts::ArimaFit::simulate: exogenous coefficients require x");
  }
  const auto out_cfg = f64_cfg_(cfg);
  const auto z = flat_f64_values_(pluss::series::randn({n}, out_cfg, seed));
  const double sigma = std::max(fit.sigma, std::sqrt(fit.variance_floor));

  std::vector<double> y_state(static_cast<std::size_t>(n), 0.0);
  std::vector<double> eps(static_cast<std::size_t>(n), 0.0);
  const auto poly = expand_polynomials_(fit.ar, fit.ma, fit.sar, fit.sma, fit.seasonal_period);

  for (std::int64_t t = 0; t < n; ++t) {
    eps[static_cast<std::size_t>(t)] = sigma * z[static_cast<std::size_t>(t)];
    double pred = fit.intercept + exog_dot_(exog, fit.beta, t);
    for (std::int64_t i = 1; i <= static_cast<std::int64_t>(poly.a.size()); ++i) {
      if (t - i >= 0) pred += poly.a[static_cast<std::size_t>(i - 1)] * y_state[static_cast<std::size_t>(t - i)];
    }
    for (std::int64_t i = 1; i <= static_cast<std::int64_t>(poly.b.size()); ++i) {
      if (t - i >= 0) pred += poly.b[static_cast<std::size_t>(i - 1)] * eps[static_cast<std::size_t>(t - i)];
    }

    y_state[static_cast<std::size_t>(t)] = pred + eps[static_cast<std::size_t>(t)];
  }

  return y_state;
}

} // namespace

Series ArimaFit::infer(const Series& y, Config cfg) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::infer: fitted model requires exogenous x");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::infer").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), nullptr, seasonal_period, variance_floor, exact_likelihood);

  std::vector<double> out(static_cast<std::size_t>(pipe.lost), std::numeric_limits<double>::quiet_NaN());
  out.insert(out.end(), eval.eps.begin(), eval.eps.end());
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

Series ArimaFit::infer(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::infer").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x, yv.numel(), "ArimaFit::infer", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), &x_trans, seasonal_period, variance_floor, exact_likelihood);

  std::vector<double> out(static_cast<std::size_t>(pipe.lost), std::numeric_limits<double>::quiet_NaN());
  out.insert(out.end(), eval.eps.begin(), eval.eps.end());
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

double ArimaFit::loglik(const Series& y) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::loglik: fitted model requires exogenous x");
  const Series yv = as_vector_(y, "ArimaFit::loglik").to(ScalarType::Float64);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  return loglik_transformed_(pipe.transformed, nullptr, to_params_(*this), seasonal_period, variance_floor, exact_likelihood);
}

double ArimaFit::loglik(const Series& y, const Series& x) const {
  const auto out_cfg = f64_cfg_({});
  const Series yv = as_vector_(y, "ArimaFit::loglik").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x, yv.numel(), "ArimaFit::loglik", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  return loglik_transformed_(pipe.transformed, &x_trans, to_params_(*this), seasonal_period, variance_floor, exact_likelihood);
}

Series ArimaFit::loglik_per_obs(const Series& y, Config cfg) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::loglik_per_obs: fitted model requires exogenous x");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::loglik_per_obs").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), nullptr, seasonal_period, variance_floor, exact_likelihood);

  std::vector<double> out(static_cast<std::size_t>(pipe.lost), std::numeric_limits<double>::quiet_NaN());
  out.insert(out.end(), eval.ll_per_obs.begin(), eval.ll_per_obs.end());
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

Series ArimaFit::loglik_per_obs(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::loglik_per_obs").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x, yv.numel(), "ArimaFit::loglik_per_obs", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), &x_trans, seasonal_period, variance_floor, exact_likelihood);

  std::vector<double> out(static_cast<std::size_t>(pipe.lost), std::numeric_limits<double>::quiet_NaN());
  out.insert(out.end(), eval.ll_per_obs.begin(), eval.ll_per_obs.end());
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

Series ArimaFit::score(const Series& y, Config cfg) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::score: fitted model requires exogenous x");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::score").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const auto g = score_numeric_(pipe.transformed, nullptr, *this, finite_diff_eps);
  return pluss::make_series(g, {static_cast<std::int64_t>(g.size())}, out_cfg);
}

Series ArimaFit::score(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::score").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x, yv.numel(), "ArimaFit::score", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  const auto g = score_numeric_(pipe.transformed, &x_trans, *this, finite_diff_eps);
  return pluss::make_series(g, {static_cast<std::int64_t>(g.size())}, out_cfg);
}

Series ArimaFit::observed_inf(const Series& y, Config cfg) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::observed_inf: fitted model requires exogenous x");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::observed_inf").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const auto h = observed_inf_numeric_(pipe.transformed, nullptr, *this, finite_diff_eps);
  const std::int64_t d = static_cast<std::int64_t>(1 + ar.size() + ma.size() + sar.size() + sma.size() + beta.size());
  return pluss::make_series(h, {d, d}, out_cfg);
}

Series ArimaFit::observed_inf(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::observed_inf").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x, yv.numel(), "ArimaFit::observed_inf", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  const auto h = observed_inf_numeric_(pipe.transformed, &x_trans, *this, finite_diff_eps);
  const std::int64_t d = static_cast<std::int64_t>(1 + ar.size() + ma.size() + sar.size() + sma.size() + beta.size());
  return pluss::make_series(h, {d, d}, out_cfg);
}

Series ArimaFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::forecast: fitted model requires exogenous x_future");
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::ArimaFit::forecast: horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::forecast").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), nullptr, seasonal_period, variance_floor, exact_likelihood);

  const auto transformed_forecast = arma_forecast_transformed_(horizon, pipe.transformed, eval.eps, nullptr, *this);
  auto level_forecast = invert_forecast_(pipe,
                                         transformed_forecast,
                                         diff_order,
                                         seasonal_diff_order,
                                         seasonal_period);
  std::transform(level_forecast.begin(), level_forecast.end(), level_forecast.begin(), [this](double v) { return v + mean; });
  return pluss::make_series(level_forecast, {horizon}, out_cfg);
}

Series ArimaFit::forecast(std::int64_t horizon,
                          const Series& y,
                          const Series& x_hist,
                          const Series& x_future,
                          Config cfg) const {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::ArimaFit::forecast: horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "ArimaFit::forecast").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto pipe = build_pipeline_(centered, diff_order, seasonal_diff_order, seasonal_period);
  const ExogMatrix x_all = as_exog_(x_hist, yv.numel(), "ArimaFit::forecast", out_cfg);
  const ExogMatrix x_trans = exog_tail_(x_all, static_cast<std::int64_t>(pipe.transformed.size()));
  const ExogMatrix xh = as_exog_(x_future, horizon, "ArimaFit::forecast", out_cfg);
  const auto eval = eval_dispatch_(pipe.transformed, to_params_(*this), &x_trans, seasonal_period, variance_floor, exact_likelihood);

  const auto transformed_forecast = arma_forecast_transformed_(horizon, pipe.transformed, eval.eps, &xh, *this);
  auto level_forecast = invert_forecast_(pipe,
                                         transformed_forecast,
                                         diff_order,
                                         seasonal_diff_order,
                                         seasonal_period);
  std::transform(level_forecast.begin(), level_forecast.end(), level_forecast.begin(), [this](double v) { return v + mean; });
  return pluss::make_series(level_forecast, {horizon}, out_cfg);
}

Series ArimaFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  PLUSS_CHECK(beta.empty(), "subsp::stats::ts::ArimaFit::simulate: fitted model requires exogenous x");
  PLUSS_CHECK(num_observations > 0, "subsp::stats::ts::ArimaFit::simulate: num_observations must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const auto transformed = arma_simulate_transformed_(num_observations, *this, nullptr, out_cfg, seed);
  auto levels = invert_zero_base_(transformed, diff_order, seasonal_diff_order, seasonal_period);
  std::transform(levels.begin(), levels.end(), levels.begin(), [this](double v) { return v + mean; });
  return pluss::make_series(levels, {num_observations}, out_cfg);
}

Series ArimaFit::simulate(const Series& x, Config cfg, std::uint64_t seed) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series xm = as_matrix_(x, "ArimaFit::simulate").to(out_cfg.platform, out_cfg.dtype);
  const auto sizes = xm.sizes();
  const std::int64_t n = static_cast<std::int64_t>(sizes[0]);
  const auto flat = flat_f64_values_(xm);
  const ExogMatrix ex{
      .values = flat,
      .n = n,
      .k = static_cast<std::int64_t>(sizes[1]),
  };
  const auto transformed = arma_simulate_transformed_(n, *this, &ex, out_cfg, seed);
  auto levels = invert_zero_base_(transformed, diff_order, seasonal_diff_order, seasonal_period);
  std::transform(levels.begin(), levels.end(), levels.begin(), [this](double v) { return v + mean; });
  return pluss::make_series(levels, {n}, out_cfg);
}

ArimaFit Arima::fit_mle(const Series& y, const ArimaOptions& options, Config cfg) {
  return fit_arima_(y, nullptr, options, cfg);
}

ArimaFit Arima::fit_mle(const Series& y, const Series& x, const ArimaOptions& options, Config cfg) {
  return fit_arima_(y, &x, options, cfg);
}

Series SarimaFit::infer(const Series& y, Config cfg) const { return core.infer(y, cfg); }
Series SarimaFit::infer(const Series& y, const Series& x, Config cfg) const { return core.infer(y, x, cfg); }
double SarimaFit::loglik(const Series& y) const { return core.loglik(y); }
double SarimaFit::loglik(const Series& y, const Series& x) const { return core.loglik(y, x); }
Series SarimaFit::loglik_per_obs(const Series& y, Config cfg) const { return core.loglik_per_obs(y, cfg); }
Series SarimaFit::loglik_per_obs(const Series& y, const Series& x, Config cfg) const { return core.loglik_per_obs(y, x, cfg); }
Series SarimaFit::score(const Series& y, Config cfg) const { return core.score(y, cfg); }
Series SarimaFit::score(const Series& y, const Series& x, Config cfg) const { return core.score(y, x, cfg); }
Series SarimaFit::observed_inf(const Series& y, Config cfg) const { return core.observed_inf(y, cfg); }
Series SarimaFit::observed_inf(const Series& y, const Series& x, Config cfg) const { return core.observed_inf(y, x, cfg); }
Series SarimaFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const { return core.forecast(horizon, y, cfg); }
Series SarimaFit::forecast(std::int64_t horizon,
                           const Series& y,
                           const Series& x_hist,
                           const Series& x_future,
                           Config cfg) const {
  return core.forecast(horizon, y, x_hist, x_future, cfg);
}
Series SarimaFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  return core.simulate(num_observations, cfg, seed);
}
Series SarimaFit::simulate(const Series& x, Config cfg, std::uint64_t seed) const { return core.simulate(x, cfg, seed); }

SarimaFit Sarima::fit_mle(const Series& y, const SarimaOptions& options, Config cfg) {
  ArimaOptions opt;
  opt.ar_order = options.p;
  opt.diff_order = options.d;
  opt.ma_order = options.q;
  opt.seasonal_ar_order = options.P;
  opt.seasonal_diff_order = options.D;
  opt.seasonal_ma_order = options.Q;
  opt.seasonal_period = options.s;
  opt.demean = options.demean;
  opt.intercept0 = options.intercept0;
  opt.ar0 = options.ar0;
  opt.ma0 = options.ma0;
  opt.sar0 = options.sar0;
  opt.sma0 = options.sma0;
  opt.beta0 = options.beta0;
  opt.variance_floor = options.variance_floor;
  opt.stationarity_margin = options.stationarity_margin;
  opt.finite_diff_eps = options.finite_diff_eps;
  opt.max_iterations = options.max_iterations;
  opt.max_evaluations = options.max_evaluations;
  opt.simplex_delta = options.simplex_delta;
  opt.exact_likelihood = options.exact_likelihood;

  SarimaFit out;
  out.core = Arima::fit_mle(y, opt, cfg);
  out.p = options.p;
  out.d = options.d;
  out.q = options.q;
  out.P = options.P;
  out.D = options.D;
  out.Q = options.Q;
  out.s = options.s;
  return out;
}

SarimaFit Sarima::fit_mle(const Series& y, const Series& x, const SarimaOptions& options, Config cfg) {
  ArimaOptions opt;
  opt.ar_order = options.p;
  opt.diff_order = options.d;
  opt.ma_order = options.q;
  opt.seasonal_ar_order = options.P;
  opt.seasonal_diff_order = options.D;
  opt.seasonal_ma_order = options.Q;
  opt.seasonal_period = options.s;
  opt.demean = options.demean;
  opt.intercept0 = options.intercept0;
  opt.ar0 = options.ar0;
  opt.ma0 = options.ma0;
  opt.sar0 = options.sar0;
  opt.sma0 = options.sma0;
  opt.beta0 = options.beta0;
  opt.variance_floor = options.variance_floor;
  opt.stationarity_margin = options.stationarity_margin;
  opt.finite_diff_eps = options.finite_diff_eps;
  opt.max_iterations = options.max_iterations;
  opt.max_evaluations = options.max_evaluations;
  opt.simplex_delta = options.simplex_delta;
  opt.exact_likelihood = options.exact_likelihood;

  SarimaFit out;
  out.core = Arima::fit_mle(y, x, opt, cfg);
  out.p = options.p;
  out.d = options.d;
  out.q = options.q;
  out.P = options.P;
  out.D = options.D;
  out.Q = options.Q;
  out.s = options.s;
  return out;
}

} // namespace pluss::subsp::stats::ts
