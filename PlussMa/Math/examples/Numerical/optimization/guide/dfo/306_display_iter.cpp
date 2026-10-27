// Watching a derivative-free run. `display_iter` makes the solver print one line per iteration to standard output: the
// iteration, the evaluations used so far, the best value found and the phase the method was in. The same switch exists on
// `DfoBuilder` as `with_display_iter()`. The run is capped at 12 iterations so that the listing stays short.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const auto start = pluss::make_series(std::vector<double>{4.0, 4.0}, {2}).to(f64);
  const auto objective = [&](const pluss::Series& x) {
    return (x - pluss::make_series(std::vector<double>{1.0, -2.0}, {2}).to(f64)).square().sum() + 3.0;
  };

  dfo::DfoOptions options;
  options.display_iter = true;
  options.max_iterations = 12;
  const auto result = dfo::solve_dfo(objective, start, options);
  std::cout << "status " << dfo::to_string(result.status) << ", minimum " << result.minimum << ", " << result.diagnostics.evaluations << " evaluations\n";
  return 0;
}
