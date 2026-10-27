#include "pluss/ma/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bio = pluss::subsp::natsci::biology;
namespace dyn = bio::dynamics;
namespace epi = bio::epidemiology;
namespace net = bio::systems::network;
namespace sto = bio::population::stochastic;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::initializer_list<double> v) { return pluss::series::make(v).to(F64); }
Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(-1, i).item<double>(); }
bool close(double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

// vdD&W eq. (6): R0 = beta1 nu / ((d+nu+r1)(d+r2) - nu p r2), DFE (0,0,1,0).
void test_treatment_r0() {
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const double pf = 0.3;
  const double expected = 1.5 * 0.3 / ((0.05 + 0.3 + 0.2) * (0.05 + 0.35) - 0.3 * pf * 0.35);
  const Series dfe = vec({0.0, 0.0, 1.0, 0.0});
  expect(close(epi::reproduction_number(epi::treatment_split(p), dfe).item<double>(), expected, 1e-8), "treatment R0");
  // untreated SEI limit: beta1 nu / (d (d + nu))
  epi::TreatmentParams u = p;
  u.r1 = 0.0; u.r2 = 0.0;
  expect(close(epi::reproduction_number(epi::treatment_split(u), dfe).item<double>(), 1.5 * 0.3 / (0.05 * 0.35), 1e-8), "SEI R0");
  // the DFE is an equilibrium of the full rate map
  expect(epi::treatment_rate(dfe, p).abs().max().item<double>() < 1e-14, "treatment DFE equilibrium");
  // batched lanes: beta1 in [B,1] broadcasts against [B, n]
  epi::TreatmentParams b = p;
  b.beta1 = vec({0.75, 1.5, 3.0}).unsqueeze(-1);
  const Series lanes = dfe.unsqueeze(0).unite(std::vector<Series>{dfe.unsqueeze(0), dfe.unsqueeze(0)}, 0);
  const Series r0 = epi::reproduction_number(epi::treatment_split(b), lanes);
  expect(close(r0.select(0, 0).item<double>(), 0.5 * expected, 1e-8) && close(r0.select(0, 2).item<double>(), 2.0 * expected, 1e-8), "batched R0 scales with beta1");
}

void test_vector_host_and_multistrain() {
  epi::VectorHostParams v;
  v.beta_s = 0.8; v.beta_m = 0.6; v.b = 0.1; v.c = 0.4; v.gamma = 0.2;
  const Series dfe = vec({0.0, 0.0, 1.0, 1.0});
  expect(close(epi::reproduction_number(epi::vector_host_split(v), dfe).item<double>(), std::sqrt(0.8 * 0.6 / (0.4 * 0.3)), 1e-8), "vector-host R0 (eq. 21)");
  expect(epi::vector_host_rate(dfe, v).abs().max().item<double>() < 1e-14, "vector-host DFE");
  epi::MultistrainParams m;
  m.beta1 = 0.9; m.beta2 = 1.4; m.b = 0.2; m.gamma1 = 0.3; m.gamma2 = 0.5; m.nu = 2.0;
  const Series mdfe = vec({0.0, 0.0, 1.0});
  expect(close(epi::reproduction_number(epi::multistrain_split(m), mdfe).item<double>(), std::max(0.9 / 0.5, 1.4 / 0.7), 1e-8), "multistrain R0 = max (eq. 17)");
  expect(epi::multistrain_rate(mdfe, m).abs().max().item<double>() < 1e-14, "multistrain DFE");
}

void test_staged() {
  const Series beta = vec({0.5, 0.3, 0.2}), nu = vec({0.4, 0.3, 0.2}), d = vec({0.1, 0.15, 0.2, 0.3});
  const auto sp = epi::staged_split(beta, nu, d, 0.1);
  const Series dfe = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  expect(close(epi::reproduction_number(sp, dfe).item<double>(), 1.8, 1e-8), "staged R0 (eq. 13)");
  expect(epi::staged_rate(dfe, beta, nu, d, 0.1).abs().max().item<double>() < 1e-14, "staged DFE");
}

void test_sir_final_size() {
  // Hethcote Thm 2.1: root of i0 + s0 - s + ln(s/s0)/sigma, and peak i_max.
  const Series sigma = vec({3.0, 2.0}), s0 = vec({0.999, 0.99}), i0 = vec({0.001, 0.01});
  const Series fs = epi::final_susceptible(sigma, s0, i0);
  expect(close(at(fs, 0), 0.059447768316268204, 1e-12) && close(at(fs, 1), 0.19979603232276646, 1e-12), "final size vs scipy brentq");
  // independent check: integrate the ODE far past the epidemic
  const auto rate = [](const Series &x) { return epi::sir_rate(x, 0.3, 0.1); };
  const Series end = dyn::simulate(rate, vec({0.999, 0.001, 0.0}), 0.0, 400.0, 8000);
  expect(close(at(end, 0), at(fs, 0), 1e-9), "final size vs ODE");
  const auto path = dyn::trajectory(rate, vec({0.999, 0.001, 0.0}), 0.0, 120.0, 960, 6);
  const double peak = path.states.select(-1, 1).max().item<double>();
  expect(std::abs(peak - epi::peak_infective(vec({3.0}), vec({0.999}), vec({0.001})).host<double>().front()) < 1e-4, "peak infective vs ODE");
  // population conserved
  expect(std::abs(end.sum().item<double>() - 1.0) < 1e-12, "SIR conserves N");
  // threshold: sigma*s0 <= 1 -> infection only decays (rate negative from start)
  expect(epi::sir_rate(vec({0.3, 0.01, 0.69}), 0.3, 0.1).select(-1, 1).item<double>() < 0.0, "below threshold i decreases");
}

void test_seir_endemic() {
  const double beta = 1.5, sigma = 0.5, gamma = 0.4, mu = 0.1;
  const double r0 = 1.5 * 0.5 / ((0.5 + 0.1) * (0.4 + 0.1));
  expect(close(epi::seir_r0(vec({beta}), vec({sigma}), vec({gamma}), vec({mu})).host<double>().front(), r0, 1e-14), "SEIR R0 formula");
  // numerical next-generation R0 of the same model agrees with the closed form
  epi::Split sp;
  sp.infected = 2;
  sp.infected_index = {1, 2}; // E and I inside the (S, E, I, R) state
  const auto row4 = [](const Series &a, const Series &b, const Series &c, const Series &d) {
    return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1), c.unsqueeze(-1), d.unsqueeze(-1)}, -1);
  };
  sp.infections = [&](const Series &x) {
    const Series s = x.select(-1, 0), i = x.select(-1, 2);
    const Series f = s.mul(i).div(x.sum(-1, false)).mul(beta);
    const Series z = f.mul(0.0);
    return row4(z, f, z, z); // new infections enter E (index 1)
  };
  sp.transfers = [&](const Series &x) {
    const Series e = x.select(-1, 1), i = x.select(-1, 2);
    const Series z = e.mul(0.0);
    return row4(z, e.mul(sigma + mu), e.mul(-sigma).add(i.mul(gamma + mu)), z);
  };
  expect(close(epi::reproduction_number(sp, vec({1.0, 0.0, 0.0, 0.0})).item<double>(), r0, 1e-8), "SEIR numerical NGM");
  // endemic equilibrium: s* = 1/R0 (Hethcote sec. 3.2)
  const auto rate = [&](const Series &x) { return epi::seir_rate(x, beta, sigma, gamma, mu); };
  const Series x = dyn::simulate(rate, vec({0.9, 0.05, 0.05, 0.0}), 0.0, 600.0, 12000);
  const double expected[4] = {0.4, 0.1, 0.1, 0.4};
  for (int k = 0; k < 4; ++k)
    expect(close(at(x, k), expected[k], 1e-8), "SEIR endemic equilibrium vs scipy");
  expect(std::abs(x.sum().item<double>() - 1.0) < 1e-12, "SEIR conserves N");
}

