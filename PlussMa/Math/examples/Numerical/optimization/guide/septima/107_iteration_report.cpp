// Watching a septima run. `reports::iter` is a ready-made progress hook that prints one line per iteration (or
// every n-th one): the iteration, the objective, the largest gradient entry and the largest step entry. Here L-BFGS
// walks down the Rosenbrock valley, printing every fifth iteration, and the gradient falls by orders of magnitude until
// the run stops.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  const auto start = pluss::make_series(std::vector<double>{-1.2, 1.0}, {2}).to(f64);

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  options.progress_hook = optim::reports::iter(std::cout, 3, 5);

  const auto rosenbrock = [](const pluss::Series& x) {
    const auto a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };
  const auto result = optim::septima(rosenbrock, start, options);
  std::cout << "finished after " << result.iteration << " iterations: " << optim::septima_exit_flag_name(result.exit_flag) << "\n";
  return 0;
}
