#include "pluss/ai/ice/models/ts/autoregressive_forecaster.h"

#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/io.h"
#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"
#include "ts_artifact.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>
#include <unordered_map>

namespace pluss::ai::ice::models {

namespace {

using io::detail::parse_kv_body;
using io::detail::require_f64;
using io::detail::require_i64;
using io::detail::require_u64;
using io::detail::write_num_line;
using ts_artifact::read_categoricals;
using ts_artifact::write_categoricals;

constexpr std::string_view kMagic = "pluss_ice_autoregressive_forecaster";
constexpr std::int64_t kSchema = 1;
constexpr std::string_view kLoadContext =
    "pluss::ai::ice::models::AutoregressiveForecaster::load";

constexpr double kHalfLog2Pi = 0.91893853320467274178;

[[nodiscard]] std::int64_t embedding_width_(std::int64_t cardinality,
                                            std::int64_t hidden_size) {
  const auto guess = static_cast<std::int64_t>(std::ceil(std::sqrt(static_cast<double>(cardinality)))) + 1;
  return std::max<std::int64_t>(2, std::min(hidden_size, guess));
}

struct FeatureSlot {
  std::int64_t variable_index = 0;
  bool categorical = false;
  std::int64_t width = 1;
  pluss::nn::Embedding embedding{nullptr};
};

struct FeatureEncoder {
  std::int64_t raw_size = 0;
  std::vector<data::CategoricalVariable> categoricals{};
  std::vector<FeatureSlot> slots{};
  std::int64_t output_width = 0;

  FeatureEncoder() = default;

  FeatureEncoder(std::int64_t raw_size_in,
                 std::vector<data::CategoricalVariable> categoricals_in,
                 std::int64_t hidden_size,
                 const Config& cfg,
                 std::uint64_t seed)
      : raw_size(raw_size_in),
        categoricals(std::move(categoricals_in)) {
    if (raw_size <= 0) return;
    std::unordered_map<std::int64_t, std::int64_t> categorical_cardinality{};
    categorical_cardinality.reserve(categoricals.size());
    for (const auto& cat : categoricals) {
      categorical_cardinality.emplace(cat.variable_index, cat.cardinality);
    }
    for (std::int64_t i = 0; i < raw_size; ++i) {
      FeatureSlot slot{};
      slot.variable_index = i;
      if (const auto it = categorical_cardinality.find(i); it != categorical_cardinality.end()) {
        slot.categorical = true;
        slot.width = embedding_width_(it->second, hidden_size);
        slot.embedding.reset(pluss::nn::EmbeddingOptions{
            .cardinality = it->second,
            .hidden_size = slot.width,
            .cfg = cfg,
            .seed = seed + static_cast<std::uint64_t>(13 * i + 1),
        });
      }
      output_width += slot.width;
      slots.push_back(std::move(slot));
    }
  }

  [[nodiscard]] std::vector<Series*> parameters() {
    std::vector<Series*> out{};
    for (auto& slot : slots) {
      if (!slot.categorical) continue;
      const auto more = slot.embedding.parameters();
      out.insert(out.end(), more.begin(), more.end());
    }
    return out;
  }

  void set_training(bool enabled) {
    for (auto& slot : slots) {
      if (!slot.categorical) continue;
      slot.embedding.train(enabled);
    }
  }

  void move_to(Platform platform) {
    for (auto& slot : slots) {
      if (!slot.categorical) continue;
      slot.embedding.to(platform);
    }
  }

