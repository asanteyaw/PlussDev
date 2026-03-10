#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-9) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!approx(a[i], b[i], tol)) return false;
  }
  return true;
}

double nadam_first_step_expected(double parameter, double sensitivity, const pluss::nn::optim::NAdamOptions& options) {
  const double mu_t = options.beta1 * (1.0 - 0.5 * std::pow(0.96, options.momentum_decay));
  const double mu_t_next = options.beta1 * (1.0 - 0.5 * std::pow(0.96, 2.0 * options.momentum_decay));
  const double mu_product = mu_t;
  const double mu_product_next = mu_product * mu_t_next;

  const double g = sensitivity;
  const double m = (1.0 - options.beta1) * g;
  const double v = (1.0 - options.beta2) * g * g;
  const double v_hat = v / (1.0 - options.beta2);
  const double g_hat = g / (1.0 - mu_product);
  const double m_hat = m / (1.0 - mu_product_next);
  const double nesterov_m = (1.0 - mu_t) * g_hat + mu_t_next * m_hat;
  const double denom = std::sqrt(v_hat) + options.epsilon;
  return parameter - options.learning_rate * (nesterov_m / denom);
}

struct ScalarModule final : pluss::nn::Module {
  Series weight;

  explicit ScalarModule(const Config& cfg)
      : Module(true), weight(pluss::make_series(1.0, cfg)) {
    parameter_record("weight", weight);
  }
};

