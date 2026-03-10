#include <pluss/ma.h>

#include <algorithm>
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
using pluss::optim::ConstrainedProblem;
using pluss::optim::OptimaAlgorithm;
using pluss::optim::OptimaExitFlag;
using pluss::optim::OptimaHessianApproximation;
using pluss::optim::OptimaLinearSystemSolver;
using pluss::optim::OptimaLinearSystemStructure;
using pluss::optim::OptimaOptions;
using pluss::optim::OptimaProblemScaling;
using pluss::optim::ParameterPack;
using pluss::optim::Trace;
using pluss::optim::optima;
using pluss::optim::trace;
using pluss::optim::square;

struct Garch11Params {
  Series omega{Series::undefined()};
  Series alpha{Series::undefined()};
  Series beta{Series::undefined()};
};

struct RunStats {
  OptimaExitFlag exit{OptimaExitFlag::Stopped};
  double objective{std::numeric_limits<double>::infinity()};
  double violation{std::numeric_limits<double>::infinity()};
  double first_order{std::numeric_limits<double>::infinity()};
  double persistence{std::numeric_limits<double>::quiet_NaN()};
};

struct Checkpoint {
  std::string name{};
  bool pass{false};
  std::string detail{};
};

Series scalar(double v, const pluss::Config& cfg) { return pluss::make_series(v, cfg); }

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
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
  return decode_params(pluss::optim::flatten_values(params));
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
  for (std::int64_t t = 0; t < returns.numel(); ++t) {
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

RunStats run_garch_optima(const Series& returns, const Series& x0, const OptimaOptions& options) {
  ConstrainedProblem problem;
  problem.objective.autodiff = [returns](const ParameterPack<Trace>& p) -> Trace {
    const Trace omega = exp(p.at("log_omega"));
    const Trace s1 = sigmoid(p.at("logit_alpha"));
    const Trace s2 = sigmoid(p.at("logit_beta"));
    const Trace alpha = 0.999 * s1;
    const Trace beta = 0.999 * (1.0 - s1) * s2;

    Trace variance = omega / clamp(1.0 - alpha - beta, 1e-8, 1e8);
    Trace nll = trace(pluss::make_series(0.0).to(ScalarType::Float64), false);
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
  const auto result = optima(problem, initial, options);
  const auto decoded = decode_params(result.params);
  return RunStats{
      result.exit_flag,
      result.evaluation.value.item<double>(),
      result.evaluation.max_constraint_violation,
      result.output.first_order_optimality,
      as_f64(decoded.alpha.add(decoded.beta)),
  };
}

std::string var_name(std::int64_t i) { return "x_" + std::to_string(i); }

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

ParameterPack<Series> make_scalar_blocks(const std::vector<double>& values) {
  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    blocks.push_back({var_name(static_cast<std::int64_t>(i)), pluss::make_series(values[i]).to(ScalarType::Float64).reshape({1})});
  }
  return ParameterPack<Series>(std::move(blocks));
}

ConstrainedProblem make_structured_problem(std::int64_t n,
                                           const std::vector<double>& target,
                                           double difference_cap) {
  ConstrainedProblem problem;
  problem.objective.autodiff = [target, n](const ParameterPack<Trace>& p) -> Trace {
    Trace value = trace(pluss::make_series(0.0).to(ScalarType::Float64), false);
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
    const std::int64_t r0 = 2 * i;
    const std::int64_t r1 = r0 + 1;
    aineq[static_cast<std::size_t>(r0 * n + i)] = 1.0;
    aineq[static_cast<std::size_t>(r0 * n + i + 1)] = -1.0;
    aineq[static_cast<std::size_t>(r1 * n + i)] = -1.0;
    aineq[static_cast<std::size_t>(r1 * n + i + 1)] = 1.0;
  }
  problem.linear.Aineq = pluss::make_series(aineq, {2 * (n - 1), n}).to(ScalarType::Float64);
  problem.linear.bineq = pluss::make_series(bineq, {2 * (n - 1)}).to(ScalarType::Float64);

  std::vector<typename ParameterPack<Series>::Block> lower_blocks;
  std::vector<typename ParameterPack<Series>::Block> upper_blocks;
  lower_blocks.reserve(static_cast<std::size_t>(n));
  upper_blocks.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    lower_blocks.push_back({var_name(i), pluss::make_series(0.0).to(ScalarType::Float64).reshape({1})});
    upper_blocks.push_back({var_name(i), pluss::make_series(1.0).to(ScalarType::Float64).reshape({1})});
  }
  problem.bounds.lower = ParameterPack<Series>(std::move(lower_blocks));
  problem.bounds.upper = ParameterPack<Series>(std::move(upper_blocks));
  return problem;
}

