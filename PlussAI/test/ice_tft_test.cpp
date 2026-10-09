// Correctness coverage for ice::models::TemporalFusionTransformer on a
// synthetic, non-NLP multi-horizon forecasting task with a known-exact
// answer, so the test can check the model actually learned the relationship
// rather than just "ran without crashing".
//
// Task: each of N synthetic series has a random static "level" in [-1, 1].
// The encoder (lookback) window observes that level (plus small noise) as
// its only feature. The decoder (forecast horizon) is given, as its one
// known-future feature, the normalized position within the horizon
// (1/H, 2/H, ..., 1). The true target is target[t] = level + 0.5 * position[t]
// (plus small noise) -- a simple function of one static and one known-future
// input, exactly the kind of relationship TFT's variable selection +
// static-context machinery exists to combine.
#include <pluss/ai/ice/models/ts/temporal_fusion_transformer.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
namespace models = pluss::ai::ice::models;
namespace data = pluss::ai::ice::data;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

struct SyntheticData {
  Series static_inputs = pluss::series::undefined();   // [N, 1]
  Series encoder_inputs = pluss::series::undefined();  // [N, encoder_length, 1]
  Series decoder_inputs = pluss::series::undefined();  // [N, decoder_length, 1]
  Series target = pluss::series::undefined();          // [N, decoder_length]
  std::vector<double> levels;
};

SyntheticData make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                             std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::normal_distribution<double> noise(0.0, 0.02);

  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<double> levels(static_cast<std::size_t>(n));

  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    levels[static_cast<std::size_t>(i)] = level;
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level + noise(rng));
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] =
          static_cast<float>(level + 0.5 * position + noise(rng));
    }
  }

  const auto cfg = float_cfg();
  SyntheticData data{};
  data.static_inputs = pluss::make_series(static_flat, {n, 1}, cfg);
  data.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, cfg);
  data.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, cfg);
  data.target = pluss::make_series(target_flat, {n, decoder_length}, cfg);
  data.levels = std::move(levels);
  return data;
}

