#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>


namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::optima;

double as_f64_scalar(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Series scalar(double value) {
  return pluss::make_series(value).to(ScalarType::Float64).reshape({1});
}

ConstrainedProblem make_structured_working_set_problem(std::int64_t n,
                                                       const Series& target,
                                                       double difference_cap) {
  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    const Series diff = x - target;
    return diff.square().sum();
  };

  std::vector<double> aeq(static_cast<std::size_t>(n), 1.0);
  std::vector<double> target_vec = target.to(pluss::pCPU, ScalarType::Float64).host<double>();
  double sum_target = 0.0;
  for (double t : target_vec) sum_target += t;
  problem.linear.Aeq = pluss::make_series(aeq, {1, n}).to(ScalarType::Float64);
  problem.linear.beq = pluss::make_series(std::vector<double>{sum_target}, {1}).to(ScalarType::Float64);

  std::vector<double> aineq(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
  std::vector<double> bineq(static_cast<std::size_t>(2 * (n - 1)), difference_cap);
  for (std::int64_t i = 0; i < n - 1; ++i) {
    const std::int64_t row0 = 2 * i;
    const std::int64_t row1 = row0 + 1;
    aineq[static_cast<std::size_t>(row0 * n + i)] = 1.0;
    aineq[static_cast<std::size_t>(row0 * n + i + 1)] = -1.0;
    aineq[static_cast<std::size_t>(row1 * n + i)] = -1.0;
    aineq[static_cast<std::size_t>(row1 * n + i + 1)] = 1.0;
  }
  problem.linear.Aineq = pluss::make_series(aineq, {2 * (n - 1), n}).to(ScalarType::Float64);
  problem.linear.bineq = pluss::make_series(bineq, {2 * (n - 1)}).to(ScalarType::Float64);
  problem.bounds.lower = pluss::subsp::math::optim::pack_series(pluss::series::zeros({n}, pluss::Config{}.with_dtype(ScalarType::Float64)));
  problem.bounds.upper = pluss::subsp::math::optim::pack_series(pluss::series::ones({n}, pluss::Config{}.with_dtype(ScalarType::Float64)));
  return problem;
}

} // namespace

TEST_CASE("Optima working set remains robust on structured active-constraint families", "[optima][working_set][stress]") {
  constexpr std::int64_t n = 24;
  const Series target =
      pluss::series::linspace(0.18, 0.72, n, pluss::Config{}.with_dtype(ScalarType::Float64)).reshape({n});

  std::vector<double> initial_values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    initial_values[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.95 : 0.05;
  }
  const Series initial = pluss::make_series(initial_values, {n}).to(ScalarType::Float64);

  const ConstrainedProblem problem = make_structured_working_set_problem(n, target, 0.16);

  OptimaOptions sqp = pluss::subsp::math::optim::presets::sequential_qp();
  sqp.algorithm = OptimaAlgorithm::SequentialQP;
  sqp.sequential_qp.max_iterations = 220;
  sqp.sequential_qp.constraint_tolerance = 1e-8;
  sqp.sequential_qp.optimality_tolerance = 1e-6;

  OptimaOptions ws = sqp;
  ws.algorithm = OptimaAlgorithm::WorkingSet;
  ws.working_set.max_iterations = 220;
  ws.working_set.max_working_set_violations = 6;
  ws.working_set.add_constraint_tolerance = 5e-4;
  ws.working_set.multiplier_smoothing = 0.7;

  const auto sqp_result = optima(problem, initial, sqp);
  const auto ws_result = optima(problem, initial, ws);

  INFO("sqp_exit=" << static_cast<int>(sqp_result.exit_flag)
                   << " ws_exit=" << static_cast<int>(ws_result.exit_flag)
                   << " sqp_viol=" << sqp_result.evaluation.max_constraint_violation
                   << " ws_viol=" << ws_result.evaluation.max_constraint_violation
                   << " sqp_obj=" << as_f64_scalar(sqp_result.evaluation.value)
                   << " ws_obj=" << as_f64_scalar(ws_result.evaluation.value));

  REQUIRE(ws_result.evaluation.status == EvalStatus::Ok);
  REQUIRE(std::isfinite(as_f64_scalar(ws_result.evaluation.value)));
  REQUIRE(ws_result.evaluation.max_constraint_violation <= 1e-6);
  // The target lies inside every constraint, so the exact optimum has no active constraint.
  REQUIRE(as_f64_scalar(ws_result.evaluation.value) <= 1e-10);
  REQUIRE(ws_result.evaluation.max_constraint_violation <= sqp_result.evaluation.max_constraint_violation + 1e-6);
  REQUIRE(as_f64_scalar(ws_result.evaluation.value) <= as_f64_scalar(sqp_result.evaluation.value) + 1e-5);
}