  [[nodiscard]] Series encode(const Series& input) const {
    if (raw_size <= 0) return pluss::series::undefined();
    PLUSS_CHECK(input.defined(),
                "pluss::ai::ice::models::AutoregressiveForecaster: required feature tensor is undefined.");
    PLUSS_CHECK(input.dim() == 2 || input.dim() == 3,
                "pluss::ai::ice::models::AutoregressiveForecaster: feature tensor must be rank 2 or 3.");
    PLUSS_CHECK(input.sizes().back() == raw_size,
                "pluss::ai::ice::models::AutoregressiveForecaster: feature width mismatch.");
    std::vector<Series> pieces{};
    pieces.reserve(slots.size());
    const std::int64_t feature_dim = input.dim() - 1;
    for (const auto& slot : slots) {
      Series piece = input.narrow_copy(feature_dim, slot.variable_index, 1);
      if (slot.categorical) {
        piece = slot.embedding->forward(piece.squeeze(feature_dim).to(ScalarType::Int64));
      }
      pieces.push_back(std::move(piece));
    }
    return pieces.size() == 1 ? pieces.front() : pluss::series::unite(pieces, -1);
  }
};

[[nodiscard]] Series last_encoder_target_(const AutoregressiveBatch& batch) {
  const auto sizes = batch.encoder_target.sizes();
  return batch.encoder_target.narrow_copy(1, sizes[1] - 1, 1).reshape({sizes[0]});
}

[[nodiscard]] Series teacher_forced_decoder_target_(const AutoregressiveBatch& batch) {
  const auto n = batch.target.sizes()[0];
  const auto horizon = batch.target.sizes()[1];
  if (horizon == 1) return last_encoder_target_(batch).reshape({n, 1});
  return pluss::series::unite(
      {
          last_encoder_target_(batch).reshape({n, 1}),
          batch.target.narrow_copy(1, 0, horizon - 1),
      },
      1);
}

[[nodiscard]] Series make_sequence_input_(const Series& encoded_features,
                                          const Series& previous_target) {
  const Series prev = previous_target.unsqueeze(-1);
  if (!encoded_features.defined()) return prev;
  return pluss::series::unite({encoded_features, prev}, -1);
}

[[nodiscard]] Series gaussian_nll_(const Series& mean,
                                   const Series& scale,
                                   const Series& target) {
  const Series safe_scale = scale.clamp_min(1e-6);
  const Series z = target.sub(mean).div(safe_scale);
  return safe_scale.log().add(z.square().mul(0.5)).add(kHalfLog2Pi).mean();
}

[[nodiscard]] AutoregressiveBatch index_select_batch_(const AutoregressiveBatch& batch,
                                                      const Series& index) {
  AutoregressiveBatch out{};
  out.static_inputs = batch.static_inputs.defined() ? batch.static_inputs.index_select(0, index) : batch.static_inputs;
  out.encoder_inputs = batch.encoder_inputs.defined() ? batch.encoder_inputs.index_select(0, index) : batch.encoder_inputs;
  out.decoder_inputs = batch.decoder_inputs.defined() ? batch.decoder_inputs.index_select(0, index) : batch.decoder_inputs;
  out.encoder_target = batch.encoder_target.defined() ? batch.encoder_target.index_select(0, index) : batch.encoder_target;
  out.target = batch.target.defined() ? batch.target.index_select(0, index) : batch.target;
  return out;
}

[[nodiscard]] Series move_field_(const Series& field, Platform platform) {
  return field.defined() ? field.to(platform) : field;
}

[[nodiscard]] AutoregressiveBatch move_batch_(const AutoregressiveBatch& batch,
                                              Platform platform) {
  AutoregressiveBatch out{};
  out.static_inputs = move_field_(batch.static_inputs, platform);
  out.encoder_inputs = move_field_(batch.encoder_inputs, platform);
  out.decoder_inputs = move_field_(batch.decoder_inputs, platform);
  out.encoder_target = move_field_(batch.encoder_target, platform);
  out.target = move_field_(batch.target, platform);
  return out;
}

[[nodiscard]] TrainOptions make_optimizer_options_(const AutoregressiveTrainOptions& options) {
  TrainOptions out{};
  out.optimizer = options.optimizer;
  out.learning_rate = options.learning_rate;
  out.objective_regularization = options.objective_regularization;
  out.loop_regularization = options.loop_regularization;
  out.validation = options.validation;
  out.execution = options.execution;
  return out;
}

[[nodiscard]] Series project_state_(const pluss::nn::Fusion& projector,
                                    const Series& static_encoded,
                                    std::int64_t rnn_layers,
                                    std::int64_t hidden_size) {
  return projector->forward(static_encoded)
      .reshape({static_encoded.sizes()[0], rnn_layers, hidden_size})
      .transpose(0, 1);
}

[[nodiscard]] std::tuple<Series, std::tuple<Series, Series>> run_lstm_(
    const pluss::nn::LSTM& lstm,
    const Series& input,
    const std::tuple<Series, Series>& state) {
  const auto& [h0, c0] = state;
  if (!h0.defined() && !c0.defined()) {
    return lstm->forward(input);
  }
  return lstm->forward(input, state);
}

void validate_batch_(const AutoregressiveBatch& batch,
                     const AutoregressiveForecasterOptions& options,
                     bool require_target,
                     const char* what) {
  PLUSS_CHECK(batch.encoder_target.defined(),
              std::string(what) + ": encoder_target is required.");
  if (require_target) {
    PLUSS_CHECK(batch.target.defined(),
                std::string(what) + ": target is required.");
  }
  PLUSS_CHECK(batch.encoder_target.dim() == 2,
              std::string(what) + ": encoder_target must be rank 2 [N, encoder_length].");
  PLUSS_CHECK(batch.encoder_target.sizes()[1] == options.encoder_length,
              std::string(what) + ": encoder_target length mismatch.");
  if (require_target) {
    PLUSS_CHECK(batch.target.dim() == 2,
                std::string(what) + ": target must be rank 2 [N, decoder_length].");
    PLUSS_CHECK(batch.target.sizes()[1] == options.decoder_length,
                std::string(what) + ": target horizon mismatch.");
  }
  if (options.static_size > 0) {
    PLUSS_CHECK(batch.static_inputs.defined(),
                std::string(what) + ": static_inputs is required when static_size > 0.");
    PLUSS_CHECK(batch.static_inputs.dim() == 2,
                std::string(what) + ": static_inputs must be rank 2 [N, static_size].");
    PLUSS_CHECK(batch.static_inputs.sizes()[1] == options.static_size,
                std::string(what) + ": static_inputs width mismatch.");
  }
  if (options.encoder_size > 0) {
    PLUSS_CHECK(batch.encoder_inputs.defined(),
                std::string(what) + ": encoder_inputs is required when encoder_size > 0.");
    PLUSS_CHECK(batch.encoder_inputs.dim() == 3,
                std::string(what) + ": encoder_inputs must be rank 3 [N, encoder_length, encoder_size].");
    PLUSS_CHECK(batch.encoder_inputs.sizes()[1] == options.encoder_length &&
                    batch.encoder_inputs.sizes()[2] == options.encoder_size,
                std::string(what) + ": encoder_inputs shape mismatch.");
  }
  if (options.decoder_size > 0) {
    PLUSS_CHECK(batch.decoder_inputs.defined(),
                std::string(what) + ": decoder_inputs is required when decoder_size > 0.");
    PLUSS_CHECK(batch.decoder_inputs.dim() == 3,
                std::string(what) + ": decoder_inputs must be rank 3 [N, decoder_length, decoder_size].");
    PLUSS_CHECK(batch.decoder_inputs.sizes()[1] == options.decoder_length &&
                    batch.decoder_inputs.sizes()[2] == options.decoder_size,
                std::string(what) + ": decoder_inputs shape mismatch.");
  }
}

} // namespace

struct AutoregressiveForecaster::Impl {
  explicit Impl(const AutoregressiveForecasterOptions& options)
      : static_encoder(options.static_size, options.static_categoricals, options.hidden_size, options.cfg,
                       options.seed + 1),
        encoder_encoder(options.encoder_size, options.encoder_categoricals, options.hidden_size, options.cfg,
                        options.seed + 101),
        decoder_encoder(options.decoder_size, options.decoder_categoricals, options.hidden_size, options.cfg,
                        options.seed + 201) {
    const std::int64_t encoder_input_width = encoder_encoder.output_width + 1;
    const std::int64_t decoder_input_width = decoder_encoder.output_width + 1;
    if (options.static_size > 0) {
      initial_hidden.reset(pluss::nn::FusionOptions{
          .in_features = static_encoder.output_width,
          .out_features = options.rnn_layers * options.hidden_size,
          .bias = true,
          .cfg = options.cfg,
          .seed = options.seed + 301,
      });
      initial_cell.reset(pluss::nn::FusionOptions{
          .in_features = static_encoder.output_width,
          .out_features = options.rnn_layers * options.hidden_size,
          .bias = true,
          .cfg = options.cfg,
          .seed = options.seed + 302,
      });
    }
    rnn.reset(pluss::nn::LSTMOptions{
        .input_size = encoder_input_width,
        .hidden_size = options.hidden_size,
        .num_layers = options.rnn_layers,
        .bias = true,
        .batch_first = true,
        .dropout = options.dropout,
        .bidirectional = false,
        .split_bias = false,
        .proj_size = 0,
        .cfg = options.cfg,
        .seed = options.seed + 401,
    });
    decoder_adapter.reset(pluss::nn::FusionOptions{
        .in_features = decoder_input_width,
        .out_features = encoder_input_width,
        .bias = true,
        .cfg = options.cfg,
        .seed = options.seed + 501,
    });
    distribution_head.reset(pluss::nn::FusionOptions{
        .in_features = options.hidden_size,
        .out_features = 2,
        .bias = true,
        .cfg = options.cfg,
        .seed = options.seed + 601,
    });
  }

