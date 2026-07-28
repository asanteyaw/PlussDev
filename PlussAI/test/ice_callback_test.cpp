#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
namespace ice = pluss::ai::ice;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

double first_scalar(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>().front();
}

struct RegressionData {
  Series x;
  Series y;
};

RegressionData make_regression_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::randn({rows, 3}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{1.5F, -2.25F, 0.75F}, std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::randn({rows, 1}, cfg, seed + 1).mul(0.03);
  const Series y = x.matmul(true_weight).add(noise);
  return RegressionData{x, y};
}

class TraceCallback final : public ice::Callback {
public:
  std::unordered_map<int, int> run_begin{};
  std::unordered_map<int, int> run_end{};
  std::unordered_map<int, int> batch_begin{};
  std::unordered_map<int, int> batch_end{};
  int epoch_begin = 0;
  int epoch_end = 0;
  int before_backward = 0;
  int after_backward = 0;
  int before_step = 0;
  int exceptions = 0;
  bool saw_epoch_loss = false;
  bool saw_epoch_val_loss = false;
  bool saw_test_metric = false;
  std::string last_exception{};

  void on_run_begin(ice::RunState& state) override { ++run_begin[static_cast<int>(state.stage)]; }
  void on_run_end(ice::RunState& state) override {
    ++run_end[static_cast<int>(state.stage)];
    if (state.stage == ice::StepStage::Test) {
      saw_test_metric = state.metric("rmse").has_value() || state.metric("accuracy").has_value();
    }
  }
  void on_epoch_begin(ice::RunState&) override { ++epoch_begin; }
  void on_epoch_end(ice::RunState& state) override {
    ++epoch_end;
    saw_epoch_loss = saw_epoch_loss || state.metric("loss").has_value();
    saw_epoch_val_loss = saw_epoch_val_loss || state.metric("val_loss").has_value();
  }
  void on_batch_begin(ice::RunState& state) override { ++batch_begin[static_cast<int>(state.stage)]; }
  void on_batch_end(ice::RunState& state) override { ++batch_end[static_cast<int>(state.stage)]; }
  void on_before_backward(ice::RunState&) override { ++before_backward; }
  void on_after_backward(ice::RunState&) override { ++after_backward; }
  void on_before_step(ice::RunState&) override { ++before_step; }
  void on_exception(ice::RunState& state) override {
    ++exceptions;
    last_exception = state.exception_message;
  }
};

class StopAfterEpochCallback final : public ice::Callback {
public:
  explicit StopAfterEpochCallback(std::int64_t stop_epoch) : stop_epoch_(stop_epoch) {}

  void on_epoch_end(ice::RunState& state) override {
    if (state.stage == ice::StepStage::Learn && state.epoch >= stop_epoch_) {
      state.stop_requested = true;
      state.stop_reason = "user_stop";
    }
  }

private:
  std::int64_t stop_epoch_ = 0;
};

class ExplodingModel final : public ice::Model {
public:
  ExplodingModel()
      : weight_(pluss::make_series(std::vector<float>{0.5F, -0.25F, 0.75F},
                                   std::vector<std::int64_t>{3, 1},
                                   float_cfg())) {
    register_parameter(weight_);
  }

  [[nodiscard]] Series forward(const Series& x) const override {
    return x.matmul(weight_);
  }

  [[nodiscard]] ice::StepResult learn_step(const ice::Batch&, const ice::StepContext&) const override {
    PLUSS_FAIL("kaboom");
  }

private:
  Series weight_ = Series::undefined();
};

