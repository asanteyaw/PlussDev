#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "../../../../Math/Numerical/optimization/nelder_mead_minimizer.h"
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

struct GarchParams {
  double omega = 0.02;
  std::vector<double> arch{0.08};
  std::vector<double> garch{0.90};
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

double stable_sigmoid_(double x) {
  if (x >= 0.0) {
    const double z = std::exp(-x);
    return 1.0 / (1.0 + z);
  }
  const double z = std::exp(x);
  return z / (1.0 + z);
}

double stable_logit_(double p) {
  const double q = std::clamp(p, 1e-12, 1.0 - 1e-12);
  return std::log(q / (1.0 - q));
}

double sample_var_(const std::vector<double>& x) {
  if (x.empty()) {
    return 0.0;
  }
  const double mean = std::accumulate(x.begin(), x.end(), 0.0) / static_cast<double>(x.size());
  const double ss = std::accumulate(x.begin(), x.end(), 0.0, [mean](const double acc, const double v) {
    const double d = v - mean;
    return acc + d * d;
  });
  return ss / static_cast<double>(x.size());
}

std::pair<std::vector<double>, double> centered_(const Series& y, bool demean) {
  auto values = flat_f64_values_(y);
  PLUSS_CHECK(!values.empty(), "subsp::stats::ts::Garch: expected non-empty response series");
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

double coeff_sum_(const GarchParams& p) {
  const double sa = std::accumulate(p.arch.begin(), p.arch.end(), 0.0);
  const double sb = std::accumulate(p.garch.begin(), p.garch.end(), 0.0);
  return sa + sb;
}

bool valid_params_(const GarchParams& p, double variance_floor, double stationarity_margin) {
  if (!(std::isfinite(p.omega) && p.omega > variance_floor)) return false;
  const auto nonneg_finite = [](double v) { return std::isfinite(v) && v >= 0.0; };
  if (!std::all_of(p.arch.begin(), p.arch.end(), nonneg_finite)) return false;
  if (!std::all_of(p.garch.begin(), p.garch.end(), nonneg_finite)) return false;
  return coeff_sum_(p) < (1.0 - stationarity_margin);
}

GarchParams clipped_params_(GarchParams p, double variance_floor, double stationarity_margin) {
  p.omega = std::max(p.omega, variance_floor * (1.0 + 1e-3));
  auto clamp_nonneg = [](double& v) { v = (std::isfinite(v) && v > 0.0) ? v : 0.0; };
  std::for_each(p.arch.begin(), p.arch.end(), clamp_nonneg);
  std::for_each(p.garch.begin(), p.garch.end(), clamp_nonneg);

  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  const double s = coeff_sum_(p);
  if (s >= cap) {
    const double scale = cap / std::max(s, 1e-15);
    std::for_each(p.arch.begin(), p.arch.end(), [scale](double& v) { v *= scale; });
    std::for_each(p.garch.begin(), p.garch.end(), [scale](double& v) { v *= scale; });
  }
  return p;
}

std::int64_t coeff_count_(std::int64_t m, std::int64_t n) {
  return m + n;
}

std::vector<double> decode_weights_stick_(const std::vector<double>& theta) {
  const std::int64_t k = static_cast<std::int64_t>(theta.size()) + 1;
  std::vector<double> w(static_cast<std::size_t>(k), 0.0);
  double remaining = 1.0;
  for (std::int64_t i = 0; i < k - 1; ++i) {
    const double z = stable_sigmoid_(theta[static_cast<std::size_t>(i)]);
    w[static_cast<std::size_t>(i)] = remaining * z;
    remaining *= (1.0 - z);
  }
  w.back() = std::max(0.0, remaining);
  return w;
}

std::vector<double> encode_weights_stick_(const std::vector<double>& w) {
  const std::int64_t k = static_cast<std::int64_t>(w.size());
  if (k <= 1) {
    return {};
  }
  std::vector<double> theta(static_cast<std::size_t>(k - 1), 0.0);
  double remaining = 1.0;
  for (std::int64_t i = 0; i < k - 1; ++i) {
    const double zi = std::clamp(w[static_cast<std::size_t>(i)] / std::max(remaining, 1e-15), 1e-12, 1.0 - 1e-12);
    theta[static_cast<std::size_t>(i)] = stable_logit_(zi);
    remaining *= (1.0 - zi);
  }
  return theta;
}

GarchParams decode_theta_(const std::vector<double>& theta,
                          std::int64_t arch_order,
                          std::int64_t garch_order,
                          double variance_floor,
                          double stationarity_margin) {
  const std::int64_t k = coeff_count_(arch_order, garch_order);
  const std::int64_t expected = (k == 0) ? 1 : (2 + (k - 1));
  PLUSS_CHECK(static_cast<std::int64_t>(theta.size()) == expected,
              "subsp::stats::ts::Garch: internal theta size mismatch");

  GarchParams p;
  p.arch.resize(static_cast<std::size_t>(arch_order));
  p.garch.resize(static_cast<std::size_t>(garch_order));
  p.omega = variance_floor + std::exp(theta[0]);

  if (k == 0) {
    return p;
  }

  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  const double persist = stable_sigmoid_(theta[1]) * cap;

  std::vector<double> weights;
  if (k == 1) {
    weights = {1.0};
  } else {
    weights = decode_weights_stick_(std::vector<double>(theta.begin() + 2, theta.end()));
  }

  std::int64_t idx = 0;
  for (std::int64_t i = 0; i < arch_order; ++i, ++idx) {
    p.arch[static_cast<std::size_t>(i)] = persist * weights[static_cast<std::size_t>(idx)];
  }
  for (std::int64_t j = 0; j < garch_order; ++j, ++idx) {
    p.garch[static_cast<std::size_t>(j)] = persist * weights[static_cast<std::size_t>(idx)];
  }
  return p;
}

std::vector<double> encode_theta_(GarchParams p,
                                  double variance_floor,
                                  double stationarity_margin) {
  p = clipped_params_(std::move(p), variance_floor, stationarity_margin);
  const std::int64_t k = coeff_count_(static_cast<std::int64_t>(p.arch.size()),
                                      static_cast<std::int64_t>(p.garch.size()));

  std::vector<double> out;
  out.reserve(static_cast<std::size_t>((k == 0) ? 1 : (2 + (k - 1))));
  out.push_back(std::log(std::max(p.omega - variance_floor, 1e-15)));

  if (k == 0) {
    return out;
  }

  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  const double persist = std::clamp(coeff_sum_(p), 1e-12, cap - 1e-12);
  out.push_back(stable_logit_(persist / cap));

  if (k > 1) {
    std::vector<double> weights;
    weights.reserve(static_cast<std::size_t>(k));
    for (double v : p.arch) {
      weights.push_back(v / persist);
    }
    for (double v : p.garch) {
      weights.push_back(v / persist);
    }
    const auto stick = encode_weights_stick_(weights);
    out.insert(out.end(), stick.begin(), stick.end());
  }
  return out;
}

std::vector<double> sigma2_path_(const std::vector<double>& e,
                                 const GarchParams& p,
                                 double variance_floor,
                                 double stationarity_margin) {
  const std::int64_t n = static_cast<std::int64_t>(e.size());
  std::vector<double> out(static_cast<std::size_t>(n), variance_floor);
  if (n == 0) {
    return out;
  }

  const double sum = coeff_sum_(p);
  const double var_uncond = (sum < 1.0 - stationarity_margin)
                                ? (p.omega / std::max(1e-15, 1.0 - sum))
                                : std::numeric_limits<double>::quiet_NaN();
  const double var0 = (std::isfinite(var_uncond) && var_uncond > variance_floor)
                          ? var_uncond
                          : std::max(sample_var_(e), variance_floor);

  const std::int64_t m = static_cast<std::int64_t>(p.arch.size());
  const std::int64_t q = static_cast<std::int64_t>(p.garch.size());

  for (std::int64_t t = 0; t < n; ++t) {
    double v = p.omega;
    for (std::int64_t i = 1; i <= m; ++i) {
      const std::int64_t idx = t - i;
      const double e2 = (idx >= 0) ? (e[static_cast<std::size_t>(idx)] * e[static_cast<std::size_t>(idx)]) : var0;
      v += p.arch[static_cast<std::size_t>(i - 1)] * e2;
    }
    for (std::int64_t j = 1; j <= q; ++j) {
      const std::int64_t idx = t - j;
      const double s = (idx >= 0) ? out[static_cast<std::size_t>(idx)] : var0;
      v += p.garch[static_cast<std::size_t>(j - 1)] * s;
    }
    out[static_cast<std::size_t>(t)] = (std::isfinite(v) && v > variance_floor) ? v : variance_floor;
  }
  return out;
}

std::vector<double> loglik_per_obs_centered_(const std::vector<double>& e,
                                             const GarchParams& p,
                                             double variance_floor,
                                             double stationarity_margin) {
  if (!valid_params_(p, variance_floor, stationarity_margin)) {
    return std::vector<double>(e.size(), -kHuge);
  }
  const auto sigma2 = sigma2_path_(e, p, variance_floor, stationarity_margin);
  std::vector<double> ll(e.size(), -kHuge);
  for (std::size_t t = 0; t < e.size(); ++t) {
    const double v = sigma2[t];
    const double et = e[t];
    const double term = -0.5 * (kLog2Pi + std::log(v) + (et * et) / v);
    ll[t] = std::isfinite(term) ? term : -kHuge;
  }
  return ll;
}

double loglik_centered_(const std::vector<double>& e,
                        const GarchParams& p,
                        double variance_floor,
                        double stationarity_margin) {
  const auto ll = loglik_per_obs_centered_(e, p, variance_floor, stationarity_margin);
  return std::accumulate(ll.begin(), ll.end(), 0.0);
}

double nll_centered_(const std::vector<double>& e,
                     const GarchParams& p,
                     double variance_floor,
                     double stationarity_margin) {
  const double ll = loglik_centered_(e, p, variance_floor, stationarity_margin);
  if (!std::isfinite(ll)) {
    return kHuge;
  }
  return -ll;
}

std::int64_t param_dim_(const GarchParams& p) {
  return 1 + static_cast<std::int64_t>(p.arch.size() + p.garch.size());
}

GarchParams perturb_param_(GarchParams p,
                           std::int64_t idx,
                           double delta,
                           double variance_floor,
                           double stationarity_margin) {
  if (idx == 0) {
    p.omega += delta;
  } else {
    const std::int64_t k_arch = static_cast<std::int64_t>(p.arch.size());
    const std::int64_t k = idx - 1;
    if (k < k_arch) {
      p.arch[static_cast<std::size_t>(k)] += delta;
    } else {
      p.garch[static_cast<std::size_t>(k - k_arch)] += delta;
    }
  }
  return clipped_params_(std::move(p), variance_floor, stationarity_margin);
}

std::vector<double> natural_param_vector_(const GarchParams& p) {
  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(param_dim_(p)));
  out.push_back(p.omega);
  out.insert(out.end(), p.arch.begin(), p.arch.end());
  out.insert(out.end(), p.garch.begin(), p.garch.end());
  return out;
}

Series make_f64_vector_(const std::vector<double>& values, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, out_cfg);
}

