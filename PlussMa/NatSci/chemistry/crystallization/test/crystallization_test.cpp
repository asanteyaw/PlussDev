// Crystallization: classical nucleation theory, JMAK kinetics (vs Monte Carlo), and the MSMPR crystallizer (closed-form steady state, moment ODEs, finite-volume PBE).
#include "pluss/subsp/natsci/chemistry/crystallization.h"
#include "crystallization_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace cr = pluss::subsp::natsci::chemistry::crystallization;
namespace pbe = pluss::subsp::natsci::common::population_balance;
namespace dyn = pluss::subsp::natsci::common::dynamics;
namespace ref = crystallization_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
cr::Msmpr msmpr() {
  const auto &m = ref::msmpr;
  cr::Msmpr p;
  p.residence_time = m[0]; p.feed_concentration = m[1]; p.saturation = m[2]; p.growth_constant = m[3]; p.growth_order = m[4];
  p.nucleation_constant = m[5]; p.nucleation_order = m[6]; p.crystal_density = m[7]; p.shape_factor = m[8];
  return p;
}
} // namespace

int main() {
  try {
    // ---- classical nucleation theory: closed forms and identities
    {
      const double gam = ref::cnt[0], vm = ref::cnt[1], temp = ref::cnt[2];
      const Series s = vec({ref::cnt[3], ref::cnt[4], ref::cnt[5], ref::cnt[6]});
      const Series r = cr::critical_radius(s, gam, vm, temp), dg = cr::nucleation_barrier(s, gam, vm, temp);
      const auto rv = r.contiguous().host<double>(), dv = dg.contiguous().host<double>();
      for (std::size_t i = 0; i < 4; ++i)
        expect(std::abs(rv[i] / ref::cnt[7 + i] - 1.0) < 1e-12 && std::abs(dv[i] / ref::cnt[11 + i] - 1.0) < 1e-12, "CNT radius and barrier at S = " + std::to_string(ref::cnt[3 + i]));
      // dG* = (4 pi / 3) gamma r*^2 (Gibbs), higher supersaturation lowers the barrier and raises the rate
      expect(dg.div(r.square().mul(gam * 4.0 * M_PI / 3.0)).sub(1.0).abs().max().item<double>() < 1e-12, "dG* = (4 pi / 3) gamma r*^2");
      const Series j = cr::nucleation_rate(s, 1e30, gam, vm, temp).log();
      expect(j.slice(0, 1, 4, 1).sub(j.slice(0, 0, 3, 1)).min().item<double>() > 0.0, "nucleation rate increases with supersaturation");
    }
    // ---- JMAK: fitted exponent of the exact curve, and the Monte Carlo of nucleation and growth in a periodic box
    {
      const Series t = vec({0.2, 0.4, 0.6, 0.8, 1.0});
      const Series x = cr::jmak_fraction(t, 0.7, 3.0);
      const auto f = cr::avrami_fit(t, x);
      expect(std::abs(f.exponent.item<double>() - 3.0) < 1e-10 && std::abs(f.rate.item<double>() - 0.7) < 1e-10, "Avrami plot recovers n and k");
      const double i_rate = ref::jmak_mc[0], g = ref::jmak_mc[1], k = cr::jmak_constant_nucleation_3d(i_rate, g);
      for (std::size_t q = 0; q < 5; ++q) {
        const double ts = ref::jmak_mc[2 + q], mc = ref::jmak_mc[7 + q];
        const double pred = cr::jmak_fraction(vec({ts}), k, 4.0).host<double>().front();
        expect(std::abs(pred - mc) < 0.03, "JMAK (n = 4, k = pi I G^3 / 3) vs Monte Carlo at t = " + std::to_string(ts) + ": " + std::to_string(pred) + " vs " + std::to_string(mc));
      }
    }
    // ---- MSMPR: closed-form steady state vs fsolve of the moment ODEs, then the dynamics
    {
      const cr::Msmpr p = msmpr();
      const Series like = vec({0.0}).select(0, 0);
      const auto ss = cr::steady_state(like, p);
      const double c = ref::msmpr[9], g = ref::msmpr[14], b = ref::msmpr[15];
      expect(std::abs(ss.concentration.item<double>() - c) < 1e-9 && std::abs(ss.growth.item<double>() - g) < 1e-10 && std::abs(ss.nucleation.item<double>() - b) < 1e-10, "closed-form steady state: c = " + std::to_string(ss.concentration.item<double>()) + " vs " + std::to_string(c));
      const double tau = ref::msmpr[0];
      std::vector<double> y(5);
      for (int j = 0; j < 4; ++j) y[static_cast<std::size_t>(1 + j)] = cr::steady_moment(ss.n0, ss.growth, tau, j).item<double>();
      y[0] = c;
      for (int j = 0; j < 4; ++j)
        expect(std::abs(y[static_cast<std::size_t>(1 + j)] - ref::msmpr[10 + static_cast<std::size_t>(j)]) < 1e-8 * (1.0 + ref::msmpr[10 + static_cast<std::size_t>(j)]), "analytic moment mu_" + std::to_string(j));
      expect(cr::moment_rate(vec(y), p).abs().max().item<double>() < 1e-8, "the moment equations vanish at the steady state");
      // trajectory: RK4 of the moment equations vs DOP853
      const auto rate = [&](const Series &s) { return cr::moment_rate(s, p); };
      Series s = vec({39.0, 1.0, 5.0, 100.0, 4000.0});
      double t = 0.0;
      for (std::size_t i = 0; i < 4; ++i) {
        const double tn = ref::msmpr_traj[6 * i];
        s = dyn::simulate(rate, s, t, tn, static_cast<std::int64_t>((tn - t) / 0.002));
        t = tn;
        for (int j = 0; j < 5; ++j)
          expect(std::abs(s.select(0, j).item<double>() - ref::msmpr_traj[6 * i + 1 + static_cast<std::size_t>(j)]) < 1e-6 * (1.0 + std::abs(ref::msmpr_traj[6 * i + 1 + static_cast<std::size_t>(j)])), "moment trajectory at t = " + std::to_string(tn) + ", component " + std::to_string(j));
      }
      // the exponential density integrates to the moments
      const auto grid = pbe::uniform_grid(0.0, 30.0, 6000);
      Series dens = Series(vec(grid.centers));
      dens = cr::exponential_density(dens, ss.n0, ss.growth, tau);
      for (int j = 0; j < 4; ++j)
        expect(std::abs(pbe::moment(dens, grid, j).item<double>() / y[static_cast<std::size_t>(1 + j)] - 1.0) < 1e-3, "density moments match the analytic steady moments, j = " + std::to_string(j));
    }
    // ---- full finite-volume population balance reaches the same crystal size distribution
    {
      const cr::Msmpr p = msmpr();
      const double tau = ref::msmpr[0], g = ref::msmpr[14], n0 = ref::msmpr[14 + 1] / g;
      const auto grid = pbe::uniform_grid(0.0, 16.0 * g * tau, 240);
      const auto rate = [&](const Series &s) { return cr::pbe_rate(s, grid, p); };
      std::vector<double> init(241, 0.0);
      init[0] = 25.0; // concentration; the crystals start from a small exponential seed
      for (std::size_t i = 0; i < 240; ++i) init[1 + i] = 0.2 * std::exp(-grid.centers[i] / 1.0);
      const Series end = pbe::ssprk3(rate, vec(init), 0.0, 36.0 * tau, 2400);
      const Series c = end.select(0, 0), n = end.slice(0, 1, 241, 1);
      expect(std::abs(c.item<double>() - ref::msmpr[9]) < 2e-3, "PBE steady supersaturation: " + std::to_string(c.item<double>()) + " vs " + std::to_string(ref::msmpr[9]));
      for (int j = 0; j < 4; ++j) {
        const double got = pbe::moment(n, grid, j).item<double>(), want = ref::msmpr[10 + static_cast<std::size_t>(j)];
        expect(std::abs(got / want - 1.0) < 0.01, "PBE steady moment mu_" + std::to_string(j) + ": " + std::to_string(got) + " vs " + std::to_string(want));
      }
      // exponential shape: n(L) = n0 exp(-L / (G tau)) at a few sizes
      for (const std::int64_t i : {5, 40, 100}) {
        const double L = grid.centers[static_cast<std::size_t>(i)];
        expect(std::abs(n.select(0, i).item<double>() / (n0 * std::exp(-L / (g * tau))) - 1.0) < 0.02, "steady CSD is exponential at L = " + std::to_string(L));
      }
    }
    std::cout << "crystallization tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
