#pragma once

// Linear compartment models  dx/dt = K x + u  (K: [..., n, n], x: [..., n]).
// Exact propagation through the matrix exponential, batched over every leading
// axis and over a time grid in ONE call: K [B, n, n] and times [T] give the
// [B, T, n, n] propagators from a single batched expm.
//
//   x(t)         = expm(K t) x0                                  (bolus / initial amounts)
//   infusion     : augmented [[K, u],[0, 0]] -> x(t) = [expm(A t)]_{x,1} (rate u held constant)
//   AUC_0^inf    = -K^{-1} x0                                    (K non-singular: elimination present)
//   steady-state repeated bolus at interval tau: x_ss(trough) = (I - expm(K tau))^{-1} expm(K tau) d
// Rate-constant builder: K = T^T - diag(rowsum(T) + k_el) for transfer rates T_ij (i -> j) and
// elimination k_el (Rowland & Tozer, Clinical Pharmacokinetics, mass-balance form).

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::pharmacokinetics {

// Transfer rates `transfer` [..., n, n] (entry (i, j) = rate of i -> j, diagonal ignored) and
// elimination `elimination` [..., n] (loss from compartment i out of the system).
inline Series rate_matrix(const Series &transfer, const Series &elimination) {
  dynamics::require_square(transfer, "rate_matrix: transfer");
  dynamics::require_nonnegative(transfer, "rate_matrix: transfer");
  dynamics::require_nonnegative(elimination, "rate_matrix: elimination");
  const std::int64_t n = transfer.size(-1);
  dynamics::require(elimination.size(-1) == n, "rate_matrix: elimination must have one entry per compartment");
  const Config cfg = Config{}.with_dtype(transfer.scalar_type()).with_platform(transfer.platform());
  const Series off = transfer.mul(pluss::series::eye(n, cfg).neg().add(1.0)); // zero the diagonal
  const Series loss = off.sum(-1, false).add(elimination);                    // [..., n]
  return off.transpose(-1, -2).sub(pluss::series::eye(n, cfg).mul(loss.unsqueeze(-1)));
}

// Propagators expm(K t) for every (lane, time): K [..., n, n], times [T] -> [..., T, n, n].
inline Series propagator(const Series &k, const Series &times) {
  dynamics::require_square(k, "propagator: K");
  dynamics::require(times.dim() == 1, "propagator: times must be 1-D");
  const Series kt = k.unsqueeze(-3).mul(times.to(k.scalar_type()).to(k.platform()).unsqueeze(-1).unsqueeze(-1));
  return pluss::subsp::linearalg::matrix_exp(kt);
}

// Amounts at each time after an initial bolus x0 [..., n]: [..., T, n].
inline Series response(const Series &k, const Series &x0, const Series &times) {
  dynamics::require(x0.size(-1) == k.size(-1), "response: x0 size mismatch");
  return propagator(k, times).matmul(x0.unsqueeze(-2).unsqueeze(-1)).squeeze(-1);
}

// Amounts under a constant input rate u [..., n] starting from x0: exact via the augmented exponential.
inline Series infusion_response(const Series &k, const Series &x0, const Series &u, const Series &times) {
  dynamics::require_square(k, "infusion_response: K");
  const std::int64_t n = k.size(-1);
  // A = [[K, u], [0, 0]]  (n+1 square); built by concatenation, no element writes.
  const Series top = k.unite(std::vector<Series>{u.unsqueeze(-1)}, -1);   // [..., n, n+1]
  const Series bottom = top.slice(-2, 0, 1, 1).mul(0.0);                    // [..., 1, n+1]
  const Series a = top.unite(std::vector<Series>{bottom}, -2);              // [..., n+1, n+1]
  const Series z0 = x0.unite(std::vector<Series>{x0.slice(-1, 0, 1, 1).mul(0.0).add(1.0)}, -1);
  return response(a, z0, times).slice(-1, 0, n, 1);
}

// Total drug exposure AUC_0^inf per compartment, [..., n]: -K^{-1} x0.
inline Series auc_infinity(const Series &k, const Series &x0) {
  return dynamics::solve(k, x0).neg();
}

// Trough amounts at steady state for repeated bolus doses d [..., n] every tau (scalar interval).
inline Series steady_state_trough(const Series &k, const Series &dose, double tau) {
  dynamics::require(tau > 0.0, "steady_state_trough: tau must be positive");
  dynamics::require_square(k, "steady_state_trough: K");
  const std::int64_t n = k.size(-1);
  const Config cfg = Config{}.with_dtype(k.scalar_type()).with_platform(k.platform());
  const Series e = pluss::subsp::linearalg::matrix_exp(k.mul(tau));
  const Series m = pluss::series::eye(n, cfg).sub(e);
  return dynamics::solve(m, dynamics::matvec(e, dose));
}

// One-compartment oral absorption (Bateman): central amount for first-order absorption ka and
// elimination ke, x_c(t) = dose ka/(ka - ke) (exp(-ke t) - exp(-ka t)); the ka -> ke limit is
// dose ka t exp(-ka t). Coefficients broadcast against t.
inline Series bateman(const Series &t, const dynamics::Coeff &dose, const dynamics::Coeff &ka,
                      const dynamics::Coeff &ke) {
  dynamics::require_positive(ka, t, "bateman: ka");
  dynamics::require_positive(ke, t, "bateman: ke");
  const Series a = ka.on(t), e = ke.on(t), d = dose.on(t);
  const Series diff = a.sub(e);
  const Series degenerate = diff.abs().lt(1e-9);
  const Series safe = degenerate.where(diff.mul(0.0).add(1.0), diff);
  const Series regular = d.mul(a).div(safe).mul(t.mul(e.neg()).exp().sub(t.mul(a.neg()).exp()));
  const Series limit = d.mul(a).mul(t).mul(t.mul(a.neg()).exp());
  return degenerate.where(limit, regular);
}

// Two-compartment (central 1, peripheral 2) disposition rates: macro-constants alpha > beta from
// k10, k12, k21: alpha + beta = k10 + k12 + k21, alpha beta = k10 k21 (Gibaldi & Perrier).
struct Macro final {
  Series alpha = pluss::series::undefined();
  Series beta = pluss::series::undefined();
};
inline Macro two_compartment_macro(const Series &k10, const Series &k12, const Series &k21) {
  const Series s = k10.add(k12).add(k21), p = k10.mul(k21);
  const Series disc = s.square().sub(p.mul(4.0)).sqrt();
  return {s.add(disc).mul(0.5), s.sub(disc).mul(0.5)};
}

// Clearance CL = V k_el and terminal half-life ln2 / lambda.
inline Series half_life(const Series &lambda) { return lambda.reciprocal().mul(std::log(2.0)); }

} // namespace pluss::subsp::natsci::biology::pharmacokinetics
