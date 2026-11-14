// The Heston PDE engine (ADI finite differences, Ikonen-Toivanen exercise): stencils checked against automatic differentiation, European
// values against the COS engine, exact identities of early exercise, and the regression engine as an independent American estimate.
#include <pluss/ma.h>

#include "../pde_stencils_internal.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace pr = fin::pricing;
using pr::ExerciseStyle;
using pr::OptionType;
using K = fin::model::Kind;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Config cfg() { return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU); }

const fin::volatility::HestonParams kHeston{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04};

double cos_price(OptionType t, double strike, double r, double q, double maturity, const fin::volatility::HestonParams& h = kHeston) {
  return pr::detail::heston_price_cos(t, 100.0, strike, r, q, maturity, h, {}, cfg());
}

double pde(OptionType t, ExerciseStyle style, double strike, double r, double q, double maturity, const fin::volatility::HestonParams& h = kHeston,
           std::vector<double> dates = {}, pr::PdeOptions o = {}) {
  return pr::detail::heston_price_pde(t, style, 100.0, strike, r, q, maturity, h, dates, o);
}

void test_stencils_against_automatic_differentiation() {
  const auto f = [](const Series& x) { return x.mul(0.02).sin().add(x.square().mul(1e-4)); };
  double previous_first = 1e9, previous_second = 1e9;
  for (const int n : {40, 80, 160}) {  // a sinh-stretched grid, as the solver builds
    std::vector<double> x(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) x[static_cast<std::size_t>(i)] = 100.0 + 20.0 * std::sinh(-2.0 + 4.0 * i / (n - 1));
    const Series points = pluss::make_series(x, {n}, cfg());
    const auto fv = f(points).items<double>();
    // for an elementwise f the derivative at node i is the diagonal of the Jacobian (order 1) and of the order-2 tensor
    const Series j1 = points.calc.diff(f, 1), j2 = points.calc.diff(f, 2);
    const auto full1 = j1.reshape({j1.numel()}).items<double>(), full2 = j2.reshape({j2.numel()}).items<double>();
    std::vector<double> d1(x.size()), d2(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
      d1[i] = full1[i * x.size() + i];
      d2[i] = full2[(i * x.size() + i) * x.size() + i];
    }
    const auto a1 = fin::internal::pde::apply_first(x, fv), a2 = fin::internal::pde::apply_second(x, fv);
    double e1 = 0.0, e2 = 0.0;
    for (std::size_t i = 1; i + 1 < x.size(); ++i) {
      e1 = std::max(e1, std::abs(a1[i] - d1[i]));
      e2 = std::max(e2, std::abs(a2[i] - d2[i]));
    }
    expect(e1 < previous_first && e2 < previous_second, "the stencil error falls as the grid is refined");
    if (n > 40) expect(e1 < previous_first / 3.0 && e2 < previous_second / 3.0, "and by about four times per doubling (second order)");
    previous_first = e1;
    previous_second = e2;
  }
}

void test_european_values_match_cos() {
  for (const auto t : {OptionType::Put, OptionType::Call})
    for (const double strike : {90.0, 100.0, 110.0}) {
      const double want = cos_price(t, strike, 0.03, 0.01, 1.0), got = pde(t, ExerciseStyle::European, strike, 0.03, 0.01, 1.0);
      expect(std::abs(got - want) < 1.5e-3, "PDE European vs COS: " + std::to_string(got) + " vs " + std::to_string(want));
    }
  // second-order convergence in the asset grid
  const double want = cos_price(OptionType::Put, 100.0, 0.03, 0.01, 1.0);
  const double coarse = std::abs(pde(OptionType::Put, ExerciseStyle::European, 100.0, 0.03, 0.01, 1.0, kHeston, {}, {.spot_nodes = 120}) - want);
  const double fine = std::abs(pde(OptionType::Put, ExerciseStyle::European, 100.0, 0.03, 0.01, 1.0, kHeston, {}, {.spot_nodes = 480}) - want);
  expect(fine < coarse / 6.0, "refining the asset grid fourfold shrinks the error: " + std::to_string(coarse) + " -> " + std::to_string(fine));
  expect(fine < 3e-4, "480 asset nodes are accurate to 3e-4: " + std::to_string(fine));
  // a different regime: short maturity, high vol of vol, Feller violated
  const fin::volatility::HestonParams wild{.kappa = 1.0, .theta = 0.05, .sigma = 0.8, .rho = -0.5, .v0 = 0.06};
  expect(std::abs(pde(OptionType::Put, ExerciseStyle::European, 100.0, 0.02, 0.0, 0.5, wild) - cos_price(OptionType::Put, 100.0, 0.02, 0.0, 0.5, wild)) < 4e-3,
         "a Feller-violating parameter set");
}

