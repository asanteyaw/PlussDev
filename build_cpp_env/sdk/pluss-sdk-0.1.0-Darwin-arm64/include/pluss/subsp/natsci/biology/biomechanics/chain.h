#pragma once

// Planar serial multibody dynamics for limbs: an open chain of n rigid links with revolute joints, fixed at the origin, gravity along -y,
// batched over lanes (time samples, subjects, parameter sets). Joint angles q_i are RELATIVE (anatomical); absolute orientation of link i is
// theta_i = q_1 + ... + q_i measured counter-clockwise from +x.
//
//   inverse_dynamics   tau = RNEA(q, qdot, qddot [, end-point force])       recursive Newton-Euler (planar), exact
//   mass_matrix        M(q): column j = RNEA(q, 0, e_j) with gravity off
//   bias_forces        h(q, qdot) = RNEA(q, qdot, 0) with gravity on (Coriolis, centrifugal, gravity)
//   forward_dynamics   qddot = M^-1 (tau + J_end^T F_end - h)
//   energy             kinetic 1/2 qdot^T M qdot, potential sum m g y_com
//   simulate           RK4 with torque(t, q, qdot) feedback callbacks
//
// Link parameters: mass m_i, length l_i, COM distance c_i from the proximal joint along the link, moment of inertia I_i about the COM.
// External force F_end = (Fx, Fy) acts on the distal end of the last link (e.g. a ground reaction force or a held load); the joint torque tau_i is the
// torque the actuator at joint i applies to link i (counter-clockwise positive). Verified against symbolic Lagrangian derivations (sympy).

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::biomechanics::chain {

struct Chain final {
  Series mass = pluss::series::undefined();    // [n]
  Series length = pluss::series::undefined();  // [n]
  Series com = pluss::series::undefined();     // [n] distance of the centre of mass from the proximal joint
  Series inertia = pluss::series::undefined(); // [n] about the centre of mass
  double gravity{9.80665};
  [[nodiscard]] std::int64_t links() const { return mass.size(-1); }
};

inline Chain make(const Series &mass, const Series &length, const Series &com, const Series &inertia, double gravity = 9.80665) {
  dynamics::require(mass.dim() == 1 && length.dim() == 1 && com.dim() == 1 && inertia.dim() == 1 && mass.size(0) == length.size(0) && mass.size(0) == com.size(0) && mass.size(0) == inertia.size(0) && mass.size(0) >= 1,
                    "chain::make: mass, length, com and inertia must be 1-D of equal length");
  dynamics::require(mass.gt(0.0).all().item<bool>() && length.gt(0.0).all().item<bool>() && inertia.ge(0.0).all().item<bool>(), "chain::make: positive masses and lengths, non-negative inertias");
  return {mass, length, com, inertia, gravity};
}

namespace detail {
struct Vec2 {
  Series x = pluss::series::undefined(), y = pluss::series::undefined();
};
inline Series cross(const Vec2 &a, const Vec2 &b) { return a.x.mul(b.y).sub(a.y.mul(b.x)); }
inline Vec2 add(const Vec2 &a, const Vec2 &b) { return {a.x.add(b.x), a.y.add(b.y)}; }
inline Vec2 sub(const Vec2 &a, const Vec2 &b) { return {a.x.sub(b.x), a.y.sub(b.y)}; }
inline Vec2 scale(const Vec2 &a, const Series &s) { return {a.x.mul(s), a.y.mul(s)}; }

// Recursive Newton-Euler with a switch for gravity. q, qd, qdd [..., n]; end force [..., 2] or undefined.
inline Series rnea(const Chain &c, const Series &q, const Series &qd, const Series &qdd, const Series &end_force, bool with_gravity) {
  const std::int64_t n = c.links();
  dynamics::require(q.size(-1) == n && qd.size(-1) == n && qdd.size(-1) == n, "chain: q, qdot, qddot must have one entry per joint");
  const Series zero = q.select(-1, 0).mul(0.0);
  Series theta = zero, omega = zero, alpha = zero;
  Vec2 a_joint{zero, with_gravity ? zero.add(c.gravity) : zero}; // fictitious upward acceleration of the base carries the weight
  std::vector<Vec2> u(static_cast<std::size_t>(n)), a_com(static_cast<std::size_t>(n)), a_next(static_cast<std::size_t>(n));
  std::vector<Series> alpha_i(static_cast<std::size_t>(n)), mass_i, len_i, com_i;
  for (std::int64_t i = 0; i < n; ++i) {
    const std::size_t k = static_cast<std::size_t>(i);
    theta = theta.add(q.select(-1, i));
    omega = omega.add(qd.select(-1, i));
    alpha = alpha.add(qdd.select(-1, i));
    u[k] = {theta.cos(), theta.sin()};
    const Vec2 p{u[k].y.neg(), u[k].x};
    const Series w2 = omega.square(), l = c.length.select(0, i), cm = c.com.select(0, i);
    a_com[k] = sub(add(a_joint, scale(p, alpha.mul(cm))), scale(u[k], w2.mul(cm)));
    a_next[k] = sub(add(a_joint, scale(p, alpha.mul(l))), scale(u[k], w2.mul(l)));
    alpha_i[k] = alpha;
    a_joint = a_next[k];
  }
  // backward pass: F = force on link i from its proximal neighbour, T = torque on link i from it
  Vec2 f_distal{zero, zero}; // force exerted by link i on link i+1 (F_{i+1}); for the last link it is minus the external force
  if (end_force.defined())
    f_distal = {end_force.select(-1, 0).neg(), end_force.select(-1, 1).neg()};
  Series t_distal = zero;
  std::vector<Series> tau(static_cast<std::size_t>(n));
  for (std::int64_t i = n - 1; i >= 0; --i) {
    const std::size_t k = static_cast<std::size_t>(i);
    const Series m = c.mass.select(0, i), l = c.length.select(0, i), cm = c.com.select(0, i), inertia = c.inertia.select(0, i);
    const Vec2 f_prox = add(scale(a_com[k], m), f_distal);                   // F_i - F_{i+1} = m a_com
    const Vec2 r_prox = scale(u[k], cm.neg()), r_dist = scale(u[k], l.sub(cm)); // from the COM to the proximal / distal joint
    // I alpha = T_i - T_{i+1} + r_prox x F_i - r_dist x F_{i+1}
    const Series t_prox = alpha_i[k].mul(inertia).add(t_distal).sub(cross(r_prox, f_prox)).add(cross(r_dist, f_distal));
    tau[k] = t_prox;
    f_distal = f_prox;
    t_distal = t_prox;
  }
  std::vector<Series> rest;
  for (std::int64_t i = 1; i < n; ++i)
    rest.push_back(tau[static_cast<std::size_t>(i)].unsqueeze(-1));
  return tau[0].unsqueeze(-1).unite(rest, tau[0].dim());
}
} // namespace detail

// ---- public calls ---------------------------------------------------------------------------------------------------------------
inline Series inverse_dynamics(const Chain &c, const Series &q, const Series &qd, const Series &qdd, const Series &end_force = pluss::series::undefined()) {
  return detail::rnea(c, q, qd, qdd, end_force, true);
}

inline Series mass_matrix(const Chain &c, const Series &q) {
  const std::int64_t n = c.links();
  const Series zero = q.mul(0.0);
  std::vector<Series> cols;
  for (std::int64_t j = 0; j < n; ++j) {
    std::vector<double> e(static_cast<std::size_t>(n), 0.0);
    e[static_cast<std::size_t>(j)] = 1.0;
    const Series unit = zero.add(pluss::make_series(e, {n}).to(q.scalar_type()).to(q.platform()));
    cols.push_back(detail::rnea(c, q, zero, unit, pluss::series::undefined(), false).unsqueeze(-1)); // column j
  }
  std::vector<Series> rest(cols.begin() + 1, cols.end());
  return rest.empty() ? cols[0] : cols[0].unite(rest, q.dim());
}

inline Series bias_forces(const Chain &c, const Series &q, const Series &qd) {
  return detail::rnea(c, q, qd, q.mul(0.0), pluss::series::undefined(), true);
}

// J_end^T F: generalised force of an end-point force (finite-length lever arms through the chain geometry)
inline Series end_point_torques(const Chain &c, const Series &q, const Series &end_force) {
  const Series zero = q.mul(0.0);
  return detail::rnea(c, q, zero, zero, end_force, false).neg(); // with no motion and no gravity the joint torques balance the load: tau = -J^T F
}

inline Series forward_dynamics(const Chain &c, const Series &q, const Series &qd, const Series &tau, const Series &end_force = pluss::series::undefined()) {
  Series rhs = tau.sub(bias_forces(c, q, qd));
  if (end_force.defined())
    rhs = rhs.add(end_point_torques(c, q, end_force));
  return dynamics::solve(mass_matrix(c, q), rhs);
}

struct Kinematics final {
  Series joint_x, joint_y; // [..., n + 1]: base, then the distal end of each link
  Series com_x, com_y;     // [..., n]
};
inline Kinematics forward_kinematics(const Chain &c, const Series &q) {
  const std::int64_t n = c.links();
  const Series zero = q.select(-1, 0).mul(0.0);
  Series theta = zero, x = zero, y = zero;
  std::vector<Series> jx{x.unsqueeze(-1)}, jy{y.unsqueeze(-1)}, cx, cy;
  for (std::int64_t i = 0; i < n; ++i) {
    theta = theta.add(q.select(-1, i));
    const Series l = c.length.select(0, i), cm = c.com.select(0, i);
    cx.push_back(x.add(theta.cos().mul(cm)).unsqueeze(-1));
    cy.push_back(y.add(theta.sin().mul(cm)).unsqueeze(-1));
    x = x.add(theta.cos().mul(l));
    y = y.add(theta.sin().mul(l));
    jx.push_back(x.unsqueeze(-1));
    jy.push_back(y.unsqueeze(-1));
  }
  const auto cat = [&](std::vector<Series> v) {
    std::vector<Series> rest(v.begin() + 1, v.end());
    return rest.empty() ? v[0] : v[0].unite(rest, v[0].dim() - 1);
  };
  return {cat(jx), cat(jy), cat(cx), cat(cy)};
}

struct Energy final {
  Series kinetic = pluss::series::undefined(), potential = pluss::series::undefined();
  [[nodiscard]] Series total() const { return kinetic.add(potential); }
};
inline Energy energy(const Chain &c, const Series &q, const Series &qd) {
  const Series m = mass_matrix(c, q);
  const Series ke = qd.unsqueeze(-2).matmul(m).matmul(qd.unsqueeze(-1)).squeeze(-1).squeeze(-1).mul(0.5);
  const Kinematics k = forward_kinematics(c, q);
  return {ke, k.com_y.mul(c.mass).sum(-1, false).mul(c.gravity)};
}

using Torque = std::function<Series(double t, const Series &q, const Series &qd)>;
struct Path final {
  std::vector<double> times;
  std::vector<Series> q, qd;
};
inline Path simulate(const Chain &c, const Torque &torque, const Series &q0, const Series &qd0, double t0, double t1, std::int64_t steps,
                     const std::function<Series(double t, const Series &q)> &end_force = nullptr) {
  dynamics::require(steps >= 1 && t1 >= t0, "chain::simulate: need steps >= 1 and t1 >= t0");
  const double dt = (t1 - t0) / static_cast<double>(steps);
  const auto accel = [&](double t, const Series &q, const Series &qd) {
    return forward_dynamics(c, q, qd, torque(t, q, qd), end_force ? end_force(t, q) : pluss::series::undefined());
  };
  Path out;
  Series q = q0, qd = qd0;
  out.times.push_back(t0);
  out.q.push_back(q);
  out.qd.push_back(qd);
  for (std::int64_t s = 0; s < steps; ++s) {
    const double t = t0 + static_cast<double>(s) * dt;
    const Series k1q = qd, k1v = accel(t, q, qd);
    const Series k2q = qd.add(k1v.mul(0.5 * dt)), k2v = accel(t + 0.5 * dt, q.add(k1q.mul(0.5 * dt)), qd.add(k1v.mul(0.5 * dt)));
    const Series k3q = qd.add(k2v.mul(0.5 * dt)), k3v = accel(t + 0.5 * dt, q.add(k2q.mul(0.5 * dt)), qd.add(k2v.mul(0.5 * dt)));
    const Series k4q = qd.add(k3v.mul(dt)), k4v = accel(t + dt, q.add(k3q.mul(dt)), qd.add(k3v.mul(dt)));
    q = q.add(k1q.add(k2q.mul(2.0)).add(k3q.mul(2.0)).add(k4q).mul(dt / 6.0));
    qd = qd.add(k1v.add(k2v.mul(2.0)).add(k3v.mul(2.0)).add(k4v).mul(dt / 6.0));
    out.times.push_back(t + dt);
    out.q.push_back(q);
    out.qd.push_back(qd);
  }
  return out;
}

} // namespace pluss::subsp::natsci::biology::biomechanics::chain
