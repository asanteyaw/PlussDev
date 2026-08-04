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
constexpr double kSqrt2OverPi = 0.79788456080286535588;
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

struct EgarchParams {
  double omega = -0.1;
  std::vector<double> arch{0.10};
  std::vector<double> asym{0.00};
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

std::pair<std::vector<double>, double> centered_(const Series& y, bool demean) {
  auto values = flat_f64_values_(y);
  PLUSS_CHECK(!values.empty(), "subsp::stats::ts::Egarch: expected non-empty response series");
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

double garch_sum_(const EgarchParams& p) {
  return std::accumulate(p.garch.begin(), p.garch.end(), 0.0);
}

EgarchParams clipped_params_(EgarchParams p, double stationarity_margin) {
  auto finite_or_zero = [](double& v) {
    if (!std::isfinite(v)) v = 0.0;
  };
  finite_or_zero(p.omega);
  std::for_each(p.arch.begin(), p.arch.end(), finite_or_zero);
  std::for_each(p.asym.begin(), p.asym.end(), finite_or_zero);
  std::for_each(p.garch.begin(), p.garch.end(), [](double& v) { v = (std::isfinite(v) && v > 0.0) ? v : 0.0; });

  const double cap = std::max(1e-9, 1.0 - stationarity_margin);
  const double s = garch_sum_(p);
  if (s >= cap) {
    const double scale = cap / std::max(s, 1e-15);
    std::for_each(p.garch.begin(), p.garch.end(), [scale](double& v) { v *= scale; });
  }
  return p;
}

bool valid_params_(const EgarchParams& p, double stationarity_margin) {
  const auto finite = [](double v) { return std::isfinite(v); };
  if (!finite(p.omega)) return false;
  if (!std::all_of(p.arch.begin(), p.arch.end(), finite)) return false;
  if (!std::all_of(p.asym.begin(), p.asym.end(), finite)) return false;
  if (!std::all_of(p.garch.begin(), p.garch.end(), [](double v) { return std::isfinite(v) && v >= 0.0; })) return false;
  return garch_sum_(p) < (1.0 - stationarity_margin);
}

std::vector<double> decode_stick_(const std::vector<double>& theta) {
  const std::int64_t k = static_cast<std::int64_t>(theta.size()) + 1;
  std::vector<double> w(static_cast<std::size_t>(k), 0.0);
  double rem = 1.0;
  for (std::int64_t i = 0; i < k - 1; ++i) {
    const double z = stable_sigmoid_(theta[static_cast<std::size_t>(i)]);
    w[static_cast<std::size_t>(i)] = rem * z;
    rem *= (1.0 - z);
  }
  w.back() = std::max(0.0, rem);
  return w;
}

std::vector<double> encode_stick_(const std::vector<double>& w) {
  if (w.size() <= 1) return {};
  std::vector<double> out(w.size() - 1, 0.0);
  double rem = 1.0;
  for (std::size_t i = 0; i + 1 < w.size(); ++i) {
    const double z = std::clamp(w[i] / std::max(rem, 1e-15), 1e-12, 1.0 - 1e-12);
    out[i] = stable_logit_(z);
    rem *= (1.0 - z);
  }
  return out;
}

EgarchParams decode_theta_(const std::vector<double>& theta,
                           std::int64_t m,
                           std::int64_t q,
                           double stationarity_margin) {
  const std::int64_t expected = 1 + 2 * m + q;
  PLUSS_CHECK(static_cast<std::int64_t>(theta.size()) == expected,
              "subsp::stats::ts::Egarch: internal theta size mismatch");

  EgarchParams p;
  p.arch.resize(static_cast<std::size_t>(m));
  p.asym.resize(static_cast<std::size_t>(m));
  p.garch.resize(static_cast<std::size_t>(q));

  std::int64_t idx = 0;
  p.omega = theta[static_cast<std::size_t>(idx++)];
  for (std::int64_t i = 0; i < m; ++i) p.arch[static_cast<std::size_t>(i)] = 2.0 * std::tanh(theta[static_cast<std::size_t>(idx++)]);
  for (std::int64_t i = 0; i < m; ++i) p.asym[static_cast<std::size_t>(i)] = 2.0 * std::tanh(theta[static_cast<std::size_t>(idx++)]);

  if (q > 0) {
    const double cap = std::max(1e-9, 1.0 - stationarity_margin);
    const double amp = stable_sigmoid_(theta[static_cast<std::size_t>(idx++)]) * cap;
    std::vector<double> w;
    if (q == 1) {
      w = {1.0};
    } else {
      w = decode_stick_(std::vector<double>(theta.begin() + idx, theta.end()));
    }
    for (std::int64_t i = 0; i < q; ++i) p.garch[static_cast<std::size_t>(i)] = amp * w[static_cast<std::size_t>(i)];
  }

  return clipped_params_(std::move(p), stationarity_margin);
}

std::vector<double> encode_theta_(EgarchParams p, double stationarity_margin) {
  p = clipped_params_(std::move(p), stationarity_margin);
  const std::int64_t m = static_cast<std::int64_t>(p.arch.size());
  const std::int64_t q = static_cast<std::int64_t>(p.garch.size());

  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(1 + 2 * m + q));
  out.push_back(p.omega);
  for (double v : p.arch) {
    const double u = std::clamp(v / 2.0, -0.999999, 0.999999);
    out.push_back(0.5 * std::log((1.0 + u) / (1.0 - u)));
  }
  for (double v : p.asym) {
    const double u = std::clamp(v / 2.0, -0.999999, 0.999999);
    out.push_back(0.5 * std::log((1.0 + u) / (1.0 - u)));
  }

  if (q > 0) {
    const double cap = std::max(1e-9, 1.0 - stationarity_margin);
    const double amp = std::clamp(garch_sum_(p), 1e-12, cap - 1e-12);
    out.push_back(stable_logit_(amp / cap));
    if (q > 1) {
      std::vector<double> w;
      w.reserve(static_cast<std::size_t>(q));
      for (double v : p.garch) w.push_back(v / amp);
      const auto stick = encode_stick_(w);
      out.insert(out.end(), stick.begin(), stick.end());
    }
  }

  return out;
}

std::vector<double> normalize_init_(const std::vector<double>& values,
                                    std::int64_t order,
                                    double default_value,
                                    const char* name) {
  PLUSS_CHECK(order >= 0, std::string("subsp::stats::ts::Egarch::fit_mle: ") + name + " order must be >= 0");
  if (order == 0) return {};
  if (values.empty()) return std::vector<double>(static_cast<std::size_t>(order), default_value);
  if (static_cast<std::int64_t>(values.size()) == 1) return std::vector<double>(static_cast<std::size_t>(order), values.front());
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == order,
              std::string("subsp::stats::ts::Egarch::fit_mle: ") + name +
                  " initial size must be 1 or equal to order");
  return values;
}

