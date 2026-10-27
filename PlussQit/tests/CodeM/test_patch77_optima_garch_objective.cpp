#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <cstdint>
#include <vector>


namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::diff::clamp;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::evaluate_problem;

struct Garch11Params {
  Series omega{pluss::series::undefined()};
  Series alpha{pluss::series::undefined()};
  Series beta{pluss::series::undefined()};
};

Series scalar(double v, const pluss::Config& cfg) {
  return make_series(v, cfg);
}

pluss::Config scalar_config_like(const Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

Series logit_series(const Series& p) {
  const auto cfg = scalar_config_like(p);
  const Series one = scalar(1.0, cfg);
  return p.log().sub(one.sub(p).log());
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
  const Series z = pluss::series::randn({n}, cfg, seed);
  Series returns = pluss::series::zeros({n}, cfg);

  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform);
  const Series one = scalar(1.0, cfg);
  Series variance = p.omega.div(one.sub(p.alpha).sub(p.beta).maximum(1e-8));

  for (std::int64_t t = 0; t < n; ++t) {
    const Series idx = pluss::series::arange(t, t + 1, idx_cfg);
    const Series eps = z.select(0, t);
    const Series r = variance.maximum(1e-12).sqrt().mul(eps);
    returns = returns.put(idx, r.reshape({1}), false);
    variance = p.omega.add(p.alpha.mul(r.mul(r))).add(p.beta.mul(variance));
  }

  return returns;
}

} // namespace

TEST_CASE("optima garch objective produces nonzero autodiff gradient through ParameterPack trace path",
          "[optim][optima][garch][autodiff]") {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Params truth{scalar(0.02, cfg), scalar(0.08, cfg), scalar(0.90, cfg)};
  const Series returns = simulate_garch11(truth, 20, 20260225ULL, cfg);

  const Series variance_guess = returns.var(false).maximum(1e-12);
  const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const Series alpha_guess = scalar(0.15, cfg);
  const Series beta_guess = scalar(0.70, cfg);
  const Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);

  ConstrainedProblem problem;
  problem.objective.autodiff = [returns](const ParameterPack<Series>& p) -> Series {
    const Series omega = p.at("log_omega").exp();
    const Series s1 = p.at("logit_alpha").sigmoid();
    const Series s2 = p.at("logit_beta").sigmoid();
    const Series alpha = 0.999 * s1;
    const Series beta = 0.999 * (1.0 - s1) * s2;

    Series variance = omega / (1.0 - alpha - beta).clamp(1e-8, 1e8);
    Series nll = (scalar(0.0, scalar_config_like(returns)));

    const std::int64_t n = returns.numel();
    for (std::int64_t t = 0; t < n; ++t) {
      if (t > 0) {
        const Series prev = returns.select(0, t - 1);
        variance = omega + alpha * prev * prev + beta * variance;
      }
      const Series var_clamped = variance.clamp(1e-12, 1e12);
      const Series r = returns.select(0, t);
      nll = nll + 0.5 * (var_clamped.log() + (r * r) / var_clamped);
    }
    return nll;
  };

  const ParameterPack<Series> params({
      {"log_omega", x0.select(0, 0).reshape({1})},
      {"logit_alpha", x0.select(0, 1).reshape({1})},
      {"logit_beta", x0.select(0, 2).reshape({1})},
  });

  const auto eval = evaluate_problem(problem, params);

  REQUIRE(eval.status == EvalStatus::Ok);
  REQUIRE(as_f64(eval.value) > 0.0);
  REQUIRE(as_f64(eval.gradient.at("log_omega").abs()) > 1e-8);
  REQUIRE(as_f64(eval.gradient.at("logit_alpha").abs()) > 1e-8);
  REQUIRE(as_f64(eval.gradient.at("logit_beta").abs()) > 1e-8);
}

TEST_CASE("optima garch objective produces nonzero gradient through single-trace objective callback",
          "[optim][optima][garch][autodiff]") {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Params truth{scalar(0.02, cfg), scalar(0.08, cfg), scalar(0.90, cfg)};
  const Series returns = simulate_garch11(truth, 20, 20260225ULL, cfg);

  const Series variance_guess = returns.var(false).maximum(1e-12);
  const Series omega_guess = variance_guess.mul(0.05).maximum(1e-6);
  const Series alpha_guess = scalar(0.15, cfg);
  const Series beta_guess = scalar(0.70, cfg);
  const Series x0 = encode_guess(omega_guess, alpha_guess, beta_guess);

  ConstrainedProblem problem = pluss::subsp::math::optim::problem()
                                   .objective([returns](const ParameterPack<Series>& p) -> Series {
                                     const Series omega = p.at("log_omega").exp();
                                     const Series s1 = p.at("logit_alpha").sigmoid();
                                     const Series s2 = p.at("logit_beta").sigmoid();
                                     const Series alpha = 0.999 * s1;
                                     const Series beta = 0.999 * (1.0 - s1) * s2;

                                     Series variance = omega / (1.0 - alpha - beta).clamp(1e-8, 1e8);
                                     Series nll = (scalar(0.0, scalar_config_like(returns)));
                                     const std::int64_t n = returns.numel();
                                     for (std::int64_t t = 0; t < n; ++t) {
                                       if (t > 0) {
                                         const Series prev = returns.select(0, t - 1);
                                         variance = omega + alpha * prev * prev + beta * variance;
                                       }
                                       const Series var_clamped = variance.clamp(1e-12, 1e12);
                                       const Series r = returns.select(0, t);
                                       nll = nll + 0.5 * (var_clamped.log() + (r * r) / var_clamped);
                                     }
                                     return nll;
                                   })
                                   .build();

  const ParameterPack<Series> params({
      {"log_omega", x0.select(0, 0).reshape({1})},
      {"logit_alpha", x0.select(0, 1).reshape({1})},
      {"logit_beta", x0.select(0, 2).reshape({1})},
  });

  const auto eval = evaluate_problem(problem, params);

  REQUIRE(eval.status == EvalStatus::Ok);
  REQUIRE(as_f64(eval.value) > 0.0);
  REQUIRE(as_f64(eval.gradient.at("log_omega").abs()) > 1e-8);
  REQUIRE(as_f64(eval.gradient.at("logit_alpha").abs()) > 1e-8);
  REQUIRE(as_f64(eval.gradient.at("logit_beta").abs()) > 1e-8);
}
