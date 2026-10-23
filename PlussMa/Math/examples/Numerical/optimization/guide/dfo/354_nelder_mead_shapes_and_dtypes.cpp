// Shapes and dtypes. The starting point may have any shape; the minimiser comes back with that shape. A 2x2 matrix of
// parameters is searched as four variables, and the objective receives a 2x2 Series. On the CPU a Float32 start is
// promoted to Float64 for the search (promote_cpu_to_float64, on by default) and the answer is cast back to Float32.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

int main() {
  // Find the 2x2 matrix closest (in sum of squares) to a target.
  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(pluss::ScalarType::Float64);
  auto f = [&](const pluss::Series& m) { return (m - target).square().sum(); };

  const auto start64 = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0}, {2, 2}).to(pluss::ScalarType::Float64);
  const auto a = dfo::nelder_mead_minimizer(f, start64);
  std::cout << "Float64 start -> shape " << a.minimizer.size(0) << "x" << a.minimizer.size(1) << ", dtype float64: "
            << (a.minimizer.scalar_type() == pluss::ScalarType::Float64) << ", minimum " << std::scientific << std::setprecision(1) << a.minimum << "\n";

  const auto start32 = start64.to(pluss::ScalarType::Float32);
  auto g = [&](const pluss::Series& m) { return (m.to(pluss::ScalarType::Float64) - target).square().sum(); };
  const auto b = dfo::nelder_mead_minimizer(g, start32);
  std::cout << "Float32 start -> shape " << b.minimizer.size(0) << "x" << b.minimizer.size(1) << ", dtype float32: "
            << (b.minimizer.scalar_type() == pluss::ScalarType::Float32) << ", minimum " << b.minimum << "\n";
}
