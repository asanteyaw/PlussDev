// Heston option values by a two-dimensional PDE.  The numerics (stretched grids, second-order finite differences, the Modified Craig-Sneyd ADI
// scheme, Rannacher start, Ikonen-Toivanen exercise splitting) live in the generic solver of pde2d_internal.h; this file states the Heston
// problem for it.  See the doc comment of pricing::detail::heston_price_pde.
#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pde2d_internal.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
namespace {

namespace pde2d = internal::pde2d;

}  // namespace

double detail::heston_price_pde(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                        const volatility::HestonParams& p, std::span<const double> dates, const PdeOptions& o) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::heston_price_pde: option type must be Call or Put");
  PLUSS_CHECK(o.spot_nodes >= 8 && o.variance_nodes >= 6 && o.time_steps >= 3,
              "subsp::math::finance::pricing::heston_price_pde: spot_nodes >= 8, variance_nodes >= 6 and time_steps >= 3 are required");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0,
              "subsp::math::finance::pricing::heston_price_pde: spot and strike must be finite and > 0");
  PLUSS_CHECK(p.kappa > 0.0 && p.theta > 0.0 && p.sigma > 0.0 && p.v0 >= 0.0 && p.rho > -1.0 && p.rho < 1.0,
              "subsp::math::finance::pricing::heston_price_pde: Heston parameters out of range");
  if (!(maturity > 0.0)) return type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
  const bool call = type == OptionType::Call, american = style == ExerciseStyle::American;

  pde2d::Options options;
  options.steps = o.time_steps;
  options.exercise = american ? pde2d::Exercise::American : style == ExerciseStyle::Bermudan ? pde2d::Exercise::Bermudan : pde2d::Exercise::None;
  if (style == ExerciseStyle::Bermudan) {
    for (const double t : dates) {
      PLUSS_CHECK(std::isfinite(t) && t > 0.0, "subsp::math::finance::pricing::heston_price_pde: exercise dates must be finite and > 0");
      if (t < maturity) options.breaks.push_back(maturity - t);
    }
    std::sort(options.breaks.begin(), options.breaks.end());
    options.breaks.erase(std::unique(options.breaks.begin(), options.breaks.end(), [](double a, double b) { return std::abs(a - b) < 1e-12; }), options.breaks.end());
  }

  // grids: the asset concentrated around the strike (a node at the strike, where the payoff has its kink), the variance near zero
  const double smax = std::max(4.0 * strike, 3.0 * spot);
  const double vmax = std::max({1.0, 10.0 * p.theta, 4.0 * p.v0, 6.0 * p.sigma * p.sigma / p.kappa});
  pde2d::Problem problem;
  problem.x = pde2d::clustered_grid(0.0, smax, strike, strike / 10.0, static_cast<std::size_t>(o.spot_nodes));
  problem.y = pde2d::origin_grid(vmax, vmax / 200.0, static_cast<std::size_t>(o.variance_nodes));
  problem.maturity = maturity;

  // dV/dtau = 1/2 v S^2 V_SS + rho sigma v S V_Sv + 1/2 sigma^2 v V_vv + (r - q) S V_S + kappa (theta - v) V_v - r V
  problem.generator = [=](double s, double v) {
    return pde2d::Coefficients{.a11 = v * s * s, .a12 = p.rho * p.sigma * v * s, .a22 = p.sigma * p.sigma * v, .b1 = (rate - dividend_yield) * s, .b2 = p.kappa * (p.theta - v), .c = rate};
  };
  problem.payoff = [=](double s, double) { return call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0); };
  // S = 0 and v = 0 are degenerate (the diffusion vanishes; the drift points inward since kappa theta > 0); at v_max the value is flat in v; at
  // S_max the value is 0 for a put and the deep in-the-money asymptote for a call
  problem.x_low = pde2d::Edge::Degenerate;
  problem.y_low = pde2d::Edge::Degenerate;
  problem.y_high = pde2d::Edge::Frozen;
  problem.x_high = pde2d::Edge::Dirichlet;
  problem.x_high_value = [=](double tau, double) {
    if (!call) return 0.0;
    const double european = smax * std::exp(-dividend_yield * tau) - strike * std::exp(-rate * tau);
    return american ? std::max(european, smax - strike) : european;
  };

  pde2d::Solver solver(std::move(problem));
  return solver.interpolate(solver.solve(options), spot, p.v0);
}

