#pragma once

// Hill-type muscle-tendon unit, batched over lanes (muscles, activations, lengths, time points).
//
// Model: De Groote, Kinney, Rao & Fregly (2016), "Evaluation of direct collocation optimal control problem formulations for solving the muscle
// redundancy problem", Ann. Biomed. Eng. (reference as cited in the OpenSim source; the paper itself was not obtained). The curves, constants and the explicit-
// tendon-dynamics equations below were read from OpenSim's implementation (opensim-core OpenSim/Actuators/DeGrooteFregly2016Muscle.{h,cpp},
// sha256 prefixes c430bfc2 / 704304cd), which documents where it deviates from the paper supplement (b11, c2, d1, d4, the passive curve).
//
//   active force-length   f_act(l)   = sum_{k=1..3} b1k exp(-(x - b2k)^2 / (2 (b3k + b4k x)^2)),  x = (l - 1) / width_scale + 1
//   passive force-length  f_pas(l)   = (exp(kPE (l - 1) / e0) - exp(kPE (0.2 - 1) / e0)) / (exp(kPE) - exp(kPE (0.2 - 1) / e0)),  kPE = 4
//   force-velocity        f_v(v)     = d1 asinh(d2 v + d3) + d4,  v in [-1, 1] (normalised fiber velocity, shortening negative)
//   tendon force-length   f_T(l_T)   = c1 exp(kT (l_T - c2)) - c3,  c1 = 0.2, c2 = 1, c3 = 0.2,  kT = ln((1 + c3)/c1) / (1 + strain_1 - c2)
//   activation dynamics   da/dt      = [ (f + 1/2)/(tau_a (1/2 + 3a/2)) + (-f + 1/2)(1/2 + 3a/2)/tau_d ] (e - a),  f = tanh(b (e - a))/2
//   fiber geometry        constant width w = l_opt sin(alpha_0):  l_fiber = sqrt(l_along^2 + w^2),  cos(alpha) = l_along / l_fiber
//   equilibrium           f_T = cos(alpha) (a f_act f_v + f_pas + damping v)   (all forces normalised by F_max)
// State of a dynamic muscle: activation a and normalised tendon force f_T; the explicit tendon dynamics give df_T/dt = f_T'(l_T) v_T / l_ts with the
// fiber velocity inverted from the force-velocity curve (valid for a > 0 and f_act > 0 on the supported fiber range 0.2 - 1.8 l_opt).

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>
#include <memory>

