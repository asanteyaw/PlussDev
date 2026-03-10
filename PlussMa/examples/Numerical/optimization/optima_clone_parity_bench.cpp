#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::optima_stall_category_name;
using pluss::subsp::math::optimization::DifferentialEvolutionPrmmConfig;
using pluss::subsp::math::optimization::NelderMeadConfig;
using pluss::subsp::math::optimization::NelderMeadOutcome;
using pluss::subsp::math::optimization::PopulationSolverOutcome;
using pluss::subsp::math::optimization::PopulationSolverStatus;
using pluss::subsp::math::optimization::SolverStatus;
using pluss::subsp::math::optimization::differential_evolution_prmm_minimizer;
using pluss::subsp::math::optimization::nelder_mead_minimizer;

struct Garch11Params {
  Series omega{Series::undefined()};
  Series alpha{Series::undefined()};
  Series beta{Series::undefined()};
};

struct BenchmarkRow {
  std::string family{};
  std::int64_t sample_size{0};
  std::string solver{};
  std::string backend_family{};
  std::string exit{};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double constraint_violation{0.0};
  double first_order_optimality{0.0};
  double complementarity{0.0};
  std::string stall{};
  double wall_time_ms{0.0};
  bool best_feasible_available{false};
  double best_feasible_value{std::numeric_limits<double>::infinity()};
  double best_feasible_constraint_violation{std::numeric_limits<double>::infinity()};
  double omega{std::numeric_limits<double>::quiet_NaN()};
  double alpha{std::numeric_limits<double>::quiet_NaN()};
  double beta{std::numeric_limits<double>::quiet_NaN()};
  double persistence{std::numeric_limits<double>::quiet_NaN()};
  double omega_abs_error{std::numeric_limits<double>::quiet_NaN()};
  double alpha_abs_error{std::numeric_limits<double>::quiet_NaN()};
  double beta_abs_error{std::numeric_limits<double>::quiet_NaN()};
  std::string note{};
};

Series scalar(double v, const pluss::Config& cfg) {
  return pluss::make_series(v, cfg);
}

pluss::Config scalar_config_like(const Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

double as_f64(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Series logit_series(const Series& p) {
  const auto cfg = scalar_config_like(p);
  const Series one = scalar(1.0, cfg);
  return p.log().sub(one.sub(p).log());
}

Garch11Params decode_params(const Series& unconstrained) {
  const Series u = unconstrained.reshape({3});
  const auto cfg = scalar_config_like(u);
  const Series persistence_cap = scalar(0.999, cfg);
  const Series one = scalar(1.0, cfg);
  const Series s1 = u.select(0, 1).sigmoid();
  const Series s2 = u.select(0, 2).sigmoid();

  Garch11Params p;
  p.omega = u.select(0, 0).exp();
  p.alpha = persistence_cap.mul(s1);
  p.beta = persistence_cap.mul(one.sub(s1)).mul(s2);
  return p;
}

Garch11Params decode_params(const ParameterPack<Series>& params) {
  const auto cfg = scalar_config_like(params.at("log_omega"));
  const Series persistence_cap = scalar(0.999, cfg);
  const Series one = scalar(1.0, cfg);
  const Series s1 = params.at("logit_alpha").sigmoid();
  const Series s2 = params.at("logit_beta").sigmoid();

  Garch11Params p;
  p.omega = params.at("log_omega").exp();
  p.alpha = persistence_cap.mul(s1);
  p.beta = persistence_cap.mul(one.sub(s1)).mul(s2);
  return p;
}

Series encode_guess(const Series& omega_guess, const Series& alpha_guess, const Series& beta_guess) {
  const auto cfg = scalar_config_like(omega_guess);
  const Series one = scalar(1.0, cfg);
  const Series persistence_cap = scalar(0.999, cfg);
  const Series s1 = alpha_guess.div(persistence_cap).clamp_min(1e-6).clamp_max(1.0 - 1e-6);
  const Series s2 = beta_guess.div(persistence_cap.mul(one.sub(s1))).clamp_min(1e-6).clamp_max(1.0 - 1e-6);

  return omega_guess.log().reshape({1}).unite(
      std::vector<Series>{
          logit_series(s1).reshape({1}),
          logit_series(s2).reshape({1}),
      },
      0);
}

Series simulate_garch11(const Garch11Params& p, std::int64_t n, std::uint64_t seed, const pluss::Config& cfg) {
  const Series z = pluss::randn({n}, cfg, seed);
  Series returns = pluss::zeros({n}, cfg);

  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform);
  const Series one = scalar(1.0, cfg);
  Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));

  for (std::int64_t t = 0; t < n; ++t) {
    const Series idx = pluss::arange(t, t + 1, idx_cfg);
    const Series eps = z.select(0, t);
    const Series r = variance.maximum(1e-12).sqrt().mul(eps);
    returns = returns.put(idx, r.reshape({1}), false);
    variance = p.omega.add(p.alpha.mul(r.mul(r))).add(p.beta.mul(variance));
  }

  return returns;
}