  [[nodiscard]] std::vector<Series*> all_parameters() {
    std::vector<Series*> out{};
    const auto append = [&](std::vector<Series*> more) {
      out.insert(out.end(), more.begin(), more.end());
    };
    append(static_encoder.parameters());
    append(encoder_encoder.parameters());
    append(decoder_encoder.parameters());
    if (initial_hidden) append(initial_hidden.parameters());
    if (initial_cell) append(initial_cell.parameters());
    append(rnn.parameters());
    append(decoder_adapter.parameters());
    append(distribution_head.parameters());
    return out;
  }

  void set_training(bool enabled) {
    static_encoder.set_training(enabled);
    encoder_encoder.set_training(enabled);
    decoder_encoder.set_training(enabled);
    if (initial_hidden) initial_hidden.train(enabled);
    if (initial_cell) initial_cell.train(enabled);
    rnn.train(enabled);
    decoder_adapter.train(enabled);
    distribution_head.train(enabled);
  }

  void move_to(Platform platform) {
    static_encoder.move_to(platform);
    encoder_encoder.move_to(platform);
    decoder_encoder.move_to(platform);
    if (initial_hidden) initial_hidden.to(platform);
    if (initial_cell) initial_cell.to(platform);
    rnn.to(platform);
    decoder_adapter.to(platform);
    distribution_head.to(platform);
  }

