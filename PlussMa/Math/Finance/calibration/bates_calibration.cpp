#include "pluss/subsp/math/finance/calibration.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

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

using pricing::OptionType;
using volatility::BatesParams;

constexpr std::size_t kBatesDim = 8;

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

void validate_bounds_(const BatesBounds& b, const char* op) {
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
  PLUSS_CHECK(std::isfinite(b.jump_intensity_min) && std::isfinite(b.jump_intensity_max) &&
                  b.jump_intensity_min < b.jump_intensity_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid jump_intensity bounds");
  PLUSS_CHECK(std::isfinite(b.jump_mean_min) && std::isfinite(b.jump_mean_max) && b.jump_mean_min < b.jump_mean_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid jump_mean bounds");
  PLUSS_CHECK(std::isfinite(b.jump_vol_min) && std::isfinite(b.jump_vol_max) && b.jump_vol_min < b.jump_vol_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid jump_vol bounds");
  PLUSS_CHECK(b.kappa_min > 0.0 && b.theta_min > 0.0 && b.sigma_min > 0.0 && b.v0_min > 0.0 &&
                  b.jump_intensity_min >= 0.0 && b.jump_vol_min > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": invalid positive lower bounds");
  PLUSS_CHECK(b.rho_min > -1.0 && b.rho_max < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": rho bounds must be in (-1,1)");
}

void validate_options_(const BatesCalibOptions& opt, const char* op) {
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

std::array<double, kBatesDim> lower_(const BatesBounds& b) {
  return {b.kappa_min, b.theta_min, b.sigma_min, b.rho_min, b.v0_min, b.jump_intensity_min, b.jump_mean_min, b.jump_vol_min};
}

std::array<double, kBatesDim> upper_(const BatesBounds& b) {
  return {b.kappa_max, b.theta_max, b.sigma_max, b.rho_max, b.v0_max, b.jump_intensity_max, b.jump_mean_max, b.jump_vol_max};
}

std::array<double, kBatesDim> to_vec_(const BatesParams& p) {
  return {p.kappa, p.theta, p.sigma, p.rho, p.v0, p.jump_intensity, p.jump_mean, p.jump_vol};
}

BatesParams from_vec_(const std::array<double, kBatesDim>& x, const BatesParams& fixed) {
  BatesParams p = fixed;
  p.kappa = x[0];
  p.theta = x[1];
  p.sigma = x[2];
  p.rho = x[3];
  p.v0 = x[4];
  p.jump_intensity = x[5];
  p.jump_mean = x[6];
  p.jump_vol = x[7];
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

double sse_impl_(const std::vector<OptionQuote>& quotes,
                 double spot,
                 double rate,
                 double dividend_yield,
                 const BatesParams& params,
                 const BatesCalibOptions& options,
                 pluss::Config cfg) {
  if (quotes.empty()) {
    return 0.0;
  }

  std::vector<std::size_t> order(quotes.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return quotes[a].maturity < quotes[b].maturity;
  });

  volatility::BatesParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.mc.scheme;
  sim_opt.method = options.mc.rand_method;
  sim_opt.randomized_quasi = options.mc.randomized_quasi;
  sim_opt.sobol_scrambling = options.mc.sobol_scrambling;
  sim_opt.seed = options.mc.seed;
  sim_opt.offset = options.mc.offset;

  const auto out_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform);
  double sse = 0.0;
  std::size_t g = 0;

  while (g < order.size()) {
    const double maturity = quotes[order[g]].maturity;
    std::size_t g_end = g + 1;
    while (g_end < order.size() && std::abs(quotes[order[g_end]].maturity - maturity) <= 1e-12) {
      ++g_end;
    }

    const std::int64_t k = static_cast<std::int64_t>(g_end - g);
    std::vector<double> strikes;
    std::vector<double> market;
    std::vector<double> weights;
    std::vector<double> is_call;
    strikes.reserve(static_cast<std::size_t>(k));
    market.reserve(static_cast<std::size_t>(k));
    weights.reserve(static_cast<std::size_t>(k));
    is_call.reserve(static_cast<std::size_t>(k));
    for (std::size_t j = g; j < g_end; ++j) {
      const auto& q = quotes[order[j]];
      strikes.push_back(q.strike);
      market.push_back(q.market_price);
      weights.push_back(std::max(0.0, q.weight));
      is_call.push_back(q.type == OptionType::Call ? 1.0 : 0.0);
    }

    const double dt = maturity / static_cast<double>(options.mc.steps);
    const auto sim =
        volatility::bates_paths(options.mc.steps, options.mc.paths, spot, dt, q_params, sim_opt, out_cfg, options.mc.seed + static_cast<std::uint64_t>(g));
    const pluss::Series terminal = sim.spot.select(0, options.mc.steps).reshape({options.mc.paths});

    const pluss::Series strike_row = pluss::make_series(strikes, {1, k}, out_cfg);
    const pluss::Series terminal_col = terminal.reshape({options.mc.paths, 1});
    const pluss::Series call_payoff = terminal_col.sub(strike_row).clamp_min(0.0).mean(0, false).reshape({k});
    const pluss::Series put_payoff = strike_row.sub(terminal_col).clamp_min(0.0).mean(0, false).reshape({k});

    const pluss::Series mask = pluss::make_series(is_call, {k}, out_cfg);
    const pluss::Series market_s = pluss::make_series(market, {k}, out_cfg);
    const pluss::Series weight_s = pluss::make_series(weights, {k}, out_cfg);
    const pluss::Series model_s = call_payoff.mul(mask).add(put_payoff.mul(mask.mul(-1.0).add(1.0)));
    sse += model_s.sub(market_s).square().mul(weight_s).sum().item<double>();

    if (!std::isfinite(sse)) {
      return std::numeric_limits<double>::infinity();
    }
    g = g_end;
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

double bates_sse(const std::vector<OptionQuote>& quotes,
                 double spot,
                 double rate,
                 double dividend_yield,
                 const BatesParams& params,
                 const BatesCalibOptions& options,
                 pluss::Config cfg) {
  validate_quotes_(quotes, "bates_sse");
  validate_market_inputs_(spot, rate, dividend_yield, "bates_sse");
  validate_options_(options, "bates_sse");
  return sse_impl_(quotes, spot, rate, dividend_yield, params, options, cfg);
}

BatesCalibResult calibrate_bates(const std::vector<OptionQuote>& quotes,
                                 double spot,
                                 double rate,
                                 double dividend_yield,
                                 const BatesParams& initial,
                                 const BatesCalibOptions& options,
                                 pluss::Config cfg) {
  validate_quotes_(quotes, "calibrate_bates");
  validate_market_inputs_(spot, rate, dividend_yield, "calibrate_bates");
  validate_options_(options, "calibrate_bates");

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0xB157E5ULL : options.seed);

  BatesCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kBatesDim>& x) -> double {
    return sse_impl_(quotes, spot, rate, dividend_yield, from_vec_(x, initial), options, cfg);
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kBatesDim> start = (start_idx == 0) ? x0 : random_point_(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;

    const auto [x_best, f_best] = pattern_search_(
        start,
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
