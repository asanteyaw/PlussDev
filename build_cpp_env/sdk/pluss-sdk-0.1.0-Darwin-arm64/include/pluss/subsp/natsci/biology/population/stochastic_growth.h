#pragma once

// Population growth in a fluctuating environment (Lewontin & Cohen 1969; Lande 1993; Lande, Engen & Saether 2003 -- none obtained) and with demographic
// noise (Feller 1951), built on the SDE solver `math::sde::fluctua` (Euler-Maruyama / Milstein / Stratonovich-Heun, lanes batched): no integrator is written here.
//
//   density-independent  dN = r N dt + s N dW           ln N_t ~ Normal(ln N0 + (r - s^2/2) t, s^2 t),  stochastic growth rate r - s^2/2 (extinction w.p. 1 iff r < s^2/2)
//   stochastic logistic   dN = r N (1 - N/K) dt + s N dW  in x = ln N:  dx = [r (1 - e^x/K) - s^2/2] dt + s dW      (additive noise: Euler-Maruyama is then
//                          strongly accurate and N stays positive); stationary density Gamma(shape 2r/s^2 - 1, rate 2r/(K s^2)) for s^2 < 2r,
//                          mean K (1 - s^2/(2r)), variance mean / rate
//   demographic (Feller)  dN = r N dt + sqrt(sigma_d^2 N) dW:  P(N_t = 0) = exp(-2 r N0 / (sigma_d^2 (1 - e^{-rt})))
// `crossing_probability` turns coarse path samples into an almost noise-free first-passage estimator: between two samples above the barrier b the probability that the
// Brownian bridge dipped below b is exp(-2 (x_i - b)(x_{i+1} - b) / (s^2 dt)) (exact for additive noise, drift irrelevant), so the probability of having crossed is
// 1 - prod (1 - p_i); averaging it over lanes needs far fewer lanes than counting crossings.

#include "pluss/subsp/math/random_processes.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>
#include <numbers>

namespace pluss::subsp::natsci::biology::population::stochastic_growth {

namespace sde = ::pluss::subsp::math::sde;

// ---- density-independent --------------------------------------------------------------------------------------------------------------------
[[nodiscard]] constexpr double stochastic_growth_rate(double r, double s) noexcept { return r - 0.5 * s * s; }
[[nodiscard]] inline Series lognormal_mean(const Series &n0, double r, double t) { return n0.mul(std::exp(r * t)); }
[[nodiscard]] inline Series lognormal_variance(const Series &n0, double r, double s, double t) { return n0.square().mul(std::exp(2.0 * r * t) * std::expm1(s * s * t)); }

// ---- stochastic logistic in log space --------------------------------------------------------------------------------------------------------
struct Logistic final {
  double r{1.0};
  double k{100.0};
  double s{0.3}; // environmental standard deviation (Ito)
  [[nodiscard]] sde::DriftFn drift() const {
    return [*this](double, const Series &x, const Series &) { return x.exp().div(k).neg().add(1.0).mul(r).sub(0.5 * s * s); };
  }
  [[nodiscard]] sde::DiffusionFn diffusion() const {
    return [*this](double, const Series &x, const Series &) { return x.mul(0.0).add(s); };
  }
  [[nodiscard]] constexpr bool persistent() const noexcept { return s * s < 2.0 * r; } // a stationary density exists
  [[nodiscard]] constexpr double shape() const noexcept { return 2.0 * r / (s * s) - 1.0; }
  [[nodiscard]] constexpr double rate() const noexcept { return 2.0 * r / (k * s * s); }
  [[nodiscard]] constexpr double mean() const noexcept { return shape() / rate(); }                 // K (1 - s^2 / (2 r))
  [[nodiscard]] constexpr double variance() const noexcept { return shape() / (rate() * rate()); }
};

struct Paths final {
  Series x = pluss::series::undefined(); // [blocks + 1, lanes...] samples of the log population at t0 + i * block_time
  double block_time{0.0};
};
// Advance in `blocks` blocks of `steps_per_block` solver steps; every block draws independent noise. Returns the log-population samples.
[[nodiscard]] inline Paths simulate(const sde::DriftFn &drift, const sde::DiffusionFn &diffusion, const Series &x0, double t_end, std::int64_t blocks, std::int64_t steps_per_block,
                                    std::uint64_t seed = 1, sde::SdeMethod method = sde::SdeMethod::EulerMaruyama) {
  dynamics::require(blocks >= 1 && steps_per_block >= 1 && t_end > 0.0, "stochastic_growth::simulate: need blocks, steps_per_block >= 1 and t_end > 0");
  const double dt = t_end / static_cast<double>(blocks);
  std::vector<Series> samples{x0};
  Series x = x0;
  for (std::int64_t b = 0; b < blocks; ++b) {
    sde::SdeOptions opt{.method = method, .steps = steps_per_block, .seed = seed + 7919ULL * static_cast<std::uint64_t>(b + 1)};
    x = sde::fluctua(drift, diffusion, x, static_cast<double>(b) * dt, static_cast<double>(b + 1) * dt, pluss::series::undefined(), opt).y_final;
    samples.push_back(x);
  }
  return {.x = samples.front().pile(std::vector<Series>(samples.begin() + 1, samples.end()), 0), .block_time = dt};
}

// per-lane probability that the path has crossed below the barrier b (log scale) by the last sample, from coarse samples [T + 1, lanes...] with constant diffusion s
[[nodiscard]] inline Series crossing_probability(const Series &x, double barrier, double s, double block_time) {
  const std::int64_t t = x.size(0);
  const Series above = x.gt(barrier);
  const Series a = x.sub(barrier);
  const Series pair = a.slice(0, 0, t - 1, 1).mul(a.slice(0, 1, t, 1)).mul(-2.0 / (s * s * block_time)).exp();     // bridge dip probability
  const Series both = above.slice(0, 0, t - 1, 1).logical_and(above.slice(0, 1, t, 1));
  const Series stay = both.where(pair.neg().add(1.0), pair.mul(0.0));                                                // lanes already below keep stay = 0 on that interval
  const Series start_ok = above.select(0, 0).to(x.scalar_type());
  return stay.cumprod(0, false).select(0, t - 2).mul(start_ok).neg().add(1.0);
}

// ---- demographic noise -------------------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline Series feller_extinction_probability(const Series &n0, double r, double sigma_d2, double t) {
  const double decay = -std::expm1(-r * t);
  return n0.mul(-2.0 * r / (sigma_d2 * decay)).exp();
}

} // namespace pluss::subsp::natsci::biology::population::stochastic_growth
