#include <pluss/pluss.h>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaStallCategory;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::optima_stall_category_name;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

struct PathologyRow {
  std::string case_name{};
  std::string solver{};
  std::string exit{};
  std::string stall{};
  EvalStatus eval_status{EvalStatus::Ok};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double violation{std::numeric_limits<double>::infinity()};
  double first_order{std::numeric_limits<double>::infinity()};
  double complementarity{std::numeric_limits<double>::infinity()};
  double step_size{0.0};
  std::string note{};
};

Series scalar(double x) {
  return make_series(x).to(ScalarType::Float64).reshape({1});
}

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

double scalar_or_nan(const Series& x) {
  if (!x.defined() || x.numel() != 1) return std::numeric_limits<double>::quiet_NaN();
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
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

ConstrainedProblem make_nonfinite_region_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p)
      -> std::pair<Series, ParameterPack<Series>> {
    const double x = scalar_or_nan(p.at("x"));
    if (!(x > 0.0)) {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      return std::pair{
          make_series(nan).to(ScalarType::Float64),
          ParameterPack<Series>({
              {"x", make_series(nan).to(ScalarType::Float64).reshape({1})},
          })};
    }
    return std::pair{
        make_series((x - 0.25) * (x - 0.25)).to(ScalarType::Float64),
        ParameterPack<Series>({
            {"x", scalar(2.0 * (x - 0.25))},
        })};
  };
  return problem;
}

ConstrainedProblem make_boundary_limited_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") + 1.0);
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", scalar(0.0)},
  });
  return problem;
}

ConstrainedProblem make_simple_quadratic_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") - 1.0);
  };
  return problem;
}

ConstrainedProblem make_recovery_failure_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p)
      -> std::pair<Series, ParameterPack<Series>> {
    const double x = scalar_or_nan(p.at("x"));
    if (!(x > 0.0)) {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      return std::pair{
          make_series(nan).to(ScalarType::Float64),
          ParameterPack<Series>({{"x", make_series(nan).to(ScalarType::Float64).reshape({1})}})};
    }
    return std::pair{
        make_series((x - 0.25) * (x - 0.25)).to(ScalarType::Float64),
        ParameterPack<Series>({{"x", scalar(2.0 * (x - 0.25))}})};
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return p.at("x") - 0.2;
  };
  return problem;
}

ConstrainedProblem make_feasibility_phase_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") * 0.0);
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return p.at("x") - 0.1;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", scalar(0.0)},
  });
  return problem;
}

ConstrainedProblem make_complementarity_probe_problem() {
  ConstrainedProblem problem;
  const Series target = make_vec2(1.0, 2.0);
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

PathologyRow run_case(const std::string& case_name,
                      const std::string& solver,
                      const ConstrainedProblem& problem,
                      const ParameterPack<Series>& initial,
                      const OptimaOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = optima(problem, initial, options);
  const auto stop = std::chrono::steady_clock::now();

  PathologyRow row;
  row.case_name = case_name;
  row.solver = solver;
  row.exit = to_string(result.exit_flag);
  row.stall = std::string(optima_stall_category_name(result.output.stall_category));
  row.eval_status = result.evaluation.status;
  row.iterations = result.iteration;
  row.evaluations = result.function_evaluations;
  row.objective = result.evaluation.value.defined() ? scalar_or_nan(result.evaluation.value) : std::numeric_limits<double>::quiet_NaN();
  row.violation = result.evaluation.max_constraint_violation;
  row.first_order = result.output.first_order_optimality;
  row.complementarity = result.output.complementarity;
  row.step_size = result.output.step_size;
  row.note = result.output.detail_message;
  row.note += " time_ms=" +
              std::to_string(std::chrono::duration<double, std::milli>(stop - start).count());
  return row;
}

void print_table(const std::vector<PathologyRow>& rows) {
  std::cout << "| Case | Solver | Exit | Stall | EvalStatus | Iter | Eval | Objective | Violation | First-order | Complementarity | Step | Note |\n";
  std::cout << "|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|\n";
  for (const auto& row : rows) {
    std::cout << "| " << row.case_name
              << " | " << row.solver
              << " | " << row.exit
              << " | " << row.stall
              << " | " << static_cast<int>(row.eval_status)
              << " | " << row.iterations
              << " | " << row.evaluations
              << " | " << row.objective
              << " | " << row.violation
              << " | " << row.first_order
              << " | " << row.complementarity
              << " | " << row.step_size
              << " | " << row.note
              << " |\n";
  }
}

} // namespace

