// American and Bermudan exercise on the Field: a binomial lattice (Black-Scholes) and Longstaff-Schwartz regression (every model).
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
namespace fin = pluss::subsp::math::finance;
namespace mod = pluss::subsp::math::finance::module;
using fin::pricing::ExerciseStyle;
using fin::pricing::OptionType;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Config f64_cpu() { return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU); }

// An independent plain-loop CRR tree (no Series): the oracle for the vectorised lattice.
double reference_tree(bool call, bool american, double s0, double k, double r, double q, double t, double sigma, int n,
                      const std::vector<int>& bermudan_steps = {}) {
  const double dt = t / n, u = std::exp(sigma * std::sqrt(dt)), d = 1.0 / u;
  const double p = (std::exp((r - q) * dt) - d) / (u - d), disc = std::exp(-r * dt);
  auto pay = [&](double s) { return call ? std::max(s - k, 0.0) : std::max(k - s, 0.0); };
  std::vector<double> v(static_cast<std::size_t>(n) + 1);
  for (int i = 0; i <= n; ++i) v[static_cast<std::size_t>(i)] = pay(s0 * std::pow(u, 2 * i - n));
  for (int j = n - 1; j >= 0; --j) {
    const bool ok = american || std::find(bermudan_steps.begin(), bermudan_steps.end(), j) != bermudan_steps.end();
    for (int i = 0; i <= j; ++i) {
      double x = disc * (p * v[static_cast<std::size_t>(i) + 1] + (1.0 - p) * v[static_cast<std::size_t>(i)]);
      if (ok) x = std::max(x, pay(s0 * std::pow(u, 2 * i - j)));
      v[static_cast<std::size_t>(i)] = x;
    }
  }
  return v[0];
}

double bs_price(bool call, double s, double k, double r, double q, double t, double sigma) {
  const auto cdf = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
  const double d1 = (std::log(s / k) + (r - q + 0.5 * sigma * sigma) * t) / (sigma * std::sqrt(t)), d2 = d1 - sigma * std::sqrt(t);
  return call ? s * std::exp(-q * t) * cdf(d1) - k * std::exp(-r * t) * cdf(d2) : k * std::exp(-r * t) * cdf(-d2) - s * std::exp(-q * t) * cdf(-d1);
}

mod::Field bs_field(fin::instrument::Vanilla inst, double spot, double r, double q, double sigma, mod::Engine engine, mod::Risk risk = mod::Risk::PriceOnly) {
  const Config cfg = f64_cpu();
  mod::Field f;
  f.instrument = std::move(inst);
  f.market = fin::market::make(spot, r, q, cfg);
  f.model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, sigma, cfg)};
  f.controls.engine = engine;
  f.controls.risk = risk;
  return f;
}

double px(const mod::Field& f) { return f.price(f64_cpu()).price.items<double>()[0]; }