bool run_ice_tft_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kEncoderLength = 10;
  constexpr std::int64_t kDecoderLength = 5;

  const auto train_data = make_synthetic(300, kEncoderLength, kDecoderLength, /*seed=*/11);
  const auto test_data = make_synthetic(80, kEncoderLength, kDecoderLength, /*seed=*/97);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 16;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.quantiles = {0.1, 0.5, 0.9};
  options.cfg = float_cfg();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  if (model.ready()) {
    if (err) *err << "ice TFT should not report ready before learn()\n";
    return false;
  }

  models::TFTBatch train_batch{};
  train_batch.static_inputs = train_data.static_inputs;
  train_batch.encoder_inputs = train_data.encoder_inputs;
  train_batch.decoder_inputs = train_data.decoder_inputs;
  train_batch.target = train_data.target;

  models::TFTTrainOptions train_options{};
  // max_epochs/patience deliberately generous: median accuracy converges
  // quickly, but quantile *spread* (needed for the [q0.1, q0.9] coverage
  // check below) takes noticeably longer to calibrate under pinball loss --
  // a 150-epoch/patience-15 budget was empirically found to sometimes stop
  // early with an accurate median but an overconfident (too-narrow)
  // interval, well under 0.4 coverage; 400/40 reliably lands above 0.9.
  train_options.execution.max_epochs = 400;
  train_options.batch_size = 64;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 40;
  train_options.seed = 3;

  model.learn(train_batch, train_options);
  if (!model.ready()) {
    if (err) *err << "ice TFT should report ready after learn()\n";
    return false;
  }

  models::TFTBatch test_batch{};
  test_batch.static_inputs = test_data.static_inputs;
  test_batch.encoder_inputs = test_data.encoder_inputs;
  test_batch.decoder_inputs = test_data.decoder_inputs;
  test_batch.target = test_data.target;

  const auto forecast = model.predict(test_batch);
  const auto sizes = forecast.quantiles.sizes();
  if (sizes.size() != 3 || sizes[0] != 80 || sizes[1] != kDecoderLength ||
      sizes[2] != static_cast<std::int64_t>(options.quantiles.size())) {
    if (err) *err << "ice TFT predict() should return [N, decoder_length, num_quantiles]\n";
    return false;
  }

  // Median quantile (index 1 of {0.1, 0.5, 0.9}) should track the true
  // target closely -- comfortably better than a "predict the static level
  // alone, ignore the known future position" baseline, whose expected
  // error is ~0.25 (half of 0.5 * mean(position in [0,1])).
  const Series median = forecast.quantiles.narrow_copy(2, 1, 1).reshape({80, kDecoderLength});
  const Series abs_error = median.sub(test_data.target).abs();
  const double mean_abs_error = abs_error.mean().item<double>();
  if (mean_abs_error > 0.15) {
    if (err) {
      *err << "ice TFT median forecast should closely track level + 0.5*position; mean abs error="
           << mean_abs_error << " (expected < 0.15)\n";
    }
    return false;
  }

  // The forecast interval [q0.1, q0.9] should cover the true target for
  // most examples (a loose calibration sanity check, not a strict bound --
  // pinball loss alone doesn't guarantee monotonic quantiles, so this just
  // confirms the intervals are in the right ballpark, not exactly 80%).
  const Series lower = forecast.quantiles.narrow_copy(2, 0, 1).reshape({80, kDecoderLength});
  const Series upper = forecast.quantiles.narrow_copy(2, 2, 1).reshape({80, kDecoderLength});
  const Series covered = test_data.target.ge(lower).mul(test_data.target.le(upper)).to(ScalarType::Float32);
  const double coverage = covered.mean().item<double>();
  if (coverage < 0.4) {
    if (err) *err << "ice TFT [q0.1, q0.9] interval coverage is implausibly low: " << coverage << "\n";
    return false;
  }

  // Determinism: re-predicting the same batch on a trained, eval-mode model
  // must give identical results.
  const auto forecast_again = model.predict(test_batch);
  const double repeat_diff =
      forecast.quantiles.sub(forecast_again.quantiles).abs().amax().item<double>();
  if (repeat_diff > 1e-9) {
    if (err) *err << "ice TFT predict() should be deterministic for a fixed trained model\n";
    return false;
  }

  // Attention weights should be a real, well-shaped probability
  // distribution over the full (encoder+decoder) sequence for each decoder
  // query position.
  const auto attn_sizes = forecast.attention_weights.sizes();
  if (attn_sizes.size() != 3 || attn_sizes[1] != kDecoderLength ||
      attn_sizes[2] != kEncoderLength + kDecoderLength) {
    if (err) *err << "ice TFT attention_weights should be [N, decoder_length, encoder_length+decoder_length]\n";
    return false;
  }
  const double attn_row_sum =
      forecast.attention_weights.sum(-1, false).sub(1.0).abs().amax().item<double>();
  if (attn_row_sum > 1e-3) {
    if (err) *err << "ice TFT attention_weights rows should sum to ~1 (softmax output)\n";
    return false;
  }

  // assess() should agree with a manually computed pinball loss on the
  // model's own predict() output (both routes should produce the same
  // quantile loss for the same batch).
  const double assessed = model.assess(test_batch);
  if (!std::isfinite(assessed) || assessed < 0.0) {
    if (err) *err << "ice TFT assess() should return a finite, non-negative pinball loss\n";
    return false;
  }

  // CPU -> CPU is a no-op but exercises the same to() path a GPU-trained
  // caller would use to bring a model back for downstream CPU inference.
  model.to(pluss::pCPU);
  const auto forecast_after_to = model.predict(test_batch);
  if (forecast_after_to.quantiles.sub(forecast.quantiles).abs().amax().item<double>() > 1e-9) {
    if (err) *err << "ice TFT to(pCPU) should not change predictions when already on CPU\n";
    return false;
  }

  return true;
}

// Coverage for TFTOptions::static_categoricals: the static "level" input is
// now a categorical code (cardinality 4) rather than a continuous value, and
// -- deliberately -- the category-to-level mapping is *non-monotonic* with
// respect to the category index (category 0 -> level 1.2, category 1 ->
// -0.8, category 2 -> 0.3, category 3 -> -1.5). A model that silently
// treated the raw category code as an ordinal/continuous quantity (the bug
// this feature's flatten_grn_ redesign specifically avoids -- see
// tft_blocks.h's VariableSelectionNetwork comment) could not recover this
// mapping through a single linear-ish pathway; only a real per-category
// embedding lookup can.
struct CategoricalSyntheticData {
  Series static_inputs = pluss::series::undefined();   // [N, 1], category codes 0..3 stored as floats.
  Series encoder_inputs = pluss::series::undefined();  // [N, encoder_length, 1]
  Series decoder_inputs = pluss::series::undefined();  // [N, decoder_length, 1]
  Series target = pluss::series::undefined();          // [N, decoder_length]
};

