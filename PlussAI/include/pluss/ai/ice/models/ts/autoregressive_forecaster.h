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
// DeepAR-style probabilistic autoregressive forecaster built on Ice's
// generic time-series batch substrate. Public name stays pluss-native:
// this is the engine's baseline autoregressive forecasting surface rather
// than a verbatim borrowed framework label.
struct AutoregressiveForecasterOptions {
  std::int64_t static_size = 0;
  std::int64_t encoder_size = 0;
  std::int64_t decoder_size = 0;
  std::int64_t encoder_length = 0;
  std::int64_t decoder_length = 0;
  std::int64_t hidden_size = 32;
  std::int64_t rnn_layers = 2;
  double dropout = 0.1;

  using CategoricalVariable = data::CategoricalVariable;
  std::vector<CategoricalVariable> static_categoricals = {};
  std::vector<CategoricalVariable> encoder_categoricals = {};
  std::vector<CategoricalVariable> decoder_categoricals = {};

  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;

  [[nodiscard]] static AutoregressiveForecasterOptions from_data(const data::DataSet& dataset) {
    AutoregressiveForecasterOptions out{};
    const auto& batch = dataset.batch();
    if (batch.has(data::field_names::static_inputs)) {
      out.static_size = batch.feature_width(data::field_names::static_inputs);
      out.static_categoricals = batch.categoricals(data::field_names::static_inputs);
    }
    if (batch.has(data::field_names::encoder_inputs)) {
      out.encoder_size = batch.feature_width(data::field_names::encoder_inputs);
      out.encoder_categoricals = batch.categoricals(data::field_names::encoder_inputs);
      out.encoder_length = batch.field(data::field_names::encoder_inputs).sizes()[1];
    }
    if (batch.has(data::field_names::decoder_inputs)) {
      out.decoder_size = batch.feature_width(data::field_names::decoder_inputs);
      out.decoder_categoricals = batch.categoricals(data::field_names::decoder_inputs);
      out.decoder_length = batch.field(data::field_names::decoder_inputs).sizes()[1];
    }
    if (out.encoder_length == 0 && batch.has(data::field_names::encoder_target)) {
      out.encoder_length = batch.field(data::field_names::encoder_target).sizes()[1];
    }
    if (out.decoder_length == 0 && batch.target().defined()) {
      out.decoder_length = batch.target().sizes()[1];
    }
    return out;
  }

  [[nodiscard]] static AutoregressiveForecasterOptions from_dataset(const data::TimeSeriesDataSet& dataset) {
    return from_data(dataset.data());
  }
};

struct AutoregressiveBatch {
  Series static_inputs = Series::undefined();
  Series encoder_inputs = Series::undefined();
  Series decoder_inputs = Series::undefined();
  Series encoder_target = Series::undefined();
  Series target = Series::undefined();

  AutoregressiveBatch() = default;
  explicit AutoregressiveBatch(const data::Batch& batch)
      : static_inputs(batch.has(data::field_names::static_inputs)
                          ? batch.field(data::field_names::static_inputs)
                          : Series::undefined()),
        encoder_inputs(batch.has(data::field_names::encoder_inputs)
                           ? batch.field(data::field_names::encoder_inputs)
                           : Series::undefined()),
        decoder_inputs(batch.has(data::field_names::decoder_inputs)
                           ? batch.field(data::field_names::decoder_inputs)
                           : Series::undefined()),
        encoder_target(batch.has(data::field_names::encoder_target)
                           ? batch.field(data::field_names::encoder_target)
                           : Series::undefined()),
        target(batch.target()) {}

  explicit AutoregressiveBatch(const data::TimeSeriesBatch& batch)
      : static_inputs(batch.static_inputs),
        encoder_inputs(batch.encoder_inputs),
        decoder_inputs(batch.decoder_inputs),
        encoder_target(batch.encoder_target),
        target(batch.target) {}
};

struct AutoregressiveTrainOptions {
  std::int64_t batch_size = 64;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  bool shuffle = true;
  std::uint64_t seed = 0;
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

struct GaussianForecast {
  Series mean = Series::undefined();   // [N, decoder_length]
  Series scale = Series::undefined();  // [N, decoder_length]
};

class AutoregressiveForecaster final {
public:
  explicit AutoregressiveForecaster(AutoregressiveForecasterOptions options);

  void learn(const AutoregressiveBatch& train, const AutoregressiveTrainOptions& options = {});
  void learn(const AutoregressiveBatch& train,
             const AutoregressiveBatch& val,
             const AutoregressiveTrainOptions& options = {});

  void learn(const data::Batch& train, const AutoregressiveTrainOptions& options = {}) {
    learn(AutoregressiveBatch(train), options);
  }
  void learn(const data::Batch& train,
             const data::Batch& val,
             const AutoregressiveTrainOptions& options = {}) {
    learn(AutoregressiveBatch(train), AutoregressiveBatch(val), options);
  }
  void learn(const data::TimeSeriesBatch& train, const AutoregressiveTrainOptions& options = {}) {
    learn(AutoregressiveBatch(train), options);
  }
  void learn(const data::TimeSeriesBatch& train,
             const data::TimeSeriesBatch& val,
             const AutoregressiveTrainOptions& options = {}) {
    learn(AutoregressiveBatch(train), AutoregressiveBatch(val), options);
  }

  [[nodiscard]] GaussianForecast predict(const AutoregressiveBatch& batch) const;
  [[nodiscard]] GaussianForecast predict(const data::Batch& batch) const {
    return predict(AutoregressiveBatch(batch));
  }
  [[nodiscard]] GaussianForecast predict(const data::TimeSeriesBatch& batch) const {
    return predict(AutoregressiveBatch(batch));
  }

  [[nodiscard]] double assess(const AutoregressiveBatch& batch) const;
  [[nodiscard]] double assess(const data::Batch& batch) const { return assess(AutoregressiveBatch(batch)); }
  [[nodiscard]] double assess(const data::TimeSeriesBatch& batch) const {
    return assess(AutoregressiveBatch(batch));
  }

  [[nodiscard]] bool ready() const noexcept;
  AutoregressiveForecaster& to(Platform platform);

private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  AutoregressiveForecasterOptions options_;
};

} // namespace pluss::ai::ice::models