namespace pluss::subsp::natsci::biology::biomechanics::muscle {

using dynamics::Coeff;

namespace detail {
inline Series gaussian_like(const Series &x, double b1, double b2, double b3, double b4) {
  const Series d = x.sub(b2), w = x.mul(b4).add(b3);
  return d.square().div(w.square().mul(2.0)).neg().exp().mul(b1);
}
// kT = ln((1 + c3) / c1) / (1 + strain_1 - c2) = ln 6 / strain_1 with c1 = c3 = 0.2, c2 = 1
inline Series tendon_k(const Coeff &strain_at_one, const Series &like) { return strain_at_one.on(like).reciprocal().mul(std::log(6.0)); }
} // namespace detail

// Alternative curve family (e.g. the Millard quintic-Bezier curves of millard.h). When Params::curves is set the muscle functions below use it instead of the
// De Groote-Fregly closed forms; all functions are elementwise on normalised quantities.
struct Curves final {
  std::function<Series(const Series &norm_fiber_length)> active;
  std::function<Series(const Series &norm_fiber_length)> passive;
  std::function<Series(const Series &norm_fiber_velocity)> force_velocity;
  std::function<Series(const Series &force_velocity_multiplier)> force_velocity_inverse;
  std::function<Series(const Series &norm_fiber_velocity)> force_velocity_slope; // optional (d f_v / d v): speeds up the damped velocity solve; bisection is used without it
  std::function<Series(const Series &norm_tendon_force)> tendon_length_from_force;
  std::function<Series(const Series &norm_tendon_length)> tendon_stiffness;
};

struct Params final {
  Coeff max_isometric_force{1000.0};   // F_max [N]
  Coeff optimal_fiber_length{0.1};     // l_opt [m]
  Coeff tendon_slack_length{0.2};      // l_ts [m]
  Coeff pennation_at_optimal{0.0};     // alpha_0 [rad]
  Coeff max_contraction_velocity{10.0}; // v_max [l_opt / s]
  Coeff activation_time{0.015};        // tau_a [s]
  Coeff deactivation_time{0.060};      // tau_d [s]
  Coeff tendon_strain_at_one{0.049};   // tendon strain at normalised force 1
  Coeff passive_strain_at_one{0.6};    // fiber strain at normalised passive force 1
  Coeff fiber_damping{0.0};            // normalised fiber damping coefficient
  Coeff width_scale{1.0};              // horizontal scale of the active force-length curve
  double activation_smoothing{0.1};    // b of the tanh switch between activation and deactivation
  std::shared_ptr<const Curves> curves{}; // empty: the closed-form De Groote-Fregly curves
};

// ---- curves (elementwise over any shape) ----------------------------------------------------------------------------------------
inline Series active_force_length(const Series &norm_fiber_length, const Coeff &width_scale = 1.0) {
  const Series x = norm_fiber_length.sub(1.0).div(width_scale.on(norm_fiber_length)).add(1.0);
  return detail::gaussian_like(x, 0.8150671134243542, 1.055033428970575, 0.162384573599574, 0.063303448465465)
      .add(detail::gaussian_like(x, 0.433004984392647, 0.716775413397760, -0.029947116970696, 0.200356847296188))
      .add(detail::gaussian_like(x, 0.1, 1.0, 0.353553390593274, 0.0));
}
inline Series passive_force_length(const Series &norm_fiber_length, const Coeff &strain_at_one = 0.6) {
  const double k = 4.0;
  const Series e0 = strain_at_one.on(norm_fiber_length);
  const Series offset = e0.reciprocal().mul(k * (0.2 - 1.0)).exp();
  return norm_fiber_length.sub(1.0).mul(k).div(e0).exp().sub(offset).div(offset.neg().add(std::exp(k)));
}
inline Series force_velocity(const Series &norm_fiber_velocity) {
  const double d1 = -0.3211346127989808, d2 = -8.149, d3 = -0.374, d4 = 0.8825327733249912;
  const Series t = norm_fiber_velocity.mul(d2).add(d3);
  return t.add(t.square().add(1.0).sqrt()).log().mul(d1).add(d4);
}
inline Series force_velocity_inverse(const Series &fv_multiplier) {
  const double d1 = -0.3211346127989808, d2 = -8.149, d3 = -0.374, d4 = 0.8825327733249912;
  return fv_multiplier.sub(d4).div(d1).sinh().sub(d3).div(d2);
}
// normalised tendon force from normalised tendon length, its inverse and its slope
inline Series tendon_force_length(const Series &norm_tendon_length, const Coeff &strain_at_one = 0.049) {
  return norm_tendon_length.sub(1.0).mul(detail::tendon_k(strain_at_one, norm_tendon_length)).exp().mul(0.2).sub(0.2);
}
inline Series tendon_length_from_force(const Series &norm_tendon_force, const Coeff &strain_at_one = 0.049) {
  return norm_tendon_force.add(0.2).div(0.2).log().div(detail::tendon_k(strain_at_one, norm_tendon_force)).add(1.0);
}
inline Series tendon_stiffness(const Series &norm_tendon_length, const Coeff &strain_at_one = 0.049) { // d f_T / d l_T
  const Series kt = detail::tendon_k(strain_at_one, norm_tendon_length);
  return norm_tendon_length.sub(1.0).mul(kt).exp().mul(kt).mul(0.2);
}

namespace detail {
[[nodiscard]] inline Series active(const Params &p, const Series &l) { return p.curves ? p.curves->active(l) : active_force_length(l, p.width_scale); }
[[nodiscard]] inline Series passive(const Params &p, const Series &l) { return p.curves ? p.curves->passive(l) : passive_force_length(l, p.passive_strain_at_one); }
[[nodiscard]] inline Series fv(const Params &p, const Series &v) { return p.curves ? p.curves->force_velocity(v) : force_velocity(v); }
[[nodiscard]] inline Series fv_inverse(const Params &p, const Series &f) { return p.curves ? p.curves->force_velocity_inverse(f) : force_velocity_inverse(f); }
[[nodiscard]] inline Series fv_slope(const Params &p, const Series &v) {
  if (p.curves) return p.curves->force_velocity_slope ? p.curves->force_velocity_slope(v) : v.mul(0.0).add(-1.0); // -1: "unknown", the solver then bisects
  const double d1 = -0.3211346127989808, d2 = -8.149, d3 = -0.374;
  return v.mul(d2).add(d3).square().add(1.0).sqrt().reciprocal().mul(d1 * d2);
}
// Fibre velocity from the force balance  a f_act f_v(v) + beta v = rhs  (normalised forces; beta = fiber damping). beta = 0 is the closed-form inverse; otherwise
// the left side is strictly increasing in v and is solved on all lanes by Newton inside a bisection bracket [-4, 4].
[[nodiscard]] inline Series damped_velocity(const Params &p, const Series &rhs, const Series &active_force) {
  Series v = fv_inverse(p, rhs.div(active_force));
  const Series beta = p.fiber_damping.on(rhs);
  if (beta.abs().max().item<double>() == 0.0) return v;
  Series lo = rhs.mul(0.0).add(-4.0), hi = rhs.mul(0.0).add(4.0);
  v = v.clamp(-4.0, 4.0);
  for (int it = 0; it < 60; ++it) {
    const Series g = active_force.mul(fv(p, v)).add(beta.mul(v)).sub(rhs);
    if (it % 3 == 2 && g.abs().max().item<double>() < 1e-14 * (1.0 + rhs.abs().max().item<double>())) break;
    const Series above = g.gt(0.0);
    hi = above.where(v, hi);
    lo = above.where(lo, v);
    const Series slope = active_force.mul(fv_slope(p, v)).add(beta);
    const Series newton = v.sub(g.div(slope.clamp(1e-12, 1e300)));
    const Series usable = fv_slope(p, v).ge(0.0).logical_and(newton.gt(lo)).logical_and(newton.lt(hi));
    v = usable.where(newton, lo.add(hi).mul(0.5));
  }
  return v;
}
[[nodiscard]] inline Series tendon_length(const Params &p, const Series &f) { return p.curves ? p.curves->tendon_length_from_force(f) : tendon_length_from_force(f, p.tendon_strain_at_one); }
[[nodiscard]] inline Series tendon_slope(const Params &p, const Series &lt) { return p.curves ? p.curves->tendon_stiffness(lt) : tendon_stiffness(lt, p.tendon_strain_at_one); }
} // namespace detail

// ---- activation dynamics -------------------------------------------------------------------------------------------------------
inline Series activation_rate(const Series &activation, const Series &excitation, const Params &p) {
  const Series z = activation.mul(1.5).add(0.5);
  const Series f = excitation.sub(activation).mul(p.activation_smoothing).tanh().mul(0.5);
  const Series ta = p.activation_time.on(activation), td = p.deactivation_time.on(activation);
  const Series rate = f.add(0.5).div(ta.mul(z)).add(f.neg().add(0.5).mul(z).div(td));
  return rate.mul(excitation.sub(activation));
}

// ---- geometry and forces ---------------------------------------------------------------------------------------------------------
struct Geometry final {
  Series tendon_length = pluss::series::undefined();       // [m]
  Series fiber_length = pluss::series::undefined();        // [m]
  Series norm_fiber_length = pluss::series::undefined();
  Series cos_pennation = pluss::series::undefined();
};
// from the muscle-tendon length and the normalised tendon force
inline Geometry geometry(const Series &mtu_length, const Series &norm_tendon_force, const Params &p) {
  Geometry g;
  const Series lts = p.tendon_slack_length.on(mtu_length), lopt = p.optimal_fiber_length.on(mtu_length);
  g.tendon_length = detail::tendon_length(p, norm_tendon_force).mul(lts);
  const Series along = mtu_length.sub(g.tendon_length);
  const Series width = lopt.mul(p.pennation_at_optimal.on(mtu_length).sin());
  g.fiber_length = along.square().add(width.square()).sqrt();
  g.norm_fiber_length = g.fiber_length.div(lopt);
  g.cos_pennation = along.div(g.fiber_length);
  return g;
}
// normalised fiber force (total, along the fiber) at the given activation, normalised fiber length and normalised fiber velocity
inline Series fiber_force(const Series &activation, const Series &norm_fiber_length, const Series &norm_fiber_velocity, const Params &p) {
  return activation.mul(detail::active(p, norm_fiber_length)).mul(detail::fv(p, norm_fiber_velocity))
      .add(detail::passive(p, norm_fiber_length)).add(norm_fiber_velocity.mul(p.fiber_damping.on(activation)));
}

// Isometric (zero fiber velocity) equilibrium of fiber and tendon at a given muscle-tendon length: the normalised tendon force f in [0, 5] that
// solves f - cos(alpha(f)) (a f_act f_pas-sum) = 0, by vectorised bisection (the residual is monotone in f over the supported range).
struct Equilibrium final {
  Series norm_tendon_force = pluss::series::undefined();
  Series norm_fiber_length = pluss::series::undefined();
  Series tendon_length = pluss::series::undefined();
  Series force = pluss::series::undefined(); // [N] along the tendon
  Series residual = pluss::series::undefined(); // max |f - cos(alpha) f_fiber| over lanes (diagnostic)
};
inline Equilibrium isometric_equilibrium(const Series &activation, const Series &mtu_length, const Params &p, int iterations = 80) {
  Series lo = activation.mul(0.0), hi = activation.mul(0.0).add(5.0);
  const auto residual_at = [&](const Series &f) {
    const Geometry g = geometry(mtu_length, f, p);
    return f.sub(g.cos_pennation.mul(fiber_force(activation, g.norm_fiber_length, activation.mul(0.0), p)));
  };
  for (int i = 0; i < iterations; ++i) {
    const Series mid = lo.add(hi).mul(0.5);
    const Series positive = residual_at(mid).gt(0.0);
    hi = positive.where(mid, hi);
    lo = positive.where(lo, mid);
  }
  Equilibrium e;
  e.norm_tendon_force = lo.add(hi).mul(0.5);
  const Geometry g = geometry(mtu_length, e.norm_tendon_force, p);
  e.norm_fiber_length = g.norm_fiber_length;
  e.tendon_length = g.tendon_length;
  e.force = e.norm_tendon_force.mul(p.max_isometric_force.on(activation));
  e.residual = residual_at(e.norm_tendon_force).abs();
  return e;
}

// ---- dynamics ----------------------------------------------------------------------------------------------------------------------
// d/dt of [activation, normalised tendon force] given the excitation, muscle-tendon length and velocity at this instant.
// Returns [..., 2]. Needs a > 0 (the fiber velocity is recovered by dividing by the active force).
inline Series state_rate(const Series &activation, const Series &norm_tendon_force, const Series &excitation, const Series &mtu_length, const Series &mtu_velocity, const Params &p) {
  const Geometry g = geometry(mtu_length, norm_tendon_force, p);
  const Series v_fiber_norm = detail::damped_velocity(p, norm_tendon_force.div(g.cos_pennation).sub(detail::passive(p, g.norm_fiber_length)), activation.mul(detail::active(p, g.norm_fiber_length)));
  const Series v_fiber = v_fiber_norm.mul(p.max_contraction_velocity.on(activation)).mul(p.optimal_fiber_length.on(activation));
  const Series v_tendon = mtu_velocity.sub(v_fiber.div(g.cos_pennation));
  const Series v_tendon_norm = v_tendon.div(p.tendon_slack_length.on(activation));
  const Series dtendon = v_tendon_norm.mul(detail::tendon_slope(p, detail::tendon_length(p, norm_tendon_force)));
  return activation_rate(activation, excitation, p).unsqueeze(-1).unite(std::vector<Series>{dtendon.unsqueeze(-1)}, activation.dim());
}

// RK4 with time-varying inputs: excitation(t), mtu_length(t), mtu_velocity(t) are callbacks returning Series broadcastable with the state.
using Signal = std::function<Series(double t)>;
struct Path final {
  std::vector<double> times;
  std::vector<Series> activation, norm_tendon_force; // per time point, [...]
};
inline Path simulate(const Params &p, const Signal &excitation, const Signal &mtu_length, const Signal &mtu_velocity, const Series &activation0, const Series &norm_tendon_force0, double t0, double t1, std::int64_t steps) {
  dynamics::require(steps >= 1 && t1 >= t0, "muscle::simulate: need steps >= 1 and t1 >= t0");
  const double dt = (t1 - t0) / static_cast<double>(steps);
  const auto f = [&](double t, const Series &a, const Series &ft) {
    return state_rate(a, ft, excitation(t), mtu_length(t), mtu_velocity(t), p);
  };
  Path out;
  Series a = activation0, ft = norm_tendon_force0;
  out.times.push_back(t0);
  out.activation.push_back(a);
  out.norm_tendon_force.push_back(ft);
  const auto part = [&](const Series &k, std::int64_t i) { return k.select(-1, i); };
  for (std::int64_t s = 0; s < steps; ++s) {
    const double t = t0 + static_cast<double>(s) * dt;
    const Series k1 = f(t, a, ft);
    const Series k2 = f(t + 0.5 * dt, a.add(part(k1, 0).mul(0.5 * dt)), ft.add(part(k1, 1).mul(0.5 * dt)));
    const Series k3 = f(t + 0.5 * dt, a.add(part(k2, 0).mul(0.5 * dt)), ft.add(part(k2, 1).mul(0.5 * dt)));
    const Series k4 = f(t + dt, a.add(part(k3, 0).mul(dt)), ft.add(part(k3, 1).mul(dt)));
    a = a.add(part(k1, 0).add(part(k2, 0).mul(2.0)).add(part(k3, 0).mul(2.0)).add(part(k4, 0)).mul(dt / 6.0));
    ft = ft.add(part(k1, 1).add(part(k2, 1).mul(2.0)).add(part(k3, 1).mul(2.0)).add(part(k4, 1)).mul(dt / 6.0));
    out.times.push_back(t + dt);
    out.activation.push_back(a);
    out.norm_tendon_force.push_back(ft);
  }
  return out;
}

} // namespace pluss::subsp::natsci::biology::biomechanics::muscle
