#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaLinearSystemStructure;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::WorkingSetOptions;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::solve_working_set;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series scalar(double v) {
  return make_series(v).to(ScalarType::Float64);
}

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::string var_name(std::int64_t i) {
  return "x_" + std::to_string(i);
}

ParameterPack<Series> make_scalar_blocks(const std::vector<double>& values) {
  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    blocks.push_back({var_name(static_cast<std::int64_t>(i)), scalar(values[i]).reshape({1})});
  }
  return ParameterPack<Series>(std::move(blocks));
}

std::vector<double> make_target_profile(std::int64_t n) {
  std::vector<double> target(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    const double phase = static_cast<double>(i) / std::max<std::int64_t>(n - 1, 1);
    target[static_cast<std::size_t>(i)] = 0.18 + 0.52 * phase + 0.05 * std::sin(3.141592653589793 * phase);
  }
  return target;
}

std::vector<double> make_infeasible_start(std::int64_t n) {
  std::vector<double> x(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) x[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.95 : 0.05;
  return x;
}

ConstrainedProblem make_problem(std::int64_t n,
                                const std::vector<double>& target,
                                double difference_cap) {
  ConstrainedProblem problem;
  problem.objective.autodiff = [target, n](const ParameterPack<Trace>& p) -> Trace {
    Trace value = trace(scalar(0.0), false);
    for (std::int64_t i = 0; i < n; ++i) {
      const Trace xi = p.at(var_name(i));
      value = value + square(xi - target[static_cast<std::size_t>(i)]);
      if (i > 0) value = value + 0.15 * square(xi - p.at(var_name(i - 1)));
    }
    return value;
  };

  double sum_target = 0.0;
  for (double t : target) sum_target += t;
  problem.linear.Aeq = make_series(std::vector<double>(static_cast<std::size_t>(n), 1.0), {1, n}).to(ScalarType::Float64);
  problem.linear.beq = make_series(std::vector<double>{sum_target}, {1}).to(ScalarType::Float64);

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
  problem.linear.Aineq = make_series(aineq, {2 * (n - 1), n}).to(ScalarType::Float64);
  problem.linear.bineq = make_series(bineq, {2 * (n - 1)}).to(ScalarType::Float64);

  std::vector<typename ParameterPack<Series>::Block> lower_blocks;
  std::vector<typename ParameterPack<Series>::Block> upper_blocks;
  lower_blocks.reserve(static_cast<std::size_t>(n));
  upper_blocks.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    lower_blocks.push_back({var_name(i), scalar(0.0).reshape({1})});
    upper_blocks.push_back({var_name(i), scalar(1.0).reshape({1})});
  }
  problem.bounds.lower = ParameterPack<Series>(std::move(lower_blocks));
  problem.bounds.upper = ParameterPack<Series>(std::move(upper_blocks));
  return problem;
}

double max_abs_error(const ParameterPack<Series>& params, const std::vector<double>& target) {
  double best = 0.0;
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(target.size()); ++i) {
    best = std::max(best, std::abs(as_f64(params.at(var_name(i))) - target[static_cast<std::size_t>(i)]));
  }
  return best;
}

} // namespace

TEST_CASE("working set converges on a structured sparse family", "[optim][working-set][structured]") {
  const std::int64_t n = 16;
  const auto target = make_target_profile(n);
  const auto problem = make_problem(n, target, 0.12);
  const auto initial = make_scalar_blocks(make_infeasible_start(n));

  WorkingSetOptions options;
  options.max_iterations = 300;
  options.constraint_tolerance = 1e-8;
  options.optimality_tolerance = 1e-6;
  options.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
  options.system_structure = OptimaLinearSystemStructure::Sparse;
  options.max_working_set_violations = 4;
  options.add_constraint_tolerance = 5e-4;

  const auto result = solve_working_set(problem, initial, options);

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-6);
  REQUIRE(result.evaluation.value.item<double>() == Approx(0.0028).margin(1e-3));
  REQUIRE(max_abs_error(result.params, target) <= 1e-2);
  REQUIRE(result.best_feasible.has_value());
}

TEST_CASE("optima dispatch exposes working set as a distinct backend", "[optim][working-set][dispatch]") {
  const std::int64_t n = 16;
  const auto target = make_target_profile(n);
  const auto problem = make_problem(n, target, 0.12);
  const auto initial = make_scalar_blocks(make_infeasible_start(n));

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::WorkingSet;
  options.working_set.max_iterations = 300;
  options.working_set.constraint_tolerance = 1e-8;
  options.working_set.optimality_tolerance = 1e-6;
  options.working_set.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
  options.working_set.system_structure = OptimaLinearSystemStructure::Sparse;
  options.working_set.max_working_set_violations = 4;
  options.working_set.add_constraint_tolerance = 5e-4;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.algorithm == OptimaAlgorithm::WorkingSet);
  REQUIRE(result.output.algorithm == "working-set");
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-6);
  REQUIRE(result.output.best_feasible_available);
  REQUIRE(result.output.detail_message.find("working set") != std::string::npos);
}
