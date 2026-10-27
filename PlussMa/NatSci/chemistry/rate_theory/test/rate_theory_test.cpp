// Statistical rate theories: partition functions, TST/Eyring, collision theory, Beyer-Swinehart densities of states (vs brute-force enumeration) and RRKM
// with its exact link to TST and the strong-collision fall-off.
#include "pluss/subsp/natsci/chemistry/rate_theory.h"
#include "rate_theory_reference.h"
#include "rotor_reference.h"
#include "master_reference.h"
#include "eckart_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace rt = pluss::subsp::natsci::chemistry::rate_theory;
namespace ref = rate_theory_reference;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel(double a, double b) { return std::abs(a / b - 1.0); }
} // namespace

int main() {
  try {
    // ---- partition functions
    expect(rel(rt::translational_per_volume(ref::trans[0], vec({ref::trans[1]})).host<double>().front(), ref::trans[2]) < 1e-13, "translational partition function per volume");
    expect(rel(rt::rotational_linear(ref::rot_lin[0], ref::rot_lin[1], vec({ref::rot_lin[2]})).host<double>().front(), ref::rot_lin[3]) < 1e-13, "linear rotor");
    expect(rel(rt::rotational_nonlinear(ref::rot_nl[0], ref::rot_nl[1], ref::rot_nl[2], ref::rot_nl[3], vec({ref::rot_nl[4]})).host<double>().front(), ref::rot_nl[5]) < 1e-13, "non-linear rotor");
    expect(rel(rt::vibrational(vec({ref::vib[1], ref::vib[2], ref::vib[3]}), vec({ref::vib[0]}).select(0, 0)).item<double>(), ref::vib[4]) < 1e-13, "vibrational partition function");
    // lanes over temperature: monotone in T, and the harmonic limit Q ~ kT / (h c nu) for kT >> h c nu
    {
      const Series t = vec({300.0, 600.0, 1200.0});
      const Series q = rt::vibrational(vec({200.0}).unsqueeze(0).expand({3, 1}), t);
      expect(q.slice(0, 1, 3, 1).sub(q.slice(0, 0, 2, 1)).min().item<double>() > 0.0, "Q_vib grows with temperature");
      expect(rel(rt::vibrational(vec({1.0, 1.0}), vec({1e6}).select(0, 0)).item<double>(), std::pow(rt::boltzmann() * 1e6 / (rt::planck() * rt::light_speed_cm()), 2.0)) < 1e-5, "classical limit of two soft modes");
    }
    // ---- transition-state theory, tunnelling, collision theory
    expect(rel(rt::wigner(vec({ref::wigner[0]}), vec({ref::wigner[1]})).host<double>().front() - 1.0, ref::wigner[2] - 1.0) < 1e-13, "Wigner correction");
    expect(rel(rt::collision_rate(ref::collision[0], ref::collision[1], ref::collision[2], vec({ref::collision[3]}), ref::collision[4]).host<double>().front(), ref::collision[5]) < 1e-13, "collision-theory rate constant");
    {
      // Eyring: k = (kT/h) exp(-dG/RT); a free-energy barrier of 80 kJ/mol at 298.15 K is ~ 6.7e-2 /s... compare with the direct formula
      const double t = 298.15, dg = 80.0e3;
      const double direct = rt::boltzmann() * t / rt::planck() * std::exp(-dg / (rt::gas_constant() * t));
      expect(rel(rt::eyring_rate(vec({dg}), vec({t})).host<double>().front(), direct) < 1e-13, "Eyring rate");
      expect(rel(rt::eyring_rate(vec({dg}), vec({t}), 1.0, 2.0).host<double>().front(), direct / 1000.0) < 1e-13, "bimolecular standard-state factor c0^(1 - m)");
      // TST from partition functions equals Eyring when Q^‡/Q = exp(dS/R)
      const double ds = -40.0, dh = 65.0e3;
      const Series q = vec({std::exp(ds / rt::gas_constant())});
      expect(rel(rt::tst_rate(q, vec({1.0}), vec({dh}), vec({t})).host<double>().front(), rt::eyring_rate(vec({dh - t * ds}), vec({t})).host<double>().front()) < 1e-13, "TST with Q^‡/Q = e^{dS/R} is the Eyring rate");
      const auto fit = rt::eyring_fit(vec(ref::eyring_T), vec(ref::eyring_k));
      expect(rel(fit.enthalpy.item<double>(), ref::eyring_fit[0]) < 1e-9 && std::abs(fit.entropy.item<double>() - ref::eyring_fit[1]) < 1e-6, "Eyring analysis vs numpy polyfit: dH " + std::to_string(fit.enthalpy.item<double>()) + ", dS " + std::to_string(fit.entropy.item<double>()));
      expect(std::abs(fit.enthalpy.item<double>() - 65.0e3) < 5.0 * fit.enthalpy_error.item<double>(), "the generating enthalpy lies within five standard errors");
    }
    // ---- Beyer-Swinehart densities of states vs brute-force enumeration of the harmonic state counts
    {
      const std::vector<std::int64_t> reactant{350, 520, 810, 1130, 1490}, ts{350, 520, 810, 1490};
      const auto bins = static_cast<std::int64_t>(ref::bs_check_bins[0]);
      const auto rr = rt::beyer_swinehart(reactant, bins).contiguous().host<double>(), rtv = rt::beyer_swinehart(ts, bins).contiguous().host<double>();
      for (std::size_t i = 0; i < ref::rho_r_sample.size(); ++i) {
        expect(rr[i * 250] == ref::rho_r_sample[i] && rtv[i * 250] == ref::rho_t_sample[i], "state counts at E = " + std::to_string(i * 250) + " cm^-1");
      }
      double total = 0.0;
      for (double v : rr) total += v;
      expect(total > 100.0, "non-trivial number of states");
    }
    // ---- RRKM with a quasi-continuum (a 1 cm^-1 mode): k(E) samples, the exact TST identity, and the fall-off
    {
      std::vector<std::int64_t> reactant, ts;
      for (double w : ref::setB_reactant) reactant.push_back(static_cast<std::int64_t>(w));
      for (double w : ref::setB_ts) ts.push_back(static_cast<std::int64_t>(w));
      const auto bins = static_cast<std::int64_t>(ref::bins_b[0]);
      const Series rr = rt::beyer_swinehart(reactant, bins), rts = rt::beyer_swinehart(ts, bins);
      const Series temperature = vec({ref::rrkm[0]}).select(0, 0);
      const auto r = rt::rrkm(rr, rts, static_cast<std::int64_t>(ref::rrkm[1]), temperature);
      const auto k = r.rate.contiguous().host<double>();
      for (std::size_t i = 0; i < ref::k_sample.size(); ++i)
        expect(rel(k[static_cast<std::size_t>(ref::k_sample_idx[i])], ref::k_sample[i]) < 1e-11, "k(E) at " + std::to_string(ref::k_sample_idx[i]) + " cm^-1");
      expect(rel(r.partition.host<double>().front(), ref::rrkm[2]) < 1e-11, "reactant partition function from the density of states");
      const double kinf = rt::high_pressure_limit(r).host<double>().front();
      expect(rel(kinf, ref::rrkm[3]) < 1e-11, "k_inf vs direct summation");
      // the exact discrete link to TST: k_inf = k_TST x [(bin / kT) / (1 - e^{-bin/kT})]
      expect(rel(kinf / ref::rrkm[4], ref::rrkm[5]) < 1e-8, "k_inf / k_TST = delta / (1 - e^{-delta}): " + std::to_string(kinf / ref::rrkm[4]) + " vs " + std::to_string(ref::rrkm[5]));
      // fall-off from the strong-collision master-equation limit: five collision frequencies in one call
      const auto kuni = rt::strong_collision(r, vec(ref::omegas)).contiguous().host<double>();
      for (std::size_t i = 0; i < kuni.size(); ++i)
        expect(rel(kuni[i], ref::kuni[i]) < 1e-11, "k_uni at omega = " + std::to_string(ref::omegas[i]));
      for (std::size_t i = 1; i < kuni.size(); ++i) expect(kuni[i] > kuni[i - 1] && kuni[i] < kinf, "fall-off is monotone and bounded by k_inf");
      expect(rel(rt::strong_collision(r, vec({1e30})).host<double>().front(), kinf) < 1e-9, "omega -> infinity gives k_inf");
      expect(rel(rt::strong_collision(r, vec({1e-3})).host<double>().front() / 1e-3, ref::f_sum_above[0]) < 1e-6, "omega -> 0: every activated molecule reacts, k_uni = omega * P(E >= E0)");
      // weak-collision master equation (exponential-down kernel, 100 cm^-1 grains): eigenvalue vs scipy, the propagator decay rate, detailed balance, limits
      const auto me = rt::master_equation(r, static_cast<std::int64_t>(master_reference::me[0]), master_reference::me[1]);
      const auto ku = rt::weak_collision(me, vec(master_reference::me_omegas)).contiguous().host<double>();
      for (std::size_t i = 0; i < ku.size(); ++i)
        // the eigenvalue of a stiff symmetric matrix (norm ~ max k) is only defined to ~1e-14 of that norm by any dense eigensolver
        expect(std::abs(ku[i] - master_reference::me_k[i]) < 1e-13 * (master_reference::me[3] + master_reference::me_omegas[i]) + 1e-8 * master_reference::me_k[i], "weak-collision k_uni at omega = " + std::to_string(master_reference::me_omegas[i]) + ": " + std::to_string(ku[i]) + " vs " + std::to_string(master_reference::me_k[i]));
      expect(std::abs(master_reference::me_decay[1] / master_reference::me_decay[2] - 1.0) < 1e-3, "the smallest eigenvalue is the long-time decay rate of the propagator (scipy expm)");
      expect(me.coupling.sub(me.coupling.transpose(0, 1)).abs().max().item<double>() < 1e-14, "symmetrised collision operator");
      const Series sqrt_f = me.population.sqrt();
      expect(me.coupling.matmul(sqrt_f.unsqueeze(-1)).squeeze(-1).sub(sqrt_f).abs().max().item<double>() < 1e-12, "detailed balance: the Boltzmann vector is stationary under collisions");
      for (std::size_t i = 1; i < ku.size(); ++i) expect(ku[i] > ku[i - 1], "k_uni increases with pressure");
      const auto hi = rt::weak_collision(me, vec({1e15, 1e17})).contiguous().host<double>();
      expect(hi[0] > ku.back() && hi[1] > hi[0] && hi[1] < master_reference::me_kinf[0] * (1.0 + 1e-9) && rel(hi[1], master_reference::me_kinf[0]) < 2e-3, "omega -> infinity approaches k_inf = sum f k from below (the dense eigensolver resolves it up to ~1e-14 of the matrix norm): " + std::to_string(hi[1]) + " vs " + std::to_string(master_reference::me_kinf[0]));
      expect(std::abs(std::log(ku[1] / ku[0]) / std::log(master_reference::me_omegas[1] / master_reference::me_omegas[0]) - 1.0) < 0.02, "low-pressure limit: k_uni is proportional to omega");
      expect(ku[2] < rt::strong_collision(r, vec({master_reference::me_omegas[2]})).host<double>().front(), "weak collisions deactivate less efficiently than strong ones: k_uni below the strong-collision value");
    }
    // ---- hindered internal rotor: plane-wave Galerkin levels vs a Fourier-collocation diagonalisation, scipy Mathieu values (N = 1), limits
    {
      const double inertia = rotor_reference::hr_I[0];
      const std::size_t stride = 2 + 40;
      for (std::size_t c = 0; c < rotor_reference::hr_cases.size() / stride; ++c) {
        const double* row = &rotor_reference::hr_cases[c * stride];
        const auto n = static_cast<std::int64_t>(row[0]);
        const auto levels = rt::hindered_rotor_levels(inertia, row[1], n).contiguous().host<double>();
        for (std::size_t i = 0; i < 40; ++i)
          expect(std::abs(levels[i] - row[2 + i]) < 1e-9 * (std::abs(row[41]) + 1e-30) + 1e-30, "rotor level " + std::to_string(i) + " for N = " + std::to_string(n) + ", V0 = " + std::to_string(row[1]));
      }
      const auto n1 = rt::hindered_rotor_levels(inertia, rotor_reference::mathieu_n1[0], 1).contiguous().host<double>();
      for (std::size_t i = 0; i < 10; ++i)
        expect(std::abs(n1[i] - rotor_reference::mathieu_n1[1 + i]) < 1e-9 * rotor_reference::mathieu_n1[10] + 1e-30, "N = 1 level " + std::to_string(i) + " vs scipy Mathieu characteristic values");
      // partition functions and the free-rotor limit
      const auto lv = rt::hindered_rotor_levels(inertia, 6.0e-21, 3);
      for (std::size_t i = 0; i < rotor_reference::hr_Q.size() / 2; ++i)
        expect(rel(rt::hindered_rotor_partition(lv, vec({rotor_reference::hr_Q[2 * i]}), 1).host<double>().front(), rotor_reference::hr_Q[2 * i + 1]) < 1e-9, "hindered-rotor Q at T = " + std::to_string(rotor_reference::hr_Q[2 * i]));
      const auto free = rt::hindered_rotor_levels(inertia, 0.0, 3, 200);
      for (std::size_t i = 0; i < rotor_reference::free_Q.size() / 2; ++i)
        expect(rel(rt::hindered_rotor_partition(free, vec({rotor_reference::free_Q[2 * i]}), 1).host<double>().front(), rotor_reference::free_Q[2 * i + 1]) < 1e-9, "free-rotor limit at T = " + std::to_string(rotor_reference::free_Q[2 * i]));
      // high barrier: N-fold degenerate wells whose levels follow the large-q asymptotics of the Mathieu characteristic values (Abramowitz & Stegun 20.2.30):
      //   a_n ~ -2q + 2 w sqrt(q) - (w^2 + 1)/8 - w (w^2 + 3) / (128 sqrt(q)),  w = 2n + 1,  E_n = (hbar^2 N^2 / 8 I) a_n + V0/2,  q = 2 I V0 / (hbar^2 N^2)
      const double hbar = rt::planck() / (2.0 * std::numbers::pi), v0 = 1.6e-18, n_sym = 3.0;
      const double scale = hbar * hbar * n_sym * n_sym / (8.0 * inertia), q = 2.0 * inertia * v0 / (hbar * hbar * n_sym * n_sym), omega = n_sym * std::sqrt(v0 / (2.0 * inertia));
      const auto stiff = rt::hindered_rotor_levels(inertia, v0, 3, 90).contiguous().host<double>();
      for (int n = 0; n < 4; ++n) {
        const double w = 2.0 * n + 1.0, e_asym = scale * (2.0 * w * std::sqrt(q) - (w * w + 1.0) / 8.0 - w * (w * w + 3.0) / (128.0 * std::sqrt(q)));
        for (int copy = 0; copy < 3; ++copy)
          expect(std::abs(stiff[static_cast<std::size_t>(3 * n + copy)] - e_asym) < 2e-3 * hbar * omega, "level " + std::to_string(n) + " (copy " + std::to_string(copy) + ") vs the Mathieu large-q asymptotics: " + std::to_string(stiff[static_cast<std::size_t>(3 * n + copy)]) + " vs " + std::to_string(e_asym));
      }
      const double q_h = rt::hindered_rotor_partition(rt::hindered_rotor_levels(inertia, v0, 3, 90), vec({150.0}), 3).host<double>().front();
      const double q_harmonic = 1.0 / (2.0 * std::sinh(hbar * omega / (2.0 * rt::boltzmann() * 150.0)));
      expect(q_h > q_harmonic && q_h < 1.1 * q_harmonic, "the anharmonic (cosine-well) softening makes Q slightly larger than the harmonic value");
      // the barrier raises the levels and lowers the entropy: Q decreases with the barrier at fixed T
      const double q_low = rt::hindered_rotor_partition(rt::hindered_rotor_levels(inertia, 1.0e-21, 3), vec({300.0}), 3).host<double>().front(), q_high = rt::hindered_rotor_partition(rt::hindered_rotor_levels(inertia, 5.0e-20, 3), vec({300.0}), 3).host<double>().front();
      expect(q_high < q_low, "with energies measured from the well bottom a higher barrier means stiffer wells, hence a smaller Q at fixed T");
    }
    // ---- Eckart tunnelling vs direct integration of the Schroedinger equation through the Eckart potential
    {
      namespace ek = eckart_reference;
      double worst_t = 0.0, worst_k = 0.0;
      for (std::size_t c = 0; c < 4; ++c) {
        const double v1 = ek::cases[3 * c], v2 = ek::cases[3 * c + 1], nu = ek::cases[3 * c + 2];
        for (std::size_t i = 0; i < 4; ++i) {
          const double e = ek::T[2 * (4 * c + i)], tref = ek::T[2 * (4 * c + i) + 1];
          const double t = rt::eckart_transmission(vec({e}), vec({v1}), vec({v2}), vec({nu})).host<double>().front();
          worst_t = std::max(worst_t, std::abs(t - tref));
        }
        for (std::size_t j = 0; j < 3; ++j) {
          const double temp = ek::kappa[2 * (3 * c + j)], kref = ek::kappa[2 * (3 * c + j) + 1];
          const double k = rt::eckart(vec({v1}), vec({v2}), vec({nu}), vec({temp})).host<double>().front();
          worst_k = std::max(worst_k, rel(k, kref));
        }
      }
      expect(worst_t < 2e-8, "Eckart T(E) vs Schroedinger integration: " + std::to_string(worst_t));
      expect(worst_k < 1e-6, "Eckart kappa(T) vs Schroedinger integration: " + std::to_string(worst_k));
      // limits: kappa -> 1 at high T, Wigner series at low curvature, batching over temperature
      const Series kt = rt::eckart(vec({40e3}), vec({60e3}), vec({1000.0}), vec({2000.0, 20000.0}));
      expect(kt.select(0, 1).item<double>() < kt.select(0, 0).item<double>() && std::abs(kt.select(0, 1).item<double>() - 1.0) < 0.05, "kappa -> 1 as T grows");
      const double tw = 4000.0, wg = rt::wigner(vec({300.0}), vec({tw})).host<double>().front();
      expect(std::abs(rt::eckart(vec({30e3}), vec({30e3}), vec({300.0}), vec({tw})).host<double>().front() / wg - 1.0) < 5e-3, "Eckart reduces to Wigner for a broad barrier at high T");
    }
    std::cout << "rate_theory tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
