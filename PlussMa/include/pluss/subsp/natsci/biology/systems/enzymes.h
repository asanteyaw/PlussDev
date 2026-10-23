#pragma once

// Enzyme kinetics beyond Michaelis-Menten, batched over lanes of substrate / inhibitor / parameter values.
//
//  * Steady-state rate laws: uncompetitive, mixed (non-competitive when Ki = Ki') and substrate inhibition (Haldane), reversible
//    Michaelis-Menten with product (Haldane relationship), rapid-equilibrium ordered and random bi-substrate, ping-pong bi-bi
//    (Cleland, Biochim. Biophys. Acta 67 (1963) 104), and Morrison's tight-binding inhibition (Morrison, Biochim. Biophys. Acta
//    185 (1969) 269), which solves the free-inhibitor depletion exactly.
//  * A general MECHANISM ENGINE (King & Altman, J. Phys. Chem. 60 (1956) 1375 by linear algebra instead of enumerating spanning trees):
//    enzyme states and first-order / pseudo-first-order steps define a rate matrix Q(concentrations); the steady-state state
//    populations are the null vector of Q with sum 1 (one row replaced by ones, batched solve) and the turnover is the net flux through
//    the product-releasing steps. Any mechanism - ordered, ping-pong, random, branched, with dead-end complexes - is a list of steps.
//  * Progress curves: the integrated Michaelis-Menten equation in terms of the Lambert W function (Schnell & Mendoza, J. Theor. Biol.
//    187 (1997) 207) and the total quasi-steady-state approximation (Borghans, de Boer & Segel 1996; Tzafriri, Bull. Math. Biol. 65
//    (2003) 1111), valid also when the enzyme is not much less abundant than the substrate.

#include "pluss/subsp/math/lambert_w.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::natsci::biology::systems::enzymes {

using dynamics::Coeff;
namespace roots = ::pluss::subsp::math::roots;

// ---- steady-state rate laws -----------------------------------------------------------------------------
// v = Vmax S / (Km + S (1 + I/Ki'))
inline Series uncompetitive_inhibition(const Series &s, const Series &inhibitor, const Coeff &vmax, const Coeff &km, const Coeff &ki_prime) {
  const Series factor = inhibitor.div(ki_prime.on(s)).add(1.0);
  return vmax.on(s).mul(s).div(km.on(s).add(s.mul(factor)));
}
// v = Vmax S / (Km (1 + I/Ki) + S (1 + I/Ki')): mixed inhibition; Ki = Ki' is pure non-competitive
inline Series mixed_inhibition(const Series &s, const Series &inhibitor, const Coeff &vmax, const Coeff &km, const Coeff &ki, const Coeff &ki_prime) {
  const Series a = inhibitor.div(ki.on(s)).add(1.0), b = inhibitor.div(ki_prime.on(s)).add(1.0);
  return vmax.on(s).mul(s).div(km.on(s).mul(a).add(s.mul(b)));
}
// substrate inhibition (Haldane): v = Vmax S / (Km + S + S^2/Ksi)
inline Series substrate_inhibition(const Series &s, const Coeff &vmax, const Coeff &km, const Coeff &ksi) {
  return vmax.on(s).mul(s).div(km.on(s).add(s).add(s.square().div(ksi.on(s))));
}
// reversible Michaelis-Menten: v = (Vf S/Ks - Vr P/Kp) / (1 + S/Ks + P/Kp), Haldane: K_eq = Vf Kp / (Vr Ks)
inline Series reversible_michaelis_menten(const Series &s, const Series &p, const Coeff &vf, const Coeff &ks, const Coeff &vr, const Coeff &kp) {
  const Series xs = s.div(ks.on(s)), xp = p.div(kp.on(s));
  return vf.on(s).mul(xs).sub(vr.on(s).mul(xp)).div(xs.add(xp).add(1.0));
}
// rapid-equilibrium ordered bi-bi (A binds first), initial rate: v = Vmax A B / (Kia Kb + Kb A + A B)
inline Series ordered_bibi_rapid(const Series &a, const Series &b, const Coeff &vmax, const Coeff &kia, const Coeff &kb) {
  return vmax.on(a).mul(a).mul(b).div(kia.on(a).mul(kb.on(a)).add(kb.on(a).mul(a)).add(a.mul(b)));
}
// rapid-equilibrium random bi-bi with interaction factor alpha: v = Vmax A B / (alpha Ka Kb + alpha Kb A + alpha Ka B + A B)
inline Series random_bibi_rapid(const Series &a, const Series &b, const Coeff &vmax, const Coeff &ka, const Coeff &kb, const Coeff &alpha) {
  const Series al = alpha.on(a);
  return vmax.on(a).mul(a).mul(b).div(al.mul(ka.on(a)).mul(kb.on(a)).add(al.mul(kb.on(a)).mul(a)).add(al.mul(ka.on(a)).mul(b)).add(a.mul(b)));
}
// ping-pong bi-bi, initial rate: Et / v = 1/kcat1 + 1/kcat2 + KmA/(kcat1 A) + KmB/(kcat2 B), i.e. v = Vmax A B / (KmB A + KmA B + A B)
// with Vmax = Et kcat1 kcat2 / (kcat1 + kcat2), KmA' = KmA kcat2 / (kcat1 + kcat2), KmB' likewise; this form takes the apparent constants
inline Series pingpong_bibi(const Series &a, const Series &b, const Coeff &vmax, const Coeff &kma, const Coeff &kmb) {
  return vmax.on(a).mul(a).mul(b).div(kmb.on(a).mul(a).add(kma.on(a).mul(b)).add(a.mul(b)));
}
// Morrison tight-binding inhibition: fractional activity v/v0 = 1 - [(E + I + Ki_app) - sqrt((E + I + Ki_app)^2 - 4 E I)] / (2 E)
inline Series morrison(const Series &enzyme_total, const Series &inhibitor_total, const Coeff &ki_app) {
  const Series sum = enzyme_total.add(inhibitor_total).add(ki_app.on(enzyme_total));
  const Series ei = sum.sub(sum.square().sub(enzyme_total.mul(inhibitor_total).mul(4.0)).clamp(0.0, 1e300).sqrt()).mul(0.5);
  return ei.div(enzyme_total).mul(-1.0).add(1.0);
}

