// A Wiener path is the object every stochastic integral in this chapter is taken against. simulate_wiener_path
// draws one (or a batch of independent ones) on a uniform grid, starting from zero. The realized quadratic
// variation of such a path, the sum of its squared increments, converges to the elapsed time: this is the
// property that separates stochastic calculus from ordinary calculus.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  // `like` fixes the batch shape and dtype: three independent paths.
  const pluss::Series like = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0}, {3}).to(f64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "steps   path 0   path 1   path 2   (realized quadratic variation on [0, 1])\n";
  for (const std::int64_t steps : {10, 100, 1000, 10000}) {
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, /*seed=*/2024);
    const pluss::Series qv = path.w.antidiff().quadratic_variation(0);
    const auto v = qv.host<double>();
    std::cout << std::setw(5) << steps << "   " << v[0] << "   " << v[1] << "   " << v[2] << "\n";
  }

  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 1000, /*seed=*/2024);
  std::cout << "\nsamples per path: " << path.w.size(0) << " (steps + 1), batch shape [" << path.w.size(1) << "]\n";
  std::cout << "first sample is the start: " << path.w.select(0, 0).host<double>()[0] << "\n";
  return 0;
}