std::vector<double> score_numeric_(const std::vector<double>& e,
                                   const GarchParams& p,
                                   double variance_floor,
                                   double stationarity_margin,
                                   double finite_diff_eps) {
  const auto base = natural_param_vector_(p);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> g(static_cast<std::size_t>(d), 0.0);
  for (std::int64_t i = 0; i < d; ++i) {
    const double h = std::max(finite_diff_eps, finite_diff_eps * std::abs(base[static_cast<std::size_t>(i)]));
    const auto p_plus = perturb_param_(p, i, h, variance_floor, stationarity_margin);
    const auto p_minus = perturb_param_(p, i, -h, variance_floor, stationarity_margin);
    const double f_plus = loglik_centered_(e, p_plus, variance_floor, stationarity_margin);
    const double f_minus = loglik_centered_(e, p_minus, variance_floor, stationarity_margin);
    const double grad = (f_plus - f_minus) / (2.0 * h);
    g[static_cast<std::size_t>(i)] = std::isfinite(grad) ? grad : 0.0;
  }
  return g;
}

std::vector<double> observed_inf_numeric_(const std::vector<double>& e,
                                          const GarchParams& p,
                                          double variance_floor,
                                          double stationarity_margin,
                                          double finite_diff_eps) {
  const auto base = natural_param_vector_(p);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> out(static_cast<std::size_t>(d * d), 0.0);

  const auto step = [finite_diff_eps](double v) {
    return std::max(finite_diff_eps, finite_diff_eps * std::abs(v));
  };

  for (std::int64_t i = 0; i < d; ++i) {
    const double hi = step(base[static_cast<std::size_t>(i)]);
    for (std::int64_t j = 0; j < d; ++j) {
      const double hj = step(base[static_cast<std::size_t>(j)]);
      const auto pp = perturb_param_(perturb_param_(p, i, hi, variance_floor, stationarity_margin),
                                     j,
                                     hj,
                                     variance_floor,
                                     stationarity_margin);
      const auto pm = perturb_param_(perturb_param_(p, i, hi, variance_floor, stationarity_margin),
                                     j,
                                     -hj,
                                     variance_floor,
                                     stationarity_margin);
      const auto mp = perturb_param_(perturb_param_(p, i, -hi, variance_floor, stationarity_margin),
                                     j,
                                     hj,
                                     variance_floor,
                                     stationarity_margin);
      const auto mm = perturb_param_(perturb_param_(p, i, -hi, variance_floor, stationarity_margin),
                                     j,
                                     -hj,
                                     variance_floor,
                                     stationarity_margin);

      const double fpp = loglik_centered_(e, pp, variance_floor, stationarity_margin);
      const double fpm = loglik_centered_(e, pm, variance_floor, stationarity_margin);
      const double fmp = loglik_centered_(e, mp, variance_floor, stationarity_margin);
      const double fmm = loglik_centered_(e, mm, variance_floor, stationarity_margin);
      const double h = (fpp - fpm - fmp + fmm) / (4.0 * hi * hj);
      out[static_cast<std::size_t>(i * d + j)] = std::isfinite(h) ? -h : 0.0;
    }
  }
  return out;
}

