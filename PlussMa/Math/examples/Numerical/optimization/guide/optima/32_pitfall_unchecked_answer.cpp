// Pitfall: every result carries an answer, even when the solver failed. After an impossible problem
// the answer is just the last point reached. Reading result.params without checking the exit flag
// treats a failure as a solution. Check exit_flag (or converged) first, and use the best feasible
// checkpoint only when output.best_feasible_available says one exists.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  // x0^2 + x1^2 <= -1 has no solution.
  const auto impossible = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, impossible);

  const auto x = r.params.at("x").host<double>();
  std::cout << "unchecked answer: [" << x[0] << ", " << x[1] << "]  (looks like a result)\n";
  std::cout << "converged: " << std::boolalpha << r.converged << ", exit flag " << static_cast<int>(r.exit_flag)
            << ", violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible point exists: " << r.output.best_feasible_available << "\n";
  if (r.exit_flag != optim::OptimaExitFlag::Converged) {
    std::cout << "decision: do not use this answer\n";
  }
  return 0;
}
