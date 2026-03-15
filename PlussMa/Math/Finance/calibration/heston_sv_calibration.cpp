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

using pricing::HestonIntegrationOptions;
using pricing::MonteCarloOptions;
using pricing::OptionType;
using volatility::HestonParams;
using volatility::SvParams;

constexpr std::size_t kHestonDim = 5;
constexpr std::size_t kSvDim = 5;

void validate_quotes_(const std::vector<OptionQuote>& quotes, const char* op) {
  PLUSS_CHECK(!quotes.empty(), std::string("subsp::math::finance::calibration::") + op + ": quotes must be non-empty");
  for (const auto& q : quotes) {
    PLUSS_CHECK(q.type == OptionType::Call || q.type == OptionType::Put,
                std::string("subsp::math::finance::calibration::") + op + ": quote option type unsupported");
    PLUSS_CHECK(std::isfinite(q.strike) && q.strike > 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": quote strike must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.maturity) && q.maturity > 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": quote maturity must be finite and > 0");
    PLUSS_CHECK(std::isfinite(q.market_price) && q.market_price >= 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": quote market_price must be finite and >= 0");
    PLUSS_CHECK(std::isfinite(q.weight) && q.weight >= 0.0,
                std::string("subsp::math::finance::calibration::") + op + ": quote weight must be finite and >= 0");
  }
}

void validate_market_inputs_(double spot, double rate, double dividend_yield, const char* op) {
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), std::string("subsp::math::finance::calibration::") + op + ": rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              std::string("subsp::math::finance::calibration::") + op + ": dividend_yield must be finite");
}

