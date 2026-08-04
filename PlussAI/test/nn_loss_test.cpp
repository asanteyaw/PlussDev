#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cstdint>
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
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nn_loss_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();

  {
    const Series pred = pluss::make_series(std::vector<float>{1.0, 2.0, 3.0}, {3}, cfg);
    const Series target = pluss::make_series(std::vector<float>{1.5, 1.0, 2.5}, {3}, cfg);
    const Series loss = pluss::nn::losses::MSE(pred, target);
    if (!approx(as_f64(loss), 0.5)) {
      if (err) *err << "LossMSE mean reduction mismatch\n";
      return false;
    }

    const Series loss_sum = pluss::nn::losses::MSE(
        pred,
        target,
        pluss::nn::losses::MSEOptions{.reduction = pluss::nn::LossReduction::Sum});
    if (!approx(as_f64(loss_sum), 1.5)) {
      if (err) *err << "LossMSE sum reduction mismatch\n";
      return false;
    }
  }

  {
    const Series pred = pluss::make_series(std::vector<float>{2.0, 4.0, 6.0}, {3}, cfg);
    const Series target = pluss::make_series(std::vector<float>{1.0, 5.0, 4.0}, {3}, cfg);
    const Series denom = pluss::make_series(std::vector<float>{1.0, 2.0, 4.0}, {3}, cfg);
    const Series loss = pluss::nn::losses::RelativeMSE(pred, target, denom);
    const Series expected = pred.sub(target).square().div(denom.abs().clamp_min(1e-12)).mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossRelativeMSE mismatch\n";
      return false;
    }
  }

  {
    const Series pred = pluss::make_series(
        std::vector<float>{
            1.0, 2.0, 3.0,
            4.0, 5.0, 6.0,
        },
        {2, 3},
        cfg);
    const Series target = pluss::make_series(std::vector<float>{0.5, 2.0, 4.0}, {1, 3}, cfg);
    const Series loss = pluss::nn::losses::MSE(pred, target);
    const Series expected = pred.sub(target).square().mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossMSE broadcast mismatch\n";
      return false;
    }
  }

  {
    const Series pred = pluss::make_series(std::vector<float>{1.0, -2.0, 0.5}, {3}, cfg);
    const Series target = pluss::make_series(std::vector<float>{0.0, 1.0, -1.5}, {3}, cfg);
    const Series l1 = pluss::nn::losses::L1(pred, target);
    const Series smooth = pluss::nn::losses::SmoothL1(
        pred,
        target,
        pluss::nn::losses::SmoothL1Options{.beta = 1.0, .reduction = pluss::nn::LossReduction::Mean});
    const Series expected_l1 = pred.sub(target).abs().mean();
    const Series expected_smooth = pred.sub(target).abs().lt(1.0).where(
        pred.sub(target).square().mul(0.5),
        pred.sub(target).abs().sub(0.5)).mean();
    if (!approx(as_f64(l1), as_f64(expected_l1))) {
      if (err) *err << "LossL1 mismatch\n";
      return false;
    }
    if (!approx(as_f64(smooth), as_f64(expected_smooth), 1e-11)) {
      if (err) *err << "LossSmoothL1 mismatch\n";
      return false;
    }
  }

  {
    const Series pred = pluss::make_series(std::vector<float>{1.0, 4.0}, {2}, cfg);
    const Series target = pluss::make_series(std::vector<float>{1.5, 2.0}, {2}, cfg);
    const Series loss = pluss::nn::losses::Huber(
        pred,
        target,
        pluss::nn::losses::HuberOptions{
            .delta = 1.0,
            .reduction = pluss::nn::LossReduction::Mean,
        });
    if (!approx(as_f64(loss), 0.8125)) {
      if (err) *err << "LossHuber mean mismatch\n";
      return false;
    }
  }

  {
    const Series logits = pluss::make_series(
        std::vector<float>{
            2.0, 0.0, -1.0,
            0.0, 2.0, -1.0,
        },
        {2, 3},
        cfg);
    const Series target = pluss::make_series(std::vector<std::int64_t>{0, 1}, {2}, cfg.with_dtype(ScalarType::Int32));
    const Series loss = pluss::nn::losses::CrossEntropy(logits, target);

    const Series expected = logits.log_softmax(1).gather(1, target.unsqueeze(1)).squeeze(1).neg().mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossCrossEntropy index-target mismatch\n";
      return false;
    }
  }

  {
    const Series logits = pluss::make_series(
        std::vector<float>{
            2.0, 0.0, -1.0,
            0.0, 2.0, -1.0,
        },
        {2, 3},
        cfg);
    const Series target_probs = pluss::make_series(
        std::vector<float>{
            0.7, 0.2, 0.1,
            0.1, 0.8, 0.1,
        },
        {2, 3},
        cfg);
    const Series loss = pluss::nn::losses::CrossEntropy(logits, target_probs);
    const Series expected = target_probs.mul(logits.log_softmax(1)).sum(1).neg().mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossCrossEntropy probabilistic-target mismatch\n";
      return false;
    }
  }

  {
    Series logits = pluss::make_series(
        std::vector<float>{
            1.5, -0.2, 0.4,
            0.1, 1.2, -0.3,
        },
        {2, 3},
        cfg);
    logits.tracks_slope_(true);
    const Series target = pluss::make_series(std::vector<std::int64_t>{2, 1}, {2}, cfg.with_dtype(ScalarType::Int64));
    const Series loss = pluss::nn::losses::CrossEntropy(logits, target);
    loss.reverse();
    if (!logits.has_sensitivity()) {
      if (err) *err << "LossCrossEntropy reverse gradient propagation mismatch\n";
      return false;
    }
  }

  {
    const Series pred = pluss::make_series(std::vector<float>{1.0, 2.0}, {2}, cfg);
    const Series target_i64 = pluss::make_series(std::vector<std::int64_t>{1, 2}, {2}, cfg.with_dtype(ScalarType::Int64));
    bool threw = false;
    try {
      (void)pluss::nn::losses::MSE(pred, target_i64);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "LossMSE dtype guard mismatch\n";
      return false;
    }
  }

  {
    const Series logits = pluss::make_series(std::vector<float>{0.0, 1.0, -1.0}, {3}, cfg);
    const Series target_i32 = pluss::make_series(std::vector<std::int32_t>{0, 1, 1}, {3}, cfg.with_dtype(ScalarType::Int32));
    bool threw = false;
    try {
      (void)pluss::nn::losses::BCEWithLogits(logits, target_i32);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "LossBCEWithLogits target dtype guard mismatch\n";
      return false;
    }
  }

  {
    const Series logits = pluss::make_series(std::vector<float>{0.0, 1.0, -1.0}, {3}, cfg);
    const Series target = pluss::make_series(std::vector<float>{0.0, 1.0, 1.0}, {3}, cfg);
    const Series loss = pluss::nn::losses::BCEWithLogits(
        logits,
        target,
        pluss::nn::losses::BCEWithLogitsOptions{.reduction = pluss::nn::LossReduction::Mean});
    const Series expected = logits.neg().softplus().add(logits.mul(pluss::make_series(1.0, cfg).sub(target))).mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossBCEWithLogits mismatch\n";
      return false;
    }
  }

  {
    const Series log_probs = pluss::make_series(
        std::vector<float>{
            -0.1, -2.4, -1.5,
            -1.8, -0.2, -2.0,
        },
        {2, 3},
        cfg);
    const Series target = pluss::make_series(std::vector<std::int64_t>{0, 1}, {2}, cfg.with_dtype(ScalarType::Int64));
    const Series loss = pluss::nn::losses::NLL(log_probs, target);
    const Series expected = log_probs.gather(1, target.unsqueeze(1)).squeeze(1).neg().mean();
    if (!approx(as_f64(loss), as_f64(expected))) {
      if (err) *err << "LossNLL mismatch\n";
      return false;
    }
  }

  {
    const Series logits = pluss::make_series(
        std::vector<float>{
            2.0, 0.2, -0.5,
            1.4, -0.2, 0.1,
            0.3, -0.7, 2.1,
        },
        {3, 3},
        cfg);
    const Series target = pluss::make_series(
        std::vector<std::int64_t>{0, -1, 2},
        {3},
        cfg.with_dtype(ScalarType::Int64));
    const Series class_weight = pluss::make_series(std::vector<float>{1.0, 2.0, 3.0}, {3}, cfg);
    const Series loss = pluss::nn::losses::CrossEntropy(
        logits,
        target,
        pluss::nn::losses::CrossEntropyOptions{
            .reduction = pluss::nn::LossReduction::Mean,
            .class_dim = 1,
            .label_smoothing = 0.0,
            .class_weight = class_weight,
            .use_ignore_index = true,
            .ignore_index = -1,
        });

    const Series log_probs = logits.log_softmax(1);
    const Series safe_target = target.eq(-1).where(
        pluss::series::zeros_like(target, ScalarType::Int64),
        target.to(ScalarType::Int64));
    const Series picked = log_probs.gather(1, safe_target.unsqueeze(1)).squeeze(1).neg();
    const Series w = class_weight.gather(0, safe_target);
    const Series mask = target.ne(-1);
    const Series numer = mask.where(picked.mul(w), pluss::series::zeros_like(picked, ScalarType::Float64)).sum();
    const Series denom = mask.where(w, pluss::series::zeros_like(w, ScalarType::Float64)).sum().clamp_min(1.0);
    const Series expected = numer.div(denom);

    if (!approx(as_f64(loss), as_f64(expected), 1e-5)) {
      if (err) *err << "LossCrossEntropy weight+ignore_index mismatch\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn loss modules compute expected values and support reverse", "[nn][loss]") {
  REQUIRE(run_nn_loss_checks(&std::cerr));
}
#else
int main() {
  if (!run_nn_loss_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nn_loss_test passed\n";
  return 0;
}
#endif