  [[nodiscard]] std::tuple<Series, std::tuple<Series, Series>> encode(
      const AutoregressiveForecasterOptions& options,
      const AutoregressiveBatch& batch) const {
    Series h0 = pluss::series::undefined();
    Series c0 = pluss::series::undefined();
    if (options.static_size > 0) {
      const Series static_encoded = static_encoder.encode(batch.static_inputs);
      h0 = project_state_(initial_hidden, static_encoded, options.rnn_layers, options.hidden_size);
      c0 = project_state_(initial_cell, static_encoded, options.rnn_layers, options.hidden_size);
    }

    if (options.encoder_length == 1) {
      return {
          pluss::series::undefined(),
          std::tuple<Series, Series>{h0, c0},
      };
    }

    Series encoder_features = encoder_encoder.encode(batch.encoder_inputs);
    if (encoder_features.defined()) {
      encoder_features = encoder_features.narrow_copy(1, 1, options.encoder_length - 1);
    }
    const Series previous_target = batch.encoder_target.narrow_copy(1, 0, options.encoder_length - 1);
    const Series encoder_input = make_sequence_input_(encoder_features, previous_target);
    return run_lstm_(rnn, encoder_input, std::tuple<Series, Series>{h0, c0});
  }

  [[nodiscard]] GaussianForecast decode_teacher_forced(
      const AutoregressiveForecasterOptions& options,
      const AutoregressiveBatch& batch,
      const std::tuple<Series, Series>& hidden_state) const {
    Series decoder_features = decoder_encoder.encode(batch.decoder_inputs);
    const Series decoder_prev_target = teacher_forced_decoder_target_(batch);
    Series decoder_input = make_sequence_input_(decoder_features, decoder_prev_target);
    decoder_input = decoder_adapter->forward(decoder_input);
    const auto [decoder_output, next_state] = run_lstm_(rnn, decoder_input, hidden_state);
    (void)next_state;
    const Series params = distribution_head->forward(decoder_output);
    const auto sizes = params.sizes();
    const Series mean = params.narrow_copy(2, 0, 1).reshape({sizes[0], sizes[1]});
    const Series scale = params.narrow_copy(2, 1, 1).reshape({sizes[0], sizes[1]}).softplus().add(1e-4);
    return GaussianForecast{.mean = mean, .scale = scale};
  }