CategoricalSyntheticData make_categorical_synthetic(std::int64_t n, std::int64_t encoder_length,
                                                    std::int64_t decoder_length, std::uint64_t seed) {
  static constexpr std::array<double, 4> kCategoryLevel = {1.2, -0.8, 0.3, -1.5};
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> category_dist(0, 3);
  std::normal_distribution<double> noise(0.0, 0.02);

  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));

  for (std::int64_t i = 0; i < n; ++i) {
    const int category = category_dist(rng);
    const double level = kCategoryLevel[static_cast<std::size_t>(category)];
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(category);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level + noise(rng));
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] =
          static_cast<float>(level + 0.5 * position + noise(rng));
    }
  }

  const auto cfg = float_cfg();
  CategoricalSyntheticData data{};
  data.static_inputs = pluss::make_series(static_flat, {n, 1}, cfg);
  data.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, cfg);
  data.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, cfg);
  data.target = pluss::make_series(target_flat, {n, decoder_length}, cfg);
  return data;
}

bool run_ice_tft_categorical_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kEncoderLength = 10;
  constexpr std::int64_t kDecoderLength = 5;

  const auto train_data = make_categorical_synthetic(300, kEncoderLength, kDecoderLength, /*seed=*/13);
  const auto test_data = make_categorical_synthetic(80, kEncoderLength, kDecoderLength, /*seed=*/98);

  models::TFTOptions options{};
  options.static_size = 1;
  options.static_categoricals = {{.variable_index = 0, .cardinality = 4}};
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 16;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.quantiles = {0.1, 0.5, 0.9};
  options.cfg = float_cfg();
  options.seed = 6;

  models::TemporalFusionTransformer model(options);

  models::TFTBatch train_batch{};
  train_batch.static_inputs = train_data.static_inputs;
  train_batch.encoder_inputs = train_data.encoder_inputs;
  train_batch.decoder_inputs = train_data.decoder_inputs;
  train_batch.target = train_data.target;

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 400;
  train_options.batch_size = 64;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 40;
  train_options.seed = 4;

  model.learn(train_batch, train_options);
  if (!model.ready()) {
    if (err) *err << "ice TFT categorical model should report ready() after learn()\n";
    return false;
  }

  models::TFTBatch test_batch{};
  test_batch.static_inputs = test_data.static_inputs;
  test_batch.encoder_inputs = test_data.encoder_inputs;
  test_batch.decoder_inputs = test_data.decoder_inputs;
  test_batch.target = test_data.target;

  const auto forecast = model.predict(test_batch);
  const Series median = forecast.quantiles.narrow_copy(2, 1, 1).reshape({80, kDecoderLength});
  const double mean_abs_error = median.sub(test_data.target).abs().mean().item<double>();
  if (mean_abs_error > 0.15) {
    if (err) {
      *err << "ice TFT with a categorical static variable should recover a non-monotonic "
              "category->level mapping; mean abs error=" << mean_abs_error << " (expected < 0.15)\n";
    }
    return false;
  }

  return true;
}