void test_lattice_matches_independent_tree() {
  const Config cfg = f64_cpu();
  for (const bool call : {false, true}) {
    for (const double q : {0.0, 0.06}) {
      auto f = bs_field(fin::instrument::american(call ? OptionType::Call : OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, q, 0.2, mod::Engine::Lattice);
      f.controls.lattice.steps = 400;
      const double want = reference_tree(call, true, 100.0, 100.0, 0.05, q, 1.0, 0.2, 400);
      expect(std::abs(px(f) - want) < 1e-10, "lattice american must equal the independent tree");
    }
  }
}

void test_known_values() {
  const Config cfg = f64_cpu();
  auto put = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
  put.controls.lattice.steps = 2000;
  expect(std::abs(px(put) - 6.0903) < 5e-3, "american put 100/100/1y 5% 20%: about 6.090");
  auto euro = bs_field(fin::instrument::make(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
  euro.controls.lattice.steps = 2000;
  expect(std::abs(px(euro) - bs_price(false, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2)) < 5e-3, "european lattice converges to the closed form");
  expect(px(put) > px(euro) + 0.3, "early exercise premium of the put is material at 5% rates");
}

void test_call_without_dividends_is_never_exercised_early() {
  const Config cfg = f64_cpu();
  auto a = bs_field(fin::instrument::american(OptionType::Call, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.25, mod::Engine::Lattice);
  auto e = bs_field(fin::instrument::make(OptionType::Call, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.25, mod::Engine::Lattice);
  expect(std::abs(px(a) - px(e)) < 1e-10, "american call without dividends equals the european call");
  a.market = fin::market::make(100.0, 0.05, 0.08, cfg);
  e.market = a.market;
  expect(px(a) > px(e) + 0.05, "with a high dividend yield the american call is worth more");
}

void test_european_is_bermudan_is_american() {
  const Config cfg = f64_cpu();
  constexpr int n = 120;
  auto base = [&](fin::instrument::Vanilla inst) {
    auto f = bs_field(std::move(inst), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
    f.controls.lattice.steps = n;
    return px(f);
  };
  const double european = base(fin::instrument::make(OptionType::Put, 100.0, 1.0, cfg));
  const double american = base(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg));
  std::vector<double> quarterly{0.25, 0.5, 0.75}, monthly, every;
  for (int m = 1; m < 12; ++m) monthly.push_back(m / 12.0);
  for (int j = 1; j < n; ++j) every.push_back(j / static_cast<double>(n));
  const double b4 = base(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, quarterly, cfg));
  const double b12 = base(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, monthly, cfg));
  const double all = base(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, every, cfg));
  expect(european < b4 && b4 < b12 && b12 < american, "european < quarterly < monthly < american");
  expect(std::abs(all - american) < 1e-12, "a bermudan exercisable on every step is the american");
  expect(std::abs(b4 - reference_tree(false, false, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2, n, {30, 60, 90})) < 1e-10, "bermudan matches the independent tree");
}

void test_bermudan_dates_beyond_maturity_and_at_maturity() {
  const Config cfg = f64_cpu();
  std::vector<double> dates{0.5, 1.0, 2.0};  // 1.0 is the maturity itself, 2.0 falls outside this option's life
  auto a = bs_field(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, dates, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
  std::vector<double> just{0.5};
  auto b = bs_field(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, just, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
  expect(std::abs(px(a) - px(b)) < 1e-12, "dates at or beyond the maturity add nothing");
}

void test_deep_in_the_money_put_exercises_immediately() {
  const Config cfg = f64_cpu();
  auto f = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 50.0, 0.05, 0.0, 0.2, mod::Engine::Lattice, mod::Risk::All);
  const auto out = f.price(cfg);
  expect(std::abs(out.price.items<double>()[0] - 50.0) < 1e-9, "deep in-the-money american put is worth its intrinsic value");
  expect(std::abs(out.greeks.delta.items<double>()[0] + 1.0) < 1e-6, "its delta is -1");
  expect(std::abs(out.greeks.gamma.items<double>()[0]) < 1e-3, "its gamma is 0");
}

void test_lattice_greeks_are_consistent() {
  const Config cfg = f64_cpu();
  auto f = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice, mod::Risk::All);
  f.controls.lattice.steps = 600;
  const auto g = f.price(cfg).greeks;
  const double delta = g.delta.items<double>()[0], gamma = g.gamma.items<double>()[0], vega = g.vega.items<double>()[0], rho = g.rho.items<double>()[0];
  expect(delta < -0.3 && delta > -0.6, "american put delta near -0.41");
  expect(gamma > 0.0 && gamma < 0.05, "gamma positive");
  expect(vega > 20.0 && vega < 50.0, "vega positive");
  expect(rho < 0.0, "put rho negative");
  auto up = f;
  up.market = fin::market::make(101.0, 0.05, 0.0, cfg);
  auto dn = f;
  dn.market = fin::market::make(99.0, 0.05, 0.0, cfg);
  expect(std::abs((px(up) - px(dn)) / 2.0 - delta) < 5e-3, "delta agrees with a wide central difference");
}

void test_vector_of_strikes() {
  const Config cfg = f64_cpu();
  auto inst = fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg);
  inst.strike = pluss::make_series({90.0, 100.0, 110.0}, {3}, cfg);
  auto f = bs_field(inst, 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice);
  f.controls.lattice.steps = 200;
  const auto p = f.price(cfg).price.items<double>();
  expect(p.size() == 3 && p[0] < p[1] && p[1] < p[2], "put prices rise with the strike");
  expect(std::abs(p[1] - reference_tree(false, true, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2, 200)) < 1e-10, "vector entry matches the scalar tree");
}

void test_monte_carlo_regression_matches_the_lattice() {
  const Config cfg = f64_cpu();
  auto f = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::MonteCarlo);
  f.controls.mc.paths = 60000;
  f.controls.mc.steps = 50;
  f.controls.mc.antithetic = true;
  f.controls.mc.seed = 11;
  const double lsm = px(f);
  const double tree = reference_tree(false, true, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2, 2000);
  expect(std::abs(lsm - tree) < 0.06, "regression price is within Monte Carlo error of the lattice (tree " + std::to_string(tree) + ", got " + std::to_string(lsm) + ")");
  expect(lsm > bs_price(false, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2) + 0.2, "and clearly above the european price");

  std::vector<double> quarterly{0.25, 0.5, 0.75};
  auto b = bs_field(fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, quarterly, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::MonteCarlo);
  b.controls.mc = f.controls.mc;
  const double lsm_b = px(b);
  const double tree_b = reference_tree(false, false, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2, 2000, {500, 1000, 1500});
  expect(std::abs(lsm_b - tree_b) < 0.06, "bermudan regression price matches the bermudan lattice (tree " + std::to_string(tree_b) + ", got " + std::to_string(lsm_b) + ")");
  expect(lsm_b < lsm + 0.03, "a bermudan is not worth more than the american");
}

