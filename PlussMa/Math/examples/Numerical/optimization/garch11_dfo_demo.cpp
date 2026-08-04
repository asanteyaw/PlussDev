#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Garch11Params {
  pluss::Series omega{pluss::Series::undefined()};
  pluss::Series alpha{pluss::Series::undefined()};
  pluss::Series beta{pluss::Series::undefined()};
};

pluss::Series scalar(double v, const pluss::Config& cfg) {
  return pluss::make_series(v, cfg);
}

pluss::Config scalar_config_like(const pluss::Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

pluss::Series as_param_vector(const Garch11Params& p) {
  return p.omega.reshape({1}).unite(
      std::vector<pluss::Series>{
          p.alpha.reshape({1}),
          p.beta.reshape({1}),
      },
      0);
}

pluss::Series logit_series(const pluss::Series& p) {
  const auto cfg = scalar_config_like(p);
  const pluss::Series one = scalar(1.0, cfg);
  return p.log().sub(one.sub(p).log());
}

Garch11Params decode_params(const pluss::Series& unconstrained) {
  const pluss::Series u = unconstrained.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({3});
  const auto cfg = scalar_config_like(u);

  const pluss::Series persistence_cap = scalar(0.999, cfg);
  const pluss::Series one = scalar(1.0, cfg);

  const pluss::Series s1 = u.select(0, 1).sigmoid();
  const pluss::Series s2 = u.select(0, 2).sigmoid();

  Garch11Params p;
  p.omega = u.select(0, 0).exp();
  p.alpha = persistence_cap.mul(s1);
  p.beta = persistence_cap.mul(one.sub(s1)).mul(s2);
  return p;
}

pluss::Series encode_guess(
    const pluss::Series& omega_guess,
    const pluss::Series& alpha_guess,
    const pluss::Series& beta_guess) {
  const auto cfg = scalar_config_like(omega_guess);
  const pluss::Series one = scalar(1.0, cfg);
  const pluss::Series persistence_cap = scalar(0.999, cfg);

  const pluss::Series s1 = alpha_guess.div(persistence_cap).clamp_min(1e-6).clamp_max(1.0 - 1e-6);
  const pluss::Series s2 = beta_guess
                               .div(persistence_cap.mul(one.sub(s1)))
                               .clamp_min(1e-6)
                               .clamp_max(1.0 - 1e-6);

  const pluss::Series head = omega_guess.log().reshape({1});
  return head.unite(
      std::vector<pluss::Series>{
          logit_series(s1).reshape({1}),
          logit_series(s2).reshape({1}),
      },
      0);
}

pluss::Series simulate_garch11(
    const Garch11Params& p,
    std::int64_t n,
    std::uint64_t seed,
    const pluss::Config& cfg) {
  const pluss::Series z = pluss::series::randn({n}, cfg, seed);
  pluss::Series returns = pluss::series::zeros({n}, cfg);

  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(cfg.platform);
  const pluss::Series one = scalar(1.0, cfg);
  pluss::Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));

  for (std::int64_t t = 0; t < n; ++t) {
    const pluss::Series idx = pluss::series::arange(t, t + 1, idx_cfg);
    const pluss::Series r = variance.maximum(1e-12).sqrt().mul(z.select(0, t));

    returns = returns.put(idx, r.reshape({1}), false);
    variance = p.omega.add(p.alpha.mul(r.mul(r))).add(p.beta.mul(variance));
  }

  return returns;
}

pluss::Series garch11_negloglik(const pluss::Series& returns, const Garch11Params& p) {
  const auto cfg = scalar_config_like(returns);
  const pluss::Series one = scalar(1.0, cfg);

  pluss::Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));
  pluss::Series nll = scalar(0.0, cfg);

  const std::int64_t n = returns.numel();
  for (std::int64_t t = 0; t < n; ++t) {
    if (t > 0) {
      const pluss::Series prev = returns.select(0, t - 1);
      variance = p.omega.add(p.alpha.mul(prev.mul(prev))).add(p.beta.mul(variance));
    }

    const pluss::Series var_clamped = variance.maximum(1e-12);
    const pluss::Series r = returns.select(0, t);
    nll = nll.add(var_clamped.log().add(r.mul(r).div(var_clamped)).mul(0.5));
  }

  return nll;
}

