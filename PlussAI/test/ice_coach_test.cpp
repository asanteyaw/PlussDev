// Direct coverage for ice::Coach as the central training/execution surface.
// Model keeps one-line convenience methods, but Coach is where the shared
// orchestration now lives, so it needs its own regression coverage.
#include <pluss/ai/ice.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

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
      predictions.to(pluss::pCPU, ScalarType::Float64).reshape({predictions.numel()}).to_vec<double>();
  const auto flat_b = chunked_predictions.to(pluss::pCPU, ScalarType::Float64)
                          .reshape({chunked_predictions.numel()})
                          .to_vec<double>();
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

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice Coach centralizes model learn infer and assess", "[ai][ice][coach]") {
  REQUIRE(run_ice_coach_checks());
}
#else
int main() {
  if (!run_ice_coach_checks(&std::cerr)) return 1;
  std::cout << "ice_coach_test passed\n";
  return 0;
}
#endif
