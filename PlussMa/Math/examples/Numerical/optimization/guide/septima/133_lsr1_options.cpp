// The L-SR1 settings that matter. `memory_size` is how many step pairs the curvature model keeps: too few and the model is
// too poor to follow the valley. `initial_radius` is the starting size of the trust region, which the method then adapts.
// Both are varied on the ten-variable chained Rosenbrock function. The last block prints the first twelve iterations
// through the progress hook, which reports the trust radius and the length of each step: the radius shrinks when the model
// predicted badly and grows when it predicted well.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const pluss::Series& x) -> pluss::Series {
    const pluss::Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  std::cout << std::left << std::setw(22) << "setting" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  const auto row = [&](const std::string& label, const optim::Lsr1SolveOptions& options) {
    const auto r = optim::l_sr1_trust_region(chain, vec(start), options);
    std::cout << std::left << std::setw(22) << label << std::setw(15) << optim::lsr1_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  };
  for (const std::int64_t memory : {1, 3, 10, 30}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.memory_size = memory;
    row("memory " + std::to_string(memory), options);
  }
  for (const double radius : {0.01, 1.0, 100.0}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.initial_radius = radius;
    std::ostringstream label;
    label << "initial radius " << radius;
    row(label.str(), options);
  }

  std::cout << "\nthe first twelve iterations\n";
  optim::Lsr1SolveOptions options;
  options.max_iterations = 12;
  options.progress_hook = [](const optim::Lsr1ProgressSnapshot& s) {
    std::cout << std::right << std::setw(4) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient " << s.gradient_norm
              << std::fixed << std::setprecision(3) << "  radius " << s.trust_radius << "  step " << s.step_norm << "\n";
    return true;
  };
  optim::l_sr1_trust_region(chain, vec(start), options);
  return 0;
}
