// The COS engine for early exercise under Heston and Bates (Fang and Oosterlee, SIAM J. Financial Math. 2 (2011) 439-463).
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
namespace mod = fin::module;
namespace pr = fin::pricing;
using pr::ExerciseStyle;
using pr::OptionType;
using K = fin::model::Kind;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Config cfg() { return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU); }

std::vector<double> equally_spaced(int count, double maturity) {
  std::vector<double> dates;
  for (int k = 1; k < count; ++k) dates.push_back(maturity * k / count);
  return dates;
}

void test_one_date_is_the_european_price() {
  const fin::volatility::HestonParams h{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04};
  for (const auto t : {OptionType::Put, OptionType::Call}) {
    const double want = pr::detail::heston_price_cos(t, 100.0, 100.0, 0.03, 0.01, 1.0, h, {}, cfg());
    const double got = pr::detail::heston_price_cos_exercise(t, ExerciseStyle::European, 100.0, 100.0, 0.03, 0.01, 1.0, h, {}, {.exercise_terms = 128, .variance_nodes = 64, .variance_tolerance = 1e-8});
    expect(std::abs(got - want) < 2e-5, "Heston European through the early-exercise recursion: " + std::to_string(got) + " vs " + std::to_string(want));
  }
  const fin::volatility::BatesParams b{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04, .jump_intensity = 0.4, .jump_mean = -0.1, .jump_vol = 0.15};
  const double want = pr::detail::bates_price_cos(OptionType::Put, 100.0, 95.0, 0.03, 0.01, 1.0, b, {}, cfg());
  const double got = pr::detail::bates_price_cos_exercise(OptionType::Put, ExerciseStyle::European, 100.0, 95.0, 0.03, 0.01, 1.0, b, {}, {.exercise_terms = 128, .variance_nodes = 64});
  expect(std::abs(got - want) < 3e-5, "Bates European: " + std::to_string(got) + " vs " + std::to_string(want));
  // no jumps: Bates is Heston
  const fin::volatility::BatesParams nojump{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04, .jump_intensity = 0.0, .jump_mean = 0.0, .jump_vol = 0.1};
  const std::vector<double> dates{0.25, 0.5, 0.75};
  expect(std::abs(pr::detail::bates_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.03, 0.0, 1.0, nojump, dates, {.exercise_terms = 96, .variance_nodes = 64}) -
                  pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.03, 0.0, 1.0, {.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04}, dates,
                                                {.exercise_terms = 96, .variance_nodes = 64})) < 1e-12,
         "Bates without jumps is Heston");
}

// Source: F. Fang and C. W. Oosterlee, SIAM J. Financial Math. 2 (2011), Table 4 (Test No. 4, q = 0.98: kappa = 5, eta = 0.16, sigma = 0.9, rho = 0.1,
// r = 0.1, T = 0.25, K = 10, v0 = 0.0625).  The table gives the error of the Bermudan price with M dates against the American reference prices
// 2.000000, 1.107621, 0.520030, 0.213677, 0.082044 for S0 = 8 ... 12; the rows for M = 10 and 20 are below.
void test_published_bermudan_errors() {
  const fin::volatility::HestonParams h{.kappa = 5.0, .theta = 0.16, .sigma = 0.9, .rho = 0.1, .v0 = 0.0625};
  const double reference[5] = {2.000000, 1.107621, 0.520030, 0.213677, 0.082044};
  const double published[2][5] = {{-1.80e-2, -4.79e-3, -2.85e-3, -1.31e-3, -5.18e-4}, {-9.54e-3, -2.39e-3, -1.40e-3, -6.65e-4, -2.78e-4}};
  const int counts[2] = {10, 20};
  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 5; ++col) {
      const double got = pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 8.0 + col, 10.0, 0.1, 0.0, 0.25, h, equally_spaced(counts[row], 0.25),
                                                       {.exercise_terms = 96, .variance_nodes = 96, .variance_tolerance = 1e-7});
      expect(std::abs((got - reference[col]) - published[row][col]) < 3e-4,
             "published Bermudan error, M " + std::to_string(counts[row]) + " S0 " + std::to_string(8 + col) + ": " + std::to_string(got - reference[col]) + " vs " + std::to_string(published[row][col]));
    }
  // a Bermudan option converges to its American limit at first order in M: the error against the American reference halves with the dates
  double previous = 0.0;
  for (const int m : {10, 20, 40}) {
    const double err = reference[2] - pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 10.0, 10.0, 0.1, 0.0, 0.25, h, equally_spaced(m, 0.25),
                                                                    {.exercise_terms = 96, .variance_nodes = 128, .variance_tolerance = 1e-7});
    if (previous > 0.0) expect(previous / err > 1.7 && previous / err < 2.3, "the Bermudan error halves when the dates double: " + std::to_string(previous / err));
    previous = err;
  }
}

