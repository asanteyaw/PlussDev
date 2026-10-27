#pragma once

// Muscle-driven limb: Hill-type muscle-tendon units (muscle.h) acting on a planar serial chain (chain.h) through joint-angle-dependent moment arms.
//
//   moment arm        r_ij(q_j) = sum_k c_ijk q_j^k          (muscle i, joint j; positive = the muscle's tension produces positive joint torque)
//   MTU length        L_i(q)    = L0_i - sum_j [ integral of r_ij from the reference angle to q_j ]       (the principle of virtual work / tendon-excursion
//                                                                                                      method: dL_i/dq_j = -r_ij)
//   MTU velocity      Ldot_i    = - sum_j r_ij qdot_j
//   joint torque      tau_j     = sum_i r_ij F_i,  F_i = F_max,i * fT_i  (tendon force)
// Coupled state: joint angles and velocities, and for every muscle its activation and normalised tendon force. All lanes broadcast: q [..., J],
// muscle states [..., M]; muscle parameters (muscle.h Params) may be M-vectors.

#include "pluss/subsp/natsci/biology/biomechanics/chain.h"
#include "pluss/subsp/natsci/biology/biomechanics/muscle.h"

namespace pluss::subsp::natsci::biology::biomechanics::musculoskeletal {

namespace chain = ::pluss::subsp::natsci::biology::biomechanics::chain;
namespace muscle = ::pluss::subsp::natsci::biology::biomechanics::muscle;

// Moment-arm polynomial coefficients: coefficients [M, J, K] (c_ij0 + c_ij1 q + ...), reference lengths L0 [M] at reference angles q_ref [J].
struct Moments final {
  Series coefficients = pluss::series::undefined(); // [M, J, K]
  Series reference_length = pluss::series::undefined(); // [M]  (muscle-tendon length at q = q_ref)
  Series reference_angle = pluss::series::undefined();  // [J]
  [[nodiscard]] std::int64_t muscles() const { return coefficients.size(0); }
  [[nodiscard]] std::int64_t joints() const { return coefficients.size(1); }
};

// constant moment arms r [M, J]
inline Moments constant_arms(const Series &arms, const Series &reference_length, const Series &reference_angle) {
  return {arms.unsqueeze(-1), reference_length, reference_angle};
}

// moment arms R(q) [..., M, J]
inline Series moment_arms(const Moments &m, const Series &q) {
  const std::int64_t k = m.coefficients.size(2);
  Series r = q.unsqueeze(-2).mul(0.0).add(m.coefficients.select(-1, 0));            // [..., M, J]
  Series power = q.unsqueeze(-2).mul(0.0).add(1.0);
  for (std::int64_t i = 1; i < k; ++i) {
    power = power.mul(q.unsqueeze(-2));
    r = r.add(power.mul(m.coefficients.select(-1, i)));
  }
  return r;
}
// L(q) [..., M] = L0 - sum_j int_{q_ref}^{q_j} r_ij ds
inline Series muscle_tendon_length(const Moments &m, const Series &q) {
  const std::int64_t k = m.coefficients.size(2);
  const Series qe = q.unsqueeze(-2), qr = m.reference_angle;
  Series integral = qe.mul(0.0);
  for (std::int64_t i = 0; i < k; ++i) {
    const double p = static_cast<double>(i + 1);
    integral = integral.add(qe.pow(p).sub(qr.pow(p)).div(p).mul(m.coefficients.select(-1, i)));
  }
  return m.reference_length.sub(integral.sum(-1, false));
}
inline Series muscle_tendon_velocity(const Moments &m, const Series &q, const Series &qd) {
  return moment_arms(m, q).mul(qd.unsqueeze(-2)).sum(-1, false).neg();
}
// joint torques [..., J] from tendon forces [..., M]
inline Series joint_torques(const Moments &m, const Series &q, const Series &tendon_force) {
  return moment_arms(m, q).mul(tendon_force.unsqueeze(-1)).sum(-2, false);
}

struct State final {
  Series q = pluss::series::undefined(), qd = pluss::series::undefined();               // [..., J]
  Series activation = pluss::series::undefined(), norm_tendon_force = pluss::series::undefined(); // [..., M]
};
// Initial muscle state in isometric equilibrium at the given activation and posture (joints at rest)
inline State equilibrium_state(const Moments &m, const muscle::Params &mp, const Series &q, const Series &activation) {
  const auto eq = muscle::isometric_equilibrium(activation, muscle_tendon_length(m, q), mp);
  return {q, q.mul(0.0), activation, eq.norm_tendon_force};
}

using Excitation = std::function<Series(double t)>; // [..., M]

struct Derivative final {
  Series q, qd, a, ft;
};
inline Derivative rate(const chain::Chain &c, const Moments &m, const muscle::Params &mp, const Series &max_force, const State &s, const Series &excitation,
                       const Series &end_force = pluss::series::undefined()) {
  const Series torque = joint_torques(m, s.q, s.norm_tendon_force.mul(max_force));
  const Series qdd = chain::forward_dynamics(c, s.q, s.qd, torque, end_force);
  const Series length = muscle_tendon_length(m, s.q), velocity = muscle_tendon_velocity(m, s.q, s.qd);
  const Series mr = muscle::state_rate(s.activation, s.norm_tendon_force, excitation, length, velocity, mp);
  return {s.qd, qdd, mr.select(-1, 0), mr.select(-1, 1)};
}

struct Path final {
  std::vector<double> times;
  std::vector<State> states;
};
inline Path simulate(const chain::Chain &c, const Moments &m, const muscle::Params &mp, const Series &max_force, const Excitation &excitation, const State &s0, double t0, double t1, std::int64_t steps) {
  dynamics::require(steps >= 1 && t1 >= t0, "musculoskeletal::simulate: need steps >= 1 and t1 >= t0");
  const double dt = (t1 - t0) / static_cast<double>(steps);
  const auto add = [](const State &s, const Derivative &d, double h) { return State{s.q.add(d.q.mul(h)), s.qd.add(d.qd.mul(h)), s.activation.add(d.a.mul(h)), s.norm_tendon_force.add(d.ft.mul(h))}; };
  Path out;
  State s = s0;
  out.times.push_back(t0);
  out.states.push_back(s);
  for (std::int64_t i = 0; i < steps; ++i) {
    const double t = t0 + static_cast<double>(i) * dt;
    const Derivative k1 = rate(c, m, mp, max_force, s, excitation(t));
    const Derivative k2 = rate(c, m, mp, max_force, add(s, k1, 0.5 * dt), excitation(t + 0.5 * dt));
    const Derivative k3 = rate(c, m, mp, max_force, add(s, k2, 0.5 * dt), excitation(t + 0.5 * dt));
    const Derivative k4 = rate(c, m, mp, max_force, add(s, k3, dt), excitation(t + dt));
    const auto comb = [&](const Series &a, const Series &b, const Series &cc, const Series &d) { return a.add(b.mul(2.0)).add(cc.mul(2.0)).add(d).mul(dt / 6.0); };
    s = {s.q.add(comb(k1.q, k2.q, k3.q, k4.q)), s.qd.add(comb(k1.qd, k2.qd, k3.qd, k4.qd)), s.activation.add(comb(k1.a, k2.a, k3.a, k4.a)), s.norm_tendon_force.add(comb(k1.ft, k2.ft, k3.ft, k4.ft))};
    out.times.push_back(t + dt);
    out.states.push_back(s);
  }
  return out;
}

} // namespace pluss::subsp::natsci::biology::biomechanics::musculoskeletal