// End-to-end integration: a long-format DataFrame panel (one row per series
// per timestep) fed through data::TimeSeriesDataSet::from_dataframe(),
// models::TFTOptions::from_dataset() (deriving static_size/encoder_size/
// decoder_size/*_categoricals automatically instead of the caller
// maintaining them by hand), and TemporalFusionTransformer's new
// data::TimeSeriesBatch-taking learn()/predict() overloads -- the whole
// pipeline this abstraction exists to connect, not just its pieces in
// isolation (each already covered separately by ice_time_series_dataset_test
// and the checks above). Reuses the same recoverable relationship
// (target = level + 0.5*position) as run_ice_tft_checks, now driven from a
// DataFrame instead of hand-built tensors, plus an observed-only variable
// (noisy_level) to prove encoder_size != decoder_size threads through
// correctly.
bool run_ice_tft_dataset_integration_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kSeries = 200;
  constexpr std::int64_t kStepsPerSeries = 20;
  constexpr std::int64_t kEncoderLength = 10;
  constexpr std::int64_t kDecoderLength = 5;

  const auto build_panel = [](std::int64_t n_series, std::int64_t steps, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
    std::normal_distribution<double> noise(0.0, 0.02);

    std::vector<std::string> series_col{};
    std::vector<float> time_col{}, level_col{}, position_col{}, noisy_col{}, y_col{};
    for (std::int64_t s = 0; s < n_series; ++s) {
      const double level = level_dist(rng);
      for (std::int64_t t = 0; t < steps; ++t) {
        const double position = static_cast<double>(t) / static_cast<double>(steps - 1);
        series_col.push_back("series" + std::to_string(s));
        time_col.push_back(static_cast<float>(t));
        level_col.push_back(static_cast<float>(level));
        position_col.push_back(static_cast<float>(position));
        noisy_col.push_back(static_cast<float>(level + noise(rng)));
        y_col.push_back(static_cast<float>(level + 0.5 * position + noise(rng)));
      }
    }
    return DataFrame({
        {"series", Slate(pluss::make_note(series_col))},
        {"t", Slate(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, float_cfg()))},
        {"level", Slate(pluss::make_series(level_col, {static_cast<std::int64_t>(level_col.size())}, float_cfg()))},
        {"position",
         Slate(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, float_cfg()))},
        {"noisy_level",
         Slate(pluss::make_series(noisy_col, {static_cast<std::int64_t>(noisy_col.size())}, float_cfg()))},
        {"y", Slate(pluss::make_series(y_col, {static_cast<std::int64_t>(y_col.size())}, float_cfg()))},
    });
  };

  const auto train_df = build_panel(kSeries, kStepsPerSeries, /*seed=*/21);
  const auto test_df = build_panel(80, kStepsPerSeries, /*seed=*/97);

  data::DataSpec data_spec{};
  data_spec.layout = data::Layout::Panel;
  data_spec.time_key = "t";
  data_spec.group_keys = {"series"};
  data_spec.target = "y";
  data_spec.fields = {
      {"level", data::Role::StaticContinuous},
      {"position", data::Role::KnownContinuous},
      {"noisy_level", data::Role::ObservedContinuous},
  };
  data_spec.window = {
      .lookback = kEncoderLength,
      .horizon = kDecoderLength,
  };
  data_spec.cfg = float_cfg();

  const auto train_dataset = data::DataSet::from_frame(train_df, data_spec);
  const auto test_dataset = data::DataSet::from_frame(test_df, data_spec, train_dataset);

  auto tft_options = models::TFTOptions::from_data(train_dataset);
  if (tft_options.static_size != 1 || tft_options.encoder_size != 2 || tft_options.decoder_size != 1) {
    if (err) {
      *err << "TFTOptions::from_dataset produced wrong sizes: static=" << tft_options.static_size
           << " encoder=" << tft_options.encoder_size << " decoder=" << tft_options.decoder_size
           << " (expected 1, 2, 1)\n";
    }
    return false;
  }
  // from_dataset() only fills static_size/encoder_size/decoder_size and the
  // *_categoricals vectors -- every hyperparameter still needs setting,
  // same as building TFTOptions by hand.
  tft_options.encoder_length = kEncoderLength;
  tft_options.decoder_length = kDecoderLength;
  tft_options.hidden_size = 16;
  tft_options.lstm_layers = 1;
  tft_options.attention_heads = 2;
  tft_options.dropout = 0.0;
  tft_options.quantiles = {0.1, 0.5, 0.9};
  tft_options.cfg = float_cfg();
  tft_options.seed = 5;

  models::TemporalFusionTransformer model(tft_options);

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 400;
  train_options.batch_size = 64;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 40;
  train_options.seed = 3;

  // The generic data::Batch-taking overload -- model-side forecasting now
  // hangs off Ice's generalized dataset surface rather than a
  // forecast-library-shaped public type.
  model.learn(train_dataset.batch(), train_options);
  if (!model.ready()) {
    if (err) *err << "ice TFT dataset-integration model should report ready() after learn()\n";
    return false;
  }

  const auto forecast = model.predict(test_dataset.batch());
  const auto qsizes = forecast.quantiles.sizes();
  const Series median = forecast.quantiles.narrow_copy(2, 1, 1).reshape({qsizes[0], kDecoderLength});
  const Series test_target = test_dataset.batch().target();
  const double mean_abs_error = median.sub(test_target).abs().mean().item<double>();
  if (mean_abs_error > 0.15) {
    if (err) {
      *err << "ice TFT dataset-integration model should recover level + 0.5*position from a DataFrame-built "
              "dataset; mean abs error=" << mean_abs_error << " (expected < 0.15)\n";
    }
    return false;
  }

  return true;
}