bool run_callback_dispatch_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(256, 81);
  auto trace = std::make_shared<TraceCallback>();

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 80;
  options.batch_size = 32;
  options.learning_rate = 0.03;
  options.validation.split_ratio = 0.2;
  options.seed = 5;

  ice::Coach coach(options, {trace});
  auto model = ice::Outline(pluss::nn::Fusion(3, 12), pluss::nn::ReLU(), pluss::nn::Fusion(12, 1));
  coach.learn(model, data.x, data.y);
  const auto report = coach.assess(model, data.x, data.y, ice::LossKind::MSE);
  const auto predictions = coach.infer(model, data.x, 41);

  if (report.metric_name != "rmse" || report.metric > 0.6) {
    if (err) *err << "Callback dispatch integration should still train a usable model\n";
    return false;
  }
  if (predictions.sizes()[0] != data.x.sizes()[0]) {
    if (err) *err << "Callback dispatch integration should preserve infer row count\n";
    return false;
  }
  if (trace->run_begin[static_cast<int>(ice::StepStage::Learn)] != 1 ||
      trace->run_end[static_cast<int>(ice::StepStage::Learn)] != 1) {
    if (err) *err << "Train run begin/end hooks should fire exactly once\n";
    return false;
  }
  if (trace->run_begin[static_cast<int>(ice::StepStage::Test)] != 1 ||
      trace->run_end[static_cast<int>(ice::StepStage::Test)] != 1) {
    if (err) *err << "Assess run begin/end hooks should fire exactly once\n";
    return false;
  }
  if (trace->run_begin[static_cast<int>(ice::StepStage::Infer)] != 1 ||
      trace->run_end[static_cast<int>(ice::StepStage::Infer)] != 1) {
    if (err) *err << "Infer run begin/end hooks should fire exactly once\n";
    return false;
  }
  if (trace->epoch_begin <= 0 || trace->epoch_end <= 0) {
    if (err) *err << "Epoch hooks should fire during training\n";
    return false;
  }
  if (trace->batch_begin[static_cast<int>(ice::StepStage::Learn)] <= 0 ||
      trace->batch_end[static_cast<int>(ice::StepStage::Learn)] <= 0 ||
      trace->before_backward <= 0 || trace->after_backward <= 0 || trace->before_step <= 0) {
    if (err) *err << "Train batch/backward/step hooks should all fire\n";
    return false;
  }
  if (trace->batch_begin[static_cast<int>(ice::StepStage::Valid)] <= 0 ||
      trace->batch_end[static_cast<int>(ice::StepStage::Valid)] <= 0) {
    if (err) *err << "Validation batch hooks should fire when validation is enabled\n";
    return false;
  }
  if (trace->batch_begin[static_cast<int>(ice::StepStage::Test)] <= 0 ||
      trace->batch_end[static_cast<int>(ice::StepStage::Test)] <= 0 ||
      trace->batch_begin[static_cast<int>(ice::StepStage::Infer)] <= 0 ||
      trace->batch_end[static_cast<int>(ice::StepStage::Infer)] <= 0) {
    if (err) *err << "Test and infer batch hooks should fire\n";
    return false;
  }
  if (!trace->saw_epoch_loss || !trace->saw_epoch_val_loss || !trace->saw_test_metric) {
    if (err) *err << "Epoch/test hooks should receive aggregate metrics\n";
    return false;
  }
  return true;
}

bool run_user_stop_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(256, 82);
  auto trace = std::make_shared<TraceCallback>();
  auto stopper = std::make_shared<StopAfterEpochCallback>(1);

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 40;
  options.batch_size = 32;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.0;
  options.loop_regularization.early_stopping = false;
  options.seed = 9;

  ice::Coach coach(options, {trace, stopper});
  auto model = ice::Outline(pluss::nn::Fusion(3, 10), pluss::nn::ReLU(), pluss::nn::Fusion(10, 1));
  coach.learn(model, data.x, data.y);

  if (trace->epoch_end != 2) {
    if (err) *err << "User callback stop request should halt after the requested epoch boundary\n";
    return false;
  }
  return true;
}

bool run_exception_callback_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(64, 83);
  auto trace = std::make_shared<TraceCallback>();

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 2;
  options.batch_size = 16;
  options.learning_rate = 0.01;

  ice::Coach coach(options, {trace});
  ExplodingModel model{};
  bool caught = false;
  try {
    coach.learn(model, data.x, data.y);
  } catch (const std::exception&) {
    caught = true;
  }

  if (!caught) {
    if (err) *err << "Coach should rethrow training failures\n";
    return false;
  }
  if (trace->exceptions != 1 || trace->last_exception.find("kaboom") == std::string::npos) {
    if (err) *err << "Exception hook should receive the underlying failure message\n";
    return false;
  }
  return true;
}

