// Early exercise: a binomial lattice (Black-Scholes) and Longstaff-Schwartz regression on simulated paths (any model).
#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config cpu_f64_() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series payoff_(OptionType type, const Series& s, double strike) {
  return type == OptionType::Call ? s.sub(strike).clamp_min(0.0) : s.mul(-1.0).add(strike).clamp_min(0.0);
}

void check_type_(OptionType type, const char* fn) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put,
              std::string("subsp::math::finance::pricing::") + fn + ": option type must be Call or Put");
}

// exercisable[j] for j = 0..steps: American at every step; Bermudan at the dates snapped to the grid plus maturity; European at maturity.
std::vector<bool> exercise_grid_(ExerciseStyle style, double maturity, std::int64_t steps, std::span<const double> dates, const char* fn) {
  std::vector<bool> ok(static_cast<std::size_t>(steps) + 1, false);
  ok[static_cast<std::size_t>(steps)] = true;
  if (style == ExerciseStyle::American) {
    std::fill(ok.begin(), ok.end(), true);
  } else if (style == ExerciseStyle::Bermudan) {
    const double dt = maturity / static_cast<double>(steps);
    for (const double t : dates) {
      PLUSS_CHECK(std::isfinite(t) && t > 0.0,
                  std::string("subsp::math::finance::pricing::") + fn + ": exercise dates must be finite and > 0");
      if (t >= maturity) continue;  // the maturity is always exercisable; later dates fall outside this option's life
      const auto j = std::clamp<std::int64_t>(std::llround(t / dt), 1, steps);
      ok[static_cast<std::size_t>(j)] = true;
    }
  }
  return ok;
}

}  // namespace

double detail::lattice_price(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield,
                     double maturity, double volatility, std::int64_t steps, std::span<const double> dates, Config) {
  check_type_(type, "lattice_price");
  PLUSS_CHECK(steps >= 1, "subsp::math::finance::pricing::lattice_price: steps must be >= 1");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0,
              "subsp::math::finance::pricing::lattice_price: spot and strike must be finite and > 0");
  PLUSS_CHECK(std::isfinite(volatility) && volatility > 0.0,
              "subsp::math::finance::pricing::lattice_price: volatility must be finite and > 0");
  if (!(maturity > 0.0)) {
    return type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
  }
  const auto ok = exercise_grid_(style, maturity, steps, dates, "lattice_price");
  const double dt = maturity / static_cast<double>(steps);
  const double vs = volatility * std::sqrt(dt);
  const double up = std::exp(vs), down = 1.0 / up;
  const double p = (std::exp((rate - dividend_yield) * dt) - down) / (up - down);
  PLUSS_CHECK(p > 0.0 && p < 1.0,
              "subsp::math::finance::pricing::lattice_price: the risk-neutral probability left (0,1); use more steps or a larger volatility");
  const double disc = std::exp(-rate * dt);

  // level j has nodes spot * exp(vs * (2i - j)), i = 0..j, i.e. spot * exp(-vs j) * g[i] with g[i] = exp(2 vs i).  The recursion is a tight scalar
  // loop (about N^2/2 node updates), so it runs on plain arrays: per-operation tensor overhead would dominate.
  std::vector<double> g(static_cast<std::size_t>(steps) + 1), v(g.size());
  for (std::size_t i = 0; i < g.size(); ++i) g[i] = std::exp(2.0 * vs * static_cast<double>(i));
  auto pay = [&](double s) { return type == OptionType::Call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0); };
  const double bottom_n = spot * std::exp(-vs * static_cast<double>(steps));
  for (std::size_t i = 0; i < v.size(); ++i) v[i] = pay(bottom_n * g[i]);
  const double up_p = p, down_p = 1.0 - p;
  for (std::int64_t j = steps - 1; j >= 0; --j) {
    const auto count = static_cast<std::size_t>(j) + 1;
    for (std::size_t i = 0; i < count; ++i) v[i] = (up_p * v[i + 1] + down_p * v[i]) * disc;
    if (ok[static_cast<std::size_t>(j)]) {
      const double bottom = spot * std::exp(-vs * static_cast<double>(j));
      for (std::size_t i = 0; i < count; ++i) v[i] = std::max(v[i], pay(bottom * g[i]));
    }
  }
  return v[0];
}

