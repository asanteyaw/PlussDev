#pragma once

#include "pluss/ai/ice/data.h"
#include "pluss/ai/ice/options.h"
#include "pluss/ai/ice/platform.h"
#include "pluss/nn.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace pluss::ai::ice::models {

// Canonical time-series model header.
//
// Temporal Fusion Transformer (Lim et al., 2019): a multi-horizon
// forecasting architecture combining variable selection, an LSTM
// encoder/decoder, static-covariate-conditioned gating throughout, and a
// causal self-attention layer for long-range dependencies, with quantile
// (not point) forecasts as output. ice's first `ice::models::` entry (see
// ICE_BLUEPRINT.md §8 Phase 4/5) — the "pre-built niche model, customize to
// your spec" half of ice's mandate, sitting on top of the generic Phase 1-3
// foundation (`ice::make_optimizer`) the same way `ice::Model`/`Outline` do.
//
// Deliberate scope cut, documented rather than silently assumed away:
//  - One shared LSTM context (from the static encoder) broadcast across
//    every LSTM layer, rather than a distinct per-layer projection.
//
// Categorical variables (a second scope cut through the rest of this
// session) are now supported — see CategoricalVariable and
// TFTOptions::static_categoricals/encoder_categoricals/decoder_categoricals
// below.
struct TFTOptions {
  // 0 disables the static covariate encoder entirely (contexts become
  // undefined and are simply not injected into variable selection/
  // enrichment/the LSTM's initial state).
  std::int64_t static_size = 0;
  // Number of time-varying features supplied at every past (lookback) step.
  std::int64_t encoder_size = 0;
  // Number of time-varying features known ahead of time for the forecast
  // horizon (calendar features, planned events, etc.) — must be >= 1.
  std::int64_t decoder_size = 0;
  std::int64_t encoder_length = 0; // lookback window length, in timesteps.
  std::int64_t decoder_length = 0; // forecast horizon length, in timesteps.
  std::int64_t hidden_size = 32;
  std::int64_t lstm_layers = 1;
  std::int64_t attention_heads = 4;
  double dropout = 0.1;
  // Forecast quantiles, e.g. {0.1, 0.5, 0.9} for an 80% interval plus the
  // median. Order here is the order predictions come back in.
  std::vector<double> quantiles = {0.1, 0.5, 0.9};

  // Marks one variable slot within a covariate group (static/encoder/
  // decoder) as categorical rather than continuous, and how many distinct
  // category values it has. `variable_index` is 0-based within that
  // group's own width (static_size/encoder_size/decoder_size) — e.g.
  // CategoricalVariable{2, 7} in static_categoricals means static_inputs'
  // 3rd column holds one of 7 category codes, not a continuous value. Any
  // index not listed in the matching *_categoricals vector is treated as
  // continuous, matching this model's original numeric-only behavior. The
  // same type data::TimeSeriesDataSet uses for its own categorical
  // bookkeeping (not a TFT-specific duplicate) — see from_dataset() below,
  // which fills static_size/encoder_size/decoder_size and these three
  // vectors from a dataset automatically.
  using CategoricalVariable = data::CategoricalVariable;
  std::vector<CategoricalVariable> static_categoricals = {};
  std::vector<CategoricalVariable> encoder_categoricals = {};
  std::vector<CategoricalVariable> decoder_categoricals = {};

  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;

  [[nodiscard]] static TFTOptions from_data(const data::DataSet& dataset) {
    return from_batch(dataset.batch());
  }

  [[nodiscard]] static TFTOptions from_batch(const data::Batch& batch) {
    TFTOptions out{};
    if (batch.has(data::field_names::static_inputs)) {
      out.static_size = batch.feature_width(data::field_names::static_inputs);
      out.static_categoricals = batch.categoricals(data::field_names::static_inputs);
    }
    out.encoder_size = batch.feature_width(data::field_names::encoder_inputs);
    out.decoder_size = batch.feature_width(data::field_names::decoder_inputs);
    out.encoder_categoricals = batch.categoricals(data::field_names::encoder_inputs);
    out.decoder_categoricals = batch.categoricals(data::field_names::decoder_inputs);
    return out;
  }

