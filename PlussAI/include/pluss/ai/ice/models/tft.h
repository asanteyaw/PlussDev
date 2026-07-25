#pragma once

#include "pluss/ai/ice/data/time_series_dataset.h"
#include "pluss/ai/ice/options.h"
#include "pluss/ai/ice/platform.h"
#include "pluss/nn.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace pluss::ai::ice::models {

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

  // Fills static_size/encoder_size/decoder_size and the three
  // *_categoricals vectors above from `dataset`'s own variable-role
  // bookkeeping -- the point of building the TFTOptions/TFTBatch pair off
  // a shared data::TimeSeriesDataSet instead of maintaining both by hand.
  // Every other field (hidden_size, lstm_layers, attention_heads,
  // quantiles, cfg, seed, ...) keeps its default; set those on the
  // returned struct before constructing the model.
  [[nodiscard]] static TFTOptions from_dataset(const data::TimeSeriesDataSet& dataset) {
    TFTOptions out{};
    const auto& batch = dataset.batch();
    out.static_size = batch.static_size;
    out.encoder_size = batch.encoder_size;
    out.decoder_size = batch.decoder_size;
    out.static_categoricals = batch.static_categoricals;
    out.encoder_categoricals = batch.encoder_categoricals;
    out.decoder_categoricals = batch.decoder_categoricals;
    return out;
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
  Series static_inputs = Series::undefined();
  Series encoder_inputs = Series::undefined();
  Series decoder_inputs = Series::undefined();
  Series target = Series::undefined();

  TFTBatch() = default;
  // Tensor shapes match data::TimeSeriesBatch exactly -- TFT was the first
  // consumer of that generic windowing/covariate abstraction, so this is a
  // plain field copy, not a reinterpretation. Explicit rather than
  // implicit so a caller can see at the call site that a conversion is
  // happening; TemporalFusionTransformer's learn()/predict()/assess()
  // overloads taking a data::TimeSeriesBatch directly do this for you.
  explicit TFTBatch(const data::TimeSeriesBatch& batch)
      : static_inputs(batch.static_inputs),
        encoder_inputs(batch.encoder_inputs),
        decoder_inputs(batch.decoder_inputs),
        target(batch.target) {}
};

// Deliberately not ice::TrainOptions: TFT's loss is always pinball/quantile
// (there's no LossKind choice to make), and training operates on a
// multi-tensor TFTBatch rather than a single (x, y) Series pair, so it
// can't reuse ice::Model's DataLoader-based train_loop_ either — same
// "doesn't fit ice::Model's generic shape" finding as Phase 2's NodeB
// migration survey. What DOES carry over is ice::make_optimizer() for
// construction (enum-selected optimizer, gradient clipping, non-finite
// diagnostics) and the TrainPlatform device-selection convenience from
// Phase 1.5.
struct TFTTrainOptions {
  std::int64_t max_epochs = 60;
  std::int64_t batch_size = 64;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  pluss::nn::optim::GradientClipOptions grad_clip{
      .mode = pluss::nn::optim::GradientClipMode::ByNorm,
      .threshold = 1.0,
  };
  bool fail_on_non_finite = true;
  bool shuffle = true;
  std::uint64_t seed = 0;
  // Only consulted by the single-TFTBatch learn() overload: carves an
  // internal validation split for early stopping. 0 disables it.
  double val_ratio = 0.1;
  bool early_stopping = true;
  std::int64_t patience = 10;
  double min_delta = 1e-6;
  TrainPlatform platform = TrainPlatform::CPU;
};

struct TFTForecast {
  Series quantiles = Series::undefined();          // [N, decoder_length, num_quantiles]
  // Averaged self-attention weights for the decoder's queries against the
  // full (encoder+decoder) sequence it attends over -- TFT's own
  // "interpretable" selling point, exposed rather than discarded.
  Series attention_weights = Series::undefined();  // [N, decoder_length, encoder_length + decoder_length]
};

class TemporalFusionTransformer final {
public:
  explicit TemporalFusionTransformer(TFTOptions options);

  void learn(const TFTBatch& train, const TFTTrainOptions& options = {});
  // Trains on an already-split train/val pair, for callers managing their
  // own splitting (mirrors ice::Model's two learn() overloads).
  void learn(const TFTBatch& train, const TFTBatch& val, const TFTTrainOptions& options = {});

  // Same two overloads, taking a data::TimeSeriesBatch directly (e.g.
  // straight from data::TimeSeriesDataSet::batch()) instead of requiring
  // the caller to wrap it in a TFTBatch first.
  void learn(const data::TimeSeriesBatch& train, const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), options);
  }
  void learn(const data::TimeSeriesBatch& train, const data::TimeSeriesBatch& val,
            const TFTTrainOptions& options = {}) {
    learn(TFTBatch(train), TFTBatch(val), options);
  }

  [[nodiscard]] TFTForecast predict(const TFTBatch& batch) const;
  [[nodiscard]] TFTForecast predict(const data::TimeSeriesBatch& batch) const { return predict(TFTBatch(batch)); }

  // Mean pinball loss over `batch.target` at the configured quantiles.
  [[nodiscard]] double assess(const TFTBatch& batch) const;
  [[nodiscard]] double assess(const data::TimeSeriesBatch& batch) const { return assess(TFTBatch(batch)); }

  [[nodiscard]] bool ready() const noexcept;

  // Moves every parameter to `platform` in place — same escape hatch as
  // ice::Model::to(), useful to bring a model trained with
  // TrainPlatform::GPU back to CPU for downstream inference.
  TemporalFusionTransformer& to(Platform platform);

private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  TFTOptions options_;
};

} // namespace pluss::ai::ice::models
