// Watching a run. The progress hook is called after every iteration with a snapshot (objective, gradient, step, violation,
// message) and returns true to continue. The program prints every 150th iteration of an Adam run on the Rosenbrock
// valley and then the iteration at which the objective first fell below 0.01, which a hook can detect and stop on.
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
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.05;
  options.max_iterations = 1500;
  std::int64_t first_below = 0;
  options.progress_hook = [&](const optim::SeptimaProgressSnapshot& s) {
    if (s.iteration % 150 == 0 || s.iteration == 1)
      std::cout << std::setw(6) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient "
                << s.gradient_inf_norm << "  step " << s.step_inf_norm << "\n";
    if (first_below == 0 && s.objective < 0.01) first_below = s.iteration;
    return true;
  };
  const auto r = optim::septima(rosenbrock, optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}}), options);
  std::cout << "the objective first fell below 0.01 at iteration " << first_below << "; the run used " << r.iteration << " iterations\n";
  return 0;
}