void test_monte_carlo_greeks_follow_the_lattice() {
  const Config cfg = f64_cpu();
  auto lat = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Lattice, mod::Risk::All);
  auto mc = lat;
  mc.controls.engine = mod::Engine::MonteCarlo;
  mc.controls.mc.paths = 60000;
  mc.controls.mc.steps = 50;
  mc.controls.mc.antithetic = true;
  mc.controls.mc.seed = 11;
  const auto a = lat.price(cfg).greeks, b = mc.price(cfg).greeks;
  expect(std::abs(a.delta.items<double>()[0] - b.delta.items<double>()[0]) < 0.03, "regression delta matches the lattice delta");
  expect(std::abs(a.vega.items<double>()[0] - b.vega.items<double>()[0]) < 4.0, "regression vega matches the lattice vega");
}

void test_every_model_prices_an_american_put() {
  const Config cfg = f64_cpu();
  const auto series = [&](double v) { return pluss::series::full({1}, v, cfg); };
  const std::vector<mod::Model> models{
      fin::model::Heston{.kappa = series(1.5), .theta = series(0.04), .sigma = series(0.3), .rho = series(-0.7), .v0 = series(0.04)},
      fin::model::Bates{.kappa = series(1.5), .theta = series(0.04), .sigma = series(0.3), .rho = series(-0.7), .v0 = series(0.04),
                        .jump_intensity = series(0.3), .jump_mean = series(-0.1), .jump_vol = series(0.15)},
      fin::model::Sabr{.alpha0 = series(0.2), .beta = series(1.0), .nu = series(0.3), .rho = series(-0.3)},
      fin::model::RoughHeston{.kappa = series(1.5), .theta = series(0.04), .sigma = series(0.3), .alpha = series(-0.2), .rho = series(-0.6), .v0 = series(0.04)},
      fin::model::RoughBergomi{.xi = series(0.04), .eta = series(0.8), .alpha = series(-0.2), .rho = series(-0.7)},
  };
  for (const auto& m : models) {
    auto f = bs_field(fin::instrument::american(OptionType::Put, 105.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::MonteCarlo);
    f.model = m;
    f.controls.mc.paths = 4000;
    f.controls.mc.steps = 25;
    f.controls.mc.seed = 3;
    const double v = px(f);
    expect(std::isfinite(v) && v >= 5.0 - 1e-9 && v < 25.0, f.model_name() + ": american put must be finite and at least its intrinsic value");
  }
}

void test_stochastic_volatility_collapses_to_the_lattice() {
  const Config cfg = f64_cpu();
  const auto series = [&](double v) { return pluss::series::full({1}, v, cfg); };
  mod::Field f = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::MonteCarlo);
  f.model = fin::model::Heston{.kappa = series(2.0), .theta = series(0.04), .sigma = series(0.01), .rho = series(0.0), .v0 = series(0.04)};
  f.controls.mc.paths = 60000;
  f.controls.mc.steps = 50;
  f.controls.mc.antithetic = true;
  f.controls.mc.seed = 5;
  const double tree = reference_tree(false, true, 100.0, 100.0, 0.05, 0.0, 1.0, 0.2, 2000);
  expect(std::abs(px(f) - tree) < 0.08, "Heston with vanishing vol-of-vol prices like Black-Scholes");
}

