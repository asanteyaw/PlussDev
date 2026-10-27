// When a run ends without converging, the last iterate is not always the best point it saw. The solver
// keeps the best feasible point it found; result.best_feasible holds it, and output.best_feasible_*
// report its value, violation and stationarity. returned_best_feasible tells you whether the answer in
// result.params is that checkpoint. Here sequential QP is stopped after three iterations, while its
// iterate is still a hair infeasible.
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
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.max_iterations = 3;
  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << std::boolalpha;
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", converged " << r.converged << "\n";
  std::cout << "returned answer: objective " << r.evaluation.value.item<double>() << ", violation "
            << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible available: " << r.output.best_feasible_available << "\n";
  std::cout << "answer is the checkpoint: " << r.output.returned_best_feasible << "\n";
  if (r.best_feasible) {
    const auto x = r.best_feasible->params.at("x").host<double>();
    std::cout << "checkpoint point [" << x[0] << ", " << x[1] << "]\n";
    std::cout << "checkpoint objective " << r.best_feasible->value.item<double>() << ", violation "
              << r.best_feasible->max_constraint_violation << "\n";
    std::cout << "report matches checkpoint: "
              << (r.output.best_feasible_value == r.best_feasible->value.item<double>()) << "\n";
  }
  return 0;
}
