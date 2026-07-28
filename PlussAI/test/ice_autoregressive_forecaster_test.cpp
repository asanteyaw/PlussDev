// Correctness coverage for ice::models::AutoregressiveForecaster on a
// synthetic autoregressive forecasting task with a known future driver.
#include <pluss/ai/ice.h>

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
namespace ice = pluss::ai::ice;
namespace data = pluss::ai::ice::data;
namespace models = pluss::ai::ice::models;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

struct SyntheticArData {
  data::TimeSeriesBatch batch{};
};

SyntheticArData make_synthetic(std::int64_t n,
                               std::int64_t encoder_length,
                               std::int64_t decoder_length,
                               std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> base_dist(-1.0, 1.0);
  std::uniform_real_distribution<double> y0_dist(-0.5, 0.5);
  std::normal_distribution<double> noise(0.0, 0.015);

  const std::int64_t total_steps = encoder_length + decoder_length;
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_inputs(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_inputs(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> encoder_target(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> target(static_cast<std::size_t>(n * decoder_length));

  for (std::int64_t i = 0; i < n; ++i) {
    const double base = base_dist(rng);
    double y_prev = y0_dist(rng) + 0.2 * base;
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(base);

    for (std::int64_t t = 0; t < total_steps; ++t) {
      const double driver = std::sin(0.2 * static_cast<double>(t + 1)) +
                            0.1 * static_cast<double>((i % 7) - 3);
      const double y_t = 0.72 * y_prev + 0.25 * base + 0.35 * driver + noise(rng);
      if (t < encoder_length) {
        encoder_inputs[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(driver);
        encoder_target[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(y_t);
      } else {
        const auto j = t - encoder_length;
        decoder_inputs[static_cast<std::size_t>(i * decoder_length + j)] = static_cast<float>(driver);
        target[static_cast<std::size_t>(i * decoder_length + j)] = static_cast<float>(y_t);
      }
      y_prev = y_t;
    }
  }

  const auto cfg = float_cfg();
  SyntheticArData out{};
  out.batch.static_inputs = pluss::make_series(static_flat, {n, 1}, cfg);
  out.batch.encoder_inputs = pluss::make_series(encoder_inputs, {n, encoder_length, 1}, cfg);
  out.batch.decoder_inputs = pluss::make_series(decoder_inputs, {n, decoder_length, 1}, cfg);
  out.batch.encoder_target = pluss::make_series(encoder_target, {n, encoder_length}, cfg);
  out.batch.target = pluss::make_series(target, {n, decoder_length}, cfg);
  return out;
}

bool run_autoregressive_forecaster_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kEncoderLength = 8;
  constexpr std::int64_t kDecoderLength = 4;

  const auto train_data = make_synthetic(320, kEncoderLength, kDecoderLength, 11ULL);
  const auto test_data = make_synthetic(80, kEncoderLength, kDecoderLength, 97ULL);

  const auto dataset = data::TimeSeriesDataSet::from_series(train_data.batch);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 24;
  options.rnn_layers = 2;
  options.dropout = 0.0;
  options.seed = 5;
  options.cfg = float_cfg();

  models::AutoregressiveForecaster model(options);
  if (model.ready()) {
    if (err) *err << "AutoregressiveForecaster should not report ready before learn()\n";
    return false;
  }

  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 220;
  train_options.batch_size = 64;
  train_options.learning_rate = 8e-3;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 30;
  train_options.seed = 3;

  model.learn(dataset.batch(), train_options);
  if (!model.ready()) {
    if (err) *err << "AutoregressiveForecaster should report ready after learn()\n";
    return false;
  }

  const auto forecast = model.predict(test_data.batch);
  const auto mean_sizes = forecast.mean.sizes();
  const auto scale_sizes = forecast.scale.sizes();
  if (mean_sizes.size() != 2 || mean_sizes[0] != 80 || mean_sizes[1] != kDecoderLength) {
    if (err) *err << "AutoregressiveForecaster predict() should return mean with shape [N, horizon]\n";
    return false;
  }
  if (scale_sizes != mean_sizes) {
    if (err) *err << "AutoregressiveForecaster scale should match mean shape exactly\n";
    return false;
  }

  const double mae = forecast.mean.sub(test_data.batch.target).abs().mean().item<double>();
  if (mae > 0.22) {
    if (err) *err << "AutoregressiveForecaster should learn the autoregressive mapping; MAE=" << mae << "\n";
    return false;
  }

  const double assessed = model.assess(test_data.batch);
  if (!std::isfinite(assessed)) {
    if (err) *err << "AutoregressiveForecaster assess() should return a finite Gaussian NLL\n";
    return false;
  }

  model.to(pluss::pCPU);
  const auto forecast_after_to = model.predict(test_data.batch);
  const double repeat_diff = forecast.mean.sub(forecast_after_to.mean).abs().amax().item<double>();
  if (repeat_diff > 1e-9) {
    if (err) *err << "AutoregressiveForecaster to(pCPU) should not change predictions when already on CPU\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice autoregressive forecaster learns a DeepAR-style task", "[ai][ice][ts]") {
  REQUIRE(run_autoregressive_forecaster_checks(&std::cerr));
}
#else
int main() {
  if (!run_autoregressive_forecaster_checks(&std::cerr)) return 1;
  std::cout << "ice_autoregressive_forecaster_test passed\n";
  return 0;
}
#endif
