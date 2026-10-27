// Adsorption isotherms, BET analysis, Langmuir-Hinshelwood optimum and a CO-oxidation microkinetic model against direct summation, regression and ODE/fsolve.
#include "pluss/subsp/natsci/chemistry/surface.h"
#include "pluss/subsp/natsci/common/reaction.h"
#include "surface_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sf = pluss::subsp::natsci::chemistry::surface;
namespace rx = pluss::subsp::natsci::common::reaction;
namespace dyn = pluss::subsp::natsci::common::dynamics;
namespace ref = surface_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> s) { return pluss::make_series(v, s).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
} // namespace

int main() {
  try {
    // ---- BET: closed form vs direct summation of the multilayer model (finite layers), and the infinite-layer limit
    {
      for (std::size_t i = 0; i < ref::bet_direct.size() / 4; ++i) {
        const double c = ref::bet_direct[4 * i], n = ref::bet_direct[4 * i + 1], x = ref::bet_direct[4 * i + 2], want = ref::bet_direct[4 * i + 3];
        expect(std::abs(sf::bet(vec({x}), c, n).host<double>().front() - want) < 1e-12 * (1.0 + want), "BET closed form vs direct sum, c = " + std::to_string(c) + ", n = " + std::to_string(n) + ", x = " + std::to_string(x));
      }
      const Series x = vec({0.05, 0.3, 0.7});
      expect(sf::bet(x, 40.0, 0.0).sub(sf::bet(x, 40.0, 400.0)).abs().max().item<double>() < 1e-12, "n -> infinity form");
      expect(sf::bet(x, 40.0, 1.0).sub(sf::langmuir(x, 40.0)).abs().max().item<double>() < 1e-12, "one layer is Langmuir with K = c");
    }
    // ---- BET plot regression vs numpy polyfit
    {
      const auto f = sf::bet_fit(vec(ref::bet_data_x), vec(ref::bet_data_q));
      expect(std::abs(f.monolayer.item<double>() - ref::bet_fit[0]) < 1e-10 && std::abs(f.c.item<double>() - ref::bet_fit[1]) < 1e-8 && std::abs(f.r_squared.item<double>() - ref::bet_fit[2]) < 1e-12, "BET plot regression");
      expect(std::abs(f.monolayer.item<double>() - 2.4) < 0.1, "the regression recovers the monolayer capacity of the generating model (1% noise)");
      expect(std::abs(sf::specific_surface_area(vec({2.0}), 1.62e-19).host<double>().front() - 2.0 * 1.62e-19 * 6.02214076e23) < 1e-6 * 2.0 * 1.62e-19 * 6.02214076e23, "specific surface area");
    }
    // ---- isotherm identities
    {
      const Series p = vec({0.1, 1.0, 10.0, 100.0});
      expect(std::abs(sf::langmuir(vec({0.0}), 0.5).host<double>().front()) < 1e-15 && std::abs(sf::langmuir(vec({1e12}), 0.5).host<double>().front() - 1.0) < 1e-9, "Langmuir limits");
      const auto root = [](double pk) { const double r = std::sqrt(0.5 * pk); return r / (1.0 + r); };
      const auto dis = sf::langmuir_dissociative(p, 0.5).contiguous().host<double>();
      for (std::size_t i = 0; i < 4; ++i)
        expect(std::abs(dis[i] - root(std::vector<double>{0.1, 1.0, 10.0, 100.0}[i])) < 1e-15, "dissociative Langmuir sqrt(KP) / (1 + sqrt(KP))");
      // competitive Langmuir: coverages sum to < 1, ratios theta_i / theta_j = K_i P_i / K_j P_j, a single species reduces to Langmuir
      const Series pp = vec({2.0, 0.5, 4.0}), kk = vec({0.7, 1.9, 0.2});
      const Series th = sf::langmuir_competitive(pp, kk);
      expect(th.sum().item<double>() < 1.0 && std::abs(th.select(0, 0).item<double>() / th.select(0, 1).item<double>() - (0.7 * 2.0) / (1.9 * 0.5)) < 1e-12, "competitive Langmuir ratios");
      expect(std::abs(sf::langmuir_competitive(vec({3.0}), vec({0.4})).host<double>().front() - sf::langmuir(vec({3.0}), 0.4).host<double>().front()) < 1e-15, "single species reduces to Langmuir");
      // Sips with n = 1 is Langmuir; Freundlich is a power law
      expect(sf::sips(p, 1.0, 0.5, 1.0).sub(sf::langmuir(p, 0.5)).abs().max().item<double>() < 1e-15, "Sips(n = 1) = Langmuir");
      expect(std::abs(sf::freundlich(vec({8.0}), 2.0, 3.0).host<double>().front() - 2.0 * 2.0) < 1e-12, "Freundlich power law");
    }
    // ---- isosteric heat of a Langmuir surface with K = K0 exp(Q / R T) equals Q (Clausius-Clapeyron by finite differences)
    {
      const double q = 42.0e3;
      const auto pressure = [&](const Series &theta, const Series &temperature) { return sf::langmuir_pressure(theta, temperature, 1e-5, q); };
      const Series h = sf::isosteric_heat(pressure, vec({0.2, 0.5, 0.8}), vec({300.0, 350.0, 400.0}));
      expect(h.sub(q).abs().max().item<double>() < 1e-3 * q, "isosteric heat = Q at every coverage");
    }
    // ---- Langmuir-Hinshelwood: the pressure optimum K_A P_A = 1 + K_B P_B and the maximal rate; Eley-Rideal saturates
    {
      const double ka = ref::lh[0], kb = ref::lh[1], pb = ref::lh[2];
      const Series pa = vec({(1.0 + kb * pb) / ka * 0.9, (1.0 + kb * pb) / ka, (1.0 + kb * pb) / ka * 1.1});
      const auto r = sf::langmuir_hinshelwood_rate(pa, vec({pb, pb, pb}), ka, kb, 1.0).contiguous().host<double>();
      expect(r[1] > r[0] && r[1] > r[2], "LH rate peaks at K_A P_A = 1 + K_B P_B");
      expect(std::abs(r[1] - ref::lh[4]) < 1e-9 && std::abs((1.0 + kb * pb) / ka - ref::lh[3]) < 1e-3, "LH maximal rate vs the grid optimum: " + std::to_string(r[1]) + " vs " + std::to_string(ref::lh[4]));
      expect(sf::eley_rideal_rate(vec({1e9}), vec({2.0}), 1.0, 3.0).host<double>().front() - 6.0 < 1e-6, "ER rate saturates at k P_B");
    }
    // ---- CO oxidation on a surface with site conservation: ODE trajectory, Newton steady state and turnover frequency vs scipy
    {
      const double kac = ref::mk_params[0], kdc = ref::mk_params[1], kao = ref::mk_params[2], kdo = ref::mk_params[3], kr = ref::mk_params[4];
      // species [CO*, O*, *]; reactions: CO adsorption, CO desorption, dissociative O2 adsorption, O2 desorption, surface reaction
      rx::Network net = rx::make(mat({1, 0, -1,  -1, 0, 1,  0, 2, -2,  0, -2, 2,  -1, -1, 2}, {5, 3}), mat({0, 0, 1,  1, 0, 0,  0, 0, 2,  0, 2, 0,  1, 1, 0}, {5, 3}));
      const Series k = vec({kac, kdc, kao, kdo, kr});
      const auto rate = [&](const Series &x) { return rx::rate(net, x, k); };
      // the scipy trajectory is stored at t = 0.5, 1, 2, 5; integrate piecewise to compare (RK4 steps of 1 ms)
      Series s = vec({0.0, 0.0, 1.0});
      double t = 0.0;
      for (std::size_t i = 0; i < ref::mk_traj.size() / 3; ++i) {
        const double tn = ref::mk_traj[3 * i];
        s = dyn::simulate(rate, s, t, tn, static_cast<std::int64_t>((tn - t) / 0.001));
        t = tn;
        expect(std::abs(s.select(0, 0).item<double>() - ref::mk_traj[3 * i + 1]) < 1e-9 && std::abs(s.select(0, 1).item<double>() - ref::mk_traj[3 * i + 2]) < 1e-9, "coverage trajectory at t = " + std::to_string(tn));
        expect(std::abs(s.sum().item<double>() - 1.0) < 1e-12, "sites are conserved");
      }
      const Series gamma = mat({1.0, 1.0, 1.0}, {1, 3});
      const auto ss = rx::steady_state(net, k, vec({0.3, 0.3, 0.4}), gamma, vec({1.0}));
      expect(ss.converged, "Newton steady state converged");
      const auto sv = ss.state.contiguous().host<double>();
      expect(std::abs(sv[0] - ref::mk_ss[0]) < 1e-10 && std::abs(sv[1] - ref::mk_ss[1]) < 1e-10 && std::abs(sv[2] - ref::mk_ss[2]) < 1e-10, "steady-state coverages vs scipy: " + std::to_string(sv[0]) + ", " + std::to_string(sv[1]));
      expect(std::abs(kr * sv[0] * sv[1] - ref::mk_ss[3]) < 1e-10, "turnover frequency");
    }
    std::cout << "surface tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
