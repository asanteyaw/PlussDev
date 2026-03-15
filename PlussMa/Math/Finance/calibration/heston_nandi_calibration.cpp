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

using pluss::Config;
using pluss::Series;
using pricing::OptionType;
using volatility::HestonNandiParams;

constexpr std::size_t kHnDim = 5;
constexpr double kLog2Pi = 1.83787706640934533908;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::calibration::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::math::finance::calibration::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::math::finance::calibration::") + op + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

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

void validate_bounds_(const HestonNandiBounds& b, const char* op) {
  PLUSS_CHECK(std::isfinite(b.omega_min) && std::isfinite(b.omega_max) && b.omega_min < b.omega_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid omega bounds");
  PLUSS_CHECK(std::isfinite(b.alpha_min) && std::isfinite(b.alpha_max) && b.alpha_min < b.alpha_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid alpha bounds");
  PLUSS_CHECK(std::isfinite(b.beta_min) && std::isfinite(b.beta_max) && b.beta_min < b.beta_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid beta bounds");
  PLUSS_CHECK(std::isfinite(b.gamma_min) && std::isfinite(b.gamma_max) && b.gamma_min < b.gamma_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid gamma bounds");
  PLUSS_CHECK(std::isfinite(b.lambda_min) && std::isfinite(b.lambda_max) && b.lambda_min < b.lambda_max,
              std::string("subsp::math::finance::calibration::") + op + ": invalid lambda bounds");
  PLUSS_CHECK(b.omega_min > 0.0 && b.alpha_min >= 0.0 && b.beta_min >= 0.0,
              std::string("subsp::math::finance::calibration::") + op +
                  ": omega_min must be > 0 and alpha_min/beta_min must be >= 0");
}

void validate_mc_(const pricing::MonteCarloOptions& opt, const char* op) {
  PLUSS_CHECK(opt.paths > 0, std::string("subsp::math::finance::calibration::") + op + ": mc.paths must be > 0");
  PLUSS_CHECK(opt.steps > 0, std::string("subsp::math::finance::calibration::") + op + ": mc.steps must be > 0");
  PLUSS_CHECK(std::isfinite(opt.importance_shift),
              std::string("subsp::math::finance::calibration::") + op + ": mc.importance_shift must be finite");
}

void validate_params_(const HestonNandiParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.omega) && p.omega > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": omega must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha >= 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": alpha must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.beta) && p.beta >= 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": beta must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.gamma), std::string("subsp::math::finance::calibration::") + op + ": gamma must be finite");
  PLUSS_CHECK(std::isfinite(p.lambda), std::string("subsp::math::finance::calibration::") + op + ": lambda must be finite");
  PLUSS_CHECK(std::isfinite(p.r), std::string("subsp::math::finance::calibration::") + op + ": r must be finite");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": variance_floor must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.stationarity_margin) && p.stationarity_margin > 0.0 && p.stationarity_margin < 1.0,
              std::string("subsp::math::finance::calibration::") + op + ": stationarity_margin must be in (0,1)");
  PLUSS_CHECK(!std::isfinite(p.initial_variance) || p.initial_variance > 0.0,
              std::string("subsp::math::finance::calibration::") + op +
                  ": initial_variance must be NaN or finite and > 0");
  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  PLUSS_CHECK(persistence < (1.0 - p.stationarity_margin),
              std::string("subsp::math::finance::calibration::") + op +
                  ": requires beta + alpha*gamma^2 < 1 - stationarity_margin");
}

void validate_options_(const HestonNandiCalibOptions& opt, const char* op) {
  validate_bounds_(opt.bounds, op);
  validate_mc_(opt.mc, op);
  if (opt.use_closed_form) {
    PLUSS_CHECK(opt.integration.integration_steps > 0,
                std::string("subsp::math::finance::calibration::") + op +
                    ": integration.integration_steps must be > 0");
    PLUSS_CHECK(std::isfinite(opt.integration.integration_u_max) && opt.integration.integration_u_max > 0.0,
                std::string("subsp::math::finance::calibration::") + op +
                    ": integration.integration_u_max must be finite and > 0");
    PLUSS_CHECK(opt.integration.periods_per_year > 0,
                std::string("subsp::math::finance::calibration::") + op +
                    ": integration.periods_per_year must be > 0");
  }
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
  PLUSS_CHECK(std::isfinite(opt.returns_weight) && opt.returns_weight >= 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": returns_weight must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(opt.options_weight) && opt.options_weight >= 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": options_weight must be finite and >= 0");
  PLUSS_CHECK((opt.returns_weight + opt.options_weight) > 0.0,
              std::string("subsp::math::finance::calibration::") + op + ": at least one objective weight must be > 0");
}

std::array<double, kHnDim> lower_(const HestonNandiBounds& b) {
  return {b.omega_min, b.alpha_min, b.beta_min, b.gamma_min, b.lambda_min};
}

std::array<double, kHnDim> upper_(const HestonNandiBounds& b) {
  return {b.omega_max, b.alpha_max, b.beta_max, b.gamma_max, b.lambda_max};
}

std::array<double, kHnDim> to_vec_(const HestonNandiParams& p) {
  return {p.omega, p.alpha, p.beta, p.gamma, p.lambda};
}

HestonNandiParams from_vec_(const std::array<double, kHnDim>& x, const HestonNandiParams& fixed) {
  HestonNandiParams p = fixed;
  p.omega = x[0];
  p.alpha = x[1];
  p.beta = x[2];
  p.gamma = x[3];
  p.lambda = x[4];
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

double option_weight_sum_(const std::vector<OptionQuote>& quotes) {
  double wsum = 0.0;
  for (const auto& q : quotes) {
    wsum += std::max(0.0, q.weight);
  }
  return wsum;
}

double unconditional_variance_(const HestonNandiParams& p) {
  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  const double denom = 1.0 - persistence;
  if (denom <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return (p.omega + p.alpha) / denom;
}

double initial_variance_(const HestonNandiParams& p) {
  if (std::isfinite(p.initial_variance)) {
    return std::max(p.initial_variance, p.variance_floor);
  }
  const double unc = unconditional_variance_(p);
  if (!std::isfinite(unc) || unc <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return std::max(unc, p.variance_floor);
}

double returns_nll_impl_(const Series& returns, const HestonNandiParams& params, Config cfg) {
  validate_params_(params, "heston_nandi_returns_nll");
  const Series y = as_vector_(returns, "heston_nandi_returns_nll").to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype);
  const std::int64_t n = y.numel();
  PLUSS_CHECK(n > 0, "subsp::math::finance::calibration::heston_nandi_returns_nll: returns must be non-empty");

  const std::vector<double> rv = y.to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  double h = initial_variance_(params);
  if (!std::isfinite(h) || h <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }

  double nll = 0.0;
  for (std::int64_t t = 0; t < n; ++t) {
    const double h_safe = std::max(h, params.variance_floor);
    const double sig = std::sqrt(h_safe);
    const double mu = params.r + params.lambda * h_safe;
    const double z = (rv[static_cast<std::size_t>(t)] - mu) / sig;
    const double ll = -0.5 * (kLog2Pi + std::log(h_safe) + z * z);
    nll -= ll;

    h = params.omega + params.beta * h_safe + params.alpha * (z - params.gamma * sig) * (z - params.gamma * sig);
    h = std::max(h, params.variance_floor);
  }

  if (!std::isfinite(nll)) {
    return std::numeric_limits<double>::infinity();
  }
  return nll;
}

double options_sse_impl_(const std::vector<OptionQuote>& quotes,
                         double spot,
                         double rate,
                         double dividend_yield,
                         const HestonNandiParams& params,
                         const HestonNandiCalibOptions& options,
                         Config cfg) {
  validate_params_(params, "heston_nandi_sse");
  if (quotes.empty()) {
    return 0.0;
  }

  if (options.use_closed_form) {
    double sse = 0.0;
    for (const auto& q : quotes) {
      const double w = std::max(0.0, q.weight);
      const double model =
          pricing::heston_nandi_price(q.type, spot, q.strike, rate, dividend_yield, q.maturity, params, options.integration);
      const double err = model - q.market_price;
      sse += w * err * err;
      if (!std::isfinite(sse)) {
        return std::numeric_limits<double>::infinity();
      }
    }
    return sse;
  }

  std::vector<std::size_t> order(quotes.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return quotes[a].maturity < quotes[b].maturity;
  });

  const auto out_cfg = f64_cfg_(cfg);
  volatility::HestonNandiParams q_params = params;
  q_params.lambda = -0.5;
  q_params.gamma = params.gamma + params.lambda + 0.5;
  q_params.r = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.mc.scheme;
  sim_opt.method = options.mc.rand_method;
  sim_opt.randomized_quasi = options.mc.randomized_quasi;
  sim_opt.sobol_scrambling = options.mc.sobol_scrambling;
  sim_opt.antithetic = options.mc.antithetic;
  sim_opt.normal_shift = options.mc.importance_sampling ? options.mc.importance_shift : 0.0;
  sim_opt.seed = options.mc.seed;
  sim_opt.offset = options.mc.offset;

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
    const auto sim = volatility::heston_nandi_paths(
        options.mc.steps, options.mc.paths, spot, dt, q_params, sim_opt, out_cfg, options.mc.seed + static_cast<std::uint64_t>(g));
    const Series terminal = sim.spot.select(0, options.mc.steps).reshape({options.mc.paths});
    const Series strike_row = pluss::make_series(strikes, {1, k}, out_cfg);
    const Series terminal_col = terminal.reshape({options.mc.paths, 1});
    const double disc = std::exp(-rate * maturity);
    const Series call_payoff = terminal_col.sub(strike_row).clamp_min(0.0).mean(0, false).reshape({k}).mul(disc);
    const Series put_payoff = strike_row.sub(terminal_col).clamp_min(0.0).mean(0, false).reshape({k}).mul(disc);
    const Series mask = pluss::make_series(is_call, {k}, out_cfg);
    const Series market_s = pluss::make_series(market, {k}, out_cfg);
    const Series weight_s = pluss::make_series(weights, {k}, out_cfg);
    const Series model_s = call_payoff.mul(mask).add(put_payoff.mul(mask.mul(-1.0).add(1.0)));
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
  converged = false;

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

double heston_nandi_returns_nll(const Series& returns, const HestonNandiParams& params, Config cfg) {
  return returns_nll_impl_(returns, params, cfg);
}

double heston_nandi_sse(const std::vector<OptionQuote>& quotes,
                        double spot,
                        double rate,
                        double dividend_yield,
                        const HestonNandiParams& params,
                        const HestonNandiCalibOptions& options,
                        Config cfg) {
  validate_quotes_(quotes, "heston_nandi_sse");
  validate_market_inputs_(spot, rate, dividend_yield, "heston_nandi_sse");
  validate_mc_(options.mc, "heston_nandi_sse");
  return options_sse_impl_(quotes, spot, rate, dividend_yield, params, options, cfg);
}

HestonNandiCalibResult calibrate_heston_nandi(const Series& returns,
                                              const std::vector<OptionQuote>& quotes,
                                              double spot,
                                              double rate,
                                              double dividend_yield,
                                              const HestonNandiParams& initial,
                                              const HestonNandiCalibOptions& options,
                                              Config cfg) {
  validate_options_(options, "calibrate_heston_nandi");
  validate_params_(initial, "calibrate_heston_nandi");

  const bool use_returns = options.returns_weight > 0.0;
  const bool use_options = options.options_weight > 0.0;

  if (use_returns) {
    (void)as_vector_(returns, "calibrate_heston_nandi");
  }
  if (use_options) {
    validate_quotes_(quotes, "calibrate_heston_nandi");
    validate_market_inputs_(spot, rate, dividend_yield, "calibrate_heston_nandi");
  }

  const auto lo = lower_(options.bounds);
  const auto hi = upper_(options.bounds);
  const auto x0 = project_(to_vec_(initial), lo, hi);

  std::mt19937_64 rng(options.seed == 0U ? 0xFE57A9ULL : options.seed);
  std::int64_t evals = 0;

  auto objective = [&](const std::array<double, kHnDim>& x) -> double {
    const HestonNandiParams p = from_vec_(x, initial);
    const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
    if (!std::isfinite(persistence) || persistence >= (1.0 - p.stationarity_margin)) {
      return std::numeric_limits<double>::infinity();
    }

    double total = 0.0;
    if (use_returns) {
      const double nll = returns_nll_impl_(returns, p, cfg);
      if (!std::isfinite(nll)) {
        return std::numeric_limits<double>::infinity();
      }
      total += options.returns_weight * nll;
    }
    if (use_options) {
      const double sse = options_sse_impl_(quotes, spot, rate, dividend_yield, p, options, cfg);
      if (!std::isfinite(sse)) {
        return std::numeric_limits<double>::infinity();
      }
      total += options.options_weight * sse;
    }
    return total;
  };

  HestonNandiCalibResult out{};
  out.params = initial;
  out.objective = std::numeric_limits<double>::infinity();
  out.evaluations = 0;
  out.converged = false;

  std::array<double, kHnDim> best_x = x0;
  double best_obj = std::numeric_limits<double>::infinity();
  bool any_converged = false;

  const auto first = pattern_search_(
      x0, lo, hi, options.max_iterations, options.step_fraction, options.step_decay, options.tolerance, objective, evals, any_converged);
  best_x = first.first;
  best_obj = first.second;

  for (std::int64_t s = 1; s < options.random_starts; ++s) {
    bool local_conv = false;
    const auto start = random_point_(rng, lo, hi);
    const auto cand = pattern_search_(start,
                                      lo,
                                      hi,
                                      options.max_iterations,
                                      options.step_fraction,
                                      options.step_decay,
                                      options.tolerance,
                                      objective,
                                      evals,
                                      local_conv);
    if (cand.second < best_obj) {
      best_obj = cand.second;
      best_x = cand.first;
    }
    any_converged = any_converged || local_conv;
  }

  out.params = from_vec_(best_x, initial);
  out.objective = best_obj;
  out.evaluations = evals;
  out.converged = any_converged;
  out.returns_nll = use_returns ? returns_nll_impl_(returns, out.params, cfg) : 0.0;
  out.options_sse = use_options ? options_sse_impl_(quotes, spot, rate, dividend_yield, out.params, options, cfg) : 0.0;
  out.options_rmse = use_options ? weighted_rmse_(out.options_sse, option_weight_sum_(quotes), quotes.size()) : 0.0;
  return out;
}

} // namespace pluss::subsp::math::finance::calibration
