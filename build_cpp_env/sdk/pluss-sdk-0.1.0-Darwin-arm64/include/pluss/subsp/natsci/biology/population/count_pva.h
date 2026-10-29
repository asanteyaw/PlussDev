#pragma once

// Count-based population viability analysis with observation error (Dennis, Ponciano, Lele, Taper & Staples, Ecol. Monogr. 76 (2006) 323):
// the log abundance X_t is a Brownian motion with drift mu and variance sigma^2 per unit time, observed with independent Gaussian error of
// variance eta^2, y_t = X_t + v_t (the "Gompertz with theta = 1", i.e. exponential-growth state-space model). It is a linear-Gaussian state-space
// model, so the exact likelihood comes from the shared Kalman filter (math::state::filter, which skips missing census years), the parameters
// from maximum likelihood on (mu, ln sigma^2, ln eta^2) with the shared Nelder-Mead minimizer, and the extinction risk from the first-passage
// formula of birth_death::quasi_extinction_cdf averaged over the filtered distribution of the current log abundance.
//
//  * Series are laid out on a regular grid of step `dt`; years without a census are NaN (to_grid places irregular census times on it).
//  * The first census initialises the state, X_k ~ N(y_k, eta^2) (flat prior on X_k); the likelihood is that of the later censuses given it.
//  * eta^2 -> 0 recovers the Dennis-Munholland-Scott estimator (birth_death::dennis_estimate, with 1/n instead of 1/(n-1) in sigma^2).
//  * Risk is plug-in: parameter uncertainty is reported through the standard errors, not folded into the risk.

#include "pluss/ma/math/optimization.h"
#include "pluss/subsp/math/state.h"
#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/population/birth_death.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