std::vector<std::vector<double>> restart_thetas_(const std::vector<double>& base,
                                                 std::int64_t restarts,
                                                 double simplex_delta) {
  PLUSS_CHECK(restarts > 0, "subsp::stats::ts::Egarch::fit_mle: nm_restarts must be > 0");
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

std::pair<std::vector<double>, std::vector<double>> sigma2_logh_path_(const std::vector<double>& e,
                                                                       const EgarchParams& p,
                                                                       double variance_floor,
                                                                       double stationarity_margin) {
  const std::int64_t n = static_cast<std::int64_t>(e.size());
  std::vector<double> sigma2(static_cast<std::size_t>(n), variance_floor);
  std::vector<double> logh(static_cast<std::size_t>(n), std::log(variance_floor));
  if (n == 0) {
    return {sigma2, logh};
  }

  const double bsum = garch_sum_(p);
  const double init_logh_candidate = (bsum < 1.0 - stationarity_margin)
                                         ? (p.omega / std::max(1e-15, 1.0 - bsum))
                                         : std::numeric_limits<double>::quiet_NaN();
  const double init_logh = std::isfinite(init_logh_candidate)
                               ? init_logh_candidate
                               : std::log(std::max(variance_floor,
                                                   std::accumulate(e.begin(), e.end(), 0.0, [](double acc, double v) { return acc + v * v; }) /
                                                       static_cast<double>(e.size())));

  const std::int64_t m = static_cast<std::int64_t>(p.arch.size());
  const std::int64_t q = static_cast<std::int64_t>(p.garch.size());

  for (std::int64_t t = 0; t < n; ++t) {
    double lt = p.omega;

    for (std::int64_t i = 1; i <= m; ++i) {
      const std::int64_t idx = t - i;
      double z = 0.0;
      if (idx >= 0) {
        z = e[static_cast<std::size_t>(idx)] / std::sqrt(std::max(sigma2[static_cast<std::size_t>(idx)], variance_floor));
      }
      lt += p.arch[static_cast<std::size_t>(i - 1)] * (std::abs(z) - kSqrt2OverPi);
      lt += p.asym[static_cast<std::size_t>(i - 1)] * z;
    }

    for (std::int64_t j = 1; j <= q; ++j) {
      const std::int64_t idx = t - j;
      const double lp = (idx >= 0) ? logh[static_cast<std::size_t>(idx)] : init_logh;
      lt += p.garch[static_cast<std::size_t>(j - 1)] * lp;
    }

    logh[static_cast<std::size_t>(t)] = lt;
    const double h = std::exp(std::clamp(lt, -100.0, 100.0));
    sigma2[static_cast<std::size_t>(t)] = (std::isfinite(h) && h > variance_floor) ? h : variance_floor;
  }

  return {sigma2, logh};
}

std::vector<double> loglik_per_obs_centered_(const std::vector<double>& e,
                                             const EgarchParams& p,
                                             double variance_floor,
                                             double stationarity_margin) {
  if (!valid_params_(p, stationarity_margin)) {
    return std::vector<double>(e.size(), -kHuge);
  }

  const auto [sigma2, _] = sigma2_logh_path_(e, p, variance_floor, stationarity_margin);
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
                        const EgarchParams& p,
                        double variance_floor,
                        double stationarity_margin) {
  const auto ll = loglik_per_obs_centered_(e, p, variance_floor, stationarity_margin);
  return std::accumulate(ll.begin(), ll.end(), 0.0);
}

double nll_centered_(const std::vector<double>& e,
                     const EgarchParams& p,
                     double variance_floor,
                     double stationarity_margin) {
  const double ll = loglik_centered_(e, p, variance_floor, stationarity_margin);
  if (!std::isfinite(ll)) return kHuge;
  return -ll;
}

std::vector<double> natural_params_(const EgarchFit& fit) {
  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(1 + fit.arch.size() + fit.asym.size() + fit.garch.size()));
  out.push_back(fit.omega);
  out.insert(out.end(), fit.arch.begin(), fit.arch.end());
  out.insert(out.end(), fit.asym.begin(), fit.asym.end());
  out.insert(out.end(), fit.garch.begin(), fit.garch.end());
  return out;
}