bool run_nn_optimizer_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();

  Series sgd_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::SGDOptions sgd_opt{};
  sgd_opt.learning_rate = 0.1;
  pluss::nn::optim::SGD sgd(std::vector<Series*>{&sgd_param}, sgd_opt);
  sgd.step(std::vector<Series>{pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(sgd_param), 0.95)) {
    if (err) *err << "SGD explicit sensitivity step mismatch\n";
    return false;
  }

  Series sgd_group_a = pluss::make_series(1.0, cfg);
  Series sgd_group_b = pluss::make_series(1.0, cfg);
  pluss::nn::optim::SGDOptions sgd_group_opt_a{};
  sgd_group_opt_a.learning_rate = 0.1;
  pluss::nn::optim::SGDOptions sgd_group_opt_b{};
  sgd_group_opt_b.learning_rate = 0.01;
  pluss::nn::optim::SGD sgd_groups(std::vector<pluss::nn::optim::SGDGroup>{
      pluss::nn::optim::SGDGroup{std::vector<Series*>{&sgd_group_a}, sgd_group_opt_a},
      pluss::nn::optim::SGDGroup{std::vector<Series*>{&sgd_group_b}, sgd_group_opt_b},
  });
  sgd_groups.step(std::vector<Series>{pluss::make_series(1.0, cfg), pluss::make_series(1.0, cfg)});
  if (!approx(scalar_f64(sgd_group_a), 0.9, 1e-9) || !approx(scalar_f64(sgd_group_b), 0.99, 1e-9)) {
    if (err) *err << "SGD parameter-group learning rates mismatch\n";
    return false;
  }

  Series clip_param = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}, cfg);
  pluss::nn::optim::SGDOptions clip_opt{};
  clip_opt.learning_rate = 1.0;
  clip_opt.clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
  clip_opt.clip.threshold = 1.0;
  pluss::nn::optim::SGD sgd_clip(std::vector<Series*>{&clip_param}, clip_opt);
  sgd_clip.step(std::vector<Series>{pluss::make_series(std::vector<double>{3.0, 4.0}, {2}, cfg)});
  if (!approx_vec(vec_f64(clip_param), std::vector<double>{0.4, 0.2}, 1e-8)) {
    if (err) *err << "SGD norm clipping mismatch\n";
    return false;
  }

  Series adam_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamOptions adam_opt{};
  adam_opt.learning_rate = 0.1;
  pluss::nn::optim::Adam adam(std::vector<Series*>{&adam_param}, adam_opt);
  const Series adam_sensitivity = pluss::make_series(0.5, cfg);
  adam.step(std::vector<Series>{adam_sensitivity});
  adam.step(std::vector<Series>{adam_sensitivity});
  if (!approx(scalar_f64(adam_param), 0.8, 1e-8)) {
    if (err) *err << "Adam update mismatch\n";
    return false;
  }

  Series adam_group_a = pluss::make_series(1.0, cfg);
  Series adam_group_b = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamOptions adam_group_opt_a{};
  adam_group_opt_a.learning_rate = 0.1;
  pluss::nn::optim::AdamOptions adam_group_opt_b{};
  adam_group_opt_b.learning_rate = 0.01;
  pluss::nn::optim::Adam adam_groups(std::vector<pluss::nn::optim::AdamGroup>{
      pluss::nn::optim::AdamGroup{std::vector<Series*>{&adam_group_a}, adam_group_opt_a},
      pluss::nn::optim::AdamGroup{std::vector<Series*>{&adam_group_b}, adam_group_opt_b},
  });
  adam_groups.step(std::vector<Series>{pluss::make_series(0.5, cfg), pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(adam_group_a), 0.9, 1e-8) || !approx(scalar_f64(adam_group_b), 0.99, 1e-8)) {
    if (err) *err << "Adam parameter-group learning rates mismatch\n";
    return false;
  }

  Series nadam_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::NAdamOptions nadam_opt{};
  nadam_opt.learning_rate = 0.1;
  pluss::nn::optim::NAdam nadam(std::vector<Series*>{&nadam_param}, nadam_opt);
  nadam.step(std::vector<Series>{adam_sensitivity});
  const double nadam_expected = nadam_first_step_expected(1.0, 0.5, nadam_opt);
  if (!approx(scalar_f64(nadam_param), nadam_expected, 1e-8)) {
    if (err) *err << "NAdam update mismatch\n";
    return false;
  }

  Series nadam_group_a = pluss::make_series(1.0, cfg);
  Series nadam_group_b = pluss::make_series(1.0, cfg);
  pluss::nn::optim::NAdamOptions nadam_group_opt_a{};
  nadam_group_opt_a.learning_rate = 0.1;
  pluss::nn::optim::NAdamOptions nadam_group_opt_b{};
  nadam_group_opt_b.learning_rate = 0.01;
  pluss::nn::optim::NAdam nadam_groups(std::vector<pluss::nn::optim::NAdamGroup>{
      pluss::nn::optim::NAdamGroup{std::vector<Series*>{&nadam_group_a}, nadam_group_opt_a},
      pluss::nn::optim::NAdamGroup{std::vector<Series*>{&nadam_group_b}, nadam_group_opt_b},
  });
  nadam_groups.step(std::vector<Series>{adam_sensitivity, adam_sensitivity});
  const double nadam_expected_a = nadam_first_step_expected(1.0, 0.5, nadam_group_opt_a);
  const double nadam_expected_b = nadam_first_step_expected(1.0, 0.5, nadam_group_opt_b);
  if (!approx(scalar_f64(nadam_group_a), nadam_expected_a, 1e-8) ||
      !approx(scalar_f64(nadam_group_b), nadam_expected_b, 1e-8)) {
    if (err) *err << "NAdam parameter-group learning rates mismatch\n";
    return false;
  }

  Series adamw_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamWOptions adamw_opt{};
  adamw_opt.learning_rate = 0.1;
  adamw_opt.weight_decay = 0.1;
  pluss::nn::optim::AdamW adamw(std::vector<Series*>{&adamw_param}, adamw_opt);
  adamw.step(std::vector<Series>{adam_sensitivity});
  if (!approx(scalar_f64(adamw_param), 0.89, 1e-8)) {
    if (err) *err << "AdamW decoupled decay mismatch\n";
    return false;
  }

  Series rms_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::RMSpropOptions rms_opt{};
  rms_opt.learning_rate = 0.01;
  rms_opt.alpha = 0.99;
  pluss::nn::optim::RMSprop rmsprop(std::vector<Series*>{&rms_param}, rms_opt);
  rmsprop.step(std::vector<Series>{adam_sensitivity});
  if (!approx(scalar_f64(rms_param), 0.9, 1e-6)) {
    if (err) *err << "RMSprop update mismatch\n";
    return false;
  }

  Series nadam_stress = pluss::make_series(std::vector<double>{1.0, -2.0, 3.0}, {3}, cfg);
  pluss::nn::optim::NAdam nadam_stress_optim(std::vector<Series*>{&nadam_stress}, nadam_opt);
  for (int i = 0; i < 200; ++i) {
    nadam_stress_optim.step(std::vector<Series>{pluss::make_series(std::vector<double>{0.1, -0.2, 0.3}, {3}, cfg)});
  }
  const Series nadam_stress_finite = nadam_stress.isfinite().all();
  if (!nadam_stress_finite.item<bool>()) {
    if (err) *err << "NAdam stress finite check failed\n";
    return false;
  }

  Series parity_math = pluss::make_series(std::vector<double>{1.0, -2.0}, {2}, cfg);
  Series parity_dl = pluss::make_series(std::vector<double>{1.0, -2.0}, {2}, cfg);
  const Series parity_sensitivity = pluss::make_series(std::vector<double>{0.2, -0.4}, {2}, cfg);

  pluss::subsp::math::optim::AdamOptions parity_math_opt{};
  parity_math_opt.learning_rate = 1e-3;
  parity_math_opt.beta1 = 0.9;
  parity_math_opt.beta2 = 0.999;
  parity_math_opt.eps = 1e-8;
  parity_math_opt.weight_decay = 0.0;
  pluss::subsp::math::optim::AdamState parity_math_state =
      pluss::subsp::math::optim::adam_state_like(std::vector<Series>{parity_math});

  pluss::nn::optim::AdamOptions parity_dl_opt{};
  parity_dl_opt.learning_rate = parity_math_opt.learning_rate;
  parity_dl_opt.beta1 = parity_math_opt.beta1;
  parity_dl_opt.beta2 = parity_math_opt.beta2;
  parity_dl_opt.epsilon = parity_math_opt.eps;
  parity_dl_opt.weight_decay = parity_math_opt.weight_decay;
  pluss::nn::optim::Adam parity_dl_adam(std::vector<Series*>{&parity_dl}, parity_dl_opt);

  for (int i = 0; i < 3; ++i) {
    std::vector<Series> math_params{parity_math};
    pluss::subsp::math::optim::adam_step_inplace(
        math_params,
        std::vector<Series>{parity_sensitivity},
        parity_math_state,
        parity_math_opt);
    parity_math = math_params[0];

    parity_dl_adam.step(std::vector<Series>{parity_sensitivity});
  }

  if (!approx_vec(vec_f64(parity_math), vec_f64(parity_dl), 1e-10)) {
    if (err) *err << "DL Adam/math::optim Adam parity mismatch\n";
    return false;
  }

  Series tape_param = pluss::make_series(2.0, cfg);
  auto tracked = pluss::subsp::math::diff::trace(tape_param, true);
  const Series tape_loss = tracked.value().square();
  tracked.clear_sensitivity();
  tape_loss.reverse();
  pluss::nn::optim::SGDOptions tape_opt{};
  tape_opt.learning_rate = 0.25;
  pluss::nn::optim::SGD tape_sgd(std::vector<Series*>{&tape_param}, tape_opt);
  tape_sgd.step();
  if (!approx(scalar_f64(tape_param), 1.0, 1e-8)) {
    if (err) *err << "Optimizer::step() sensitivity pull mismatch\n";
    return false;
  }
  if (!tape_param.tracks_slope() || tape_param.has_sensitivity()) {
    if (err) *err << "parameter slope/sensitivity post-step invariant mismatch\n";
    return false;
  }

  ScalarModule module(cfg);
  pluss::nn::optim::SGD mod_sgd(module, sgd_opt);
  mod_sgd.step(std::vector<Series>{pluss::make_series(0.1, cfg)});
  if (!approx(scalar_f64(module.weight), 0.99, 1e-9)) {
    if (err) *err << "Module-based optimizer constructor mismatch\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn optimizers update parameters with sensitivity semantics") {
  REQUIRE(run_nn_optimizer_checks(&std::cerr));
}
#else
int main() {
  if (!run_nn_optimizer_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nn_optim_test passed\n";
  return 0;
}
#endif
