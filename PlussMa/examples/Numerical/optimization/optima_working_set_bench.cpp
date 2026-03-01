#include <pluss/pluss.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
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
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

struct Row {
  std::int64_t variables{0};
  std::string solver{};
  std::string exit{};
  std::string stall{};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double violation{std::numeric_limits<double>::infinity()};
  double first_order{std::numeric_limits<double>::infinity()};
  double max_abs_error{std::numeric_limits<double>::infinity()};
  double time_ms{0.0};
};

Series scalar(double v) {
  return pluss::make_series(v).to(ScalarType::Float64);
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

std::vector<double> unpack_scalar_blocks(const ParameterPack<Series>& params) {
  std::vector<double> out;
  out.reserve(params.size());
  for (const auto& block : params.blocks()) out.push_back(as_f64(block.value));
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
  problem.linear.Aeq = pluss::make_series(std::vector<double>(static_cast<std::size_t>(n), 1.0), {1, n}).to(ScalarType::Float64);
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
    case OptimaExitFlag::Converged: return "Converged";
    case OptimaExitFlag::MaxIterations: return "MaxIterations";
    case OptimaExitFlag::Stopped: return "Stopped";
    case OptimaExitFlag::NoFeasiblePoint: return "NoFeasiblePoint";
    case OptimaExitFlag::ObjectiveTargetReached: return "ObjectiveTargetReached";
    case OptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case OptimaExitFlag::InvalidProblem: return "InvalidProblem";
    case OptimaExitFlag::NoProgress: return "NoProgress";
    case OptimaExitFlag::ConstraintStationary: return "ConstraintStationary";
  }
  return "Unknown";
}

double max_abs_error_to_target(const ParameterPack<Series>& params, const std::vector<double>& target) {
  const auto values = unpack_scalar_blocks(params);
  double best = 0.0;
  for (std::size_t i = 0; i < values.size() && i < target.size(); ++i) {
    best = std::max(best, std::abs(values[i] - target[i]));
  }
  return best;
}

Row run_case(std::int64_t n,
             std::string solver,
             const ConstrainedProblem& problem,
             const ParameterPack<Series>& initial,
             const std::vector<double>& target,
             const OptimaOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = optima(problem, initial, options);
  const auto stop = std::chrono::steady_clock::now();

  Row row;
  row.variables = n;
  row.solver = std::move(solver);
  row.exit = to_string(result.exit_flag);
  row.stall = std::string(optima_stall_category_name(result.output.stall_category));
  row.iterations = result.iteration;
  row.evaluations = result.function_evaluations;
  row.objective = result.evaluation.value.item<double>();
  row.violation = result.evaluation.max_constraint_violation;
  row.first_order = result.output.first_order_optimality;
  row.max_abs_error = max_abs_error_to_target(result.params, target);
  row.time_ms = std::chrono::duration<double, std::milli>(stop - start).count();
  return row;
}

void print_rows(const std::vector<Row>& rows) {
  std::cout << "| n | Solver | Exit | Stall | Iter | Eval | Objective | Constr viol | First-order | Max |x-target| | Time ms |\n";
  std::cout << "|---:|---|---|---|---:|---:|---:|---:|---:|---:|---:|\n";
  for (const auto& row : rows) {
    std::cout << "| " << row.variables
              << " | " << row.solver
              << " | " << row.exit
              << " | " << row.stall
              << " | " << row.iterations
              << " | " << row.evaluations
              << " | " << row.objective
              << " | " << row.violation
              << " | " << row.first_order
              << " | " << row.max_abs_error
              << " | " << row.time_ms
              << " |\n";
  }
}

} // namespace

int main() {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "# Optima WorkingSet Benchmark\n\n";

  std::vector<Row> rows;
  for (const std::int64_t n : {16, 32}) {
    const auto target = make_target_profile(n);
    const auto problem = make_problem(n, target, 0.12);
    const auto initial = make_scalar_blocks(make_infeasible_start(n));

    OptimaOptions sqp;
    sqp.algorithm = OptimaAlgorithm::SequentialQP;
    sqp.sequential_qp.max_iterations = 300;
    sqp.sequential_qp.constraint_tolerance = 1e-8;
    sqp.sequential_qp.optimality_tolerance = 1e-6;
    sqp.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    sqp.sequential_qp.system_structure = OptimaLinearSystemStructure::Sparse;
    sqp.sequential_qp.linear_system_solver = OptimaLinearSystemSolver::Auto;
    sqp.sequential_qp.hessian_approximation = OptimaHessianApproximation::Bfgs;

    OptimaOptions ws = sqp;
    ws.algorithm = OptimaAlgorithm::WorkingSet;
    ws.working_set.max_iterations = 300;
    ws.working_set.constraint_tolerance = 1e-8;
    ws.working_set.optimality_tolerance = 1e-6;
    ws.working_set.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    ws.working_set.system_structure = OptimaLinearSystemStructure::Sparse;
    ws.working_set.linear_system_solver = OptimaLinearSystemSolver::Auto;
    ws.working_set.hessian_approximation = OptimaHessianApproximation::Bfgs;
    ws.working_set.max_working_set_violations = 4;
    ws.working_set.add_constraint_tolerance = 5e-4;

    rows.push_back(run_case(n, "SequentialQP", problem, initial, target, sqp));
    rows.push_back(run_case(n, "WorkingSet", problem, initial, target, ws));
  }

  print_rows(rows);
  return 0;
}