TEST_CASE("Internal protocol feasibility mode recovers from infeasible starts", "[optima][internal_protocol][feasibility]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    const Series center = (pluss::make_series(std::vector<double>{0.2, 0.2}, {2}).to(ScalarType::Float64));
    return (x - center).square().sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    const Series total = x.sum();
    return 1.2 - total;
  };
  problem.bounds.lower = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.1, 0.1}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::subsp::math::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 160;
  options.internal_protocol.enable_feasibility_phase = true;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-6;

  const auto result = optima(problem, initial, options);
  const Series x = result.params.at("x").reshape({2});
  const double sum_x = as_f64_scalar(x.sum());

  INFO("exit=" << static_cast<int>(result.exit_flag)
               << " violation=" << result.evaluation.max_constraint_violation
               << " sum_x=" << sum_x
               << " detail=" << result.output.detail_message);

  REQUIRE(result.evaluation.status == EvalStatus::Ok);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-6);
  REQUIRE(sum_x >= Approx(1.2).margin(1e-5));
  REQUIRE(result.exit_flag != OptimaExitFlag::InvalidProblem);
  REQUIRE(result.exit_flag != OptimaExitFlag::NonFiniteEvaluation);
}

TEST_CASE("Internal protocol remains numerically stable on ill-conditioned scaled constraints", "[optima][internal_protocol][numerical_stability]") {
  ConstrainedProblem problem;
  const Series weights = pluss::make_series(std::vector<double>{1e6, 1e-6}, {2}).to(ScalarType::Float64);
  problem.objective.autodiff = [weights](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    return (weights * x.square()).sum();
  };
  problem.linear.Aeq = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(ScalarType::Float64);
  problem.linear.beq = pluss::make_series(std::vector<double>{1.0}, {1}).to(ScalarType::Float64);
  problem.bounds.lower = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{-2.0, -2.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{2.0, 2.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.8, 0.2}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::subsp::math::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 200;
  options.internal_protocol.constraint_tolerance = 1e-10;
  options.internal_protocol.optimality_tolerance = 1e-8;
  options.internal_protocol.problem_scaling = pluss::subsp::math::optim::OptimaProblemScaling::ObjectiveAndConstraints;

  const auto result = optima(problem, initial, options);
  const Series x = result.params.at("x").reshape({2});
  const double x0 = as_f64_scalar(x.select(0, 0));
  const double x1 = as_f64_scalar(x.select(0, 1));

  INFO("exit=" << static_cast<int>(result.exit_flag)
               << " objective=" << as_f64_scalar(result.evaluation.value)
               << " violation=" << result.evaluation.max_constraint_violation
               << " first_order=" << result.output.first_order_optimality
               << " x0=" << x0
               << " x1=" << x1);

  REQUIRE(result.evaluation.status == EvalStatus::Ok);
  REQUIRE(std::isfinite(as_f64_scalar(result.evaluation.value)));
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-8);
  REQUIRE(std::isfinite(result.output.first_order_optimality));
  REQUIRE(std::abs(x0) <= 1e-2);
  REQUIRE(x1 == Approx(1.0).margin(1e-2));
}

TEST_CASE("Internal protocol does not enter feasibility mode when already feasible", "[optima][internal_protocol][regression]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    return x.square().sum();
  };
  problem.bounds.lower = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{-1.0, -1.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.2, -0.3}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::subsp::math::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 20;
  options.internal_protocol.max_line_search_iterations = 0; // force no accepted LS step
  options.internal_protocol.enable_restoration = false;     // force direct fallback path
  options.internal_protocol.enable_feasibility_phase = true;
  options.internal_protocol.constraint_tolerance = 1e-8;

  const auto result = optima(problem, initial, options);
  const bool mentions_feasibility =
      result.message.find("feasibility phase") != std::string::npos ||
      result.output.detail_message.find("feasibility phase") != std::string::npos;

  INFO("exit=" << static_cast<int>(result.exit_flag)
               << " violation=" << result.evaluation.max_constraint_violation
               << " message=" << result.message
               << " detail=" << result.output.detail_message);

  REQUIRE(result.evaluation.status == EvalStatus::Ok);
  REQUIRE(result.evaluation.max_constraint_violation <= options.internal_protocol.constraint_tolerance);
  REQUIRE_FALSE(mentions_feasibility);
}