ScalarGreeks detail::heston_pde_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                       const volatility::HestonParams& p, std::span<const double> dates, const PdeOptions& o) {
  ScalarGreeks g;
  if (!(maturity > 0.0)) {
    g.price = type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  // solve once and read everything that does not need another solve off the surface
  const bool call = type == OptionType::Call, american = style == ExerciseStyle::American;
  const auto build = [&](double r) {
    pde2d::Options options;
    options.steps = o.time_steps;
    options.exercise = american ? pde2d::Exercise::American : style == ExerciseStyle::Bermudan ? pde2d::Exercise::Bermudan : pde2d::Exercise::None;
    if (style == ExerciseStyle::Bermudan) {
      for (const double t : dates) if (t > 0.0 && t < maturity) options.breaks.push_back(maturity - t);
      std::sort(options.breaks.begin(), options.breaks.end());
      options.breaks.erase(std::unique(options.breaks.begin(), options.breaks.end(), [](double a, double b) { return std::abs(a - b) < 1e-12; }), options.breaks.end());
    }
    const double smax = std::max(4.0 * strike, 3.0 * spot);
    const double vmax = std::max({1.0, 10.0 * p.theta, 4.0 * p.v0, 6.0 * p.sigma * p.sigma / p.kappa});
    pde2d::Problem problem;
    problem.x = pde2d::clustered_grid(0.0, smax, strike, strike / 10.0, static_cast<std::size_t>(o.spot_nodes));
    problem.y = pde2d::origin_grid(vmax, vmax / 200.0, static_cast<std::size_t>(o.variance_nodes));
    problem.maturity = maturity;
    problem.generator = [=](double s, double v) {
      return pde2d::Coefficients{.a11 = v * s * s, .a12 = p.rho * p.sigma * v * s, .a22 = p.sigma * p.sigma * v, .b1 = (r - dividend_yield) * s, .b2 = p.kappa * (p.theta - v), .c = r};
    };
    problem.payoff = [=](double s, double) { return call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0); };
    problem.x_low = pde2d::Edge::Degenerate;
    problem.y_low = pde2d::Edge::Degenerate;
    problem.y_high = pde2d::Edge::Frozen;
    problem.x_high = pde2d::Edge::Dirichlet;
    problem.x_high_value = [=](double tau, double) {
      if (!call) return 0.0;
      const double european = smax * std::exp(-dividend_yield * tau) - strike * std::exp(-r * tau);
      return american ? std::max(european, smax - strike) : european;
    };
    // the derivative in the interest rate: the drift (S dS) and the discount (-1) of the generator, and the deep in-the-money boundary of a call
    problem.generator_tangent = [](double s, double) { return pde2d::Coefficients{.b1 = s, .c = 1.0}; };
    problem.x_high_tangent = [=](double tau, double) {
      if (!call) return 0.0;
      const double european = smax * std::exp(-dividend_yield * tau) - strike * std::exp(-r * tau);
      return (american && smax - strike > european) ? 0.0 : strike * tau * std::exp(-r * tau);
    };
    return std::make_pair(pde2d::Solver(std::move(problem)), options);
  };
  auto [solver, options] = build(rate);
  std::vector<double> u_rate;
  const std::vector<double> u = solver.solve(options, &u_rate);
  const auto V = [&](double s, double v) { return solver.interpolate(u, s, v); };
  const auto V_rate = [&](double s, double v) { return solver.interpolate(u_rate, s, v); };
  const double v0 = p.v0;
  const double hs = 0.01 * spot, hv = std::max(0.1 * v0, 2e-3);
  const double v_lo = std::max(v0 - hv, 0.0), v_hi = v0 + hv;
  g.price = V(spot, v0);
  const double Vs = (V(spot + hs, v0) - V(spot - hs, v0)) / (2.0 * hs);
  const double Vss = (V(spot + hs, v0) - 2.0 * g.price + V(spot - hs, v0)) / (hs * hs);
  const double Vv = (V(spot, v_hi) - V(spot, v_lo)) / (v_hi - v_lo);
  const double Vvv = (V(spot, v_hi) - 2.0 * g.price + V(spot, v_lo)) / (hv * hv);   // (v_lo may be clamped at zero: then use the one-sided pair)
  const double Vsv = (V(spot + hs, v_hi) - V(spot + hs, v_lo) - V(spot - hs, v_hi) + V(spot - hs, v_lo)) / (2.0 * hs * (v_hi - v_lo));
  g.delta = Vs;
  g.gamma = Vss;
  g.vega = Vv;
  const double intrinsic = call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
  const bool exercised = style != ExerciseStyle::European && g.price <= intrinsic + 1e-10;
  // V_tau from the PDE in the continuation region; in the exercise region the value is the payoff and does not change with the time to maturity
  const double Vtau = exercised ? 0.0
                                : 0.5 * v0 * spot * spot * Vss + p.rho * p.sigma * v0 * spot * Vsv + 0.5 * p.sigma * p.sigma * v0 * Vvv +
                                      (rate - dividend_yield) * spot * Vs + p.kappa * (p.theta - v0) * Vv - rate * g.price;
  g.theta = -Vtau;
  g.rho = V_rate(spot, v0);
  return g;
}

}  // namespace pluss::subsp::math::finance::pricing