EgarchParams to_params_(const EgarchFit& fit) {
  return clipped_params_(EgarchParams{fit.omega, fit.arch, fit.asym, fit.garch}, fit.stationarity_margin);
}

EgarchParams perturb_param_(EgarchParams p,
                            std::int64_t idx,
                            double delta,
                            double stationarity_margin) {
  if (idx == 0) {
    p.omega += delta;
    return clipped_params_(std::move(p), stationarity_margin);
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
  if (apply(p.arch) || apply(p.asym) || apply(p.garch)) {
    return clipped_params_(std::move(p), stationarity_margin);
  }
  return clipped_params_(std::move(p), stationarity_margin);
}

std::vector<double> score_numeric_(const std::vector<double>& e, const EgarchFit& fit, double eps) {
  const auto base = natural_params_(fit);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> g(static_cast<std::size_t>(d), 0.0);
  const auto p0 = to_params_(fit);
  for (std::int64_t i = 0; i < d; ++i) {
    const double h = std::max(eps, eps * std::abs(base[static_cast<std::size_t>(i)]));
    const auto p_plus = perturb_param_(p0, i, h, fit.stationarity_margin);
    const auto p_minus = perturb_param_(p0, i, -h, fit.stationarity_margin);
    const double f_plus = loglik_centered_(e, p_plus, fit.variance_floor, fit.stationarity_margin);
    const double f_minus = loglik_centered_(e, p_minus, fit.variance_floor, fit.stationarity_margin);
    g[static_cast<std::size_t>(i)] = (f_plus - f_minus) / (2.0 * h);
  }
  return g;
}

std::vector<double> observed_inf_numeric_(const std::vector<double>& e, const EgarchFit& fit, double eps) {
  const auto base = natural_params_(fit);
  const std::int64_t d = static_cast<std::int64_t>(base.size());
  std::vector<double> out(static_cast<std::size_t>(d * d), 0.0);
  const auto p0 = to_params_(fit);
  const auto step = [eps](double v) { return std::max(eps, eps * std::abs(v)); };

  for (std::int64_t i = 0; i < d; ++i) {
    const double hi = step(base[static_cast<std::size_t>(i)]);
    for (std::int64_t j = 0; j < d; ++j) {
      const double hj = step(base[static_cast<std::size_t>(j)]);

      const auto pp = perturb_param_(perturb_param_(p0, i, hi, fit.stationarity_margin), j, hj, fit.stationarity_margin);
      const auto pm = perturb_param_(perturb_param_(p0, i, hi, fit.stationarity_margin), j, -hj, fit.stationarity_margin);
      const auto mp = perturb_param_(perturb_param_(p0, i, -hi, fit.stationarity_margin), j, hj, fit.stationarity_margin);
      const auto mm = perturb_param_(perturb_param_(p0, i, -hi, fit.stationarity_margin), j, -hj, fit.stationarity_margin);

      const double fpp = loglik_centered_(e, pp, fit.variance_floor, fit.stationarity_margin);
      const double fpm = loglik_centered_(e, pm, fit.variance_floor, fit.stationarity_margin);
      const double fmp = loglik_centered_(e, mp, fit.variance_floor, fit.stationarity_margin);
      const double fmm = loglik_centered_(e, mm, fit.variance_floor, fit.stationarity_margin);
      out[static_cast<std::size_t>(i * d + j)] = -(fpp - fpm - fmp + fmm) / (4.0 * hi * hj);
    }
  }
  return out;
}

EgarchFit fit_egarch_(const Series& y, const EgarchOptions& options, Config cfg) {
  PLUSS_CHECK(options.arch_order >= 0, "subsp::stats::ts::Egarch::fit_mle: arch_order must be >= 0");
  PLUSS_CHECK(options.garch_order >= 0, "subsp::stats::ts::Egarch::fit_mle: garch_order must be >= 0");
  PLUSS_CHECK(options.variance_floor > 0.0, "subsp::stats::ts::Egarch::fit_mle: variance_floor must be > 0");
  PLUSS_CHECK(options.stationarity_margin > 0.0 && options.stationarity_margin < 1.0,
              "subsp::stats::ts::Egarch::fit_mle: stationarity_margin must be in (0,1)");
  PLUSS_CHECK(options.finite_diff_eps > 0.0, "subsp::stats::ts::Egarch::fit_mle: finite_diff_eps must be > 0");
  PLUSS_CHECK(options.nm_restarts > 0, "subsp::stats::ts::Egarch::fit_mle: nm_restarts must be > 0");

  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "Egarch::fit_mle").to(out_cfg.platform, out_cfg.dtype);
  const auto [centered, mean] = centered_(yv, options.demean);
  PLUSS_CHECK(centered.size() >= 8, "subsp::stats::ts::Egarch::fit_mle: expected at least 8 observations");

  EgarchParams init;
  init.omega = options.omega0;
  init.arch = normalize_init_(options.arch0, options.arch_order, 0.10, "arch");
  init.asym = normalize_init_(options.asym0, options.arch_order, 0.00, "asym");
  init.garch = normalize_init_(options.garch0,
                               options.garch_order,
                               (options.garch_order > 0) ? (0.90 / static_cast<double>(options.garch_order)) : 0.0,
                               "garch");
  init = clipped_params_(std::move(init), options.stationarity_margin);

  const auto theta0 = encode_theta_(init, options.stationarity_margin);
  pluss::subsp::math::optimization::NelderMeadConfig nm;
  nm.max_iterations = options.max_iterations;
  nm.max_evaluations = options.max_evaluations;
  nm.simplex_delta = options.simplex_delta;
  nm.position_tolerance = 1e-7;
  nm.value_tolerance = 1e-7;

  const auto objective = [&](const Series& theta) -> double {
    const auto tv = flat_f64_values_(theta);
    const auto p = decode_theta_(tv, options.arch_order, options.garch_order, options.stationarity_margin);
    return nll_centered_(centered, p, options.variance_floor, options.stationarity_margin);
  };

  double best_nll = std::numeric_limits<double>::infinity();
  EgarchParams p = init;
  for (const auto& start : restart_thetas_(theta0, options.nm_restarts, options.simplex_delta)) {
    const Series initial = pluss::make_series(start, {static_cast<std::int64_t>(start.size())}, out_cfg);
    const auto out = pluss::subsp::math::optimization::nelder_mead_minimizer(objective, initial, nm);
    const auto theta_best = flat_f64_values_(out.minimizer);
    const auto cand = decode_theta_(theta_best, options.arch_order, options.garch_order, options.stationarity_margin);
    const double nll = nll_centered_(centered, cand, options.variance_floor, options.stationarity_margin);
    if (std::isfinite(nll) && nll < best_nll) {
      best_nll = nll;
      p = cand;
    }
  }
  PLUSS_CHECK(std::isfinite(best_nll), "subsp::stats::ts::Egarch::fit_mle: optimizer failed to find finite solution");

  return EgarchFit{
      .mean = mean,
      .omega = p.omega,
      .arch = p.arch,
      .asym = p.asym,
      .garch = p.garch,
      .variance_floor = options.variance_floor,
      .stationarity_margin = options.stationarity_margin,
      .finite_diff_eps = options.finite_diff_eps,
  };
}

} // namespace

