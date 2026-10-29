#pragma once

// Closed constant-volume ideal-gas reactor, batched over lanes.
//
// State y = (T, C_1 .. C_S) with concentrations in mol m^-3. The equations
// are those of Cantera's IdealGasReactor (zeroD/IdealGasReactor.cpp, GitHub
// source) specialised to a closed, adiabatic, rigid vessel:
//   dC_k/dt = omega_k
//   m c_v dT/dt = -V sum_k omega_k u_k   =>   dT/dt = -sum_k u_k omega_k /
//   sum_k C_k cv_k
// with partial molar internal energy u_k = h_k - R T and cv_k = cp_k - R.
// An isothermal variant fixes T. Total internal energy density sum_k C_k u_k
// and the element totals C @ composition are conserved (adiabatic case) and
// serve as run-time diagnostics.

#include "pluss/subsp/natsci/chemistry/kinetics.h"
#include "pluss/subsp/natsci/common/stiff.h"

namespace pluss::subsp::natsci::chemistry::reactor {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace stiff = ::pluss::subsp::natsci::common::stiff;

[[nodiscard]] inline Series make_state(const Series &t, const Series &c) {
  return t.unsqueeze(-1).unite(std::vector<Series>{c}, -1);
}
[[nodiscard]] inline Series temperature(const Series &y) {
  return y.select(-1, 0);
}
[[nodiscard]] inline Series concentrations(const Series &y) {
  return y.slice(-1, 1, y.size(-1), 1);
}

[[nodiscard]] inline Series pressure(const Series &y) {
  return temperature(y)
      .mul(thermo::gas_constant())
      .mul(concentrations(y).sum(-1, false));
}

// Internal energy per volume, sum_k C_k u_k  [J m^-3].
[[nodiscard]] inline Series
internal_energy_density(const kinetics::Mechanism &m, const Series &y) {
  const Series t = temperature(y);
  const Series rt = t.mul(thermo::gas_constant());
  const Series u = m.thermo().h_rt(t).sub(1.0).mul(rt.unsqueeze(-1));
  return concentrations(y).mul(u).sum(-1, false);
}

[[nodiscard]] inline Series element_totals(const kinetics::Mechanism &m,
                                           const Series &y) {
  return dynamics::matvec(m.composition().transpose(0, 1), concentrations(y));
}

// dy/dt for the closed constant-volume reactor.
[[nodiscard]] inline Series rate(const kinetics::Mechanism &m, const Series &y,
                                 bool adiabatic = true) {
  const Series t = temperature(y);
  const Series c = concentrations(y);
  const auto props = m.thermo().eval(t); // evaluated once for rates and energy
  const Series omega = m.net_production_rates(t, c, props);
  Series dt = t.mul(0.0);
  if (adiabatic) {
    const double r = thermo::gas_constant();
    const Series rt = t.mul(r);
    const Series u = props.h_rt.sub(1.0).mul(rt.unsqueeze(-1));
    const Series cv = props.cp_r.sub(1.0).mul(r);
    dt = omega.mul(u).sum(-1, false).neg().div(c.mul(cv).sum(-1, false));
  }
  return dt.unsqueeze(-1).unite(std::vector<Series>{omega}, -1);
}

// Integrate the reactor; lanes share the (adaptive) step.
[[nodiscard]] inline stiff::Result
integrate(const kinetics::Mechanism &m, const Series &y0, double t_end,
          bool adiabatic = true, const stiff::Options &options = {}) {
  return stiff::integrate(
      [&](const Series &y) { return rate(m, y, adiabatic); }, y0, 0.0, t_end,
      options);
}

// States on a time grid: [len(times), ..., 1 + S].
[[nodiscard]] inline Series trajectory(const kinetics::Mechanism &m,
                                       const Series &y0,
                                       const std::vector<double> &times,
                                       bool adiabatic = true,
                                       const stiff::Options &options = {}) {
  return stiff::integrate_at(
      [&](const Series &y) { return rate(m, y, adiabatic); }, y0, times,
      options);
}

// Time of the steepest temperature rise on a sampled grid (ignition delay by
// the maximum-dT/dt definition); T is [len(times), ...] and the result [...].
[[nodiscard]] inline Series max_slope_time(const std::vector<double> &times,
                                           const Series &temperature_path) {
  const auto n = static_cast<std::int64_t>(times.size());
  dynamics::require(n >= 3 && temperature_path.size(0) == n,
                    "reactor: need a sampled temperature path");
  const Series tt = pluss::make_series(times, {n})
                        .to(temperature_path.scalar_type())
                        .to(temperature_path.platform());
  const Series dtemp = temperature_path.slice(0, 1, n, 1)
                           .sub(temperature_path.slice(0, 0, n - 1, 1));
  const Series dtime = tt.slice(0, 1, n, 1).sub(tt.slice(0, 0, n - 1, 1));
  std::vector<std::int64_t> shape(
      static_cast<std::size_t>(temperature_path.dim()), 1);
  shape[0] = n - 1;
  const Series slope = dtemp.div(dtime.reshape(shape));
  const Series mid = tt.slice(0, 1, n, 1)
                         .add(tt.slice(0, 0, n - 1, 1))
                         .mul(0.5)
                         .reshape(shape);
  const Series best = slope.amax(0, true);
  const Series is_best = slope.ge(best);
  return is_best.to(slope.scalar_type())
      .mul(mid)
      .sum(0, false)
      .div(is_best.to(slope.scalar_type()).sum(0, false));
}

} // namespace pluss::subsp::natsci::chemistry::reactor