  [[nodiscard]] GaussianForecast decode_autoregressive(
      const AutoregressiveForecasterOptions& options,
      const AutoregressiveBatch& batch,
      std::tuple<Series, Series> hidden_state) const {
    const std::int64_t n = batch.encoder_target.sizes()[0];
    std::vector<Series> means{};
    std::vector<Series> scales{};
    means.reserve(static_cast<std::size_t>(options.decoder_length));
    scales.reserve(static_cast<std::size_t>(options.decoder_length));

    Series previous_target = last_encoder_target_(batch);
    Series decoder_features = decoder_encoder.encode(batch.decoder_inputs);
    for (std::int64_t step = 0; step < options.decoder_length; ++step) {
      Series step_features = pluss::series::undefined();
      if (decoder_features.defined()) {
        step_features = decoder_features.narrow_copy(1, step, 1);
      }
      Series step_input =
          make_sequence_input_(step_features, previous_target.reshape({n, 1}));
      step_input = decoder_adapter->forward(step_input);
      auto [step_output, next_state] = run_lstm_(rnn, step_input, hidden_state);
      hidden_state = std::move(next_state);
      const Series params = distribution_head->forward(step_output); // [N,1,2]
      Series mean = params.narrow_copy(2, 0, 1).reshape({n, 1});
      Series scale = params.narrow_copy(2, 1, 1).reshape({n, 1}).softplus().add(1e-4);
      means.push_back(mean);
      scales.push_back(scale);
      previous_target = mean.reshape({n});
    }

    return GaussianForecast{
        .mean = means.size() == 1 ? means.front() : pluss::series::unite(means, 1),
        .scale = scales.size() == 1 ? scales.front() : pluss::series::unite(scales, 1),
    };
  }

  FeatureEncoder static_encoder{};
  FeatureEncoder encoder_encoder{};
  FeatureEncoder decoder_encoder{};
  pluss::nn::Fusion initial_hidden{nullptr};
  pluss::nn::Fusion initial_cell{nullptr};
  pluss::nn::LSTM rnn{nullptr};
  pluss::nn::Fusion decoder_adapter{nullptr};
  pluss::nn::Fusion distribution_head{nullptr};
  bool trained = false;
};

AutoregressiveForecaster::AutoregressiveForecaster(AutoregressiveForecasterOptions options)
    : impl_(std::make_shared<Impl>(options)),
      options_(std::move(options)) {
  PLUSS_CHECK(options_.encoder_length > 0 && options_.decoder_length > 0 &&
                  options_.hidden_size > 0 && options_.rnn_layers > 0,
              "pluss::ai::ice::models::AutoregressiveForecaster: encoder_length, decoder_length, "
              "hidden_size and rnn_layers must be positive.");
  PLUSS_CHECK(options_.dropout >= 0.0 && options_.dropout <= 1.0,
              "pluss::ai::ice::models::AutoregressiveForecaster: dropout must be in [0, 1].");
}

bool AutoregressiveForecaster::ready() const noexcept { return impl_->trained; }

AutoregressiveForecaster& AutoregressiveForecaster::to(Platform platform) {
  PLUSS_CALL_GUARD();
  impl_->move_to(platform);
  return *this;
}

void AutoregressiveForecaster::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::AutoregressiveForecaster::save: model has not been trained.");
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::AutoregressiveForecaster::save: output path cannot be empty.");
  const auto weights_path = std::string(filepath) + ".weights";
  io::save_parameters(weights_path, impl_->all_parameters());

