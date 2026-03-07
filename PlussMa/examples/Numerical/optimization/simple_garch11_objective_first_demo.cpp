#include <pluss/pluss.h>

#include <cstdint>
#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;
using pluss::optim::OptimaOptions;

Series simulate_garch11(const Series& omega,
                        const Series& alpha,
                        const Series& beta,
                        std::int64_t n,
                        std::uint64_t seed,
                        const pluss::Config& cfg) {
  const Series z = pluss::randn({n}, cfg, seed);
  Series returns = pluss::zeros({n}, cfg);
  Series variance = omega / (pluss::make_series(1.0, cfg) - alpha - beta);
  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform);
  for (std::int64_t t = 0; t < n; ++t) {
    const Series r = variance.sqrt() * z.select(0, t);
    returns = returns.put(pluss::arange(t, t + 1, idx_cfg), r.reshape({1}), false);
    variance = omega + alpha * (r * r) + beta * variance;
  }
  return returns;
}

Series garch11_negloglik(const Series& x,
                         const Series& returns,
                         const pluss::Config& cfg) {
  const Series omega = x.select(0, 0).reshape({1});
  const Series alpha = x.select(0, 1).reshape({1});
  const Series beta = x.select(0, 2).reshape({1});
  Series nll = pluss::make_series(0.0, cfg);
  Series variance = omega / (pluss::make_series(1.0, cfg) - alpha - beta);
  const std::int64_t n = returns.numel();
  for (std::int64_t t = 0; t < n; ++t) {
    if (t > 0) {
      const Series prev = returns.select(0, t - 1);
      variance = omega + alpha * (prev * prev) + beta * variance;
    }
    const Series r = returns.select(0, t);
    nll = nll + 0.5 * (variance.log() + (r * r) / variance);
  }
  return nll;
}

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series truth_omega = pluss::make_series(0.02, cfg);
  const Series truth_alpha = pluss::make_series(0.08, cfg);
  const Series truth_beta = pluss::make_series(0.90, cfg);
  const Series returns = simulate_garch11(truth_omega, truth_alpha, truth_beta, 200, 20260306ULL, cfg);
  const auto objective_args = pluss::optim::pack_args(returns, cfg);

  const Series initial = pluss::make_series({0.01, 0.15, 0.70}, cfg);

  OptimaOptions options = pluss::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 120;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-6;
  options.progress_hook = pluss::optim::reports::iter(std::cout);

  const auto constraints = pluss::optim::constraints()
                               .lbs(pluss::make_series({1e-6, 1e-6, 1e-6}, cfg))
                               .ubs(pluss::make_series({1.0, 0.999, 0.999}, cfg))
                               .ln_ineq(
                                   pluss::make_series({0.0, 1.0, 1.0}, cfg).reshape({1, 3}),
                                   pluss::make_series({0.999}, cfg))
                               .nl_ineq([](const pluss::optim::ParameterPack<pluss::optim::Trace>& p) -> pluss::optim::Trace {
                                 return pluss::optim::sum(p.at("x") * p.at("x")) - 2.0;
                               })
                               .build();

  const auto result = pluss::optim::optima(garch11_negloglik, initial, options, constraints, objective_args);

  std::cout << std::fixed << std::setprecision(6);
  pluss::optim::reports::compact_summary(std::cout, result) << '\n';
  const Series x_hat = result.params.at("x").reshape({3});
  std::cout << "estimated omega=" << x_hat.select(0, 0) << '\n';
  std::cout << "estimated alpha=" << x_hat.select(0, 1) << '\n';
  std::cout << "estimated beta=" << x_hat.select(0, 2) << '\n';
  std::cout << "estimated persistence="
            << (x_hat.select(0, 1) + x_hat.select(0, 2)) << '\n';
  return 0;
}
