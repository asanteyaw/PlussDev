#pragma once

// Local parameter sensitivities and practical identifiability for ODE models dy/dt = f(y, p).
//
//   forward sensitivity:  S = dy/dp  obeys  dS/dt = J_y S + J_p,  S(0) = 0 (fixed initial state)
// integrated together with the state as ONE augmented system z = [y, vec S] of size n + n P (RK4 through
// the shared dynamics engine). The Jacobians J_y [..., n, n] and J_p [..., n, P] each come from a single
// batched central-difference evaluation of f (all perturbations of all lanes in one call).
//
//   Convention: f(y, p) receives y [..., n] (or [..., K, n] during a Jacobian sweep) and p [..., P] (or
//   [..., 1, P] / [..., P, P]); read parameters as p.select(-1, i) so they broadcast against the lane and
//   perturbation axes.
//
// Identifiability: with observation times t_k and unit noise, the Fisher information is
//   FIM = sum_k S_k^T S_k  (scale columns by 1/sigma for weighted data),  Cramer-Rao covariance >= FIM^{-1};
// eigenvalues of the FIM (singular values of the stacked sensitivity matrix, squared) give the numerical
// rank: parameter combinations no data can separate appear as vanishing eigenvalues.

#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::common::sensitivity {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;

using ParamRateFn = std::function<Series(const Series &y, const Series &p)>;

struct Sensitivities final {
  Series times = pluss::series::undefined();  // [samples + 1]
  Series states = pluss::series::undefined(); // [samples + 1, ..., n]
  Series dydp = pluss::series::undefined();   // [samples + 1, ..., n, P]
};

// Trajectory of the state and its sensitivities dy/dp. `step` is the relative finite-difference step
// for the two Jacobians (central, O(step^2)); components smaller than 1e-3 use an absolute step.
inline Sensitivities trajectory(const ParamRateFn &f, const Series &y0, const Series &p, double t0, double t1,
                                std::int64_t samples, std::int64_t steps_per_sample = 50, double step = 1e-6) {
  dynamics::require(static_cast<bool>(f), "sensitivity: rate callback is required");
  dynamics::require(y0.dim() >= 1 && p.dim() >= 1, "sensitivity: y0 and p need a species / parameter axis");
  dynamics::require(step > 0.0, "sensitivity: step must be positive");
  const std::int64_t n = y0.size(-1), np = p.size(-1);
  std::vector<std::int64_t> lead, s_shape, flat_shape;
  for (std::int64_t i = 0; i + 1 < y0.dim(); ++i)
    lead.push_back(y0.size(i));
  s_shape = lead;
  s_shape.push_back(n);
  s_shape.push_back(np);
  flat_shape = lead;
  flat_shape.push_back(n * np);

  const Series pstep = p.abs().clamp_min(1e-3).mul(step); // [..., P]
  auto rate = [&](const Series &z) {
    const Series y = z.slice(-1, 0, n, 1);
    const Series s = z.slice(-1, n, n + n * np, 1).reshape(s_shape); // [..., n, P]
    const dynamics::RateFn fy = [&](const Series &yy) { return f(yy, p.unsqueeze(-2)); };
    const dynamics::RateFn fp = [&](const Series &pp) { return f(y.unsqueeze(-2), pp); };
    const Series jy = dynamics::jacobian_scaled(fy, y, y.abs().clamp_min(1e-3).mul(step)); // [..., n, n]
    const Series jp = dynamics::jacobian_scaled(fp, p, pstep);                              // [..., n, P]
    const Series sdot = jy.matmul(s).add(jp).reshape(flat_shape);
    return f(y, p).unite(std::vector<Series>{sdot}, -1);
  };
  const Series z0 = y0.unite(std::vector<Series>{y0.slice(-1, 0, 1, 1).mul(0.0).expand(flat_shape)}, -1);
  const auto path = dynamics::trajectory(rate, z0, t0, t1, samples, steps_per_sample);
  std::vector<std::int64_t> d_shape{samples + 1};
  d_shape.insert(d_shape.end(), s_shape.begin(), s_shape.end());
  return {path.times, path.states.slice(-1, 0, n, 1), path.states.slice(-1, n, n + n * np, 1).reshape(d_shape)};
}

// Fisher information from stacked sensitivities [T, ..., n, P]: sum over observation times and outputs.
inline Series fisher_information(const Series &dydp) {
  dynamics::require(dydp.dim() >= 3, "fisher_information: need [T, ..., n, P]");
  return dydp.transpose(-1, -2).matmul(dydp).sum(0, false); // [..., P, P]
}

// Eigenvalues of the symmetric PSD FIM, descending: [..., P].
inline Series fisher_spectrum(const Series &fim) { return pluss::subsp::linearalg::svd_s(fim, false); }

// Numerical rank: number of eigenvalues above rel_tol * largest, [...].
inline Series numerical_rank(const Series &spectrum, double rel_tol = 1e-8) {
  const Series top = spectrum.amax(-1, true);
  return spectrum.ge(top.mul(rel_tol)).to(spectrum.scalar_type()).sum(-1, false);
}

// Cramer-Rao lower bound on each parameter's standard deviation, sqrt(diag(FIM^{-1})): [..., P].
inline Series cramer_rao_std(const Series &fim) {
  const Series inv = pluss::subsp::linearalg::inverse(fim);
  const std::int64_t np = fim.size(-1);
  const Config cfg = Config{}.with_dtype(fim.scalar_type()).with_platform(fim.platform());
  return inv.mul(pluss::series::eye(np, cfg)).sum(-1, false).sqrt();
}

// Dimensionless (relative) sensitivities d ln y_i / d ln p_j = (p_j / y_i) S_ij: [..., n, P].
inline Series normalized(const Series &dydp, const Series &y, const Series &p) {
  return dydp.mul(p.unsqueeze(-2)).div(y.unsqueeze(-1));
}

} // namespace pluss::subsp::natsci::common::sensitivity