std::vector<double> forecast_variance_(std::int64_t horizon,
                                       const std::vector<double>& e,
                                       const std::vector<double>& sigma2,
                                       const GarchParams& p,
                                       double variance_floor,
                                       double stationarity_margin) {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::GarchFit::forecast: horizon must be > 0");
  PLUSS_CHECK(!e.empty(), "subsp::stats::ts::GarchFit::forecast: expected non-empty response history");

  const std::int64_t n = static_cast<std::int64_t>(e.size());
  const double sum = coeff_sum_(p);
  const double uncond_candidate = (sum < 1.0 - stationarity_margin)
                                      ? (p.omega / std::max(1e-15, 1.0 - sum))
                                      : std::numeric_limits<double>::quiet_NaN();
  const double uncond = (std::isfinite(uncond_candidate) && uncond_candidate > variance_floor)
                            ? uncond_candidate
                            : std::max(sample_var_(e), variance_floor);

  std::vector<double> out(static_cast<std::size_t>(horizon), variance_floor);
  const std::int64_t m = static_cast<std::int64_t>(p.arch.size());
  const std::int64_t q = static_cast<std::int64_t>(p.garch.size());

  for (std::int64_t h = 1; h <= horizon; ++h) {
    double v = p.omega;

    for (std::int64_t i = 1; i <= m; ++i) {
      const std::int64_t idx = h - i;
      if (idx <= 0) {
        const std::int64_t obs = n + idx - 1;
        const double e2 = (obs >= 0) ? (e[static_cast<std::size_t>(obs)] * e[static_cast<std::size_t>(obs)]) : uncond;
        v += p.arch[static_cast<std::size_t>(i - 1)] * e2;
      } else {
        v += p.arch[static_cast<std::size_t>(i - 1)] * out[static_cast<std::size_t>(idx - 1)];
      }
    }

    for (std::int64_t j = 1; j <= q; ++j) {
      const std::int64_t idx = h - j;
      if (idx <= 0) {
        const std::int64_t obs = n + idx - 1;
        const double s = (obs >= 0) ? sigma2[static_cast<std::size_t>(obs)] : uncond;
        v += p.garch[static_cast<std::size_t>(j - 1)] * s;
      } else {
        v += p.garch[static_cast<std::size_t>(j - 1)] * out[static_cast<std::size_t>(idx - 1)];
      }
    }

    out[static_cast<std::size_t>(h - 1)] = (std::isfinite(v) && v > variance_floor) ? v : variance_floor;
  }
  return out;
}

