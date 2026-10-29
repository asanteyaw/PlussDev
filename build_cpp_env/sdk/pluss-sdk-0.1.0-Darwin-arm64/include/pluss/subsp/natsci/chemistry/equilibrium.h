#pragma once

// Chemical equilibrium of ideal-gas mixtures by Gibbs / Helmholtz
// minimisation with the element-potential Newton method of
//   S. Gordon & B. J. McBride, NASA Reference Publication 1311 (1994),
//   "Computer Program for Calculation of Complex Chemical Equilibrium
//   Compositions and Applications. I. Analysis" (read from the NTRS scan):
//   mu_j/RT = g_j/RT + ln(n_j/n) + ln(P/P0)        (tp, hp; eq. 2.11)
//   mu_j/RT = g_j/RT + ln(n_j R T/(V P0))          (tv, uv; eq. 2.35)
//   reduced Newton systems in (pi_i, dln n, dln T): eqs. 2.24-2.27 (tp, hp) and
//   2.45-2.47 (tv, uv); species corrections from eqs. 2.18 / 2.40; step control
//   lambda = min(1, lambda1, lambda2), eqs. 3.1-3.3; log-variable update
//   eq. 3.4.
// The pi_i of the final iteration are the element potentials: at the solution
//   mu_j/RT = sum_i pi_i a_ij  for every species that exists.
//
// Scope: gas-phase species only (no condensed phases, no ions). Everything is
// batched over leading lane axes and all lanes iterate together.
// Absolute amounts: n_j in mol, element totals b0 [..., E] in mol, H/U in J,
// V in m^3. Elements with zero total are dropped per lane together with every
// species containing them. Standard pressure is thermo::standard_pressure.

#include "pluss/subsp/natsci/chemistry/kinetics.h"