void test_agrees_with_the_pde_and_the_regression() {
  const fin::volatility::HestonParams feller{.kappa = 1.15, .theta = 0.0348, .sigma = 0.39, .rho = -0.64, .v0 = 0.0348};  // violates the Feller condition
  const auto dates = equally_spaced(20, 0.25);
  const double cos = pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.04, 0.0, 0.25, feller, dates, {.exercise_terms = 128, .variance_nodes = 128, .variance_tolerance = 1e-6});
  const double pde = pr::detail::heston_price_pde(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.04, 0.0, 0.25, feller, dates, {.spot_nodes = 480, .variance_nodes = 120, .time_steps = 400});
  expect(std::abs(cos - pde) < 6e-4, "COS and PDE Bermudan prices agree: " + std::to_string(cos) + " vs " + std::to_string(pde));
  // published (Table 5 of the same paper) 3.2047434; the printed values are less converged than the two independent computations above
  expect(std::abs(cos - 3.2047434) < 1.5e-3, "close to the published value: " + std::to_string(cos));
  // American (extrapolated) against the PDE
  const fin::volatility::HestonParams easy{.kappa = 5.0, .theta = 0.16, .sigma = 0.9, .rho = 0.1, .v0 = 0.0625};
  for (const double s : {9.0, 10.0, 11.0}) {
    const double a = pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::American, s, 10.0, 0.1, 0.0, 0.25, easy, {}, {.exercise_terms = 96, .variance_nodes = 96});
    const double b = pr::detail::heston_price_pde(OptionType::Put, ExerciseStyle::American, s, 10.0, 0.1, 0.0, 0.25, easy, {}, {.spot_nodes = 480, .variance_nodes = 120, .time_steps = 200});
    expect(std::abs(a - b) < 2e-3, "American COS vs PDE at S0 " + std::to_string(s) + ": " + std::to_string(a) + " vs " + std::to_string(b));
  }
}

void test_calls_and_bates() {
  const fin::volatility::HestonParams h{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04};
  // an American call without dividends is the European call
  const double e = pr::detail::heston_price_cos(OptionType::Call, 100.0, 100.0, 0.05, 0.0, 1.0, h, {}, cfg());
  const double a = pr::detail::heston_price_cos_exercise(OptionType::Call, ExerciseStyle::American, 100.0, 100.0, 0.05, 0.0, 1.0, h, {}, {.exercise_terms = 96, .variance_nodes = 96});
  expect(std::abs(a - e) < 2e-3, "American call without dividends: " + std::to_string(a) + " vs " + std::to_string(e));
  // Bates: European < Bermudan < American, and the jumps matter
  const fin::volatility::BatesParams b{.kappa = 1.5, .theta = 0.04, .sigma = 0.3, .rho = -0.7, .v0 = 0.04, .jump_intensity = 0.4, .jump_mean = -0.1, .jump_vol = 0.15};
  const std::vector<double> quarterly{0.25, 0.5, 0.75};
  const pr::CosOptions o{.exercise_terms = 96, .variance_nodes = 96};
  const double european = pr::detail::bates_price_cos_exercise(OptionType::Put, ExerciseStyle::European, 100.0, 100.0, 0.05, 0.0, 1.0, b, {}, o);
  const double bermudan = pr::detail::bates_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.05, 0.0, 1.0, b, quarterly, o);
  const double american = pr::detail::bates_price_cos_exercise(OptionType::Put, ExerciseStyle::American, 100.0, 100.0, 0.05, 0.0, 1.0, b, {}, o);
  expect(european < bermudan && bermudan < american, "Bates: European < Bermudan < American");
  // against the regression engine (low-biased) through the Field
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.4, -0.1, 0.15});
  f.controls.engine = mod::Engine::MonteCarlo;
  f.controls.risk = mod::Risk::PriceOnly;
  f.controls.mc.paths = 60000;
  f.controls.mc.steps = 50;
  f.controls.mc.antithetic = true;
  f.controls.mc.seed = 5;
  const double regression = f.price().price.items<double>()[0];
  expect(american - regression > -0.05 && american - regression < 0.15, "Bates American COS above the low-biased regression: " + std::to_string(american) + " vs " + std::to_string(regression));
}

void test_the_resolution_guard_refuses_instead_of_returning_garbage() {
  const fin::volatility::HestonParams feller{.kappa = 1.15, .theta = 0.0348, .sigma = 0.39, .rho = -0.64, .v0 = 0.0348};
  bool refused = false;
  try {
    pr::detail::heston_price_cos_exercise(OptionType::Put, ExerciseStyle::Bermudan, 100.0, 100.0, 0.04, 0.0, 0.25, feller, equally_spaced(40, 0.25), {.exercise_terms = 64, .variance_nodes = 32});
  } catch (const std::exception& e) {
    refused = std::string(e.what()).find("does not resolve the transition density") != std::string::npos;
  }
  expect(refused, "32 variance nodes cannot resolve a 40-date Feller-violating problem, and the engine says so");
}

void test_field_engine() {
  mod::Field f;
  f.instrument = fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, std::vector<double>{0.5});
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  f.controls.engine = mod::Engine::Cos;
  f.controls.risk = mod::Risk::PriceOnly;
  f.controls.cos.exercise_terms = 96;
  f.controls.cos.variance_nodes = 64;
  const double bermudan = f.price().price.items<double>()[0];
  f.instrument = fin::instrument::make(OptionType::Put, 100.0, 1.0);
  const double european = f.price().price.items<double>()[0];
  expect(bermudan > european + 0.1, "one interim date adds value through the Field: " + std::to_string(bermudan) + " vs " + std::to_string(european));
  f.model = fin::model::make(K::Sabr, {0.2, 1.0, 0.3, -0.3});
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  bool threw = false;
  try {
    f.price();
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("is not supported for model Sabr") != std::string::npos;
  }
  expect(threw, "SABR has no Cos engine at all");
}

}  // namespace

int main() {
  try {
    test_one_date_is_the_european_price();
    test_published_bermudan_errors();
    test_agrees_with_the_pde_and_the_regression();
    test_calls_and_bates();
    test_the_resolution_guard_refuses_instead_of_returning_garbage();
    test_field_engine();
    std::cout << "american_heston_cos_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "american_heston_cos_test failed: " << e.what() << '\n';
    return 1;
  }
}