std::vector<double> simulate_series_(std::int64_t n,
                                     const GarchFit& fit,
                                     Config cfg,
                                     std::uint64_t seed) {
  PLUSS_CHECK(n > 0, "subsp::stats::ts::GarchFit::simulate: num_observations must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const auto z = flat_f64_values_(pluss::randn({n}, out_cfg, seed));

  GarchParams p{fit.omega, fit.arch, fit.garch};
  const double sum = coeff_sum_(p);
  const double uncond_candidate = (sum < 1.0 - fit.stationarity_margin)
                                      ? (p.omega / std::max(1e-15, 1.0 - sum))
                                      : std::numeric_limits<double>::quiet_NaN();
  const double uncond = (std::isfinite(uncond_candidate) && uncond_candidate > fit.variance_floor)
                            ? uncond_candidate
                            : std::max(1.0, fit.variance_floor);

  std::vector<double> eps(static_cast<std::size_t>(n), 0.0);
  std::vector<double> sigma2(static_cast<std::size_t>(n), uncond);
  std::vector<double> y(static_cast<std::size_t>(n), fit.mean);

  const std::int64_t m = static_cast<std::int64_t>(p.arch.size());
  const std::int64_t q = static_cast<std::int64_t>(p.garch.size());

  for (std::int64_t t = 0; t < n; ++t) {
    double v = p.omega;
    for (std::int64_t i = 1; i <= m; ++i) {
      const std::int64_t idx = t - i;
      const double e2 = (idx >= 0) ? (eps[static_cast<std::size_t>(idx)] * eps[static_cast<std::size_t>(idx)]) : uncond;
      v += p.arch[static_cast<std::size_t>(i - 1)] * e2;
    }
    for (std::int64_t j = 1; j <= q; ++j) {
      const std::int64_t idx = t - j;
      const double s = (idx >= 0) ? sigma2[static_cast<std::size_t>(idx)] : uncond;
      v += p.garch[static_cast<std::size_t>(j - 1)] * s;
    }

    sigma2[static_cast<std::size_t>(t)] = (std::isfinite(v) && v > fit.variance_floor) ? v : fit.variance_floor;
    eps[static_cast<std::size_t>(t)] = std::sqrt(sigma2[static_cast<std::size_t>(t)]) * z[static_cast<std::size_t>(t)];
    y[static_cast<std::size_t>(t)] = fit.mean + eps[static_cast<std::size_t>(t)];
  }
  return y;
}

std::vector<double> normalize_init_coeffs_(const std::vector<double>& values,
                                           std::int64_t order,
                                           double default_value,
                                           const char* name) {
  PLUSS_CHECK(order >= 0, std::string("subsp::stats::ts::Garch::fit_mle: ") + name + " order must be >= 0");
  if (order == 0) {
    return {};
  }

  std::vector<double> out;
  if (values.empty()) {
    out.assign(static_cast<std::size_t>(order), default_value);
  } else if (static_cast<std::int64_t>(values.size()) == 1) {
    out.assign(static_cast<std::size_t>(order), values.front());
  } else if (static_cast<std::int64_t>(values.size()) == order) {
    out = values;
  } else {
    PLUSS_CHECK(false,
                std::string("subsp::stats::ts::Garch::fit_mle: ") + name +
                    " initial vector size must be 1 or equal to order");
  }

  PLUSS_CHECK(std::all_of(out.begin(), out.end(), [](double v) { return std::isfinite(v) && v >= 0.0; }),
              std::string("subsp::stats::ts::Garch::fit_mle: ") + name +
                  " initial coefficients must be finite and non-negative");
  return out;
}

std::vector<std::vector<double>> restart_thetas_(const std::vector<double>& base,
                                                 std::int64_t restarts,
                                                 double simplex_delta) {
  PLUSS_CHECK(restarts > 0, "subsp::stats::ts::Garch::fit_mle: nm_restarts must be > 0");
  std::vector<std::vector<double>> starts;
  starts.reserve(static_cast<std::size_t>(restarts));
  starts.push_back(base);
  for (std::int64_t r = 1; r < restarts; ++r) {
    const double amp = simplex_delta * (0.5 + 0.5 * static_cast<double>(r) / static_cast<double>(restarts - 1));
    std::vector<double> v = base;
    for (std::size_t i = 0; i < v.size(); ++i) {
      const double sign = (((static_cast<std::int64_t>(i) + r) % 2) == 0) ? 1.0 : -1.0;
      v[i] += sign * amp * std::max(1.0, std::abs(base[i]));
    }
    starts.push_back(std::move(v));
  }
  return starts;
}

GarchFit to_fit_(const GarchParams& p, double mean, const GarchOptions& options) {
  return GarchFit{
      .mean = mean,
      .omega = p.omega,
      .arch = p.arch,
      .garch = p.garch,
      .variance_floor = options.variance_floor,
      .stationarity_margin = options.stationarity_margin,
      .finite_diff_eps = options.finite_diff_eps,
  };
}

GarchFit fit_garch_(const Series& y, const GarchOptions& options, Config cfg) {
  PLUSS_CHECK(options.arch_order >= 0, "subsp::stats::ts::Garch::fit_mle: arch_order must be >= 0");
  PLUSS_CHECK(options.garch_order >= 0, "subsp::stats::ts::Garch::fit_mle: garch_order must be >= 0");
  PLUSS_CHECK(options.arch_order + options.garch_order >= 0,
              "subsp::stats::ts::Garch::fit_mle: invalid orders");
  PLUSS_CHECK(options.variance_floor > 0.0, "subsp::stats::ts::Garch::fit_mle: variance_floor must be > 0");
  PLUSS_CHECK(options.stationarity_margin > 0.0 && options.stationarity_margin < 1.0,
              "subsp::stats::ts::Garch::fit_mle: stationarity_margin must be in (0,1)");
  PLUSS_CHECK(options.finite_diff_eps > 0.0, "subsp::stats::ts::Garch::fit_mle: finite_diff_eps must be > 0");
  PLUSS_CHECK(options.max_iterations > 0, "subsp::stats::ts::Garch::fit_mle: max_iterations must be > 0");
  PLUSS_CHECK(options.max_evaluations > 0, "subsp::stats::ts::Garch::fit_mle: max_evaluations must be > 0");
  PLUSS_CHECK(options.nm_restarts > 0, "subsp::stats::ts::Garch::fit_mle: nm_restarts must be > 0");

  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "Garch::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const auto [centered, mean] = centered_(yv, options.demean);
  PLUSS_CHECK(centered.size() >= 8, "subsp::stats::ts::Garch::fit_mle: expected at least 8 observations");

  GarchParams init;
  init.omega = options.omega0;
  init.arch = normalize_init_coeffs_(options.arch0,
                                     options.arch_order,
                                     (options.arch_order > 0) ? (0.08 / static_cast<double>(options.arch_order)) : 0.0,
                                     "arch");
  init.garch = normalize_init_coeffs_(options.garch0,
                                      options.garch_order,
                                      (options.garch_order > 0) ? (0.90 / static_cast<double>(options.garch_order)) : 0.0,
                                      "garch");
  init = clipped_params_(std::move(init), options.variance_floor, options.stationarity_margin);

  const auto theta0 = encode_theta_(init, options.variance_floor, options.stationarity_margin);
  pluss::subsp::math::optimization::NelderMeadConfig nm;
  nm.max_iterations = options.max_iterations;
  nm.max_evaluations = options.max_evaluations;
  nm.simplex_delta = options.simplex_delta;
  nm.position_tolerance = 1e-7;
  nm.value_tolerance = 1e-7;

  const auto objective = [&](const Series& theta) -> double {
    const auto tv = flat_f64_values_(theta);
    const auto p = decode_theta_(tv,
                                 options.arch_order,
                                 options.garch_order,
                                 options.variance_floor,
                                 options.stationarity_margin);
    return nll_centered_(centered, p, options.variance_floor, options.stationarity_margin);
  };

  double best_nll = std::numeric_limits<double>::infinity();
  GarchParams best_p = init;
  for (const auto& start : restart_thetas_(theta0, options.nm_restarts, options.simplex_delta)) {
    const Series initial = pluss::make_series(start, {static_cast<std::int64_t>(start.size())}, out_cfg);
    const auto out = pluss::subsp::math::optimization::nelder_mead_minimizer(objective, initial, nm);
    const auto theta_best = flat_f64_values_(out.minimizer);
    const auto p = decode_theta_(theta_best,
                                 options.arch_order,
                                 options.garch_order,
                                 options.variance_floor,
                                 options.stationarity_margin);
    const double nll = nll_centered_(centered, p, options.variance_floor, options.stationarity_margin);
    if (std::isfinite(nll) && nll < best_nll) {
      best_nll = nll;
      best_p = p;
    }
  }
  PLUSS_CHECK(std::isfinite(best_nll), "subsp::stats::ts::Garch::fit_mle: optimizer failed to find finite solution");
  return to_fit_(best_p, mean, options);
}

GarchParams to_params_(const GarchFit& fit) {
  return GarchParams{fit.omega, fit.arch, fit.garch};
}

GarchFit to_generic_(const Garch11Fit& fit) {
  return GarchFit{
      .mean = fit.mean,
      .omega = fit.omega,
      .arch = {fit.alpha},
      .garch = {fit.beta},
      .variance_floor = fit.variance_floor,
      .stationarity_margin = fit.stationarity_margin,
      .finite_diff_eps = fit.finite_diff_eps,
  };
}

} // namespace

