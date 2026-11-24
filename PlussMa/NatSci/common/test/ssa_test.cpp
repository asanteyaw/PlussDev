// Gillespie direct method: the moments of a birth-death ensemble against its closed form, reproducibility by seed, and the validation of the
// propensity callback (shape, sign, finiteness).
#include "pluss/error.h"
#include "pluss/ma/natsci/common.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace ssa = pluss::ma::natsci::common::ssa;
using pluss::Series;

namespace {
void expect(bool c, const std::string &m) {
  if (!c)
    throw pluss::Error(m);
}
const auto F64 = pluss::ScalarType::Float64;
Series immigration_death(const Series &x) { return x.mul(0.0).add(5.0).unite(std::vector<Series>{x.mul(0.5)}, -1); }
template <class F> bool rejects(F &&f) {
  try {
    f();
  } catch (const pluss::Error &) {
    return true;
  }
  return false;
}
} // namespace

int main() {
  try {
    const Series nu = pluss::series::make({1.0, -1.0}, {2, 1});
    const Series x0 = pluss::series::zeros({2000, 1}, pluss::Config{}.with_dtype(F64));
    ssa::SsaOptions options;
    options.seed = 12345;
    const auto run = ssa::gillespie(immigration_death, nu, x0, 20.0, options);
    const double mean = run.state.mean().item<double>();
    expect(run.completed && std::abs(mean - 10.0) < 0.3, "birth-death mean must approach k / g = 10");
    expect(ssa::gillespie(immigration_death, nu, x0, 20.0, options).state.eq(run.state).all().item<bool>(), "a fixed seed must reproduce the ensemble");

    const Series four = pluss::series::full({4, 1}, 5.0, pluss::Config{}.with_dtype(F64));
    expect(rejects([&] { ssa::gillespie([](const Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<Series>{x.mul(-1.0)}, -1); }, nu, four, 5.0); }),
           "a negative propensity must be rejected");
    expect(rejects([&] { ssa::gillespie([](const Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<Series>{x.mul(std::numeric_limits<double>::quiet_NaN())}, -1); }, nu, four, 5.0); }),
           "a NaN propensity must be rejected");
    expect(rejects([&] { ssa::gillespie([](const Series &x) { return x.select(-1, 0); }, nu, four, 5.0); }), "a propensity of the wrong rank must be rejected");
    expect(rejects([&] { ssa::gillespie([](const Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<Series>{x, x}, -1); }, nu, four, 5.0); }),
           "a propensity with the wrong number of reactions must be rejected");
    std::cout << "ssa_test: all checks passed\n";
    return 0;
  } catch (const pluss::Error &e) {
    std::cerr << "ssa_test failed: " << e.message() << "\n";
    return 1;
  }
}