void test_early_exercise_identities() {
  const double e = pde(OptionType::Call, ExerciseStyle::European, 100.0, 0.05, 0.0, 1.0), a = pde(OptionType::Call, ExerciseStyle::American, 100.0, 0.05, 0.0, 1.0);
  expect(std::abs(a - e) < 1e-4, "an American call without dividends is the European call: " + std::to_string(a - e));
  const double pe = pde(OptionType::Put, ExerciseStyle::European, 100.0, 0.05, 0.0, 1.0), pa = pde(OptionType::Put, ExerciseStyle::American, 100.0, 0.05, 0.0, 1.0);
  const double quarterly = pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 0.05, 0.0, 1.0, kHeston, {0.25, 0.5, 0.75});
  std::vector<double> monthly;
  for (int m = 1; m < 12; ++m) monthly.push_back(m / 12.0);
  const double monthly_price = pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 0.05, 0.0, 1.0, kHeston, monthly);
  expect(pe < quarterly && quarterly < monthly_price && monthly_price < pa + 1e-6, "European < Bermudan(quarterly) < Bermudan(monthly) < American");
  expect(pa - pe > 0.3, "the early-exercise premium of the put is material");
  expect(pa >= 0.0 && pa >= std::max(100.0 - 100.0, 0.0), "above intrinsic");
  // dates at or beyond the maturity add nothing
  const double with_late = pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 0.05, 0.0, 1.0, kHeston, {0.5, 1.0, 2.0});
  const double only_half = pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 0.05, 0.0, 1.0, kHeston, {0.5});
  expect(std::abs(with_late - only_half) < 1e-12, "dates at or beyond the maturity are not part of the option's life");
  // a Bermudan on every time step is the American
  std::vector<double> every;
  for (int j = 1; j < 60; ++j) every.push_back(j / 60.0);
  const double dense = pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 0.05, 0.0, 1.0, kHeston, every, {.time_steps = 60});
  const double american60 = pde(OptionType::Put, ExerciseStyle::American, 100.0, 0.05, 0.0, 1.0, kHeston, {}, {.time_steps = 60});
  expect(dense <= american60 + 1e-6 && american60 - dense < 1.5e-2, "daily-ish Bermudan approaches the American: " + std::to_string(dense) + " vs " + std::to_string(american60));
}

void test_agrees_with_the_regression_engine() {
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  f.controls.risk = mod::Risk::PriceOnly;
  f.controls.engine = mod::Engine::Pde;
  const double grid = f.price().price.items<double>()[0];
  f.controls.engine = mod::Engine::MonteCarlo;
  f.controls.mc.paths = 80000;
  f.controls.mc.steps = 50;
  f.controls.mc.antithetic = true;
  f.controls.mc.seed = 7;
  const double regression = f.price().price.items<double>()[0];
  expect(grid - regression > -0.03 && grid - regression < 0.1, "the PDE price is above the low-biased regression price by at most its bias: " + std::to_string(grid) + " vs " + std::to_string(regression));
  f.controls.engine = mod::Engine::Cos;
  f.instrument = fin::instrument::make(OptionType::Put, 100.0, 1.0);
  expect(grid > f.price().price.items<double>()[0] + 0.3, "and above the European price");
}

void test_field_greeks_and_strips() {
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  f.controls.engine = mod::Engine::Pde;
  f.controls.risk = mod::Risk::All;
  const auto out = f.price();
  const double delta = out.greeks.delta.items<double>()[0], gamma = out.greeks.gamma.items<double>()[0], vega = out.greeks.vega.items<double>()[0];
  expect(delta < -0.3 && delta > -0.6, "delta of an American put: " + std::to_string(delta));
  expect(gamma > 0.0 && gamma < 0.06, "gamma: " + std::to_string(gamma));
  expect(vega > 0.0, "vega with respect to the initial variance: " + std::to_string(vega));
  auto up = f, dn = f;
  up.market = fin::market::make(102.0, 0.05, 0.0);
  dn.market = fin::market::make(98.0, 0.05, 0.0);
  up.controls.risk = dn.controls.risk = mod::Risk::PriceOnly;
  expect(std::abs((up.price().price.items<double>()[0] - dn.price().price.items<double>()[0]) / 4.0 - delta) < 6e-3, "delta agrees with a wide central difference");

  f.controls.risk = mod::Risk::PriceOnly;
  f.instrument.strike = pluss::make_series({90.0, 100.0, 110.0}, {3}, cfg());
  const auto strip = f.price().price.items<double>();
  expect(strip.size() == 3 && strip[0] < strip[1] && strip[1] < strip[2], "a strip of strikes");

  f.instrument = fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, std::vector<double>{0.5});
  const double bermudan = f.price().price.items<double>()[0];
  f.instrument = fin::instrument::make(OptionType::Put, 100.0, 1.0);
  const double european = f.price().price.items<double>()[0];
  expect(bermudan > european, "one interim date adds value");
}

