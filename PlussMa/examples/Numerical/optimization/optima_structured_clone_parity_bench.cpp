#include <pluss/pluss.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaHessianApproximation;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaLinearSystemStructure;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::optima_stall_category_name;

struct StructuredBenchmarkRow {
  std::string family{};
  std::int64_t variables{0};
  std::string solver{};
  std::string exit{};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double constraint_violation{std::numeric_limits<double>::infinity()};
  double first_order_optimality{std::numeric_limits<double>::infinity()};
  double complementarity{0.0};
  std::string stall{};
  double wall_time_ms{0.0};
  double max_abs_error{std::numeric_limits<double>::infinity()};
  bool best_feasible_available{false};
  std::string note{};
};

std::string var_name(std::int64_t i) {
  return "x_" + std::to_string(i);
}

Series scalar(double v) {
  return pluss::make_series(v).to(ScalarType::Float64);
}

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

ParameterPack<Series> make_scalar_blocks(const std::vector<double>& values) {
  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    blocks.push_back({var_name(static_cast<std::int64_t>(i)), scalar(values[i]).reshape({1})});
  }
  return ParameterPack<Series>(std::move(blocks));
}

std::vector<double> unpack_scalar_blocks(const ParameterPack<Series>& params) {
  std::vector<double> out;
  out.reserve(params.size());
  for (const auto& block : params.blocks()) {
    out.push_back(as_f64(block.value));
  }
  return out;
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
  for (std::int64_t i = 0; i < n; ++i) {
    x[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.95 : 0.05;
  }
  return x;
}

ConstrainedProblem make_structured_problem(std::int64_t n,
                                           const std::vector<double>& target,
                                           double difference_cap) {
  ConstrainedProblem problem;

  problem.objective.autodiff =
      [target, n](const ParameterPack<Trace>& p) -> Trace {
    Trace value = pluss::subsp::math::optim::trace(scalar(0.0), false);
    for (std::int64_t i = 0; i < n; ++i) {
      const Trace xi = p.at(var_name(i));
      const double ti = target[static_cast<std::size_t>(i)];
      value = value + square(xi - ti);
      if (i > 0) {
        const Trace xim1 = p.at(var_name(i - 1));
        value = value + 0.15 * square(xi - xim1);
      }
    }
    return value;
  };

  double sum_target = 0.0;
  for (double t : target) sum_target += t;
  std::vector<double> aeq(static_cast<std::size_t>(n), 1.0);
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

const char* to_string(OptimaExitFlag flag) {
  switch (flag) {
    case OptimaExitFlag::Converged:
      return "Converged";
    case OptimaExitFlag::MaxIterations:
      return "MaxIterations";
    case OptimaExitFlag::Stopped:
      return "Stopped";
    case OptimaExitFlag::NoFeasiblePoint:
      return "NoFeasiblePoint";
    case OptimaExitFlag::ObjectiveTargetReached:
      return "ObjectiveTargetReached";
    case OptimaExitFlag::NonFiniteEvaluation:
      return "NonFiniteEvaluation";
    case OptimaExitFlag::InvalidProblem:
      return "InvalidProblem";
    case OptimaExitFlag::NoProgress:
      return "NoProgress";
    case OptimaExitFlag::ConstraintStationary:
      return "ConstraintStationary";
  }
  return "Unknown";
}

double max_abs_error_to_target(const ParameterPack<Series>& params, const std::vector<double>& target) {
  const auto x = unpack_scalar_blocks(params);
  double best = 0.0;
  for (std::size_t i = 0; i < x.size() && i < target.size(); ++i) {
    best = std::max(best, std::abs(x[i] - target[i]));
  }
  return best;
}

StructuredBenchmarkRow run_structured_case(std::int64_t n,
                                           std::string solver,
                                           const ConstrainedProblem& problem,
                                           const ParameterPack<Series>& initial,
                                           const std::vector<double>& target,
                                           const OptimaOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = optima(problem, initial, options);
  const auto stop = std::chrono::steady_clock::now();

  StructuredBenchmarkRow row;
  row.family = "BandedProfile";
  row.variables = n;
  row.solver = std::move(solver);
  row.exit = to_string(result.exit_flag);
  row.iterations = result.iteration;
  row.evaluations = result.function_evaluations;
  row.objective = result.evaluation.value.item<double>();
  row.constraint_violation = result.evaluation.max_constraint_violation;
  row.first_order_optimality = result.output.first_order_optimality;
  row.complementarity = result.output.complementarity;
  row.stall = std::string(optima_stall_category_name(result.output.stall_category));
  row.wall_time_ms = std::chrono::duration<double, std::milli>(stop - start).count();
  row.max_abs_error = max_abs_error_to_target(result.params, target);
  row.best_feasible_available = result.output.best_feasible_available;
  row.note = result.output.detail_message;
  return row;
}

void print_table(const std::vector<StructuredBenchmarkRow>& rows) {
  std::cout << "| Family | n | Solver | Exit | Stall | Iter | Eval | Objective | Constr viol | First-order | Complementarity | Max |x-target| | Time ms | Best feasible |\n";
  std::cout << "|---|---:|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|\n";
  for (const auto& row : rows) {
    std::cout << "| " << row.family
              << " | " << row.variables
              << " | " << row.solver
              << " | " << row.exit
              << " | " << row.stall
              << " | " << row.iterations
              << " | " << row.evaluations
              << " | " << row.objective
              << " | " << row.constraint_violation
              << " | " << row.first_order_optimality
              << " | " << row.complementarity
              << " | " << row.max_abs_error
              << " | " << row.wall_time_ms
              << " | " << (row.best_feasible_available ? "yes" : "no")
              << " |\n";
  }
}

void print_notes(const std::vector<StructuredBenchmarkRow>& rows) {
  std::cout << "\nNotes:\n";
  for (const auto& row : rows) {
    std::cout << "- n=" << row.variables << " " << row.solver << ": " << row.note << "\n";
  }
}

} // namespace

int main() {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "# Optima Clone-Parity Benchmark: Structured Banded Family\n\n";

  std::vector<StructuredBenchmarkRow> rows;
  for (const std::int64_t n : {16, 32}) {
    const auto target = make_target_profile(n);
    const auto initial_values = make_infeasible_start(n);
    const auto initial = make_scalar_blocks(initial_values);
    const auto problem = make_structured_problem(n, target, 0.12);

    OptimaOptions sqp_options;
    sqp_options.algorithm = OptimaAlgorithm::SequentialQP;
    sqp_options.sequential_qp.max_iterations = 120;
    sqp_options.sequential_qp.constraint_tolerance = 1e-8;
    sqp_options.sequential_qp.optimality_tolerance = 1e-6;
    sqp_options.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    sqp_options.sequential_qp.system_structure = OptimaLinearSystemStructure::Sparse;
    sqp_options.sequential_qp.linear_system_solver = OptimaLinearSystemSolver::Auto;
    sqp_options.sequential_qp.hessian_approximation = OptimaHessianApproximation::Bfgs;

    OptimaOptions ip_options;
    ip_options.algorithm = OptimaAlgorithm::InteriorPoint;
    ip_options.interior_point.max_iterations = 200;
    ip_options.interior_point.constraint_tolerance = 1e-8;
    ip_options.interior_point.optimality_tolerance = 1e-6;
    ip_options.interior_point.initial_barrier_parameter = 1e-2;
    ip_options.interior_point.minimum_barrier_parameter = 1e-8;
    ip_options.interior_point.enable_feasibility_phase = true;
    ip_options.interior_point.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    ip_options.interior_point.system_structure = OptimaLinearSystemStructure::Auto;
    ip_options.interior_point.linear_system_solver = OptimaLinearSystemSolver::Auto;
    ip_options.interior_point.hessian_approximation = OptimaHessianApproximation::Lbfgs;
    ip_options.interior_point.lbfgs_memory = 10;
    ip_options.interior_point.max_krylov_iterations = 2 * (n - 1);

    rows.push_back(run_structured_case(n, "SequentialQP", problem, initial, target, sqp_options));
    rows.push_back(run_structured_case(n, "InteriorPoint", problem, initial, target, ip_options));
  }

  print_table(rows);
  print_notes(rows);
  return 0;
}
