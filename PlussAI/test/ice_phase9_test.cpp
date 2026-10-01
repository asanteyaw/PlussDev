#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
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

struct RegressionData {
  Series x = pluss::series::undefined();
  Series y = pluss::series::undefined();
};

RegressionData make_regression_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::series::randn({rows, 3}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{1.25F, -1.75F, 0.55F}, std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::series::randn({rows, 1}, cfg, seed + 1).mul(0.02);
  const Series y = x.matmul(true_weight).add(noise);
  return RegressionData{x, y};
}

class PersistentEpochCallback final : public ice::Callback {
public:
  std::int64_t epochs_seen = 0;
  std::int64_t restored_epoch_count_before_first_resumed_epoch = -1;
  int checkpoint_loads = 0;

  void on_epoch_begin(ice::RunState& state) override {
    if (state.stage == ice::StepStage::Learn && checkpoint_loads > 0 &&
        restored_epoch_count_before_first_resumed_epoch < 0) {
      restored_epoch_count_before_first_resumed_epoch = epochs_seen;
    }
  }

  void on_epoch_end(ice::RunState& state) override {
    if (state.stage == ice::StepStage::Learn) {
      ++epochs_seen;
    }
  }

  void on_checkpoint_load(ice::RunState&, std::string_view) override { ++checkpoint_loads; }

  [[nodiscard]] std::string state_key() const override { return "persistent_epoch"; }

  [[nodiscard]] std::unordered_map<std::string, std::string> save_state() const override {
    return {{"epochs_seen", std::to_string(epochs_seen)}};
  }

  void load_state(const std::unordered_map<std::string, std::string>& state) override {
    const auto it = state.find("epochs_seen");
    if (it != state.end()) {
      epochs_seen = std::stoll(it->second);
    }
  }
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
    PLUSS_FAIL("phase9 explosion");
  }

private:
  Series weight_ = pluss::series::undefined();
};

bool run_resume_callback_state_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(96, 401);
  const auto temp_root =
      std::filesystem::temp_directory_path() / "pluss_ice_phase9_resume_callback";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root);
  const auto stem = (temp_root / "resume_callback").string();

  auto build_model = []() {
    return ice::Outline(pluss::nn::Fusion(3, 10), pluss::nn::ReLU(), pluss::nn::Fusion(10, 1));
  };

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 2;
  options.batch_size = 16;
  options.learning_rate = 0.03;
  options.validation.split_ratio = 0.2;
  options.shuffle = true;
  options.seed = 501;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.save_weights_only = false;
  options.checkpoint.every_n_epochs = 1;
  options.checkpoint.dirpath = temp_root.string();
  options.checkpoint.filename = "resume_callback";

  auto first_trace = std::make_shared<PersistentEpochCallback>();
  auto model = build_model();
  ice::Coach(options, {first_trace}).learn(model, data.x, data.y);

  auto resumed_trace = std::make_shared<PersistentEpochCallback>();
  auto resumed_model = build_model();
  auto resume_options = options;
  resume_options.execution.max_epochs = 3;
  resume_options.checkpoint.resume_from = stem + "-last.ckpt";
  ice::Coach(resume_options, {resumed_trace}).learn(resumed_model, data.x, data.y);

  const bool ok =
      resumed_trace->checkpoint_loads == 1 &&
      resumed_trace->restored_epoch_count_before_first_resumed_epoch == first_trace->epochs_seen;
  if (!ok && err) {
    *err << "Resumed callback state should be restored after on_run_begin and before the first resumed epoch; "
         << "expected epoch_count=" << first_trace->epochs_seen
         << " got=" << resumed_trace->restored_epoch_count_before_first_resumed_epoch << "\n";
  }
  std::filesystem::remove_all(temp_root);
  return ok;
}

bool run_weights_only_resume_guard_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(96, 402);
  const auto temp_root =
      std::filesystem::temp_directory_path() / "pluss_ice_phase9_weights_only";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root);

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 1;
  options.batch_size = 16;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.0;
  options.shuffle = false;
  options.seed = 502;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.save_weights_only = true;
  options.checkpoint.dirpath = temp_root.string();
  options.checkpoint.filename = "weights_only";

  auto model = ice::Outline(pluss::nn::Fusion(3, 8), pluss::nn::ReLU(), pluss::nn::Fusion(8, 1));
  ice::Coach(options).learn(model, data.x, data.y);

  const auto weights_path = temp_root / "weights_only-last.ckpt.weights";
  if (!std::filesystem::exists(weights_path)) {
    if (err) *err << "Weights-only checkpointing should emit the .weights artifact\n";
    std::filesystem::remove_all(temp_root);
    return false;
  }

  auto resumed = ice::Outline(pluss::nn::Fusion(3, 8), pluss::nn::ReLU(), pluss::nn::Fusion(8, 1));
  auto resume_options = options;
  resume_options.execution.max_epochs = 2;
  resume_options.checkpoint.resume_from = (temp_root / "weights_only-last.ckpt").string();
  try {
    ice::Coach(resume_options).learn(resumed, data.x, data.y);
    if (err) *err << "Resume from weights-only checkpoints should fail loudly\n";
    std::filesystem::remove_all(temp_root);
    return false;
  } catch (const std::exception& e) {
    if (std::string(e.what()).find("weights-only checkpoint") == std::string::npos) {
      if (err) *err << "Unexpected weights-only resume guard message: " << e.what() << "\n";
      std::filesystem::remove_all(temp_root);
      return false;
    }
  }

  std::filesystem::remove_all(temp_root);
  return true;
}

