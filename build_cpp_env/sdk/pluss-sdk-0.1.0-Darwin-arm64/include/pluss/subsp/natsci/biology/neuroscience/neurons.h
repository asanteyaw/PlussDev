#pragma once

// Point-neuron models, batched over lanes (a lane = one neuron or parameter set), plus a shared
// hybrid (threshold-and-reset) simulator.
//
//  lif           tau dV/dt = -(V - E_L) + R I; spike at V >= V_th, reset to V_r.
//                Exact interspike interval for constant drive (RI + E_L > V_th):
//                    T = tau ln( (RI + E_L - V_r) / (RI + E_L - V_th) ).
//  izhikevich    dv/dt = 0.04 v^2 + 5 v + 140 - u + I,  du/dt = a (b v - u);
//                if v >= 30 mV: v <- c, u <- u + d.  Equations, 30 mV threshold, reset rule and the
//                regular-spiking defaults (a,b,c,d) = (0.02, 0.2, -65, 8) as implemented in NEST
//                (models/izhikevich.cpp / .h, read from GitHub); the 2003 paper itself (IEEE Trans.
//                Neural Netw. 14:1569) was not obtainable.
//  fitzhugh_nagumo  dv/dt = v - v^3/3 - w + I,  dw/dt = eps (v + a - b w); the standard two-variable
//                reduction of Hodgkin-Huxley (form from secondary sources; the 1961 paper was not read).
//                Its Hopf points follow from tr J = 0 at the equilibrium: v* = +-sqrt(1 - eps b).
//
// Spike times use linear interpolation of the threshold crossing inside the step, and after a
// reset each spiking lane is advanced by the remaining sub-step time, so the spike-train timing
// error does not accumulate as O(dt) per spike.

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <functional>
#include <limits>