double EgarchFit::persistence() const {
  return std::accumulate(garch.begin(), garch.end(), 0.0);
}

Series EgarchFit::infer(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "EgarchFit::infer").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto [sigma2, _] = sigma2_logh_path_(centered, to_params_(*this), variance_floor, stationarity_margin);
  return pluss::make_series(sigma2, {static_cast<std::int64_t>(sigma2.size())}, out_cfg);
}

double EgarchFit::loglik(const Series& y) const {
  const Series yv = as_vector_(y, "EgarchFit::loglik").to(ScalarType::Float64);
  const auto centered = centered_with_mean_(yv, mean);
  return loglik_centered_(centered, to_params_(*this), variance_floor, stationarity_margin);
}

Series EgarchFit::loglik_per_obs(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "EgarchFit::loglik_per_obs").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto ll = loglik_per_obs_centered_(centered, to_params_(*this), variance_floor, stationarity_margin);
  return pluss::make_series(ll, {static_cast<std::int64_t>(ll.size())}, out_cfg);
}

Series EgarchFit::score(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "EgarchFit::score").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto g = score_numeric_(centered, *this, finite_diff_eps);
  return pluss::make_series(g, {static_cast<std::int64_t>(g.size())}, out_cfg);
}

Series EgarchFit::observed_inf(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "EgarchFit::observed_inf").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto info = observed_inf_numeric_(centered, *this, finite_diff_eps);
  const std::int64_t d = static_cast<std::int64_t>(1 + arch.size() + asym.size() + garch.size());
  return pluss::make_series(info, {d, d}, out_cfg);
}