bool run_save_on_exception_checkpoint_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(64, 403);
  const auto temp_root =
      std::filesystem::temp_directory_path() / "pluss_ice_phase9_exception";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root);

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 2;
  options.batch_size = 16;
  options.learning_rate = 0.01;
  options.checkpoint.enabled = true;
  options.checkpoint.save_on_exception = true;
  options.checkpoint.save_weights_only = false;
  options.checkpoint.dirpath = temp_root.string();
  options.checkpoint.filename = "exception_case";

  ExplodingModel model{};
  bool threw = false;
  try {
    ice::Coach(options).learn(model, data.x, data.y);
  } catch (const std::exception&) {
    threw = true;
  }

  const auto checkpoint = temp_root / "exception_case-exception.ckpt";
  const bool ok = threw && std::filesystem::exists(checkpoint) &&
      std::filesystem::exists(checkpoint.string() + ".weights");
  if (!ok && err) {
    *err << "save_on_exception should persist exception checkpoint metadata and weights artifacts\n";
  }
  std::filesystem::remove_all(temp_root);
  return ok;
}

bool run_corrupt_checkpoint_rejection_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(96, 404);
  const auto temp_root =
      std::filesystem::temp_directory_path() / "pluss_ice_phase9_corrupt";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root);
  const auto checkpoint = temp_root / "corrupt-last.ckpt";

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 1;
  options.batch_size = 16;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.2;
  options.shuffle = true;
  options.seed = 503;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.save_weights_only = false;
  options.checkpoint.dirpath = temp_root.string();
  options.checkpoint.filename = "corrupt";

  auto model = ice::Outline(pluss::nn::Fusion(3, 10), pluss::nn::ReLU(), pluss::nn::Fusion(10, 1));
  ice::Coach(options).learn(model, data.x, data.y);

  {
    std::ofstream out(checkpoint, std::ios::app);
    out << "corrupt=1\n";
  }

  auto resumed = ice::Outline(pluss::nn::Fusion(3, 10), pluss::nn::ReLU(), pluss::nn::Fusion(10, 1));
  auto resume_options = options;
  resume_options.execution.max_epochs = 3;
  resume_options.checkpoint.resume_from = checkpoint.string();
  try {
    ice::Coach(resume_options).learn(resumed, data.x, data.y);
    if (err) *err << "Corrupted checkpoints should be rejected by checksum verification\n";
    std::filesystem::remove_all(temp_root);
    return false;
  } catch (const std::exception& e) {
    if (std::string(e.what()).find("checksum mismatch") == std::string::npos) {
      if (err) *err << "Unexpected corrupt checkpoint error: " << e.what() << "\n";
      std::filesystem::remove_all(temp_root);
      return false;
    }
  }

  std::filesystem::remove_all(temp_root);
  return true;
}

bool run_missing_best_monitor_guard_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(96, 405);

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 1;
  options.batch_size = 16;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.0;
  options.shuffle = false;
  options.seed = 504;
  options.checkpoint.enabled = true;
  options.checkpoint.save_best = true;
  options.checkpoint.save_weights_only = false;

  auto model = ice::Outline(pluss::nn::Fusion(3, 8), pluss::nn::ReLU(), pluss::nn::Fusion(8, 1));
  try {
    ice::Coach(options).learn(model, data.x, data.y);
    if (err) *err << "save_best should fail when the monitored metric is never produced\n";
    return false;
  } catch (const std::exception& e) {
    if (std::string(e.what()).find("checkpoint.save_best requires") == std::string::npos) {
      if (err) *err << "Unexpected missing-monitor guard message: " << e.what() << "\n";
      return false;
    }
  }
  return true;
}

bool run_negative_execution_policy_guard_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(96, 406);

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 1;
  options.execution.max_train_batches = -1;
  options.batch_size = 16;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.0;

  auto model = ice::Outline(pluss::nn::Fusion(3, 8), pluss::nn::ReLU(), pluss::nn::Fusion(8, 1));
  try {
    ice::Coach(options).learn(model, data.x, data.y);
    if (err) *err << "Negative execution batch caps should fail fast\n";
    return false;
  } catch (const std::exception& e) {
    if (std::string(e.what()).find("max_train_batches must be non-negative") == std::string::npos) {
      if (err) *err << "Unexpected negative execution policy guard message: " << e.what() << "\n";
      return false;
    }
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice phase 9 restores callback state on resume", "[ai][ice][phase9]") {
  REQUIRE(run_resume_callback_state_checks());
}

TEST_CASE("ice phase 9 rejects weights-only resume", "[ai][ice][phase9]") {
  REQUIRE(run_weights_only_resume_guard_checks());
}

TEST_CASE("ice phase 9 persists exception checkpoints", "[ai][ice][phase9]") {
  REQUIRE(run_save_on_exception_checkpoint_checks());
}

TEST_CASE("ice phase 9 rejects corrupted checkpoints", "[ai][ice][phase9]") {
  REQUIRE(run_corrupt_checkpoint_rejection_checks());
}

TEST_CASE("ice phase 9 guards missing best-monitor metrics", "[ai][ice][phase9]") {
  REQUIRE(run_missing_best_monitor_guard_checks());
}

TEST_CASE("ice phase 9 guards negative execution caps", "[ai][ice][phase9]") {
  REQUIRE(run_negative_execution_policy_guard_checks());
}
#else
int main() {
  bool ok = true;
  ok = run_resume_callback_state_checks(&std::cerr) && ok;
  ok = run_weights_only_resume_guard_checks(&std::cerr) && ok;
  ok = run_save_on_exception_checkpoint_checks(&std::cerr) && ok;
  ok = run_corrupt_checkpoint_rejection_checks(&std::cerr) && ok;
  ok = run_missing_best_monitor_guard_checks(&std::cerr) && ok;
  ok = run_negative_execution_policy_guard_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice_phase9_test passed\n";
  return 0;
}
#endif
