#include "pluss/ai/ice/models/ts/temporal_fusion_transformer.h"

#include "tft_blocks.h"
#include "ts_artifact.h"

#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/io.h"
#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>

namespace pluss::ai::ice::models {

namespace {

using io::detail::parse_kv_body;
using io::detail::require_f64;
using io::detail::require_i64;
using io::detail::require_u64;
using io::detail::write_num_line;
using ts_artifact::read_categoricals;
using ts_artifact::write_categoricals;

constexpr std::string_view kMagic = "pluss_ice_temporal_fusion_transformer";
constexpr std::int64_t kSchema = 1;
constexpr std::string_view kLoadContext =
    "pluss::ai::ice::models::TemporalFusionTransformer::load";

} // namespace

struct TemporalFusionTransformer::Impl {
  explicit Impl(const TFTOptions& options) {
    const bool has_static = options.static_size > 0;
    const auto hidden = options.hidden_size;
    const auto num_quantiles = static_cast<std::int64_t>(options.quantiles.size());

    if (has_static) {
      static_vsn.reset(detail::VariableSelectionOptions{
          .num_vars = options.static_size, .hidden_size = hidden, .context_size = 0,
          .categoricals = options.static_categoricals,
          .cfg = options.cfg, .seed = options.seed + 1,
      });
      c_selection_grn.reset(detail::GatedResidualNetworkOptions{
          .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = 0,
          .cfg = options.cfg, .seed = options.seed + 2,
      });
      c_enrichment_grn.reset(detail::GatedResidualNetworkOptions{
          .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = 0,
          .cfg = options.cfg, .seed = options.seed + 3,
      });
      c_hidden_grn.reset(detail::GatedResidualNetworkOptions{
          .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = 0,
          .cfg = options.cfg, .seed = options.seed + 4,
      });
      c_cell_grn.reset(detail::GatedResidualNetworkOptions{
          .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = 0,
          .cfg = options.cfg, .seed = options.seed + 5,
      });
    }

    const std::int64_t selection_context = has_static ? hidden : 0;
    encoder_vsn.reset(detail::VariableSelectionOptions{
        .num_vars = options.encoder_size, .hidden_size = hidden, .context_size = selection_context,
        .categoricals = options.encoder_categoricals,
        .cfg = options.cfg, .seed = options.seed + 10,
    });
    decoder_vsn.reset(detail::VariableSelectionOptions{
        .num_vars = options.decoder_size, .hidden_size = hidden, .context_size = selection_context,
        .categoricals = options.decoder_categoricals,
        .cfg = options.cfg, .seed = options.seed + 20,
    });

    lstm.reset(pluss::nn::LSTMOptions{
        .input_size = hidden, .hidden_size = hidden, .num_layers = options.lstm_layers, .bias = true,
        .batch_first = true, .dropout = 0.0, .bidirectional = false, .split_bias = false, .proj_size = 0,
        .cfg = options.cfg, .seed = options.seed + 30,
    });
    post_lstm_gate.reset(detail::GatedAddNormOptions{.size = hidden, .cfg = options.cfg, .seed = options.seed + 40});
    static_enrichment_grn.reset(detail::GatedResidualNetworkOptions{
        .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = selection_context,
        .cfg = options.cfg, .seed = options.seed + 50,
    });
    attention.reset(pluss::nn::MultiheadAttentionOptions{
        .embed_dim = hidden, .num_heads = options.attention_heads, .dropout = options.dropout, .bias = true,
        .batch_first = true, .average_weights = true, .scale = 0.0, .cfg = options.cfg,
        .seed = options.seed + 60, .training = true,
    });
    post_attn_gate.reset(detail::GatedAddNormOptions{.size = hidden, .cfg = options.cfg, .seed = options.seed + 70});
    position_wise_grn.reset(detail::GatedResidualNetworkOptions{
        .input_size = hidden, .hidden_size = hidden, .output_size = hidden, .context_size = 0,
        .cfg = options.cfg, .seed = options.seed + 80,
    });
    final_gate.reset(detail::GatedAddNormOptions{.size = hidden, .cfg = options.cfg, .seed = options.seed + 90});
    output_head.reset(pluss::nn::FusionOptions{
        .in_features = hidden, .out_features = num_quantiles, .bias = true, .cfg = options.cfg,
        .seed = options.seed + 100,
    });
  }

