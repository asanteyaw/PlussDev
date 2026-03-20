#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::optim::ConstrainedProblem;
using pluss::optim::ConstraintMultipliers;
using pluss::optim::OptimaAlgorithm;
using pluss::optim::OptimaExitFlag;
using pluss::optim::OptimaHessianApproximation;
using pluss::optim::OptimaLinearSystemSolver;
using pluss::optim::OptimaLinearSystemStructure;
using pluss::optim::OptimaProgressSnapshot;
using pluss::optim::OptimaProgressStage;
using pluss::optim::OptimaProblemScaling;
using pluss::optim::ParameterPack;
using pluss::optim::Trace;
using pluss::optim::optima;
using pluss::optim::optima_stall_category_name;

struct Garch11Params {
  Series omega{Series::undefined()};
  Series alpha{Series::undefined()};
  Series beta{Series::undefined()};
};

Series scalar(double v, const pluss::Config& cfg) {
  return pluss::make_series(v, cfg);
}

pluss::Config scalar_config_like(const Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

Garch11Params decode_params(const ParameterPack<Series>& params) {
  Garch11Params p;
  p.omega = params.at("omega");
  p.alpha = params.at("alpha");
  p.beta = params.at("beta");
  return p;
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

double as_f64(const Series& x) {
  if (!x.defined() || x.numel() != 1) throw std::runtime_error("expected scalar Series");
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

double max_abs_series(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.abs().max().to(pluss::pCPU, ScalarType::Float64).item<double>();
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

std::string_view to_string(OptimaLinearSystemSolver solver) {
  switch (solver) {
    case OptimaLinearSystemSolver::Auto: return "auto";
    case OptimaLinearSystemSolver::Cholesky: return "cholesky";
    case OptimaLinearSystemSolver::PivotedDirect: return "pivoted-direct";
    case OptimaLinearSystemSolver::Inverse: return "inverse";
    case OptimaLinearSystemSolver::PseudoInverse: return "pseudoinverse";
    case OptimaLinearSystemSolver::ConjugateGradient: return "conjugate-gradient";
  }
  return "unknown";
}

std::string_view to_string(OptimaLinearSystemStructure structure) {
  switch (structure) {
    case OptimaLinearSystemStructure::Auto: return "auto";
    case OptimaLinearSystemStructure::Dense: return "dense";
    case OptimaLinearSystemStructure::Sparse: return "sparse";
  }
  return "unknown";
}

std::string_view to_string(OptimaHessianApproximation hessian) {
  switch (hessian) {
    case OptimaHessianApproximation::Bfgs: return "bfgs";
    case OptimaHessianApproximation::Identity: return "identity";
    case OptimaHessianApproximation::Lbfgs: return "lbfgs";
    case OptimaHessianApproximation::Exact: return "exact";
    case OptimaHessianApproximation::HessianVector: return "hessian-vector";
  }
  return "unknown";
}

std::string_view to_string(OptimaProblemScaling scaling) {
  switch (scaling) {
    case OptimaProblemScaling::None: return "none";
    case OptimaProblemScaling::InitialPoint: return "initial-point";
    case OptimaProblemScaling::ObjectiveAndConstraints: return "objective-and-constraints";
  }
  return "unknown";
}

std::string_view to_string(OptimaProgressStage stage) {
  switch (stage) {
    case OptimaProgressStage::Start: return "start";
    case OptimaProgressStage::Iteration: return "iter";
    case OptimaProgressStage::Done: return "done";
  }
  return "iter";
}

void print_ip_options(const pluss::optim::InternalProtocolOptions& opt) {
  std::cout << "InternalProtocol options\n";
  std::cout << "  max_iterations=" << opt.max_iterations << '\n';
  std::cout << "  max_line_search_iterations=" << opt.max_line_search_iterations << '\n';
  std::cout << "  max_restoration_iterations=" << opt.max_restoration_iterations << '\n';
  std::cout << "  initial_step_size=" << opt.initial_step_size << '\n';
  std::cout << "  backtracking=" << opt.backtracking << '\n';
  std::cout << "  armijo=" << opt.armijo << '\n';
  std::cout << "  initial_barrier_parameter=" << opt.initial_barrier_parameter << '\n';
  std::cout << "  minimum_barrier_parameter=" << opt.minimum_barrier_parameter << '\n';
  std::cout << "  barrier_decay=" << opt.barrier_decay << '\n';
  std::cout << "  equality_penalty=" << opt.equality_penalty << '\n';
  std::cout << "  centrality_fraction=" << opt.centrality_fraction << '\n';
  std::cout << "  dual_recentering_fraction=" << opt.dual_recentering_fraction << '\n';
  std::cout << "  feasibility_activation_factor=" << opt.feasibility_activation_factor << '\n';
  std::cout << "  constraint_tolerance=" << opt.constraint_tolerance << '\n';
  std::cout << "  optimality_tolerance=" << opt.optimality_tolerance << '\n';
  std::cout << "  fraction_to_boundary=" << opt.fraction_to_boundary << '\n';
  std::cout << "  pivot_threshold=" << opt.pivot_threshold << '\n';
  std::cout << "  initialization_margin=" << opt.initialization_margin << '\n';
  std::cout << "  bfgs_curvature_tolerance=" << opt.bfgs_curvature_tolerance << '\n';
  std::cout << "  lbfgs_memory=" << opt.lbfgs_memory << '\n';
  std::cout << "  max_krylov_iterations=" << opt.max_krylov_iterations << '\n';
  std::cout << "  iterative_dimension_threshold=" << opt.iterative_dimension_threshold << '\n';
  std::cout << "  krylov_tolerance=" << opt.krylov_tolerance << '\n';
  std::cout << "  krylov_absolute_tolerance=" << opt.krylov_absolute_tolerance << '\n';
  std::cout << "  enable_feasibility_phase=" << (opt.enable_feasibility_phase ? "true" : "false") << '\n';
  std::cout << "  project_bounds=" << (opt.project_bounds ? "true" : "false") << '\n';
  std::cout << "  enable_restoration=" << (opt.enable_restoration ? "true" : "false") << '\n';
  std::cout << "  problem_scaling=" << to_string(opt.problem_scaling) << '\n';
  std::cout << "  linear_system_solver=" << to_string(opt.linear_system_solver) << '\n';
  std::cout << "  system_structure=" << to_string(opt.system_structure) << '\n';
  std::cout << "  hessian_approximation=" << to_string(opt.hessian_approximation) << '\n';
  std::cout << '\n';
}

void print_multiplier_summary(const ConstraintMultipliers& multipliers) {
  std::cout << "Multiplier summary\n";
  std::cout << "  linear_equalities max=" << max_abs_series(multipliers.linear_equalities) << '\n';
  std::cout << "  nonlinear_equalities max=" << max_abs_series(multipliers.nonlinear_equalities) << '\n';
  std::cout << "  linear_inequalities max=" << max_abs_series(multipliers.linear_inequalities) << '\n';
  std::cout << "  nonlinear_inequalities max=" << max_abs_series(multipliers.nonlinear_inequalities) << '\n';
  std::cout << "  lower_bounds max=" << max_abs_series(multipliers.lower_bounds) << '\n';
  std::cout << "  upper_bounds max=" << max_abs_series(multipliers.upper_bounds) << '\n';
}

} // namespace

int main() {
  using namespace pluss::optim;

  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };

  const Series returns = simulate_garch11(truth, 200, 20260225ULL, cfg);
  const Series variance_guess = returns.var(false).maximum(1e-12);
  const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const Series alpha_guess = scalar(0.15, cfg);
  const Series beta_guess = scalar(0.70, cfg);

  const ConstrainedProblem problem = pluss::optim::problem()
      .objective([returns, cfg](const ParameterPack<Trace>& p) -> Trace {
    const Trace omega = clamp(p.at("omega"), 1e-12, 1e12);
    const Trace alpha = clamp(p.at("alpha"), 1e-12, 0.999);
    const Trace beta = clamp(p.at("beta"), 1e-12, 0.999);
    Trace variance = omega / clamp(1.0 - alpha - beta, 1e-8, 1e8);
    Trace nll = trace(scalar(0.0, cfg), false);

    const std::int64_t n = returns.numel();
    for (std::int64_t t_idx = 0; t_idx < n; ++t_idx) {
      if (t_idx > 0) {
        const Trace prev = trace(returns.select(0, t_idx - 1), false);
        variance = omega + alpha * prev * prev + beta * variance;
      }
    const Trace var_clamped = clamp(variance, 1e-12, 1e12);
    const Trace r = trace(returns.select(0, t_idx), false);
    nll = nll + 0.5 * (log(var_clamped) + (r * r) / var_clamped);
  }
  return nll;
      })
      .nonlinear_inequality([](const ParameterPack<Trace>& p) -> Trace {
        return p.at("alpha") + p.at("beta") - 0.999;
      })
      .lbs(pluss::optim::pack_scalars({
          {"omega", scalar(1e-6, cfg)},
          {"alpha", scalar(1e-6, cfg)},
          {"beta", scalar(1e-6, cfg)},
      }))
      .ubs(pluss::optim::pack_scalars({
          {"omega", scalar(1.0, cfg)},
          {"alpha", scalar(0.999, cfg)},
          {"beta", scalar(0.999, cfg)},
      }))
      .build();

  const ParameterPack<Series> initial = pluss::optim::pack_scalars({
      {"omega", omega_guess},
      {"alpha", alpha_guess},
      {"beta", beta_guess},
  });

  OptimaOptions options = pluss::optim::presets::robust_internal_protocol();
  options.internal_protocol.max_line_search_iterations = 20;
  options.internal_protocol.max_krylov_iterations = 25;
  options.internal_protocol.iterative_dimension_threshold = 16;
  options.internal_protocol.krylov_tolerance = 1e-3;
  options.internal_protocol.krylov_absolute_tolerance = 1e-10;
  options.internal_protocol.pivot_threshold = 1e-12;
  options.internal_protocol.initialization_margin = 1e-4;
  options.internal_protocol.lbfgs_memory = 10;

  options.internal_protocol.progress_hook = [](const OptimaProgressSnapshot& snapshot) {
    std::cout << std::fixed << std::setprecision(6)
              << "[ip:" << to_string(snapshot.stage) << "]"
              << " iter=" << snapshot.iteration
              << " evals=" << snapshot.function_evaluations
              << " value=" << (snapshot.evaluation.value.defined() && snapshot.evaluation.value.numel() == 1
                                   ? snapshot.evaluation.value.item<double>()
                                   : std::numeric_limits<double>::quiet_NaN())
              << " viol=" << snapshot.evaluation.max_constraint_violation
              << " stat=" << snapshot.stationarity
              << " compl=" << snapshot.complementarity
              << " barrier=" << snapshot.backend_parameter
              << " step=" << snapshot.step_size
              << " krylov=" << snapshot.krylov_iterations
              << " stall=" << optima_stall_category_name(snapshot.stall_category)
              << " msg=\"" << snapshot.message << "\"\n";
    return false;
  };

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "GARCH(1,1) InternalProtocol demo\n";
  std::cout << "true    omega=" << as_f64(truth.omega)
            << " alpha=" << as_f64(truth.alpha)
            << " beta=" << as_f64(truth.beta)
            << " persistence=" << as_f64(truth.alpha.add(truth.beta)) << "\n\n";

  print_ip_options(options.internal_protocol);

  const auto result = optima(problem, initial, options);
  const Garch11Params est = decode_params(result.params);
  const Series truth_value = garch11_negloglik(returns, truth);
  const Garch11Params initial_guess{omega_guess, alpha_guess, beta_guess};
  const Series initial_value = garch11_negloglik(returns, initial_guess);

  std::cout << "\nFinal report\n";
  std::cout << "  exit=" << to_string(result.exit_flag) << '\n';
  std::cout << "  message=" << result.message << '\n';
  std::cout << "  iterations=" << result.iteration << '\n';
  std::cout << "  evaluations=" << result.function_evaluations << '\n';
  std::cout << "  objective=" << result.evaluation.value.item<double>() << '\n';
  std::cout << "  stationarity=" << result.stationarity << '\n';
  std::cout << "  step_size=" << result.last_step_size << '\n';
  std::cout << "  barrier_parameter=" << result.backend_parameter << '\n';
  std::cout << "  constraint_violation=" << result.evaluation.max_constraint_violation << '\n';
  std::cout << "  first_order_optimality=" << result.output.first_order_optimality << '\n';
  std::cout << "  complementarity=" << result.output.complementarity << '\n';
  std::cout << "  krylov_iterations=" << result.output.krylov_iterations << '\n';
  std::cout << "  stall_category=" << optima_stall_category_name(result.output.stall_category) << '\n';
  std::cout << "  stall_measure=" << result.output.stall_measure << '\n';
  std::cout << "  stall_reference=" << result.output.stall_reference << '\n';
  std::cout << "  best_feasible_available=" << (result.output.best_feasible_available ? "true" : "false") << '\n';
  std::cout << "  returned_best_feasible=" << (result.output.returned_best_feasible ? "true" : "false") << '\n';
  std::cout << "  detail_message=" << result.output.detail_message << "\n\n";

  std::cout << "Likelihood checkpoints\n";
  std::cout << "  truth_objective=" << as_f64(truth_value) << '\n';
  std::cout << "  initial_objective=" << as_f64(initial_value) << "\n\n";

  std::cout << "Compact summary\n  ";
  pluss::optim::print_summary(std::cout, result) << "\n\n";

  std::cout << "Estimated parameters\n";
  std::cout << "  omega=" << as_f64(est.omega) << '\n';
  std::cout << "  alpha=" << as_f64(est.alpha) << '\n';
  std::cout << "  beta=" << as_f64(est.beta) << '\n';
  std::cout << "  persistence=" << as_f64(est.alpha.add(est.beta)) << "\n\n";

  std::cout << "Absolute parameter errors\n";
  std::cout << "  omega=" << std::abs(as_f64(est.omega.sub(truth.omega))) << '\n';
  std::cout << "  alpha=" << std::abs(as_f64(est.alpha.sub(truth.alpha))) << '\n';
  std::cout << "  beta=" << std::abs(as_f64(est.beta.sub(truth.beta))) << "\n\n";

  if (result.best_feasible.has_value()) {
    const Garch11Params best = decode_params(result.best_feasible->params);
    std::cout << "Best feasible parameters\n";
    std::cout << "  objective=" << result.best_feasible->value.item<double>() << '\n';
    std::cout << "  first_order_opt=" << result.best_feasible->first_order_opt << '\n';
    std::cout << "  constraint_violation=" << result.best_feasible->max_constraint_violation << '\n';
    std::cout << "  omega=" << as_f64(best.omega) << '\n';
    std::cout << "  alpha=" << as_f64(best.alpha) << '\n';
    std::cout << "  beta=" << as_f64(best.beta) << '\n';
    std::cout << "  persistence=" << as_f64(best.alpha.add(best.beta)) << "\n\n";
  }

  print_multiplier_summary(result.multipliers);
  return 0;
}