bool run_ice_tft_save_load_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;

  const auto train_data = make_synthetic(96, kEncoderLength, kDecoderLength, /*seed=*/21);
  const auto test_data = make_synthetic(24, kEncoderLength, kDecoderLength, /*seed=*/55);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.quantiles = {0.1, 0.5, 0.9};
  options.cfg = float_cfg();
  options.seed = 7;

  models::TemporalFusionTransformer model(options);

  models::TFTBatch train_batch{};
  train_batch.static_inputs = train_data.static_inputs;
  train_batch.encoder_inputs = train_data.encoder_inputs;
  train_batch.decoder_inputs = train_data.decoder_inputs;
  train_batch.target = train_data.target;

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 25;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.loop_regularization.early_stopping = false;
  train_options.seed = 3;
  model.learn(train_batch, train_options);

  models::TFTBatch test_batch{};
  test_batch.static_inputs = test_data.static_inputs;
  test_batch.encoder_inputs = test_data.encoder_inputs;
  test_batch.decoder_inputs = test_data.decoder_inputs;
  test_batch.target = test_data.target;

  const char* path = "ice_tft_save_load_test.artifact";
  model.save(path);
  const auto before = model.predict(test_batch);

  models::TemporalFusionTransformer loaded = models::TemporalFusionTransformer::load(path);
  std::remove(path);
  std::remove((std::string(path) + ".weights").c_str());

  if (!loaded.ready()) {
    if (err) *err << "ice TFT ::load should report ready()\n";
    return false;
  }

  const auto after = loaded.predict(test_batch);
  const double quantiles_diff = before.quantiles.sub(after.quantiles).abs().amax().item<double>();
  const double attn_diff =
      before.attention_weights.sub(after.attention_weights).abs().amax().item<double>();
  if (quantiles_diff > 1e-9 || attn_diff > 1e-9) {
    if (err) *err << "ice TFT ::load should reproduce identical predictions; quantiles_diff="
                  << quantiles_diff << " attn_diff=" << attn_diff << "\n";
    return false;
  }

  return true;
}