  std::ostringstream out{};
  out << kMagic << '\n';
  write_num_line(out, "schema", kSchema);
  write_num_line(out, "static_size", options_.static_size);
  write_num_line(out, "encoder_size", options_.encoder_size);
  write_num_line(out, "decoder_size", options_.decoder_size);
  write_num_line(out, "encoder_length", options_.encoder_length);
  write_num_line(out, "decoder_length", options_.decoder_length);
  write_num_line(out, "hidden_size", options_.hidden_size);
  write_num_line(out, "rnn_layers", options_.rnn_layers);
  write_num_line(out, "dropout", options_.dropout);
  write_categoricals(out, "static_categoricals", options_.static_categoricals);
  write_categoricals(out, "encoder_categoricals", options_.encoder_categoricals);
  write_categoricals(out, "decoder_categoricals", options_.decoder_categoricals);
  write_num_line(out, "cfg_dtype", static_cast<int>(options_.cfg.dtype));
  write_num_line(out, "cfg_platform", static_cast<int>(options_.cfg.platform));
  write_num_line(out, "seed", options_.seed);

  std::ofstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::AutoregressiveForecaster::save: failed to open '" +
                  std::string(filepath) + "'.");
  file << out.str();
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::AutoregressiveForecaster::save: failed while writing '" +
                  std::string(filepath) + "'.");
}

AutoregressiveForecaster AutoregressiveForecaster::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::AutoregressiveForecaster::load: input path cannot be empty.");
  std::ifstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::AutoregressiveForecaster::load: failed to open '" +
                  std::string(filepath) + "'.");
  std::string magic{};
  PLUSS_CHECK(std::getline(file, magic) && magic == kMagic,
              "pluss::ai::ice::models::AutoregressiveForecaster::load: invalid model artifact header.");
  std::ostringstream body{};
  body << file.rdbuf();
  const auto kv = parse_kv_body(body.str(), kLoadContext);
  PLUSS_CHECK(require_i64(kv, "schema", kLoadContext) == kSchema,
              "pluss::ai::ice::models::AutoregressiveForecaster::load: unsupported schema.");

  AutoregressiveForecasterOptions options{};
  options.static_size = require_i64(kv, "static_size", kLoadContext);
  options.encoder_size = require_i64(kv, "encoder_size", kLoadContext);
  options.decoder_size = require_i64(kv, "decoder_size", kLoadContext);
  options.encoder_length = require_i64(kv, "encoder_length", kLoadContext);
  options.decoder_length = require_i64(kv, "decoder_length", kLoadContext);
  options.hidden_size = require_i64(kv, "hidden_size", kLoadContext);
  options.rnn_layers = require_i64(kv, "rnn_layers", kLoadContext);
  options.dropout = require_f64(kv, "dropout", kLoadContext);
  options.static_categoricals = read_categoricals(kv, "static_categoricals", kLoadContext);
  options.encoder_categoricals = read_categoricals(kv, "encoder_categoricals", kLoadContext);
  options.decoder_categoricals = read_categoricals(kv, "decoder_categoricals", kLoadContext);
  options.cfg = Config{}
                    .with_dtype(static_cast<ScalarType>(require_i64(kv, "cfg_dtype", kLoadContext)))
                    .with_platform(static_cast<Platform>(require_i64(kv, "cfg_platform", kLoadContext)));
  options.seed = require_u64(kv, "seed", kLoadContext);

  AutoregressiveForecaster model(options);
  const auto weights_path = std::string(filepath) + ".weights";
  io::load_parameters(weights_path, model.impl_->all_parameters());
  for (auto* parameter : model.impl_->all_parameters()) {
    parameter->tracks_slope_(false);
    parameter->clear();
  }
  model.impl_->trained = true;
  model.impl_->set_training(false);
  return model;
}

void AutoregressiveForecaster::learn(const AutoregressiveBatch& train,
                                     const AutoregressiveTrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_batch_(train, options_, /*require_target=*/true,
                  "pluss::ai::ice::models::AutoregressiveForecaster::learn");
  if (options.validation.split_ratio <= 0.0) {
    learn(train, AutoregressiveBatch{}, options);
    return;
  }

  const auto n = train.encoder_target.sizes()[0];
  std::mt19937_64 rng(options.seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  if (options.shuffle) std::ranges::shuffle(order, rng);

  const auto val_n =
      static_cast<std::int64_t>(static_cast<double>(n) * options.validation.split_ratio);
  const auto train_n = n - val_n;
  PLUSS_CHECK(
      train_n > 0 && val_n > 0,
      "pluss::ai::ice::models::AutoregressiveForecaster::learn: validation.split_ratio leaves an empty split.");

  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(train.encoder_target.platform());
  const Series train_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin(), order.begin() + train_n), {train_n}, idx_cfg);
  const Series val_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin() + train_n, order.end()), {val_n}, idx_cfg);

  learn(index_select_batch_(train, train_index), index_select_batch_(train, val_index), options);
}

