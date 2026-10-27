// Polymer physics and polymerization kinetics against independent oracles (see verification/gen_polymers_ref.py).
#include "pluss/subsp/natsci/chemistry/polymer_dynamics.h"
#include "pluss/subsp/natsci/chemistry/polymers.h"
#include "pluss/subsp/natsci/chemistry/step_growth.h"
#include "pluss/subsp/natsci/common/population_balance.h"
#include "pluss/subsp/natsci/common/stiff.h"
#include "polymers_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace poly = pluss::subsp::natsci::chemistry::polymers;
namespace stiff = pluss::subsp::natsci::common::stiff;
namespace dyn = pluss::subsp::natsci::common::dynamics;
namespace pbal = pluss::subsp::natsci::common::population_balance;
namespace sg = pluss::subsp::natsci::chemistry::step_growth;
namespace pd = pluss::subsp::natsci::chemistry::polymer_dynamics;
namespace ref = polymers_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel(double a, double b) { return std::abs(a - b) / (1.0 + std::abs(b)); }
double max_rel(const Series &got, const std::vector<double> &want) {
  const auto g = got.contiguous().host<double>();
  expect(g.size() == want.size(), "size mismatch");
  double worst = 0.0;
  for (std::size_t i = 0; i < g.size(); ++i)
    worst = std::isfinite(g[i]) ? std::max(worst, rel(g[i], want[i])) : 1e300;
  return worst;
}
} // namespace