// ---- mechanism engine -------------------------------------------------------------------------------------
struct Step final {
  int from{0}, to{0};
  Coeff k = 0.0;        // rate constant (1/s, or 1/(M s) when `species` >= 0)
  int species{-1};      // index into the concentration vector multiplying k (pseudo-first-order step), -1 for none
  double product_sign{0.0}; // +1: the step releases product forward, -1: a reverse step consumes it, 0: no product flux
};
struct Mechanism final {
  int states{0};
  std::vector<Step> steps;
};

// rate matrix Q[..., to, from] (columns sum to zero) for concentrations [..., S] (or [S] shared by all lanes of the constants)
inline Series rate_matrix(const Mechanism &m, const Series &conc) {
  const std::int64_t n = m.states;
  const Config cfg = Config{}.with_dtype(conc.scalar_type()).with_platform(conc.platform());
  Series q = pluss::series::zeros({n, n}, cfg);
  for (const auto &st : m.steps) {
    Series rate = st.k.on(conc.select(-1, 0));
    if (st.species >= 0)
      rate = rate.mul(conc.select(-1, st.species));
    std::vector<double> inc(static_cast<std::size_t>(n * n), 0.0);
    inc[static_cast<std::size_t>(st.to * n + st.from)] += 1.0;
    inc[static_cast<std::size_t>(st.from * n + st.from)] -= 1.0;
    const Series incidence = pluss::make_series(inc, {n, n}).to(conc.scalar_type()).to(conc.platform());
    q = q.add(rate.unsqueeze(-1).unsqueeze(-1).mul(incidence));
  }
  return q;
}
// steady-state state populations (fractions of the total enzyme) [..., states]
inline Series steady_state(const Mechanism &m, const Series &conc) {
  const std::int64_t n = m.states;
  const Series q = rate_matrix(m, conc);
  std::vector<double> mask(static_cast<std::size_t>(n * n), 1.0), ones(static_cast<std::size_t>(n * n), 0.0), rhs(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t j = 0; j < n; ++j) {
    mask[static_cast<std::size_t>((n - 1) * n + j)] = 0.0;
    ones[static_cast<std::size_t>((n - 1) * n + j)] = 1.0;
  }
  rhs[static_cast<std::size_t>(n - 1)] = 1.0;
  const Series mk = pluss::make_series(mask, {n, n}).to(conc.scalar_type()).to(conc.platform());
  const Series on = pluss::make_series(ones, {n, n}).to(conc.scalar_type()).to(conc.platform());
  const Series b = pluss::make_series(rhs, {n}).to(conc.scalar_type()).to(conc.platform());
  const Series a = q.mul(mk).add(on);
  return dynamics::solve(a, b.add(q.select(-1, 0).mul(0.0)));
}
// turnover per enzyme: net product release rate (1/s) at the steady state
inline Series turnover(const Mechanism &m, const Series &conc) {
  const Series pi = steady_state(m, conc);
  Series v = pi.select(-1, 0).mul(0.0);
  for (const auto &st : m.steps) {
    if (st.product_sign == 0.0)
      continue;
    Series rate = st.k.on(conc.select(-1, 0));
    if (st.species >= 0)
      rate = rate.mul(conc.select(-1, st.species));
    v = v.add(rate.mul(pi.select(-1, st.from)).mul(st.product_sign));
  }
  return v;
}


