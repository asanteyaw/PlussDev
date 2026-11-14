// The central validation of a Field: every problem collected, errors raise, warnings do not.
#include <pluss/ma.h>

#include "../validation_internal.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace val = fin::internal::validation;
using K = fin::model::Kind;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Config cfg() { return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU); }

bool has(const val::Report& r, const std::string& where, const std::string& text) {
  return std::any_of(r.issues.begin(), r.issues.end(), [&](const val::Issue& i) {
    return i.where.find(where) != std::string::npos && i.message.find(text) != std::string::npos;
  });
}

mod::Field good_heston() {
  mod::Field f;
  f.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0, cfg());
  f.market = fin::market::make(100.0, 0.02, 0.0, cfg());
  f.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}, cfg());
  f.controls.engine = mod::Engine::Cos;
  return f;
}

void test_a_good_field_is_clean() {
  const auto r = val::check(good_heston());
  expect(r.ok() && r.issues.empty(), "a valid field has no issues: " + r.text());
}

void test_every_problem_is_collected() {
  auto f = good_heston();
  f.market.spot = pluss::series::full({1}, -5.0, cfg());
  f.instrument.strike = pluss::series::full({1}, 0.0, cfg());
  f.model.as<fin::model::Heston>().rho = pluss::series::full({1}, 1.5, cfg());
  f.controls.engine = mod::Engine::Analytic;
  const auto r = val::check(f);
  expect(!r.ok(), "invalid");
  expect(has(r, "market.spot", "> 0") && has(r, "instrument.strike", "> 0") && has(r, "model.Heston.rho", "(-1, 1)") &&
             has(r, "controls.engine", "Supported engines: Fft, Cos, MonteCarlo"),
         "all four problems reported together: " + r.text());
  bool threw = false;
  try {
    f.validate();
  } catch (const std::exception& e) {
    const std::string m = e.what();
    threw = m.find("4 problems") != std::string::npos && m.find("market.spot") != std::string::npos && m.find("model.Heston.rho") != std::string::npos;
  }
  expect(threw, "validate() raises one message listing every error");
}

void test_warnings_do_not_raise() {
  auto f = good_heston();
  f.model = fin::model::make(K::Heston, {0.5, 0.01, 0.8, -0.7, 0.04}, cfg());
  const auto r = val::check(f);
  expect(r.ok() && r.warnings().size() == 1 && has(r, "model.Heston.sigma", "Feller"), "the Feller violation is a warning: " + r.text());
  f.validate();
}

void test_exercise_rules() {
  auto f = good_heston();
  f.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0, cfg());
  expect(val::check(f).ok(), "Cos prices early exercise under Heston: " + val::check(f).text());
  f.controls.engine = mod::Engine::Fft;
  expect(has(val::check(f), "controls.engine", "European exercise only"), "Fft refuses early exercise");
  f.controls.engine = mod::Engine::MonteCarlo;
  f.controls.mc.control_variate = true;
  f.controls.mc.basis_degree = 12;
  const auto r = val::check(f);
  expect(has(r, "controls.mc.basis_degree", "[1, 8]") && has(r, "controls.mc", "control variates"), "regression settings checked: " + r.text());
  f.controls.mc = {};
  f.instrument = fin::instrument::bermudan(fin::pricing::Put, 100.0, 1.0, std::vector<double>{-0.5}, cfg());
  expect(has(val::check(f), "instrument.exercise_times", "> 0"), "a Bermudan date must be positive");
  f.instrument.exercise_times = pluss::series::undefined();
  expect(has(val::check(f), "instrument.exercise_times", "defined"), "a Bermudan needs dates");
  f.instrument = fin::instrument::make(fin::pricing::Put, 100.0, 1.0, cfg());
  f.instrument.exercise_times = pluss::series::full({2}, 0.5, cfg());
  expect(val::check(f).warnings().size() == 1, "dates on a European are ignored, with a warning");
}

void test_sizes_and_lattice() {
  auto f = good_heston();
  f.instrument.strike = pluss::series::full({3}, 100.0, cfg());
  f.market.rate = pluss::series::full({2}, 0.02, cfg());
  expect(has(val::check(f), "instrument/market", "same number of elements"), "3 strikes against 2 rates");
  mod::Field b;
  b.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0, cfg());
  b.market = fin::market::make(100.0, 0.05, 0.0, cfg());
  b.model = fin::model::make(K::BlackScholes, {0.001}, cfg());
  b.controls.engine = mod::Engine::Lattice;
  b.controls.lattice.steps = 10;
  expect(has(val::check(b), "controls.lattice.steps", "risk-neutral probability"), "tree needs room");
  b.controls.lattice.steps = 0;
  expect(has(val::check(b), "controls.lattice.steps", ">= 1"), "steps >= 1");
}

void test_default_field_asks_for_its_inputs() {
  const mod::Field blank;
  const auto r = val::check(blank);
  expect(has(r, "instrument.strike", "defined") && has(r, "market.spot", "defined") && has(r, "model.BlackScholes.volatility", "defined"), "blank field: " + r.text());
}

}  // namespace

int main() {
  try {
    test_a_good_field_is_clean();
    test_every_problem_is_collected();
    test_warnings_do_not_raise();
    test_exercise_rules();
    test_sizes_and_lattice();
    test_default_field_asks_for_its_inputs();
    std::cout << "finance_validation_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "finance_validation_test failed: " << e.what() << '\n';
    return 1;
  }
}
