#include "pluss/ma/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace gen = bio::genetics;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

void test_deterministic() {
  // Hardy-Weinberg and fitness algebra
  const Series hw = gen::hardy_weinberg(vec({0.3}).squeeze(0));
  expect(close(at(hw, 0), 0.09, 1e-14) && close(at(hw, 1), 0.42, 1e-14) && close(at(hw, 2), 0.49, 1e-14), "HWE");
  // neutral selection leaves p unchanged
  expect(close(gen::select(vec({0.3}), gen::Fitness{}).host<double>().front(), 0.3, 1e-14), "neutral select");
  // heterozygote advantage converges to t/(s+t) (here 0.6)
  gen::Fitness w;
  w.aa = 0.8; w.ab = 1.0; w.bb = 0.7;
  Series p = vec({0.1, 0.9});
  for (int g = 0; g < 400; ++g)
    p = gen::expected_next(p, w);
  expect(close(at(p, 0), 0.6, 1e-10) && close(at(p, 1), 0.6, 1e-10), "overdominance equilibrium");
  expect(close(gen::heterozygote_advantage_equilibrium(vec({0.2}), vec({0.3})).host<double>().front(), 0.6, 1e-14), "equilibrium formula");
  // mutation-selection balance, recessive deleterious allele a (aa fitness 1-s),
  // mutation A -> a at rate mu: equilibrium q* ~ sqrt(mu/s) for small mu.
  gen::Fitness rec;
  rec.bb = 0.9;
  Series freq = vec({0.99}); // frequency of A
  for (int g = 0; g < 6000; ++g)
    freq = gen::expected_next(freq, rec, 1e-4, 0.0);
  const double q_star = 1.0 - freq.host<double>().front();
  expect(std::abs(q_star / std::sqrt(1e-4 / 0.1) - 1.0) < 0.05, "mutation-selection balance q ~ sqrt(mu/s)");
}

void test_binomial() {
  const Series p = vec({0.0, 0.2, 0.5, 1.0});
  const Series pmf = gen::binomial_pmf(10, p);
  expect(pmf.sum(-1, false).sub(1.0).abs().max().item<double>() < 1e-12, "pmf normalised");
  expect(close(pmf.select(0, 1).select(-1, 3).item<double>(), 120.0 * std::pow(0.2, 3) * std::pow(0.8, 7), 1e-12), "pmf value");
  expect(pmf.select(0, 0).select(-1, 0).item<double>() == 1.0 && pmf.select(0, 3).select(-1, 10).item<double>() == 1.0, "exact absorbing rows");
  // sampler moments over many lanes
  const std::int64_t B = 40000;
  const Series pl = pluss::series::zeros({B}, F64).add(0.3);
  const Series k = gen::sample_binomial(20, pl, 4242);
  expect(std::abs(k.mean().item<double>() - 6.0) < 4.0 * std::sqrt(20 * 0.3 * 0.7 / B), "binomial mean");
  expect(std::abs(k.sub(k.mean()).square().mean().item<double>() - 4.2) < 0.1, "binomial variance");
}

void test_exact_chain() {
  const std::int64_t n_pop = 40;
  const double s = 0.01;
  const Series p = gen::transition_matrix(n_pop, gen::additive(s));
  const auto a = gen::absorption(p);
  expect(close(a.fixation.select(-1, 1).item<double>(), 0.024707161706865002, 1e-10), "exact fixation of a new mutant");
  expect(close(a.fixation.select(-1, 10).item<double>(), 0.2262549711602779, 1e-10), "exact fixation p=0.125");
  expect(close(a.fixation.select(-1, 40).item<double>(), 0.6883518818842888, 1e-10), "exact fixation p=0.5");
  expect(close(a.time.select(-1, 10).item<double>(), 66.10829290389516, 1e-9), "exact absorption time");
  // Kimura diffusion formula is within 0.5% of the exact chain here
  const Series freq = vec({1.0 / 80.0, 0.5});
  const Series kim = gen::kimura_fixation(freq, static_cast<double>(n_pop), s);
  expect(close(at(kim, 0), 0.024810475257533264, 1e-12) && close(at(kim, 1), 0.6899744811276124, 1e-12), "Kimura formula value");
  expect(std::abs(at(kim, 0) / 0.024707161706865002 - 1.0) < 0.005, "Kimura vs exact chain (0.5%)");
  // neutral: fixation probability is exactly p0; time matches the diffusion value to 2%
  const auto neutral = gen::absorption(gen::transition_matrix(n_pop, gen::Fitness{}));
  expect(close(neutral.fixation.select(-1, 40).item<double>(), 0.5, 1e-10) && close(neutral.fixation.select(-1, 20).item<double>(), 0.25, 1e-10), "neutral fixation = p0");
  expect(close(neutral.time.select(-1, 40).item<double>(), 108.9504121940426, 1e-9), "neutral time exact");
  expect(std::abs(neutral.time.select(-1, 40).item<double>() / (4.0 * n_pop * std::log(2.0)) - 1.0) < 0.02, "neutral time ~ diffusion");
  expect(close(gen::kimura_fixation(vec({0.3}), 50.0, 0.0).host<double>().front(), 0.3, 1e-12), "Kimura s->0");
}