// Published reference values.  Source: K. J. in 't Hout and T. Haentjens, "ADI schemes for pricing American options under the Heston
// model" (arXiv:1309.0110): Tables 2 and 3 quote the Ikonen-Toivanen Runge-Kutta values (row "RK-IT") for the classical test case
// kappa = 5, eta = 0.16, sigma = 0.9, rho = 0.1, r = 0.1, T = 0.25, K = 10; Table 4 quotes Bermudan puts by the COS method of
// Fang and Oosterlee for a parameter set that violates the Feller condition.
void test_published_values() {
  const double ikonen_toivanen[2][5] = {{2.0000, 1.1076, 0.5199, 0.2135, 0.0820}, {2.0785, 1.3336, 0.7959, 0.4482, 0.2427}};
  const double v0s[2] = {0.0625, 0.25};
  for (int row = 0; row < 2; ++row) {
    const fin::volatility::HestonParams h{.kappa = 5.0, .theta = 0.16, .sigma = 0.9, .rho = 0.1, .v0 = v0s[row]};
    for (int col = 0; col < 5; ++col) {
      const double s = 8.0 + col;
      const double got = pr::detail::heston_price_pde(OptionType::Put, ExerciseStyle::American, s, 10.0, 0.1, 0.0, 0.25, h);
      expect(std::abs(got - ikonen_toivanen[row][col]) < 3e-4,
             "published American put, v0 " + std::to_string(v0s[row]) + " S0 " + std::to_string(s) + ": " + std::to_string(got) + " vs " + std::to_string(ikonen_toivanen[row][col]));
    }
  }
  const fin::volatility::HestonParams feller{.kappa = 1.15, .theta = 0.0348, .sigma = 0.39, .rho = -0.64, .v0 = 0.0348};
  const double cos_bermudan[3][3] = {{9.9784, 3.2047, 0.9274}, {9.9916, 3.2073, 0.9281}, {9.9958, 3.2079, 0.9280}};  // N = 20, 40, 60 dates; S0 = 90, 100, 110
  const int dates_count[3] = {20, 40, 60};
  for (int row = 0; row < 3; ++row) {
    std::vector<double> dates;
    for (int k = 1; k < dates_count[row]; ++k) dates.push_back(0.25 * k / dates_count[row]);
    for (int col = 0; col < 3; ++col) {
      const double got = pr::detail::heston_price_pde(OptionType::Put, ExerciseStyle::Bermudan, 90.0 + 10.0 * col, 100.0, 0.04, 0.0, 0.25, feller, dates, {.time_steps = 200});
      expect(std::abs(got - cos_bermudan[row][col]) < 3e-3, "published Bermudan put, N " + std::to_string(dates_count[row]) + " S0 " + std::to_string(90 + 10 * col) + ": " +
                                                                std::to_string(got) + " vs " + std::to_string(cos_bermudan[row][col]));
    }
  }
}

void test_validation() {
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1, -0.1, 0.15});
  f.controls.engine = mod::Engine::Pde;
  bool threw = false;
  try {
    f.price();
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("Supported engines: Fft, Cos, MonteCarlo") != std::string::npos;
  }
  expect(threw, "Bates has no PDE engine and the error lists what it has");
  f.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  f.controls.pde.spot_nodes = 3;
  threw = false;
  try {
    f.price();
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("controls.pde.spot_nodes") != std::string::npos;
  }
  expect(threw, "the grid size is validated");
}

}  // namespace

int main() {
  try {
    test_stencils_against_automatic_differentiation();
    test_european_values_match_cos();
    test_early_exercise_identities();
    test_published_values();
    test_agrees_with_the_regression_engine();
    test_field_greeks_and_strips();
    test_validation();
    std::cout << "american_pde_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "american_pde_test failed: " << e.what() << '\n';
    return 1;
  }
}
