#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  using namespace pluss;
  using namespace pluss::subsp::math::optim;

  auto make_vec2 = [](double x0, double x1) {
    return pluss::make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
  };

  const Series target = make_vec2(1.0, 2.0);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    const auto err = p.at("x") - target;
    return err.square().sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return p.at("x").sum() - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });

  ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  PenaltyMethodOptions options;
  options.max_iterations = 80;
  options.initial_step_size = 0.2;
  options.penalty = 12.0;
  options.constraint_tolerance = 5e-3;
  options.optimality_tolerance = 5e-3;

  const auto result = solve_penalty_method(problem, initial, options);
  const auto x = result.params.at("x").host<double>();

  std::cout << "iterations=" << result.iteration
            << " f_evals=" << result.function_evaluations
            << " message=" << result.message << '\n';
  std::cout << "x=[" << x[0] << ", " << x[1] << "]"
            << " value=" << result.evaluation.value.item<double>()
            << " max_violation=" << result.evaluation.max_constraint_violation
            << " first_order_opt=" << result.evaluation.first_order_opt << '\n';

  if (result.best_feasible.has_value()) {
    const auto best_x = result.best_feasible->params.at("x").host<double>();
    std::cout << "best_feasible=[" << best_x[0] << ", " << best_x[1] << "]"
              << " value=" << result.best_feasible->value.item<double>() << '\n';
  }

  return 0;
}
