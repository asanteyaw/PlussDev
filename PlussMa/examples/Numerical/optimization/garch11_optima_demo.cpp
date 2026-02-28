#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Series;
using pluss::ScalarType;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optimization::DifferentialEvolutionPrmmConfig;
using pluss::subsp::math::optimization::NelderMeadConfig;
using pluss::subsp::math::optimization::NelderMeadOutcome;
using pluss::subsp::math::optimization::PopulationSolverOutcome;
using pluss::subsp::math::optimization::differential_evolution_prmm_minimizer;
using pluss::subsp::math::optimization::nelder_mead_minimizer;

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

  Garch11Params p;
  p.omega = params.at("log_omega").exp();
  p.alpha = persistence_cap.mul(params.at("logit_alpha").sigmoid());
  p.beta = persistence_cap.mul(one.sub(params.at("logit_alpha").sigmoid())).mul(params.at("logit_beta").sigmoid());
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

double as_f64(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
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

const char* to_string(pluss::subsp::math::optimization::SolverStatus status) {
  using pluss::subsp::math::optimization::SolverStatus;
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

const char* to_string(pluss::subsp::math::optimization::PopulationSolverStatus status) {
  using pluss::subsp::math::optimization::PopulationSolverStatus;
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

void print_report(const std::string& label,
                  const OptimaOptions& options,
                  const ConstrainedProblem& problem,
                  const ParameterPack<Series>& initial,
                  const Garch11Params& truth) {
  const auto result = optima(problem, initial, options);
  const Garch11Params est = decode_params(result.params);

  std::cout << label
            << " exit=" << to_string(result.exit_flag)
            << " iterations=" << result.iteration
            << " evaluations=" << result.function_evaluations
            << " objective=" << result.evaluation.value.item<double>()
            << " stationarity=" << result.stationarity
            << " message=" << result.message << "\n";

  std::cout << "  fitted omega=" << as_f64(est.omega)
            << " alpha=" << as_f64(est.alpha)
            << " beta=" << as_f64(est.beta)
            << " persistence=" << as_f64(est.alpha.add(est.beta)) << "\n";

  std::cout << "  abs err omega=" << std::abs(as_f64(est.omega.sub(truth.omega)))
            << " alpha=" << std::abs(as_f64(est.alpha.sub(truth.alpha)))
            << " beta=" << std::abs(as_f64(est.beta.sub(truth.beta))) << "\n\n";
}

void print_report(const std::string& label,
                  const NelderMeadOutcome& out,
                  const Garch11Params& truth) {
  const Garch11Params est = decode_params(out.minimizer);
  std::cout << label
            << " exit=" << to_string(out.status)
            << " iterations=" << out.diagnostics.iterations
            << " evaluations=" << out.diagnostics.evaluations
            << " objective=" << out.minimum << "\n";

  std::cout << "  fitted omega=" << as_f64(est.omega)
            << " alpha=" << as_f64(est.alpha)
            << " beta=" << as_f64(est.beta)
            << " persistence=" << as_f64(est.alpha.add(est.beta)) << "\n";

  std::cout << "  abs err omega=" << std::abs(as_f64(est.omega.sub(truth.omega)))
            << " alpha=" << std::abs(as_f64(est.alpha.sub(truth.alpha)))
            << " beta=" << std::abs(as_f64(est.beta.sub(truth.beta))) << "\n\n";
}

void print_report(const std::string& label,
                  const PopulationSolverOutcome& out,
                  const Garch11Params& truth) {
  const Garch11Params est = decode_params(out.minimizer);
  std::cout << label
            << " exit=" << to_string(out.status)
            << " iterations=" << out.diagnostics.iterations
            << " evaluations=" << out.diagnostics.evaluations
            << " objective=" << out.minimum << "\n";

  std::cout << "  fitted omega=" << as_f64(est.omega)
            << " alpha=" << as_f64(est.alpha)
            << " beta=" << as_f64(est.beta)
            << " persistence=" << as_f64(est.alpha.add(est.beta)) << "\n";

  std::cout << "  abs err omega=" << std::abs(as_f64(est.omega.sub(truth.omega)))
            << " alpha=" << std::abs(as_f64(est.alpha.sub(truth.alpha)))
            << " beta=" << std::abs(as_f64(est.beta.sub(truth.beta))) << "\n\n";
}

} // namespace

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };

  const Series returns = simulate_garch11(truth, 50, 20260225ULL, cfg);
  const Series variance_guess = returns.var(false).maximum(1e-12);
  const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const Series alpha_guess = scalar(0.15, cfg);
  const Series beta_guess = scalar(0.70, cfg);
  const Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);
  const auto objective = [&](const Series& x_unconstrained) {
    return garch11_negloglik(returns, decode_params(x_unconstrained));
  };

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
  };

  const ParameterPack<Series> initial({
      {"log_omega", x0.select(0, 0).reshape({1})},
      {"logit_alpha", x0.select(0, 1).reshape({1})},
      {"logit_beta", x0.select(0, 2).reshape({1})},
  });

  OptimaOptions sqp_options;
  sqp_options.algorithm = OptimaAlgorithm::SequentialQP;
  sqp_options.sequential_qp.max_iterations = 15;
  sqp_options.sequential_qp.constraint_tolerance = 1e-8;
  sqp_options.sequential_qp.optimality_tolerance = 1e-6;
  sqp_options.sequential_qp.active_set_tolerance = 1e-6;
  sqp_options.sequential_qp.regularization = 1e-8;
  sqp_options.sequential_qp.max_line_search_iterations = 12;

  OptimaOptions ip_options;
  ip_options.algorithm = OptimaAlgorithm::InteriorPoint;
  ip_options.interior_point.max_iterations = 20;
  ip_options.interior_point.initial_barrier_parameter = 1e-2;
  ip_options.interior_point.minimum_barrier_parameter = 1e-8;
  ip_options.interior_point.barrier_decay = 0.2;
  ip_options.interior_point.constraint_tolerance = 1e-8;
  ip_options.interior_point.optimality_tolerance = 1e-6;
  ip_options.interior_point.max_line_search_iterations = 16;

  NelderMeadConfig nm_options;
  nm_options.position_tolerance = 1e-7;
  nm_options.value_tolerance = 1e-7;
  nm_options.max_iterations = 120;
  nm_options.max_evaluations = 240;

  DifferentialEvolutionPrmmConfig de_options;
  de_options.population_size = 12;
  de_options.elite_count = 4;
  de_options.max_generations = 10;
  de_options.max_evaluations = 0;
  de_options.check_every = 5;
  de_options.relative_improvement_tolerance = 1e-7;
  de_options.display = pluss::subsp::math::optimization::PopulationSolverDisplay::None;
  de_options.rng_seed = 20260226ULL;

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "GARCH(1,1) estimation benchmark on shared likelihood surface\n";
  std::cout << "true    omega=" << as_f64(truth.omega)
            << " alpha=" << as_f64(truth.alpha)
            << " beta=" << as_f64(truth.beta)
            << " persistence=" << as_f64(truth.alpha.add(truth.beta)) << "\n\n";

  const auto nm_out = nelder_mead_minimizer(objective, x0, nm_options);
  const auto de_out = differential_evolution_prmm_minimizer(objective, x0, de_options);

  print_report("SequentialQP", sqp_options, problem, initial, truth);
  print_report("InteriorPoint", ip_options, problem, initial, truth);
  print_report("NelderMead", nm_out, truth);
  print_report("DE_PRMM", de_out, truth);

  return 0;
}