bool run_early_stop_callback_unit_checks(std::ostream* err = nullptr) {
  ice::ValidationPolicy validation{};
  validation.monitor = "val_loss";
  validation.start_from_epoch = 0;
  validation.direction = ice::MonitorDirection::Min;

  ice::LoopRegularization regularization{};
  regularization.early_stopping = true;
  regularization.patience = 2;
  regularization.min_delta = 0.1;
  regularization.restore_best_weights = true;

  ice::EarlyStopCallback callback(validation, regularization);
  Series parameter = pluss::make_series(std::vector<float>{0.0F}, std::vector<std::int64_t>{1}, float_cfg());
  const std::vector<Series*> parameters{&parameter};
  ice::RunState state{
      .model = nullptr,
      .stage = ice::StepStage::Learn,
      .target_platform = pluss::pCPU,
      .epoch = -1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 1,
      .metrics = {},
      .parameters = &parameters,
      .stop_requested = false,
      .stop_reason = {},
      .exception_message = {},
  };

  callback.on_run_begin(state);

  parameter = pluss::make_series(std::vector<float>{0.0F}, std::vector<std::int64_t>{1}, float_cfg());
  state.epoch = 0;
  state.metrics.clear();
  state.put_metric("val_loss", 5.0);
  callback.on_epoch_end(state);

  parameter = pluss::make_series(std::vector<float>{1.0F}, std::vector<std::int64_t>{1}, float_cfg());
  state.stop_requested = false;
  state.epoch = 1;
  state.metrics.clear();
  state.put_metric("val_loss", 4.0);
  callback.on_epoch_end(state);

  parameter = pluss::make_series(std::vector<float>{2.0F}, std::vector<std::int64_t>{1}, float_cfg());
  state.stop_requested = false;
  state.epoch = 2;
  state.metrics.clear();
  state.put_metric("val_loss", 4.05);
  callback.on_epoch_end(state);

  parameter = pluss::make_series(std::vector<float>{3.0F}, std::vector<std::int64_t>{1}, float_cfg());
  state.stop_requested = false;
  state.epoch = 3;
  state.metrics.clear();
  state.put_metric("val_loss", 4.02);
  callback.on_epoch_end(state);

  if (!state.stop_requested || callback.stopped_epoch() != 3 || callback.best_epoch() != 1) {
    if (err) *err << "EarlyStopCallback should stop after patience exhaustion and remember the best epoch\n";
    return false;
  }
  callback.on_run_end(state);
  if (std::abs(first_scalar(parameter) - 1.0) > 1e-9) {
    if (err) *err << "EarlyStopCallback restore_best_weights should restore the best parameter snapshot\n";
    return false;
  }

  ice::ValidationPolicy accuracy_validation{};
  accuracy_validation.monitor = "val_accuracy";
  accuracy_validation.direction = ice::MonitorDirection::Auto;
  ice::LoopRegularization accuracy_regularization{};
  accuracy_regularization.early_stopping = true;
  accuracy_regularization.patience = 1;
  accuracy_regularization.min_delta = 0.0;
  ice::EarlyStopCallback accuracy_callback(accuracy_validation, accuracy_regularization);
  accuracy_callback.on_run_begin(state);
  state.stop_requested = false;
  state.epoch = 0;
  state.metrics.clear();
  state.put_metric("val_accuracy", 0.7);
  accuracy_callback.on_epoch_end(state);
  state.stop_requested = false;
  state.epoch = 1;
  state.metrics.clear();
  state.put_metric("val_accuracy", 0.6);
  accuracy_callback.on_epoch_end(state);
  if (!state.stop_requested) {
    if (err) *err << "EarlyStopCallback auto direction should treat accuracy-style metrics as maximize\n";
    return false;
  }

  state.stop_requested = false;
  state.epoch = 0;
  state.metrics.clear();
  ice::EarlyStopCallback missing_metric_callback(validation, regularization);
  missing_metric_callback.on_run_begin(state);
  try {
    missing_metric_callback.on_epoch_end(state);
    if (err) *err << "EarlyStopCallback should fail loudly when the monitor is missing\n";
    return false;
  } catch (const std::exception&) {
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice callback and event system phase 6", "[ai][ice][callback]") {
  REQUIRE(run_callback_dispatch_checks());
  REQUIRE(run_user_stop_checks());
  REQUIRE(run_exception_callback_checks());
  REQUIRE(run_early_stop_callback_unit_checks());
}
#else
int main() {
  bool ok = true;
  ok = run_callback_dispatch_checks(&std::cerr) && ok;
  ok = run_user_stop_checks(&std::cerr) && ok;
  ok = run_exception_callback_checks(&std::cerr) && ok;
  ok = run_early_stop_callback_unit_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice_callback_test passed\n";
  return 0;
}
#endif