double GarchFit::persistence() const {
  const GarchParams p = to_params_(*this);
  return coeff_sum_(p);
}

double GarchFit::unconditional_var() const {
  const double den = 1.0 - persistence();
  PLUSS_CHECK(den > 0.0, "subsp::stats::ts::GarchFit::unconditional_var: persistence must be < 1");
  return omega / den;
}

Series GarchFit::infer(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "GarchFit::infer").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto sigma2 = sigma2_path_(centered, to_params_(*this), variance_floor, stationarity_margin);
  return make_f64_vector_(sigma2, out_cfg);
}

double GarchFit::loglik(const Series& y) const {
  const Series yv = as_vector_(y, "GarchFit::loglik").to(ScalarType::Float64);
  const auto centered = centered_with_mean_(yv, mean);
  return loglik_centered_(centered, to_params_(*this), variance_floor, stationarity_margin);
}

Series GarchFit::loglik_per_obs(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "GarchFit::loglik_per_obs").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto ll = loglik_per_obs_centered_(centered, to_params_(*this), variance_floor, stationarity_margin);
  return make_f64_vector_(ll, out_cfg);
}

Series GarchFit::score(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "GarchFit::score").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto g = score_numeric_(centered,
                                to_params_(*this),
                                variance_floor,
                                stationarity_margin,
                                finite_diff_eps);
  return make_f64_vector_(g, out_cfg);
}

