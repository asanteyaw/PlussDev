// The learning rate sets the size of every Adam step, and it is the setting that matters most. The Rosenbrock valley is
// solved for 2000 iterations with five learning rates spanning three orders of magnitude. Too small and the run is
// still far away when the budget ends; too large and it overshoots the narrow valley and oscillates; between them
// there is a range that works, and the best of them can finish early. Each row is the exit flag, the objective at the
// end and the point.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<optim::Trace(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  };
  const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});

  std::cout << std::left << std::setw(15) << "learning rate" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(12) << "objective"
            << "x\n";
  for (const double rate : {0.001, 0.01, 0.05, 0.2, 1.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.adam.learning_rate = rate;
    const auto r = optim::septima(rosenbrock, start, options);
    std::cout << std::left << std::setw(15) << std::defaultfloat << rate << std::setw(15) << (r.converged ? "Converged" : "MaxIterations")
              << std::setw(8) << r.iteration << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << std::fixed
              << std::setprecision(4) << r.params[0].item<double>() << "\n";
  }
  return 0;
}
