// An objective that is undefined in part of the space. f(x) = -x - 0.2 log(1 - x) is defined only for x < 1; its minimum
// is at x = 0.8. A large Adam step (learning rate 1.5) from x = 0 jumps past x = 1, where the logarithm is NaN, and the run ends with
// `NonFiniteEvaluation`. The settings `nan_recovery_attempts` and `nan_recovery_shrink` retry an update that itself produces
// non-finite parameters or optimizer state with a smaller step; they do not detect an objective that is non-finite at the
// new point, so they do not help here. What does help is a bound that keeps every iterate inside the domain: with
// x <= 0.99 the same run converges. A smaller learning rate would also work, but only a bound guarantees it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto objective = [](const pluss::Series& x) -> pluss::Series { return (x * -1.0).sum() - 0.2 * (1.0 - x).log().sum(); };

  std::cout << std::left << std::setw(42) << "setup" << std::setw(22) << "exit" << std::setw(8) << "iters" << "x\n";
  const auto run = [&](const char* label, std::int64_t recovery_attempts, bool bounded) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 800;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 1.5;
    options.step.nan_recovery_attempts = recovery_attempts;
    optim::SeptimaConstraints constraints;
    if (bounded) constraints.ubs = {vec({0.99})};
    const auto r = optim::septima(objective, vec({0.0}), options, constraints);
    const char* exit = r.exit_flag == optim::SeptimaExitFlag::Converged ? "Converged"
                       : r.exit_flag == optim::SeptimaExitFlag::NonFiniteEvaluation ? "NonFiniteEvaluation" : "MaxIterations";
    std::cout << std::left << std::setw(42) << label << std::setw(22) << exit << std::setw(8) << r.iteration << std::fixed << std::setprecision(4)
              << as_f64(r.params[0]) << "\n";
  };
  run("learning rate 1.5, no recovery", 0, false);
  run("learning rate 1.5, 6 recovery attempts", 6, false);
  run("learning rate 1.5, bound x <= 0.99", 2, true);
  return 0;
}