void AutoregressiveForecaster::learn(const AutoregressiveBatch& train,
                                     const AutoregressiveBatch& val,
                                     const AutoregressiveTrainOptions& options) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(options.execution.max_epochs > 0 && options.batch_size > 0,
              "pluss::ai::ice::models::AutoregressiveForecaster::learn: max_epochs and batch_size must be positive.");
  PLUSS_CHECK(options.validation.every_n_epochs > 0,
              "pluss::ai::ice::models::AutoregressiveForecaster::learn: validation.every_n_epochs must be positive.");
  validate_batch_(train, options_, /*require_target=*/true,
                  "pluss::ai::ice::models::AutoregressiveForecaster::learn");
  if (val.encoder_target.defined()) {
    validate_batch_(val, options_, /*require_target=*/true,
                    "pluss::ai::ice::models::AutoregressiveForecaster::learn");
  }

  const Platform target_platform = resolve_platform(options.execution.platform);
  impl_->move_to(target_platform);

  const AutoregressiveBatch train_on_device = move_batch_(train, target_platform);
  const bool has_val = val.encoder_target.defined() && val.target.defined();
  const AutoregressiveBatch val_on_device = has_val ? move_batch_(val, target_platform) : AutoregressiveBatch{};

  const auto parameters = impl_->all_parameters();
  PLUSS_CHECK(!parameters.empty(),
              "pluss::ai::ice::models::AutoregressiveForecaster::learn: model has no parameters.");
  auto optimizer = ice::make_optimizer(parameters, make_optimizer_options_(options));

  const auto n = train_on_device.encoder_target.sizes()[0];
  std::mt19937_64 rng(options.seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(target_platform);
  CallbackChain callbacks{};
  if (has_val &&
      (options.loop_regularization.early_stopping ||
       options.loop_regularization.restore_best_weights)) {
    callbacks.add(std::make_shared<EarlyStopCallback>(options.validation, options.loop_regularization));
  }
  RunState state{
      .model = nullptr,
      .stage = StepStage::Learn,
      .target_platform = target_platform,
      .epoch = -1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 0,
      .metrics = {},
      .parameters = &parameters,
      .stop_requested = false,
      .stop_reason = {},
      .exception_message = {},
  };

  impl_->set_training(true);
  try {
    callbacks.on_run_begin(state);
    for (std::int64_t epoch = 0; epoch < options.execution.max_epochs; ++epoch) {
      state.stage = StepStage::Learn;
      state.epoch = epoch;
      state.batch_index = -1;
      state.rows = 0;
      state.metrics.clear();
      callbacks.on_epoch_begin(state);
      if (state.stop_requested) break;
      if (options.shuffle) std::ranges::shuffle(order, rng);
      double train_weighted_loss_sum = 0.0;
      std::int64_t train_total_rows = 0;
      std::int64_t batch_counter = 0;
      for (std::int64_t start = 0; start < n; start += options.batch_size) {
        const auto end = std::min(n, start + options.batch_size);
        state.batch_index = batch_counter;
        state.rows = end - start;
        state.metrics.clear();
        callbacks.on_batch_begin(state);
        if (state.stop_requested) break;

        const Series index =
            pluss::make_series(std::vector<std::int64_t>(order.begin() + start, order.begin() + end),
                               {end - start}, idx_cfg);
        const AutoregressiveBatch mini = index_select_batch_(train_on_device, index);
        const auto [encoder_out, hidden_state] = impl_->encode(options_, mini);
        (void)encoder_out;
        GaussianForecast forecast = impl_->decode_teacher_forced(options_, mini, hidden_state);
        Series loss = gaussian_nll_(forecast.mean, forecast.scale, mini.target);

        if (options.objective_regularization.l1_penalty != 0.0) {
          Series l1 = Series::full({}, 0.0, loss.scalar_type()).to(loss.platform());
          for (const Series* parameter : parameters) {
            l1 = l1.add(parameter->abs().sum());
          }
          loss = loss.add(l1.mul(options.objective_regularization.l1_penalty));
        }
        if (options.objective_regularization.l2_penalty != 0.0) {
          Series l2 = Series::full({}, 0.0, loss.scalar_type()).to(loss.platform());
          for (const Series* parameter : parameters) {
            l2 = l2.add(parameter->mul(*parameter).sum());
          }
          loss = loss.add(l2.mul(options.objective_regularization.l2_penalty));
        }

        const double loss_value = loss.item<double>();
        train_weighted_loss_sum += loss_value * static_cast<double>(end - start);
        train_total_rows += end - start;
        state.metrics.clear();
        state.put_metric("loss", loss_value);
        optimizer->clear();
        callbacks.on_before_backward(state);
        if (state.stop_requested) break;
        loss.reverse();
        callbacks.on_after_backward(state);
        if (state.stop_requested) break;
        callbacks.on_before_step(state);
        if (state.stop_requested) break;
        optimizer->step();
        callbacks.on_batch_end(state);
        ++state.global_step;
        if (state.stop_requested) break;
        ++batch_counter;
      }

      const double train_loss_value =
          train_total_rows > 0 ? train_weighted_loss_sum / static_cast<double>(train_total_rows) : 0.0;
      if (train_total_rows > 0) {
        state.put_metric("loss", train_loss_value);
      }

      const bool should_validate =
          has_val && epoch >= options.validation.start_from_epoch &&
          ((epoch - options.validation.start_from_epoch) % options.validation.every_n_epochs == 0);
      if (should_validate) {
        impl_->set_training(false);
        const NoGradGuard guard;
        const auto [val_encoder_out, val_hidden_state] = impl_->encode(options_, val_on_device);
        (void)val_encoder_out;
        const GaussianForecast val_forecast =
            impl_->decode_teacher_forced(options_, val_on_device, val_hidden_state);
        const double val_loss_value =
            gaussian_nll_(val_forecast.mean, val_forecast.scale, val_on_device.target).item<double>();
        impl_->set_training(true);
        state.stage = StepStage::Learn;
        state.metrics.clear();
        state.put_metric("loss", train_loss_value);
        state.put_metric("val_loss", val_loss_value);
      }
      callbacks.on_epoch_end(state);
      if (state.stop_requested) break;
    }
    callbacks.on_run_end(state);
  } catch (const std::exception& e) {
    impl_->set_training(false);
    state.exception_message = e.what();
    callbacks.on_exception(state);
    PLUSS_FAIL(
        std::string("pluss::ai::ice::models::AutoregressiveForecaster::learn: training became numerically unstable: ") +
        e.what());
  }

  impl_->set_training(false);
  impl_->trained = true;
}