// ---- allosteric (concerted, MWC) enzymes ---------------------------------------------------------------------------------------
// Monod-Wyman-Changeux model with m identical ligand / substrate sites, written as in Einav & Phillips, J. Phys. Chem. B 121 (2017) 3813
// (arXiv:1701.06122, eq. 1; read from the open arXiv PDF, SHA-256 631b627a...): the active ensemble has dissociation constant K_A, the
// inactive one K_I, and -epsilon is the free-energy (in kT) favouring the inactive state without ligand, so the Boltzmann weights are
//     active: (1 + c/K_A)^m,   inactive: e^{-beta eps} (1 + c/K_I)^m,    p_active(c) = active / (active + inactive),
// leakiness p_active(0) = 1/(1 + e^{-beta eps}), maximum 1/(1 + e^{-beta eps} (K_A/K_I)^m) at saturating ligand, dynamic range = max - min,
// EC50 where the normalised response is 1/2 and the effective Hill coefficient h = 2 d log(normalised) / d log c at EC50.
// For an enzyme whose catalytic sites turn over at k_cat only in the active ensemble, v / (m k_cat Et) is the mean fraction of occupied
// active sites: alpha (1 + alpha)^{m-1} / (...), alpha = c / K_A (Monod, Wyman & Changeux 1965, K-system); `activity_ratio` b = kcat_I/kcat_A
// lets the inactive ensemble turn over too.
inline Series mwc_active_probability(const Series &c, int m, const Coeff &k_active, const Coeff &k_inactive, const Coeff &beta_eps) {
  const Series a = c.div(k_active.on(c)).add(1.0).pow(static_cast<double>(m));
  const Series i = beta_eps.on(c).exp().reciprocal().mul(c.div(k_inactive.on(c)).add(1.0).pow(static_cast<double>(m)));
  return a.div(a.add(i));
}
inline Series mwc_enzyme_velocity(const Series &c, int m, const Coeff &vmax, const Coeff &k_active, const Coeff &k_inactive, const Coeff &beta_eps, const Coeff &activity_ratio = 0.0) {
  const double mm = static_cast<double>(m);
  const Series alpha = c.div(k_active.on(c)), gamma = c.div(k_inactive.on(c));
  const Series l = beta_eps.on(c).exp().reciprocal();
  const Series act = alpha.mul(alpha.add(1.0).pow(mm - 1.0));
  const Series ina = l.mul(activity_ratio.on(c)).mul(gamma).mul(gamma.add(1.0).pow(mm - 1.0));
  const Series den = alpha.add(1.0).pow(mm).add(l.mul(gamma.add(1.0).pow(mm)));
  return vmax.on(c).mul(act.add(ina)).div(den);
}
struct MwcCharacteristics final {
  Series leakiness = pluss::series::undefined(), maximum = pluss::series::undefined(), dynamic_range = pluss::series::undefined();
  Series ec50 = pluss::series::undefined(), hill = pluss::series::undefined();
};
// leakiness / dynamic range in closed form, EC50 by batched bisection in log c, h from a central difference of log(normalised response) in log c
inline MwcCharacteristics mwc_characteristics(int m, const Series &k_active, const Series &k_inactive, const Series &beta_eps) {
  const Series l = beta_eps.exp().reciprocal();
  MwcCharacteristics out;
  out.leakiness = l.add(1.0).reciprocal();
  out.maximum = l.mul(k_active.div(k_inactive).pow(static_cast<double>(m))).add(1.0).reciprocal();
  out.dynamic_range = out.maximum.sub(out.leakiness);
  const auto normalised = [&](const Series &logc) {
    const Series c = logc.exp();
    return mwc_active_probability(c, m, k_active, k_inactive, beta_eps).sub(out.leakiness).div(out.dynamic_range);
  };
  Series lo = k_active.log().sub(40.0), hi = k_active.log().add(40.0);
  const auto g = [&](const Series &logc) { return normalised(logc).sub(0.5); };
  roots::Options opt;
  opt.max_iterations = 200;
  const Series logc50 = roots::bisect(g, lo, hi, opt).root;
  out.ec50 = logc50.exp();
  const double h = 1e-5;
  out.hill = normalised(logc50.add(h)).log().sub(normalised(logc50.sub(h)).log()).div(2.0 * h).mul(2.0);
  return out;
}
// Adair form for sequential / general cooperative enzymes: cumulative macroscopic association constants beta_j (j = 1..m, [..., m]);
// per-site turnover k_j may differ with occupancy: v = sum_j k_j j beta_j c^j / (1 + sum_j beta_j c^j) / m... returned as mean occupied fraction
inline Series adair_velocity(const Series &c, const Series &beta, const Coeff &vmax) {
  const std::int64_t m = beta.size(-1);
  Series z = c.mul(0.0).add(1.0), num = c.mul(0.0);
  Series power = c.mul(0.0).add(1.0);
  for (std::int64_t j = 1; j <= m; ++j) {
    power = power.mul(c);
    const Series term = beta.select(-1, j - 1).mul(power);
    z = z.add(term);
    num = num.add(term.mul(static_cast<double>(j)));
  }
  return vmax.on(c).mul(num).div(z.mul(static_cast<double>(m)));
}