namespace {

// The exercise rule of a Longstaff-Schwartz fit: for every exercisable step with enough in-the-money paths, the regression coefficients of the continuation value
// on powers of s / strike. Applied to any set of paths it gives each path's stopping step without a new regression.
struct LsmPolicy {
  std::vector<std::int64_t> steps;   // ascending
  std::vector<Series> coefficients;  // [terms, 1] per entry of `steps`
};

// Backward induction of the fit. Returns the value (before the American comparison with immediate exercise); fills `policy` when given.
double lsm_fit_(OptionType type, ExerciseStyle style, double strike, double rate, double maturity, const Series& paths, std::span<const double> dates, std::int64_t degree,
                LsmPolicy* policy, const char* fn) {
  const Config cfg = cpu_f64_();
  const std::int64_t steps = paths.size(1) - 1;
  const double dt = maturity / static_cast<double>(steps);
  const auto ok = exercise_grid_(style, maturity, steps, dates, fn);
  const std::int64_t terms = degree + 1;
  const Series powers = pluss::series::arange(0, terms, cfg);
  const Series ridge = pluss::series::eye(terms, cfg);

  Series cash = payoff_(type, paths.select(1, steps), strike);  // value at `last`, discounted to the time of step `last`
  std::int64_t last = steps;
  for (std::int64_t j = steps - 1; j >= 1; --j) {
    if (!ok[static_cast<std::size_t>(j)]) continue;
    cash = cash.mul(std::exp(-rate * dt * static_cast<double>(last - j)));
    last = j;
    const Series s = paths.select(1, j);
    const Series now = payoff_(type, s, strike);
    const Series itm = now.gt(0.0).to(ScalarType::Float64);
    if (itm.sum().item<double>() < static_cast<double>(terms + 1)) continue;
    const Series phi = s.div(strike).unsqueeze(1).pow(powers);  // [paths, terms]
    const Series weighted = phi.mul(itm.unsqueeze(1)).transpose(0, 1);  // [terms, paths]
    Series normal = weighted.matmul(phi);
    normal = normal.add(ridge.mul(1e-10 * normal.select(0, 0).select(0, 0).item<double>()));
    const Series coef = pluss::subsp::math::linalg::cholesky_solve(weighted.matmul(cash.unsqueeze(1)), pluss::subsp::math::linalg::cholesky(normal));
    const Series continuation = phi.matmul(coef).squeeze(1);
    const Series stop = now.gt(continuation).to(ScalarType::Float64).mul(itm).gt(0.5);
    cash = stop.where(now, cash);
    if (policy) {
      policy->steps.insert(policy->steps.begin(), j);
      policy->coefficients.insert(policy->coefficients.begin(), coef);
    }
  }
  return cash.mul(std::exp(-rate * dt * static_cast<double>(last))).mean().item<double>();
}

void check_lsm_inputs_(OptionType type, double strike, double maturity, const Series& asset_paths, std::int64_t degree, const char* fn) {
  check_type_(type, fn);
  PLUSS_CHECK(asset_paths.defined() && asset_paths.dim() == 2 && asset_paths.size(1) >= 2,
              std::string("subsp::math::finance::pricing::") + fn + ": asset_paths must be shaped [paths, steps + 1] with steps >= 1");
  PLUSS_CHECK(degree >= 1 && degree <= 8, std::string("subsp::math::finance::pricing::") + fn + ": degree must be in [1, 8]");
  PLUSS_CHECK(std::isfinite(strike) && strike > 0.0 && std::isfinite(maturity) && maturity > 0.0,
              std::string("subsp::math::finance::pricing::") + fn + ": strike and maturity must be finite and > 0");
}

}  // namespace

double detail::lsm_price(OptionType type, ExerciseStyle style, double strike, double rate, double maturity, const Series& asset_paths,
                 std::span<const double> dates, std::int64_t degree) {
  check_lsm_inputs_(type, strike, maturity, asset_paths, degree, "lsm_price");
  const Series paths = asset_paths.to(pluss::pCPU, ScalarType::Float64);
  const double value = lsm_fit_(type, style, strike, rate, maturity, paths, dates, degree, nullptr, "lsm_price");
  if (style != ExerciseStyle::American) return value;
  const double s0 = paths.select(1, 0).mean().item<double>();
  return std::max(value, type == OptionType::Call ? std::max(s0 - strike, 0.0) : std::max(strike - s0, 0.0));
}