void test_closed_form_engines_refuse_early_exercise() {
  const Config cfg = f64_cpu();
  auto f = bs_field(fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg), 100.0, 0.05, 0.0, 0.2, mod::Engine::Analytic);
  bool threw = false;
  try {
    f.price(cfg);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("European exercise only") != std::string::npos;
  }
  expect(threw, "Analytic must refuse american exercise with a message that says why");
  const auto series = [&](double v) { return pluss::series::full({1}, v, cfg); };
  f.model = fin::model::Heston{.kappa = series(1.5), .theta = series(0.04), .sigma = series(0.3), .rho = series(-0.7), .v0 = series(0.04)};
  for (const auto engine : {mod::Engine::Fft}) {  // Cos now prices early exercise for Heston (american_heston_cos_test); the Fourier-inversion engine does not
    f.controls.engine = engine;
    threw = false;
    try {
      f.price(cfg);
    } catch (const std::exception& e) {
      threw = std::string(e.what()).find("European exercise only") != std::string::npos;
    }
    expect(threw, "Fft must refuse american exercise");
  }
  f.model = fin::model::Heston{.kappa = series(1.5), .theta = series(0.04), .sigma = series(0.3), .rho = series(-0.7), .v0 = series(0.04)};
  f.controls.engine = mod::Engine::Lattice;
  threw = false;
  try {
    f.price(cfg);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("Supported engines") != std::string::npos;
  }
  expect(threw, "the lattice is Black-Scholes only and says which engines the model supports");
}

void test_calibrating_to_american_quotes() {
  const Config cfg = f64_cpu();
  auto inst = fin::instrument::american(OptionType::Put, 100.0, 1.0, cfg);
  inst.strike = pluss::make_series({90.0, 100.0, 110.0}, {3}, cfg);
  auto f = bs_field(inst, 100.0, 0.05, 0.0, 0.27, mod::Engine::Lattice);
  f.controls.lattice.steps = 300;
  const auto quotes = f.price(cfg).price;
  f.model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, 0.2, cfg)};
  const auto objective = f.bind_objective_typed<fin::model::BlackScholes>(quotes, cfg);
  double lo = 0.05, hi = 0.6;
  for (int i = 0; i < 60; ++i) {  // bisection on the sum of squared price errors' derivative sign: the objective is unimodal in sigma
    const double a = lo + (hi - lo) / 3.0, b = hi - (hi - lo) / 3.0;
    const auto at = [&](double sigma) { return objective(fin::model::BlackScholes{.volatility = pluss::series::full({1}, sigma, cfg)}); };
    (at(a) < at(b)) ? hi = b : lo = a;
  }
  expect(std::abs(0.5 * (lo + hi) - 0.27) < 1e-6, "the implied volatility of american quotes is recovered");
}

}  // namespace

int main() {
  try {
    test_lattice_matches_independent_tree();
    test_known_values();
    test_call_without_dividends_is_never_exercised_early();
    test_european_is_bermudan_is_american();
    test_bermudan_dates_beyond_maturity_and_at_maturity();
    test_deep_in_the_money_put_exercises_immediately();
    test_lattice_greeks_are_consistent();
    test_vector_of_strikes();
    test_monte_carlo_regression_matches_the_lattice();
    test_monte_carlo_greeks_follow_the_lattice();
    test_every_model_prices_an_american_put();
    test_stochastic_volatility_collapses_to_the_lattice();
    test_closed_form_engines_refuse_early_exercise();
    test_calibrating_to_american_quotes();
    std::cout << "american_exercise_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "american_exercise_test failed: " << e.what() << '\n';
    return 1;
  }
}