void validate_heston_bounds_(const HestonBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.kappa_min) && std::isfinite(b.kappa_max) && b.kappa_min < b.kappa_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid kappa bounds");
  PLUSS_CHECK(std::isfinite(b.theta_min) && std::isfinite(b.theta_max) && b.theta_min < b.theta_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid theta bounds");
  PLUSS_CHECK(std::isfinite(b.sigma_min) && std::isfinite(b.sigma_max) && b.sigma_min < b.sigma_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid sigma bounds");
  PLUSS_CHECK(std::isfinite(b.rho_min) && std::isfinite(b.rho_max) && b.rho_min < b.rho_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid rho bounds");
  PLUSS_CHECK(std::isfinite(b.v0_min) && std::isfinite(b.v0_max) && b.v0_min < b.v0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid v0 bounds");
  PLUSS_CHECK(b.rho_min > -1.0 && b.rho_max < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": rho bounds must be in (-1,1)");
  PLUSS_CHECK(b.kappa_min > 0.0 && b.theta_min > 0.0 && b.sigma_min > 0.0 && b.v0_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": positive lower bounds required");
}

void validate_sv_bounds_(const SvBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.logvar_mean_min) && std::isfinite(b.logvar_mean_max) && b.logvar_mean_min < b.logvar_mean_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid logvar_mean bounds");
  PLUSS_CHECK(std::isfinite(b.phi_min) && std::isfinite(b.phi_max) && b.phi_min < b.phi_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid phi bounds");
  PLUSS_CHECK(std::isfinite(b.vol_of_logvar_min) && std::isfinite(b.vol_of_logvar_max) && b.vol_of_logvar_min < b.vol_of_logvar_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid vol_of_logvar bounds");
  PLUSS_CHECK(std::isfinite(b.rho_min) && std::isfinite(b.rho_max) && b.rho_min < b.rho_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid rho bounds");
  PLUSS_CHECK(std::isfinite(b.logvar0_min) && std::isfinite(b.logvar0_max) && b.logvar0_min < b.logvar0_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid logvar0 bounds");
  PLUSS_CHECK(b.phi_min > -1.0 && b.phi_max < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": phi bounds must be in (-1,1)");
  PLUSS_CHECK(b.rho_min > -1.0 && b.rho_max < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": rho bounds must be in (-1,1)");
  PLUSS_CHECK(b.vol_of_logvar_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": vol_of_logvar lower bound must be > 0");
}

void validate_heston_options_(const HestonCalibOptions& opt, const char* op) {
  validate_heston_bounds_(opt.bounds, op);
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

void validate_sv_options_(const SvCalibOptions& opt, const char* op) {
  validate_sv_bounds_(opt.bounds, op);
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

std::array<double, kHestonDim> heston_lower_(const HestonBounds& b) {
  return {b.kappa_min, b.theta_min, b.sigma_min, b.rho_min, b.v0_min};
}

std::array<double, kHestonDim> heston_upper_(const HestonBounds& b) {
  return {b.kappa_max, b.theta_max, b.sigma_max, b.rho_max, b.v0_max};
}

std::array<double, kSvDim> sv_lower_(const SvBounds& b) {
  return {b.logvar_mean_min, b.phi_min, b.vol_of_logvar_min, b.rho_min, b.logvar0_min};
}

std::array<double, kSvDim> sv_upper_(const SvBounds& b) {
  return {b.logvar_mean_max, b.phi_max, b.vol_of_logvar_max, b.rho_max, b.logvar0_max};
}

std::array<double, kHestonDim> heston_to_vec_(const HestonParams& p) {
  return {p.kappa, p.theta, p.sigma, p.rho, p.v0};
}

std::array<double, kSvDim> sv_to_vec_(const SvParams& p) {
  return {p.logvar_mean, p.phi, p.vol_of_logvar, p.rho, p.logvar0};
}

HestonParams heston_from_vec_(const std::array<double, kHestonDim>& x, const HestonParams& fixed) {
  HestonParams p = fixed;
  p.kappa = x[0];
  p.theta = x[1];
  p.sigma = x[2];
  p.rho = x[3];
  p.v0 = x[4];
  return p;
}

SvParams sv_from_vec_(const std::array<double, kSvDim>& x, const SvParams& fixed) {
  SvParams p = fixed;
  p.logvar_mean = x[0];
  p.phi = x[1];
  p.vol_of_logvar = x[2];
  p.rho = x[3];
  p.logvar0 = x[4];
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

double heston_model_price_(const OptionQuote& q,
                           double spot,
                           double rate,
                           double dividend_yield,
                           const HestonParams& params,
                           const HestonCalibOptions& options,
                           pluss::Config cfg) {
  if (options.method == HestonCalibMethod::MonteCarlo) {
    return pricing::heston_price_mc(
        q.type, spot, q.strike, rate, dividend_yield, q.maturity, params, options.mc, cfg);
  }
  return pricing::heston_price(
      q.type, spot, q.strike, rate, dividend_yield, q.maturity, params, options.integration);
}

double heston_sse_impl_(const std::vector<OptionQuote>& quotes,
                        double spot,
                        double rate,
                        double dividend_yield,
                        const HestonParams& params,
                        const HestonCalibOptions& options,
                        pluss::Config cfg) {
  double sse = 0.0;
  for (const auto& q : quotes) {
    const double model_price = heston_model_price_(q, spot, rate, dividend_yield, params, options, cfg);
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    const double w = std::max(0.0, q.weight);
    const double e = model_price - q.market_price;
    sse += w * e * e;
  }
  return sse;
}

double sv_sse_impl_(const std::vector<OptionQuote>& quotes,
                    double spot,
                    double rate,
                    double dividend_yield,
                    const SvParams& params,
                    const SvCalibOptions& options,
                    pluss::Config cfg) {
  double sse = 0.0;
  for (const auto& q : quotes) {
    const double model_price = pricing::sv_price_mc(
        q.type, spot, q.strike, rate, dividend_yield, q.maturity, params, options.mc, cfg);
    if (!std::isfinite(model_price)) {
      return std::numeric_limits<double>::infinity();
    }
    const double w = std::max(0.0, q.weight);
    const double e = model_price - q.market_price;
    sse += w * e * e;
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

  converged = false;
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    bool improved = false;

    for (std::size_t j = 0; j < D; ++j) {
      for (double dir : {1.0, -1.0}) {
        std::array<double, D> cand = x;
        cand[j] = std::clamp(cand[j] + dir * step[j], lo[j], hi[j]);
        if (same_vec_(cand, x, 0.0)) {
          continue;
        }
        const double f = objective(cand);
        ++evaluations;
        if (f < best) {
          best = f;
          x = cand;
          improved = true;
        }
      }
    }

    if (!improved) {
      for (double& s : step) {
        s *= step_decay;
      }
    }

    if (max_step_(step) < tolerance) {
      converged = true;
      break;
    }
  }

  return {x, best};
}

double quote_weight_sum_(const std::vector<OptionQuote>& quotes) {
  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  return wsum;
}

} // namespace

double heston_sse(const std::vector<OptionQuote>& quotes,
                  double spot,
                  double rate,
                  double dividend_yield,
                  const HestonParams& params,
                  const HestonCalibOptions& options,
                  pluss::Config cfg) {
  validate_quotes_(quotes, "heston_sse");
  validate_market_inputs_(spot, rate, dividend_yield, "heston_sse");
  validate_heston_options_(options, "heston_sse");
  return heston_sse_impl_(quotes, spot, rate, dividend_yield, params, options, cfg);
}

HestonCalibResult calibrate_heston(const std::vector<OptionQuote>& quotes,
                                   double spot,
                                   double rate,
                                   double dividend_yield,
                                   const HestonParams& initial,
                                   const HestonCalibOptions& options,
                                   pluss::Config cfg) {
  validate_quotes_(quotes, "calibrate_heston");
  validate_market_inputs_(spot, rate, dividend_yield, "calibrate_heston");
  validate_heston_options_(options, "calibrate_heston");

  const auto lo = heston_lower_(options.bounds);
  const auto hi = heston_upper_(options.bounds);
  const auto x0 = project_(heston_to_vec_(initial), lo, hi);

  std::int64_t evals = 0;
  bool conv_local = false;

  auto objective = [&](const std::array<double, kHestonDim>& x) -> double {
    return heston_sse_impl_(quotes,
                            spot,
                            rate,
                            dividend_yield,
                            heston_from_vec_(x, initial),
                            options,
                            cfg);
  };

  auto [x_best, f_best] = pattern_search_(x0,
                                          lo,
                                          hi,
                                          options.max_iterations,
                                          options.step_fraction,
                                          options.step_decay,
                                          options.tolerance,
                                          objective,
                                          evals,
                                          conv_local);

  std::mt19937_64 rng(options.seed == 0 ? 0x4D657273656E6E65ULL : options.seed);
  bool any_converged = conv_local;

  for (std::int64_t s = 1; s < options.random_starts; ++s) {
    bool this_conv = false;
    const auto start = random_point_(rng, lo, hi);
    auto [x_cand, f_cand] = pattern_search_(start,
                                            lo,
                                            hi,
                                            options.max_iterations,
                                            options.step_fraction,
                                            options.step_decay,
                                            options.tolerance,
                                            objective,
                                            evals,
                                            this_conv);
    any_converged = any_converged || this_conv;
    if (f_cand < f_best) {
      f_best = f_cand;
      x_best = x_cand;
    }
  }

  const HestonParams p_best = heston_from_vec_(x_best, initial);
  return HestonCalibResult{
      .params = p_best,
      .objective = f_best,
      .rmse = weighted_rmse_(f_best, quote_weight_sum_(quotes), quotes.size()),
      .evaluations = evals,
      .converged = any_converged,
  };
}

double sv_sse(const std::vector<OptionQuote>& quotes,
              double spot,
              double rate,
              double dividend_yield,
              const SvParams& params,
              const SvCalibOptions& options,
              pluss::Config cfg) {
  validate_quotes_(quotes, "sv_sse");
  validate_market_inputs_(spot, rate, dividend_yield, "sv_sse");
  validate_sv_options_(options, "sv_sse");
  return sv_sse_impl_(quotes, spot, rate, dividend_yield, params, options, cfg);
}

SvCalibResult calibrate_sv(const std::vector<OptionQuote>& quotes,
                           double spot,
                           double rate,
                           double dividend_yield,
                           const SvParams& initial,
                           const SvCalibOptions& options,
                           pluss::Config cfg) {
  validate_quotes_(quotes, "calibrate_sv");
  validate_market_inputs_(spot, rate, dividend_yield, "calibrate_sv");
  validate_sv_options_(options, "calibrate_sv");

  const auto lo = sv_lower_(options.bounds);
  const auto hi = sv_upper_(options.bounds);
  const auto x0 = project_(sv_to_vec_(initial), lo, hi);

  std::int64_t evals = 0;
  bool conv_local = false;

  auto objective = [&](const std::array<double, kSvDim>& x) -> double {
    return sv_sse_impl_(quotes,
                        spot,
                        rate,
                        dividend_yield,
                        sv_from_vec_(x, initial),
                        options,
                        cfg);
  };

  auto [x_best, f_best] = pattern_search_(x0,
                                          lo,
                                          hi,
                                          options.max_iterations,
                                          options.step_fraction,
                                          options.step_decay,
                                          options.tolerance,
                                          objective,
                                          evals,
                                          conv_local);

  std::mt19937_64 rng(options.seed == 0 ? 0x53564D6F64656C21ULL : options.seed);
  bool any_converged = conv_local;

  for (std::int64_t s = 1; s < options.random_starts; ++s) {
    bool this_conv = false;
    const auto start = random_point_(rng, lo, hi);
    auto [x_cand, f_cand] = pattern_search_(start,
                                            lo,
                                            hi,
                                            options.max_iterations,
                                            options.step_fraction,
                                            options.step_decay,
                                            options.tolerance,
                                            objective,
                                            evals,
                                            this_conv);
    any_converged = any_converged || this_conv;
    if (f_cand < f_best) {
      f_best = f_cand;
      x_best = x_cand;
    }
  }

  const SvParams p_best = sv_from_vec_(x_best, initial);
  return SvCalibResult{
      .params = p_best,
      .objective = f_best,
      .rmse = weighted_rmse_(f_best, quote_weight_sum_(quotes), quotes.size()),
      .evaluations = evals,
      .converged = any_converged,
  };
}

} // namespace pluss::subsp::math::finance::calibration