  [[nodiscard]] std::vector<Series*> all_parameters() {
    std::vector<Series*> out{};
    const auto append = [&](std::vector<Series*> more) {
      out.insert(out.end(), more.begin(), more.end());
    };
    if (static_vsn) {
      append(static_vsn.parameters());
      append(c_selection_grn.parameters());
      append(c_enrichment_grn.parameters());
      append(c_hidden_grn.parameters());
      append(c_cell_grn.parameters());
    }
    append(encoder_vsn.parameters());
    append(decoder_vsn.parameters());
    append(lstm.parameters());
    append(post_lstm_gate.parameters());
    append(static_enrichment_grn.parameters());
    append(attention.parameters());
    append(post_attn_gate.parameters());
    append(position_wise_grn.parameters());
    append(final_gate.parameters());
    append(output_head.parameters());
    return out;
  }

  void set_training(bool enabled) {
    if (static_vsn) {
      static_vsn.train(enabled);
      c_selection_grn.train(enabled);
      c_enrichment_grn.train(enabled);
      c_hidden_grn.train(enabled);
      c_cell_grn.train(enabled);
    }
    encoder_vsn.train(enabled);
    decoder_vsn.train(enabled);
    lstm.train(enabled);
    post_lstm_gate.train(enabled);
    static_enrichment_grn.train(enabled);
    attention.train(enabled);
    post_attn_gate.train(enabled);
    position_wise_grn.train(enabled);
    final_gate.train(enabled);
    output_head.train(enabled);
  }

  void move_to(Platform platform) {
    if (static_vsn) {
      static_vsn.to(platform);
      c_selection_grn.to(platform);
      c_enrichment_grn.to(platform);
      c_hidden_grn.to(platform);
      c_cell_grn.to(platform);
    }
    encoder_vsn.to(platform);
    decoder_vsn.to(platform);
    lstm.to(platform);
    post_lstm_gate.to(platform);
    static_enrichment_grn.to(platform);
    attention.to(platform);
    post_attn_gate.to(platform);
    position_wise_grn.to(platform);
    final_gate.to(platform);
    output_head.to(platform);
  }

  detail::VariableSelectionNetwork static_vsn{nullptr};
  detail::GatedResidualNetwork c_selection_grn{nullptr};
  detail::GatedResidualNetwork c_enrichment_grn{nullptr};
  detail::GatedResidualNetwork c_hidden_grn{nullptr};
  detail::GatedResidualNetwork c_cell_grn{nullptr};

  detail::VariableSelectionNetwork encoder_vsn{nullptr};
  detail::VariableSelectionNetwork decoder_vsn{nullptr};

  pluss::nn::LSTM lstm{nullptr};
  detail::GatedAddNorm post_lstm_gate{nullptr};
  detail::GatedResidualNetwork static_enrichment_grn{nullptr};
  pluss::nn::MultiheadAttention attention{nullptr};
  detail::GatedAddNorm post_attn_gate{nullptr};
  detail::GatedResidualNetwork position_wise_grn{nullptr};
  detail::GatedAddNorm final_gate{nullptr};
  pluss::nn::Fusion output_head{nullptr};

  bool trained = false;

