#include "pluss/subsp/math/finance/calibration.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::calibration {
namespace {

using rates::CirParams;
using rates::HjmParams;
using rates::HullWhiteParams;
using rates::VasicekParams;

constexpr std::size_t kRateDim = 4;
constexpr std::size_t kHullWhiteDim = 5;
constexpr std::size_t kHjmDim = 2;

void validate_quotes_(const std::vector<RateQuote>& quotes, const char* op) {
  PLUSS_CHECK(!quotes.empty(), std::string("subsp::math::finance::calibration::") + op + ": quotes must be non-empty");
  for (const auto& q : quotes) {
    PLUSS_CHECK(std::isfinite(q.maturity) && q.maturity > 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": maturity must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.zcb_price) && q.zcb_price > 0.0 && q.zcb_price <= 1.5,
                std::string("subsp::math::finance::calibration::") + op + ": zcb_price must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.weight) && q.weight >= 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": weight must be finite and >= 0");
  }
}

void validate_bounds_(const VasicekBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.kappa_min) && std::isfinite(b.kappa_max) && b.kappa_min < b.kappa_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid kappa bounds");
  PLUSS_CHECK(std::isfinite(b.theta_min) && std::isfinite(b.theta_max) && b.theta_min < b.theta_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid theta bounds");
  PLUSS_CHECK(std::isfinite(b.sigma_min) && std::isfinite(b.sigma_max) && b.sigma_min < b.sigma_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid sigma bounds");
  PLUSS_CHECK(std::isfinite(b.r0_min) && std::isfinite(b.r0_max) && b.r0_min < b.r0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid r0 bounds");
  PLUSS_CHECK(b.kappa_min > 0.0 && b.sigma_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": kappa_min and sigma_min must be > 0");
}

void validate_bounds_(const CirBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.kappa_min) && std::isfinite(b.kappa_max) && b.kappa_min < b.kappa_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid kappa bounds");
  PLUSS_CHECK(std::isfinite(b.theta_min) && std::isfinite(b.theta_max) && b.theta_min < b.theta_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid theta bounds");
  PLUSS_CHECK(std::isfinite(b.sigma_min) && std::isfinite(b.sigma_max) && b.sigma_min < b.sigma_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid sigma bounds");
  PLUSS_CHECK(std::isfinite(b.r0_min) && std::isfinite(b.r0_max) && b.r0_min < b.r0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid r0 bounds");
  PLUSS_CHECK(b.kappa_min > 0.0 && b.theta_min > 0.0 && b.sigma_min > 0.0 && b.r0_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": lower bounds must be positive");
}

void validate_bounds_(const HullWhiteBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.kappa_min) && std::isfinite(b.kappa_max) && b.kappa_min < b.kappa_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid kappa bounds");
  PLUSS_CHECK(std::isfinite(b.sigma_min) && std::isfinite(b.sigma_max) && b.sigma_min < b.sigma_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid sigma bounds");
  PLUSS_CHECK(std::isfinite(b.r0_min) && std::isfinite(b.r0_max) && b.r0_min < b.r0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid r0 bounds");
  PLUSS_CHECK(std::isfinite(b.theta0_min) && std::isfinite(b.theta0_max) && b.theta0_min < b.theta0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid theta0 bounds");
  PLUSS_CHECK(std::isfinite(b.theta_slope_min) && std::isfinite(b.theta_slope_max) && b.theta_slope_min < b.theta_slope_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid theta_slope bounds");
  PLUSS_CHECK(b.kappa_min > 0.0 && b.sigma_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": kappa_min and sigma_min must be > 0");
}

void validate_bounds_(const HjmBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.r0_min) && std::isfinite(b.r0_max) && b.r0_min < b.r0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid r0 bounds");
  PLUSS_CHECK(std::isfinite(b.vol_scale_min) && std::isfinite(b.vol_scale_max) && b.vol_scale_min < b.vol_scale_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid vol_scale bounds");
  PLUSS_CHECK(b.vol_scale_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": vol_scale_min must be > 0");
}

void validate_options_(const VasicekCalibOptions& opt, const char* op) {
  validate_bounds_(opt.bounds, op);
  PLUSS_CHECK(opt.random_starts >= 1,
              std::string("subsp::math::finance::calibration::") + op + ": random_starts must be >= 1");
  PLUSS_CHECK(opt.max_iterations > 0,
              std::string("subsp::math::finance::calibration::") + op + ": max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(opt.step_fraction) && opt.step_fraction > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_fraction must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.step_decay) && opt.step_decay > 0.0 && opt.step_decay < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_decay must be in (0,1)");
  PLUSS_CHECK(std::isfinite(opt.tolerance) && opt.tolerance > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": tolerance must be finite and > 0");
}

void validate_options_(const CirCalibOptions& opt, const char* op) {
  validate_bounds_(opt.bounds, op);
  PLUSS_CHECK(opt.random_starts >= 1,
              std::string("subsp::math::finance::calibration::") + op + ": random_starts must be >= 1");
  PLUSS_CHECK(opt.max_iterations > 0,
              std::string("subsp::math::finance::calibration::") + op + ": max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(opt.step_fraction) && opt.step_fraction > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_fraction must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.step_decay) && opt.step_decay > 0.0 && opt.step_decay < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_decay must be in (0,1)");
  PLUSS_CHECK(std::isfinite(opt.tolerance) && opt.tolerance > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": tolerance must be finite and > 0");
}

void validate_options_(const HullWhiteCalibOptions& opt, const char* op) {
  validate_bounds_(opt.bounds, op);
  PLUSS_CHECK(opt.random_starts >= 1,
              std::string("subsp::math::finance::calibration::") + op + ": random_starts must be >= 1");
  PLUSS_CHECK(opt.max_iterations > 0,
              std::string("subsp::math::finance::calibration::") + op + ": max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(opt.step_fraction) && opt.step_fraction > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_fraction must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.step_decay) && opt.step_decay > 0.0 && opt.step_decay < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_decay must be in (0,1)");
  PLUSS_CHECK(std::isfinite(opt.tolerance) && opt.tolerance > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": tolerance must be finite and > 0");
}

void validate_options_(const HjmCalibOptions& opt, const char* op) {
  validate_bounds_(opt.bounds, op);
  PLUSS_CHECK(opt.random_starts >= 1,
              std::string("subsp::math::finance::calibration::") + op + ": random_starts must be >= 1");
  PLUSS_CHECK(opt.max_iterations > 0,
              std::string("subsp::math::finance::calibration::") + op + ": max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(opt.step_fraction) && opt.step_fraction > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_fraction must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.step_decay) && opt.step_decay > 0.0 && opt.step_decay < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": step_decay must be in (0,1)");
  PLUSS_CHECK(std::isfinite(opt.tolerance) && opt.tolerance > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": tolerance must be finite and > 0");
  PLUSS_CHECK(opt.mc_paths > 0 && opt.mc_steps > 0,
              std::string("subsp::math::finance::calibration::") + op + ": mc_paths/mc_steps must be > 0");
}

std::array<double, kRateDim> lower_(const VasicekBounds& b) {
  return {b.kappa_min, b.theta_min, b.sigma_min, b.r0_min};
}

std::array<double, kRateDim> upper_(const VasicekBounds& b) {
  return {b.kappa_max, b.theta_max, b.sigma_max, b.r0_max};
}

std::array<double, kRateDim> lower_(const CirBounds& b) {
  return {b.kappa_min, b.theta_min, b.sigma_min, b.r0_min};
}

std::array<double, kRateDim> upper_(const CirBounds& b) {
  return {b.kappa_max, b.theta_max, b.sigma_max, b.r0_max};
}

std::array<double, kHullWhiteDim> lower_(const HullWhiteBounds& b) {
  return {b.kappa_min, b.sigma_min, b.r0_min, b.theta0_min, b.theta_slope_min};
}

std::array<double, kHullWhiteDim> upper_(const HullWhiteBounds& b) {
  return {b.kappa_max, b.sigma_max, b.r0_max, b.theta0_max, b.theta_slope_max};
}

std::array<double, kHjmDim> lower_(const HjmBounds& b) {
  return {b.r0_min, b.vol_scale_min};
}

std::array<double, kHjmDim> upper_(const HjmBounds& b) {
  return {b.r0_max, b.vol_scale_max};
}

std::array<double, kRateDim> to_vec_(const VasicekParams& p) {
  return {p.kappa, p.theta, p.sigma, p.r0};
}

std::array<double, kRateDim> to_vec_(const CirParams& p) {
  return {p.kappa, p.theta, p.sigma, p.r0};
}

std::array<double, kHullWhiteDim> to_vec_(const HullWhiteParams& p) {
  return {p.kappa, p.sigma, p.r0, p.theta0, p.theta_slope};
}

std::array<double, kHjmDim> to_vec_(const HjmParams& p) {
  return {p.r0, 1.0};
}

VasicekParams from_vec_(const std::array<double, kRateDim>& x, const VasicekParams& fixed) {
  VasicekParams p = fixed;
  p.kappa = x[0];
  p.theta = x[1];
  p.sigma = x[2];
  p.r0 = x[3];
  return p;
}

CirParams from_vec_(const std::array<double, kRateDim>& x, const CirParams& fixed) {
  CirParams p = fixed;
  p.kappa = x[0];
  p.theta = x[1];
  p.sigma = x[2];
  p.r0 = x[3];
  return p;
}

HullWhiteParams from_vec_(const std::array<double, kHullWhiteDim>& x, const HullWhiteParams& fixed) {
  HullWhiteParams p = fixed;
  p.kappa = x[0];
  p.sigma = x[1];
  p.r0 = x[2];
  p.theta0 = x[3];
  p.theta_slope = x[4];
  return p;
}

HjmParams from_vec_(const std::array<double, kHjmDim>& x, const HjmParams& fixed) {
  HjmParams p = fixed;
  p.r0 = x[0];
  const double scale = x[1];
  for (double& v : p.vol) {
    v *= scale;
  }
  return p;
}

template <std::size_t D>
std::array<double, D> project_(std::array<double, D> x,
                               const std::array<double, D>& lo,
                               const std::array<double, D>& hi) {
  for (std::size_t i = 0; i < D; ++i) {
    x[i] = std::clamp(x[i], lo[i], hi[i]);
  }
  return x;
}

template <std::size_t D>
bool same_vec_(const std::array<double, D>& a, const std::array<double, D>& b, double tol = 0.0) {
  for (std::size_t i = 0; i < D; ++i) {
    if (std::abs(a[i] - b[i]) > tol) {
      return false;
    }
  }
  return true;
}

template <std::size_t D>
double max_step_(const std::array<double, D>& step) {
  double m = 0.0;
  for (double v : step) {
    m = std::max(m, std::abs(v));
  }
  return m;
}

double weighted_rmse_(double sse, double wsum, std::size_t n) {
  if (n == 0) {
    return std::numeric_limits<double>::infinity();
  }
  const double denom = (wsum > 0.0) ? wsum : static_cast<double>(n);
  return std::sqrt(std::max(0.0, sse / denom));
}

double vasicek_sse_impl_(const std::vector<RateQuote>& quotes, const VasicekParams& params) {
  double sse = 0.0;
  for (const auto& q : quotes) {
    const double model_price = rates::vasicek_zcb(q.maturity, params.r0, params);
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    const double e = model_price - q.zcb_price;
    sse += std::max(0.0, q.weight) * e * e;
  }
  return sse;
}

double cir_sse_impl_(const std::vector<RateQuote>& quotes, const CirParams& params) {
  double sse = 0.0;
  for (const auto& q : quotes) {
    const double model_price = rates::cir_zcb(q.maturity, std::max(params.r0, params.rate_floor), params);
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    const double e = model_price - q.zcb_price;
    sse += std::max(0.0, q.weight) * e * e;
  }
  return sse;
}

double hull_white_sse_impl_(const std::vector<RateQuote>& quotes, const HullWhiteParams& params) {
  double sse = 0.0;
  for (const auto& q : quotes) {
    const double model_price = rates::hull_white_zcb(q.maturity, params.r0, params);
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    const double e = model_price - q.zcb_price;
    sse += std::max(0.0, q.weight) * e * e;
  }
  return sse;
}

double hjm_sse_impl_(const std::vector<RateQuote>& quotes, const HjmParams& params, const HjmCalibOptions& options) {
  const double maturity_tol = 1e-12;
  rates::SimOptions sim = options.sim_options;
  if (sim.seed == 0U) {
    sim.seed = options.seed;
  }

  std::vector<std::pair<double, double>> cache;
  cache.reserve(quotes.size());
  double sse = 0.0;
  for (const auto& q : quotes) {
    const auto it = std::find_if(cache.begin(), cache.end(), [&](const auto& m) {
      return std::abs(m.first - q.maturity) <= maturity_tol;
    });
    const double model_price = (it == cache.end())
                                   ? rates::hjm_zcb_mc(q.maturity, params, options.mc_paths, options.mc_steps, sim)
                                   : it->second;
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    if (it == cache.end()) {
      cache.emplace_back(q.maturity, model_price);
    }
    const double e = model_price - q.zcb_price;
    sse += std::max(0.0, q.weight) * e * e;
  }
  return sse;
}

template <std::size_t D>
std::array<double, D> random_point_(std::mt19937_64& rng,
                                    const std::array<double, D>& lo,
                                    const std::array<double, D>& hi) {
  std::array<double, D> x{};
  for (std::size_t i = 0; i < D; ++i) {
    std::uniform_real_distribution<double> dist(lo[i], hi[i]);
    x[i] = dist(rng);
  }
  return x;
}

template <std::size_t D, typename ObjectiveFn>
std::pair<std::array<double, D>, double> pattern_search_(const std::array<double, D>& start,
                                                          const std::array<double, D>& lo,
                                                          const std::array<double, D>& hi,
                                                          std::int64_t max_iterations,
                                                          double step_fraction,
                                                          double step_decay,
                                                          double tolerance,
                                                          ObjectiveFn&& objective,
                                                          std::int64_t& evaluations,
                                                          bool& converged) {
  std::array<double, D> x = project_(start, lo, hi);
  std::array<double, D> step{};
  for (std::size_t i = 0; i < D; ++i) {
    step[i] = std::max(tolerance * 10.0, (hi[i] - lo[i]) * step_fraction);
  }

  double best = objective(x);
  ++evaluations;

  for (std::int64_t it = 0; it < max_iterations; ++it) {
    bool improved = false;
    for (std::size_t d = 0; d < D; ++d) {
      std::array<double, D> plus = x;
      plus[d] = std::min(hi[d], x[d] + step[d]);
      if (!same_vec_(plus, x)) {
        const double val = objective(plus);
        ++evaluations;
        if (val < best) {
          best = val;
          x = plus;
          improved = true;
          continue;
        }
      }

      std::array<double, D> minus = x;
      minus[d] = std::max(lo[d], x[d] - step[d]);
      if (!same_vec_(minus, x)) {
        const double val = objective(minus);
        ++evaluations;
        if (val < best) {
          best = val;
          x = minus;
          improved = true;
        }
      }
    }

    if (!improved) {
      for (double& s : step) {
        s *= step_decay;
      }
      if (max_step_(step) <= tolerance) {
        converged = true;
        break;
      }
    }
  }

  if (max_step_(step) <= tolerance) {
    converged = true;
  }
  return {x, best};
}

} // namespace

double vasicek_sse(const std::vector<RateQuote>& quotes,
                   const VasicekParams& params,
                   const VasicekCalibOptions& options) {
  validate_quotes_(quotes, "vasicek_sse");
  validate_options_(options, "vasicek_sse");
  return vasicek_sse_impl_(quotes, params);
}

VasicekCalibResult calibrate_vasicek(const std::vector<RateQuote>& quotes,
                                     const VasicekParams& initial,
                                     const VasicekCalibOptions& options) {
  validate_quotes_(quotes, "calibrate_vasicek");
  validate_options_(options, "calibrate_vasicek");

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0xA915ULL : options.seed);
  VasicekCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kRateDim>& x) -> double {
    return vasicek_sse_impl_(quotes, from_vec_(x, initial));
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kRateDim> start = (start_idx == 0) ? x0 : random_point_(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;

    const auto [x_best, f_best] = pattern_search_(start,
                                                  lo,
                                                  hi,
                                                  options.max_iterations,
                                                  options.step_fraction,
                                                  options.step_decay,
                                                  options.tolerance,
                                                  objective,
                                                  eval_local,
                                                  converged_local);

    out.evaluations += eval_local;
    if (f_best < out.objective) {
      out.objective = f_best;
      out.params = from_vec_(x_best, initial);
      out.converged = converged_local;
    } else if (converged_local) {
      out.converged = true;
    }
  }

  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  out.rmse = weighted_rmse_(out.objective, wsum, quotes.size());
  return out;
}

double cir_sse(const std::vector<RateQuote>& quotes, const CirParams& params, const CirCalibOptions& options) {
  validate_quotes_(quotes, "cir_sse");
  validate_options_(options, "cir_sse");
  return cir_sse_impl_(quotes, params);
}

CirCalibResult calibrate_cir(const std::vector<RateQuote>& quotes,
                             const CirParams& initial,
                             const CirCalibOptions& options) {
  validate_quotes_(quotes, "calibrate_cir");
  validate_options_(options, "calibrate_cir");

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0xC1715ULL : options.seed);
  CirCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kRateDim>& x) -> double {
    return cir_sse_impl_(quotes, from_vec_(x, initial));
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kRateDim> start = (start_idx == 0) ? x0 : random_point_(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;

    const auto [x_best, f_best] = pattern_search_(start,
                                                  lo,
                                                  hi,
                                                  options.max_iterations,
                                                  options.step_fraction,
                                                  options.step_decay,
                                                  options.tolerance,
                                                  objective,
                                                  eval_local,
                                                  converged_local);

    out.evaluations += eval_local;
    if (f_best < out.objective) {
      out.objective = f_best;
      out.params = from_vec_(x_best, initial);
      out.converged = converged_local;
    } else if (converged_local) {
      out.converged = true;
    }
  }

  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  out.rmse = weighted_rmse_(out.objective, wsum, quotes.size());
  return out;
}

double hull_white_sse(const std::vector<RateQuote>& quotes,
                      const HullWhiteParams& params,
                      const HullWhiteCalibOptions& options) {
  validate_quotes_(quotes, "hull_white_sse");
  validate_options_(options, "hull_white_sse");
  return hull_white_sse_impl_(quotes, params);
}

HullWhiteCalibResult calibrate_hull_white(const std::vector<RateQuote>& quotes,
                                          const HullWhiteParams& initial,
                                          const HullWhiteCalibOptions& options) {
  validate_quotes_(quotes, "calibrate_hull_white");
  validate_options_(options, "calibrate_hull_white");

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0x11AA55ULL : options.seed);
  HullWhiteCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kHullWhiteDim>& x) -> double {
    return hull_white_sse_impl_(quotes, from_vec_(x, initial));
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kHullWhiteDim> start = (start_idx == 0) ? x0 : random_point_(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;
    const auto [x_best, f_best] = pattern_search_(start,
                                                  lo,
                                                  hi,
                                                  options.max_iterations,
                                                  options.step_fraction,
                                                  options.step_decay,
                                                  options.tolerance,
                                                  objective,
                                                  eval_local,
                                                  converged_local);
    out.evaluations += eval_local;
    if (f_best < out.objective) {
      out.objective = f_best;
      out.params = from_vec_(x_best, initial);
      out.converged = converged_local;
    } else if (converged_local) {
      out.converged = true;
    }
  }

  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  out.rmse = weighted_rmse_(out.objective, wsum, quotes.size());
  return out;
}

double hjm_sse(const std::vector<RateQuote>& quotes, const HjmParams& params, const HjmCalibOptions& options) {
  validate_quotes_(quotes, "hjm_sse");
  validate_options_(options, "hjm_sse");
  return hjm_sse_impl_(quotes, params, options);
}

HjmCalibResult calibrate_hjm(const std::vector<RateQuote>& quotes,
                             const HjmParams& initial,
                             const HjmCalibOptions& options) {
  validate_quotes_(quotes, "calibrate_hjm");
  validate_options_(options, "calibrate_hjm");

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0x66CC33ULL : options.seed);
  HjmCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kHjmDim>& x) -> double {
    return hjm_sse_impl_(quotes, from_vec_(x, initial), options);
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kHjmDim> start = (start_idx == 0) ? x0 : random_point_(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;
    const auto [x_best, f_best] = pattern_search_(start,
                                                  lo,
                                                  hi,
                                                  options.max_iterations,
                                                  options.step_fraction,
                                                  options.step_decay,
                                                  options.tolerance,
                                                  objective,
                                                  eval_local,
                                                  converged_local);
    out.evaluations += eval_local;
    if (f_best < out.objective) {
      out.objective = f_best;
      out.params = from_vec_(x_best, initial);
      out.converged = converged_local;
    } else if (converged_local) {
      out.converged = true;
    }
  }

  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  out.rmse = weighted_rmse_(out.objective, wsum, quotes.size());
  return out;
}

} // namespace pluss::subsp::math::finance::calibration
