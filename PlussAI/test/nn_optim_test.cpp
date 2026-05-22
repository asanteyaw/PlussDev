#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config train_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-6) {
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

struct NestedModule final : pluss::nn::Module {
  ScalarModule inner;

  explicit NestedModule(const Config& cfg)
      : Module(true), inner(cfg) {
    inner.weight.tracks_slope_(false);
    module_record("inner", inner);
  }
};

bool run_nn_optimizer_checks(std::ostream* err = nullptr) {
  const Config cfg = train_cfg();
  const char* stage = "init";
  try {

  {
    stage = "fp64 contract";
    Series fp64_param = pluss::make_series(1.0, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
    pluss::nn::optim::AdamOptions bad_opt{};
    bool threw = false;
    try {
      pluss::nn::optim::Adam bad(std::vector<Series*>{&fp64_param}, bad_opt);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer Float32-only contract mismatch\n";
      return false;
    }
  }

  {
    stage = "closure loss dtype contract";
    Series p = pluss::make_series(1.0f, cfg);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-2;
    pluss::nn::optim::Adam optimizer(std::vector<Series*>{&p}, opt);
    bool threw = false;
    try {
      (void)optimizer.step([&]() -> Series {
        return p.square().to(ScalarType::Float64);
      });
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer closure loss dtype contract mismatch\n";
      return false;
    }
  }

  stage = "sgd explicit";
  Series sgd_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::SGDOptions sgd_opt{};
  sgd_opt.learning_rate = 0.1;
  pluss::nn::optim::SGD sgd(std::vector<Series*>{&sgd_param}, sgd_opt);
  sgd.step(std::vector<Series>{pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(sgd_param), 0.95)) {
    if (err) *err << "SGD explicit sensitivity step mismatch\n";
    return false;
  }

  {
    stage = "sgd closure explicit gradients";
    Series p = pluss::make_series(1.0, cfg);
    const Series target = pluss::make_series(3.0, cfg);
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 0.1;
    pluss::nn::optim::SGD tx(std::vector<Series*>{&p}, opt);
    for (int i = 0; i < 3; ++i) {
      (void)tx.step([&]() -> std::pair<Series, std::vector<Series>> {
        const Series grad = p.sub(target);
        const Series loss = p.sub(target).square().mul(0.5);
        return {loss, std::vector<Series>{grad}};
      });
    }
    if (!approx(scalar_f64(p), 1.542, 1e-6)) {
      if (err) *err << "SGD closure explicit-gradient step mismatch\n";
      return false;
    }
  }

  stage = "sgd groups";
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
  if (!approx(scalar_f64(sgd_group_a), 0.9, 1e-6) || !approx(scalar_f64(sgd_group_b), 0.99, 1e-6)) {
    if (err) *err << "SGD parameter-group learning rates mismatch\n";
    return false;
  }

  stage = "sgd clip";
  Series clip_param = pluss::make_series(std::vector<float>{1.0, 1.0}, {2}, cfg);
  pluss::nn::optim::SGDOptions clip_opt{};
  clip_opt.learning_rate = 1.0;
  clip_opt.clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
  clip_opt.clip.threshold = 1.0;
  pluss::nn::optim::SGD sgd_clip(std::vector<Series*>{&clip_param}, clip_opt);
  sgd_clip.step(std::vector<Series>{pluss::make_series(std::vector<float>{3.0, 4.0}, {2}, cfg)});
  if (!approx_vec(vec_f64(clip_param), std::vector<double>{0.4, 0.2}, 1e-6)) {
    if (err) *err << "SGD norm clipping mismatch\n";
    return false;
  }

  {
    stage = "sgd lanes";
    Series loop_a = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series loop_b = pluss::make_series(std::vector<float>{0.5, 3.0}, {2}, cfg);
    Series bat_a = loop_a.clone();
    Series bat_b = loop_b.clone();
    const Series g_a = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);
    const Series g_b = pluss::make_series(std::vector<float>{-0.3, 0.1}, {2}, cfg);

    pluss::nn::optim::SGDOptions loop_opt{};
    loop_opt.learning_rate = 0.1;
    loop_opt.momentum = 0.9;
    loop_opt.dampening = 0.1;
    loop_opt.weight_decay = 1e-2;
    loop_opt.nesterov = true;
    pluss::nn::optim::SGDOptions bat_opt = loop_opt;
    bat_opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;

    pluss::nn::optim::SGD loop_sgd(std::vector<Series*>{&loop_a, &loop_b}, loop_opt);
    pluss::nn::optim::SGD bat_sgd(std::vector<Series*>{&bat_a, &bat_b}, bat_opt);
    for (int i = 0; i < 8; ++i) {
      loop_sgd.step(std::vector<Series>{g_a, g_b});
      bat_sgd.step(std::vector<Series>{g_a, g_b});
    }

    if (!approx_vec(vec_f64(loop_a), vec_f64(bat_a), 1e-6) ||
        !approx_vec(vec_f64(loop_b), vec_f64(bat_b), 1e-6)) {
      if (err) *err << "SGD batched-lane parity mismatch\n";
      return false;
    }
  }

  {
    stage = "sgd strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::SGDOptions loop_opt{};
    loop_opt.learning_rate = 0.1;
    pluss::nn::optim::SGDOptions strict_fused_opt{};
    strict_fused_opt.learning_rate = 0.1;
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    pluss::nn::optim::SGD loop_sgd(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::SGD strict_sgd(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 8; ++i) {
      loop_sgd.step(std::vector<Series>{g});
      strict_sgd.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "SGD strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_sgd.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "SGD strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "sgd capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    pluss::nn::optim::SGDOptions host_opt{};
    host_opt.learning_rate = 0.1;
    host_opt.momentum = 0.9;
    host_opt.nesterov = true;
    pluss::nn::optim::SGDOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;

    pluss::nn::optim::SGD host_sgd(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::SGD cap_sgd(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_sgd.step(std::vector<Series>{g});
      cap_sgd.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "SGD capturable-mode parity mismatch\n";
      return false;
    }
  }

  {
    stage = "sgd strict capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    pluss::nn::optim::SGDOptions host_opt{};
    host_opt.learning_rate = 0.1;
    host_opt.momentum = 0.9;
    host_opt.nesterov = true;
    pluss::nn::optim::SGDOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;
    cap_opt.execution.strict = true;
    pluss::nn::optim::SGD host_sgd(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::SGD cap_sgd(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_sgd.step(std::vector<Series>{g});
      cap_sgd.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "SGD strict capturable parity mismatch\n";
      return false;
    }
  }

  stage = "adagrad explicit";
  Series adagrad_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdagradOptions adagrad_opt{};
  adagrad_opt.learning_rate = 0.1;
  pluss::nn::optim::Adagrad adagrad(std::vector<Series*>{&adagrad_param}, adagrad_opt);
  adagrad.step(std::vector<Series>{pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(adagrad_param), 0.9, 1e-6)) {
    if (err) *err << "Adagrad update mismatch\n";
    return false;
  }

  stage = "adagrad groups";
  Series adagrad_group_a = pluss::make_series(1.0, cfg);
  Series adagrad_group_b = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdagradOptions adagrad_group_opt_a{};
  adagrad_group_opt_a.learning_rate = 0.1;
  pluss::nn::optim::AdagradOptions adagrad_group_opt_b{};
  adagrad_group_opt_b.learning_rate = 0.01;
  pluss::nn::optim::Adagrad adagrad_groups(std::vector<pluss::nn::optim::AdagradGroup>{
      pluss::nn::optim::AdagradGroup{std::vector<Series*>{&adagrad_group_a}, adagrad_group_opt_a},
      pluss::nn::optim::AdagradGroup{std::vector<Series*>{&adagrad_group_b}, adagrad_group_opt_b},
  });
  adagrad_groups.step(std::vector<Series>{pluss::make_series(0.5, cfg), pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(adagrad_group_a), 0.9, 1e-6) ||
      !approx(scalar_f64(adagrad_group_b), 0.99, 1e-6)) {
    if (err) *err << "Adagrad parameter-group learning rates mismatch\n";
    return false;
  }

  {
    stage = "adagrad lanes";
    Series loop_a = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series loop_b = pluss::make_series(std::vector<float>{0.5f, 3.0f}, {2}, cfg);
    Series bat_a = loop_a.clone();
    Series bat_b = loop_b.clone();
    const Series g_a = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    const Series g_b = pluss::make_series(std::vector<float>{-0.3f, 0.1f}, {2}, cfg);

    pluss::nn::optim::AdagradOptions loop_opt{};
    loop_opt.learning_rate = 0.1;
    loop_opt.weight_decay = 1e-2;
    pluss::nn::optim::AdagradOptions bat_opt = loop_opt;
    bat_opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;

    pluss::nn::optim::Adagrad loop_adagrad(std::vector<Series*>{&loop_a, &loop_b}, loop_opt);
    pluss::nn::optim::Adagrad bat_adagrad(std::vector<Series*>{&bat_a, &bat_b}, bat_opt);
    for (int i = 0; i < 8; ++i) {
      loop_adagrad.step(std::vector<Series>{g_a, g_b});
      bat_adagrad.step(std::vector<Series>{g_a, g_b});
    }

    if (!approx_vec(vec_f64(loop_a), vec_f64(bat_a), 1e-6) ||
        !approx_vec(vec_f64(loop_b), vec_f64(bat_b), 1e-6)) {
      if (err) *err << "Adagrad batched-lane parity mismatch\n";
      return false;
    }
  }

  {
    stage = "adagrad strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::AdagradOptions loop_opt{};
    loop_opt.learning_rate = 0.1;
    pluss::nn::optim::AdagradOptions strict_fused_opt{};
    strict_fused_opt.learning_rate = 0.1;
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    pluss::nn::optim::Adagrad loop_adagrad(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::Adagrad strict_adagrad(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 8; ++i) {
      loop_adagrad.step(std::vector<Series>{g});
      strict_adagrad.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "Adagrad strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_adagrad.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "Adagrad strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "adagrad capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    pluss::nn::optim::AdagradOptions host_opt{};
    host_opt.learning_rate = 0.1;
    host_opt.lr_decay = 1e-2;
    host_opt.weight_decay = 1e-2;
    pluss::nn::optim::AdagradOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;

    pluss::nn::optim::Adagrad host_adagrad(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::Adagrad cap_adagrad(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_adagrad.step(std::vector<Series>{g});
      cap_adagrad.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "Adagrad capturable-mode parity mismatch\n";
      return false;
    }
  }

  stage = "adam explicit";
  Series adam_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamOptions adam_opt{};
  adam_opt.learning_rate = 0.1;
  pluss::nn::optim::Adam adam(std::vector<Series*>{&adam_param}, adam_opt);
  const Series adam_sensitivity = pluss::make_series(0.5, cfg);
  adam.step(std::vector<Series>{adam_sensitivity});
  adam.step(std::vector<Series>{adam_sensitivity});
  const double adam_actual = scalar_f64(adam_param);
  if (!approx(adam_actual, 0.8, 2e-6)) {
    if (err) *err << "Adam update mismatch (got=" << adam_actual << ", expected=0.8)\n";
    return false;
  }

  {
    stage = "adam closure autodiff";
    Series p_closure = pluss::make_series(1.0, cfg);
    Series p_manual = p_closure.clone();
    const Series target = pluss::make_series(2.0, cfg);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 0.05;
    pluss::nn::optim::Adam tx_closure(std::vector<Series*>{&p_closure}, opt);
    pluss::nn::optim::Adam tx_manual(std::vector<Series*>{&p_manual}, opt);
    for (int i = 0; i < 12; ++i) {
      (void)tx_closure.step([&]() -> Series {
        auto tracked = pluss::subsp::math::diff::trace(p_closure, true);
        return tracked.value().sub(target).square().mul(0.5);
      });

      auto tracked = pluss::subsp::math::diff::trace(p_manual, true);
      const Series loss = tracked.value().sub(target).square().mul(0.5);
      tx_manual.clear();
      loss.reverse();
      tx_manual.step();
    }
    if (!approx(scalar_f64(p_closure), scalar_f64(p_manual), 1e-6)) {
      if (err) *err << "Adam closure autodiff parity mismatch\n";
      return false;
    }
  }

  {
    stage = "adam closure diagnostics non-finite loss";
    Series p = pluss::make_series(1.0, cfg);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-3;
    pluss::nn::optim::Adam tx(std::vector<Series*>{&p}, opt);
    pluss::nn::optim::StepDiagnosticsOptions diag{};
    diag.fail_if_non_finite_loss = true;
    tx.set_step_diagnostics(diag);
    bool threw = false;
    try {
      (void)tx.step([&]() -> Series {
        return pluss::make_series(std::numeric_limits<float>::quiet_NaN(), cfg);
      });
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer closure non-finite-loss diagnostics mismatch\n";
      return false;
    }
  }

  {
    stage = "adam diagnostics no sensitivity";
    Series guarded = pluss::make_series(1.0, cfg);
    guarded.tracks_slope_(true);
    pluss::nn::optim::Adam guarded_opt(std::vector<Series*>{&guarded}, adam_opt);
    pluss::nn::optim::StepDiagnosticsOptions diag{};
    diag.fail_if_no_sensitivity = true;
    guarded_opt.set_step_diagnostics(diag);
    bool threw = false;
    try {
      guarded_opt.step();
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer step diagnostics missing-sensitivity fail-loud mismatch\n";
      return false;
    }
  }

  {
    stage = "adam diagnostics non-finite grad";
    Series nan_grad_param = pluss::make_series(1.0, cfg);
    pluss::nn::optim::Adam nan_grad_opt(std::vector<Series*>{&nan_grad_param}, adam_opt);
    pluss::nn::optim::StepDiagnosticsOptions diag{};
    diag.fail_if_non_finite_sensitivity = true;
    nan_grad_opt.set_step_diagnostics(diag);
    bool threw = false;
    try {
      nan_grad_opt.step(std::vector<Series>{
          pluss::make_series(std::numeric_limits<double>::quiet_NaN(), cfg)});
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer non-finite sensitivity fail-loud mismatch\n";
      return false;
    }
  }

  {
    stage = "adam diagnostics non-finite param";
    Series inf_param = pluss::make_series(std::numeric_limits<double>::infinity(), cfg);
    pluss::nn::optim::Adam inf_param_opt(std::vector<Series*>{&inf_param}, adam_opt);
    pluss::nn::optim::StepDiagnosticsOptions diag{};
    diag.fail_if_non_finite_parameter = true;
    inf_param_opt.set_step_diagnostics(diag);
    bool threw = false;
    try {
      inf_param_opt.step(std::vector<Series>{pluss::make_series(0.1, cfg)});
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer non-finite parameter fail-loud mismatch\n";
      return false;
    }
  }

  {
    stage = "adam coverage diagnostics";
    Series a = pluss::make_series(2.0, cfg);
    Series b = pluss::make_series(-1.0, cfg);
    auto ta = pluss::subsp::math::diff::trace(a, true);
    auto tb = pluss::subsp::math::diff::trace(b, true);
    (void)tb;
    const Series loss = ta.value().square();
    pluss::nn::optim::Adam cov_opt(std::vector<Series*>{&a, &b}, adam_opt);
    pluss::nn::optim::StepDiagnosticsOptions diag{};
    diag.minimum_tracked_coverage = 1.0;
    cov_opt.set_step_diagnostics(diag);
    cov_opt.clear();
    loss.reverse();
    bool threw = false;
    try {
      cov_opt.step();
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Optimizer minimum_tracked_coverage fail-loud mismatch\n";
      return false;
    }
  }

  stage = "adam groups";
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
  if (!approx(scalar_f64(adam_group_a), 0.9, 1e-6) || !approx(scalar_f64(adam_group_b), 0.99, 1e-6)) {
    if (err) *err << "Adam parameter-group learning rates mismatch\n";
    return false;
  }

  {
    stage = "adam lanes";
    Series loop_a = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series loop_b = pluss::make_series(std::vector<float>{0.5, 3.0}, {2}, cfg);
    Series bat_a = loop_a.clone();
    Series bat_b = loop_b.clone();
    const Series g_a = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);
    const Series g_b = pluss::make_series(std::vector<float>{-0.3, 0.1}, {2}, cfg);

    pluss::nn::optim::AdamOptions loop_opt{};
    loop_opt.learning_rate = 1e-3;
    pluss::nn::optim::AdamOptions bat_opt = loop_opt;
    bat_opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;

    pluss::nn::optim::Adam loop_adam(std::vector<Series*>{&loop_a, &loop_b}, loop_opt);
    pluss::nn::optim::Adam bat_adam(std::vector<Series*>{&bat_a, &bat_b}, bat_opt);
    for (int i = 0; i < 12; ++i) {
      loop_adam.step(std::vector<Series>{g_a, g_b});
      bat_adam.step(std::vector<Series>{g_a, g_b});
    }

    if (!approx_vec(vec_f64(loop_a), vec_f64(bat_a), 1e-6) ||
        !approx_vec(vec_f64(loop_b), vec_f64(bat_b), 1e-6)) {
      if (err) *err << "Adam batched-lane parity mismatch\n";
      return false;
    }
  }

  {
    stage = "adam capturable";
    Series host = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);

    pluss::nn::optim::AdamOptions host_opt{};
    host_opt.learning_rate = 1e-3;
    pluss::nn::optim::AdamOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;

    pluss::nn::optim::Adam host_adam(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::Adam cap_adam(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 12; ++i) {
      host_adam.step(std::vector<Series>{g});
      cap_adam.step(std::vector<Series>{g});
    }

    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "Adam capturable-mode parity mismatch\n";
      return false;
    }
  }

  {
    stage = "adam strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::AdamOptions loop_opt{};
    loop_opt.learning_rate = 1e-3;
    pluss::nn::optim::AdamOptions strict_fused_opt{};
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    strict_fused_opt.learning_rate = 1e-3;
    pluss::nn::optim::Adam loop_adam(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::Adam strict_adam(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 12; ++i) {
      loop_adam.step(std::vector<Series>{g});
      strict_adam.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "Adam strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_adam.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "Adam strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "adam strict capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::AdamOptions host_opt{};
    host_opt.learning_rate = 1e-3;
    pluss::nn::optim::AdamOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;
    cap_opt.execution.strict = true;
    pluss::nn::optim::Adam host_adam(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::Adam cap_adam(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_adam.step(std::vector<Series>{g});
      cap_adam.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "Adam strict capturable parity mismatch\n";
      return false;
    }
  }

  stage = "nadam explicit";
  Series nadam_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::NAdamOptions nadam_opt{};
  nadam_opt.learning_rate = 0.1;
  pluss::nn::optim::NAdam nadam(std::vector<Series*>{&nadam_param}, nadam_opt);
  nadam.step(std::vector<Series>{adam_sensitivity});
  const double nadam_expected = nadam_first_step_expected(1.0, 0.5, nadam_opt);
  if (!approx(scalar_f64(nadam_param), nadam_expected, 1e-6)) {
    if (err) *err << "NAdam update mismatch\n";
    return false;
  }

  stage = "nadam groups";
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
  if (!approx(scalar_f64(nadam_group_a), nadam_expected_a, 1e-6) ||
      !approx(scalar_f64(nadam_group_b), nadam_expected_b, 1e-6)) {
    if (err) *err << "NAdam parameter-group learning rates mismatch\n";
    return false;
  }

  {
    stage = "nadam lanes";
    Series loop_a = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series loop_b = pluss::make_series(std::vector<float>{0.5, 3.0}, {2}, cfg);
    Series bat_a = loop_a.clone();
    Series bat_b = loop_b.clone();
    const Series g_a = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);
    const Series g_b = pluss::make_series(std::vector<float>{-0.3, 0.1}, {2}, cfg);

    pluss::nn::optim::NAdamOptions loop_opt{};
    loop_opt.learning_rate = 1e-3;
    loop_opt.weight_decay = 1e-2;
    loop_opt.decoupled_weight_decay = true;
    pluss::nn::optim::NAdamOptions bat_opt = loop_opt;
    bat_opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;

    pluss::nn::optim::NAdam loop_nadam(std::vector<Series*>{&loop_a, &loop_b}, loop_opt);
    pluss::nn::optim::NAdam bat_nadam(std::vector<Series*>{&bat_a, &bat_b}, bat_opt);
    for (int i = 0; i < 10; ++i) {
      loop_nadam.step(std::vector<Series>{g_a, g_b});
      bat_nadam.step(std::vector<Series>{g_a, g_b});
    }

    if (!approx_vec(vec_f64(loop_a), vec_f64(bat_a), 1e-6) ||
        !approx_vec(vec_f64(loop_b), vec_f64(bat_b), 1e-6)) {
      if (err) *err << "NAdam batched-lane parity mismatch\n";
      return false;
    }
  }

  {
    stage = "nadam strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::NAdamOptions loop_opt{};
    loop_opt.learning_rate = 1e-3;
    pluss::nn::optim::NAdamOptions strict_fused_opt{};
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    strict_fused_opt.learning_rate = 1e-3;
    pluss::nn::optim::NAdam loop_nadam(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::NAdam strict_nadam(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 10; ++i) {
      loop_nadam.step(std::vector<Series>{g});
      strict_nadam.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "NAdam strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_nadam.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "NAdam strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "nadam strict capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::NAdamOptions host_opt{};
    host_opt.learning_rate = 1e-3;
    host_opt.weight_decay = 1e-2;
    host_opt.decoupled_weight_decay = true;
    pluss::nn::optim::NAdamOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;
    cap_opt.execution.strict = true;
    pluss::nn::optim::NAdam host_nadam(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::NAdam cap_nadam(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 10; ++i) {
      host_nadam.step(std::vector<Series>{g});
      cap_nadam.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "NAdam strict capturable parity mismatch\n";
      return false;
    }
  }

  stage = "adamw explicit";
  Series adamw_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamWOptions adamw_opt{};
  adamw_opt.learning_rate = 0.1;
  adamw_opt.weight_decay = 0.1;
  pluss::nn::optim::AdamW adamw(std::vector<Series*>{&adamw_param}, adamw_opt);
  adamw.step(std::vector<Series>{adam_sensitivity});
  if (!approx(scalar_f64(adamw_param), 0.89, 1e-6)) {
    if (err) *err << "AdamW decoupled decay mismatch\n";
    return false;
  }

  {
    stage = "adamw capturable";
    Series host = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);

    pluss::nn::optim::AdamWOptions host_opt{};
    host_opt.learning_rate = 1e-3;
    host_opt.weight_decay = 1e-2;
    pluss::nn::optim::AdamWOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;

    pluss::nn::optim::AdamW host_adamw(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::AdamW cap_adamw(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 12; ++i) {
      host_adamw.step(std::vector<Series>{g});
      cap_adamw.step(std::vector<Series>{g});
    }

    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "AdamW capturable-mode parity mismatch\n";
      return false;
    }
  }

  stage = "rmsprop explicit";
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

  {
    stage = "rmsprop lanes";
    Series loop_a = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series loop_b = pluss::make_series(std::vector<float>{0.5, 3.0}, {2}, cfg);
    Series bat_a = loop_a.clone();
    Series bat_b = loop_b.clone();
    const Series g_a = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);
    const Series g_b = pluss::make_series(std::vector<float>{-0.3, 0.1}, {2}, cfg);

    pluss::nn::optim::RMSpropOptions loop_opt{};
    loop_opt.learning_rate = 1e-2;
    loop_opt.alpha = 0.95;
    loop_opt.epsilon = 1e-8;
    loop_opt.momentum = 0.4;
    loop_opt.weight_decay = 1e-2;
    loop_opt.centered = true;
    pluss::nn::optim::RMSpropOptions bat_opt = loop_opt;
    bat_opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;

    pluss::nn::optim::RMSprop loop_rms(std::vector<Series*>{&loop_a, &loop_b}, loop_opt);
    pluss::nn::optim::RMSprop bat_rms(std::vector<Series*>{&bat_a, &bat_b}, bat_opt);
    for (int i = 0; i < 9; ++i) {
      loop_rms.step(std::vector<Series>{g_a, g_b});
      bat_rms.step(std::vector<Series>{g_a, g_b});
    }

    if (!approx_vec(vec_f64(loop_a), vec_f64(bat_a), 1e-6) ||
        !approx_vec(vec_f64(loop_b), vec_f64(bat_b), 1e-6)) {
      if (err) *err << "RMSprop batched-lane parity mismatch\n";
      return false;
    }
  }

  {
    stage = "adamw strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::AdamWOptions loop_opt{};
    loop_opt.learning_rate = 1e-3;
    loop_opt.weight_decay = 1e-2;
    pluss::nn::optim::AdamWOptions strict_fused_opt = loop_opt;
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    pluss::nn::optim::AdamW loop_adamw(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::AdamW strict_adamw(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 12; ++i) {
      loop_adamw.step(std::vector<Series>{g});
      strict_adamw.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "AdamW strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_adamw.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "AdamW strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "rmsprop strict fused";
    Series loop_param = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series strict_fused = loop_param.clone();
    const Series g = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    pluss::nn::optim::RMSpropOptions loop_opt{};
    loop_opt.learning_rate = 1e-2;
    pluss::nn::optim::RMSpropOptions strict_fused_opt{};
    strict_fused_opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
    strict_fused_opt.execution.strict = true;
    strict_fused_opt.learning_rate = 1e-2;
    pluss::nn::optim::RMSprop loop_rms(std::vector<Series*>{&loop_param}, loop_opt);
    pluss::nn::optim::RMSprop strict_rms(std::vector<Series*>{&strict_fused}, strict_fused_opt);
    for (int i = 0; i < 9; ++i) {
      loop_rms.step(std::vector<Series>{g});
      strict_rms.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(loop_param), vec_f64(strict_fused), 1e-6)) {
      if (err) *err << "RMSprop strict fused parity mismatch\n";
      return false;
    }
    const auto& lane = strict_rms.last_step_lane_telemetry();
    if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
        !lane.front().fused_supported) {
      if (err) *err << "RMSprop strict fused telemetry mismatch\n";
      return false;
    }
  }

  {
    stage = "rmsprop capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    pluss::nn::optim::RMSpropOptions host_opt{};
    host_opt.learning_rate = 1e-2;
    host_opt.alpha = 0.95;
    host_opt.momentum = 0.4;
    host_opt.weight_decay = 1e-2;
    host_opt.centered = true;
    pluss::nn::optim::RMSpropOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;
    pluss::nn::optim::RMSprop host_rms(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::RMSprop cap_rms(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_rms.step(std::vector<Series>{g});
      cap_rms.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "RMSprop capturable-mode parity mismatch\n";
      return false;
    }
  }

  {
    stage = "rmsprop strict capturable";
    Series host = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series cap = host.clone();
    const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    pluss::nn::optim::RMSpropOptions host_opt{};
    host_opt.learning_rate = 1e-2;
    host_opt.alpha = 0.95;
    host_opt.momentum = 0.4;
    host_opt.weight_decay = 1e-2;
    host_opt.centered = true;
    pluss::nn::optim::RMSpropOptions cap_opt = host_opt;
    cap_opt.execution.capturable = true;
    cap_opt.execution.strict = true;
    pluss::nn::optim::RMSprop host_rms(std::vector<Series*>{&host}, host_opt);
    pluss::nn::optim::RMSprop cap_rms(std::vector<Series*>{&cap}, cap_opt);
    for (int i = 0; i < 8; ++i) {
      host_rms.step(std::vector<Series>{g});
      cap_rms.step(std::vector<Series>{g});
    }
    if (!approx_vec(vec_f64(host), vec_f64(cap), 1e-6)) {
      if (err) *err << "RMSprop strict capturable parity mismatch\n";
      return false;
    }
  }

  stage = "nadam stress";
  Series nadam_stress = pluss::make_series(std::vector<float>{1.0, -2.0, 3.0}, {3}, cfg);
  pluss::nn::optim::NAdam nadam_stress_optim(std::vector<Series*>{&nadam_stress}, nadam_opt);
  for (int i = 0; i < 200; ++i) {
    nadam_stress_optim.step(std::vector<Series>{pluss::make_series(std::vector<float>{0.1, -0.2, 0.3}, {3}, cfg)});
  }
  const Series nadam_stress_finite = nadam_stress.isfinite().all();
  if (!nadam_stress_finite.item<bool>()) {
    if (err) *err << "NAdam stress finite check failed\n";
    return false;
  }

  stage = "math parity";
  Series parity_math = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
  Series parity_dl = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
  const Series parity_sensitivity = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);

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

  if (!approx_vec(vec_f64(parity_math), vec_f64(parity_dl), 1e-6)) {
    if (err) *err << "DL Adam/math::optim Adam parity mismatch\n";
    return false;
  }

  {
    stage = "adam capture restore";
    Series ref = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    Series resumed = pluss::make_series(std::vector<float>{1.0, -2.0}, {2}, cfg);
    const Series g = pluss::make_series(std::vector<float>{0.2, -0.4}, {2}, cfg);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-3;

    pluss::nn::optim::Adam ref_optim(std::vector<Series*>{&ref}, opt);
    for (int i = 0; i < 8; ++i) {
      ref_optim.step(std::vector<Series>{g});
    }

    pluss::nn::optim::Adam pre_optim(std::vector<Series*>{&resumed}, opt);
    for (int i = 0; i < 3; ++i) {
      pre_optim.step(std::vector<Series>{g});
    }
    const auto checkpoint = pre_optim.capture_state();

    pluss::nn::optim::Adam post_optim(std::vector<Series*>{&resumed}, opt);
    post_optim.restore_state(checkpoint);
    for (int i = 0; i < 5; ++i) {
      post_optim.step(std::vector<Series>{g});
    }

    if (!approx_vec(vec_f64(ref), vec_f64(resumed), 1e-6)) {
      if (err) *err << "Adam capture/restore parity mismatch\n";
      return false;
    }
  }

  {
    stage = "nadam capture restore";
    Series ref = pluss::make_series(1.0, cfg);
    Series resumed = pluss::make_series(1.0, cfg);
    const Series g = pluss::make_series(0.5, cfg);
    pluss::nn::optim::NAdamOptions opt{};
    opt.learning_rate = 0.01;

    pluss::nn::optim::NAdam ref_optim(std::vector<Series*>{&ref}, opt);
    for (int i = 0; i < 8; ++i) {
      ref_optim.step(std::vector<Series>{g});
    }

    pluss::nn::optim::NAdam pre_optim(std::vector<Series*>{&resumed}, opt);
    for (int i = 0; i < 3; ++i) {
      pre_optim.step(std::vector<Series>{g});
    }
    const auto checkpoint = pre_optim.capture_state();

    pluss::nn::optim::NAdam post_optim(std::vector<Series*>{&resumed}, opt);
    post_optim.restore_state(checkpoint);
    for (int i = 0; i < 5; ++i) {
      post_optim.step(std::vector<Series>{g});
    }

    if (!approx(scalar_f64(ref), scalar_f64(resumed), 1e-6)) {
      if (err) *err << "NAdam capture/restore parity mismatch\n";
      return false;
    }
  }

  stage = "minimax explicit";
  Series minimax_param = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
  const Series minimax_target = pluss::make_series(std::vector<float>{3.0, 3.0}, {2}, cfg);
  pluss::nn::optim::MinimaxOptions minimax_opt{};
  minimax_opt.learning_rate = 0.05;
  minimax_opt.dual_learning_rate = 0.1;
  minimax_opt.inequality_penalty = 10.0;

  pluss::nn::optim::MinimaxConstraints minimax_constraints{};
  minimax_constraints.lower_bounds = {pluss::make_series(std::vector<float>{0.0, 0.0}, {2}, cfg)};
  minimax_constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum().sub(4.0)};
  };
  minimax_constraints.inequalities_trace = [](const std::vector<pluss::subsp::math::diff::Trace>& p) {
    return std::vector<pluss::subsp::math::diff::Trace>{pluss::subsp::math::diff::sum(p[0]) - 4.0};
  };

  pluss::nn::optim::Minimax minimax({&minimax_param}, minimax_opt, minimax_constraints);
  for (int i = 0; i < 120; ++i) {
    minimax.step(std::vector<Series>{minimax_param.sub(minimax_target)});
  }
  const auto minimax_v = vec_f64(minimax_param);
  if (!(minimax_v[0] >= -1e-5 && minimax_v[1] >= -1e-5 && minimax_v[0] + minimax_v[1] <= 4.05)) {
    if (err) *err << "Minimax explicit-gradient constrained step mismatch\n";
    return false;
  }

  stage = "minimax tape";
  Series minimax_tape_param = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
  pluss::nn::optim::Minimax minimax_tape({&minimax_tape_param}, minimax_opt, minimax_constraints);
  for (int i = 0; i < 120; ++i) {
    auto tx = pluss::subsp::math::diff::trace(minimax_tape_param, true);
    const Series loss = tx.value().sub(minimax_target).square().sum().mul(0.5);
    tx.clear_sensitivity();
    loss.reverse();
    minimax_tape.step();
  }
  const auto minimax_tape_v = vec_f64(minimax_tape_param);
  if (!(minimax_tape_v[0] >= -1e-5 && minimax_tape_v[1] >= -1e-5 &&
        minimax_tape_v[0] + minimax_tape_v[1] <= 4.05)) {
    if (err) *err << "Minimax tape-gradient constrained step mismatch\n";
    return false;
  }

  stage = "minimax tape staged inplace";
  Series minimax_tape_staged_param = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
  pluss::nn::optim::Minimax minimax_tape_staged({&minimax_tape_staged_param}, minimax_opt, minimax_constraints);
  for (int i = 0; i < 120; ++i) {
    auto tx = pluss::subsp::math::diff::trace(minimax_tape_staged_param, true);
    Series staged = pluss::zeros_like(tx.value());
    staged.select(0, 0).copy_(tx.value().select(0, 0));
    staged.select(0, 1).copy_(tx.value().select(0, 1));
    const Series loss = staged.sub(minimax_target).square().sum().mul(0.5);
    tx.clear_sensitivity();
    loss.reverse();
    minimax_tape_staged.step();
  }
  const auto minimax_tape_staged_v = vec_f64(minimax_tape_staged_param);
  if (!(minimax_tape_staged_v[0] >= -1e-5 && minimax_tape_staged_v[1] >= -1e-5 &&
        minimax_tape_staged_v[0] + minimax_tape_staged_v[1] <= 4.05)) {
    if (err) *err << "Minimax tape staged-inplace constrained step mismatch\n";
    return false;
  }

  stage = "minimax methods";
  for (const auto method : {pluss::nn::optim::MinimaxPrimalMethod::Adam,
                            pluss::nn::optim::MinimaxPrimalMethod::AdamW,
                            pluss::nn::optim::MinimaxPrimalMethod::SGD,
                            pluss::nn::optim::MinimaxPrimalMethod::RMSprop}) {
    Series method_param = pluss::make_series(std::vector<float>{0.5, -0.5}, {2}, cfg);
    pluss::nn::optim::MinimaxOptions method_opt = minimax_opt;
    method_opt.primal_method = method;
    method_opt.learning_rate = 0.03;
    method_opt.momentum = 0.8;
    pluss::nn::optim::Minimax method_optim({&method_param}, method_opt, minimax_constraints);
    for (int i = 0; i < 60; ++i) {
      method_optim.step(std::vector<Series>{method_param.sub(minimax_target)});
    }
    const auto pv = vec_f64(method_param);
    if (!(std::isfinite(pv[0]) && std::isfinite(pv[1]) && pv[0] >= -1e-5 && pv[1] >= -1e-5)) {
      if (err) *err << "Minimax primal-method switch produced non-finite or infeasible values\n";
      return false;
    }
  }

  stage = "minimax groups";
  Series group_a = pluss::make_series(1.0, cfg);
  Series group_b = pluss::make_series(1.0, cfg);
  pluss::nn::optim::MinimaxOptions gopt_fast = minimax_opt;
  pluss::nn::optim::MinimaxOptions gopt_slow = minimax_opt;
  gopt_fast.learning_rate = 0.10;
  gopt_slow.learning_rate = 0.01;
  gopt_fast.primal_method = pluss::nn::optim::MinimaxPrimalMethod::Adam;
  gopt_slow.primal_method = pluss::nn::optim::MinimaxPrimalMethod::Adam;
  pluss::nn::optim::Minimax group_minimax(
      {
          pluss::nn::optim::MinimaxGroup{{&group_a}, gopt_fast},
          pluss::nn::optim::MinimaxGroup{{&group_b}, gopt_slow},
      },
      {});
  group_minimax.step(std::vector<Series>{pluss::make_series(1.0, cfg), pluss::make_series(1.0, cfg)});
  if (!(std::abs(scalar_f64(group_a) - 1.0) > std::abs(scalar_f64(group_b) - 1.0))) {
    if (err) *err << "Minimax parameter-group learning-rate behavior mismatch\n";
    return false;
  }

  stage = "minimax schedule";
  Series scheduled_param = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
  pluss::nn::optim::MinimaxOptions scheduled_opt = minimax_opt;
  scheduled_opt.primal_method = pluss::nn::optim::MinimaxPrimalMethod::AdamW;
  scheduled_opt.dual_schedule = pluss::nn::optim::MinimaxSchedule::ViolationAdaptive;
  scheduled_opt.penalty_schedule = pluss::nn::optim::MinimaxSchedule::Exponential;
  scheduled_opt.penalty_exponential_growth = 1.002;
  pluss::nn::optim::Minimax scheduled_minimax(
      {&scheduled_param},
      scheduled_opt,
      minimax_constraints);
  for (int i = 0; i < 80; ++i) {
    scheduled_minimax.step(std::vector<Series>{scheduled_param.sub(minimax_target)});
  }
  const auto sv = vec_f64(scheduled_param);
  if (!(std::isfinite(sv[0]) && std::isfinite(sv[1]))) {
    if (err) *err << "Minimax adaptive schedule path produced non-finite values\n";
    return false;
  }

  {
    stage = "minimax capture restore";
    Series ref = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
    Series resumed = pluss::make_series(std::vector<float>{2.0, -1.0}, {2}, cfg);
    pluss::nn::optim::MinimaxOptions opt = minimax_opt;
    opt.primal_method = pluss::nn::optim::MinimaxPrimalMethod::AdamW;
    opt.dual_schedule = pluss::nn::optim::MinimaxSchedule::ViolationAdaptive;
    opt.penalty_schedule = pluss::nn::optim::MinimaxSchedule::Exponential;
    opt.penalty_exponential_growth = 1.001;

    pluss::nn::optim::Minimax ref_optim({&ref}, opt, minimax_constraints);
    for (int i = 0; i < 60; ++i) {
      ref_optim.step(std::vector<Series>{ref.sub(minimax_target)});
    }

    pluss::nn::optim::Minimax pre_optim({&resumed}, opt, minimax_constraints);
    for (int i = 0; i < 20; ++i) {
      pre_optim.step(std::vector<Series>{resumed.sub(minimax_target)});
    }
    const auto checkpoint = pre_optim.capture_state();

    pluss::nn::optim::Minimax post_optim({&resumed}, opt, minimax_constraints);
    post_optim.restore_state(checkpoint);
    for (int i = 0; i < 40; ++i) {
      post_optim.step(std::vector<Series>{resumed.sub(minimax_target)});
    }

    if (!approx_vec(vec_f64(ref), vec_f64(resumed), 1e-6)) {
      if (err) *err << "Minimax capture/restore parity mismatch\n";
      return false;
    }
  }

  stage = "tape sensitivity pull";
  Series tape_param = pluss::make_series(2.0, cfg);
  auto tracked = pluss::subsp::math::diff::trace(tape_param, true);
  const Series tape_loss = tracked.value().square();
  tracked.clear_sensitivity();
  tape_loss.reverse();
  pluss::nn::optim::SGDOptions tape_opt{};
  tape_opt.learning_rate = 0.25;
  pluss::nn::optim::SGD tape_sgd(std::vector<Series*>{&tape_param}, tape_opt);
  tape_sgd.step();
  if (!approx(scalar_f64(tape_param), 1.0, 1e-6)) {
    if (err) *err << "Optimizer::step() sensitivity pull mismatch\n";
    return false;
  }
  if (!tape_param.tracks_slope() || tape_param.has_sensitivity()) {
    if (err) *err << "parameter slope/sensitivity post-step invariant mismatch\n";
    return false;
  }

  stage = "optimizer update leaf safety";
  Series leaf_param = pluss::make_series(1.5, cfg);
  auto leaf_tracked = pluss::subsp::math::diff::trace(leaf_param, true);
  const Series leaf_loss = leaf_tracked.value().square().mul(0.5);
  pluss::nn::optim::SGDOptions leaf_opt{};
  leaf_opt.learning_rate = 0.1;
  pluss::nn::optim::SGD leaf_sgd(std::vector<Series*>{&leaf_param}, leaf_opt);
  leaf_sgd.clear();
  leaf_loss.reverse();
  leaf_sgd.step();
  if (!leaf_param.is_leaf().item<bool>()) {
    if (err) *err << "Optimizer step must leave parameters as leaf tensors\n";
    return false;
  }
  if (!leaf_param.tracks_slope() || leaf_param.has_sensitivity()) {
    if (err) *err << "Optimizer step leaf/sensitivity invariants mismatch\n";
    return false;
  }

  stage = "module constructor";
  ScalarModule module(cfg);
  if (!module.weight.tracks_slope()) {
    if (err) *err << "module parameter_record should enable slope tracking\n";
    return false;
  }
  module.to(pluss::pCPU, ScalarType::Float32);
  if (!module.weight.tracks_slope()) {
    if (err) *err << "module::to should preserve parameter slope tracking\n";
    return false;
  }
  pluss::nn::optim::SGD mod_sgd(module, sgd_opt);
  mod_sgd.clear();
  mod_sgd.step(std::vector<Series>{pluss::make_series(0.1, cfg)});
  if (!approx(scalar_f64(module.weight), 0.99, 1e-6)) {
    if (err) *err << "Module-based optimizer constructor mismatch\n";
    return false;
  }

  stage = "nested module registration";
  NestedModule nested(cfg);
  if (!nested.inner.weight.tracks_slope()) {
    if (err) *err << "module_record should re-enable child parameter slope tracking\n";
    return false;
  }

  if (pluss::mps::is_ready()) {
    const Config mps_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
    const Series mps_grad = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, mps_cfg);

    {
      stage = "mps adam strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::AdamOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::AdamOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::Adam ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::Adam tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS Adam strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS Adam strict fused telemetry mismatch\n";
        return false;
      }
    }
    {
      stage = "mps sgd strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::SGDOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::SGDOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::SGD ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::SGD tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS SGD strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS SGD strict fused telemetry mismatch\n";
        return false;
      }
    }
    {
      stage = "mps adagrad strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::AdagradOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::AdagradOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::Adagrad ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::Adagrad tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS Adagrad strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS Adagrad strict fused telemetry mismatch\n";
        return false;
      }
    }
    {
      stage = "mps adamw strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::AdamWOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::AdamWOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::AdamW ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::AdamW tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS AdamW strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS AdamW strict fused telemetry mismatch\n";
        return false;
      }
    }
    {
      stage = "mps rmsprop strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::RMSpropOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::RMSpropOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::RMSprop ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::RMSprop tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS RMSprop strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS RMSprop strict fused telemetry mismatch\n";
        return false;
      }
    }
    {
      stage = "mps nadam strict fused";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      Series ref = p.clone();
      pluss::nn::optim::NAdamOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Fused;
      opt.execution.strict = true;
      pluss::nn::optim::NAdamOptions ref_opt = opt;
      ref_opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      pluss::nn::optim::NAdam ref_tx(std::vector<Series*>{&ref}, ref_opt);
      pluss::nn::optim::NAdam tx(std::vector<Series*>{&p}, opt);
      for (int i = 0; i < 6; ++i) {
        ref_tx.step(std::vector<Series>{mps_grad});
        tx.step(std::vector<Series>{mps_grad});
      }
      if (!approx_vec(vec_f64(p), vec_f64(ref), 1e-5)) {
        if (err) *err << "MPS NAdam strict fused parity mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().resolved != pluss::nn::optim::UpdateLane::Fused ||
          !lane.front().fused_supported) {
        if (err) *err << "MPS NAdam strict fused telemetry mismatch\n";
        return false;
      }
    }

    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::AdamOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::Adam tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS Adam step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS Adam lane telemetry mismatch\n";
        return false;
      }
    }
    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::AdamWOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::AdamW tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS AdamW step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS AdamW lane telemetry mismatch\n";
        return false;
      }
    }
    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::NAdamOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::NAdam tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS NAdam step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS NAdam lane telemetry mismatch\n";
        return false;
      }
    }
    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::AdagradOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::Adagrad tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS Adagrad step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS Adagrad lane telemetry mismatch\n";
        return false;
      }
    }
    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::RMSpropOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::RMSprop tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS RMSprop step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS RMSprop lane telemetry mismatch\n";
        return false;
      }
    }
    {
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, mps_cfg);
      pluss::nn::optim::SGDOptions opt{};
      opt.learning_rate = 0.01;
      pluss::nn::optim::SGD tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{mps_grad});
      if (p.platform() != pluss::pMPS || !p.isfinite().all().item<bool>()) {
        if (err) *err << "MPS SGD step/platform finite invariant mismatch\n";
        return false;
      }
      const auto& lane = tx.last_step_lane_telemetry();
      if (lane.size() != 1 || lane.front().platform != pluss::pMPS ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "MPS SGD lane telemetry mismatch\n";
        return false;
      }
    }
  }

  if (pluss::cuda::is_ready()) {
    const Config cuda_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCUDA);
    const Series cuda_grad = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cuda_cfg);

    {
      stage = "cuda adam strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::AdamOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::Adam tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA Adam strict batched lane mismatch\n";
        return false;
      }
    }
    {
      stage = "cuda adamw strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::AdamWOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::AdamW tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA AdamW strict batched lane mismatch\n";
        return false;
      }
    }
    {
      stage = "cuda nadam strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::NAdamOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::NAdam tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA NAdam strict batched lane mismatch\n";
        return false;
      }
    }
    {
      stage = "cuda adagrad strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::AdagradOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::Adagrad tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA Adagrad strict batched lane mismatch\n";
        return false;
      }
    }
    {
      stage = "cuda rmsprop strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::RMSpropOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::RMSprop tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA RMSprop strict batched lane mismatch\n";
        return false;
      }
    }
    {
      stage = "cuda sgd strict batched";
      Series p = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cuda_cfg);
      pluss::nn::optim::SGDOptions opt{};
      opt.learning_rate = 0.01;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
      opt.execution.strict = true;
      pluss::nn::optim::SGD tx(std::vector<Series*>{&p}, opt);
      tx.step(std::vector<Series>{cuda_grad});
      const auto& lane = tx.last_step_lane_telemetry();
      if (p.platform() != pluss::pCUDA || !p.isfinite().all().item<bool>() ||
          lane.size() != 1 ||
          lane.front().resolved != pluss::nn::optim::UpdateLane::Batched ||
          !lane.front().batched_supported) {
        if (err) *err << "CUDA SGD strict batched lane mismatch\n";
        return false;
      }
    }
  }

  {
    stage = "optimizer lane telemetry";
    Series a = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series b = pluss::make_series(std::vector<float>{0.5f, 3.0f}, {2}, cfg);
    const Series ga = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);
    const Series gb = pluss::make_series(std::vector<float>{-0.3f, 0.1f}, {2}, cfg);
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-3;
    opt.execution.lane = pluss::nn::optim::UpdateLane::Batched;
    pluss::nn::optim::Adam tx(std::vector<Series*>{&a, &b}, opt);
    tx.step(std::vector<Series>{ga, gb});
    const auto& telemetry = tx.last_step_lane_telemetry();
    if (telemetry.size() != 1) {
      if (err) *err << "Optimizer lane telemetry should expose one entry per group\n";
      return false;
    }
    const auto& lane = telemetry.front();
    if (lane.group_index != 0 ||
        lane.requested != pluss::nn::optim::UpdateLane::Batched ||
        lane.resolved != pluss::nn::optim::UpdateLane::Batched ||
        lane.platform != pluss::pCPU ||
        lane.dtype != ScalarType::Float32 ||
        lane.active_parameters != 2 ||
        !lane.batch_shape_compatible ||
        !lane.batched_supported ||
        lane.capturable_supported != true) {
      if (err) *err << "Optimizer lane telemetry contract mismatch\n";
      return false;
    }
  }

  return true;
  } catch (const std::exception& ex) {
    if (err) {
      *err << "Unexpected exception at stage '" << stage << "': " << ex.what() << '\n';
    }
    return false;
  }
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
