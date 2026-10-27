// Bounds are the simplest constraint. After every step Septima clips each parameter into its bounds, so the iterates are
// always inside the box and a bound never produces a violation. The objective is 0.5 * |x - t|^2 with the target
// t = (2, 1, -1) and the box [0, 1.5] on every component, so the answer clips each coordinate of t into the box:
// (1.5, 1, 0). A bound is given per block, in block order, as a `Series` of the same shape as the block. The table
// compares the two optimizers; `gradient` is the objective's own gradient and `stationarity` is what the solver tests.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0, -1.0});
  auto objective = [&](const pluss::Series& x) -> pluss::Series { return (x - target).square().sum() * 0.5; };

  optim::SeptimaConstraints box;
  box.lbs = {vec({0.0, 0.0, 0.0})};
  box.ubs = {vec({1.5, 1.5, 1.5})};

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(11) << "exit" << std::setw(7) << "iters" << std::setw(24) << "x" << std::setw(11)
            << "gradient" << "stationarity\n";
  for (const auto optimizer : {optim::SeptimaOptimizer::Adam, optim::SeptimaOptimizer::Lbfgs}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optimizer;
    options.step.adam.learning_rate = 0.05;
    options.max_iterations = 2000;
    // Start outside the box: the first step already brings it back in.
    const auto r = optim::septima(objective, vec({3.0, -2.0, 0.5}), options, box);
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>() << ", "
      << r.params[0].select(0, 2).item<double>();
    std::cout << std::left << std::setw(10) << (optimizer == optim::SeptimaOptimizer::Adam ? "Adam" : "L-BFGS") << std::setw(11)
              << (r.converged ? "Converged" : "not converged") << std::setw(7) << r.iteration << std::setw(24) << x.str() << std::scientific
              << std::setprecision(1) << std::setw(11) << r.gradient_inf_norm << r.stationarity << "\n";
  }
  return 0;
}
