// The COS engine for early exercise under Black-Scholes (Fang and Oosterlee, Numer. Math. 114 (2009) 27-62).
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

double european(bool call, double s, double k, double r, double q, double t, double sg) {
  const auto cdf = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
  const double d1 = (std::log(s / k) + (r - q + 0.5 * sg * sg) * t) / (sg * std::sqrt(t)), d2 = d1 - sg * std::sqrt(t);
  return call ? s * std::exp(-q * t) * cdf(d1) - k * std::exp(-r * t) * cdf(d2) : k * std::exp(-r * t) * cdf(-d2) - s * std::exp(-q * t) * cdf(-d1);
}

double cos_price(OptionType t, ExerciseStyle style, double s, double k, double r, double q, double maturity, double sg, std::vector<double> dates = {}, pr::CosOptions o = {}) {
  return pr::detail::black_scholes_price_cos(t, style, s, k, r, q, maturity, sg, dates, o);
}

void test_european_is_the_closed_form() {
  for (const bool call : {false, true})
    for (const double q : {0.0, 0.03}) {
      const double got = cos_price(call ? OptionType::Call : OptionType::Put, ExerciseStyle::European, 100, 95, 0.05, q, 1.3, 0.25);
      expect(std::abs(got - european(call, 100, 95, 0.05, q, 1.3, 0.25)) < 1e-9, "COS European vs the closed form");
    }
}

// Source: F. Fang and C. W. Oosterlee, Numer. Math. 114 (2009), Section 4.1, Test No. 1: a Bermudan put with ten equally spaced
// exercise dates, S0 = 100, K = 110, T = 1, r = 0.1, sigma = 0.2; reference value 10.479520123 (computed there by another method).
void test_published_bermudan_value() {
  std::vector<double> dates;
  for (int k = 1; k < 10; ++k) dates.push_back(k / 10.0);
  const double got = cos_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 110, 0.1, 0.0, 1.0, 0.2, dates, {.exercise_terms = 256});
  expect(std::abs(got - 10.479520123) < 5e-9, "published Bermudan put: " + std::to_string(got));
  // exponential convergence in the number of terms
  double previous = 1e9;
  for (const int n : {32, 64, 128}) {
    const double err = std::abs(cos_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 110, 0.1, 0.0, 1.0, 0.2, dates, {.exercise_terms = n}) - 10.479520123);
    expect(err < previous / 5.0, "the error falls fast with the number of terms");
    previous = err;
  }
}

void test_agrees_with_other_engines() {
  // Bermudan: against the binomial lattice, extrapolated for its first-order error
  const std::vector<double> dates{0.25, 0.5, 0.75};
  const double coarse = pr::detail::lattice_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 100, 0.05, 0.0, 1.0, 0.2, 4000, dates);
  const double fine = pr::detail::lattice_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 100, 0.05, 0.0, 1.0, 0.2, 8000, dates);
  expect(std::abs(cos_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 100, 0.05, 0.0, 1.0, 0.2, dates) - (2.0 * fine - coarse)) < 5e-5, "COS Bermudan vs the extrapolated lattice");
  // European <= Bermudan <= American, and a call without dividends is never exercised early
  const double e = cos_price(OptionType::Put, ExerciseStyle::European, 100, 100, 0.05, 0.0, 1.0, 0.2), b = cos_price(OptionType::Put, ExerciseStyle::Bermudan, 100, 100, 0.05, 0.0, 1.0, 0.2, dates);
  const double a = cos_price(OptionType::Put, ExerciseStyle::American, 100, 100, 0.05, 0.0, 1.0, 0.2);
  expect(e < b && b < a, "European < Bermudan < American");
  expect(std::abs(cos_price(OptionType::Call, ExerciseStyle::American, 100, 100, 0.05, 0.0, 1.0, 0.25) - european(true, 100, 100, 0.05, 0.0, 1.0, 0.25)) < 1e-5,
         "an American call without dividends is the European call");
  // American by extrapolation against the spectral price
  struct Case { OptionType t; double s, k, r, q, T, v, tol; } cases[] = {{OptionType::Put, 100, 100, 0.05, 0.0, 1.0, 0.2, 1e-4}, {OptionType::Put, 100, 100, 0.02, 0.06, 1.0, 0.3, 1e-4},
                                                                      {OptionType::Call, 100, 100, 0.03, 0.07, 1.0, 0.25, 1e-4}, {OptionType::Put, 80, 100, 0.04, 0.01, 3.0, 0.2, 3e-3}};
  for (const auto& c : cases) {
    const double want = pr::detail::spectral_price(c.t, c.s, c.k, c.r, c.q, c.T, c.v), got = cos_price(c.t, ExerciseStyle::American, c.s, c.k, c.r, c.q, c.T, c.v);
    expect(std::abs(got - want) < c.tol, "COS American vs spectral: " + std::to_string(got) + " vs " + std::to_string(want));
  }
}

void test_field_engine() {
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(K::BlackScholes, {0.2});
  f.controls.engine = mod::Engine::Cos;
  f.controls.risk = mod::Risk::All;
  const auto out = f.price();
  expect(std::abs(out.price.items<double>()[0] - 6.0903706) < 1e-4, "Field American through COS");
  const double delta = out.greeks.delta.items<double>()[0];
  expect(delta < -0.38 && delta > -0.44, "delta: " + std::to_string(delta));
  f.instrument = fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, std::vector<double>{0.5});
  f.controls.risk = mod::Risk::PriceOnly;
  const double bermudan = f.price().price.items<double>()[0];
  f.instrument = fin::instrument::make(OptionType::Put, 100.0, 1.0);
  const double euro = f.price().price.items<double>()[0];
  expect(bermudan > euro && std::abs(euro - european(false, 100, 100, 0.05, 0, 1, 0.2)) < 1e-9, "one interim date adds value; European is the closed form");

  f.model = fin::model::make(K::Sabr, {0.2, 1.0, 0.3, -0.3});
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  bool threw = false;
  try {
    f.price();
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("is not supported for model Sabr") != std::string::npos;
  }
  expect(threw, "SABR has no Cos engine, and the error lists what it has");
}

}  // namespace

int main() {
  try {
    test_european_is_the_closed_form();
    test_published_bermudan_value();
    test_agrees_with_other_engines();
    test_field_engine();
    std::cout << "american_cos_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "american_cos_test failed: " << e.what() << '\n';
    return 1;
  }
}