double max_abs_error_to_target(const ParameterPack<Series>& params, const std::vector<double>& target) {
  double best = 0.0;
  for (std::size_t i = 0; i < target.size(); ++i) {
    best = std::max(best, std::abs(as_f64(params.at(var_name(static_cast<std::int64_t>(i)))) - target[i]));
  }
  return best;
}

} // namespace

int main(int argc, char** argv) {
  const bool strict = argc > 1 && std::string(argv[1]) == "--strict";
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  std::vector<Checkpoint> checks;
  checks.reserve(10);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };
  const std::vector<std::pair<std::int64_t, std::uint64_t>> tiers{
      {40, 20260225ULL},
      {200, 20260235ULL},
      {800, 20260245ULL},
  };

  for (const auto& [n, seed] : tiers) {
    const Series returns = simulate_garch11(truth, n, seed, cfg);
    const Series variance_guess = returns.var(false).maximum(1e-12);
    const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
    const Series alpha_guess = scalar(0.15, cfg);
    const Series beta_guess = scalar(0.70, cfg);
    const Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);

    OptimaOptions sqp;
    sqp.algorithm = OptimaAlgorithm::SequentialQP;
    sqp.sequential_qp.max_iterations = n >= 800 ? 600 : 450;
    sqp.sequential_qp.constraint_tolerance = 1e-8;
    sqp.sequential_qp.optimality_tolerance = 1e-6;
    sqp.sequential_qp.active_set_tolerance = 1e-6;
    sqp.sequential_qp.regularization = 1e-8;
    sqp.sequential_qp.max_line_search_iterations = 16;
    sqp.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

    OptimaOptions ip;
    ip.algorithm = OptimaAlgorithm::InternalProtocol;
    ip.internal_protocol.max_iterations = n >= 800 ? 260 : 160;
    ip.internal_protocol.initial_barrier_parameter = 1e-2;
    ip.internal_protocol.minimum_barrier_parameter = 1e-8;
    ip.internal_protocol.barrier_decay = 0.2;
    ip.internal_protocol.constraint_tolerance = 1e-8;
    ip.internal_protocol.optimality_tolerance = 1e-6;
    ip.internal_protocol.max_line_search_iterations = 20;
    ip.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

    const RunStats sqp_run = run_garch_optima(returns, x0, sqp);
    const RunStats ip_run = run_garch_optima(returns, x0, ip);

    {
      const bool pass = sqp_run.exit == OptimaExitFlag::Converged && sqp_run.violation <= 1e-8;
      checks.push_back(Checkpoint{
          "GARCH N=" + std::to_string(n) + " SQP",
          pass,
          "exit=" + std::to_string(static_cast<int>(sqp_run.exit)) +
              " obj=" + std::to_string(sqp_run.objective) +
              " viol=" + std::to_string(sqp_run.violation)});
    }

    if (n == 40) {
      const bool pass = ip_run.violation <= 1e-8 &&
                        (ip_run.exit == OptimaExitFlag::Converged ||
                         (ip_run.exit == OptimaExitFlag::MaxIterations &&
                          ip_run.objective <= sqp_run.objective + 1e-2 &&
                          ip_run.first_order <= 5e-3));
      checks.push_back(Checkpoint{
          "GARCH N=40 InternalProtocol",
          pass,
          "exit=" + std::to_string(static_cast<int>(ip_run.exit)) +
              " obj_gap=" + std::to_string(ip_run.objective - sqp_run.objective) +
              " first_order=" + std::to_string(ip_run.first_order)});
    } else {
      const bool pass = ip_run.exit == OptimaExitFlag::Converged &&
                        ip_run.violation <= 1e-8 &&
                        ip_run.first_order <= 5e-3;
      checks.push_back(Checkpoint{
          "GARCH N=" + std::to_string(n) + " InternalProtocol",
          pass,
          "exit=" + std::to_string(static_cast<int>(ip_run.exit)) +
              " obj=" + std::to_string(ip_run.objective) +
              " first_order=" + std::to_string(ip_run.first_order)});
    }
  }

  {
    const std::int64_t n = 64;
    const auto target = make_target_profile(n);
    const auto initial = make_scalar_blocks(make_infeasible_start(n));
    const auto problem = make_structured_problem(n, target, 0.12);

    OptimaOptions sqp;
    sqp.algorithm = OptimaAlgorithm::SequentialQP;
    sqp.sequential_qp.max_iterations = 550;
    sqp.sequential_qp.constraint_tolerance = 1e-8;
    sqp.sequential_qp.optimality_tolerance = 1e-6;
    sqp.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    sqp.sequential_qp.system_structure = OptimaLinearSystemStructure::Sparse;
    sqp.sequential_qp.linear_system_solver = OptimaLinearSystemSolver::Auto;
    sqp.sequential_qp.hessian_approximation = OptimaHessianApproximation::Bfgs;

    OptimaOptions ip;
    ip.algorithm = OptimaAlgorithm::InternalProtocol;
    ip.internal_protocol.max_iterations = 650;
    ip.internal_protocol.constraint_tolerance = 1e-8;
    ip.internal_protocol.optimality_tolerance = 1e-6;
    ip.internal_protocol.initial_barrier_parameter = 1e-2;
    ip.internal_protocol.minimum_barrier_parameter = 1e-8;
    ip.internal_protocol.enable_feasibility_phase = true;
    ip.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    ip.internal_protocol.system_structure = OptimaLinearSystemStructure::Auto;
    ip.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::Auto;
    ip.internal_protocol.hessian_approximation = OptimaHessianApproximation::Lbfgs;
    ip.internal_protocol.lbfgs_memory = 10;
    ip.internal_protocol.max_krylov_iterations = 2 * (n - 1);

    const auto sqp_run = optima(problem, initial, sqp);
    const auto ip_run = optima(problem, initial, ip);

    const bool sqp_pass = sqp_run.exit_flag == OptimaExitFlag::Converged &&
                          sqp_run.evaluation.max_constraint_violation <= 1e-8 &&
                          max_abs_error_to_target(sqp_run.params, target) <= 0.01;
    checks.push_back(Checkpoint{
        "Structured n=64 SQP",
        sqp_pass,
        "exit=" + std::to_string(static_cast<int>(sqp_run.exit_flag)) +
            " err=" + std::to_string(max_abs_error_to_target(sqp_run.params, target))});

    const bool ip_pass = ip_run.exit_flag == OptimaExitFlag::Converged &&
                         ip_run.evaluation.max_constraint_violation <= 1e-8 &&
                         max_abs_error_to_target(ip_run.params, target) <= 0.01;
    checks.push_back(Checkpoint{
        "Structured n=64 InternalProtocol",
        ip_pass,
        "exit=" + std::to_string(static_cast<int>(ip_run.exit_flag)) +
            " err=" + std::to_string(max_abs_error_to_target(ip_run.params, target))});
  }

  std::size_t passed = 0;
  for (const auto& check : checks) if (check.pass) ++passed;
  const double score = checks.empty() ? 0.0 : (100.0 * static_cast<double>(passed) / static_cast<double>(checks.size()));

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "# Optima Clone-Parity Scorecard\n\n";
  std::cout << "| Check | Pass | Detail |\n";
  std::cout << "|---|---|---|\n";
  for (const auto& check : checks) {
    std::cout << "| " << check.name << " | " << (check.pass ? "yes" : "no") << " | " << check.detail << " |\n";
  }
  std::cout << "\nScore: " << passed << "/" << checks.size() << " = " << score << "%\n";
  if (strict && passed != checks.size()) {
    std::cout << "Strict mode: failed\n";
    return 2;
  }
  return 0;
}
