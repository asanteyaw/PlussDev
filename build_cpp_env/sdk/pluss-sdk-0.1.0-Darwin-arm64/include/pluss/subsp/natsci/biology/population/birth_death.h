#pragma once

// Demographic stochasticity: exact theory for birth-death populations, branching processes and diffusion-approximation viability analysis.
//
//  * Birth-death chains on {0, ..., N} with per-state rates b_n, d_n (b_N = 0 truncation, 0 absorbing): generator Q, the transient
//    distribution p(t) = p(0) exp(Q t), the exact mean time to extinction from every start, and the quasi-stationary distribution with its
//    decay rate (the smallest eigenpair of -Q restricted to the transient states, by inverse iteration). Gillespie simulation lives in
//    common::ssa (population::stochastic) and is the Monte Carlo cross-check.
//  * Linear birth-death process (per-capita rates b, d) closed forms: mean n0 e^{(b-d)t}, extinction probability by time t
//    p_ext = alpha^{n0} with alpha = d (e^{(b-d)t} - 1) / (b e^{(b-d)t} - d) (alpha = b t / (1 + b t) when b = d), ultimate min(1, (d/b)^{n0}).
//  * Galton-Watson branching: extinction by generation n and ultimate extinction as the smallest root of the offspring pgf f(s) = s.
//  * Count-based viability (diffusion approximation of ln N as Brownian motion with drift mu and variance sigma^2 per unit time):
//    probability that the first passage of ln N to the quasi-extinction threshold x_d = ln(N0 / N_c) below the start occurs by time t,
//    Phi((-x_d - mu t) / (sigma sqrt t)) + exp(-2 mu x_d / sigma^2) Phi((-x_d + mu t) / (sigma sqrt t)), ultimate probability 1 (mu <= 0) or
//    exp(-2 mu x_d / sigma^2); and the regression estimator of (mu, sigma^2) from a census series with unequal intervals (Dennis, Munholland
//    & Scott, Ecol. Monogr. 61 (1991) 115: y_i = ln(N_{i+1}/N_i)/sqrt(tau_i) regressed through the origin on sqrt(tau_i)).

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::population::birth_death {

// ---- finite birth-death chains --------------------------------------------------------------------------------------------------
// birth, death [..., N+1] (index = population size); returns Q [..., N+1, N+1] with rows summing to zero
inline Series generator(const Series &birth, const Series &death) {
  dynamics::require(birth.size(-1) == death.size(-1), "birth_death::generator: birth and death must have the same length");
  const std::int64_t m = birth.size(-1);
  const Config cfg = Config{}.with_dtype(birth.scalar_type()).with_platform(birth.platform());
  const Series down = pluss::series::eye(m, m, cfg).slice(-1, 1, m, 1).unite(std::vector<Series>{pluss::series::zeros({m, 1}, cfg)}, 1);   // Q[n, n-1]
  const Series up = pluss::series::zeros({m, 1}, cfg).unite(std::vector<Series>{pluss::series::eye(m, m, cfg).slice(-1, 0, m - 1, 1)}, 1); // Q[n, n+1]
  const Series diag = pluss::series::eye(m, cfg);
  const Series b = birth.unsqueeze(-1), d = death.unsqueeze(-1);
  return b.mul(up).add(d.mul(down)).sub(b.add(d).mul(diag));
}

inline Series transient(const Series &q, const Series &p0, double t) {
  dynamics::require(t >= 0.0, "birth_death::transient: t must be non-negative");
  return p0.unsqueeze(-2).matmul(pluss::subsp::linearalg::matrix_exp(q.mul(t))).squeeze(-2);
}

// Expected time to absorption at 0 from each start n = 1..N: tau = (-Q_T)^-1 1, [..., N]
inline Series mean_time_to_extinction(const Series &q) {
  const std::int64_t m = q.size(-1);
  const Series qt = q.slice(-2, 1, m, 1).slice(-1, 1, m, 1);
  const Series ones = qt.sum(-1, false).mul(0.0).add(1.0);
  return dynamics::solve(qt.neg(), ones);
}

struct QuasiStationary final {
  Series distribution = pluss::series::undefined(); // over n = 1..N, sums to 1: [..., N]
  Series decay_rate = pluss::series::undefined();   // mu_1: extinction rate from the QSD (mean time to extinction from it is 1 / mu_1)
};
inline QuasiStationary quasi_stationary(const Series &q, std::int64_t iterations = 400) {
  const std::int64_t m = q.size(-1);
  const Series qt = q.slice(-2, 1, m, 1).slice(-1, 1, m, 1);
  const Series inv = pluss::subsp::linearalg::inverse(qt.transpose(-1, -2).neg());
  Series x = qt.sum(-1, false).mul(0.0).add(1.0 / static_cast<double>(m - 1));
  Series growth = x.sum(-1, false).mul(0.0).add(1.0);
  for (std::int64_t i = 0; i < iterations; ++i) {
    const Series y = inv.matmul(x.unsqueeze(-1)).squeeze(-1);
    growth = y.sum(-1, false).div(x.sum(-1, false));
    x = y.div(y.sum(-1, true));
  }
  return {x, growth.reciprocal()};
}

// ---- linear birth-death process ---------------------------------------------------------------------------------------------------
inline Series linear_mean(const Series &n0, double birth, double death, const Series &t) { return n0.mul(t.mul(birth - death).exp()); }
inline Series linear_extinction_probability(const Series &n0, double birth, double death, const Series &t) {
  dynamics::require(birth > 0.0 && death >= 0.0, "linear birth-death: need b > 0 and d >= 0");
  Series alpha = pluss::series::undefined();
  if (std::abs(birth - death) < 1e-14 * birth) {
    const Series bt = t.mul(birth);
    alpha = bt.div(bt.add(1.0));
  } else {
    const Series e = t.mul(birth - death).exp();
    alpha = e.sub(1.0).mul(death).div(e.mul(birth).sub(death));
  }
  return alpha.pow(n0);
}
inline double linear_ultimate_extinction(double n0, double birth, double death) { return birth <= death ? 1.0 : std::pow(death / birth, n0); }

// ---- Galton-Watson branching ----------------------------------------------------------------------------------------------------
// offspring distribution p [..., M] (p_k = P(k children), k = 0..M-1): pgf f(s) = sum p_k s^k
inline Series offspring_pgf(const Series &p, const Series &s) {
  const std::int64_t m = p.size(-1);
  const Config cfg = Config{}.with_dtype(p.scalar_type()).with_platform(p.platform());
  const Series k = pluss::series::arange(0, m, 1, p.scalar_type()).to(p.platform());
  return s.unsqueeze(-1).pow(k).mul(p).sum(-1, false);
}
inline Series offspring_mean(const Series &p) {
  const Series k = pluss::series::arange(0, p.size(-1), 1, p.scalar_type()).to(p.platform());
  return p.mul(k).sum(-1, false);
}
// P(extinct by generation n), n = 0..generations, starting from one individual: q_{n+1} = f(q_n), q_0 = 0 -> [..., generations + 1]
inline Series extinction_by_generation(const Series &p, std::int64_t generations) {
  Series q = p.sum(-1, false).mul(0.0);
  std::vector<Series> cols{q.unsqueeze(-1)};
  for (std::int64_t i = 0; i < generations; ++i) {
    q = offspring_pgf(p, q);
    cols.push_back(q.unsqueeze(-1));
  }
  std::vector<Series> rest(cols.begin() + 1, cols.end());
  return cols[0].unite(rest, q.dim());
}
// ultimate extinction probability: the smallest fixed point of the pgf (limit of the iteration above; 1 when the mean offspring <= 1)
inline Series ultimate_extinction(const Series &p, std::int64_t max_iterations = 100000, double tolerance = 1e-15) {
  Series q = p.sum(-1, false).mul(0.0);
  for (std::int64_t i = 0; i < max_iterations; ++i) {
    const Series next = offspring_pgf(p, q);
    const double d = next.sub(q).abs().max().item<double>();
    q = next;
    if (d < tolerance)
      break;
  }
  return q;
}

// ---- count-based viability analysis ------------------------------------------------------------------------------------------------
inline Series normal_cdf(const Series &z) { return z.mul(1.0 / std::sqrt(2.0)).erf().add(1.0).mul(0.5); }
// drift mu, variance sigma2 (per unit time), distance x_d = ln(N0 / Nc) > 0 above the threshold, horizon t > 0
inline Series quasi_extinction_cdf(const Series &mu, const Series &sigma2, const Series &x_d, const Series &t) {
  const Series s = sigma2.mul(t).sqrt();
  const Series first = normal_cdf(x_d.neg().sub(mu.mul(t)).div(s));
  const Series second = mu.mul(x_d).mul(-2.0).div(sigma2).exp().mul(normal_cdf(x_d.neg().add(mu.mul(t)).div(s)));
  return first.add(second);
}
inline Series ultimate_quasi_extinction(const Series &mu, const Series &sigma2, const Series &x_d) {
  const Series p = mu.mul(x_d).mul(-2.0).div(sigma2).exp();
  return mu.le(0.0).where(mu.mul(0.0).add(1.0), p);
}
struct DiffusionEstimate final {
  Series mu = pluss::series::undefined();
  Series sigma2 = pluss::series::undefined();
  Series mu_standard_error = pluss::series::undefined();
};
// counts N_0..N_q at times t_0..t_q (last axis): regression of y_i = ln(N_{i+1}/N_i)/sqrt(tau_i) on x_i = sqrt(tau_i) through the origin
inline DiffusionEstimate dennis_estimate(const Series &counts, const Series &times) {
  const std::int64_t q = counts.size(-1) - 1;
  dynamics::require(q >= 2 && times.size(-1) == q + 1, "dennis_estimate: need at least three censuses and matching times");
  const Series tau = times.slice(-1, 1, q + 1, 1).sub(times.slice(-1, 0, q, 1));
  const Series x = tau.sqrt();
  const Series y = counts.slice(-1, 1, q + 1, 1).div(counts.slice(-1, 0, q, 1)).log().div(x);
  const Series mu = x.mul(y).sum(-1, false).div(x.square().sum(-1, false));
  const Series resid = y.sub(mu.unsqueeze(-1).mul(x));
  DiffusionEstimate out;
  out.mu = mu;
  out.sigma2 = resid.square().sum(-1, false).div(static_cast<double>(q - 1));
  out.mu_standard_error = out.sigma2.div(x.square().sum(-1, false)).sqrt();
  return out;
}

} // namespace pluss::subsp::natsci::biology::population::birth_death
