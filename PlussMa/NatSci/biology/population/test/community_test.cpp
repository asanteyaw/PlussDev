// Random-community stability, Lotka-Volterra invasion, chemostat competition and adaptive dynamics against numpy/sympy/scipy and published criteria.
#include "pluss/subsp/natsci/biology/population/community.h"
#include "community_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace cm = pluss::subsp::natsci::biology::population::community;
namespace ref = community_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
} // namespace

int main() {
  try {
    // ---- spectral abscissa vs numpy eigvals on 30 sparse random matrices (batched in one call)
    {
      const Series m = mat(ref::rand_mats, {30, 9, 9});
      const auto got = cm::spectral_abscissa(m).contiguous().to_vec<double>();
      for (std::size_t i = 0; i < 30; ++i)
        expect(std::abs(got[i] - ref::rand_abscissa[i]) < 1e-6, "spectral abscissa " + std::to_string(i) + ": " + std::to_string(got[i]) + " vs " + std::to_string(ref::rand_abscissa[i]));
    }
    // ---- elliptic law: the rightmost eigenvalue sits near sigma sqrt(S C) (1 + tau) - d for large S (Allesina & Tang, May)
    {
      const std::int64_t s = 200;
      for (const double tau : {0.0, -2.0 / M_PI, 2.0 / M_PI}) {
        const Series a = cm::random_community(s, 6, 0.1, 1.0, tau, 1.0, 41);
        const auto ab = cm::spectral_abscissa(a).contiguous().to_vec<double>();
        double mean = 0.0;
        for (double v : ab)
          mean += v / 6.0;
        const double pred = cm::predicted_abscissa(s, 0.1, 1.0, tau, 1.0);
        const double radius = 0.1 * std::sqrt(static_cast<double>(s));
        expect(std::abs(mean - pred) < 0.08 * radius + 0.02, "elliptic law, tau = " + std::to_string(tau) + ": abscissa " + std::to_string(mean) + " vs predicted " + std::to_string(pred));
      }
      // the stable/unstable threshold: sigma sqrt(S C) = d for tau = 0
      const Series below = cm::random_community(s, 4, 0.8 / std::sqrt(static_cast<double>(s)), 1.0, 0.0, 1.0, 5), above = cm::random_community(s, 4, 1.25 / std::sqrt(static_cast<double>(s)), 1.0, 0.0, 1.0, 6);
      expect(cm::spectral_abscissa(below).lt(0.0).all().item<bool>() && cm::spectral_abscissa(above).gt(0.0).all().item<bool>(), "May criterion: stable below, unstable above sigma sqrt(S C) = d");
      // the constructed matrices have the requested pair correlation
      const Series big = cm::random_community(60, 20, 1.0, 1.0, 0.5, 0.0, 9);
      const Series prod = big.mul(big.transpose(-1, -2));
      const double corr = prod.sum().item<double>() / (20.0 * 60.0 * 59.0);
      expect(std::abs(corr - 0.5) < 0.03, "pair correlation E[M_ij M_ji] = tau sigma^2: " + std::to_string(corr));
    }
    // ---- Lotka-Volterra invasion growth rates
    {
      const Series inv = cm::invasion_growth_rate(vec(ref::glv_r), mat(ref::glv_A, {6, 6}), vec({1, 1, 0, 1, 1, 0}));
      const auto v = inv.contiguous().to_vec<double>();
      for (std::size_t i = 0; i < 6; ++i)
        expect(std::abs(v[i] - ref::glv_invasion[i]) < 1e-12, "invasion growth rate of species " + std::to_string(i));
      expect(std::abs(v[0]) < 1e-12 && std::abs(v[1]) < 1e-12 && std::abs(v[3]) < 1e-12 && std::abs(v[4]) < 1e-12, "residents have zero growth at their equilibrium");
    }
    // ---- chemostat: R* ordering, equilibrium of the best competitor, and the simulated competitive exclusion
    {
      const double d = ref::chem[0], s0 = ref::chem[1];
      const Series mu = mat({ref::chem[2], ref::chem[3], ref::chem[4]}, {3}), kh = mat({ref::chem[5], ref::chem[6], ref::chem[7]}, {3}), mo = mat({ref::chem[8], ref::chem[9], ref::chem[10]}, {3}), q = mat({ref::chem[11], ref::chem[12], ref::chem[13]}, {3});
      const Series rs = cm::r_star(mu, kh, mo);
      const auto rv = rs.contiguous().to_vec<double>();
      for (std::size_t i = 0; i < 3; ++i)
        expect(std::abs(rv[i] - ref::chem_rstar[i]) < 1e-14, "R* of species " + std::to_string(i));
      std::size_t best = 0;
      for (std::size_t i = 1; i < 3; ++i)
        if (rv[i] < rv[best]) best = i;
      const double nstar = cm::single_species_density(d, s0, rs.select(0, static_cast<std::int64_t>(best)), mo.select(0, static_cast<std::int64_t>(best)), q.select(0, static_cast<std::int64_t>(best))).item<double>();
      expect(std::abs(rv[best] - ref::chem_eq[0]) < 1e-9 && std::abs(nstar - ref::chem_eq[1]) < 1e-9, "single-species chemostat equilibrium vs fsolve");
      // simulate all three together from a common start: the lowest R* wins, the others wash out, resource settles at R*
      const auto rate = [&](const Series &y) { return cm::chemostat_rate(y, d, s0, mu, kh, mo, q); };
      const Series y0 = vec({5.0, 1.0, 1.0, 1.0});
      const Series y = pluss::subsp::natsci::common::dynamics::simulate(rate, y0, 0.0, 600.0, 60000);
      const auto yv = y.contiguous().to_vec<double>();
      expect(std::abs(yv[0] - rv[best]) < 1e-6, "resource settles at the best competitor's R*: " + std::to_string(yv[0]) + " vs " + std::to_string(rv[best]));
      for (std::size_t i = 0; i < 3; ++i) {
        if (i == best)
          expect(std::abs(yv[1 + i] - nstar) < 1e-5 * nstar, "winner density N*");
        else
          expect(yv[1 + i] < 1e-8, "losing species is excluded");
      }
    }
    // ---- adaptive dynamics of Gaussian logistic competition: derivatives, classification, singular point and canonical-equation flow
    {
      const auto fitness = [&](double sa, double sk) {
        return cm::Fitness([sa, sk](const Series &y, const Series &x) {
          return cm::logistic_competition_fitness(y, x, [&](const Series &t) { return t.square().mul(-0.5 / (sk * sk)).exp(); },
                                                  [&](const Series &mutant, const Series &resident) { return mutant.sub(resident).square().mul(-0.5 / (sa * sa)).exp(); });
        });
      };
      for (std::size_t c = 0; c < 4; ++c) {
        const double sa = ref::ad[5 * c], sk = ref::ad[5 * c + 1];
        const auto f = fitness(sa, sk);
        const auto d = cm::second_derivatives(f, vec({0.0}));
        expect(std::abs(d.fyy.item<double>() - ref::ad[5 * c + 2]) < 1e-5 * (1.0 + std::abs(ref::ad[5 * c + 2])), "f_yy = 1/sa^2 - 1/sk^2 (sympy), case " + std::to_string(c));
        expect(std::abs(d.fxy.item<double>() - ref::ad[5 * c + 3]) < 1e-5 * (1.0 + std::abs(ref::ad[5 * c + 3])), "f_xy = -1/sa^2 (sympy), case " + std::to_string(c));
        expect(std::abs(cm::selection_gradient(f, vec({0.7})).item<double>() - ref::ad[5 * c + 4]) < 1e-7, "selection gradient at x = 0.7 vs sympy");
        const auto cl = cm::classify(f, vec({0.0}));
        expect(cl.convergence_stable.item<bool>(), "the singular point x* = 0 is always convergence stable");
        if (std::abs(sa - sk) > 1e-3) {
          expect(cl.branching.item<bool>() == (sa < sk), "evolutionary branching iff sigma_alpha < sigma_K, case " + std::to_string(c));
          expect(cl.css.item<bool>() == (sa > sk), "continuously stable strategy iff sigma_alpha > sigma_K, case " + std::to_string(c));
        }
      }
      const auto f = fitness(0.5, 1.0);
      const double xs = cm::singular_strategy(f, vec({0.4})).item<double>();
      expect(std::abs(xs) < 1e-7, "Newton finds the singular strategy x* = 0: " + std::to_string(xs));
      // canonical equation dx/dt = df/dy |_{y = x} = -x / sigma_K^2 (arXiv:1005.3862 eq. 10) integrated by RK4 from the generic pipeline
      const double sk = 1.3;
      const auto g = fitness(0.5, sk);
      const auto rate = [&](const Series &x) { return cm::selection_gradient(g, x); };
      const Series xt = pluss::subsp::natsci::common::dynamics::simulate(rate, vec({1.0, -0.6}), 0.0, 2.0, 400);
      const auto xv = xt.contiguous().to_vec<double>();
      const double decay = std::exp(-2.0 / (sk * sk));
      expect(std::abs(xv[0] - decay) < 1e-6 && std::abs(xv[1] + 0.6 * decay) < 1e-6, "canonical equation flow x(t) = x0 exp(-t / sigma_K^2): " + std::to_string(xv[0]) + " vs " + std::to_string(decay));
    }
    std::cout << "community tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
