#include "pluss/ma/natsci/biology.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/stability.h"
#include "../../../common/test/stability_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace gene = bio::systems::gene_networks;
namespace inter = bio::population::interaction;
namespace stab = pluss::subsp::natsci::common::stability;
namespace roots = pluss::subsp::math::roots;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
Series arr(const double *p, std::int64_t n) { return pluss::make_series(std::vector<double>(p, p + n), {n}).to(F64); }

// Repressilator: 224 parameter lanes at once. For each lane the symmetric steady state p* solves
// p = alpha/(1+p^n) + alpha0; the number of unstable modes of the 6x6 Jacobian must equal the number
// of positive-real-part eigenvalues from numpy (the paper's printed criterion was not recoverable,
// so the Jacobian itself is the authority).
void test_repressilator_grid() {
  const std::int64_t lanes = 224;
  const Series params = pluss::make_series(std::vector<double>(stability_reference::rep_params, stability_reference::rep_params + lanes * 4), {lanes, 4}).to(F64);
  const Series alpha = params.select(-1, 0), alpha0 = params.select(-1, 1), beta = params.select(-1, 2), n = params.select(-1, 3);
  const Series pstar = roots::illinois([&](const Series &p) { return p.sub(alpha.div(p.pow(n).add(1.0))).sub(alpha0); },
                                       alpha.mul(0.0).add(1e-12), alpha.mul(0.0).add(1e4), {400, 0.0, 1e-13, 8}).root;
  const Series state = pstar.unsqueeze(-1).mul(pluss::series::ones({6}, F64)); // m = p = p*
  // coefficients carry a trailing singleton axis so they broadcast against the [lanes, perturbation] state axes
  const Series ca = alpha.unsqueeze(-1), ca0 = alpha0.unsqueeze(-1), cb = beta.unsqueeze(-1), cn = n.unsqueeze(-1);
  const auto rate = [&](const Series &s) { return gene::repressilator_rate(s, ca, ca0, cb, cn); };
  const auto r = stab::local_stability(rate, state);
  std::int64_t checked = 0, wrong = 0, unstable = 0;
  for (std::int64_t i = 0; i < lanes; ++i) {
    if (stability_reference::rep_margin[i] < 1e-5)
      continue;
    ++checked;
    const auto got = std::llround(r.unstable.select(0, i).item<double>());
    wrong += (got != std::llround(stability_reference::rep_count[i]));
    unstable += (got > 0);
  }
  expect(checked > 200 && wrong == 0, "repressilator stability (Routh-Hurwitz) vs numpy eigenvalues: " + std::to_string(wrong) + " of " + std::to_string(checked) + " lanes differ");
  expect(unstable > 100 && unstable < checked, "grid spans both stable and unstable regimes");
  // the paper's parameter set (alpha=216, alpha0/alpha=1e-3, beta=0.2, n=2) oscillates: 2 unstable modes (a Hopf pair)
  const Series one = vec({216.0}), a0 = vec({0.216}), be = vec({0.2}), nn = vec({2.0});
  const Series ps = roots::illinois([&](const Series &p) { return p.sub(one.div(p.pow(nn).add(1.0))).sub(a0); }, one.mul(0.0).add(1e-12), one.mul(0.0).add(1e4), {400, 0.0, 1e-13, 8}).root;
  const auto paper = stab::local_stability([&](const Series &s) { return gene::repressilator_rate(s, 216.0, 0.216, 0.2, 2.0); }, ps.unsqueeze(-1).mul(pluss::series::ones({6}, F64)));
  expect(paper.unstable.host<double>().front() >= 2.0, "paper parameters: unstable steady state (Hopf pair)");
}

// Rosenzweig-MacArthur paradox of enrichment: the coexistence point loses stability at
// K_H = 2 x* + 1/(a h) with x* = m/(a (e - m h)). Locate it by lane-wise bisection on the unstable-mode count.
void test_rosenzweig_macarthur_hopf() {
  const double r = 1.0, a = 1.0, h = 0.5, e = 0.6, m = 0.3;
  const double xs = m / (a * (e - m * h));
  const auto unstable_at = [&](const Series &k) { // k: [lanes]
    const Series ys = k.reciprocal().mul(-xs).add(1.0).mul((1.0 + a * h * xs) * r / a); // r (1 - x*/K)(1 + a h x*)/a
    const Series state = k.mul(0.0).add(xs).unsqueeze(-1).unite(std::vector<Series>{ys.unsqueeze(-1)}, -1);
    const Series kk = k.unsqueeze(-1);
    return stab::local_stability([&](const Series &s) { return inter::rosenzweig_macarthur_rate(s, r, kk, a, h, e, m); }, state).unstable;
  };
  const Series lo = vec({2.0, 2.5}), hi = vec({6.0, 5.0});
  const Series kh = roots::bisect([&](const Series &k) { return unstable_at(k).neg().add(0.5); }, lo, hi, {60}).root;
  expect(std::abs(kh.select(0, 0).item<double>() - (2 * xs + 1.0 / (a * h))) < 1e-6, "Hopf threshold of the Rosenzweig-MacArthur model equals 2x* + 1/(ah)");
  expect(std::abs(kh.select(0, 1).item<double>() - kh.select(0, 0).item<double>()) < 1e-6, "second lane with a different bracket finds the same threshold");
  expect(std::abs(kh.select(0, 0).item<double>() - stability_reference::rm_KH_numpy[0]) < 1e-6, "Hopf threshold equals the numpy eigenvalue crossing");
  // below / above the threshold
  const Series ks = vec({3.0, 3.6});
  expect(unstable_at(ks).select(0, 0).item<double>() == 0.0 && unstable_at(ks).select(0, 1).item<double>() == 2.0, "stable below K_H, unstable pair above");
}

void test_lotka_volterra_and_competition() {
  // classical predator-prey: the interior equilibrium is a centre (purely imaginary pair)
  const auto lv = [](const Series &s) { return inter::predator_prey_rate(s, 1.0, 0.5, 0.75, 0.25); };
  const auto c = stab::local_stability(lv, vec({3.0, 2.0}));
  expect(c.marginal.item<double>() == 1.0 && c.unstable.item<double>() == 0.0, "Lotka-Volterra centre is marginal");
  // two-species competition at the coexistence point: stable iff alpha12 alpha21 < 1
  const auto coexist = [](double a12, double a21) {
    const Series alpha = pluss::series::make({{1.0, a12}, {a21, 1.0}}).to(F64);
    const Series k = vec({1.0, 1.0}), rr = vec({1.0, 1.0});
    const Series n_star = dyn::solve(alpha, k); // alpha n = K for K = 1
    const auto rate = [&](const Series &s) { return inter::competitive_rate(s, rr, k, alpha); };
    return stab::local_stability(rate, n_star);
  };
  expect(coexist(0.5, 0.6).unstable.item<double>() == 0.0, "weak competition: stable coexistence");
  expect(coexist(1.5, 1.2).unstable.item<double>() == 1.0, "strong competition: coexistence is a saddle");
}
} // namespace

int main() {
  const auto run = [](const char *name, void (*fn)()) {
    try {
      fn();
    } catch (const std::exception &e) {
      std::cerr << name << ": " << e.what() << "\n";
      std::exit(1);
    }
  };
  run("repressilator_grid", test_repressilator_grid);
  run("rosenzweig_macarthur_hopf", test_rosenzweig_macarthur_hopf);
  run("lotka_volterra_and_competition", test_lotka_volterra_and_competition);
  std::cout << "NatSci stability model checks passed\n";
}