Series EgarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  PLUSS_CHECK(horizon > 0, "subsp::stats::ts::EgarchFit::forecast: horizon must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, "EgarchFit::forecast").to(out_cfg.platform, out_cfg.dtype);
  const auto centered = centered_with_mean_(yv, mean);
  const auto [sigma2_hist, logh_hist] = sigma2_logh_path_(centered, to_params_(*this), variance_floor, stationarity_margin);

  const auto p = to_params_(*this);
  const double bsum = garch_sum_(p);
  const double init_logh = (bsum < 1.0 - stationarity_margin)
                               ? (p.omega / std::max(1e-15, 1.0 - bsum))
                               : std::log(std::max(variance_floor,
                                                   std::accumulate(centered.begin(), centered.end(), 0.0, [](double a, double v) { return a + v * v; }) /
                                                       static_cast<double>(centered.size())));

  std::vector<double> out(static_cast<std::size_t>(horizon), variance_floor);
  for (std::int64_t h = 1; h <= horizon; ++h) {
    double lt = p.omega;
    for (std::int64_t j = 1; j <= static_cast<std::int64_t>(p.garch.size()); ++j) {
      const std::int64_t idx = h - j;
      if (idx <= 0) {
        const std::int64_t obs = static_cast<std::int64_t>(logh_hist.size()) + idx - 1;
        const double lprev = (obs >= 0) ? logh_hist[static_cast<std::size_t>(obs)] : init_logh;
        lt += p.garch[static_cast<std::size_t>(j - 1)] * lprev;
      } else {
        lt += p.garch[static_cast<std::size_t>(j - 1)] * std::log(out[static_cast<std::size_t>(idx - 1)]);
      }
    }
    out[static_cast<std::size_t>(h - 1)] = std::max(std::exp(std::clamp(lt, -100.0, 100.0)), variance_floor);
  }

  return pluss::make_series(out, {horizon}, out_cfg);
}