  [[nodiscard]] static TFTOptions from_dataset(const data::TimeSeriesDataSet& dataset) {
    return from_batch(dataset.batch().to_batch());
  }
};

// One training/inference example set. Shapes (N = batch size):
//   static_inputs:  [N, static_size]                     (undefined if static_size == 0)
//   encoder_inputs: [N, encoder_length, encoder_size]
//   decoder_inputs: [N, decoder_length, decoder_size]
//   target:         [N, decoder_length]                  (required for learn()/assess(), ignored by predict())
// Categorical variable slots (per TFTOptions::*_categoricals) still live in
// these same tensors, packed as their integer category code stored as a
// float (e.g. category 3 -> 3.0) — there's no separate tensor for them.
struct TFTBatch {
  Series static_inputs = pluss::series::undefined();
  Series encoder_inputs = pluss::series::undefined();
  Series decoder_inputs = pluss::series::undefined();
  Series target = pluss::series::undefined();

  TFTBatch() = default;
  explicit TFTBatch(const data::Batch& batch)
      : static_inputs(batch.has(data::field_names::static_inputs)
                          ? batch.field(data::field_names::static_inputs)
                          : pluss::series::undefined()),
        encoder_inputs(batch.field(data::field_names::encoder_inputs)),
        decoder_inputs(batch.field(data::field_names::decoder_inputs)),
        target(batch.target()) {}

  explicit TFTBatch(const data::TimeSeriesBatch& batch)
      : static_inputs(batch.static_inputs),
        encoder_inputs(batch.encoder_inputs),
        decoder_inputs(batch.decoder_inputs),
        target(batch.target) {}
};

struct TFTTrainOptions {
  std::int64_t batch_size = 64;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  bool shuffle = true;
  std::uint64_t seed = 0;
  ProgressPolicy progress{};
  ValidationPolicy validation{};
  ObjectiveRegularization objective_regularization{};
  LoopRegularization loop_regularization{};
  CheckpointPolicy checkpoint{};
  ExecutionPolicy execution{
      .grad_clip =
          {
              .mode = pluss::nn::optim::GradientClipMode::ByNorm,
              .threshold = 1.0,
          },
      .max_epochs = 60,
      .fail_on_non_finite = true,
      .platform = TrainPlatform::CPU,
  };
};

struct TFTForecast {
  Series quantiles = pluss::series::undefined();
  Series attention_weights = pluss::series::undefined();
};

class TemporalFusionTransformer final {
public:
  explicit TemporalFusionTransformer(TFTOptions options);

  void learn(const TFTBatch& train, const TFTTrainOptions& options = {});
  void learn(const TFTBatch& train, const TFTBatch& val, const TFTTrainOptions& options = {});

  void learn(const data::Batch& train, const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), options);
  }
  void learn(const data::Batch& train, const data::Batch& val, const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), TFTBatch(val), options);
  }
  void learn(const data::TimeSeriesBatch& train, const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), options);
  }
  void learn(const data::TimeSeriesBatch& train,
             const data::TimeSeriesBatch& val,
             const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), TFTBatch(val), options);
  }

  [[nodiscard]] TFTForecast predict(const TFTBatch& batch) const;
  [[nodiscard]] TFTForecast predict(const data::Batch& batch) const { return predict(TFTBatch(batch)); }
  [[nodiscard]] TFTForecast predict(const data::TimeSeriesBatch& batch) const {
    return predict(TFTBatch(batch));
  }

  [[nodiscard]] double assess(const TFTBatch& batch) const;
  [[nodiscard]] double assess(const data::Batch& batch) const { return assess(TFTBatch(batch)); }
  [[nodiscard]] double assess(const data::TimeSeriesBatch& batch) const {
    return assess(TFTBatch(batch));
  }

  [[nodiscard]] bool ready() const noexcept;
  TemporalFusionTransformer& to(Platform platform);

  void save(std::string_view filepath) const;
  [[nodiscard]] static TemporalFusionTransformer load(std::string_view filepath);

private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  TFTOptions options_;
};

} // namespace pluss::ai::ice::models
