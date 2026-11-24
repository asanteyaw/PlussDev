#include "pluss/error.h"
#include "pluss/ma/natsci/common.h"
#include "pluss/subsp/natsci/common/speciation.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace spec = pluss::subsp::natsci::common::speciation;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::vector<double> v) { return pluss::series::make(v).to(F64); }
Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(F64); }
double at(const Series &s, std::int64_t i) { return s.reshape({s.numel()}).items<double>()[static_cast<std::size_t>(i)]; }

// components H+ and CO3 2-; complexes OH-, HCO3-, H2CO3 (pKw = 14, pK2 = 10.33, pK1 = 6.35)
struct Carbonate {
  Series n = mat({-1, 1, 2, 0, 1, 1}, 2, 3);
  Series ln_k = vec({-14.0 * std::log(10.0), 10.33 * std::log(10.0), 16.68 * std::log(10.0)});
};

void test_acid_added() {
  const Carbonate m;
  const auto r = spec::solve(m.n, m.ln_k, vec({0.005, 0.010}));
  expect(r.converged, "carbonate with acid converges");
  const double ph = -std::log10(at(r.free, 0));
  expect(std::abs(ph - 10.295806) < 1e-5, "carbonate buffer pH");
  // mass balances close
  expect(std::abs(at(r.free, 1) + at(r.complexes, 1) + at(r.complexes, 2) - 0.010) < 1e-12, "carbonate balance");
}

void test_iteration_limit_is_flagged() {
  const Carbonate m;
  spec::Options few;
  few.max_iterations = 3;
  const auto r = spec::solve(m.n, m.ln_k, vec({0.005, 0.010}), few);
  expect(!r.converged && r.iterations == 3, "hitting max_iterations is reported through converged");
}

// A pure base has zero proton excess, and an excess of base a negative one: both have a solution in H+, which is positive.
void test_zero_and_negative_proton_total() {
  const Carbonate m;
  const auto zero = spec::solve(m.n, m.ln_k, vec({0.0, 0.010}));
  expect(zero.converged, "pure carbonate (zero proton excess) converges");
  const double h = at(zero.free, 0), oh = at(zero.complexes, 0), hco3 = at(zero.complexes, 1), h2co3 = at(zero.complexes, 2);
  expect(std::abs(h - oh + hco3 + 2.0 * h2co3) < 1e-10, "proton condition holds for the pure base");
  const double ph = -std::log10(h);
  expect(ph > 11.0 && ph < 11.3, "0.01 M carbonate has a pH near 11.15");

  const auto excess = spec::solve(m.n, m.ln_k, vec({-0.004, 0.010}));
  expect(excess.converged && -std::log10(at(excess.free, 0)) > ph, "excess base raises the pH further");

  // lanes mix positive, zero and negative totals
  const auto lanes = spec::solve(m.n, m.ln_k, mat({0.005, 0.010, 0.0, 0.010, -0.004, 0.010}, 3, 2));
  expect(lanes.converged, "mixed lanes converge together");
  expect(std::abs(-std::log10(at(lanes.free, 0)) - 10.295806) < 1e-5 && std::abs(-std::log10(at(lanes.free, 2)) - ph) < 1e-8,
         "lanes agree with single solves");
}

// A mass component cannot have a negative total: there is no solution, and the flag says so.
void test_impossible_total_is_flagged() {
  const auto n = mat({1.0, 1.0}, 1, 2);
  const auto r = spec::solve(n, vec({1.0, 2.0}), vec({-1.0}));
  expect(!r.converged, "a negative total of a pure mass component is not converged");
}

void test_contract() {
  const Carbonate m;
  bool threw = false;
  try {
    (void)spec::solve(m.n, m.ln_k, vec({0.005, std::nan("")}));
  } catch (const pluss::Error &) {
    threw = true;
  }
  expect(threw, "non-finite totals rejected");
  threw = false;
  try {
    (void)spec::solve(m.n, vec({0.0, 0.0}), vec({0.005, 0.010}));
  } catch (const pluss::Error &) {
    threw = true;
  }
  expect(threw, "wrong ln_k length rejected");
}
} // namespace

int main() {
  try {
    test_acid_added();
    test_iteration_limit_is_flagged();
    test_zero_and_negative_proton_total();
    test_impossible_total_is_flagged();
    test_contract();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "speciation tests passed\n";
  return 0;
}
