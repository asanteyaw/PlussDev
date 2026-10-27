#include "pluss/ma/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace growth = bio::population::growth;
namespace inter = bio::population::interaction;
namespace sto = bio::population::stochastic;
namespace sel = bio::population::selection;
namespace stru = bio::population::structured;
using pluss::Series;

namespace {
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
const auto F64 = pluss::ScalarType::Float64;

Series vec(std::initializer_list<double> v) {
  return pluss::series::make(v).to(F64);
}
Series mat(std::initializer_list<std::initializer_list<double>> m) {
  return pluss::series::make(m).to(F64);
}
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

void test_growth_closed_forms() {
  const Series n0 = vec({1.5});
  expect(close(growth::logistic(n0, 0.7, 10.0, 2.0).host<double>().front() , 4.171215384570667, 1e-12), "logistic closed form");
  expect(close(growth::gompertz(n0, 0.7, 10.0, 2.0).host<double>().front(), 6.263637361227007, 1e-12), "gompertz closed form");
  // numerical RK4 against closed form
  auto rate = [](const Series &n) { return growth::logistic_rate(n, 0.7, 10.0); };
  expect(close(dyn::simulate(rate, n0, 0, 2, 200).host<double>().front(), 4.171215384570667, 1e-9), "logistic RK4");
  auto grate = [](const Series &n) { return growth::gompertz_rate(n, 0.7, 10.0); };
  expect(close(dyn::simulate(grate, n0, 0, 2, 200).host<double>().front(), 6.263637361227007, 1e-9), "gompertz RK4");
  // Allee equilibria: rate zero at 0, A, K; sign pattern
  const Series pts = vec({0.0, 2.0, 10.0, 1.0, 5.0});
  const Series r = growth::strong_allee_rate(pts, 1.0, 2.0, 10.0);
  expect(std::abs(at(r, 0)) < 1e-14 && std::abs(at(r, 1)) < 1e-14 && std::abs(at(r, 2)) < 1e-14, "allee zeros");
  expect(at(r, 3) < 0.0 && at(r, 4) > 0.0, "allee signs");
  // theta=1 reduces to logistic
  expect(close(growth::theta_logistic_rate(n0, 0.7, 10.0, 1.0).host<double>().front(),
               growth::logistic_rate(n0, 0.7, 10.0).host<double>().front(), 1e-14), "theta=1");
  // batch / broadcast: Series r per lane
  const Series lanes = vec({1.5, 1.5, 1.5});
  const Series rr = vec({0.1, 0.7, 1.0});
  const Series out = growth::logistic(lanes, rr, 10.0, 2.0);
  expect(close(at(out, 1), 4.171215384570667, 1e-12), "batched logistic lane");
}

void test_predator_prey() {
  const auto rate = [](const Series &s) { return inter::predator_prey_rate(s, 1.0, 0.5, 0.75, 0.25); };
  const Series y0 = vec({2.0, 1.5});
  const Series y = dyn::simulate(rate, y0, 0, 5, 2000);
  expect(close(at(y, 0), 2.412445716524, 1e-8) && close(at(y, 1), 2.878914044763, 1e-8), "LV vs scipy DOP853");
  const Series v0 = inter::predator_prey_invariant(y0, 1.0, 0.5, 0.75, 0.25);
  const Series v1 = inter::predator_prey_invariant(y, 1.0, 0.5, 0.75, 0.25);
  expect(close(v0.item<double>(), 0.324674506472, 1e-10), "invariant value");
  expect(std::abs(v1.item<double>() - v0.item<double>()) < 1e-9, "invariant conserved");
  const Series eq = inter::predator_prey_equilibrium(1.0, 0.5, 0.75, 0.25, y0.select(-1, 0));
  expect(close(at(eq, 0), 3.0, 1e-14) && close(at(eq, 1), 2.0, 1e-14), "LV equilibrium");
  expect(rate(eq).abs().max().item<double>() < 1e-14, "equilibrium rate zero");
  // Holling
  const Series prey = vec({2.0});
  expect(close(inter::holling2(prey, 0.5, 0.4).host<double>().front(), 1.0 / 1.4, 1e-14), "holling2");
  expect(close(inter::holling3(prey, 0.5, 0.4, 2.0).host<double>().front(), 2.0 / 1.8, 1e-14), "holling3");
}

void test_glv() {
  const Series A = mat({{-1.0, -0.3}, {-0.2, -0.8}});
  const Series r = vec({1.0, 0.7});
  const auto rate = [&](const Series &n) { return inter::glv_rate(n, r, A); };
  const Series n = dyn::simulate(rate, vec({0.4, 0.3}), 0, 4, 400);
  expect(close(at(n, 0), 0.80524901, 1e-7) && close(at(n, 1), 0.60702438, 1e-7), "gLV vs scipy");
  const Series eq = inter::glv_equilibrium(r, A);
  expect(close(at(eq, 0), 0.7972973, 1e-7) && close(at(eq, 1), 0.67567568, 1e-7), "gLV equilibrium");
  expect(inter::is_feasible(eq).item<bool>(), "feasible");
  expect(rate(eq).abs().max().item<double>() < 1e-12, "gLV equilibrium rate zero");
}

void test_replicator() {
  const Series P = mat({{0, -1, 2}, {2, 0, -1}, {-1, 1, 0}});
  const auto rate = [&](const Series &x) { return sel::replicator_rate(x, P); };
  const Series x = dyn::simulate(rate, vec({0.5, 0.3, 0.2}), 0, 3, 600);
  expect(close(at(x, 0), 0.15186787, 1e-7) && close(at(x, 1), 0.63908685, 1e-7) &&
             close(at(x, 2), 0.20904527, 1e-7), "replicator vs scipy");
  expect(std::abs(x.sum(-1, false).item<double>() - 1.0) < 1e-12, "simplex preserved");
}

void test_leslie() {
  const Series f = vec({0.0, 1.5, 2.0, 0.5});
  const Series s = vec({0.6, 0.8, 0.5});
  const Series m = stru::leslie(f, s);
  const auto e = stru::dominant(m);
  expect(e.converged, "power iteration converged");
  expect(close(e.lambda.item<double>(), 1.3059353858057234, 1e-10), "lambda");
  const double v[4] = {0.54093651, 0.24852831, 0.15224539, 0.05828979};
  const double u[4] = {0.73547437, 1.60080334, 1.23416772, 0.28158911};
  for (int i = 0; i < 4; ++i) {
    expect(close(at(e.right, i), v[i], 1e-7), "stable distribution");
    expect(close(at(e.left, i), u[i], 1e-7), "reproductive value");
  }
  const Series el = stru::elasticity(m, e);
  expect(close(el.select(-2, 0).select(-1, 1).item<double>(), 0.20994859, 1e-7), "elasticity F2");
  expect(close(el.select(-2, 1).select(-1, 0).item<double>(), 0.39784494, 1e-7), "elasticity S1");
  expect(close(el.sum(-1, false).sum(-1, false).item<double>(), 1.0, 1e-10), "elasticities sum to 1");
  const Series r0 = stru::net_reproductive_rate(f, s);
  expect(close(r0.item<double>(), 1.98, 1e-14), "R0");
  expect(close(stru::generation_time(r0, e.lambda).item<double>(), 2.559186213791574, 1e-10), "T");
  // projection converges to lambda growth
  const Series n10 = stru::project(m, vec({10, 5, 3, 1}), 60);
  const Series n11 = stru::project(m, n10, 1);
  expect(close(n11.sum(-1, false).item<double>() / n10.sum(-1, false).item<double>(), 1.3059353858057234, 1e-8), "asymptotic growth");
  // batched lanes
  const Series mb = m.unsqueeze(0).unite(std::vector<Series>{m.mul(0.5).unsqueeze(0)}, 0);
  const auto eb = stru::dominant(mb);
  expect(eb.lambda.size(0) == 2 && close(eb.lambda.select(0, 1).item<double>(), 0.5 * 1.3059353858057234, 1e-9), "batched eigen");
}

void test_validation() {
  bool threw = false;
  try { (void)growth::logistic_rate(vec({1.0}), 1.0, -1.0); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "negative K rejected");
  threw = false;
  try { (void)stru::leslie(vec({1.0, 1.0}), vec({1.5})); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "survival > 1 rejected");
}

// Immigration-death: 0 -> A at rate lam, A -> 0 at rate mu*A.
// Exact transient law: A(t) ~ Poisson(lam/mu (1 - e^{-mu t})) from A(0)=0;
// mean = variance = that rate. Death only: Binomial(A0, e^{-mu t}).
void test_ssa() {
  const double lam = 5.0, mu = 0.5, t_end = 3.0;
  const std::int64_t B = 40000;
  const Series nu = mat({{1.0}, {-1.0}});
  const auto prop = [&](const Series &x) {
    return x.mul(0.0).add(lam).unite(std::vector<Series>{x.mul(mu)}, -1);
  };
  sto::SsaOptions opt;
  opt.seed = 12345;
  const auto r = sto::gillespie(prop, nu, pluss::series::zeros({B, 1}, F64), t_end, opt);
  expect(r.completed, "ssa completed");
  const double m = lam / mu * (1.0 - std::exp(-mu * t_end));
  const double mean = r.state.mean().item<double>();
  const double var = r.state.sub(mean).square().mean().item<double>();
  expect(std::abs(mean - m) < 4.0 * std::sqrt(m / B), "ssa mean (Poisson)");
  expect(std::abs(var - m) < 6.0 * m * std::sqrt(2.0 / B), "ssa variance (Poisson)");
  // deterministic with a fixed seed
  const auto r2 = sto::gillespie(prop, nu, pluss::series::zeros({B, 1}, F64), t_end, opt);
  expect(r.state.sub(r2.state).abs().max().item<double>() == 0.0, "ssa reproducible");
  // extinction: pure death from 10 -> binomial mean 10 e^{-mu t}
  const Series nud = mat({{-1.0}});
  const auto pd = [&](const Series &x) { return x.mul(mu); };
  const auto d = sto::gillespie(pd, nud, pluss::series::zeros({B, 1}, F64).add(10.0), t_end, opt);
  const double p = std::exp(-mu * t_end);
  expect(std::abs(d.state.mean().item<double>() - 10.0 * p) < 4.0 * std::sqrt(10.0 * p * (1 - p) / B), "ssa death mean");
  expect(d.state.min().item<double>() >= 0.0, "ssa non-negative");
}
} // namespace

int main() {
  test_growth_closed_forms();
  test_predator_prey();
  test_glv();
  test_replicator();
  test_leslie();
  test_validation();
  test_ssa();
  std::cout << "NatSci population checks passed\n";
}
