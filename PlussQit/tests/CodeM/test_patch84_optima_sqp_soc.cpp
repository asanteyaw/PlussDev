#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <string>
#include <vector>


namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::series::full;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::SequentialQPOptions;
using pluss::subsp::math::optim::init_sequential_qp_state;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::sequential_qp_step;

Series scalar(double value) {
  return full({}, value, ScalarType::Float64).to(pluss::pCPU);
}

Series vec1(double value) {
  return make_series(std::vector<double>{value}, {1}).to(ScalarType::Float64).to(pluss::pCPU);
}

Series vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

} // namespace

TEST_CASE("sequential qp caps the step to remain bound-feasible", "[sqp][bounds][optima]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    return (p.at("x") + 1.0).square();
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", vec1(0.0)},
  });

  const ParameterPack<Series> initial({
      {"x", vec1(0.2)},
  });

  SequentialQPOptions options;
  options.initial_step_size = 10.0;
  options.max_line_search_iterations = 1;
  options.fraction_to_boundary = 0.995;

  const auto state = init_sequential_qp_state(problem, initial, options);
  const auto next = sequential_qp_step(problem, state, options);
  const double x = next.params.at("x").host<double>()[0];

  REQUIRE(next.iteration == state.iteration + 1);
  REQUIRE(next.last_step_size < 1.0);
  REQUIRE(x >= 0.0);
  REQUIRE(x == Approx(0.001).margin(5e-3));
}

TEST_CASE("sequential qp can accept a second-order correction on a curved inequality", "[sqp][soc][optima]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    return (p.at("x0") - 1.0).square() + p.at("x1").square();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return p.at("x0").square() - p.at("x1");
  };

  const ParameterPack<Series> initial({
      {"x0", scalar(0.2)},
      {"x1", scalar(0.04)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.sequential_qp.max_line_search_iterations = 20;
  options.sequential_qp.max_second_order_corrections = 2;
  options.sequential_qp.initial_step_size = 1.0;
  options.sequential_qp.active_set_tolerance = 5e-2;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(as_f64(result.params.at("x1")) + 1e-4 >=
          as_f64(result.params.at("x0")) * as_f64(result.params.at("x0")));
  const bool terminated_normally = result.exit_flag == pluss::subsp::math::optim::OptimaExitFlag::Converged ||
                                   result.exit_flag == pluss::subsp::math::optim::OptimaExitFlag::MaxIterations;
  REQUIRE(terminated_normally);
  // minimizer of (x0-1)^2 + x0^4: 2(x0-1) + 4 x0^3 = 0
  REQUIRE(as_f64(result.params.at("x0")) == Approx(0.5898).margin(2e-3));
}