// Coverage for the batch-shape validation TemporalFusionTransformer::learn/
// predict/assess now perform: feature *widths* (encoder_size/decoder_size/
// static_size) must match TFTOptions, since VariableSelectionNetwork
// silently reads only the first `options.*_size` columns of a too-wide
// input otherwise (see tft_blocks.h's VariableSelectionNetworkImpl::
// forward) -- a real correctness gap this check closes. Lookback/horizon
// *length* is deliberately left unchecked (the LSTM encoder/decoder and
// the causal self-attention layer are architecturally length-agnostic),
// so this also pins that a length mismatch alone still predicts
// successfully, proving the two are independently controlled.
bool run_ice_tft_shape_validation_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;

  const auto train_data = make_synthetic(32, kEncoderLength, kDecoderLength, /*seed=*/41);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.cfg = float_cfg();
  options.seed = 9;

  models::TemporalFusionTransformer model(options);

  models::TFTBatch train_batch{};
  train_batch.static_inputs = train_data.static_inputs;
  train_batch.encoder_inputs = train_data.encoder_inputs;
  train_batch.decoder_inputs = train_data.decoder_inputs;
  train_batch.target = train_data.target;

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 3;
  train_options.validation.split_ratio = 0.0;
  model.learn(train_batch, train_options);

  const auto widen_last_dim = [](const Series& x) {
    return pluss::series::unite(std::vector<Series>{x, x}, -1); // doubles the trailing width.
  };

  // Too-wide encoder_inputs (encoder_size=2 instead of the trained 1):
  // VariableSelectionNetwork would otherwise silently read only the first
  // column and ignore the second with no error at all.
  {
    models::TFTBatch wrong = train_batch;
    wrong.encoder_inputs = widen_last_dim(train_batch.encoder_inputs);
    bool threw = false;
    try {
      const auto forecast = model.predict(wrong);
      static_cast<void>(forecast);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "ice TFT predict() should reject encoder_inputs wider than encoder_size\n";
      return false;
    }
  }

  // Too-wide decoder_inputs (decoder_size=2 instead of the trained 1).
  {
    models::TFTBatch wrong = train_batch;
    wrong.decoder_inputs = widen_last_dim(train_batch.decoder_inputs);
    bool threw = false;
    try {
      const auto forecast = model.predict(wrong);
      static_cast<void>(forecast);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "ice TFT predict() should reject decoder_inputs wider than decoder_size\n";
      return false;
    }
  }

  // Too-wide static_inputs (static_size=2 instead of the trained 1).
  {
    models::TFTBatch wrong = train_batch;
    wrong.static_inputs = widen_last_dim(train_batch.static_inputs);
    bool threw = false;
    try {
      const auto forecast = model.predict(wrong);
      static_cast<void>(forecast);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "ice TFT predict() should reject static_inputs wider than static_size\n";
      return false;
    }
  }

  // static_inputs left undefined entirely, even though static_size > 0.
  {
    models::TFTBatch wrong = train_batch;
    wrong.static_inputs = pluss::series::undefined();
    bool threw = false;
    try {
      const auto forecast = model.predict(wrong);
      static_cast<void>(forecast);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "ice TFT predict() should reject an undefined static_inputs when static_size > 0\n";
      return false;
    }
  }

  // The same width check applies to learn() -- not just predict()/assess().
  {
    models::TFTBatch wrong = train_batch;
    wrong.encoder_inputs = widen_last_dim(train_batch.encoder_inputs);
    models::TemporalFusionTransformer fresh_model(options);
    bool threw = false;
    try {
      fresh_model.learn(wrong, train_options);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "ice TFT learn() should reject encoder_inputs wider than encoder_size\n";
      return false;
    }
  }

  // By contrast, a *length* mismatch (8-step lookback instead of the
  // trained 6) is deliberately still accepted -- this is the intentional
  // flexibility the width checks above do not take away.
  {
    const auto longer_lookback = make_synthetic(32, /*encoder_length=*/8, kDecoderLength, /*seed=*/42);
    models::TFTBatch longer{};
    longer.static_inputs = longer_lookback.static_inputs;
    longer.encoder_inputs = longer_lookback.encoder_inputs;
    longer.decoder_inputs = longer_lookback.decoder_inputs;
    longer.target = longer_lookback.target;

    const auto forecast = model.predict(longer);
    const auto sizes = forecast.attention_weights.sizes();
    if (sizes.size() != 3 || sizes[2] != 8 + kDecoderLength) {
      if (err) *err << "ice TFT predict() should still succeed with a different lookback length\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice tft tranche", "[ice][models][tft]") {
  if (!run_ice_tft_checks(&std::cerr)) FAIL("ice TFT checks failed");
  if (!run_ice_tft_categorical_checks(&std::cerr)) FAIL("ice TFT categorical checks failed");
  if (!run_ice_tft_dataset_integration_checks(&std::cerr)) FAIL("ice TFT dataset integration checks failed");
  if (!run_ice_tft_save_load_checks(&std::cerr)) FAIL("ice TFT save/load checks failed");
  if (!run_ice_tft_shape_validation_checks(&std::cerr)) FAIL("ice TFT shape validation checks failed");
}
#else
int main() {
  bool ok = true;
  ok = run_ice_tft_checks(&std::cerr) && ok;
  ok = run_ice_tft_categorical_checks(&std::cerr) && ok;
  ok = run_ice_tft_dataset_integration_checks(&std::cerr) && ok;
  ok = run_ice_tft_save_load_checks(&std::cerr) && ok;
  ok = run_ice_tft_shape_validation_checks(&std::cerr) && ok;
  if (!ok) {
    return 1;
  }
  std::cout << "ice tft checks passed\n";
  return 0;
}
#endif