// ---- progress curves --------------------------------------------------------------------------------------
// integrated Michaelis-Menten: S(t) = Km W( (S0/Km) exp((S0 - Vmax t)/Km) ) (Schnell-Mendoza); valid in the QSSA regime E0 << S0 + Km
inline Series integrated_michaelis_menten(const Series &t, const Coeff &s0, const Coeff &vmax, const Coeff &km) {
  const Series k = km.on(t), s = s0.on(t);
  const Series z = s.div(k).log().add(s.sub(vmax.on(t).mul(t)).div(k));
  return k.mul(math::special::lambert_w0_from_log(z));
}
// total QSSA: complex C = [(E_T + S_T + K) - sqrt((E_T + S_T + K)^2 - 4 E_T S_T)] / 2, dS_T/dt = -kcat C, K = (k_-1 + kcat)/k_1
inline dynamics::RateFn tqssa_rates(const Coeff &e_total, const Coeff &kcat, const Coeff &michaelis) {
  return [=](const Series &st) {
    const Series e = e_total.on(st), k = michaelis.on(st);
    const Series sum = e.add(st).add(k);
    const Series c = sum.sub(sum.square().sub(e.mul(st).mul(4.0)).clamp(0.0, 1e300).sqrt()).mul(0.5);
    return c.mul(kcat.on(st)).mul(-1.0);
  };
}
// full mass-action E + S <-> C -> E + P: y = [S, E, C, P]
inline dynamics::RateFn mass_action_rates(const Coeff &k1, const Coeff &k_minus1, const Coeff &kcat) {
  return [=](const Series &y) {
    const Series s = y.select(-1, 0), e = y.select(-1, 1), c = y.select(-1, 2);
    const Series bind = k1.on(s).mul(s).mul(e), unbind = k_minus1.on(s).mul(c), cat = kcat.on(s).mul(c);
    const Series ds = unbind.sub(bind), de = unbind.add(cat).sub(bind), dc = bind.sub(unbind).sub(cat);
    return ds.unsqueeze(-1).unite(std::vector<Series>{de.unsqueeze(-1), dc.unsqueeze(-1), cat.unsqueeze(-1)}, -1);
  };
}

} // namespace pluss::subsp::natsci::biology::systems::enzymes