int main() {
  std::cout << "# Optima Clone-Parity Benchmark: Pathological Stall Suite\n\n";

  std::vector<PathologyRow> rows;

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::SequentialQP;
    const auto problem = make_nonfinite_region_problem();
    const ParameterPack<Series> initial({{"x", scalar(-0.5)}});
    rows.push_back(run_case("NonFiniteInitial", "SequentialQP", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.initial_barrier_parameter = 0.1;
    const auto problem = make_nonfinite_region_problem();
    const ParameterPack<Series> initial({{"x", scalar(-0.5)}});
    rows.push_back(run_case("NonFiniteInitial", "InternalProtocol", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::SequentialQP;
    options.sequential_qp.max_iterations = 5;
    const auto problem = make_boundary_limited_problem();
    const ParameterPack<Series> initial({{"x", scalar(0.0)}});
    rows.push_back(run_case("BoundaryLimited", "SequentialQP", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::SequentialQP;
    options.sequential_qp.max_iterations = 5;
    options.sequential_qp.max_line_search_iterations = 0;
    options.sequential_qp.enable_restoration = false;
    const auto problem = make_simple_quadratic_problem();
    const ParameterPack<Series> initial({{"x", scalar(0.0)}});
    rows.push_back(run_case("SearchDirectionCollapse", "SequentialQP", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::SequentialQP;
    options.sequential_qp.max_iterations = 5;
    options.sequential_qp.max_line_search_iterations = 0;
    options.sequential_qp.max_restoration_iterations = 0;
    options.sequential_qp.enable_restoration = true;
    const auto problem = make_recovery_failure_problem();
    const ParameterPack<Series> initial({{"x", scalar(1.0)}});
    rows.push_back(run_case("RecoveryFailure", "SequentialQP", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.max_iterations = 5;
    options.internal_protocol.max_line_search_iterations = 0;
    options.internal_protocol.enable_restoration = false;
    options.internal_protocol.enable_feasibility_phase = true;
    options.internal_protocol.max_restoration_iterations = 0;
    options.internal_protocol.initial_barrier_parameter = 0.01;
    options.internal_protocol.minimum_barrier_parameter = 1e-6;
    const auto problem = make_feasibility_phase_problem();
    const ParameterPack<Series> initial({{"x", scalar(1.0)}});
    rows.push_back(run_case("FeasibilityFailure", "InternalProtocol", problem, initial, options));
  }

  {
    OptimaOptions options;
    options.algorithm = OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.max_iterations = 1;
    options.internal_protocol.initial_barrier_parameter = 0.1;
    options.internal_protocol.minimum_barrier_parameter = 1e-10;
    options.internal_protocol.constraint_tolerance = 1e-6;
    options.internal_protocol.optimality_tolerance = 1e-6;
    options.internal_protocol.dual_recentering_fraction = 0.0;
    const auto problem = make_complementarity_probe_problem();
    const ParameterPack<Series> initial({
        {"x", make_vec2(0.7, 0.7)},
    });
    rows.push_back(run_case("ComplementarityProbe", "InternalProtocol", problem, initial, options));
  }

  print_table(rows);
  std::cout << "\nNotes:\n";
  std::cout << "- If `ComplementarityProbe` lands on `complementarity-stagnation`, live complementarity coverage is closed.\n";
  return 0;
}