Series GarchFit::observed_inf(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "GarchFit::observed_inf").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto h = observed_inf_numeric_(centered,
                                       to_params_(*this),
                                       variance_floor,
                                       stationarity_margin,
                                       finite_diff_eps);
  const std::int64_t d = static_cast<std::int64_t>(1 + arch.size() + garch.size());
  return pluss::make_series(h, {d, d}, out_cfg);
}

Series GarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "GarchFit::forecast").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto sigma2 = sigma2_path_(centered, to_params_(*this), variance_floor, stationarity_margin);
  const auto out = forecast_variance_(horizon,
                                      centered,
                                      sigma2,
                                      to_params_(*this),
                                      variance_floor,
                                      stationarity_margin);
  return make_f64_vector_(out, out_cfg);
}

Series GarchFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  const auto out_cfg = f64_cfg_(cfg);
  const auto y = simulate_series_(num_observations, *this, out_cfg, seed);
  return make_f64_vector_(y, out_cfg);
}

GarchFit Garch::fit_mle(const Series& y, const GarchOptions& options, Config cfg) {
  return fit_garch_(y, options, cfg);
}

double Garch11Fit::unconditional_var() const {
  return to_generic_(*this).unconditional_var();
}

Series Garch11Fit::infer(const Series& y, Config cfg) const {
  return to_generic_(*this).infer(y, cfg);
}

