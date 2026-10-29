#pragma once

// Metabolic control analysis (MCA) of a network at steady state, batched over lanes.
//
// Stoichiometry N [S, R] (rows species, columns reactions), rate vector v(x) [..., R], elasticity matrix E = dv/dx [..., R, S].
// With C independent moiety conservations Gamma [C, S] (Gamma N = 0) the unscaled control coefficients solve
//
//     N E C^S = -N ,   Gamma C^S = 0         (concentration control, [S, R])
//     C^J     = I + E C^S                    (flux control, [R, R])
//
// which is the link-matrix result C^S = -L (N_R E L)^-1 N_R of COPASI's MCA task (CMCAMethod.cpp, read, sha afeb4f85cb57) written without
// choosing independent rows: we solve the (consistent, full-column-rank) system through its normal equations
// (E^T N^T N E + Gamma^T Gamma) C^S = -E^T N^T N, then refine once. Scaled coefficients, response coefficients to parameters and the
// summation / connectivity theorems are provided as separate functions so each can be audited on its own.
//
// Elasticities default to central differences with relative steps; a model that knows its analytic E (mass action:
// common::reaction::flux_jacobian) passes it straight to `coefficients`.

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::metabolism::control {

namespace dynamics = ::pluss::subsp::natsci::biology::dynamics;
using RateFn = dynamics::RateFn; // x [..., S] -> v [..., R]

// d v_r / d x_s [..., R, S], central difference with step rel * max(|x_s|, floor)
inline Series elasticities(const RateFn &v, const Series &x, double rel = 1e-6, double floor = 1e-12) {
  dynamics::require(rel > 0.0, "control::elasticities: relative step must be positive");
  const Series step = x.abs().clamp(floor, 1e300).mul(rel);
  return dynamics::jacobian_scaled(v, x, step);
}

// d v_r / d p_q [..., R, P] for v(x, p); p [..., P]. v receives x [..., K, S] and p [..., K, P] (K perturbed copies) and returns [..., K, R].
inline Series parameter_derivatives(const std::function<Series(const Series &, const Series &)> &v, const Series &x, const Series &p,
                                    double rel = 1e-6, double floor = 1e-12) {
  const std::int64_t np = p.size(-1);
  const Config cfg = Config{}.with_dtype(p.scalar_type()).with_platform(p.platform());
  const Series step = p.abs().clamp(floor, 1e300).mul(rel);
  const Series e = pluss::series::eye(np, cfg).mul(step.unsqueeze(-1));        // [..., P(q), P]
  const Series base = p.unsqueeze(-2);                                          // [..., 1, P]
  const Series xs = x.unsqueeze(-2).add(e.sum(-1, true).mul(0.0));              // [..., P, S] (x repeated per perturbation)
  const Series columns = v(xs, base.add(e)).sub(v(xs, base.sub(e))).div(step.mul(2.0).unsqueeze(-1)); // [..., P, R]
  return columns.transpose(-1, -2);
}

// Orthonormal basis of the left null space of N (moiety conservation laws), Gamma [C, S]; empty rows (C = 0) when none.
inline Series conservation_laws(const Series &stoichiometry, double tolerance = 1e-10) {
  dynamics::require(stoichiometry.dim() == 2, "control::conservation_laws: stoichiometry must be [S, R]");
  const auto dec = ::pluss::subsp::linearalg::svd(stoichiometry, true);
  const auto sv = dec.s.contiguous().host<double>();
  std::int64_t rank = 0;
  const double cut = tolerance * (sv.empty() ? 1.0 : std::max(1.0, sv.front()));
  for (double s : sv)
    if (s > cut)
      ++rank;
  return dec.u.slice(1, rank, stoichiometry.size(0), 1).transpose(0, 1).contiguous(); // [S - rank, S]
}

struct Coefficients final {
  Series concentration = pluss::series::undefined(); // C^S [..., S, R], unscaled
  Series flux = pluss::series::undefined();          // C^J [..., R, R], unscaled
};

// Unscaled concentration and flux control coefficients. `conservation` is Gamma [C, S] (pass an undefined Series when N E is invertible).
inline Coefficients coefficients(const Series &stoichiometry, const Series &elasticity, const Series &conservation = pluss::series::undefined()) {
  dynamics::require(stoichiometry.dim() == 2 && elasticity.dim() >= 2 && elasticity.size(-1) == stoichiometry.size(0) && elasticity.size(-2) == stoichiometry.size(1),
                    "control::coefficients: N must be [S, R] and E [..., R, S]");
  const Series n = stoichiometry.to(elasticity.scalar_type()).to(elasticity.platform());
  const Series ne = n.matmul(elasticity);                       // [..., S, S]
  const Series net = ne.transpose(-1, -2);
  Series m = net.matmul(ne);
  if (conservation.defined() && conservation.size(0) > 0) {
    const Series g = conservation.to(elasticity.scalar_type()).to(elasticity.platform());
    m = m.add(g.transpose(0, 1).matmul(g));
  }
  const Series inv = pluss::subsp::linearalg::inverse(m);
  const Series rhs = net.matmul(n).neg();                       // -E^T N^T N  [..., S, R]
  Series cs = inv.matmul(rhs);
  cs = cs.add(inv.matmul(rhs.sub(m.matmul(cs))));               // one step of iterative refinement
  const Config cfg = Config{}.with_dtype(elasticity.scalar_type()).with_platform(elasticity.platform());
  Coefficients out;
  out.concentration = cs;
  out.flux = elasticity.matmul(cs).add(pluss::series::eye(stoichiometry.size(1), cfg));
  return out;
}

// ---- scaling ----------------------------------------------------------------------------------------------------------------
// eps_rs = (x_s / v_r) dv_r/dx_s
inline Series scale_elasticities(const Series &e, const Series &x, const Series &v) { return e.mul(x.unsqueeze(-2)).div(v.unsqueeze(-1)); }
// C^{S_k}_j = (v_j / x_k) dS_k ...
inline Series scale_concentration(const Series &cs, const Series &x, const Series &v) { return cs.mul(v.unsqueeze(-2)).div(x.unsqueeze(-1)); }
// C^{J_i}_j = (v_j / J_i) ...
inline Series scale_flux(const Series &cj, const Series &v) { return cj.mul(v.unsqueeze(-2)).div(v.unsqueeze(-1)); }

// Response coefficients R = C dv/dp (unscaled): [..., S, P] and [..., R, P]
inline Series response(const Series &control_coefficients, const Series &dv_dp) { return control_coefficients.matmul(dv_dp); }

// ---- theorems (maximum absolute residual per lane, for auditing) ---------------------------------------------------------------
// Summation: sum_j C^{J_i}_j = 1 and sum_j C^{S_k}_j = 0 (scaled coefficients).
inline Series summation_defect(const Series &scaled_flux, const Series &scaled_concentration) {
  const Series f = scaled_flux.sum(-1, false).sub(1.0).abs().amax(-1, false);
  const Series c = scaled_concentration.sum(-1, false).abs().amax(-1, false);
  return f.maximum(c);
}
// Connectivity on the free (non-conserved) directions B = ker Gamma: C^J E B = 0 and C^S E B = -B (unscaled, B [S, S - C]).
inline Series connectivity_defect(const Coefficients &c, const Series &elasticity, const Series &free_directions) {
  const Series b = free_directions.to(elasticity.scalar_type()).to(elasticity.platform());
  const Series eb = elasticity.matmul(b);
  const Series f = c.flux.matmul(eb).abs().amax(-1, false).amax(-1, false);
  const Series s = c.concentration.matmul(eb).add(b).abs().amax(-1, false).amax(-1, false);
  return f.maximum(s);
}
// Orthonormal basis of ker Gamma, [S, S - C].
inline Series free_directions(const Series &conservation) {
  const auto dec = ::pluss::subsp::linearalg::svd(conservation, true);
  return dec.vh.slice(0, conservation.size(0), conservation.size(1), 1).transpose(0, 1).contiguous();
}

// ---- steady state for a general rate law ---------------------------------------------------------------------------------
struct SteadyState final {
  Series state = pluss::series::undefined();
  Series residual = pluss::series::undefined(); // max |N v| per lane
  std::int64_t iterations{0};
  bool converged{false};
};
// Damped Newton on [N v(x); Gamma x - T] = 0 (normal equations; the step is shortened per lane to keep species positive).
inline SteadyState steady_state(const RateFn &v, const Series &stoichiometry, const Series &x0, const Series &conservation, const Series &totals,
                                double tolerance = 1e-12, std::int64_t max_iterations = 100) {
  dynamics::require_positive(x0, "control::steady_state: initial guess");
  const Config cfg = Config{}.with_dtype(x0.scalar_type()).with_platform(x0.platform());
  const Series n = stoichiometry.to(x0.scalar_type()).to(x0.platform());
  const bool has_g = conservation.defined() && conservation.size(0) > 0;
  const Series g = has_g ? conservation.to(x0.scalar_type()).to(x0.platform()) : pluss::series::undefined();
  const Series eye = pluss::series::eye(x0.size(-1), cfg);
  const RateFn rate = [&](const Series &x) { return dynamics::matvec(n, v(x)); }; // N v, [..., S]
  Series x = x0;
  SteadyState out;
  for (std::int64_t it = 0; it < max_iterations; ++it) {
    const Series f = rate(x);
    const Series j = dynamics::jacobian_scaled(rate, x, x.abs().clamp(1e-12, 1e300).mul(1e-7)); // [..., S, S]
    const Series jt = j.transpose(-1, -2);
    Series lhs = jt.matmul(j).add(eye.mul(1e-14));
    Series rhs = dynamics::matvec(jt, f.neg());
    if (has_g) {
      lhs = lhs.add(g.transpose(0, 1).matmul(g));
      rhs = rhs.add(totals.sub(dynamics::matvec(g, x)).matmul(g));
    }
    const Series step = dynamics::solve(lhs, rhs);
    const Series ratio = x.div(step.neg().clamp(1e-300, 1e300));
    const Series limit = step.lt(0.0).where(ratio, ratio.mul(0.0).add(1e300)).amin(-1, true).mul(0.9);
    x = x.add(step.mul(limit.clamp(0.0, 1.0)));
    out.iterations = it + 1;
    out.residual = rate(x).abs().amax(-1, false);
    if (out.residual.max().item<double>() < tolerance && step.abs().max().item<double>() < 1e-9) {
      out.converged = true;
      break;
    }
  }
  out.state = x;
  return out;
}

} // namespace pluss::subsp::natsci::biology::metabolism::control
