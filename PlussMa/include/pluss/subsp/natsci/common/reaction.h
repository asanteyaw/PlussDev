#pragma once

// Mass-action reaction networks, batched.
//
//   dx/dt = N^T v(x),   v_r = k_r prod_s x_s^{o_rs}
//
// N is the net stoichiometry [R, S] (rows = reactions), o the reactant-order
// matrix [R, S]. States are [..., S] and rate constants [R] or [..., R], so a
// whole parameter sweep / ensemble evaluates in single Series ops.
// Stochastic propensities use the combinatorial form
//   a_r = c_r prod_s C(x_s, o_rs)
// (Gillespie 1977 / Wikipedia "Gillespie algorithm"), feeding
// common::ssa::gillespie unchanged.
// Steady states are found by Newton's method on the rate equations closed with
// the linear conservation laws L x = T (the same moiety-conservation
// treatment as libRoadRunner's steady-state solver, where an unreduced
// Jacobian is singular).

#include "pluss/subsp/natsci/common/dynamics.h"

namespace pluss::subsp::natsci::common::reaction {

struct Network final {
  Series stoichiometry = pluss::series::undefined(); // [R, S]
  Series orders = pluss::series::undefined();        // [R, S]
  [[nodiscard]] std::int64_t reactions() const { return stoichiometry.size(0); }
  [[nodiscard]] std::int64_t species() const { return stoichiometry.size(1); }
};

inline Network make(const Series &stoichiometry, const Series &orders) {
  dynamics::require(stoichiometry.dim() == 2 && orders.dim() == 2 &&
                        stoichiometry.size(0) == orders.size(0) &&
                        stoichiometry.size(1) == orders.size(1),
                    "network: stoichiometry and orders must be [R, S]");
  dynamics::require_finite(stoichiometry, "network: stoichiometry");
  dynamics::require_nonnegative(orders, "network: orders");
  return {stoichiometry, orders.to(stoichiometry.scalar_type())};
}

// Reaction fluxes v [..., R].
inline Series flux(const Network &net, const Series &x, const Series &k) {
  dynamics::require_species(x, net.species(), "network: state");
  const Series terms = x.unsqueeze(-2).pow(net.orders.to(x.platform()));
  return dynamics::prod_last(terms).mul(k);
}

inline Series rate(const Network &net, const Series &x, const Series &k) {
  return flux(net, x, k).matmul(net.stoichiometry.to(x.platform()));
}

// d v_r / d x_s  [..., R, S], analytic and zero-safe.
inline Series flux_jacobian(const Network &net, const Series &x,
                            const Series &k) {
  const std::int64_t s = net.species();
  const Series o = net.orders.to(x.platform());                // [R, S]
  const Series xs = x.unsqueeze(-2);                           // [..., 1, S]
  const Series base = xs.pow(o);                               // [..., R, S(t)]
  const Config cfg =
      Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
  const Series eye = pluss::series::eye(s, cfg);               // [S(s), S(t)]
  // derivative factor for t == s: o_rs x_s^{o_rs - 1}
  const Series deriv = o.mul(xs.pow(o.sub(1.0).clamp(0.0, 1e30)))
                           .mul(o.gt(0.0).to(x.scalar_type()));  // [..., R, S]
  // [..., R, S(s), S(t)] : base_t everywhere, deriv_s on the diagonal
  const Series b = base.unsqueeze(-2);
  const Series d = deriv.unsqueeze(-1);
  const Series mixed = b.mul(eye.neg().add(1.0)).add(d.mul(eye));
  // product over t, but off-diagonal slots hold base (fine) and diagonal d
  return dynamics::prod_last(mixed).mul(k.unsqueeze(-1));
}

// Jacobian of the rate equations: [..., S, S] (row = d xdot_i, col = x_j).
inline Series jacobian(const Network &net, const Series &x, const Series &k) {
  return net.stoichiometry.to(x.platform()).transpose(0, 1)
      .matmul(flux_jacobian(net, x, k));
}

// Stochastic mass-action propensities a [B, R] for population::stochastic.
inline Series propensity(const Network &net, const Series &x, const Series &c) {
  dynamics::require_species(x, net.species(), "network: state");
  const std::int64_t max_order =
      static_cast<std::int64_t>(std::llround(net.orders.max().item<double>()));
  const Series o = net.orders.to(x.platform());
  const Series xs = x.unsqueeze(-2); // [B, 1, S]
  // C(x, o) = prod_{i=0}^{o-1} (x - i) / o!; masked per order so one loop
  // over the (tiny) maximum order covers every reaction at once.
  Series comb = xs.mul(0.0).add(1.0).add(o.mul(0.0));
  double factorial = 1.0;
  for (std::int64_t i = 0; i < max_order; ++i) {
    factorial *= static_cast<double>(i + 1);
    const Series active = o.gt(static_cast<double>(i)).to(x.scalar_type());
    const Series term = xs.sub(static_cast<double>(i)).clamp(0.0, 1e300);
    comb = comb.mul(active.mul(term).add(active.neg().add(1.0)));
  }
  // divide each reaction's product by o_rs!  (per-entry factorial)
  const Series fact = o.add(1.0).lgamma().exp();
  return dynamics::prod_last(comb.div(fact)).mul(c);
}

// prod_s x_s^{o_ms} for M order rows at once, in log space: one matmul and one exp
// instead of an elementwise power over [..., M, S] followed by a product reduction.
// `orders_t` is the transposed order matrix [S, M]. States are clamped to a tiny
// positive floor, so use this only for non-negative states (concentrations):
// a zero state gives exactly 1 for order 0 and ~1e-300 (zero flux) for order >= 1.
inline Series positive_products(const Series &orders_t, const Series &x) {
  return x.clamp(1e-300, 1e300).log().matmul(orders_t).exp();
}

struct SteadyState final {
  Series state = pluss::series::undefined();
  Series residual = pluss::series::undefined(); // max |rate| per lane
  std::int64_t iterations{0};
  bool converged{false};
};

// Newton on  [rate(x); L x - T] = 0 via damped normal equations; each step is
// shortened lane-wise to keep species positive (fraction-to-boundary 0.9).
inline SteadyState steady_state(const Network &net, const Series &k,
                                const Series &x0, const Series &conservation,
                                const Series &totals, double tolerance = 1e-12,
                                std::int64_t max_iterations = 100) {
  dynamics::require_positive(x0, "steady_state: initial guess");
  dynamics::require(conservation.dim() == 2 &&
                        conservation.size(1) == net.species(),
                    "steady_state: conservation matrix must be [C, S]");
  const Config cfg =
      Config{}.with_dtype(x0.scalar_type()).with_platform(x0.platform());
  const Series eye = pluss::series::eye(net.species(), cfg);
  const Series l = conservation.to(x0.scalar_type()).to(x0.platform());
  const Series ltl = l.transpose(0, 1).matmul(l);
  Series x = x0;
  SteadyState out;
  for (std::int64_t it = 0; it < max_iterations; ++it) {
    const Series f = rate(net, x, k);
    const Series j = jacobian(net, x, k);
    const Series defect = totals.sub(dynamics::matvec(l, x));
    const Series jt = j.transpose(-1, -2);
    const Series lhs = jt.matmul(j).add(ltl).add(eye.mul(1e-14));
    const Series rhs = dynamics::matvec(jt, f.neg())
                           .add(defect.matmul(l)); // L^T defect
    const Series step = dynamics::solve(lhs, rhs);
    const Series ratio = x.div(step.neg().clamp(1e-300, 1e300));
    const Series limit = step.lt(0.0).where(ratio, ratio.mul(0.0).add(1e300))
                             .amin(-1, true).mul(0.9);
    const Series alpha = limit.clamp(0.0, 1.0);
    x = x.add(step.mul(alpha));
    out.iterations = it + 1;
    out.residual = rate(net, x, k).abs().amax(-1, false);
    if (out.residual.max().item<double>() < tolerance &&
        step.abs().max().item<double>() < 1e-9) {
      out.converged = true;
      break;
    }
  }
  out.state = x;
  return out;
}

} // namespace pluss::subsp::natsci::common::reaction
