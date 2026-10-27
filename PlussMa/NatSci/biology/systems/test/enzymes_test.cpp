// Enzyme kinetics: rate laws, the mechanism engine and progress curves against exact / numerical oracles
// (see NatSci/biology/verification/gen_enzymes_ref.py: sympy exact steady states, mpmath Lambert W, scipy Radau).
#include "enzymes_reference.h"
#include "pluss/subsp/natsci/biology/metabolism/thermodynamics.h"
#include "pluss/subsp/natsci/biology/systems/enzymes.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace enz = pluss::subsp::natsci::biology::systems::enzymes;
namespace th = pluss::subsp::natsci::biology::metabolism::thermodynamics;
namespace dyn = pluss::subsp::natsci::biology::dynamics;
namespace ref = enzymes_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
Series mat(const std::vector<double> &v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(F64); }
double rel(double a, double b) { return std::abs(a - b) / (1e-300 + std::abs(b)); }
double max_rel(const Series &got, const std::vector<double> &want) {
  const auto g = got.contiguous().host<double>();
  expect(g.size() == want.size(), "size mismatch");
  double worst = 0.0;
  for (std::size_t i = 0; i < g.size(); ++i)
    worst = std::isfinite(g[i]) ? std::max(worst, rel(g[i], want[i])) : 1e300;
  return worst;
}
enz::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { return enz::Step{from, to, k, species, sign}; }
} // namespace