TEST_CASE("Optima autodiff stays connected through staged copy_ objectives", "[optima][autodiff][inplace]") {
  ConstrainedProblem problem;
  const Series target = pluss::make_series(std::vector<double>{0.4, -0.6, 0.8}, {3}).to(ScalarType::Float64);
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    const Series x = p.at("x");
    const Series xv = x;
    Series staged = pluss::series::zeros_like(xv);
    for (std::int64_t i = 0; i < xv.size(0); ++i) {
      staged.select(0, i).copy_(xv.select(0, i));
    }
    const Series diff = staged - target;
    return diff.square().sum();
  };
  problem.bounds.lower = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{-2.0, -2.0, -2.0}, {3}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::subsp::math::optim::pack_series(pluss::make_series(std::vector<double>{2.0, 2.0, 2.0}, {3}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{1.4, -1.2, 1.1}, {3}).to(ScalarType::Float64);
  OptimaOptions options = pluss::subsp::math::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-6;

  const auto result = optima(problem, initial, options);

  INFO("exit=" << static_cast<int>(result.exit_flag)
               << " status=" << static_cast<int>(result.evaluation.status)
               << " objective=" << as_f64_scalar(result.evaluation.value)
               << " first_order=" << result.output.first_order_optimality
               << " detail=" << result.output.detail_message);

  REQUIRE(result.evaluation.status == EvalStatus::Ok);
  REQUIRE(std::isfinite(as_f64_scalar(result.evaluation.value)));
  REQUIRE(std::isfinite(result.output.first_order_optimality));
}

TEST_CASE("analytic nonlinear callbacks may leave an unused residual kind undefined", "[optima][analytic]") {
  // minimize x^2 + y^2 subject to 4 - x*y <= 0; the callback defines only the inequality.
  const auto scalar = [](double v) { return pluss::make_series(v).to(ScalarType::Float64); };
  const auto pair = [&](double x, double y) { return pluss::subsp::math::optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); };
  const auto block = [&](double v) { return pluss::make_series(std::vector<double>{v}, {1, 1}).to(ScalarType::Float64); };

  for (const auto algorithm :
       {OptimaAlgorithm::SequentialQP, OptimaAlgorithm::WorkingSet, OptimaAlgorithm::InternalProtocol}) {
    ConstrainedProblem problem;
    problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
      return p.at("x").square() + p.at("y").square();
    };
    problem.nonlinear.analytic = [&](const ParameterPack<Series>& p) {
      const double x = as_f64(p.at("x")), y = as_f64(p.at("y"));
      pluss::subsp::math::optim::NonlinearConstraintEvaluation e;
      e.inequality = pluss::make_series(std::vector<double>{4.0 - x * y}, {1}).to(ScalarType::Float64);
      e.inequality_jacobian = ParameterPack<Series>(std::vector<ParameterPack<Series>::Block>{{"x", block(-y)}, {"y", block(-x)}});
      return e;
    };
    problem.bounds.lower = pair(0.1, 0.1);
    problem.bounds.upper = pair(10, 10);
    OptimaOptions options;
    options.algorithm = algorithm;
    const auto result = optima(problem, pair(1, 3), options);
    REQUIRE(result.exit_flag == OptimaExitFlag::Converged);
    REQUIRE(as_f64(result.params.at("x")) == Approx(2.0).margin(1e-4));
    REQUIRE(as_f64(result.params.at("y")) == Approx(2.0).margin(1e-4));
  }
}

TEST_CASE("the elastic QP solver recovers when its default start stalls", "[optima][qp]") {
  // A step with two boxed variables whose bounds are far from the iterate: the minimizer of the model is the interior
  // point d = -g (curvature 1), every bound is slack and every elastic variable is zero. With the default start the
  // interior iteration cycled for penalty weights between 100 and 10^6 and returned a wrong step.
  using pluss::subsp::math::optim::detail::QpLinearSolverSettings;
  using pluss::subsp::math::optim::detail::solve_elastic_qp_;
  const std::vector<double> B = {1, 0, 0, 1}, g = {-11.39554, -0.61824};
  const std::vector<double> Ai = {-1, 0, 0, -1, 1, 0, 0, 1}, c = {-0.9, -49.0, -19.0, -950.0}, Ae, h;
  for (const double rho : {10.0, 100.0, 1e3, 1e4, 1e5, 1e6}) {
    const auto result = solve_elastic_qp_(B, g, Ae, h, Ai, c, rho, QpLinearSolverSettings{});
    INFO("rho = " << rho);
    REQUIRE(result.ok);
    REQUIRE(result.d[0] == Approx(11.39554).margin(1e-6));
    REQUIRE(result.d[1] == Approx(0.61824).margin(1e-6));
    REQUIRE(result.elastic_max < 1e-7);
  }
}

TEST_CASE("Optima exit flags have readable names", "[optima][names]") {
  using pluss::subsp::math::optim::optima_exit_flag_name;
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::Converged) == "Converged");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::MaxIterations) == "MaxIterations");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::Stopped) == "Stopped");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::NoFeasiblePoint) == "NoFeasiblePoint");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::ObjectiveTargetReached) == "ObjectiveTargetReached");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::NonFiniteEvaluation) == "NonFiniteEvaluation");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::InvalidProblem) == "InvalidProblem");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::NoProgress) == "NoProgress");
  REQUIRE(optima_exit_flag_name(OptimaExitFlag::ConstraintStationary) == "ConstraintStationary");
}