double as_f64(const pluss::Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

std::string to_string(pluss::subsp::math::optimization::PopulationSolverStatus status) {
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

void print_report(
    const std::string& name,
    const pluss::subsp::math::optimization::PopulationSolverOutcome& out,
    const Garch11Params& truth,
    const Garch11Params& est) {
  const double err_omega = as_f64(est.omega.sub(truth.omega).abs());
  const double err_alpha = as_f64(est.alpha.sub(truth.alpha).abs());
  const double err_beta = as_f64(est.beta.sub(truth.beta).abs());
  const double truth_persistence = as_f64(truth.alpha.add(truth.beta));
  const double est_persistence = as_f64(est.alpha.add(est.beta));

  std::cout << name
            << " status=" << to_string(out.status)
            << " iterations=" << out.diagnostics.iterations
            << " evaluations=" << out.diagnostics.evaluations
            << " objective=" << out.minimum
            << "\n";

  std::cout << "  fitted omega=" << as_f64(est.omega)
            << " alpha=" << as_f64(est.alpha)
            << " beta=" << as_f64(est.beta)
            << " persistence=" << est_persistence
            << " (truth=" << truth_persistence << ")\n";

  std::cout << "  abs err omega=" << err_omega
            << " alpha=" << err_alpha
            << " beta=" << err_beta
            << "\n\n";
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };

  const pluss::Series returns = simulate_garch11(truth, 2000, 20260225ULL, cfg);

  const pluss::Series variance_guess = returns.var(false).maximum(1e-12);
  const pluss::Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const pluss::Series alpha_guess = scalar(0.12, cfg);
  const pluss::Series beta_guess = scalar(0.78, cfg);
  const pluss::Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);

  DifferentialEvolutionPrmmConfig de_cfg;
  de_cfg.population_size = 96;
  de_cfg.elite_count = 20;
  de_cfg.max_generations = 260;
  de_cfg.max_evaluations = 0;
  de_cfg.check_every = 10;
  de_cfg.relative_improvement_tolerance = 1e-8;
  de_cfg.display = PopulationSolverDisplay::None;
  de_cfg.rng_seed = 20260226ULL;

  ParticleSwarmConfig pso_cfg;
  pso_cfg.population_size = 84;
  pso_cfg.max_generations = 320;
  pso_cfg.max_evaluations = 0;
  pso_cfg.check_every = 10;
  pso_cfg.relative_improvement_tolerance = 1e-8;
  pso_cfg.center_particle = true;
  pso_cfg.linear_inertia_schedule = true;
  pso_cfg.time_varying_acceleration = true;
  pso_cfg.display = PopulationSolverDisplay::None;
  pso_cfg.rng_seed = 20260227ULL;

  const auto objective = [&](const pluss::Series& x_unconstrained) {
    return garch11_negloglik(returns, decode_params(x_unconstrained));
  };

  const auto out_de = differential_evolution_prmm_minimizer(objective, x0, de_cfg);
  const auto out_pso = particle_swarm_minimizer(objective, x0, pso_cfg);

  const Garch11Params est_de = decode_params(out_de.minimizer);
  const Garch11Params est_pso = decode_params(out_pso.minimizer);

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "GARCH(1,1) estimation via population DFO solvers\n";
  std::cout << "true    omega=" << as_f64(truth.omega)
            << " alpha=" << as_f64(truth.alpha)
            << " beta=" << as_f64(truth.beta)
            << " persistence=" << as_f64(truth.alpha.add(truth.beta))
            << "\n\n";

  print_report("de_prmm", out_de, truth, est_de);
  print_report("pso", out_pso, truth, est_pso);

  return 0;
}
