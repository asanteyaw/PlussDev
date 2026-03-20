#include "pluss/subsp/math/finance/calibration.h"
#include "pluss/subsp/math/finance/calibration/detail/search_core.h"
#include "pluss/subsp/math/finance/calibration/detail/validation_core.h"

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
namespace caldetail = pluss::subsp::math::finance::calibration::detail;

constexpr std::size_t kBatesDim = 8;

void validate_quotes_(const std::vector<OptionQuote>& quotes, const char* op) {
  caldetail::validate_option_quotes(quotes, op);
}

void validate_market_inputs_(double spot, double rate, double dividend_yield, const char* op) {
  caldetail::validate_market_inputs(spot, rate, dividend_yield, op);
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
  caldetail::validate_search_controls(
      opt.random_starts, opt.max_iterations, opt.step_fraction, opt.step_decay, opt.tolerance, op);
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
  const auto x0 = caldetail::project(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0 ? 0xB157E5ULL : options.seed);

  BatesCalibResult out{};
  out.params = from_vec_(x0, initial);
  out.objective = std::numeric_limits<double>::infinity();

  auto objective = [&](const std::array<double, kBatesDim>& x) -> double {
    return sse_impl_(quotes, spot, rate, dividend_yield, from_vec_(x, initial), options, cfg);
  };

  for (std::int64_t start_idx = 0; start_idx < options.random_starts; ++start_idx) {
    std::array<double, kBatesDim> start = (start_idx == 0) ? x0 : caldetail::random_point(rng, lo, hi);
    bool converged_local = false;
    std::int64_t eval_local = 0;

    const auto [x_best, f_best] = caldetail::pattern_search(
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
  out.rmse = caldetail::weighted_rmse(out.objective, wsum, quotes.size());
  return out;
}

} // namespace pluss::subsp::math::finance::calibration
