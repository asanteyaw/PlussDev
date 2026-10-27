#include <pluss/ma.h>

#include <cstdint>
#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaOptions;

pluss::Series simulate_garch11(const pluss::Series& omega,
                        const pluss::Series& alpha,
                        const pluss::Series& beta,
                        std::int64_t n,
                        std::uint64_t seed,
                        const pluss::Config& cfg) {
  const pluss::Series z = pluss::series::randn({n}, cfg, seed);
  pluss::Series returns = pluss::series::zeros({n}, cfg);
  pluss::Series variance = omega / (pluss::make_series(1.0, cfg) - alpha - beta);
  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform);
  for (std::int64_t t = 0; t < n; ++t) {
    const pluss::Series r = variance.sqrt() * z.select(0, t);
    returns = returns.put(pluss::series::arange(t, t + 1, idx_cfg), r.reshape({1}), false);
    variance = omega + alpha * (r * r) + beta * variance;
  }
  return returns;
}

pluss::Series garch11_negloglik(const pluss::Series& x,
                                const pluss::Series& returns,
                                const pluss::Config& cfg) {
  const pluss::Series omega = x.select(0, 0).reshape({1});
  const pluss::Series alpha = x.select(0, 1).reshape({1});
  const pluss::Series beta = x.select(0, 2).reshape({1});
  pluss::Series nll = pluss::make_series(0.0, cfg);
  pluss::Series variance = omega / (pluss::make_series(1.0, cfg) - alpha - beta);
  const std::int64_t n = returns.numel();
  for (std::int64_t t = 0; t < n; ++t) {
    if (t > 0) {
      const pluss::Series prev = returns.select(0, t - 1);
      variance = omega + alpha * (prev * prev) + beta * variance;
    }
    const pluss::Series r = returns.select(0, t);
    nll = nll + 0.5 * (variance.log() + (r * r) / variance);
  }
  return nll;
}

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series truth_omega = pluss::make_series(0.02, cfg);
  const pluss::Series truth_alpha = pluss::make_series(0.08, cfg);
  const pluss::Series truth_beta = pluss::make_series(0.90, cfg);

  const pluss::Series returns = simulate_garch11(truth_omega, truth_alpha, truth_beta, 200, 20260306ULL, cfg);
  const ConstrainedProblem problem =
      pluss::subsp::math::optim::problem()
          .objective([returns, cfg](const pluss::subsp::math::optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
            const auto omega = p.at("omega");
            const auto alpha = p.at("alpha");
            const auto beta = p.at("beta");
            auto nll = pluss::make_series(0.0, cfg);
            auto variance = omega / (pluss::make_series(1.0, cfg) - alpha - beta);
            const std::int64_t n = returns.numel();
            for (std::int64_t t = 0; t < n; ++t) {
              if (t > 0) {
                const auto prev = returns.select(0, t - 1);
                variance = omega + alpha * (prev * prev) + beta * variance;
              }
              const auto r = returns.select(0, t);
              nll = nll + 0.5 * (variance.log() + (r * r) / variance);
            }
            return nll;
          })
          .ln_ineq(
              pluss::make_series({0.0, 1.0, 1.0}, cfg).reshape({1, 3}),
              pluss::make_series({0.999}, cfg))
          .nl_ineq([](const pluss::subsp::math::optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
            const auto omega = p.at("omega");
            const auto alpha = p.at("alpha");
            const auto beta = p.at("beta");
            return omega * omega + alpha * alpha + beta * beta - 2.0;
          })
          .lbs(pluss::subsp::math::optim::pack_scalars({{"omega", pluss::make_series(1e-6, cfg)},
                                           {"alpha", pluss::make_series(1e-6, cfg)},
                                           {"beta", pluss::make_series(1e-6, cfg)}}))
          .ubs(pluss::subsp::math::optim::pack_scalars({{"omega", pluss::make_series(1.0, cfg)},
                                           {"alpha", pluss::make_series(0.999, cfg)},
                                           {"beta", pluss::make_series(0.999, cfg)}}))
          .build();

  const auto initial = pluss::subsp::math::optim::pack_scalars({{"omega", pluss::make_series(0.01, cfg)},
                                                    {"alpha", pluss::make_series(0.15, cfg)},
                                                    {"beta", pluss::make_series(0.70, cfg)}});

  OptimaOptions options = pluss::subsp::math::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 120;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-6;

  const auto result = pluss::subsp::math::optim::optima(problem, initial, options);

  std::cout << std::fixed << std::setprecision(6);
  pluss::subsp::math::optim::reports::compact_summary(std::cout, result) << '\n';
  const pluss::Series omega_hat = result.params.at("omega");
  const pluss::Series alpha_hat = result.params.at("alpha");
  const pluss::Series beta_hat = result.params.at("beta");
  std::cout << "estimated omega=" << omega_hat << '\n';
  std::cout << "estimated alpha=" << alpha_hat << '\n';
  std::cout << "estimated beta=" << beta_hat << '\n';
  std::cout << "estimated persistence=" << (alpha_hat + beta_hat) << '\n';

  return 0;
}