// Stochastic SIR as a mass-action network: S+I->2I at c = beta/N, I->R at gamma.
void test_stochastic_sir() {
  const double N = 1000.0, beta = 0.2, gamma = 0.1;
  const auto n = net::make(mat({{-1, 1, 0}, {0, -1, 1}}), mat({{1, 1, 0}, {0, 1, 0}}));
  const Series c = vec({beta / N, gamma});
  const auto prop = [&](const Series &x) { return net::propensity(n, x, c); };
  const std::int64_t B = 300;
  sto::SsaOptions opt;
  opt.seed = 20260;
  opt.max_events = 6000;
  const Series x0 = pluss::series::zeros({B, 3}, F64).add(vec({990.0, 10.0, 0.0}));
  const auto r = sto::gillespie(prop, n.stoichiometry, x0, 600.0, opt);
  expect(r.completed, "SSA reached extinction in every replicate");
  expect(r.state.sum(-1, false).sub(N).abs().max().item<double>() == 0.0, "SSA conserves N");
  const Series recovered = r.state.select(-1, 2);
  const Series major = recovered.gt(0.1 * N);
  const double count = major.to(F64).sum().item<double>();
  expect(count > 0.9 * B, "almost all runs are major outbreaks (extinction prob (1/R0)^I0)");
  const double mean_attack = recovered.mul(major.to(F64)).sum().item<double>() / count / N;
  const double fs = epi::final_susceptible(vec({2.0}), vec({0.99}), vec({0.01})).host<double>().front();
  expect(std::abs(mean_attack - (1.0 - fs)) < 0.015, "stochastic attack rate matches deterministic final size");
}

void test_validation() {
  // epidemiologically inconsistent split: V with positive off-diagonal entry
  epi::Split bad;
  bad.infected = 2;
  bad.infections = [](const Series &x) { return x.mul(0.0); };
  bad.transfers = [](const Series &x) { return x.select(-1, 0).unsqueeze(-1).unite(std::vector<Series>{x.select(-1, 0).unsqueeze(-1)}, -1); };
  bool threw = false;
  try { (void)epi::next_generation(bad, vec({0.0, 0.0})); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "V off-diagonal sign check");
  threw = false;
  try { (void)epi::final_susceptible(vec({-1.0}), vec({0.9}), vec({0.1})); } catch (const std::invalid_argument &) { threw = true; }
  expect(threw, "negative contact number rejected");
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
  run("treatment_r0", test_treatment_r0);
  run("vector_host_and_multistrain", test_vector_host_and_multistrain);
  run("staged", test_staged);
  run("sir_final_size", test_sir_final_size);
  run("seir_endemic", test_seir_endemic);
  run("stochastic_sir", test_stochastic_sir);
  run("validation", test_validation);
  std::cout << "NatSci epidemiology checks passed\n";
}
