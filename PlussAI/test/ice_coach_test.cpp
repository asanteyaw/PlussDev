// Direct coverage for ice::Coach as the central training/execution surface.
// Model keeps one-line convenience methods, but Coach is where the shared
// orchestration now lives, so it needs its own regression coverage.
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
  Series x = pluss::series::undefined();
  Series y = pluss::series::undefined();
};

RegressionData make_regression_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::series::randn({rows, 3}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{1.25F, -2.0F, 0.75F}, std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::series::randn({rows, 1}, cfg, seed + 1).mul(0.03);
  const Series y = x.matmul(true_weight).add(noise);
  return RegressionData{x, y};
}

bool run_ice_coach_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(320, 77);

  auto model = ice::Outline(pluss::nn::Fusion(3, 12), pluss::nn::ReLU(), pluss::nn::Fusion(12, 1));

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 140;
  options.batch_size = 64;
  options.learning_rate = 0.03;
  options.validation.split_ratio = 0.2;
  options.seed = 19;

  const ice::Coach coach(options);
  coach.learn(model, data.x, data.y);

  if (!model.ready()) {
    if (err) *err << "ice Coach should mark the trained model as ready\n";
    return false;
  }

  const auto report = coach.assess(model, data.x, data.y, ice::LossKind::MSE);
  if (report.metric_name != "rmse" || report.metric > 0.45) {
    if (err) *err << "ice Coach should fit a clean regression target; got rmse=" << report.metric << "\n";
    return false;
  }

  const Series predictions = coach.infer(model, data.x);
  const Series chunked_predictions = coach.infer(model, data.x, /*chunk_size=*/53);
  const auto flat_a =
      predictions.to(pluss::pCPU, ScalarType::Float64).reshape({predictions.numel()}).host<double>();
  const auto flat_b = chunked_predictions.to(pluss::pCPU, ScalarType::Float64)
                          .reshape({chunked_predictions.numel()})
                          .host<double>();
  if (flat_a.size() != flat_b.size()) {
    if (err) *err << "ice Coach chunked infer() should preserve row count\n";
    return false;
  }
  for (std::size_t i = 0; i < flat_a.size(); ++i) {
    if (std::abs(flat_a[i] - flat_b[i]) > 1e-4) {
      if (err) *err << "ice Coach chunked infer() should closely match full infer()\n";
      return false;
    }
  }

  return true;
}

// Regression for a dangling-string_view bug in maybe_save_checkpoint_:
// the save_last call site passes state.last_checkpoint_path itself as
// `path`, and the function used to reassign that same field (and
// state.checkpoint_path) to a fresh std::string *before* forwarding
// `path` to on_checkpoint_save -- once the reassignment reallocated the
// string's heap buffer, `path` dangled and the callback observed
// corrupted bytes instead of the real filepath.
class CaptureCheckpointSaveCallback final : public ice::Callback {
 public:
  int fire_count = 0;
  std::string last_filepath{};
  void on_checkpoint_save(ice::RunState& state, std::string_view filepath) override {
    ++fire_count;
    last_filepath = std::string(filepath);
    static_cast<void>(state);
  }
};

bool run_ice_coach_checkpoint_callback_checks(std::ostream* err = nullptr) {
  const auto data = make_regression_data(32, 11);
  auto model = ice::Outline(pluss::nn::Fusion(3, 1));

  const auto dirpath =
      (std::filesystem::temp_directory_path() / "ice_coach_checkpoint_callback_test").string();

  ice::TrainOptions options{};
  options.execution.max_epochs = 2;
  options.batch_size = 32;
  options.learning_rate = 0.05;
  options.validation.split_ratio = 0.0;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.dirpath = dirpath;
  options.checkpoint.filename = "demo";

  const auto expected_path = (std::filesystem::path(dirpath) / "demo-last.ckpt").string();

  const auto capture = std::make_shared<CaptureCheckpointSaveCallback>();
  ice::Coach coach(options);
  coach.add_callback(capture);
  coach.learn(model, data.x, data.y);

  if (capture->fire_count != options.execution.max_epochs) {
    if (err) *err << "on_checkpoint_save should fire once per epoch when save_last is set\n";
    return false;
  }
  if (capture->last_filepath != expected_path) {
    if (err) *err << "on_checkpoint_save received a corrupted/stale filepath: \""
                  << capture->last_filepath << "\" (expected \"" << expected_path << "\")\n";
    return false;
  }

  std::filesystem::remove_all(dirpath);
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice Coach centralizes model learn infer and assess", "[ai][ice][coach]") {
  REQUIRE(run_ice_coach_checks());
}

TEST_CASE("ice Coach on_checkpoint_save receives the real filepath, not a dangling view",
          "[ai][ice][coach][callback]") {
  REQUIRE(run_ice_coach_checkpoint_callback_checks());
}
#else
int main() {
  if (!run_ice_coach_checks(&std::cerr)) return 1;
  if (!run_ice_coach_checkpoint_callback_checks(&std::cerr)) return 1;
  std::cout << "ice_coach_test passed\n";
  return 0;
}
#endif