int main() {
  try {
    // ---- worm-like chain: <R^2> and <Rg^2> against quadrature of their definitions (P = 1)
    {
      const Series big_l = vec(ref::wlc_L);
      expect(max_rel(poly::wlc_mean_square_end_to_end(big_l, 1.0), ref::wlc_r2) < 1e-9, "wlc <R^2> vs quadrature");
      expect(max_rel(poly::wlc_radius_of_gyration_squared(big_l, 1.0), ref::wlc_rg2) < 1e-8, "wlc <Rg^2> vs quadrature");
      // limits: rod L^2 and L^2/12; coil 2PL and PL/3
      const Series tiny = vec({1e-6});
      expect(std::abs(poly::wlc_mean_square_end_to_end(tiny, 1.0).host<double>().front() / 1e-12 - 1.0) < 1e-6, "rod limit of <R^2>");
      expect(std::abs(poly::wlc_radius_of_gyration_squared(tiny, 1.0).host<double>().front() / (1e-12 / 12.0) - 1.0) < 1e-6, "rod limit of <Rg^2>");
      const Series huge = vec({1e6});
      expect(std::abs(poly::wlc_mean_square_end_to_end(huge, 1.0).host<double>().front() / 2e6 - 1.0) < 1e-5, "coil limit of <R^2>");
      expect(std::abs(poly::wlc_radius_of_gyration_squared(huge, 1.0).host<double>().front() / (1e6 / 3.0) - 1.0) < 1e-5, "coil limit of <Rg^2>");
    }
    // ---- freely-rotating chain against the brute-force double sum, and its continuum limit (worm-like chain)
    {
      const Series n = vec(ref::frc_n), b = vec(ref::frc_b), c = vec(ref::frc_c);
      expect(max_rel(poly::freely_rotating_mean_square_end_to_end(n, b, c), ref::frc_r2) < 1e-11, "freely rotating chain vs brute force");
      const double bond = 0.001, persistence = 1.0, contour = 5.0;
      const Series frc = poly::freely_rotating_mean_square_end_to_end(vec({contour / bond}), bond, 1.0 - bond / persistence);
      const double wlc = poly::wlc_mean_square_end_to_end(vec({contour}), persistence).host<double>().front();
      expect(std::abs(frc.host<double>().front() / wlc - 1.0) < 3e-3, "freely rotating chain approaches the worm-like chain");
    }
    // ---- force-extension
    {
      expect(max_rel(poly::fjc_extension(vec(ref::fjc_a), 1.0, 1.0), ref::fjc_x) < 1e-11, "Langevin function (incl. the small-force series branch)");
      expect(std::abs(poly::fjc_extension(vec({-0.3}), 1.0, 1.0).host<double>().front() + poly::fjc_extension(vec({0.3}), 1.0, 1.0).host<double>().front()) < 1e-15, "odd in the force");
      expect(std::abs(poly::wlc_force(vec({0.5}), 2.0, 4.0).host<double>().front() - 2.5) < 1e-13, "Marko-Siggia spot value: f P / kT = 5/4");
      const Series f = vec({0.01, 0.1, 1.0, 5.0, 50.0});
      const Series z = poly::wlc_extension(f, 1.0, 1.0);
      expect(poly::wlc_force(z, 1.0, 1.0).sub(f).abs().max().item<double>() < 1e-9, "wlc_extension inverts wlc_force");
      const auto zz = z.contiguous().host<double>();
      expect(zz[0] > 0.0 && zz[4] < 1.0 && zz[1] > zz[0] && zz[3] > zz[2], "extension is monotone and bounded by the contour length");
    }
    // ---- distributions
    {
      const Series x = pluss::make_series(std::vector<double>(300), {300}).to(F64);
      std::vector<double> xs(300);
      for (int i = 0; i < 300; ++i)
        xs[static_cast<std::size_t>(i)] = i + 1.0;
      const Series xv = vec(xs);
      (void)x;
      const double p = ref::fs_p[0];
      expect(max_rel(poly::flory_number_fraction(xv, p), ref::fs_n) < 1e-12, "Flory number fraction vs scipy geom");
      expect(max_rel(poly::flory_weight_fraction(xv, p), ref::fs_w) < 1e-12, "Flory weight fraction");
      std::vector<double> xl(3000);
      for (int i = 0; i < 3000; ++i)
        xl[static_cast<std::size_t>(i)] = i + 1.0;
      const Series xlong = vec(xl);
      const auto mw = poly::molecular_weights(xlong, poly::flory_number_fraction(xlong, p));
      expect(std::abs(mw.mn.item<double>() - 1.0 / (1.0 - p)) < 1e-9 * (1.0 / (1.0 - p)), "Mn = 1/(1-p)");
      expect(std::abs(mw.mw.item<double>() - (1.0 + p) / (1.0 - p)) < 1e-9 * ((1.0 + p) / (1.0 - p)), "Mw = (1+p)/(1-p)");
      expect(std::abs(mw.dispersity.item<double>() - (1.0 + p)) < 1e-9, "dispersity 1 + p");
      expect(std::abs(poly::carothers_degree_of_polymerization(vec({0.99}), 1.0).host<double>().front() - 100.0) < 1e-10, "Carothers X_n at p = 0.99");
      expect(std::abs(poly::carothers_degree_of_polymerization(vec({0.99}), 0.98).host<double>().front() - 1.98 / (1.98 - 2 * 0.98 * 0.99)) < 1e-10, "Carothers with stoichiometric imbalance");
      expect(max_rel(poly::poisson_number_fraction(xv, ref::po_nu[0]), ref::po_n) < 1e-11, "Poisson distribution vs scipy");
      const auto pw = poly::molecular_weights(xv, poly::poisson_number_fraction(xv, ref::po_nu[0]));
      expect(rel(pw.mn.item<double>(), ref::po_avg[0]) < 1e-11 && rel(pw.mw.item<double>(), ref::po_avg[1]) < 1e-11 && rel(pw.mz.item<double>(), ref::po_avg[2]) < 1e-11, "Poisson averages");
      expect(std::abs(pw.dispersity.item<double>() - (1.0 + ref::po_nu[0] / ((ref::po_nu[0] + 1.0) * (ref::po_nu[0] + 1.0)))) < 1e-9, "Poisson dispersity 1 + nu/(nu+1)^2");
    }
    // ---- free-radical polymerization: closed moment equations vs the chain-length-resolved kinetics
    {
      poly::RadicalKinetics k;
      k.kd = ref::rp_params[0]; k.f = ref::rp_params[1]; k.kp = ref::rp_params[2]; k.ktc = ref::rp_params[3]; k.ktd = ref::rp_params[4]; k.ktm = ref::rp_params[5];
      const Series y0 = vec({ref::rp_params[6], ref::rp_params[7], 0, 0, 0, 0, 0, 0});
      stiff::Options o;
      o.rtol = std::getenv("POLY_RTOL") ? std::atof(std::getenv("POLY_RTOL")) : 1e-8;
      o.atol = 1e-18;
      o.max_steps = 4000000;
      const Series y = stiff::integrate_at(poly::free_radical_rates(k), y0, ref::rp_times, o);
      const auto got = y.contiguous().host<double>();
      double worst = 0.0;
      for (std::size_t t = 1; t < ref::rp_times.size(); ++t)
        for (int c = 0; c < 8; ++c) {
          const double want = ref::rp_states[t * 8 + static_cast<std::size_t>(c)];
          const double have = got[t * 8 + static_cast<std::size_t>(c)];
          const double e = std::abs(have - want) / (std::abs(want) + 1e-14);
          if (std::getenv("POLY_DEBUG") && e > 1e-6) std::fprintf(stderr, "t=%g comp %d have %.10g want %.10g err %.2e\n", ref::rp_times[t], c, have, want, e);
          worst = std::max(worst, e);
        }
      expect(worst < 2e-5, "radical moments vs chain-resolved kinetics, worst relative error " + std::to_string(worst));
      // quasi-steady state: lam0 = sqrt(R_i/k_t) (checked at t = 20 s where the initiator has hardly changed)
      const Series state20 = y.select(0, 1);
      const double lam0 = state20.select(-1, 2).item<double>();
      const double qssa = poly::qssa_radical_concentration(state20.select(-1, 0), k).item<double>();
      expect(std::abs(lam0 / qssa - 1.0) < 2e-3, "quasi-steady-state radical concentration");
      // molar masses of the accumulated polymer are finite and ordered
      const auto mw = poly::radical_molecular_weights(y.select(0, 3), 100.0);
      expect(mw.mn.item<double>() > 100.0 && mw.mw.item<double>() > mw.mn.item<double>() && mw.dispersity.item<double>() < 3.0, "Mn, Mw and dispersity are sensible");
      // mass balance: consumed monomer = polymer units in live + dead chains (+ nothing else)
      for (std::size_t t = 1; t < ref::rp_times.size(); ++t) {
        const double consumed = ref::rp_params[7] - got[t * 8 + 1];
        const double units = got[t * 8 + 3] + got[t * 8 + 6];
        expect(std::abs(consumed - units) < 1e-7 * (1.0 + consumed), "monomer mass balance");
      }
    }
    // ---- copolymerization
    {
      const Series f1 = vec(ref::ml_f1);
      std::vector<double> r1v = ref::ml_r1, r2v = ref::ml_r2;
      expect(max_rel(poly::mayo_lewis(f1, vec(r1v), vec(r2v)), ref::ml_F1) < 1e-13, "Mayo-Lewis composition");
      const Series az = poly::azeotrope(0.5, 0.3, f1);
      expect(std::abs(poly::mayo_lewis(az, 0.5, 0.3).sub(az).abs().max().item<double>()) < 1e-13, "azeotrope: F1 = f1");
      for (std::size_t i = 0; i < ref::sk_f10.size(); ++i) {
        const double got = poly::skeist_feed_fraction(vec({ref::sk_f10[i]}), ref::sk_r1[i], ref::sk_r2[i], 0.6, 600).host<double>().front();
        expect(rel(got, ref::sk_f1[i]) < 1e-9, "Skeist drift vs DOP853, case " + std::to_string(i));
        const double conv = poly::meyer_lowry_conversion(vec({ref::sk_f1[i]}), vec({ref::sk_f10[i]}), ref::sk_r1[i], ref::sk_r2[i]).host<double>().front();
        expect(std::abs(conv - 0.6) < 1e-9, "Meyer-Lowry closed form reproduces the conversion, case " + std::to_string(i));
      }
    }
    // ---- Flory-Huggins
    {
      // symmetric blends: binodal (1 -+ m)/2, ln((1+m)/(1-m)) = chi N m
      const Series n = vec(ref::fhs_n), chi = vec(ref::fhs_chi);
      const auto bin = poly::fh_binodal(chi, n, n);
      expect(max_rel(bin.low, ref::fhs_a) < 1e-9 && max_rel(bin.high, ref::fhs_b) < 1e-9, "symmetric binodal vs the exact relation");
      // asymmetric (polymer solutions): scipy fsolve of the coexistence conditions
      const Series n1 = vec(ref::fha_n1), n2 = vec(ref::fha_n2), chia = vec(ref::fha_chi);
      const auto asym = poly::fh_binodal(chia, n1, n2);
      expect(max_rel(asym.low, ref::fha_a) < 1e-7 && max_rel(asym.high, ref::fha_b) < 1e-7, "asymmetric binodal vs fsolve");
      // coexistence conditions hold at the computed points
      for (int which = 0; which < 2; ++which) {
        const Series a = asym.low, b = asym.high;
        const Series r1 = poly::fh_mu1_per_site(a, n1, n2, chia).sub(poly::fh_mu1_per_site(b, n1, n2, chia));
        const Series r2 = poly::fh_mu2_per_site(a, n1, n2, chia).sub(poly::fh_mu2_per_site(b, n1, n2, chia));
        if (std::getenv("POLY_DEBUG")) std::fprintf(stderr, "mu residuals %.3e %.3e\n", r1.abs().max().item<double>(), r2.abs().max().item<double>());
        expect(r1.abs().max().item<double>() < 1e-11 && r2.abs().max().item<double>() < 1e-11, "equal chemical potentials");
      }
      // critical point: spinodal minimum, chi_c = (1/sqrt n1 + 1/sqrt n2)^2 / 2, phi_c = sqrt n1 / (sqrt n1 + sqrt n2)
      const auto crit = poly::fh_critical(vec({1.0}), vec({100.0}));
      expect(std::abs(crit.chi.host<double>().front() - 0.605) < 1e-13 && std::abs(crit.phi.host<double>().front() - 1.0 / 11.0) < 1e-13, "critical point of a polymer solution");
      const Series phi = vec({0.05, 0.0909090909090909, 0.15, 0.3});
      const Series cs = poly::fh_spinodal_chi(phi, 1.0, 100.0);
      expect(cs.min().host<double>().front() >= crit.chi.host<double>().front() - 1e-12, "spinodal lies above the critical chi");
      const auto sp = poly::fh_spinodal_compositions(vec({0.8}), vec({1.0}), vec({100.0}));
      expect(std::abs(poly::fh_spinodal_chi(sp.low, 1.0, 100.0).host<double>().front() - 0.8) < 1e-12 && std::abs(poly::fh_spinodal_chi(sp.high, 1.0, 100.0).host<double>().front() - 0.8) < 1e-12, "spinodal compositions satisfy chi_s(phi) = chi");
    }

    // ---- step-growth kinetics: closed forms against ODE integration of the rate laws
    {
      const Series t = vec(ref::sg_t);
      const double k = ref::sg_params[0], c0 = ref::sg_params[1];
      expect(max_rel(sg::conversion_third_order(t, k, c0), ref::sg_third_p) < 1e-10, "third-order conversion vs DOP853");
      for (std::size_t i = 0; i < ref::sg_r.size(); ++i) {
        const std::vector<double> want(ref::sg_imbalanced_p.begin() + static_cast<std::ptrdiff_t>(i * ref::sg_t.size()), ref::sg_imbalanced_p.begin() + static_cast<std::ptrdiff_t>((i + 1) * ref::sg_t.size()));
        expect(max_rel(sg::conversion_imbalanced(t, k, c0, ref::sg_r[i]), want) < 1e-10, "imbalanced conversion vs DOP853, r = " + std::to_string(ref::sg_r[i]));
      }
      expect(max_rel(sg::conversion_second_order(t, k, c0), std::vector<double>(ref::sg_imbalanced_p.end() - static_cast<std::ptrdiff_t>(ref::sg_t.size()), ref::sg_imbalanced_p.end())) < 1e-10, "second-order conversion equals the r = 1 integration");
      // closed moment equations vs the Flory distribution at the same conversion
      const double c = 1.3, kk = 0.9, tt = 2.5;
      const Series y0 = vec({c, c, c, c}); // monodisperse monomers: M_k(0) = c
      const Series y = dyn::simulate(sg::linear_moment_rates(kk), y0, 0.0, tt, 400);
      const double p = kk * c * tt / (1.0 + kk * c * tt);
      std::vector<double> xs(4000);
      for (int i = 0; i < 4000; ++i)
        xs[static_cast<std::size_t>(i)] = i + 1.0;
      const auto mw = poly::molecular_weights(vec(xs), poly::flory_number_fraction(vec(xs), p));
      const double xn = y.select(-1, 1).item<double>() / y.select(-1, 0).item<double>(), xw = y.select(-1, 2).item<double>() / y.select(-1, 1).item<double>(), xz = y.select(-1, 3).item<double>() / y.select(-1, 2).item<double>();
      expect(rel(xn, mw.mn.item<double>()) < 1e-9 && rel(xw, mw.mw.item<double>()) < 1e-8 && rel(xz, mw.mz.item<double>()) < 1e-8, "step-growth moment equations reproduce the Flory distribution averages");
      expect(rel(xw / xn, 1.0 + p) < 1e-9, "dispersity 1 + p");
    }
    // ---- AB_{f-1}: Flory weight-average degree of polymerization against the Smoluchowski kinetics of the reactive group pairs
    {
      expect(max_rel(sg::afb_weight_average(vec(ref::afb_alpha), 3.0), ref::afb_xw_f3) < 2e-6, "AB2 weight average vs Smoluchowski kinetics");
      expect(max_rel(sg::afb_weight_average(vec(ref::afb_alpha_f4), 4.0), ref::afb_xw_f4) < 2e-6, "AB3 weight average vs Smoluchowski kinetics");
      expect(std::abs(sg::afb_critical_conversion(3.0, vec({0.0})).item<double>() - 0.5) < 1e-15, "AB2 gel point");
      expect(sg::afb_weight_average(vec({0.4999}), 3.0).host<double>().front() > 1e3, "X_w diverges at the gel point");
      expect(rel(sg::afb_weight_average(vec({0.3}), 2.0).host<double>().front(), (1.0 + 0.3) / (1.0 - 0.3)) < 1e-14, "f = 2 reduces to the linear (1+a)/(1-a)");
    }
    // ---- A_f + B_g gelation: critical conversion and sol fractions against Monte Carlo random bonding
    {
      const Series f = vec(ref::gel_f), g = vec(ref::gel_g), r = vec(ref::gel_r), pa = vec(ref::gel_pa);
      const auto fv = f.host<double>(), gv = g.host<double>();
      const auto rv = r.host<double>(), pv = pa.host<double>();
      for (std::size_t i = 0; i < fv.size(); ++i) {
        const auto s = sg::sol_gel(vec({pv[i]}), vec({rv[i]}), fv[i], gv[i]);
        expect(std::abs(s.sol_fraction.host<double>().front() - ref::gel_sol_mc[i]) < 8e-3, "sol fraction vs Monte Carlo, case " + std::to_string(i) + ": " + std::to_string(s.sol_fraction.host<double>().front()) + " vs " + std::to_string(ref::gel_sol_mc[i]));
        const double pc = sg::critical_conversion(fv[i], gv[i], vec({rv[i]})).host<double>().front();
        expect((pv[i] > pc) == (s.sol_fraction.host<double>().front() < 1.0 - 1e-9), "gel exists exactly beyond the critical conversion");
      }
      const Series below = vec({0.3, 0.5});
      const auto none = sg::sol_gel(below, vec({1.0, 1.0}), 4.0, 2.0);
      expect(none.sol_fraction.sub(1.0).abs().max().item<double>() < 1e-12, "no gel below the critical conversion (f=4, g=2, r=1: pc = 1/sqrt(3) = 0.577)");
    }
    // ---- Rouse and Zimm dynamics
    {
      // exact Rouse eigenvalues vs numpy eigvalsh of the connectivity matrix
      for (std::size_t i = 0; i < ref::ro_n.size(); ++i) {
        const auto n = static_cast<std::int64_t>(ref::ro_n[i]);
        const auto ev = pd::rouse_eigenvalues(n).host<double>();
        for (int q = 0; q < 3; ++q)
          expect(std::abs(ev[static_cast<std::size_t>(q + 1)] - ref::ro_eig[i * 3 + static_cast<std::size_t>(q)]) < 1e-12, "Rouse eigenvalue vs eigvalsh");
      }
      // Doi-Edwards limit tau_p -> zeta N^2 b^2 / (3 pi^2 kT p^2) and eta -> c zeta N^2 b^2 / 36 for large N
      const std::int64_t n = 400;
      const Series tau = pd::rouse_relaxation_times(n, vec({1.0}), 1.0, 1.0);
      const auto tv = tau.host<double>();
      expect(std::abs(tv[0] / (static_cast<double>(n) * n / (3.0 * M_PI * M_PI)) - 1.0) < 1e-3, "tau_1 -> zeta N^2 b^2 / (3 pi^2 kT)");
      expect(std::abs(tv[0] / tv[1] - 4.0) < 1e-2, "tau_p ~ p^-2");
      expect(std::abs(pd::rouse_viscosity(tau, 1.0, 1.0).host<double>().front() / (static_cast<double>(n) * n / 36.0) - 1.0) < 2e-3, "Rouse viscosity c zeta N^2 b^2 / 36");
      expect(std::abs(pd::rouse_diffusion(n, vec({2.0}), 1.0).host<double>().front() - 1.0 / (2.0 * n)) < 1e-15, "Rouse diffusion kT/(N zeta)");
      // G(0) = c kT (N - 1) and monotone decay
      const Series modulus = pd::rouse_modulus(vec({0.0, 1.0, 100.0, 1e6}), tau, 1.0, 1.0);
      const auto gv = modulus.host<double>();
      expect(std::abs(gv[0] - (n - 1)) < 1e-12 && gv[1] > gv[2] && gv[2] > gv[3] && gv[3] < 1e-6, "Rouse modulus G(0) = c kT (N-1), monotone decay");
      // Zimm eigenvalues and diffusion vs numpy
      for (std::size_t i = 0; i < ref::zi_n.size(); ++i) {
        const auto nb = static_cast<std::int64_t>(ref::zi_n[i]);
        const auto lam = pd::zimm_eigenvalues(nb, ref::zi_hs[i]).host<double>();
        for (int q = 0; q < 4; ++q)
          expect(rel(lam[static_cast<std::size_t>(q + 1)], ref::zi_lambda_1_4[i * 4 + static_cast<std::size_t>(q)]) < 1e-9, "Zimm eigenvalue vs numpy eigvals(H A), case " + std::to_string(i));
        expect(rel(pd::zimm_diffusion(nb, ref::zi_hs[i], 1.0, 1.0), ref::zi_dsum[i]) < 1e-12, "Zimm diffusion kT/(N^2 zeta) sum H vs numpy");
      }
      // hydrodynamic interaction speeds the chain up: tau^Z_1 < tau^R_1, and D grows as N^-1/2 instead of N^-1 (large-N scaling)
      const double tz = pd::zimm_relaxation_times(100, 0.15, 1.0, 1.0, 1.0).select(-1, 0).item<double>();
      const double tr = pd::rouse_relaxation_times(100, vec({1.0}), 1.0, 1.0).select(-1, 0).host<double>().front();
      expect(tz < tr, "Zimm relaxes faster than Rouse");
      const double d1 = pd::zimm_diffusion(50, 0.25, 1.0, 1.0), d2 = pd::zimm_diffusion(200, 0.25, 1.0, 1.0);
      const double exponent = std::log(d1 / d2) / std::log(4.0);
      expect(exponent > 0.45 && exponent < 0.75, "Zimm D ~ N^-1/2 (fitted exponent " + std::to_string(exponent) + ")");
      expect(std::abs(pd::zimm_diffusion(30, 0.0, 2.0, 1.0) - 1.0 / (30.0 * 2.0)) < 1e-14, "h* = 0 recovers Rouse diffusion");
    }

    // ---- reversible step growth: MIT OCW 10.569 equilibrium formulas, kinetics against ODE integration, distribution against the population balance
    {
      // equilibrium relations
      const Series k = vec({1.0, 9.0, 100.0, 1000.0});
      expect(max_rel(sg::equilibrium_dp_closed(k), std::vector<double>{2.0, 4.0, 11.0, 1.0 + std::sqrt(1000.0)}) < 1e-14, "closed-system X_n = 1 + sqrt(K)");
      expect(max_rel(sg::equilibrium_conversion_closed(k), std::vector<double>{0.5, 0.75, 10.0 / 11.0, std::sqrt(1000.0) / (1.0 + std::sqrt(1000.0))}) < 1e-14, "closed-system conversion");
      // open system: [W]_eq = K c0 / (X_n (X_n - 1)) and its inverse
      const Series dp = vec({10.0, 40.0, 200.0});
      const Series w = sg::equilibrium_condensate(dp, vec({200.0, 200.0, 200.0}), 2.0);
      expect(max_rel(w, std::vector<double>{200.0 * 2.0 / 90.0, 200.0 * 2.0 / 1560.0, 200.0 * 2.0 / 39800.0}) < 1e-14, "condensate level for a target X_n (K = 200, c0 = 2 M)");
      expect(max_rel(sg::equilibrium_dp_open(vec({200.0, 200.0, 200.0}), w, 2.0), std::vector<double>{10.0, 40.0, 200.0}) < 1e-12, "X_n(w) inverts w(X_n)");
      // kinetics vs DOP853 integration of dp/dt = k c0 [(1-p)^2 - p^2/K] (closed system: w = c0 p)
      for (std::size_t i = 0; i < ref::rev_params.size() / 3; ++i) {
        const double kf = ref::rev_params[3 * i], c0 = ref::rev_params[3 * i + 1], keq = ref::rev_params[3 * i + 2];
        const std::vector<double> want(ref::rev_p.begin() + static_cast<std::ptrdiff_t>(i * ref::rev_t.size()), ref::rev_p.begin() + static_cast<std::ptrdiff_t>((i + 1) * ref::rev_t.size()));
        std::vector<double> got;
        for (const double t : ref::rev_t) {
          // state [p, w]: the condensate equals c0 p in a closed system, so start from w = 0 and let the production term fill it
          const Series y = dyn::simulate(sg::reversible_rates(kf, keq, c0), vec({0.0, 0.0}), 0.0, t, 4000);
          got.push_back(y.select(-1, 0).item<double>());
        }
        expect(max_rel(vec(got), want) < 1e-6, "reversible conversion vs DOP853, case " + std::to_string(i));
        const Series yeq = dyn::simulate(sg::reversible_rates(kf, keq, c0), vec({0.0, 0.0}), 0.0, 400.0, 8000);
        expect(rel(yeq.select(-1, 0).item<double>(), std::sqrt(keq) / (1.0 + std::sqrt(keq))) < 1e-6, "long-time conversion is sqrt(K)/(1+sqrt(K))");
      }
      // open system with the condensate held at w: the steady state p solves K c0 (1-p)^2 = w p
      for (std::size_t i = 0; i < ref::open_p.size(); ++i) {
        const double keq = ref::open_params[3 * i], c0 = ref::open_params[3 * i + 1], wc = ref::open_params[3 * i + 2];
        const Series dpv = sg::equilibrium_dp_open(vec({keq}), vec({wc}), c0);
        expect(rel(dpv.host<double>().front(), ref::open_dp[i]) < 1e-12, "open-system X_n vs the numerical root, case " + std::to_string(i));
        // condensate removal fast enough to hold w small: simulate with the rate law and an imposed w by making removal strong: p follows the quasi-steady value
      }
      // the molecular-weight distribution: population balance with random coupling and random scission relaxes to the most probable distribution
      {
        const std::int64_t bins = 80;
        const auto g = pbal::integer_grid(bins);
        const auto agg = pbal::build_aggregation_operator(g, vec({0.0}));
        std::vector<double> bk;
        const double keq = 9.0, kf = 1.0, krate = kf / keq; // per-pair coupling rate kf (kernel 2 kf), per-bond scission rate k_r [W]
        const double c0 = 1.0;
        std::vector<double> kern(static_cast<std::size_t>(bins * bins), 2.0 * kf);
        const Series kernel = vec(kern).reshape({bins, bins});
        const auto brk = pbal::build_breakage_operator(g, [](double x, double y) { return y > 1.5 ? 2.0 / (y - 1.0) * (x >= 1.0 && x <= y - 1.0 ? 1.0 : 0.0) : 0.0; }, vec({0.0}), 8);
        (void)brk;
        // discrete scission operator built directly: chain k breaks at k_r' (k-1) into (i, k-i), uniform over the k-1 positions; k_r' = k_r w with w = c0 p
        // (water produced by the bonds themselves in this closed system), so the scission rate follows the conversion p = 1 - M0/M1
        const dyn::RateFn rate = [&](const Series &n) {
          const Series gain_loss = pbal::aggregation_rhs(n, agg, kernel);
          const Series m0 = n.sum(-1, true), m1 = pbal::pivot_moment(n, g, 1.0).unsqueeze(-1);
          const Series p = m0.div(m1).mul(-1.0).add(1.0);
          std::vector<double> scission(static_cast<std::size_t>(bins * bins), 0.0);
          for (std::int64_t k2 = 1; k2 < bins; ++k2)       // parent k2 + 1 (index k2): positions j = 1..k2
            for (std::int64_t j = 1; j <= k2; ++j) {
              scission[static_cast<std::size_t>(k2 * bins + (j - 1))] += 1.0 / static_cast<double>(k2);
              scission[static_cast<std::size_t>(k2 * bins + (k2 - j))] += 1.0 / static_cast<double>(k2);
            }
          const Series d = vec(scission).reshape({bins, bins});
          std::vector<double> bonds(static_cast<std::size_t>(bins));
          for (std::int64_t k2 = 0; k2 < bins; ++k2)
            bonds[static_cast<std::size_t>(k2)] = static_cast<double>(k2); // a chain of length k2 + 1 has k2 bonds
          const Series per_chain = vec(bonds).mul(p.mul(c0 * krate));      // scission rate of each chain: k_r w (k - 1), w = c0 p
          const Series broken = n.mul(per_chain);
          return gain_loss.add(broken.matmul(d)).sub(broken);
        };
        std::vector<double> n0(static_cast<std::size_t>(bins), 0.0);
        n0[0] = c0;
        const Series y = dyn::simulate(rate, vec(n0), 0.0, 60.0, 3000);
        const double p_eq = std::sqrt(keq) / (1.0 + std::sqrt(keq));
        const auto nv = y.host<double>();
        double total = 0.0;
        for (const double v : nv) total += v;
        double worst = 0.0;
        for (std::int64_t kk = 1; kk <= 20; ++kk)
          worst = std::max(worst, std::abs(nv[static_cast<std::size_t>(kk - 1)] / total - (1.0 - p_eq) * std::pow(p_eq, kk - 1)));
        expect(worst < 2e-3, "reversible population balance relaxes to the most probable distribution with p = sqrt(K)/(1+sqrt(K)), worst " + std::to_string(worst));
        expect(std::abs(c0 / total - 1.0 - std::sqrt(keq)) < 5e-3, "X_n of the relaxed distribution is 1 + sqrt(K)");
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "polymers tests passed\n";
  return 0;
}