double Garch11Fit::loglik(const Series& y) const {
  return to_generic_(*this).loglik(y);
}

Series Garch11Fit::loglik_per_obs(const Series& y, Config cfg) const {
  return to_generic_(*this).loglik_per_obs(y, cfg);
}

Series Garch11Fit::score(const Series& y, Config cfg) const {
  return to_generic_(*this).score(y, cfg);
}

Series Garch11Fit::observed_inf(const Series& y, Config cfg) const {
  return to_generic_(*this).observed_inf(y, cfg);
}

Series Garch11Fit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  return to_generic_(*this).forecast(horizon, y, cfg);
}

Series Garch11Fit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  return to_generic_(*this).simulate(num_observations, cfg, seed);
}

Garch11Fit Garch11::fit_mle(const Series& y, const Garch11Options& options, Config cfg) {
  GarchOptions opt;
  opt.arch_order = 1;
  opt.garch_order = 1;
  opt.demean = options.demean;
  opt.omega0 = options.omega0;
  opt.arch0 = {options.alpha0};
  opt.garch0 = {options.beta0};
  opt.variance_floor = options.variance_floor;
  opt.stationarity_margin = options.stationarity_margin;
  opt.finite_diff_eps = options.finite_diff_eps;
  opt.max_iterations = options.max_iterations;
  opt.max_evaluations = options.max_evaluations;
  opt.simplex_delta = options.simplex_delta;
  opt.nm_restarts = options.nm_restarts;

  const auto fit = Garch::fit_mle(y, opt, cfg);
  PLUSS_CHECK(fit.arch.size() == 1 && fit.garch.size() == 1,
              "subsp::stats::ts::Garch11::fit_mle: internal parameter size mismatch");
  return Garch11Fit{
      .mean = fit.mean,
      .omega = fit.omega,
      .alpha = fit.arch[0],
      .beta = fit.garch[0],
      .variance_floor = fit.variance_floor,
      .stationarity_margin = fit.stationarity_margin,
      .finite_diff_eps = fit.finite_diff_eps,
  };
}

} // namespace pluss::subsp::stats::ts