namespace pluss::subsp::natsci::biology::population::count_pva {

namespace state = ::pluss::subsp::math::state;
namespace optimization = ::pluss::subsp::math::optimization;

struct Parameters final {
  double mu = 0.0;     // drift of ln N per unit time
  double sigma2 = 1.0; // process variance per unit time
  double eta2 = 0.0;   // observation-error variance of ln N
};

namespace detail {
constexpr auto kF64 = pluss::ScalarType::Float64;
inline Series f64(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::series::make(v, std::move(shape)).to(kF64); }
inline Series scalar_matrix(double v) { return f64({v}, {1, 1}); }
struct Prepared {
  std::int64_t first = 0; // index of the first census (initialises the state)
  std::vector<double> y;  // later observations on the grid (NaN = missing)
};
inline Prepared prepare(const std::vector<double> &log_counts) {
  Prepared p;
  const auto it = std::find_if(log_counts.begin(), log_counts.end(), [](double v) { return std::isfinite(v); });
  dynamics::require(it != log_counts.end(), "count_pva: no finite census");
  p.first = it - log_counts.begin();
  p.y.assign(it + 1, log_counts.end());
  const auto seen = std::count_if(p.y.begin(), p.y.end(), [](double v) { return std::isfinite(v); });
  dynamics::require(seen >= 3, "count_pva: need at least four censuses to identify mu, sigma^2 and eta^2");
  return p;
}
inline std::vector<double> log_series(const Series &counts) {
  const auto raw = counts.to(kF64).contiguous().template host<double>();
  std::vector<double> y(raw.size());
  for (std::size_t i = 0; i < raw.size(); ++i) {
    dynamics::require(std::isnan(raw[i]) || raw[i] > 0.0, "count_pva: counts must be positive (NaN marks a missing census)");
    y[i] = std::isnan(raw[i]) ? raw[i] : std::log(raw[i]);
  }
  return y;
}
inline state::LinearModel model(double y_first, const Parameters &q, double dt, std::int64_t steps) {
  return state::LinearModel{.transition = scalar_matrix(1.0),
                            .observation = scalar_matrix(1.0),
                            .process_cov = scalar_matrix(q.sigma2 * dt),
                            .observation_cov = scalar_matrix(q.eta2),
                            .initial_mean = f64({y_first}, {1}),
                            .initial_cov = scalar_matrix(q.eta2),
                            .input = pluss::series::ones({steps, 1}, kF64),
                            .state_input = scalar_matrix(q.mu * dt)};
}
inline state::FilterResult run(const std::vector<double> &logn, const Parameters &q, double dt, const Prepared &p) {
  return state::filter(f64(p.y, {static_cast<std::int64_t>(p.y.size()), 1}), model(logn[p.first], q, dt, static_cast<std::int64_t>(p.y.size())), state::FilterOptions{.info_tol = 0.0, .jitter = 0.0});
}
inline double loglik_of(const std::vector<double> &logn, const Parameters &q, double dt, const Prepared &p) { return run(logn, q, dt, p).loglik.template host<double>().front(); }
inline Parameters unpack(const std::array<double, 3> &t) { return {t[0], std::exp(t[1]), std::exp(std::max(t[2], -40.0))}; }
} // namespace detail

// census times (any units, increasing) -> counts placed on a regular grid of step dt (NaN where no census); times are rounded to the nearest step
struct Grid final {
  Series counts = pluss::series::undefined();
  Series times = pluss::series::undefined();
};
inline Grid to_grid(const Series &times, const Series &counts, double dt) {
  const auto t = times.to(detail::kF64).contiguous().template host<double>();
  const auto n = counts.to(detail::kF64).contiguous().template host<double>();
  dynamics::require(t.size() == n.size() && !t.empty() && dt > 0.0, "count_pva::to_grid: times and counts must match and dt must be positive");
  const std::int64_t m = static_cast<std::int64_t>(std::llround((t.back() - t.front()) / dt)) + 1;
  std::vector<double> grid(static_cast<std::size_t>(m), std::numeric_limits<double>::quiet_NaN()), tg(static_cast<std::size_t>(m));
  for (std::int64_t i = 0; i < m; ++i)
    tg[i] = t.front() + dt * static_cast<double>(i);
  for (std::size_t i = 0; i < t.size(); ++i) {
    const std::int64_t k = static_cast<std::int64_t>(std::llround((t[i] - t.front()) / dt));
    dynamics::require(std::abs(t[i] - (t.front() + dt * static_cast<double>(k))) < 1e-6 * dt, "count_pva::to_grid: census time is not a multiple of dt from the first census");
    dynamics::require(std::isnan(grid[k]), "count_pva::to_grid: two censuses fall on the same grid step");
    grid[k] = n[i];
  }
  return {detail::f64(grid, {m}), detail::f64(tg, {m})};
}

// exact log-likelihood of the later censuses given the first
inline double loglik(const Series &counts, const Parameters &q, double dt = 1.0) {
  const auto logn = detail::log_series(counts);
  return detail::loglik_of(logn, q, dt, detail::prepare(logn));
}

struct Fit final {
  Parameters parameters;
  double loglik = 0.0;
  double mu_se = std::numeric_limits<double>::quiet_NaN(); // from the inverse observed information (central differences)
  double sigma2_se = std::numeric_limits<double>::quiet_NaN();
  double eta2_se = std::numeric_limits<double>::quiet_NaN();
  bool converged = false;
  double filtered_mean = 0.0; // E[ln N] at the last census (a posteriori), and its variance
  double filtered_var = 0.0;
  double dt = 1.0;
  std::int64_t first = 0;
  Series smoothed_mean = pluss::series::undefined(); // E[ln N_t | all data] and sd, for t after the first census
  Series smoothed_sd = pluss::series::undefined();
};

inline Fit fit(const Series &counts, double dt = 1.0, std::int64_t restarts = 3) {
  using detail::Prepared;
  const auto logn = detail::log_series(counts);
  const Prepared p = detail::prepare(logn);
  // start from the observation-error-free regression on the finite censuses
  std::vector<double> yt, tt;
  for (std::size_t i = 0; i < logn.size(); ++i)
    if (std::isfinite(logn[i])) {
      yt.push_back(std::exp(logn[i]));
      tt.push_back(dt * static_cast<double>(i));
    }
  const auto d0 = birth_death::dennis_estimate(detail::f64(yt, {static_cast<std::int64_t>(yt.size())}), detail::f64(tt, {static_cast<std::int64_t>(tt.size())}));
  const double s0 = std::max(d0.sigma2.item<double>(), 1e-8);
  std::array<double, 3> theta = {d0.mu.item<double>(), std::log(0.5 * s0), std::log(0.25 * s0)};
  auto negll = [&](const std::array<double, 3> &t) { return -detail::loglik_of(logn, detail::unpack(t), dt, p); };
  Fit out;
  optimization::NelderMeadConfig cfg;
  cfg.position_tolerance = 1e-10;
  cfg.value_tolerance = 1e-12;
  cfg.max_iterations = 4000;
  cfg.max_evaluations = 8000;
  cfg.simplex_delta = 0.3;
  double best = negll(theta);
  for (std::int64_t r = 0; r < std::max<std::int64_t>(restarts, 1); ++r) {
    const auto res = optimization::nelder_mead_minimizer([&](const Series &x) {
      const auto v = x.to(detail::kF64).contiguous().template host<double>();
      return detail::f64({negll({v[0], v[1], v[2]})}, {});
    }, detail::f64({theta[0], theta[1], theta[2]}, {3}), cfg);
    const auto v = res.minimizer.to(detail::kF64).contiguous().template host<double>();
    const double val = negll({v[0], v[1], v[2]});
    const bool improved = val < best - 1e-12;
    if (val <= best)
      theta = {v[0], v[1], v[2]}, best = val;
    out.converged = true;
    if (!improved && r > 0)
      break;
  }
  out.parameters = detail::unpack(theta);
  out.loglik = -best;
  out.dt = dt;
  out.first = p.first;
  // standard errors: central-difference Hessian of -loglik in (mu, ln sigma^2, ln eta^2), inverted by cofactors, delta method for the variances
  std::array<std::array<double, 3>, 3> H{};
  std::array<double, 3> step = {1e-4 * std::max(1.0, std::abs(theta[0])), 1e-3, 1e-3};
  for (int i = 0; i < 3; ++i)
    for (int j = i; j < 3; ++j) {
      auto at = [&](double si, double sj) {
        auto t = theta;
        t[i] += si * step[i];
        t[j] += sj * step[j];
        return negll(t);
      };
      H[i][j] = H[j][i] = (i == j) ? (at(1, 0) - 2.0 * best + at(-1, 0)) / (step[i] * step[i]) : (at(1, 1) - at(1, -1) - at(-1, 1) + at(-1, -1)) / (4.0 * step[i] * step[j]);
    }
  const double det = H[0][0] * (H[1][1] * H[2][2] - H[1][2] * H[2][1]) - H[0][1] * (H[1][0] * H[2][2] - H[1][2] * H[2][0]) + H[0][2] * (H[1][0] * H[2][1] - H[1][1] * H[2][0]);
  if (std::isfinite(det) && det > 0.0) {
    const double c00 = (H[1][1] * H[2][2] - H[1][2] * H[2][1]) / det, c11 = (H[0][0] * H[2][2] - H[0][2] * H[2][0]) / det, c22 = (H[0][0] * H[1][1] - H[0][1] * H[1][0]) / det;
    if (c00 > 0.0 && c11 > 0.0 && c22 > 0.0) {
      out.mu_se = std::sqrt(c00);
      out.sigma2_se = out.parameters.sigma2 * std::sqrt(c11);
      out.eta2_se = out.parameters.eta2 * std::sqrt(c22);
    }
  }
  const auto sm = state::smooth(detail::f64(p.y, {static_cast<std::int64_t>(p.y.size()), 1}), detail::model(logn[p.first], out.parameters, dt, static_cast<std::int64_t>(p.y.size())), state::FilterOptions{.info_tol = 0.0, .jitter = 0.0});
  const std::int64_t n = static_cast<std::int64_t>(p.y.size());
  out.filtered_mean = sm.filter.filtered_mean.select(0, n - 1).template host<double>().front();
  out.filtered_var = sm.filter.filtered_cov.select(0, n - 1).template host<double>().front();
  out.smoothed_mean = sm.smoothed_mean.squeeze(-1);
  out.smoothed_sd = sm.smoothed_cov.squeeze(-1).squeeze(-1).sqrt();
  return out;
}

// Probability that ln N falls to ln(threshold) within each horizon (time units), given the filtered distribution of the current log abundance,
// X_T ~ N(m, P):  int cdf(x_d) N(x_d; m - ln threshold, P) dx_d, with probability one when x_d <= 0. Returns shape [H].
inline Series extinction_risk(const Fit &f, double threshold, const Series &horizons) {
  dynamics::require(threshold > 0.0, "count_pva::extinction_risk: threshold must be positive");
  const double m = f.filtered_mean - std::log(threshold), sd = std::sqrt(f.filtered_var);
  const double already = 0.5 * std::erfc(m / (sd * std::sqrt(2.0))); // P(x_d <= 0)
  const Series t = horizons.to(detail::kF64);
  const double hi = m + 9.0 * sd;
  Series upper = t.mul(0.0);
  if (hi > 0.0) {
    constexpr std::int64_t n = 2001;
    const Series x = Series::linspace(0.0, hi, n, detail::kF64);
    std::vector<double> w(n);
    for (std::int64_t i = 0; i < n; ++i)
      w[i] = (i == 0 || i == n - 1) ? 1.0 : (i % 2 ? 4.0 : 2.0);
    const double h = hi / static_cast<double>(n - 1);
    const Series dens = x.sub(m).div(sd).square().mul(-0.5).exp().mul(1.0 / (sd * std::sqrt(2.0 * std::numbers::pi)));
    const Series cdf = birth_death::quasi_extinction_cdf(detail::f64({f.parameters.mu}, {1, 1}), detail::f64({f.parameters.sigma2}, {1, 1}), x.unsqueeze(0), t.unsqueeze(-1));
    upper = cdf.mul(dens).mul(detail::f64(w, {n}).mul(h / 3.0)).sum(-1, false);
  }
  return upper.add(already);
}
// ultimate (infinite-horizon) risk
inline double ultimate_extinction_risk(const Fit &f, double threshold) {
  const double m = f.filtered_mean - std::log(threshold), sd = std::sqrt(f.filtered_var);
  if (f.parameters.mu <= 0.0)
    return 1.0;
  const double already = 0.5 * std::erfc(m / (sd * std::sqrt(2.0)));
  const double hi = m + 9.0 * sd;
  if (hi <= 0.0)
    return already;
  constexpr std::int64_t n = 2001;
  const Series x = Series::linspace(0.0, hi, n, detail::kF64);
  std::vector<double> w(n);
  for (std::int64_t i = 0; i < n; ++i)
    w[i] = (i == 0 || i == n - 1) ? 1.0 : (i % 2 ? 4.0 : 2.0);
  const Series dens = x.sub(m).div(sd).square().mul(-0.5).exp().mul(1.0 / (sd * std::sqrt(2.0 * std::numbers::pi)));
  const Series p = birth_death::ultimate_quasi_extinction(detail::f64({f.parameters.mu}, {1}), detail::f64({f.parameters.sigma2}, {1}), x);
  return p.mul(dens).mul(detail::f64(w, {n}).mul(hi / static_cast<double>(n - 1) / 3.0)).sum().item<double>() + already;
}

} // namespace pluss::subsp::natsci::biology::population::count_pva