int main() {
  try {
    // ---- Lambert W (log argument) against mpmath
    {
      expect(max_rel(pluss::subsp::math::special::lambert_w0_from_log(vec(ref::lw_z)), ref::lw_w) < 1e-12, "Lambert W vs mpmath");
      const Series x = vec({0.0, 1e-9, 0.3, 1.0, 50.0});
      const Series w = pluss::subsp::math::special::lambert_w0(x);
      expect(w.mul(w.exp()).sub(x).abs().max().item<double>() < 1e-12, "w e^w = x");
      expect(std::abs(pluss::subsp::math::special::lambert_w0(vec({1.0})).host<double>().front() - 0.5671432904097838) < 1e-14, "omega constant");
    }
    // ---- mechanism engine: Michaelis-Menten against the exact steady state and against kcat S / (Km + S)
    {
      enz::Mechanism mm{2, {step(0, 1, 10.0, 0), step(1, 0, 1.0), step(1, 0, 0.5, -1, 1.0)}};
      const Series s = vec(ref::mm_S);
      const Series conc = s.unsqueeze(-1);
      expect(max_rel(enz::turnover(mm, conc), ref::mm_v) < 1e-12, "engine MM vs sympy exact");
      const double km = 1.5 / 10.0;
      expect(max_rel(enz::turnover(mm, conc), std::vector<double>{0.5 * 0.1 / (km + 0.1), 0.5 * 0.5 / (km + 0.5), 0.5 * 2.0 / (km + 2.0), 0.5 * 10.0 / (km + 10.0)}) < 1e-12, "engine MM equals the closed form");
      const auto pi = enz::steady_state(mm, conc).contiguous().host<double>();
      expect(std::abs(pi[0] + pi[1] - 1.0) < 1e-14, "populations sum to one");
    }
    // ---- ordered bi-bi and ping-pong with all reverse steps, concentrations [A, B, P, Q], all lanes in one call
    {
      enz::Mechanism ob{5, {step(0, 1, 20.0, 0), step(1, 0, 3.0), step(1, 2, 15.0, 1), step(2, 1, 2.0), step(2, 3, 8.0), step(3, 2, 1.0), step(3, 4, 6.0, -1, 1.0), step(4, 3, 4.0, 2, -1.0),
                          step(4, 0, 9.0), step(0, 4, 2.0, 3)}};
      expect(max_rel(enz::turnover(ob, mat(ref::ob_conc, 4, 4)), ref::ob_v) < 1e-11, "ordered bi-bi vs sympy exact");
      enz::Mechanism pp{4, {step(0, 1, 12.0, 0), step(1, 0, 2.0), step(1, 2, 7.0, -1, 1.0), step(2, 1, 3.0, 2, -1.0), step(2, 3, 10.0, 1), step(3, 2, 4.0), step(3, 0, 5.0), step(0, 3, 1.0, 3)}};
      expect(max_rel(enz::turnover(pp, mat(ref::pp_conc, 3, 4)), ref::pp_v) < 1e-11, "ping-pong bi-bi vs sympy exact");
      // initial rates (no products): Cleland's ping-pong form with the apparent constants
      const double k1 = 12, km1 = 2, k2 = 7, k3 = 10, km3 = 4, k4 = 5;
      const double kma = (km1 + k2) / k1, kmb = (km3 + k4) / k3, vmax = k2 * k4 / (k2 + k4);
      const Series a = vec({ref::pp_conc[0], ref::pp_conc[8]}), b = vec({ref::pp_conc[1], ref::pp_conc[9]});
      const Series closed = enz::pingpong_bibi(a, b, vmax, kma * k4 / (k2 + k4), kmb * k2 / (k2 + k4));
      const std::vector<double> eng = {enz::turnover(pp, mat(std::vector<double>(ref::pp_conc.begin(), ref::pp_conc.begin() + 4), 1, 4)).host<double>().front(), enz::turnover(pp, mat(std::vector<double>(ref::pp_conc.begin() + 8, ref::pp_conc.begin() + 12), 1, 4)).host<double>().front()};
      expect(max_rel(closed, eng) < 1e-12, "ping-pong closed form equals the engine's initial rate");
    }
    // ---- dead-end complexes: the exact steady state reproduces uncompetitive and substrate-inhibition laws
    {
      const double k1 = 10.0, km1 = 1.0, kcat = 0.5, km = (km1 + kcat) / k1;
      std::vector<double> got, want;
      for (std::size_t i = 0; i < ref::dead_v.size(); ++i) {
        const double kon = ref::dead_params[4 * i], koff = ref::dead_params[4 * i + 1], s = ref::dead_params[4 * i + 2], inh = ref::dead_params[4 * i + 3];
        enz::Mechanism m{3, {step(0, 1, k1, 0), step(1, 0, km1), step(1, 0, kcat, -1, 1.0), step(1, 2, kon, 1), step(2, 1, koff)}};
        got.push_back(enz::turnover(m, vec({s, inh}).unsqueeze(0)).host<double>().front());
        want.push_back(enz::uncompetitive_inhibition(vec({s}), vec({inh}), kcat, km, koff / kon).host<double>().front());
      }
      expect(max_rel(vec(got), ref::dead_v) < 1e-12, "dead-end engine vs sympy exact");
      expect(max_rel(vec(want), ref::dead_v) < 1e-12, "uncompetitive inhibition closed form vs exact steady state");
      expect(max_rel(enz::substrate_inhibition(vec(ref::subinh_S), 0.5, km, 3.0), ref::subinh_v) < 1e-12, "substrate inhibition (Haldane) vs exact steady state");
      // mixed inhibition reduces to the competitive and uncompetitive limits
      const Series s = vec({0.2, 1.0}), inh = vec({1.0, 2.0});
      expect(max_rel(enz::mixed_inhibition(s, inh, 0.5, km, 0.8, 1e300), std::vector<double>{0.5 * 0.2 / (km * (1 + 1.0 / 0.8) + 0.2), 0.5 * 1.0 / (km * (1 + 2.0 / 0.8) + 1.0)}) < 1e-12, "mixed -> competitive");
      expect(max_rel(enz::mixed_inhibition(s, inh, 0.5, km, 1e300, 0.8), enz::uncompetitive_inhibition(s, inh, 0.5, km, 0.8).contiguous().host<double>()) < 1e-12, "mixed -> uncompetitive");
      // reversible Michaelis-Menten: no net flux at equilibrium (Haldane relationship Keq = Vf Kp / (Vr Ks))
      const double vf = 2.0, ks = 0.5, vr = 0.8, kp = 1.5, keq = vf * kp / (vr * ks);
      expect(std::abs(enz::reversible_michaelis_menten(vec({1.0}), vec({keq}), vf, ks, vr, kp).host<double>().front()) < 1e-14, "reversible MM: zero flux at S/P = 1/Keq... i.e. P/S = Keq");
    }
    // ---- Morrison tight binding
    {
      std::vector<double> got;
      for (std::size_t i = 0; i < ref::mor_a.size(); ++i)
        got.push_back(enz::morrison(vec({ref::mor_E[i]}), vec({ref::mor_I[i]}), ref::mor_K[i]).host<double>().front());
      expect(max_rel(vec(got), ref::mor_a) < 1e-9, "Morrison vs the numerical binding equilibrium");
    }
    // ---- progress curves
    {
      const auto run = [&](const std::vector<double> &p, const std::vector<double> &t, const std::vector<double> &s_ref, const std::vector<double> &st_ref, double &sm_err, double &tq_err) {
        const double k1 = p[0], km1 = p[1], kcat = p[2], e0 = p[3], s0 = p[4], km = (km1 + kcat) / k1;
        const Series tt = vec(t);
        const Series sm = enz::integrated_michaelis_menten(tt, s0, kcat * e0, km);
        sm_err = 0.0;
        for (std::size_t i = 0; i < t.size(); ++i)
          sm_err = std::max(sm_err, std::abs(sm.contiguous().host<double>()[i] - s_ref[i]) / s0);
        tq_err = 0.0;
        for (std::size_t i = 1; i < t.size(); ++i) {
          const Series st = dyn::simulate(enz::tqssa_rates(e0, kcat, km), vec({s0}), 0.0, t[i], 3000);
          tq_err = std::max(tq_err, std::abs(st.host<double>().front() - st_ref[i]) / s0);
        }
      };
      double sm_a, tq_a, sm_b, tq_b;
      run(ref::pcA_p, ref::pcA_t, ref::pcA_S, ref::pcA_ST, sm_a, tq_a);
      run(ref::pcB_p, ref::pcB_t, ref::pcB_S, ref::pcB_ST, sm_b, tq_b);
      expect(sm_a < 6e-3, "Schnell-Mendoza S(t) in the QSSA regime (E0 << S0), worst " + std::to_string(sm_a));
      expect(tq_a < 3e-3 && tq_b < 3e-2, "total QSSA S_T(t): " + std::to_string(tq_a) + ", " + std::to_string(tq_b));
      if (std::getenv("ENZ_DEBUG")) std::fprintf(stderr, "sm_a %g tq_a %g sm_b %g tq_b %g\n", sm_a, tq_a, sm_b, tq_b);
      expect(sm_b > 5.0 * tq_b, "with E0 ~ S0 the standard QSSA fails (" + std::to_string(sm_b) + ") while the total QSSA holds (" + std::to_string(tq_b) + ")");
    }

    // ---- biochemical thermodynamics: transformed Gibbs energies against explicit species-level equilibria
    {
      // ATP + H2O -> ADP + Pi; ATP / ADP: one proton (pKa 6.5 / 6.4), Pi: three (2.15, 7.20, 12.35); formation energies are test inputs
      const std::size_t cases = ref::th_pH.size();
      std::vector<double> got(cases);
      for (std::size_t i = 0; i < cases; ++i) {
        const double temperature = ref::th_T[i];
        const Series ph = vec({ref::th_pH[i]});
        const Series atp = th::transformed_formation_gibbs(vec({ref::th_dgf0[0]}), ref::th_nh[0], vec({6.5}), ph, temperature);
        const Series adp = th::transformed_formation_gibbs(vec({ref::th_dgf0[1]}), ref::th_nh[1], vec({6.4}), ph, temperature);
        const Series pi = th::transformed_formation_gibbs(vec({ref::th_dgf0[2]}), ref::th_nh[2], vec({2.15, 7.20, 12.35}), ph, temperature);
        const Series w = th::transformed_formation_gibbs_fixed(vec({ref::th_dgf0[3]}), ref::th_nh[3], ph, temperature);
        const Series dgf = atp.unsqueeze(-1).unite(std::vector<Series>{w.unsqueeze(-1), adp.unsqueeze(-1), pi.unsqueeze(-1)}, -1);
        const Series dgr = th::reaction_gibbs(dgf, vec({-1.0, -1.0, 1.0, 1.0}));
        got[i] = std::log(th::apparent_equilibrium_constant(dgr, temperature).host<double>().front());
      }
      double worst = 0.0;
      for (std::size_t i = 0; i < cases; ++i)
        worst = std::max(worst, std::abs(got[i] - ref::th_lnK[i]));
      expect(worst < 1e-9, "apparent equilibrium constant vs explicit species equilibrium, worst |d ln K'| = " + std::to_string(worst));
      // limits: pH << pKa gives P = 1; at pH = pKa the two-state polynomial is 2; batched lanes agree with scalar calls
      expect(std::abs(th::proton_binding_polynomial(vec({0.0}), vec({7.0}).unsqueeze(0).select(0, 0)).host<double>().front() - (1.0 + std::pow(10.0, -7.0))) < 1e-15, "P at pH 0");
      expect(std::abs(th::proton_binding_polynomial(vec({6.5}), vec({6.5})).host<double>().front() - 2.0) < 1e-14, "P = 2 at pH = pKa");
      // slope of Delta_r G'^o with pH far from every pKa equals nu_H R T ln10 (here nu_H counts hydrogens of the products minus reactants)
      const double rt = th::gas_constant() * 298.15;
      const Series g = th::transformed_formation_gibbs(vec({0.0, 0.0}), 2.0, vec({1.0}).unsqueeze(0).select(0, 0), vec({10.0, 11.0}), 298.15);
      const auto gv = g.contiguous().host<double>();
      expect(std::abs((gv[1] - gv[0]) / (rt * std::log(10.0)) - 1.0) < 1e-6, "far above the pKa the group behaves like the deprotonated species (n - 1 hydrogens)");
      // extended Debye-Hueckel: zero ionic strength -> no correction, monotone in |z|
      const Series lg = th::debye_huckel_ln_gamma(vec({1.0, 2.0, 3.0}), vec({0.1, 0.1, 0.1}), 1.17582);
      const auto lv = lg.contiguous().host<double>();
      expect(lv[0] < 0.0 && lv[1] < lv[0] && lv[2] < lv[1], "activity coefficient decreases with charge");
      expect(std::abs(th::debye_huckel_ln_gamma(vec({2.0}), vec({0.0}), 1.17582).host<double>().front()) < 1e-15, "gamma = 1 at zero ionic strength");
    }

    // ---- ionic strength, pMg and charge: the Legendre transform as implemented in eQuilibrator (transcribed in the generator)
    {
      std::vector<th::Microspecies> ms;
      for (std::size_t i = 0; i < ref::eq_ms.size(); i += 4)
        ms.push_back({ref::eq_ms[i], ref::eq_ms[i + 1], ref::eq_ms[i + 2], ref::eq_ms[i + 3]});
      const std::size_t n = ref::eq_group_gibbs.size();
      for (std::size_t i = 0; i < n; ++i) {
        const double ph = ref::eq_cases[4 * i], pmg = ref::eq_cases[4 * i + 1], ionic = ref::eq_cases[4 * i + 2], temperature = ref::eq_cases[4 * i + 3];
        const double got = th::transformed_group_gibbs(ms, vec({ph}), pmg, vec({ionic}), temperature).host<double>().front();
        expect(std::abs(got - ref::eq_group_gibbs[i]) < 1e-6 * (1.0 + std::abs(ref::eq_group_gibbs[i])), "group transform vs eQuilibrator, case " + std::to_string(i) + ": " + std::to_string(got) + " vs " + std::to_string(ref::eq_group_gibbs[i]));
        expect(std::abs(th::debye_huckel_factor(vec({ionic}), temperature).host<double>().front() - ref::eq_dh[i]) < 1e-9 * (1.0 + std::abs(ref::eq_dh[i])), "Debye-Hueckel factor vs eQuilibrator");
      }
      // DH(I, T) = R T alpha_Alberty sqrt(I)/(1 + 1.6 sqrt(I)) with alpha = 1.17582 at 298.15 K (Alberty), within the 1e-3 of the published value
      const double alpha = th::debye_huckel_factor(vec({1.0}), 298.15).host<double>().front() * (1.0 + 1.6) / (th::gas_constant() * 298.15);
      expect(std::abs(alpha - 1.17582) < 1e-3, "alpha(298.15 K) = 1.17582 as quoted by Alberty, got " + std::to_string(alpha));
      expect(std::abs(th::magnesium_formation_gibbs(298.15) + 455300.0) < 1e-9, "Delta_f G(Mg2+) at 298.15 K");
    }

    // ---- MWC allosteric enzymes (Einav & Phillips 2017 eq. 1) against explicit enumeration of all 2^(m+1) microstates
    {
      const std::size_t cases = ref::mwc_cases.size() / 5, nc = ref::mwc_c.size();
      const Series c = vec(ref::mwc_c);
      for (std::size_t i = 0; i < cases; ++i) {
        const int m = static_cast<int>(ref::mwc_cases[5 * i]);
        const double ka = ref::mwc_cases[5 * i + 1], ki = ref::mwc_cases[5 * i + 2], be = ref::mwc_cases[5 * i + 3], b = ref::mwc_cases[5 * i + 4];
        const std::vector<double> pw(ref::mwc_p.begin() + static_cast<std::ptrdiff_t>(i * nc), ref::mwc_p.begin() + static_cast<std::ptrdiff_t>((i + 1) * nc));
        const std::vector<double> vw(ref::mwc_v.begin() + static_cast<std::ptrdiff_t>(i * nc), ref::mwc_v.begin() + static_cast<std::ptrdiff_t>((i + 1) * nc));
        expect(max_rel(enz::mwc_active_probability(c, m, ka, ki, be), pw) < 1e-12, "MWC active probability vs enumeration, case " + std::to_string(i));
        expect(max_rel(enz::mwc_enzyme_velocity(c, m, 1.0, ka, ki, be, b), vw) < 1e-12, "MWC enzyme velocity vs enumeration, case " + std::to_string(i));
        const auto ch = enz::mwc_characteristics(m, vec({ka}), vec({ki}), vec({be}));
        expect(rel(ch.leakiness.host<double>().front(), ref::mwc_chars[4 * i]) < 1e-13 && rel(ch.maximum.host<double>().front(), ref::mwc_chars[4 * i + 1]) < 1e-12, "MWC leakiness / maximum, case " + std::to_string(i));
        expect(rel(ch.ec50.host<double>().front(), ref::mwc_chars[4 * i + 2]) < 1e-8 && rel(ch.hill.host<double>().front(), ref::mwc_chars[4 * i + 3]) < 1e-6, "MWC EC50 and Hill coefficient, case " + std::to_string(i));
      }
      // no cooperativity when both states bind alike and turn over alike: v = Vmax c / (K + c)
      const Series cc = vec({0.1, 1.0, 10.0});
      expect(max_rel(enz::mwc_enzyme_velocity(cc, 4, 2.0, 1.5, 1.5, -2.0, 1.0), std::vector<double>{2.0 * 0.1 / 1.6, 2.0 * 1.0 / 2.5, 2.0 * 10.0 / 11.5}) < 1e-13, "K_A = K_I removes the cooperativity");
      // the rapid-equilibrium MWC rate law is the limit of the mass-action mechanism: lumped states (conformation, occupancy), m = 3,
      // binding of the j-th site (m - j) kon c, unbinding j koff, conformational exchange fast, catalysis j kcat only from the active ensemble
      {
        const int m = 3;
        const double kon = 1e5, koff = 1e5 * 0.5, kcat = 1.0;          // K_A = koff / kon = 0.5
        const double kon_t = 1e5, koff_t = 1e5 * 20.0;                  // K_I = 20
        const double kex = 1e6, l = std::exp(2.0);                      // L = e^{-beta eps} = e^{2}... T-bar = L R-bar at zero ligand
        std::vector<enz::Step> steps;
        // states 0..m: active with j occupied; m+1..2m+1: inactive with j occupied
        const auto act = [](int j) { return j; };
        const auto ina = [m](int j) { return m + 1 + j; };
        for (int j = 0; j < m; ++j) {
          steps.push_back(step(act(j), act(j + 1), (m - j) * kon, 0));
          steps.push_back(step(act(j + 1), act(j), (j + 1) * koff));
          steps.push_back(step(ina(j), ina(j + 1), (m - j) * kon_t, 0));
          steps.push_back(step(ina(j + 1), ina(j), (j + 1) * koff_t));
        }
        for (int j = 0; j <= m; ++j) { // detailed balance fixes the exchange: inactive/active population ratio at occupancy j is L (K_A/K_I)^j
          const double ratio = l * std::pow(0.5 / 20.0, j);
          steps.push_back(step(act(j), ina(j), kex * ratio / (1.0 + ratio)));
          steps.push_back(step(ina(j), act(j), kex / (1.0 + ratio)));
        }
        for (int j = 1; j <= m; ++j)
          steps.push_back(step(act(j), act(j - 1), j * kcat, -1, 1.0)); // turnover releases product and frees the site
        enz::Mechanism mech{2 * (m + 1), steps};
        const Series conc = vec({0.05, 0.3, 1.0, 5.0, 40.0}).unsqueeze(-1);
        const Series v = enz::turnover(mech, conc);
        const Series closed = enz::mwc_enzyme_velocity(vec({0.05, 0.3, 1.0, 5.0, 40.0}), m, static_cast<double>(m) * kcat, 0.5, 20.0, -2.0, 0.0);
        expect(max_rel(v, closed.contiguous().host<double>()) < 2e-3, "mass-action MWC mechanism approaches the rapid-equilibrium rate law (fast exchange)");
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "enzymes tests passed\n";
  return 0;
}