void test_moran() {
  const double r = 1.3;
  const std::int64_t n = 20;
  const auto a = gen::absorption(gen::moran_transition(n, r));
  expect(close(a.fixation.select(-1, 1).item<double>(), 0.23198991139849542, 1e-12), "Moran exact chain");
  expect(close(a.time.select(-1, 1).item<double>(), 93.08162906668534, 1e-9), "Moran absorption time");
  const Series start = pluss::series::arange(0, n + 1, 1, F64);
  expect(a.fixation.sub(gen::moran_fixation(r, n, start)).abs().max().item<double>() < 1e-12, "chain equals closed form for all starts");
  expect(close(gen::moran_fixation(r, n, vec({1.0})).host<double>().front(), (1 - 1 / r) / (1 - std::pow(r, -double(n))), 1e-13), "closed form (1-1/r)/(1-r^-N)");
  expect(close(gen::moran_fixation(1.0, n, vec({5.0})).host<double>().front(), 0.25, 1e-14), "neutral Moran i/N");
}

void test_simulation_vs_exact() {
  const std::int64_t n_pop = 15, B = 6000;
  const double s = 0.03;
  const auto exact = gen::absorption(gen::transition_matrix(n_pop, gen::additive(s)));
  const double u1 = exact.fixation.select(-1, 3).item<double>(); // 3 copies of 30
  const Series p0 = pluss::series::zeros({B}, F64).add(3.0 / 30.0);
  const auto run = gen::simulate(p0, n_pop, gen::additive(s), 3000, 777);
  expect(run.absorbed, "all replicates absorbed");
  const double fixed = run.frequency.ge(1.0).to(F64).mean().item<double>();
  expect(std::abs(fixed - u1) < 4.0 * std::sqrt(u1 * (1 - u1) / B), "simulated fixation probability matches exact chain");
  expect(run.frequency.min().item<double>() >= 0.0 && run.frequency.max().item<double>() <= 1.0, "frequencies stay in [0,1]");
  // batched selection coefficients in one call: stronger selection fixes more often
  const Series svec = vec({0.0, 0.1}).unsqueeze(-1);
  const Series pl = pluss::series::zeros({2, 3000}, F64).add(0.1);
  const auto r2 = gen::simulate(pl, n_pop, gen::additive(svec), 3000, 99);
  const Series fr = r2.frequency.ge(1.0).to(F64).mean(-1, false);
  expect(at(fr, 1) > at(fr, 0) + 0.15, "selection raises fixation probability (batched lanes)");
  expect(std::abs(at(fr, 0) - 0.1) < 0.02, "neutral lane fixes with probability p0");
  // reproducible
  const auto again = gen::simulate(p0, n_pop, gen::additive(s), 3000, 777);
  expect(run.frequency.sub(again.frequency).abs().max().item<double>() == 0.0, "seeded runs reproduce");
  // mutation keeps polymorphism: with mu = nu the long-run mean frequency is 0.5
  const auto mut = gen::simulate(pluss::series::zeros({2000}, F64).add(0.9), 50, gen::Fitness{}, 400, 5, 0.02, 0.02);
  expect(std::abs(mut.frequency.mean().item<double>() - 0.5) < 0.03, "mutation-drift stationarity");
}

void test_validation() {
  bool threw = false;
  try { (void)gen::expected_next(vec({1.5}), gen::Fitness{}); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "frequency > 1 rejected");
  threw = false;
  try { (void)gen::moran_transition(1, 1.1); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "Moran N < 2 rejected");
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
  run("deterministic", test_deterministic);
  run("binomial", test_binomial);
  run("exact_chain", test_exact_chain);
  run("moran", test_moran);
  run("simulation_vs_exact", test_simulation_vs_exact);
  run("validation", test_validation);
  std::cout << "NatSci genetics checks passed\n";
}
