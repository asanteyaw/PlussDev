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
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) -> Trace {
    const auto err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) -> Trace {
    return sum(p.at("x")) - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });

  ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::WorkingSet;
  options.working_set.max_iterations = 40;
  options.working_set.constraint_tolerance = 1e-6;
  options.working_set.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  std::cout << "algorithm=" << result.output.algorithm
            << " iterations=" << result.iteration
            << " f_evals=" << result.function_evaluations
            << " message=" << result.message << '\n';
  std::cout << "x=[" << x[0] << ", " << x[1] << "]"
            << " value=" << result.evaluation.value.item<double>()
            << " max_violation=" << result.evaluation.max_constraint_violation
            << " stationarity=" << result.stationarity << '\n';

  return 0;
}
