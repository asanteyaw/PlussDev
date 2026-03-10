#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::clamp;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::evaluate_problem;
using pluss::subsp::math::optim::exp;
using pluss::subsp::math::optim::log;
using pluss::subsp::math::optim::sigmoid;
using pluss::subsp::math::optim::trace;

struct Garch11Params {
  Series omega{Series::undefined()};
  Series alpha{Series::undefined()};
  Series beta{Series::undefined()};
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
  problem.objective.autodiff = [returns](const ParameterPack<Trace>& p) -> Trace {
    const Trace omega = exp(p.at("log_omega"));
    const Trace s1 = sigmoid(p.at("logit_alpha"));
    const Trace s2 = sigmoid(p.at("logit_beta"));
    const Trace alpha = 0.999 * s1;
    const Trace beta = 0.999 * (1.0 - s1) * s2;

    Trace variance = omega / clamp(1.0 - alpha - beta, 1e-8, 1e8);
    Trace nll = trace(scalar(0.0, scalar_config_like(returns)), false);

    const std::int64_t n = returns.numel();
    for (std::int64_t t = 0; t < n; ++t) {
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

  const ParameterPack<Series> params({
      {"log_omega", x0.select(0, 0).reshape({1})},
      {"logit_alpha", x0.select(0, 1).reshape({1})},
      {"logit_beta", x0.select(0, 2).reshape({1})},
  });

  const auto eval = evaluate_problem(problem, params);

  REQUIRE(eval.status == EvalStatus::Ok);
  REQUIRE(eval.value.item<double>() > 0.0);
  REQUIRE(eval.gradient.at("log_omega").abs().item<double>() > 1e-8);
  REQUIRE(eval.gradient.at("logit_alpha").abs().item<double>() > 1e-8);
  REQUIRE(eval.gradient.at("logit_beta").abs().item<double>() > 1e-8);
}