  // The full TFT forward pass, composing every block above. A member
  // (rather than a free function) purely because Impl is private to
  // TemporalFusionTransformer -- nothing outside this translation unit ever
  // calls it.
  [[nodiscard]] TFTForecast forward(const TFTOptions& options, const TFTBatch& batch) {
    // [N, hidden] -> [N, 1, hidden]; broadcasts against [N, time, hidden]
    // inside a GatedResidualNetwork's context injection. Passes an
    // undefined context straight through (no static covariates configured).
    const auto expand_context = [](const Series& context) {
      return context.defined() ? context.unsqueeze(1) : context;
    };

    const bool has_static = options.static_size > 0;

    Series c_selection = pluss::series::undefined();
    Series c_enrichment = pluss::series::undefined();
    Series c_hidden = pluss::series::undefined();
    Series c_cell = pluss::series::undefined();
    if (has_static) {
      const auto [static_embedding, static_weights] = static_vsn->forward(batch.static_inputs);
      (void)static_weights;
      c_selection = c_selection_grn->forward(static_embedding);
      c_enrichment = c_enrichment_grn->forward(static_embedding);
      c_hidden = c_hidden_grn->forward(static_embedding);
      c_cell = c_cell_grn->forward(static_embedding);
    }

    const auto [encoder_selected, encoder_weights] =
        encoder_vsn->forward(batch.encoder_inputs, expand_context(c_selection));
    const auto [decoder_selected, decoder_weights] =
        decoder_vsn->forward(batch.decoder_inputs, expand_context(c_selection));
    (void)encoder_weights;
    (void)decoder_weights;

    std::tuple<Series, Series> initial_state{
        pluss::series::undefined(), pluss::series::undefined()};
    if (has_static) {
      const auto batch_size = batch.encoder_inputs.sizes()[0];
      const Series h0 = c_hidden.unsqueeze(0).expand({options.lstm_layers, batch_size, options.hidden_size});
      const Series c0 = c_cell.unsqueeze(0).expand({options.lstm_layers, batch_size, options.hidden_size});
      initial_state = std::tuple<Series, Series>{h0, c0};
    }

    auto [encoder_lstm_out, encoder_state] = lstm->forward(encoder_selected, initial_state);
    auto [decoder_lstm_out, decoder_state] = lstm->forward(decoder_selected, encoder_state);
    (void)decoder_state;

    const Series lstm_out = pluss::series::unite({encoder_lstm_out, decoder_lstm_out}, 1);
    const Series vsn_out = pluss::series::unite({encoder_selected, decoder_selected}, 1);
    const Series gated_lstm = post_lstm_gate->forward(lstm_out, vsn_out);

    const Series enriched = static_enrichment_grn->forward(gated_lstm, expand_context(c_enrichment));

    // Self-attention over the full (encoder+decoder) sequence with a causal
    // mask, then keep only the decoder queries -- the encoder half exists so
    // the decoder has something to attend back into, not as an output itself.
    auto [attn_out, attn_weights] =
        attention->forward(enriched, enriched, enriched, pluss::series::undefined(), pluss::series::undefined(), /*causal=*/true);

    const Series decoder_enriched = enriched.narrow_copy(1, options.encoder_length, options.decoder_length);
    const Series decoder_attn_out = attn_out.narrow_copy(1, options.encoder_length, options.decoder_length);
    const Series decoder_gated_lstm = gated_lstm.narrow_copy(1, options.encoder_length, options.decoder_length);
    const Series decoder_attn_weights =
        attn_weights.narrow_copy(1, options.encoder_length, options.decoder_length);

    const Series post_attn = post_attn_gate->forward(decoder_attn_out, decoder_enriched);
    const Series ff_out = position_wise_grn->forward(post_attn);
    const Series final_out = final_gate->forward(ff_out, decoder_gated_lstm);

    const Series quantiles = output_head->forward(final_out);
    return TFTForecast{.quantiles = quantiles, .attention_weights = decoder_attn_weights};
  }
};

namespace {

[[nodiscard]] Series index_select_field_(const Series& field, const Series& index) {
  return field.defined() ? field.index_select(0, index) : field;
}

[[nodiscard]] TFTBatch index_select_batch_(const TFTBatch& batch, const Series& index) {
  TFTBatch out{};
  out.static_inputs = index_select_field_(batch.static_inputs, index);
  out.encoder_inputs = index_select_field_(batch.encoder_inputs, index);
  out.decoder_inputs = index_select_field_(batch.decoder_inputs, index);
  out.target = index_select_field_(batch.target, index);
  return out;
}

[[nodiscard]] Series move_field_(const Series& field, Platform platform) {
  return field.defined() ? field.to(platform) : field;
}

[[nodiscard]] TFTBatch move_batch_(const TFTBatch& batch, Platform platform) {
  TFTBatch out{};
  out.static_inputs = move_field_(batch.static_inputs, platform);
  out.encoder_inputs = move_field_(batch.encoder_inputs, platform);
  out.decoder_inputs = move_field_(batch.decoder_inputs, platform);
  out.target = move_field_(batch.target, platform);
  return out;
}

// Validates feature *widths* (encoder_size/decoder_size/static_size)
// against `options`. Deliberately does not validate the *length* axis
// (encoder_inputs/decoder_inputs' own sizes()[1]) -- the LSTM encoder/
// decoder and the causal self-attention layer are architecturally
// length-agnostic, so a batch with a different lookback/horizon length
// than training still predicts correctly; see TemporalFusionTransformer's
// own Introduction for why that is kept, unlike the width checks below.
// Width mismatches are not architecturally safe: VariableSelectionNetwork
// (tft_blocks.h) narrows exactly `options.*_size` columns out of its input's
// last axis regardless of that axis's actual size -- a too-narrow input
// throws from narrow_copy's own bounds check, but a too-wide one silently
// reads only the first `options.*_size` columns and ignores the rest,
// which is why this check exists rather than relying on that incidental
// throw.
void validate_batch_shape_(const TFTBatch& batch, const TFTOptions& options, const char* what) {
  PLUSS_CHECK(batch.encoder_inputs.dim() == 3 && batch.encoder_inputs.sizes()[2] == options.encoder_size,
             std::string(what) + ": encoder_inputs must be [N, encoder_length, encoder_size].");
  PLUSS_CHECK(batch.decoder_inputs.dim() == 3 && batch.decoder_inputs.sizes()[2] == options.decoder_size,
             std::string(what) + ": decoder_inputs must be [N, decoder_length, decoder_size].");
  if (options.static_size > 0) {
    PLUSS_CHECK(batch.static_inputs.defined(),
               std::string(what) + ": static_inputs is required when TFTOptions::static_size > 0.");
    PLUSS_CHECK(batch.static_inputs.dim() == 2 && batch.static_inputs.sizes()[1] == options.static_size,
               std::string(what) + ": static_inputs must be [N, static_size].");
  }
}

[[nodiscard]] TrainOptions make_optimizer_options_(const TFTTrainOptions& options) {
  TrainOptions out{};
  out.optimizer = options.optimizer;
  out.learning_rate = options.learning_rate;
  out.objective_regularization = options.objective_regularization;
  out.loop_regularization = options.loop_regularization;
  out.validation = options.validation;
  out.progress = options.progress;
  out.execution = options.execution;
  return out;
}

} // namespace

TemporalFusionTransformer::TemporalFusionTransformer(TFTOptions options)
    : impl_(std::make_shared<Impl>(options)), options_(std::move(options)) {
  PLUSS_CHECK(options_.encoder_size > 0 && options_.decoder_size > 0 && options_.encoder_length > 0 &&
                 options_.decoder_length > 0 && options_.hidden_size > 0 && options_.lstm_layers > 0 &&
                 options_.attention_heads > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer: encoder_size, decoder_size, encoder_length, "
             "decoder_length, hidden_size, lstm_layers and attention_heads must all be positive.");
  PLUSS_CHECK(options_.hidden_size % options_.attention_heads == 0,
             "pluss::ai::ice::models::TemporalFusionTransformer: hidden_size must be a multiple of "
             "attention_heads.");
  PLUSS_CHECK(!options_.quantiles.empty(),
             "pluss::ai::ice::models::TemporalFusionTransformer: quantiles must not be empty.");
}

bool TemporalFusionTransformer::ready() const noexcept { return impl_->trained; }

TemporalFusionTransformer& TemporalFusionTransformer::to(Platform platform) {
  PLUSS_CALL_GUARD();
  impl_->move_to(platform);
  return *this;
}

void TemporalFusionTransformer::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::TemporalFusionTransformer::save: model has not been trained.");
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::TemporalFusionTransformer::save: output path cannot be empty.");
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
  write_num_line(out, "lstm_layers", options_.lstm_layers);
  write_num_line(out, "attention_heads", options_.attention_heads);
  write_num_line(out, "dropout", options_.dropout);
  write_num_line(out, "quantile_count", static_cast<std::int64_t>(options_.quantiles.size()));
  for (std::size_t i = 0; i < options_.quantiles.size(); ++i) {
    write_num_line(out, "quantile." + std::to_string(i), options_.quantiles[i]);
  }
  write_categoricals(out, "static_categoricals", options_.static_categoricals);
  write_categoricals(out, "encoder_categoricals", options_.encoder_categoricals);
  write_categoricals(out, "decoder_categoricals", options_.decoder_categoricals);
  write_num_line(out, "cfg_dtype", static_cast<int>(options_.cfg.dtype));
  write_num_line(out, "cfg_platform", static_cast<int>(options_.cfg.platform));
  write_num_line(out, "seed", options_.seed);

