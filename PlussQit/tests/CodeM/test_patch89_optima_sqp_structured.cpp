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
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::NonlinearConstraintEvaluation;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaLinearSystemStructure;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::SequentialQPOptions;
using pluss::subsp::math::optim::init_sequential_qp_state;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::solve_sequential_qp;

Series make_vec(std::initializer_list<double> xs) {
  return make_series(std::vector<double>(xs), {static_cast<std::int64_t>(xs.size())})
      .to(ScalarType::Float64)
      .to(pluss::pCPU);
}

} // namespace

TEST_CASE("sequential qp tracks best feasible points using solver feasibility tolerance",
          "[optim][sqp][best-feasible][tolerance]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>&) {
    return std::pair{
        make_series(1.0).to(ScalarType::Float64),
        ParameterPack<Series>({
            {"x", make_vec({0.0, 0.0})},
        })};
  };
  problem.nonlinear.analytic = [](const ParameterPack<Series>&) {
    NonlinearConstraintEvaluation eval;
    eval.equality = make_series(std::vector<double>{1e-8}, {1}).to(ScalarType::Float64);
    eval.inequality = pluss::series::empty({0}, ScalarType::Float64).to(pluss::pCPU);
    eval.equality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({1, 2}, ScalarType::Float64).to(pluss::pCPU)},
    });
    eval.inequality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({0, 2}, ScalarType::Float64).to(pluss::pCPU)},
    });
    return eval;
  };

  const ParameterPack<Series> initial({
      {"x", make_vec({0.0, 0.0})},
  });

  SequentialQPOptions options;
  options.constraint_tolerance = 1e-6;

  const auto state = init_sequential_qp_state(problem, initial, options);

  REQUIRE(state.evaluation.status == EvalStatus::Ok);
  REQUIRE(state.evaluation.max_constraint_violation == Approx(1e-8).margin(1e-12));
  REQUIRE(state.best_feasible.has_value());
}

TEST_CASE("sequential qp can use Krylov routing for sparse-structured KKT solves",
          "[optim][sqp][linear-policy][krylov]") {
  const Series target = make_vec({1.0, 2.0});

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    return ((p.at("x") - target).square()).sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return p.at("x").sum() - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec({0.0, 0.0})},
  });

  const ParameterPack<Series> initial({
      {"x", make_vec({1.0, 1.0})},
  });

  SequentialQPOptions options;
  options.max_iterations = 40;
  options.system_structure = OptimaLinearSystemStructure::Sparse;
  options.linear_system_solver = OptimaLinearSystemSolver::ConjugateGradient;
  options.max_krylov_iterations = 20;
  options.constraint_tolerance = 1e-6;
  options.optimality_tolerance = 1e-5;

  const auto result = solve_sequential_qp(problem, initial, options);
  const auto x = result.params.at("x").host<double>();

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(5e-2));
  REQUIRE(x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(x[1] == Approx(1.25).margin(5e-2));
  // The quadratic subproblem's reduced Newton system is solved by conjugate gradients, and says so.
  REQUIRE(result.converged);
  REQUIRE(result.krylov_iterations > 0);
}

TEST_CASE("sequential qp honours every linear-system solver choice for its subproblem",
          "[optim][sqp][linear-policy]") {
  const Series target = make_vec({1.0, 2.0});
  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    return ((p.at("x") - target).square()).sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series { return p.at("x").sum() - 1.5; };
  problem.bounds.lower = ParameterPack<Series>({{"x", make_vec({0.0, 0.0})}});
  const ParameterPack<Series> initial({{"x", make_vec({1.0, 1.0})}});

  for (auto solver : {OptimaLinearSystemSolver::Auto, OptimaLinearSystemSolver::Cholesky,
                      OptimaLinearSystemSolver::PivotedDirect, OptimaLinearSystemSolver::Inverse,
                      OptimaLinearSystemSolver::PseudoInverse, OptimaLinearSystemSolver::ConjugateGradient}) {
    SequentialQPOptions options;
    options.max_iterations = 40;
    options.linear_system_solver = solver;
    options.constraint_tolerance = 1e-6;
    options.optimality_tolerance = 1e-5;
    const auto result = solve_sequential_qp(problem, initial, options);
    const auto x = result.params.at("x").host<double>();
    REQUIRE(result.converged);
    REQUIRE(x[0] == Approx(0.25).margin(1e-4));
    REQUIRE(x[1] == Approx(1.25).margin(1e-4));
    if (solver == OptimaLinearSystemSolver::ConjugateGradient) {
      REQUIRE(result.krylov_iterations > 0);
    } else {
      REQUIRE(result.krylov_iterations == 0);
    }
  }
}
