// The Nelder-Mead minimiser called directly, without the DFO facade. `nelder_mead_minimizer(objective, initial,
// config)` returns a NelderMeadOutcome: the minimiser (with the shape and dtype of `initial`), the minimum, a status,
// and diagnostics (iterations, evaluations, and the last simplex step taken). The objective may return a Series or a
// plain double. Here it minimises the 2-D Rosenbrock function from (-1.2, 1).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto rosenbrock = [](const pluss::Series& x) {
    const auto a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
  };
  const auto out = dfo::nelder_mead_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer  : " << out.minimizer.select(0, 0).item<double>() << ", "
            << out.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum    : " << out.minimum << "\n";
  std::cout << "status     : " << (out.status == dfo::SolverStatus::Converged ? "Converged" : "budget") << "\n";
  std::cout << "iterations : " << out.diagnostics.iterations << "\n";
  std::cout << "evaluations: " << out.diagnostics.evaluations << "\n";
  std::cout << "last step  : " << out.diagnostics.last_step << "\n";
}