GaussianForecast AutoregressiveForecaster::predict(const AutoregressiveBatch& batch) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_->trained,
              "pluss::ai::ice::models::AutoregressiveForecaster::predict: model has not been trained.");
  validate_batch_(batch, options_, /*require_target=*/false,
                  "pluss::ai::ice::models::AutoregressiveForecaster::predict");
  const auto parameters = impl_->all_parameters();
  const NoGradGuard guard;
  const Platform model_platform = parameters.front()->platform();
  const AutoregressiveBatch batch_on_device = move_batch_(batch, model_platform);
  const auto [encoder_out, hidden_state] = impl_->encode(options_, batch_on_device);
  (void)encoder_out;
  return impl_->decode_autoregressive(options_, batch_on_device, hidden_state);
}

double AutoregressiveForecaster::assess(const AutoregressiveBatch& batch) const {
  PLUSS_CALL_GUARD();
  validate_batch_(batch, options_, /*require_target=*/true,
                  "pluss::ai::ice::models::AutoregressiveForecaster::assess");
  const GaussianForecast forecast = predict(batch);
  const Series target_on_device = batch.target.to(forecast.mean.platform());
  return gaussian_nll_(forecast.mean, forecast.scale, target_on_device).item<double>();
}

} // namespace pluss::ai::ice::models
