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
using pluss::optim::ConstrainedProblem;
using pluss::optim::EvalStatus;
using pluss::optim::OptimaAlgorithm;
using pluss::optim::OptimaExitFlag;
using pluss::optim::OptimaOptions;
using pluss::optim::ParameterPack;
using pluss::optim::Trace;
using pluss::optim::optima;
using pluss::optim::square;
using pluss::optim::sum;
using pluss::optim::trace;

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
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) -> Trace {
    const Trace x = p.at("x");
    const Trace diff = x - trace(target, false);
    return sum(square(diff));
  };

  std::vector<double> aeq(static_cast<std::size_t>(n), 1.0);
  std::vector<double> target_vec = target.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
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
  problem.bounds.lower = pluss::optim::pack_series(pluss::zeros({n}, pluss::Config{}.with_dtype(ScalarType::Float64)));
  problem.bounds.upper = pluss::optim::pack_series(pluss::ones({n}, pluss::Config{}.with_dtype(ScalarType::Float64)));
  return problem;
}

} // namespace

TEST_CASE("Optima working set remains robust on structured active-constraint families", "[optima][working_set][stress]") {
  constexpr std::int64_t n = 24;
  const Series target =
      pluss::linspace(0.18, 0.72, n, pluss::Config{}.with_dtype(ScalarType::Float64)).reshape({n});

  std::vector<double> initial_values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    initial_values[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.95 : 0.05;
  }
  const Series initial = pluss::make_series(initial_values, {n}).to(ScalarType::Float64);

  const ConstrainedProblem problem = make_structured_working_set_problem(n, target, 0.16);

  OptimaOptions sqp = pluss::optim::presets::sequential_qp();
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
  REQUIRE(ws_result.output.active_multiplier_count >= 1);
  REQUIRE(ws_result.evaluation.max_constraint_violation <= sqp_result.evaluation.max_constraint_violation + 1e-6);
  REQUIRE(as_f64_scalar(ws_result.evaluation.value) <= as_f64_scalar(sqp_result.evaluation.value) + 1e-5);
}

TEST_CASE("Internal protocol feasibility mode recovers from infeasible starts", "[optima][internal_protocol][feasibility]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) -> Trace {
    const Trace x = p.at("x");
    const Trace center = trace(pluss::make_series(std::vector<double>{0.2, 0.2}, {2}).to(ScalarType::Float64), false);
    return sum(square(x - center));
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) -> Trace {
    const Trace x = p.at("x");
    const Trace total = sum(x);
    return 1.2 - total;
  };
  problem.bounds.lower = pluss::optim::pack_series(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::optim::pack_series(pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.1, 0.1}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::optim::presets::internal_protocol();
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
  problem.objective.autodiff = [weights](const ParameterPack<Trace>& p) -> Trace {
    const Trace x = p.at("x");
    return sum(trace(weights, false) * square(x));
  };
  problem.linear.Aeq = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(ScalarType::Float64);
  problem.linear.beq = pluss::make_series(std::vector<double>{1.0}, {1}).to(ScalarType::Float64);
  problem.bounds.lower = pluss::optim::pack_series(pluss::make_series(std::vector<double>{-2.0, -2.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::optim::pack_series(pluss::make_series(std::vector<double>{2.0, 2.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.8, 0.2}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 200;
  options.internal_protocol.constraint_tolerance = 1e-10;
  options.internal_protocol.optimality_tolerance = 1e-8;
  options.internal_protocol.problem_scaling = pluss::optim::OptimaProblemScaling::ObjectiveAndConstraints;

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
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) -> Trace {
    const Trace x = p.at("x");
    return sum(square(x));
  };
  problem.bounds.lower = pluss::optim::pack_series(pluss::make_series(std::vector<double>{-1.0, -1.0}, {2}).to(ScalarType::Float64));
  problem.bounds.upper = pluss::optim::pack_series(pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(ScalarType::Float64));

  const Series initial = pluss::make_series(std::vector<double>{0.2, -0.3}, {2}).to(ScalarType::Float64);
  OptimaOptions options = pluss::optim::presets::internal_protocol();
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