ScalarGreeks detail::lsm_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double maturity, double level,
                                const std::function<Series(const LsmTracked&)>& simulate, std::span<const double> dates, std::int64_t degree, const LsmModel& model) {
  ScalarGreeks g;
  const Config cfg = cpu_f64_();
  const Series paths = simulate(LsmTracked{}).to(pluss::pCPU, ScalarType::Float64);
  check_lsm_inputs_(type, strike, maturity, paths, degree, "lsm_greeks");
  const std::int64_t steps = paths.size(1) - 1;
  const double dt = maturity / static_cast<double>(steps);
  const bool call = type == OptionType::Call;
  const double sign = call ? 1.0 : -1.0;
  LsmPolicy policy;
  const double fitted = lsm_fit_(type, style, strike, rate, maturity, paths, dates, degree, &policy, "lsm_greeks");
  const double intrinsic0 = call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
  if (style == ExerciseStyle::American && intrinsic0 >= fitted) {   // immediate exercise is optimal: the value is the payoff
    g.price = intrinsic0;
    g.delta = intrinsic0 > 0.0 ? sign : 0.0;
    return g;
  }
  const Series powers = pluss::series::arange(0, degree + 1, cfg);
  const Series grid = pluss::series::arange(0, steps + 1, cfg);   // step indices, to select the stopping step by a mask
  const Series discount_row = grid.mul(-rate * dt).exp();
  // each path's stopping step: the earliest exercisable step at which the payoff beats the fitted continuation value
  const auto stopping_step = [&](const Series& p) {
    Series idx = pluss::series::full({p.size(0)}, static_cast<double>(steps), cfg);
    for (std::size_t k = policy.steps.size(); k-- > 0;) {
      const Series s = p.select(1, policy.steps[k]);
      const Series now = payoff_(type, s, strike);
      const Series continuation = s.div(strike).unsqueeze(1).pow(powers).matmul(policy.coefficients[k]).squeeze(1);
      const Series stop = now.gt(continuation).to(ScalarType::Float64).mul(now.gt(0.0).to(ScalarType::Float64)).gt(0.5);
      idx = stop.where(pluss::series::full({p.size(0)}, static_cast<double>(policy.steps[k]), cfg), idx);
    }
    return idx;
  };
  const auto stop_mask = [&](const Series& idx) { return idx.unsqueeze(1).eq(grid.unsqueeze(0)).to(ScalarType::Float64); };   // [paths, steps + 1]
  const auto estimate = [&](const Series& p, const Series& mask) { return payoff_(type, p, strike).mul(mask).mul(discount_row.unsqueeze(0)).sum(1).mean(); };
  const Series mask = stop_mask(stopping_step(paths));
  const Series base = estimate(paths, mask);
  g.price = base.item<double>();
  if (style == ExerciseStyle::American) g.price = std::max(g.price, intrinsic0);

  // delta and rho pathwise along the stopping rule (the value is stationary in the rule): S_stop scales with the spot and, through the drift, with exp(r t)
  const Series stop_spot = paths.mul(mask).sum(1);
  const Series stop_time = mask.mul(grid.mul(dt).unsqueeze(0)).sum(1);
  const Series stop_disc = mask.mul(discount_row.unsqueeze(0)).sum(1);
  const Series live = (call ? stop_spot.gt(strike) : stop_spot.lt(strike)).to(ScalarType::Float64);
  const Series stop_pay = payoff_(type, stop_spot, strike);
  const double stopped_stock = sign * stop_disc.mul(live).mul(stop_spot).mean().item<double>();
  if (model.homogeneous) {
    g.delta = stopped_stock / spot;
    g.rho = stop_disc.mul(stop_time).mul(live.mul(stop_spot).mul(sign).sub(stop_pay)).mean().item<double>();

    // gamma: the same rule applied to the paths with the spot scaled (they are proportional to it); no new regression
    constexpr double eps = 0.05;   // the payoff is piecewise linear along a fixed rule: a wide step lowers the variance of the second difference
    const auto at_scale = [&](double scale) {
      const Series scaled = paths.mul(scale);
      return estimate(scaled, stop_mask(stopping_step(scaled))).item<double>();
    };
    g.gamma = (at_scale(1.0 + eps) - 2.0 * base.item<double>() + at_scale(1.0 - eps)) / (eps * eps * spot * spot);

    // vega: the paths are differentiated in the volatility level (a tracked scalar) along the same stopping rule
    Series tracked = pluss::make_series(level).to(pluss::pCPU, ScalarType::Float64);
    tracked.tracks_slope_(true);
    estimate(simulate(LsmTracked{.level = &tracked}).to(pluss::pCPU, ScalarType::Float64), mask).reverse();
    g.vega = tracked.sensitivity().item<double>();
  } else {
    // paths that are not proportional to the spot: one reverse pass differentiates the stopped estimate in the level, the initial spot (a copy per path) and the drift;
    // the discount factor's own dependence on the rate adds -t * payoff
    Series lv = pluss::make_series(level).to(pluss::pCPU, ScalarType::Float64);
    Series dr = pluss::make_series(model.carry).to(pluss::pCPU, ScalarType::Float64);
    Series f0 = pluss::series::full({paths.size(0)}, spot, cfg);
    lv.tracks_slope_(true);
    dr.tracks_slope_(true);
    f0.tracks_slope_(true);
    estimate(simulate(LsmTracked{.level = &lv, .spot = &f0, .drift = &dr}).to(pluss::pCPU, ScalarType::Float64), mask).reverse();
    g.vega = lv.sensitivity().item<double>();
    g.delta = f0.sensitivity().sum().item<double>();
    g.rho = dr.sensitivity().item<double>() - stop_disc.mul(stop_time).mul(stop_pay).mean().item<double>();

    // gamma: the stopping rule applied to paths simulated from the spot scaled up and down with the same innovations; no new regression
    constexpr double eps = 0.05;   // the payoff is piecewise linear along a fixed rule: a wide step lowers the variance of the second difference
    const auto at_scale = [&](double scale) {
      const Series moved = simulate(LsmTracked{.spot_scale = scale}).to(pluss::pCPU, ScalarType::Float64);
      return estimate(moved, stop_mask(stopping_step(moved))).item<double>();
    };
    g.gamma = (at_scale(1.0 + eps) - 2.0 * base.item<double>() + at_scale(1.0 - eps)) / (eps * eps * spot * spot);
  }

  // theta: the fit over the one and two earlier horizons of the same paths (second-order backward difference), as for the European strip
  const auto earlier = [&](std::int64_t drop) {
    const Series cut = paths.slice(1, 0, steps + 1 - drop);
    return detail::lsm_price(type, style, strike, rate, maturity * static_cast<double>(steps - drop) / static_cast<double>(steps), cut, dates, degree);
  };
  if (model.bs_volatility > 0.0) {   // the value solves the Black-Scholes equation at the spot, which is in the continuation region
    g.theta = rate * g.price - model.carry * spot * g.delta - 0.5 * model.bs_volatility * model.bs_volatility * spot * spot * g.gamma;
  } else if (steps >= 3) {
    g.theta = -(3.0 * g.price - 4.0 * earlier(1) + earlier(2)) / (2.0 * dt);
  } else {
    g.theta = -(g.price - earlier(1)) / dt;
  }
  return g;
}

