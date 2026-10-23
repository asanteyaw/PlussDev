#pragma once

// Legged-locomotion templates, batched over lanes (parameter sets, initial apex states).
//
//  * Spring-loaded inverted pendulum (SLIP) for running/hopping: a point mass m on a massless linear leg spring (stiffness k, rest length L0)
//    with a fixed touchdown angle of attack alpha0 (leg angle above the ground). Flight is ballistic (exact parabola), stance integrates
//        m x'' = k (L0 - L) (x - x_foot) / L,   m y'' = k (L0 - L) y / L - m g,   L = |(x - x_foot, y)|
//    with RK4 and a secant-refined liftoff time (leg length returns to L0 with the mass moving away from the ground).
//    `slip_apex_map(y_apex, v_x)` returns the next apex (y, v_x), the stance duration and the peak leg force; the template and the fixed-angle-of-attack
//    control are those of Blickhan (1989), Seyfarth et al. (2002), Geyer, Seyfarth & Blickhan (2005); none was obtained, the model is verified by
//    energy conservation and by an independent DOP853 integration with event detection.
//  * Linear inverted pendulum (walking, Kajita-style; Hof 2008 extrapolated centre of mass / capture point): with the CoM at constant height z0 over a
//    point foot,  x'' = (g / z0) x,  time constant Tc = sqrt(z0 / g), closed forms x(t) = x0 cosh(t/Tc) + Tc v0 sinh(t/Tc), v(t) = x0 sinh(t/Tc)/Tc + v0 cosh(t/Tc),
//    conserved orbital energy  v^2 - x^2 / Tc^2,  capture point  x + Tc v  (the foot position that brings the pendulum to rest over the foot).

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::biomechanics::locomotion {

using dynamics::Coeff;

// ---- linear inverted pendulum ------------------------------------------------------------------------------------------------------
inline Series lipm_time_constant(const Coeff &height, const Series &like, double gravity = 9.80665) { return height.on(like).div(gravity).sqrt(); }
struct Lipm final {
  Series x = pluss::series::undefined(), v = pluss::series::undefined();
};
inline Lipm lipm_state(const Series &x0, const Series &v0, const Series &t, const Series &tc) {
  const Series c = t.div(tc).cosh(), s = t.div(tc).sinh();
  return {x0.mul(c).add(tc.mul(v0).mul(s)), x0.mul(s).div(tc).add(v0.mul(c))};
}
inline Series lipm_orbital_energy(const Series &x, const Series &v, const Series &tc) { return v.square().sub(x.div(tc).square()); }
inline Series lipm_capture_point(const Series &x, const Series &v, const Series &tc) { return x.add(tc.mul(v)); }
// time for the pendulum to reach position x1 (> x0 side permitting), or NaN if it never does: from x1 = x0 cosh + Tc v0 sinh
inline Series lipm_time_to_position(const Series &x0, const Series &v0, const Series &x1, const Series &tc) {
  // x(t) = A e^{t/Tc} + B e^{-t/Tc},  A = (x0 + Tc v0)/2, B = (x0 - Tc v0)/2  =>  A u^2 - x1 u + B = 0 with u = e^{t/Tc} >= 1
  const Series a = x0.add(tc.mul(v0)).mul(0.5), b = x0.sub(tc.mul(v0)).mul(0.5);
  const Series disc = x1.square().sub(a.mul(b).mul(4.0));
  const Series root = disc.clamp(0.0, 1e300).sqrt();
  const Series u1 = x1.add(root).div(a.mul(2.0)), u2 = x1.sub(root).div(a.mul(2.0));
  const Series nan = x0.mul(0.0).add(std::numeric_limits<double>::quiet_NaN());
  const Series u = u1.ge(1.0).logical_and(u2.ge(1.0)).where(u1.minimum(u2), u1.ge(1.0).where(u1, u2.ge(1.0).where(u2, nan)));
  return u.log().mul(tc);
}

// ---- SLIP ---------------------------------------------------------------------------------------------------------------------------
struct SlipParams final {
  Coeff mass{80.0};
  Coeff stiffness{20000.0};
  Coeff rest_length{1.0};
  Coeff attack_angle{1.2};  // leg angle above the ground at touchdown [rad]
  double gravity{9.80665};
};
struct SlipStep final {
  Series apex_height = pluss::series::undefined();  // y at the next apex
  Series apex_speed = pluss::series::undefined();   // v_x at the next apex
  Series stance_time = pluss::series::undefined();
  Series peak_force = pluss::series::undefined();   // maximum leg force [N]
  Series valid = pluss::series::undefined();        // Bool: touchdown happened, stance ended in liftoff, apex reached
};

namespace detail {
struct Stance {
  Series x, y, vx, vy;
};
inline void stance_accel(const SlipParams &p, const Series &xf, const Series &x, const Series &y, Series &ax, Series &ay, Series &force) {
  const Series dx = x.sub(xf);
  const Series len = dx.square().add(y.square()).sqrt();
  const Series f = p.rest_length.on(x).sub(len).mul(p.stiffness.on(x));
  force = f;
  ax = f.mul(dx).div(len).div(p.mass.on(x));
  ay = f.mul(y).div(len).div(p.mass.on(x)).sub(p.gravity);
}
inline Stance rk4(const SlipParams &p, const Series &xf, const Stance &s, const Series &dt) {
  Series ax, ay, f;
  const auto acc = [&](const Stance &q) { Series a, b, c; stance_accel(p, xf, q.x, q.y, a, b, c); return std::pair<Series, Series>{a, b}; };
  const auto k1a = acc(s);
  const Stance s2{s.x.add(s.vx.mul(dt.mul(0.5))), s.y.add(s.vy.mul(dt.mul(0.5))), s.vx.add(k1a.first.mul(dt.mul(0.5))), s.vy.add(k1a.second.mul(dt.mul(0.5)))};
  const auto k2a = acc(s2);
  const Stance s3{s.x.add(s2.vx.mul(dt.mul(0.5))), s.y.add(s2.vy.mul(dt.mul(0.5))), s.vx.add(k2a.first.mul(dt.mul(0.5))), s.vy.add(k2a.second.mul(dt.mul(0.5)))};
  const auto k3a = acc(s3);
  const Stance s4{s.x.add(s3.vx.mul(dt)), s.y.add(s3.vy.mul(dt)), s.vx.add(k3a.first.mul(dt)), s.vy.add(k3a.second.mul(dt))};
  const auto k4a = acc(s4);
  const Series sixth = dt.div(6.0);
  return {s.x.add(s.vx.add(s2.vx.mul(2.0)).add(s3.vx.mul(2.0)).add(s4.vx).mul(sixth)), s.y.add(s.vy.add(s2.vy.mul(2.0)).add(s3.vy.mul(2.0)).add(s4.vy).mul(sixth)),
          s.vx.add(k1a.first.add(k2a.first.mul(2.0)).add(k3a.first.mul(2.0)).add(k4a.first).mul(sixth)), s.vy.add(k1a.second.add(k2a.second.mul(2.0)).add(k3a.second.mul(2.0)).add(k4a.second).mul(sixth))};
}
} // namespace detail

// One apex-to-apex step from an apex at height y_apex and horizontal speed v_x (the apex x position is irrelevant). dt: stance integration step [s].
inline SlipStep slip_apex_map(const SlipParams &p, const Series &y_apex, const Series &v_x, double dt = 2e-4, std::int64_t max_steps = 20000) {
  const Series l0 = p.rest_length.on(y_apex), alpha = p.attack_angle.on(y_apex);
  const Series y_td = l0.mul(alpha.sin());
  const Series flies = y_apex.gt(y_td);
  const Series tf = y_apex.sub(y_td).clamp(0.0, 1e300).mul(2.0 / p.gravity).sqrt();
  // touchdown state; the foot sits L0 cos(alpha) ahead of the mass
  detail::Stance s{y_apex.mul(0.0), y_td, v_x, tf.mul(-p.gravity)};
  s.x = v_x.mul(tf);
  const Series xf = s.x.add(l0.mul(alpha.cos()));
  Series t = y_apex.mul(0.0), peak = y_apex.mul(0.0), liftoff_t = y_apex.mul(0.0);
  Series done = flies.logical_not();
  const Series dts = y_apex.mul(0.0).add(dt);
  const auto leg_length = [&](const detail::Stance &q) { return q.x.sub(xf).square().add(q.y.square()).sqrt(); };
  for (std::int64_t i = 0; i < max_steps && !done.all().item<bool>(); ++i) {
    const detail::Stance next = detail::rk4(p, xf, s, dts);
    const Series len_next = leg_length(next), len_prev = leg_length(s);
    const Series radial_out = next.x.sub(xf).mul(next.vx).add(next.y.mul(next.vy));
    const Series active = done.logical_not();
    const Series lifted = active.logical_and(len_next.ge(l0)).logical_and(radial_out.gt(0.0)).logical_and(t.gt(0.0));
    if (lifted.any().item<bool>()) {
      // partial step h in (0, dt] landing exactly at L = L0: secant/Newton on L(h) - L0 with RK4 sub-steps from the pre-step state
      Series h = lifted.where(len_prev.sub(l0).neg().div(len_next.sub(len_prev)), len_prev.mul(0.0)).clamp(0.0, 1.0).mul(dt);
      for (int it = 0; it < 5; ++it) {
        const detail::Stance trial = detail::rk4(p, xf, s, h);
        const Series radial = trial.x.sub(xf).mul(trial.vx).add(trial.y.mul(trial.vy)).div(leg_length(trial)); // dL/dt
        h = lifted.where(h.sub(leg_length(trial).sub(l0).div(radial.clamp(1e-9, 1e300))), h).clamp(0.0, dt);
      }
      const detail::Stance landed = detail::rk4(p, xf, s, h);
      liftoff_t = lifted.where(t.add(h), liftoff_t);
      s = {lifted.where(landed.x, s.x), lifted.where(landed.y, s.y), lifted.where(landed.vx, s.vx), lifted.where(landed.vy, s.vy)};
    }
    const Series running = active.logical_and(lifted.logical_not());
    Series ax, ay, force;
    detail::stance_accel(p, xf, next.x, next.y, ax, ay, force);
    peak = running.where(peak.maximum(force), peak);
    s = {running.where(next.x, s.x), running.where(next.y, s.y), running.where(next.vx, s.vx), running.where(next.vy, s.vy)};
    t = running.where(t.add(dt), t);
    done = done.logical_or(lifted);
  }
  SlipStep out;
  out.valid = flies.logical_and(done).logical_and(s.vy.gt(0.0));
  const Series rise = s.vy.clamp(0.0, 1e300);
  out.apex_height = s.y.add(rise.square().div(2.0 * p.gravity));
  out.apex_speed = s.vx;
  out.stance_time = liftoff_t;
  out.peak_force = peak;
  return out;
}

} // namespace pluss::subsp::natsci::biology::biomechanics::locomotion