namespace pluss::subsp::natsci::biology::neuroscience {

using dynamics::Coeff;

inline Series lif_rate(const Series &v, const Coeff &tau, const Coeff &e_leak, const Coeff &resistance,
                       const Coeff &current) {
  return v.neg().add(e_leak.on(v)).add(resistance.on(v).mul(current.on(v))).div(tau.on(v));
}
// Exact interspike interval; infinity where the drive is subthreshold.
inline Series lif_interspike_interval(const Coeff &tau, const Coeff &e_leak, const Coeff &resistance,
                                      const Coeff &current, const Coeff &v_threshold, const Coeff &v_reset,
                                      const Series &like) {
  const Series drive = resistance.on(like).mul(current.on(like)).add(e_leak.on(like));
  const Series num = drive.sub(v_reset.on(like)), den = drive.sub(v_threshold.on(like));
  const Series fires = den.gt(0.0);
  const Series safe = fires.where(den, den.mul(0.0).add(1.0));
  return fires.where(tau.on(like).mul(num.div(safe).log()), like.mul(0.0).add(std::numeric_limits<double>::infinity()));
}

struct IzhikevichParams final {
  Coeff a{0.02}, b{0.2}, c{-65.0}, d{8.0};
  double threshold{30.0};
};
// state [..., 2] = (v, u)
inline Series izhikevich_rate(const Series &s, const Coeff &current, const IzhikevichParams &p = {}) {
  dynamics::require_species(s, 2, "izhikevich: state");
  const Series v = s.select(-1, 0), u = s.select(-1, 1);
  const Series dv = v.square().mul(0.04).add(v.mul(5.0)).add(140.0).sub(u).add(current.on(v));
  const Series du = p.a.on(v).mul(p.b.on(v).mul(v).sub(u));
  return dv.unsqueeze(-1).unite(std::vector<Series>{du.unsqueeze(-1)}, -1);
}

// state [..., 2] = (v, w)
inline Series fitzhugh_nagumo_rate(const Series &s, const Coeff &current, const Coeff &a = 0.7,
                                   const Coeff &b = 0.8, const Coeff &epsilon = 0.08) {
  dynamics::require_species(s, 2, "fitzhugh_nagumo: state");
  const Series v = s.select(-1, 0), w = s.select(-1, 1);
  const Series dv = v.sub(v.pow(3.0).div(3.0)).sub(w).add(current.on(v));
  const Series dw = epsilon.on(v).mul(v.add(a.on(v)).sub(b.on(v).mul(w)));
  return dv.unsqueeze(-1).unite(std::vector<Series>{dw.unsqueeze(-1)}, -1);
}

// Hopf points of FitzHugh-Nagumo: the input currents where the equilibrium's trace vanishes.
inline Series fitzhugh_nagumo_hopf_currents(double a, double b, double epsilon, const Series &like) {
  const double vs = std::sqrt(1.0 - epsilon * b);
  const auto current = [&](double v) { return -v + v * v * v / 3.0 + (v + a) / b; };
  return pluss::make_series(std::vector<double>{current(-vs), current(vs)}, {2}).to(like.scalar_type()).to(like.platform());
}

struct SpikeOptions final {
  double dt{0.05};
  double t_end{100.0};
  std::int64_t max_spikes{64};   // recorded per lane
  std::int64_t voltage_index{0}; // component compared with the threshold
  double threshold{30.0};
};

struct SpikeResult final {
  Series state = pluss::series::undefined();  // [..., n] at t_end
  Series count = pluss::series::undefined();  // [...] spikes fired
  Series times = pluss::series::undefined();  // [..., max_spikes]; unused slots are -1
};

// Shared hybrid simulator. `reset(state, spiked)` returns the post-spike state for lanes with
// spiked = 1 (it must leave the other lanes unchanged).
using ResetFn = std::function<Series(const Series &state, const Series &spiked)>;

inline SpikeResult simulate_spiking(const dynamics::RateFn &rate, const ResetFn &reset, Series state,
                                    const SpikeOptions &opt = {}) {
  dynamics::require(opt.dt > 0.0 && opt.t_end > 0.0, "spiking: dt and t_end must be positive");
  const std::int64_t steps = static_cast<std::int64_t>(std::llround(opt.t_end / opt.dt));
  const Series lead = state.select(-1, 0);
  const Config cfg = Config{}.with_dtype(state.scalar_type()).with_platform(state.platform());
  Series count = lead.mul(0.0);
  Series times = lead.mul(0.0).unsqueeze(-1).add(pluss::series::zeros({opt.max_spikes}, cfg).add(-1.0));
  const Series slots = pluss::series::arange(0, opt.max_spikes, 1, state.scalar_type()).to(state.platform());
  for (std::int64_t k = 0; k < steps; ++k) {
    const double t0 = static_cast<double>(k) * opt.dt;
    const Series before = state;
    const Series v_before = state.select(-1, opt.voltage_index);
    Series next = dynamics::simulate(rate, state, t0, t0 + opt.dt, 1);
    const Series v_after = next.select(-1, opt.voltage_index);
    const Series spiked = v_after.ge(opt.threshold).logical_and(v_before.lt(opt.threshold));
    // linear interpolation of the crossing time inside the step
    const Series dv = v_after.sub(v_before);
    const Series flat = dv.abs().lt(1e-300);
    const Series frac = flat.where(dv.mul(0.0), v_before.neg().add(opt.threshold)).div(flat.where(dv.mul(0.0).add(1.0), dv));
    const Series t_cross = frac.clamp(0.0, 1.0).mul(opt.dt).add(t0);
    const Series slot_hit = slots.eq(count.unsqueeze(-1)).to(state.scalar_type()).mul(spiked.unsqueeze(-1).to(state.scalar_type()));
    times = slot_hit.gt(0.0).where(t_cross.unsqueeze(-1).add(slots.mul(0.0)), times);
    count = count.add(spiked.to(state.scalar_type()));
    // state at the crossing instant (linear interpolation of every component), then the reset
    const Series at_crossing = before.add(next.sub(before).mul(frac.clamp(0.0, 1.0).unsqueeze(-1)));
    state = reset(spiked.unsqueeze(-1).where(at_crossing, next), spiked.to(state.scalar_type()));
    // The reset happened at t_cross, not at the end of the step: advance each spiking lane by the
    // remaining time t0 + dt - t_cross. The rate is autonomous, so a per-lane time rescaling
    // s = remaining/dt (0 for lanes that did not spike) turns this into one ordinary RK4 step of
    // s * rate over [0, dt], reusing the same integrator.
    if (spiked.any().item<bool>()) {
      const Series remaining = spiked.to(state.scalar_type()).mul(t_cross.neg().add(t0 + opt.dt)).div(opt.dt).unsqueeze(-1);
      state = dynamics::simulate([&](const Series &y) { return rate(y).mul(remaining); }, state, 0.0, opt.dt, 1);
    }
  }
  return {state, count, times};
}

inline SpikeResult simulate_izhikevich(const Series &state0, const Coeff &current, const IzhikevichParams &p = {},
                                       SpikeOptions opt = {}) {
  opt.threshold = p.threshold;
  const auto rate = [&](const Series &s) { return izhikevich_rate(s, current, p); };
  const auto reset = [&](const Series &s, const Series &spiked) {
    const Series v = s.select(-1, 0), u = s.select(-1, 1);
    const Series fire = spiked.gt(0.0);
    const Series v_new = fire.where(p.c.on(v).add(v.mul(0.0)), v);
    const Series u_new = fire.where(u.add(p.d.on(v)), u);
    return v_new.unsqueeze(-1).unite(std::vector<Series>{u_new.unsqueeze(-1)}, -1);
  };
  return simulate_spiking(rate, reset, state0, opt);
}

inline SpikeResult simulate_lif(const Series &v0, const Coeff &tau, const Coeff &e_leak, const Coeff &resistance,
                                const Coeff &current, double v_threshold, double v_reset, SpikeOptions opt = {}) {
  opt.threshold = v_threshold;
  opt.voltage_index = 0;
  const auto rate = [&](const Series &s) { // s: [..., 1]; evaluate on the voltage lanes [...]
    return lif_rate(s.select(-1, 0), tau, e_leak, resistance, current).unsqueeze(-1);
  };
  const auto reset = [&](const Series &s, const Series &spiked) {
    return spiked.gt(0.0).unsqueeze(-1).where(s.mul(0.0).add(v_reset), s);
  };
  // LIF has a single variable: carry it as [..., 1]
  return simulate_spiking(rate, reset, v0.unsqueeze(-1), opt);
}

} // namespace pluss::subsp::natsci::biology::neuroscience
