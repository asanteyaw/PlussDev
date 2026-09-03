#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
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
  Series x;
  Series y;
};

RegressionData make_regression_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::series::randn({rows, 3}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{1.4F, -1.8F, 0.65F}, std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::series::randn({rows, 1}, cfg, seed + 1).mul(0.02);
  const Series y = x.matmul(true_weight).add(noise);
  return RegressionData{x, y};
}

std::vector<double> flat_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

class StepCountCallback final : public ice::Callback {
public:
  int before_step = 0;
  int checkpoint_saves = 0;
  int checkpoint_loads = 0;

  void on_before_step(ice::RunState&) override { ++before_step; }
  void on_checkpoint_save(ice::RunState&, std::string_view) override { ++checkpoint_saves; }
  void on_checkpoint_load(ice::RunState&, std::string_view) override { ++checkpoint_loads; }
};

bool run_phase78_deterministic_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(160, 301);

  auto build_model = []() {
    return ice::Outline(pluss::nn::Fusion(3, 10), pluss::nn::ReLU(), pluss::nn::Fusion(10, 1));
  };

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 12;
  options.execution.deterministic = true;
  options.execution.max_train_batches = 4;
  options.execution.max_valid_batches = 0;
  options.execution.precision = ScalarType::Float32;
  options.batch_size = 16;
  options.learning_rate = 0.03;
  options.validation.split_ratio = 0.0;
  options.shuffle = true;
  options.seed = 77;

  pluss::tils::rng_seed(options.seed);
  auto model_a = build_model();
  pluss::tils::rng_seed(options.seed);
  auto model_b = build_model();
  ice::Coach(options).learn(model_a, data.x, data.y);
  ice::Coach(options).learn(model_b, data.x, data.y);

  const auto pred_a = flat_vec(model_a.infer(data.x));
  const auto pred_b = flat_vec(model_b.infer(data.x));
  if (pred_a.size() != pred_b.size()) {
    if (err) *err << "Deterministic repeated runs should preserve prediction shape\n";
    return false;
  }
  for (std::size_t i = 0; i < pred_a.size(); ++i) {
    if (std::abs(pred_a[i] - pred_b[i]) > 1e-8) {
      if (err) *err << "Deterministic repeated runs should match exactly on CPU\n";
      return false;
    }
  }
  return true;
}

bool run_phase78_accumulation_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(128, 302);
  auto trace = std::make_shared<StepCountCallback>();

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.execution.max_epochs = 1;
  options.execution.accumulation_steps = 2;
  options.execution.max_train_batches = 5;
  options.batch_size = 16;
  options.learning_rate = 0.02;
  options.validation.split_ratio = 0.0;
  options.shuffle = false;
  options.seed = 88;

  auto model = ice::Outline(pluss::nn::Fusion(3, 8), pluss::nn::ReLU(), pluss::nn::Fusion(8, 1));
  ice::Coach coach(options, {trace});
  coach.learn(model, data.x, data.y);

  if (trace->before_step != 3) {
    if (err) *err << "Accumulation should step ceil(5/2)=3 times, got " << trace->before_step << "\n";
    return false;
  }
  return true;
}

bool run_phase78_checkpoint_resume_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(256, 303);
  auto trace = std::make_shared<StepCountCallback>();

  const auto temp_root =
      std::filesystem::temp_directory_path() / "pluss_ice_phase78_checkpoint";
  std::filesystem::create_directories(temp_root);
  const auto stem = (temp_root / "resume_case").string();

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 3;
  options.execution.deterministic = true;
  options.batch_size = 32;
  options.learning_rate = 0.025;
  options.validation.split_ratio = 0.2;
  options.shuffle = true;
  options.seed = 99;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.save_best = true;
  options.checkpoint.save_weights_only = false;
  options.checkpoint.every_n_epochs = 1;
  options.checkpoint.dirpath = temp_root.string();
  options.checkpoint.filename = "resume_case";

  auto build_model = []() {
    return ice::Outline(pluss::nn::Fusion(3, 12), pluss::nn::ReLU(), pluss::nn::Fusion(12, 1));
  };

  auto model = build_model();
  ice::Coach(options, {trace}).learn(model, data.x, data.y);

  const auto last_checkpoint = stem + "-last.ckpt";
  const auto best_checkpoint = stem + "-best.ckpt";
  if (!std::filesystem::exists(last_checkpoint) ||
      !std::filesystem::exists(last_checkpoint + ".weights") ||
      !std::filesystem::exists(best_checkpoint) ||
      !std::filesystem::exists(best_checkpoint + ".weights")) {
    if (err) *err << "Full checkpointing should materialize last/best checkpoint artifacts\n";
    return false;
  }
  if (trace->checkpoint_saves <= 0) {
    if (err) *err << "Checkpoint callback hook should fire during checkpoint saves\n";
    return false;
  }

  auto resumed = build_model();
  auto resume_trace = std::make_shared<StepCountCallback>();
  auto resume_options = options;
  resume_options.execution.max_epochs = 5;
  resume_options.checkpoint.resume_from = last_checkpoint;
  ice::Coach(resume_options, {resume_trace}).learn(resumed, data.x, data.y);

  if (resume_trace->checkpoint_loads != 1) {
    if (err) *err << "Resume should trigger exactly one checkpoint-load callback\n";
    return false;
  }
  const auto report = resumed.assess(data.x, data.y, ice::LossKind::MSE);
  if (report.metric_name != "rmse" || report.metric > 0.75) {
    if (err) *err << "Resumed model should remain trainable after checkpoint restore\n";
    return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice phase 7 and 8 deterministic execution stays reproducible", "[ai][ice][phase78]") {
  REQUIRE(run_phase78_deterministic_checks());
}

TEST_CASE("ice phase 8 accumulation reduces optimizer step count", "[ai][ice][phase78]") {
  REQUIRE(run_phase78_accumulation_checks());
}

TEST_CASE("ice phase 7 full checkpointing resumes training", "[ai][ice][phase78]") {
  REQUIRE(run_phase78_checkpoint_resume_checks());
}
#else
int main() {
  bool ok = true;
  ok = run_phase78_deterministic_checks(&std::cerr) && ok;
  ok = run_phase78_accumulation_checks(&std::cerr) && ok;
  ok = run_phase78_checkpoint_resume_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice_phase78_test passed\n";
  return 0;
}
#endif
