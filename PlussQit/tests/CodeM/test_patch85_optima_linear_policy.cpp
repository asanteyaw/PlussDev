#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaLinearSystemStructure;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::SequentialQPOptions;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::solve_sequential_qp;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

ConstrainedProblem make_sqp_problem() {
  const Series target = make_vec2(1.0, 2.0);
  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) {
    const auto err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return sum(p.at("x")) - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

} // namespace

TEST_CASE("sequential qp can use pivoted direct KKT solves", "[optim][sqp][linear-policy]") {
  const auto problem = make_sqp_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  SequentialQPOptions options;
  options.max_iterations = 40;
  options.linear_system_solver = OptimaLinearSystemSolver::PivotedDirect;
  options.pivot_threshold = 1e-10;

  const auto result = solve_sequential_qp(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(x[1] == Approx(1.25).margin(5e-2));
}

TEST_CASE("interior point auto routing can prefer Krylov for sparse-structured problems",
          "[optim][ip][linear-policy]") {
  const std::int64_t n = 10;
  Series target = pluss::linspace(0.1, 1.0, n, ScalarType::Float64).to(pluss::pCPU);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) -> Trace {
    return sum(square(p.at("x") - trace(target, false)));
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", pluss::zeros({n}, ScalarType::Float64).to(pluss::pCPU)},
  });

  const ParameterPack<Series> initial({
      {"x", pluss::full({n}, 1.5, ScalarType::Float64).to(pluss::pCPU)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 60;
  options.internal_protocol.system_structure = OptimaLinearSystemStructure::Sparse;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::Auto;
  options.internal_protocol.iterative_dimension_threshold = 1000;
  options.internal_protocol.max_krylov_iterations = 25;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.evaluation.value.item<double>() < 1e-2);
  REQUIRE(result.output.krylov_iterations > 0);
}