Series garch11_negloglik(const Series& returns, const Garch11Params& p) {
  const auto cfg = scalar_config_like(returns);
  const Series one = scalar(1.0, cfg);
  Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));
  Series nll = scalar(0.0, cfg);

  const std::int64_t n = returns.numel();
  for (std::int64_t t = 0; t < n; ++t) {
    if (t > 0) {
      const Series prev = returns.select(0, t - 1);
      variance = p.omega.add(p.alpha.mul(prev.mul(prev))).add(p.beta.mul(variance));
    }

    const Series var_clamped = variance.maximum(1e-12);
    const Series r = returns.select(0, t);
    nll = nll.add(var_clamped.log().add(r.mul(r).div(var_clamped)).mul(0.5));
  }

  return nll;
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

const char* to_string(SolverStatus status) {
  switch (status) {
    case SolverStatus::Converged:
      return "Converged";
    case SolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case SolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

const char* to_string(PopulationSolverStatus status) {
  switch (status) {
    case PopulationSolverStatus::Converged:
      return "Converged";
    case PopulationSolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case PopulationSolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

void fill_param_errors_(BenchmarkRow& row, const Garch11Params& truth, const Garch11Params& est) {
  row.omega = as_f64(est.omega);
  row.alpha = as_f64(est.alpha);
  row.beta = as_f64(est.beta);
  row.persistence = as_f64(est.alpha.add(est.beta));
  row.omega_abs_error = std::abs(as_f64(est.omega.sub(truth.omega)));
  row.alpha_abs_error = std::abs(as_f64(est.alpha.sub(truth.alpha)));
  row.beta_abs_error = std::abs(as_f64(est.beta.sub(truth.beta)));
}

BenchmarkRow run_sequential_qp(const std::string& family,
                               std::int64_t sample_size,
                               const ConstrainedProblem& problem,
                               const ParameterPack<Series>& initial,
                               const Garch11Params& truth,
                               const OptimaOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = optima(problem, initial, options);
  const auto stop = std::chrono::steady_clock::now();

  BenchmarkRow row;
  row.family = family;
  row.sample_size = sample_size;
  row.solver = "SequentialQP";
  row.backend_family = "Optima";
  row.exit = to_string(result.exit_flag);
  row.iterations = result.iteration;
  row.evaluations = result.function_evaluations;
  row.objective = result.evaluation.value.item<double>();
  row.constraint_violation = result.evaluation.max_constraint_violation;
  row.first_order_optimality = result.output.first_order_optimality;
  row.complementarity = result.output.complementarity;
  row.stall = std::string(optima_stall_category_name(result.output.stall_category));
  row.wall_time_ms =
      std::chrono::duration<double, std::milli>(stop - start).count();
  row.best_feasible_available = result.output.best_feasible_available;
  row.best_feasible_value = result.output.best_feasible_value;
  row.best_feasible_constraint_violation = result.output.best_feasible_constraint_violation;
  row.note = result.output.detail_message;
  fill_param_errors_(row, truth, decode_params(result.params));
  return row;
}

BenchmarkRow run_internal_protocol(const std::string& family,
                                std::int64_t sample_size,
                                const ConstrainedProblem& problem,
                                const ParameterPack<Series>& initial,
                                const Garch11Params& truth,
                                const OptimaOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = optima(problem, initial, options);
  const auto stop = std::chrono::steady_clock::now();

  BenchmarkRow row;
  row.family = family;
  row.sample_size = sample_size;
  row.solver = "InternalProtocol";
  row.backend_family = "Optima";
  row.exit = to_string(result.exit_flag);
  row.iterations = result.iteration;
  row.evaluations = result.function_evaluations;
  row.objective = result.evaluation.value.item<double>();
  row.constraint_violation = result.evaluation.max_constraint_violation;
  row.first_order_optimality = result.output.first_order_optimality;
  row.complementarity = result.output.complementarity;
  row.stall = std::string(optima_stall_category_name(result.output.stall_category));
  row.wall_time_ms =
      std::chrono::duration<double, std::milli>(stop - start).count();
  row.best_feasible_available = result.output.best_feasible_available;
  row.best_feasible_value = result.output.best_feasible_value;
  row.best_feasible_constraint_violation = result.output.best_feasible_constraint_violation;
  row.note = result.output.detail_message;
  fill_param_errors_(row, truth, decode_params(result.params));
  return row;
}

BenchmarkRow run_nelder_mead(const std::string& family,
                             std::int64_t sample_size,
                             const Series& x0,
                             const Garch11Params& truth,
                             const std::function<Series(const Series&)>& objective,
                             const NelderMeadConfig& options) {
  const auto start = std::chrono::steady_clock::now();
  const NelderMeadOutcome out = nelder_mead_minimizer(objective, x0, options);
  const auto stop = std::chrono::steady_clock::now();

  BenchmarkRow row;
  row.family = family;
  row.sample_size = sample_size;
  row.solver = "NelderMead";
  row.backend_family = "DFO";
  row.exit = to_string(out.status);
  row.iterations = out.diagnostics.iterations;
  row.evaluations = out.diagnostics.evaluations;
  row.objective = out.minimum;
  row.wall_time_ms =
      std::chrono::duration<double, std::milli>(stop - start).count();
  row.note = "baseline derivative-free reference";
  fill_param_errors_(row, truth, decode_params(out.minimizer));
  return row;
}

BenchmarkRow run_de_prmm(const std::string& family,
                         std::int64_t sample_size,
                         const Series& x0,
                         const Garch11Params& truth,
                         const std::function<Series(const Series&)>& objective,
                         const DifferentialEvolutionPrmmConfig& options) {
  const auto start = std::chrono::steady_clock::now();
  const PopulationSolverOutcome out = differential_evolution_prmm_minimizer(objective, x0, options);
  const auto stop = std::chrono::steady_clock::now();

  BenchmarkRow row;
  row.family = family;
  row.sample_size = sample_size;
  row.solver = "DE_PRMM";
  row.backend_family = "DFO";
  row.exit = to_string(out.status);
  row.iterations = out.diagnostics.iterations;
  row.evaluations = out.diagnostics.evaluations;
  row.objective = out.minimum;
  row.wall_time_ms =
      std::chrono::duration<double, std::milli>(stop - start).count();
  row.note = "population derivative-free reference";
  fill_param_errors_(row, truth, decode_params(out.minimizer));
  return row;
}

void print_markdown_table(const std::vector<BenchmarkRow>& rows) {
  std::cout << "| Problem | N | Solver | Family | Exit | Stall | Iter | Eval | Objective | Constr viol | First-order | Complementarity | Best feasible | Best feas viol | Time ms | Persistence | |omega err| |alpha err| |beta err| |\n";
  std::cout << "|---|---:|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
  for (const auto& row : rows) {
    std::cout << "| " << row.family
              << " | " << row.sample_size
              << " | " << row.solver
              << " | " << row.backend_family
              << " | " << row.exit
              << " | " << row.stall
              << " | " << row.iterations
              << " | " << row.evaluations
              << " | " << row.objective
              << " | " << row.constraint_violation
              << " | " << row.first_order_optimality
              << " | " << row.complementarity
              << " | " << (row.best_feasible_available ? row.best_feasible_value : std::numeric_limits<double>::quiet_NaN())
              << " | " << (row.best_feasible_available ? row.best_feasible_constraint_violation
                                                       : std::numeric_limits<double>::quiet_NaN())
              << " | " << row.wall_time_ms
              << " | " << row.persistence
              << " | " << row.omega_abs_error
              << " | " << row.alpha_abs_error
              << " | " << row.beta_abs_error
              << " |\n";
  }
}

void print_notes(const std::vector<BenchmarkRow>& rows) {
  std::cout << "\nNotes:\n";
  for (const auto& row : rows) {
    std::cout << "- " << row.family << " N=" << row.sample_size << " " << row.solver << ": " << row.note << "\n";
  }
}

} // namespace

int main() {
  struct GarchTier {
    std::int64_t n{0};
    std::uint64_t seed{0};
    bool run_dfo{false};
  };

  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };

  std::vector<BenchmarkRow> rows;
  const std::vector<GarchTier> tiers{
      {40, 20260225ULL, true},
      {200, 20260235ULL, false},
      {800, 20260245ULL, false},
  };

  for (const GarchTier& tier : tiers) {
    const Series returns = simulate_garch11(truth, tier.n, tier.seed, cfg);
    const Series variance_guess = returns.var(false).maximum(1e-12);
    const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
    const Series alpha_guess = scalar(0.15, cfg);
    const Series beta_guess = scalar(0.70, cfg);
    const Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);

    ConstrainedProblem problem;
    problem.objective.autodiff = [returns, cfg](const ParameterPack<Trace>& p) -> Trace {
      using namespace pluss::subsp::math::optim;

      const Trace omega = exp(p.at("log_omega"));
      const Trace s1 = sigmoid(p.at("logit_alpha"));
      const Trace s2 = sigmoid(p.at("logit_beta"));
      const Trace alpha = 0.999 * s1;
      const Trace beta = 0.999 * (1.0 - s1) * s2;

      Trace variance = omega / clamp(1.0 - alpha - beta, 1e-8, 1e8);
      Trace nll = trace(scalar(0.0, cfg), false);

      for (std::int64_t t = 0; t < returns.numel(); ++t) {
        if (t > 0) {
          const Trace prev = trace(returns.select(0, t - 1), false);
          variance = omega + alpha * prev * prev + beta * variance;
        }

        const Trace var_clamped = clamp(variance, 1e-12, 1e12);
        const Trace r = trace(returns.select(0, t), false);
        nll = nll + 0.5 * (log(var_clamped) + (r * r) / var_clamped);
      }
      return nll;
    };

    const ParameterPack<Series> initial({
        {"log_omega", x0.select(0, 0).reshape({1})},
        {"logit_alpha", x0.select(0, 1).reshape({1})},
        {"logit_beta", x0.select(0, 2).reshape({1})},
    });

    OptimaOptions sqp_options;
    sqp_options.algorithm = OptimaAlgorithm::SequentialQP;
    sqp_options.sequential_qp.max_iterations = tier.n >= 800 ? 600 : 450;
    sqp_options.sequential_qp.constraint_tolerance = 1e-8;
    sqp_options.sequential_qp.optimality_tolerance = 1e-6;
    sqp_options.sequential_qp.active_set_tolerance = 1e-6;
    sqp_options.sequential_qp.regularization = 1e-8;
    sqp_options.sequential_qp.max_line_search_iterations = 16;
    sqp_options.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

    OptimaOptions ip_options;
    ip_options.algorithm = OptimaAlgorithm::InternalProtocol;
    ip_options.internal_protocol.max_iterations = tier.n >= 800 ? 260 : 160;
    ip_options.internal_protocol.initial_barrier_parameter = 1e-2;
    ip_options.internal_protocol.minimum_barrier_parameter = 1e-8;
    ip_options.internal_protocol.barrier_decay = 0.2;
    ip_options.internal_protocol.constraint_tolerance = 1e-8;
    ip_options.internal_protocol.optimality_tolerance = 1e-6;
    ip_options.internal_protocol.max_line_search_iterations = 20;
    ip_options.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

    const std::string family = "Garch11";
    rows.push_back(run_sequential_qp(family, tier.n, problem, initial, truth, sqp_options));
    rows.push_back(run_internal_protocol(family, tier.n, problem, initial, truth, ip_options));

    if (tier.run_dfo) {
      NelderMeadConfig nm_options;
      nm_options.position_tolerance = 1e-8;
      nm_options.value_tolerance = 1e-8;
      nm_options.max_iterations = 500;
      nm_options.max_evaluations = 1200;

      DifferentialEvolutionPrmmConfig de_options;
      de_options.population_size = 24;
      de_options.elite_count = 8;
      de_options.max_generations = 30;
      de_options.max_evaluations = 0;
      de_options.check_every = 5;
      de_options.relative_improvement_tolerance = 1e-8;
      de_options.display = pluss::subsp::math::optimization::PopulationSolverDisplay::None;
      de_options.rng_seed = tier.seed + 1ULL;

      const auto objective = [&](const Series& x_unconstrained) {
        return garch11_negloglik(returns, decode_params(x_unconstrained));
      };

      rows.push_back(run_nelder_mead(family, tier.n, x0, truth, objective, nm_options));
      rows.push_back(run_de_prmm(family, tier.n, x0, truth, objective, de_options));
    }
  }

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "# Optima Clone-Parity Benchmark: High-Impact GARCH Tiers\n\n";
  std::cout << "Truth: omega=" << as_f64(truth.omega)
            << ", alpha=" << as_f64(truth.alpha)
            << ", beta=" << as_f64(truth.beta)
            << ", persistence=" << as_f64(truth.alpha.add(truth.beta))
            << "\n\n";
  print_markdown_table(rows);
  print_notes(rows);
  return 0;
}