ScalarGreeks detail::lattice_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield,
                                    double maturity, double volatility, std::int64_t steps, std::span<const double> dates, Config cfg) {
  ScalarGreeks g;
  g.price = lattice_price(type, style, spot, strike, rate, dividend_yield, maturity, volatility, steps, dates, cfg);
  if (!(maturity > 0.0)) {
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  PLUSS_CHECK(steps >= 2, "subsp::math::finance::pricing::lattice_greeks: steps must be >= 2");
  const auto ok = exercise_grid_(style, maturity, steps, dates, "lattice_greeks");
  const double dt = maturity / static_cast<double>(steps);
  const double vs = volatility * std::sqrt(dt);
  const double up = std::exp(vs), down = 1.0 / up;
  const double p = (std::exp((rate - dividend_yield) * dt) - down) / (up - down);
  const double disc = std::exp(-rate * dt);
  std::vector<double> gg(static_cast<std::size_t>(steps) + 1), v(gg.size());
  for (std::size_t i = 0; i < gg.size(); ++i) gg[i] = std::exp(2.0 * vs * static_cast<double>(i));
  auto pay = [&](double s) { return type == OptionType::Call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0); };
  const double bottom_n = spot * std::exp(-vs * static_cast<double>(steps));
  for (std::size_t i = 0; i < v.size(); ++i) v[i] = pay(bottom_n * gg[i]);
  double level1[2] = {0, 0}, level2[3] = {0, 0, 0};
  for (std::int64_t j = steps - 1; j >= 0; --j) {
    const auto count = static_cast<std::size_t>(j) + 1;
    for (std::size_t i = 0; i < count; ++i) v[i] = (p * v[i + 1] + (1.0 - p) * v[i]) * disc;
    if (ok[static_cast<std::size_t>(j)]) {
      const double bottom = spot * std::exp(-vs * static_cast<double>(j));
      for (std::size_t i = 0; i < count; ++i) v[i] = std::max(v[i], pay(bottom * gg[i]));
    }
    if (j == 2) { level2[0] = v[0]; level2[1] = v[1]; level2[2] = v[2]; }
    if (j == 1) { level1[0] = v[0]; level1[1] = v[1]; }
  }
  const double s_uu = spot * up * up, s_dd = spot * down * down;
  g.delta = (level1[1] - level1[0]) / (spot * up - spot * down);
  g.gamma = ((level2[2] - level2[1]) / (s_uu - spot) - (level2[1] - level2[0]) / (spot - s_dd)) / (0.5 * (s_uu - s_dd));
  g.theta = (level2[1] - v[0]) / (2.0 * dt);   // V(tau - 2 dt) at the same spot less V(tau), per year: minus the derivative in the time to maturity
  // vega and rho by differentiating the recursion itself (forward mode): along the active branch of each node's max(), the derivative of the value is carried
  // in step with the value. Sigma moves the node spots and the probability; the rate moves the probability and the discount factor.
  {
    const double sq = std::sqrt(dt);
    const double d_up = up * sq, d_down = -down * sq;                                   // d up / d sigma, d down / d sigma
    const double carry = std::exp((rate - dividend_yield) * dt);
    const double dp_ds = (-d_down * (up - down) - (carry - down) * (d_up - d_down)) / ((up - down) * (up - down));   // p = (carry - down) / (up - down)
    const double dp_dr = dt * carry / (up - down);
    const double ddisc_dr = -dt * disc;
    std::vector<double> vs_(v.size()), vr_(v.size());                                   // value, d/d sigma and d/d rate at the current level
    std::vector<double> value(v.size());
    const double sign = type == OptionType::Call ? 1.0 : -1.0;
    for (std::size_t i = 0; i < value.size(); ++i) {
      const double node = bottom_n * gg[i];
      const double intrinsic = pay(node);
      value[i] = intrinsic;
      // node = spot * exp(vs (2 i - N)): d node / d sigma = node sqrt(dt) (2 i - N)
      vs_[i] = intrinsic > 0.0 ? sign * node * sq * (2.0 * static_cast<double>(i) - static_cast<double>(steps)) : 0.0;
      vr_[i] = 0.0;
    }
    for (std::int64_t j = steps - 1; j >= 0; --j) {
      const auto count = static_cast<std::size_t>(j) + 1;
      for (std::size_t i = 0; i < count; ++i) {
        const double cont = (p * value[i + 1] + (1.0 - p) * value[i]) * disc;
        const double cont_s = (dp_ds * (value[i + 1] - value[i]) + p * vs_[i + 1] + (1.0 - p) * vs_[i]) * disc;
        const double cont_r = (dp_dr * (value[i + 1] - value[i]) + p * vr_[i + 1] + (1.0 - p) * vr_[i]) * disc + (p * value[i + 1] + (1.0 - p) * value[i]) * ddisc_dr;
        value[i] = cont; vs_[i] = cont_s; vr_[i] = cont_r;
      }
      if (ok[static_cast<std::size_t>(j)]) {
        const double bottom = spot * std::exp(-vs * static_cast<double>(j));
        for (std::size_t i = 0; i < count; ++i) {
          const double node = bottom * gg[i];
          const double intrinsic = pay(node);
          if (intrinsic > value[i]) {   // exercised: the value is the payoff, which depends on sigma through the node position only
            value[i] = intrinsic;
            vs_[i] = sign * node * sq * (2.0 * static_cast<double>(i) - static_cast<double>(j));
            vr_[i] = 0.0;
          }
        }
      }
    }
    g.vega = vs_[0];
    g.rho = vr_[0];
  }
  return g;
}

}  // namespace pluss::subsp::math::finance::pricing