  std::ofstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TemporalFusionTransformer::save: failed to open '" +
                  std::string(filepath) + "'.");
  file << out.str();
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TemporalFusionTransformer::save: failed while writing '" +
                  std::string(filepath) + "'.");
}

TemporalFusionTransformer TemporalFusionTransformer::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::TemporalFusionTransformer::load: input path cannot be empty.");
  std::ifstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TemporalFusionTransformer::load: failed to open '" +
                  std::string(filepath) + "'.");
  std::string magic{};
  PLUSS_CHECK(std::getline(file, magic) && magic == kMagic,
              "pluss::ai::ice::models::TemporalFusionTransformer::load: invalid model artifact header.");
  std::ostringstream body{};
  body << file.rdbuf();
  const auto kv = parse_kv_body(body.str(), kLoadContext);
  PLUSS_CHECK(require_i64(kv, "schema", kLoadContext) == kSchema,
              "pluss::ai::ice::models::TemporalFusionTransformer::load: unsupported schema.");

  TFTOptions options{};
  options.static_size = require_i64(kv, "static_size", kLoadContext);
  options.encoder_size = require_i64(kv, "encoder_size", kLoadContext);
  options.decoder_size = require_i64(kv, "decoder_size", kLoadContext);
  options.encoder_length = require_i64(kv, "encoder_length", kLoadContext);
  options.decoder_length = require_i64(kv, "decoder_length", kLoadContext);
  options.hidden_size = require_i64(kv, "hidden_size", kLoadContext);
  options.lstm_layers = require_i64(kv, "lstm_layers", kLoadContext);
  options.attention_heads = require_i64(kv, "attention_heads", kLoadContext);
  options.dropout = require_f64(kv, "dropout", kLoadContext);
  const auto quantile_count = require_i64(kv, "quantile_count", kLoadContext);
  options.quantiles.clear();
  options.quantiles.reserve(static_cast<std::size_t>(quantile_count));
  for (std::int64_t i = 0; i < quantile_count; ++i) {
    options.quantiles.push_back(require_f64(kv, "quantile." + std::to_string(i), kLoadContext));
  }
  options.static_categoricals = read_categoricals(kv, "static_categoricals", kLoadContext);
  options.encoder_categoricals = read_categoricals(kv, "encoder_categoricals", kLoadContext);
  options.decoder_categoricals = read_categoricals(kv, "decoder_categoricals", kLoadContext);
  options.cfg = Config{}
                    .with_dtype(static_cast<ScalarType>(require_i64(kv, "cfg_dtype", kLoadContext)))
                    .with_platform(static_cast<Platform>(require_i64(kv, "cfg_platform", kLoadContext)));
  options.seed = require_u64(kv, "seed", kLoadContext);

  TemporalFusionTransformer model(options);
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

