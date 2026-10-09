#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
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
  Series x = pluss::series::undefined();
  Series y = pluss::series::undefined();
};

RegressionData make_regression_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::series::randn({rows, 3}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{1.5F, -2.25F, 0.75F}, std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::series::randn({rows, 1}, cfg, seed + 1).mul(0.03);
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
  Series weight_ = pluss::series::undefined();
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

bool run_epoch_logger_checks(std::ostream* err = nullptr) {
  ice::ProgressPolicy policy{};
  policy.log_every_n_epochs = 2;
  policy.metrics = {"loss", "val_loss", "accuracy"};
  policy.precision = 4;

  std::ostringstream output{};
  ice::EpochLoggerCallback logger(policy, output);
  ice::TrainOptions recipe{};
  recipe.execution.max_epochs = 5;
  ice::RunState state{.stage = ice::StepStage::Learn, .recipe = &recipe};

  state.epoch = 0;
  state.put_metric("loss", 2.0);
  logger.on_epoch_end(state);
  if (!output.str().empty()) {
    if (err) *err << "Epoch logger should skip epochs outside its cadence\n";
    return false;
  }

  state.epoch = 1;
  state.metrics.clear();
  state.put_metric("loss", 1.25);
  state.put_metric("accuracy", 0.75);
  logger.on_epoch_end(state);
  if (output.str() != "epoch 2/5: loss=1.25, accuracy=0.75\n") {
    if (err) *err << "Epoch logger emitted an unexpected metric line: " << output.str();
    return false;
  }

  state.epoch = 4;
  state.metrics.clear();
  state.put_metric("loss", 0.5);
  logger.on_epoch_end(state);
  if (output.str().find("epoch 5/5: loss=0.5\n") == std::string::npos) {
    if (err) *err << "Epoch logger should always emit the final epoch\n";
    return false;
  }

  ice::ProgressPolicy bar_policy{};
  bar_policy.progress_bar = true;
  bar_policy.bar_width = 10;
  bar_policy.metrics = {"loss"};
  std::ostringstream bar_output{};
  ice::EpochLoggerCallback bar(bar_policy, bar_output);
  state.epoch = 1;
  state.metrics.clear();
  state.put_metric("loss", 1.0);
  bar.on_epoch_end(state);
  bar.on_run_end(state);
  if (bar_output.str().find("[====      ] 40% epoch 2/5: loss=1") == std::string::npos ||
      bar_output.str().back() != '\n') {
    if (err) *err << "Progress bar should report completion and terminate its live line\n";
    return false;
  }

  bool rejected = false;
  try {
    ice::ProgressPolicy invalid{};
    invalid.log_every_n_epochs = 0;
    ice::EpochLoggerCallback invalid_logger(invalid, output);
    static_cast<void>(invalid_logger);
  } catch (const std::exception&) {
    rejected = true;
  }
  if (!rejected) {
    if (err) *err << "Epoch logger should reject a non-positive cadence\n";
    return false;
  }

  return true;
}

bool run_automatic_progress_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(32, 85);
  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 3;
  options.batch_size = 32;
  options.validation.split_ratio = 0.0;
  options.loop_regularization.early_stopping = false;
  options.progress.verbose = true;
  options.progress.log_every_n_epochs = 2;
  options.progress.metrics = {"loss"};

  std::ostringstream output{};
  auto* original = std::cout.rdbuf(output.rdbuf());
  try {
    auto model = ice::Outline(pluss::nn::Fusion(3, 1));
    model.learn(data.x, data.y, options);
  } catch (...) {
    std::cout.rdbuf(original);
    throw;
  }
  std::cout.rdbuf(original);

  const std::string text = output.str();
  if (text.find("epoch 1/3") != std::string::npos ||
      text.find("epoch 2/3: loss=") == std::string::npos ||
      text.find("epoch 3/3: loss=") == std::string::npos) {
    if (err) *err << "Coach should install the configured epoch logger automatically\n";
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

  // Validation every second epoch: an off-cadence epoch without the metric
  // is skipped, so it neither stops the run nor throws.
  ice::ValidationPolicy cadence_validation = validation;
  cadence_validation.every_n_epochs = 2;
  ice::EarlyStopCallback cadence_callback(cadence_validation, regularization);
  cadence_callback.on_run_begin(state);
  state.stop_requested = false;
  state.epoch = 0;
  state.metrics.clear();
  state.put_metric("val_loss", 3.0);
  cadence_callback.on_epoch_end(state);
  state.epoch = 1;
  state.metrics.clear();
  cadence_callback.on_epoch_end(state);
  if (state.stop_requested || cadence_callback.wait_count() != 0) {
    if (err) *err << "EarlyStopCallback should skip epochs off the validation cadence\n";
    return false;
  }

  return true;
}

// Regression for a dead-code bug in EarlyStopCallback::on_run_begin: it
// used to guard its reset behind a `loaded_from_checkpoint_` flag that
// could never actually be true at that point (the flag is only set by
// load_state(), which Coach::learn calls *after* on_run_begin, by
// design -- on_run_begin's reset always runs first and is always
// overwritten with the real values afterward). The flag and its dead
// branch were removed; this test pins the behavior that actually
// matters: wait_count/best_value genuinely end up restored from the
// checkpoint once the resumed run reaches its first epoch, regardless
// of what on_run_begin itself did in between.
bool run_early_stop_checkpoint_resume_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(40, 84);
  const auto dirpath =
      (std::filesystem::temp_directory_path() / "ice_early_stop_resume_test").string();

  ice::TrainOptions save_options{};
  save_options.loss = ice::LossKind::MSE;
  save_options.execution.max_epochs = 3;
  save_options.batch_size = 40;
  save_options.learning_rate = 0.05;
  save_options.validation.split_ratio = 0.3;
  save_options.shuffle = false;
  save_options.loop_regularization.early_stopping = true;
  save_options.loop_regularization.patience = 100;  // never actually stops early
  save_options.checkpoint.enabled = true;
  save_options.checkpoint.save_last = true;
  save_options.checkpoint.dirpath = dirpath;
  save_options.checkpoint.filename = "resume";

  auto model = ice::Outline(pluss::nn::Fusion(3, 1));
  ice::Coach(save_options).learn(model, data.x, data.y);

  ice::TrainOptions resume_options = save_options;
  resume_options.execution.max_epochs = 4;
  resume_options.checkpoint.resume_from = dirpath + "/resume-last.ckpt";

  class CaptureAtEpochBeginCallback final : public ice::Callback {
   public:
    std::optional<std::int64_t> wait_count_at_epoch_begin{};
    bool best_value_defined_at_epoch_begin = false;
    void on_epoch_begin(ice::RunState&) override {
      if (wait_count_at_epoch_begin.has_value()) return;  // only the first (resumed) epoch
      wait_count_at_epoch_begin = early_stop->wait_count();
      best_value_defined_at_epoch_begin = early_stop->best_value().has_value();
    }
    std::shared_ptr<ice::EarlyStopCallback> early_stop{};
  };

  ice::ValidationPolicy validation{};
  ice::LoopRegularization regularization{};
  regularization.early_stopping = true;
  regularization.patience = 100;
  const auto resumed_early_stop = std::make_shared<ice::EarlyStopCallback>(validation, regularization);
  const auto capture = std::make_shared<CaptureAtEpochBeginCallback>();
  capture->early_stop = resumed_early_stop;

  auto resumed_model = ice::Outline(pluss::nn::Fusion(3, 1));
  ice::Coach resume_coach(resume_options);
  resume_coach.add_callback(resumed_early_stop);
  resume_coach.add_callback(capture);
  resume_coach.learn(resumed_model, data.x, data.y);

  std::filesystem::remove_all(dirpath);

  if (!capture->best_value_defined_at_epoch_begin) {
    if (err) {
      *err << "EarlyStopCallback should have its best_value restored by the time the resumed "
              "run's first epoch begins\n";
    }
    return false;
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
  REQUIRE(run_epoch_logger_checks());
  REQUIRE(run_automatic_progress_checks());
}

TEST_CASE("ice EarlyStopCallback state survives a checkpoint resume", "[ai][ice][callback][coach]") {
  REQUIRE(run_early_stop_checkpoint_resume_checks());
}
#else
int main() {
  bool ok = true;
  ok = run_callback_dispatch_checks(&std::cerr) && ok;
  ok = run_user_stop_checks(&std::cerr) && ok;
  ok = run_exception_callback_checks(&std::cerr) && ok;
  ok = run_early_stop_callback_unit_checks(&std::cerr) && ok;
  ok = run_epoch_logger_checks(&std::cerr) && ok;
  ok = run_automatic_progress_checks(&std::cerr) && ok;
  ok = run_early_stop_checkpoint_resume_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice_callback_test passed\n";
  return 0;
}
#endif
