#include <pluss/pluss.h>

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
  return pluss::value(v, cfg);
}

pluss::Config scalar_config_like(const pluss::Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

pluss::Series logit_series(const pluss::Series& p) {
  const auto cfg = scalar_config_like(p);
  const pluss::Series one = scalar(1.0, cfg);
  return p.log().sub(one.sub(p).log());
}

Garch11Params decode_params(const pluss::Series& unconstrained) {
  const pluss::Series u = unconstrained.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({3});
  const auto cfg = scalar_config_like(u);

  const pluss::Series u0 = u.select(0, 0);
  const pluss::Series u1 = u.select(0, 1);
  const pluss::Series u2 = u.select(0, 2);

  const pluss::Series persistence_cap = scalar(0.999, cfg);
  const pluss::Series one = scalar(1.0, cfg);
  const pluss::Series s1 = u1.sigmoid();
  const pluss::Series s2 = u2.sigmoid();

  Garch11Params p;
  p.omega = u0.exp();
  p.alpha = persistence_cap.mul(s1);
  p.beta = persistence_cap.mul(one.sub(s1)).mul(s2);
  return p;
}

pluss::Series simulate_garch11(
    const Garch11Params& p,
    std::int64_t n,
    std::uint64_t seed,
    const pluss::Config& cfg) {
  const pluss::Series z = pluss::randn({n}, cfg, seed);
  pluss::Series returns = pluss::zeros({n}, cfg);

  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(cfg.platform);
  const pluss::Series one = scalar(1.0, cfg);
  pluss::Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));

  for (std::int64_t t = 0; t < n; ++t) {
    const pluss::Series idx = pluss::arange(t, t + 1, idx_cfg);
    const pluss::Series eps = z.select(0, t);
    const pluss::Series r = variance.maximum(1e-12).sqrt().mul(eps);

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

std::string to_string(pluss::subsp::math::optimization::SolverStatus status) {
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

double as_f64(const pluss::Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series one = scalar(1.0, cfg);

  const Garch11Params truth{
      scalar(0.02, cfg),
      scalar(0.08, cfg),
      scalar(0.90, cfg),
  };

  const pluss::Series returns = simulate_garch11(truth, 3000, 20260225ULL, cfg);

  const pluss::Series persistence_cap = scalar(0.999, cfg);
  const pluss::Series variance_guess = returns.var(false).maximum(1e-12);
  const pluss::Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const pluss::Series alpha_guess = scalar(0.15, cfg);
  const pluss::Series beta_guess = scalar(0.70, cfg);

  const pluss::Series s1_guess = alpha_guess.div(persistence_cap).clamp_min(1e-6).clamp_max(1.0 - 1e-6);
  const pluss::Series s2_guess =
      beta_guess.div(persistence_cap.mul(one.sub(s1_guess))).clamp_min(1e-6).clamp_max(1.0 - 1e-6);

  const pluss::Series x0 = omega_guess.log().reshape({1}).unite(
      std::vector<pluss::Series>{
          logit_series(s1_guess).reshape({1}),
          logit_series(s2_guess).reshape({1}),
      },
      0);

  NelderMeadConfig cfg_nm;
  cfg_nm.position_tolerance = 1e-8;
  cfg_nm.value_tolerance = 1e-8;
  cfg_nm.max_iterations = 6000;
  cfg_nm.max_evaluations = 12000;
  cfg_nm.display = SolverDisplay::Iter;

  const auto out = nelder_mead_minimizer(
      [&](const pluss::Series& x_unconstrained) {
        const auto p = decode_params(x_unconstrained);
        return garch11_negloglik(returns, p);
      },
      x0,
      cfg_nm);

  const Garch11Params est = decode_params(out.minimizer);

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "GARCH(1,1) estimation via nelder_mead_minimizer\n";
  std::cout << "status=" << to_string(out.status)
            << " iterations=" << out.diagnostics.iterations
            << " evaluations=" << out.diagnostics.evaluations
            << " objective=" << out.minimum << "\n\n";

  std::cout << "true    omega=" << as_f64(truth.omega)
            << " alpha=" << as_f64(truth.alpha)
            << " beta=" << as_f64(truth.beta)
            << " persistence=" << as_f64(truth.alpha.add(truth.beta)) << "\n";
  std::cout << "fitted  omega=" << as_f64(est.omega)
            << " alpha=" << as_f64(est.alpha)
            << " beta=" << as_f64(est.beta)
            << " persistence=" << as_f64(est.alpha.add(est.beta)) << "\n";
  std::cout << "abs err omega=" << std::abs(as_f64(est.omega.sub(truth.omega)))
            << " alpha=" << std::abs(as_f64(est.alpha.sub(truth.alpha)))
            << " beta=" << std::abs(as_f64(est.beta.sub(truth.beta))) << "\n";

  return 0;
}