void TemporalFusionTransformer::learn(const TFTBatch& train, const TFTTrainOptions& options) {
  PLUSS_CALL_GUARD();
  if (options.validation.split_ratio <= 0.0) {
    learn(train, TFTBatch{}, options);
    return;
  }

  const auto n = train.encoder_inputs.sizes()[0];
  std::mt19937_64 rng(options.seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  if (options.shuffle) std::ranges::shuffle(order, rng);

  const auto val_n =
      static_cast<std::int64_t>(static_cast<double>(n) * options.validation.split_ratio);
  const auto train_n = n - val_n;
  PLUSS_CHECK(train_n > 0 && val_n > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: validation.split_ratio leaves an "
             "empty split.");

  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(train.encoder_inputs.platform());
  const Series train_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin(), order.begin() + train_n), {train_n}, idx_cfg);
  const Series val_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin() + train_n, order.end()), {val_n}, idx_cfg);

  learn(index_select_batch_(train, train_index), index_select_batch_(train, val_index), options);
}

void TemporalFusionTransformer::learn(const TFTBatch& train, const TFTBatch& val, const TFTTrainOptions& options) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(options.execution.max_epochs > 0 && options.batch_size > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: max_epochs and batch_size must be "
             "positive.");
  PLUSS_CHECK(options.validation.every_n_epochs > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: validation.every_n_epochs must be "
             "positive.");
  PLUSS_CHECK(train.encoder_inputs.defined() && train.decoder_inputs.defined() && train.target.defined(),
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: encoder_inputs, decoder_inputs and "
             "target are required.");
  validate_batch_shape_(train, options_, "pluss::ai::ice::models::TemporalFusionTransformer::learn(train)");

  const Platform target_platform = resolve_platform(options.execution.platform);
  impl_->move_to(target_platform);

  const TFTBatch train_on_device = move_batch_(train, target_platform);
  const bool has_val = val.encoder_inputs.defined() && val.target.defined();
  if (has_val) {
    validate_batch_shape_(val, options_, "pluss::ai::ice::models::TemporalFusionTransformer::learn(val)");
  }
  const TFTBatch val_on_device = has_val ? move_batch_(val, target_platform) : TFTBatch{};

  const auto parameters = impl_->all_parameters();
  PLUSS_CHECK(!parameters.empty(),
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: model has no parameters.");
  const TrainOptions runtime_options = make_optimizer_options_(options);
  auto optimizer = ice::make_optimizer(parameters, runtime_options);

  const auto n = train_on_device.encoder_inputs.sizes()[0];
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
  if (options.progress.verbose) {
    callbacks.add(std::make_shared<EpochLoggerCallback>(options.progress));
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
      .recipe = &runtime_options,
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
        const TFTBatch mini = index_select_batch_(train_on_device, index);
        const TFTForecast forecast = impl_->forward(options_, mini);
        Series loss = detail::quantile_loss(forecast.quantiles, mini.target, options_.quantiles);
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
        const TFTForecast val_forecast = impl_->forward(options_, val_on_device);
        const double val_loss_value =
            detail::quantile_loss(val_forecast.quantiles, val_on_device.target, options_.quantiles)
                .item<double>();
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
        std::string("pluss::ai::ice::models::TemporalFusionTransformer::learn: training became numerically "
                    "unstable: ") +
        e.what());
  }

  impl_->set_training(false);
  impl_->trained = true;
}

TFTForecast TemporalFusionTransformer::predict(const TFTBatch& batch) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_->trained,
             "pluss::ai::ice::models::TemporalFusionTransformer::predict: model has not been trained.");
  PLUSS_CHECK(batch.encoder_inputs.defined() && batch.decoder_inputs.defined(),
             "pluss::ai::ice::models::TemporalFusionTransformer::predict: encoder_inputs and decoder_inputs "
             "are required.");
  validate_batch_shape_(batch, options_, "pluss::ai::ice::models::TemporalFusionTransformer::predict");

  const auto parameters = impl_->all_parameters();
  const NoGradGuard guard;
  const Platform model_platform = parameters.front()->platform();
  const TFTBatch batch_on_device = move_batch_(batch, model_platform);
  return impl_->forward(options_, batch_on_device);
}

double TemporalFusionTransformer::assess(const TFTBatch& batch) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(batch.target.defined(),
             "pluss::ai::ice::models::TemporalFusionTransformer::assess: target is required.");
  const TFTForecast forecast = predict(batch);
  const Series target_on_device = batch.target.to(forecast.quantiles.platform());
  return detail::quantile_loss(forecast.quantiles, target_on_device, options_.quantiles).item<double>();
}

} // namespace pluss::ai::ice::models