Series EgarchFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  PLUSS_CHECK(num_observations > 0, "subsp::stats::ts::EgarchFit::simulate: num_observations must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const auto z = flat_f64_values_(pluss::series::randn({num_observations}, out_cfg, seed));

  const auto p = to_params_(*this);
  const double bsum = garch_sum_(p);
  const double init_logh = (bsum < 1.0 - stationarity_margin) ? (p.omega / std::max(1e-15, 1.0 - bsum)) : std::log(variance_floor);

  std::vector<double> y(static_cast<std::size_t>(num_observations), mean);
  std::vector<double> e(static_cast<std::size_t>(num_observations), 0.0);
  std::vector<double> h(static_cast<std::size_t>(num_observations), variance_floor);
  std::vector<double> logh(static_cast<std::size_t>(num_observations), init_logh);

  for (std::int64_t t = 0; t < num_observations; ++t) {
    double lt = p.omega;
    for (std::int64_t i = 1; i <= static_cast<std::int64_t>(p.arch.size()); ++i) {
      const std::int64_t idx = t - i;
      double zi = 0.0;
      if (idx >= 0) {
        zi = e[static_cast<std::size_t>(idx)] / std::sqrt(std::max(h[static_cast<std::size_t>(idx)], variance_floor));
      }
      lt += p.arch[static_cast<std::size_t>(i - 1)] * (std::abs(zi) - kSqrt2OverPi);
      lt += p.asym[static_cast<std::size_t>(i - 1)] * zi;
    }
    for (std::int64_t j = 1; j <= static_cast<std::int64_t>(p.garch.size()); ++j) {
      const std::int64_t idx = t - j;
      const double lp = (idx >= 0) ? logh[static_cast<std::size_t>(idx)] : init_logh;
      lt += p.garch[static_cast<std::size_t>(j - 1)] * lp;
    }

    logh[static_cast<std::size_t>(t)] = lt;
    h[static_cast<std::size_t>(t)] = std::max(std::exp(std::clamp(lt, -100.0, 100.0)), variance_floor);
    e[static_cast<std::size_t>(t)] = std::sqrt(h[static_cast<std::size_t>(t)]) * z[static_cast<std::size_t>(t)];
    y[static_cast<std::size_t>(t)] = mean + e[static_cast<std::size_t>(t)];
  }

  return pluss::make_series(y, {num_observations}, out_cfg);
}

EgarchFit Egarch::fit_mle(const Series& y, const EgarchOptions& options, Config cfg) {
  return fit_egarch_(y, options, cfg);
}

} // namespace pluss::subsp::stats::ts