namespace pluss::subsp::natsci::chemistry::equilibrium {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;

enum class Problem { TP, HP, TV, UV };

struct Options final {
  std::int64_t max_iterations{200};
  double tolerance{
      1e-11}; // max dimensionless correction over lanes and unknowns
  double initial_temperature{
      2500.0}; // hp / uv starting point (clamped into the data window)
};

struct Result final {
  Series moles = pluss::series::undefined();       // [..., S]
  Series temperature = pluss::series::undefined(); // [...]
  Series pressure = pluss::series::undefined();    // [...] Pa
  Series potentials =
      pluss::series::undefined(); // [..., E] element potentials pi_i
  Series active =
      pluss::series::undefined(); // [..., S] 1 where the species can exist
  std::int64_t iterations{0};
  bool converged{false};
};

namespace detail {
inline Series cols(const std::vector<Series> &parts) {
  Series out = parts.front();
  if (parts.size() > 1)
    out = out.unite(std::vector<Series>(parts.begin() + 1, parts.end()), -1);
  return out;
}
inline Series rows(const std::vector<Series> &parts) {
  Series out = parts.front();
  if (parts.size() > 1)
    out = out.unite(std::vector<Series>(parts.begin() + 1, parts.end()), -2);
  return out;
}
inline Series scalar_block(const Series &v) {
  return v.unsqueeze(-1).unsqueeze(-1);
} // [..., 1, 1]
} // namespace detail

// first/second carry the two specified state functions per problem:
//   TP: (temperature, pressure)   HP: (pressure, total enthalpy [J])
//   TV: (temperature, volume)     UV: (volume,   total internal energy [J])
inline Result solve(const thermo::Nasa7 &th, const Series &composition,
                    Problem problem, const Series &b0, const Series &first,
                    const Series &second, const Options &opt = {}) {
  const bool tp = problem == Problem::TP, hp = problem == Problem::HP;
  const bool tv = problem == Problem::TV, uv = problem == Problem::UV;
  const bool has_n = tp || hp; // total moles is an independent unknown
  const bool has_t = hp || uv; // temperature is an unknown
  const std::int64_t ns = composition.size(0), ne = composition.size(1);
  const double r = thermo::gas_constant();
  const double p0 = thermo::standard_pressure;
  dynamics::require(th.species() == ns,
                    "equilibrium: composition rows must match the species");
  dynamics::require_nonnegative(b0, "equilibrium: element totals");
  dynamics::require(b0.size(-1) == ne,
                    "equilibrium: element totals need one entry per element");

  const Series like = b0.select(-1, 0);
  const Config cfg =
      Config{}.with_dtype(like.scalar_type()).with_platform(like.platform());
  const Series zero = like.mul(0.0);
  const Series comp =
      composition.to(like.scalar_type()).to(like.platform()); // [S, E]
  const Series comp_t = comp.transpose(0, 1);                 // [E, S]

  // elements / species that exist in each lane
  const Series elem_on = b0.gt(0.0).to(like.scalar_type()); // [..., E]
  const Series bad = elem_on.neg().add(1.0).matmul(comp_t); // [..., S]
  const Series active = bad.le(0.0).to(like.scalar_type());
  const Series active_b = active.gt(0.0);
  const Series n_active = active.sum(-1, false);
  dynamics::require(n_active.min().item<double>() > 0.0,
                    "equilibrium: no species can form from these elements");

  // initial estimate: equal moles of every possible species summing to the atom
  // total
  const Series b_sum = b0.sum(-1, false);
  Series ln_nj = active_b.where(
      b_sum.div(n_active).log().unsqueeze(-1).add(active.mul(0.0)),
      active.mul(0.0).add(-60.0));
  Series ln_n = b_sum.log();

  Series pres = zero, volume = zero, temp = zero, target = zero;
  if (tp) {
    temp = first;
    pres = second;
  }
  if (hp) {
    pres = first;
    target = second;
    temp = zero.add(opt.initial_temperature);
  }
  if (tv) {
    temp = first;
    volume = second;
  }
  if (uv) {
    volume = first;
    target = second;
    temp = zero.add(opt.initial_temperature);
  }
  const double ln_lo = std::log(th.t_min()), ln_hi = std::log(th.t_max());
  Series ln_t = temp.log().clamp(ln_lo, ln_hi);
  const Series ln_p_ratio = has_n ? pres.div(p0).log() : zero;

  const Series eye_e = pluss::series::eye(ne, cfg);
  Result out;
  Series pi = b0.mul(0.0);
  for (std::int64_t it = 0; it < opt.max_iterations; ++it) {
    const Series t = ln_t.exp();
    const Series rt = t.mul(r);
    const Series nj = ln_nj.exp().mul(active);
    const Series n_tot = has_n ? ln_n.exp() : nj.sum(-1, false);
    const auto thermo = th.eval(t);
    const Series h_rt = thermo.h_rt, g_rt = thermo.g_rt(), cp_r = thermo.cp_r;
    const Series mu =
        has_n ? g_rt.add(ln_nj)
                    .sub(ln_n.unsqueeze(-1))
                    .add(ln_p_ratio.unsqueeze(-1))
              : g_rt.add(ln_nj).add(
                    rt.div(volume.mul(p0)).log().unsqueeze(-1)); // mu_j/RT
    const Series x = uv ? h_rt.sub(1.0) : h_rt;  // H/RT (hp) or U/RT (uv)
    const Series cx = uv ? cp_r.sub(1.0) : cp_r; // Cp/R or Cv/R
    const Series w = nj; // weights; zero for impossible species

    const Series a_mat =
        comp_t.matmul(w.unsqueeze(-1).mul(comp)); // [..., E, E]
    const Series c_vec =
        w.unsqueeze(-2).matmul(comp).squeeze(-2); // sum_j a_kj n_j = b_k
    const Series e_vec =
        w.mul(x).unsqueeze(-2).matmul(comp).squeeze(-2); // sum_j a_kj n_j X_j
    const Series wmu = w.mul(mu);
    const Series wmu_e = wmu.unsqueeze(-2).matmul(comp).squeeze(-2);
    const Series s0 = w.sum(-1, false);
    const Series sx = w.mul(x).sum(-1, false);
    const Series sc_xx = w.mul(cx).add(w.mul(x).mul(x)).sum(-1, false);

    const Series row_mask = elem_on.unsqueeze(-1),
                 col_mask = elem_on.unsqueeze(-2);
    const Series a_use = a_mat.mul(row_mask).mul(col_mask).add(
        eye_e.mul(elem_on.neg().add(1.0).unsqueeze(-1)));
    const Series c_use = c_vec.mul(elem_on), e_use = e_vec.mul(elem_on);

    std::vector<Series> matrix_rows, rhs_parts;
    {
      std::vector<Series> block{a_use};
      if (has_n)
        block.push_back(c_use.unsqueeze(-1));
      if (has_t)
        block.push_back(e_use.unsqueeze(-1));
      matrix_rows.push_back(detail::cols(block));
      rhs_parts.push_back(b0.sub(c_vec).add(wmu_e).mul(elem_on));
    }
    if (has_n) {
      std::vector<Series> block{c_use.unsqueeze(-2),
                                detail::scalar_block(s0.sub(n_tot))};
      if (has_t)
        block.push_back(detail::scalar_block(sx));
      matrix_rows.push_back(detail::cols(block));
      rhs_parts.push_back(n_tot.sub(s0).add(wmu.sum(-1, false)).unsqueeze(-1));
    }
    if (has_t) {
      std::vector<Series> block{e_use.unsqueeze(-2)};
      if (has_n)
        block.push_back(detail::scalar_block(sx));
      block.push_back(detail::scalar_block(sc_xx));
      matrix_rows.push_back(detail::cols(block));
      const Series total = nj.mul(x).sum(-1, false).mul(
          rt); // h = sum n_j H_j  or  u = sum n_j U_j  [J]
      rhs_parts.push_back(target.sub(total)
                              .div(rt)
                              .add(w.mul(x).mul(mu).sum(-1, false))
                              .unsqueeze(-1));
    }
    const Series sol =
        dynamics::solve(detail::rows(matrix_rows), detail::cols(rhs_parts));

    pi = sol.slice(-1, 0, ne, 1);
    std::int64_t pos = ne;
    Series d_ln_n = zero, d_ln_t = zero;
    if (has_n)
      d_ln_n = sol.select(-1, pos++);
    if (has_t)
      d_ln_t = sol.select(-1, pos++);
    Series d_ln_nj = pi.matmul(comp_t).sub(mu); // eq. 2.18 / 2.40 (mu term)
    if (has_n)
      d_ln_nj = d_ln_nj.add(d_ln_n.unsqueeze(-1));
    if (has_t)
      d_ln_nj = d_ln_nj.add(x.mul(d_ln_t.unsqueeze(-1)));
    d_ln_nj = d_ln_nj.mul(active);
    if (!has_n)
      d_ln_n = nj.mul(d_ln_nj).sum(-1, false).div(n_tot);

    // step control, eqs. 3.1-3.3
    const double size = 18.420681;
    Series denom = d_ln_nj.abs().amax(-1, false);
    if (has_t)
      denom = denom.maximum(d_ln_t.abs().mul(5.0));
    if (has_n)
      denom = denom.maximum(d_ln_n.abs().mul(5.0));
    const Series lam1 = denom.clamp(1e-300, 1e300).reciprocal().mul(2.0);
    const Series ln_frac =
        ln_nj.sub((has_n ? ln_n : n_tot.log()).unsqueeze(-1));
    const Series small =
        ln_frac.le(-size).logical_and(d_ln_nj.ge(0.0)).logical_and(active_b);
    const Series num = ln_frac.neg().sub(9.2103404).abs();
    const Series den =
        d_ln_nj.sub(d_ln_n.unsqueeze(-1)).abs().clamp(1e-300, 1e300);
    const Series lam2 =
        small.where(num.div(den), num.mul(0.0).add(1e300)).amin(-1, false);
    const Series lam = lam1.minimum(lam2).minimum(zero.add(1.0));

    ln_nj = active_b.where(
        ln_nj.add(d_ln_nj.mul(lam.unsqueeze(-1))).clamp(-700.0, 700.0), ln_nj);
    if (has_n)
      ln_n = ln_n.add(d_ln_n.mul(lam));
    if (has_t)
      ln_t = ln_t.add(d_ln_t.mul(lam)).clamp(ln_lo, ln_hi);

    out.iterations = it + 1;
    double worst =
        d_ln_nj.mul(nj).abs().div(nj.sum(-1, true)).max().item<double>();
    if (has_n)
      worst = std::max(worst, d_ln_n.abs().max().item<double>());
    if (has_t)
      worst = std::max(worst, d_ln_t.abs().max().item<double>());
    if (worst < opt.tolerance && lam.min().item<double>() >= 1.0) {
      out.converged = true;
      break;
    }
  }
  const Series t_fin = ln_t.exp();
  const Series nj_fin = ln_nj.exp().mul(active);
  out.moles = nj_fin;
  out.temperature = t_fin;
  out.potentials = pi;
  out.active = active;
  out.pressure =
      has_n ? pres : nj_fin.sum(-1, false).mul(r).mul(t_fin).div(volume);
  return out;
}

inline Result tp(const kinetics::Mechanism &m, const Series &temperature,
                 const Series &pressure, const Series &element_moles,
                 const Options &opt = {}) {
  return solve(m.thermo(), m.composition(), Problem::TP, element_moles,
               temperature, pressure, opt);
}
inline Result hp(const kinetics::Mechanism &m, const Series &pressure,
                 const Series &enthalpy, const Series &element_moles,
                 const Options &opt = {}) {
  return solve(m.thermo(), m.composition(), Problem::HP, element_moles,
               pressure, enthalpy, opt);
}
inline Result tv(const kinetics::Mechanism &m, const Series &temperature,
                 const Series &volume, const Series &element_moles,
                 const Options &opt = {}) {
  return solve(m.thermo(), m.composition(), Problem::TV, element_moles,
               temperature, volume, opt);
}
inline Result uv(const kinetics::Mechanism &m, const Series &volume,
                 const Series &internal_energy, const Series &element_moles,
                 const Options &opt = {}) {
  return solve(m.thermo(), m.composition(), Problem::UV, element_moles, volume,
               internal_energy, opt);
}

} // namespace pluss::subsp::natsci::chemistry::equilibrium
