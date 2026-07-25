#include "pluss/ai/ice/models/tft.h"

#include "tft_blocks.h"

#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <random>

namespace pluss::ai::ice::models {

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

    Series c_selection{};
    Series c_enrichment{};
    Series c_hidden{};
    Series c_cell{};
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

    std::tuple<Series, Series> initial_state{};
    if (has_static) {
      const auto batch_size = batch.encoder_inputs.sizes()[0];
      const Series h0 = c_hidden.unsqueeze(0).expand({options.lstm_layers, batch_size, options.hidden_size});
      const Series c0 = c_cell.unsqueeze(0).expand({options.lstm_layers, batch_size, options.hidden_size});
      initial_state = std::tuple<Series, Series>{h0, c0};
    }

    auto [encoder_lstm_out, encoder_state] = lstm->forward(encoder_selected, initial_state);
    auto [decoder_lstm_out, decoder_state] = lstm->forward(decoder_selected, encoder_state);
    (void)decoder_state;

    const Series lstm_out = pluss::unite({encoder_lstm_out, decoder_lstm_out}, 1);
    const Series vsn_out = pluss::unite({encoder_selected, decoder_selected}, 1);
    const Series gated_lstm = post_lstm_gate->forward(lstm_out, vsn_out);

    const Series enriched = static_enrichment_grn->forward(gated_lstm, expand_context(c_enrichment));

    // Self-attention over the full (encoder+decoder) sequence with a causal
    // mask, then keep only the decoder queries -- the encoder half exists so
    // the decoder has something to attend back into, not as an output itself.
    auto [attn_out, attn_weights] =
        attention->forward(enriched, enriched, enriched, Series(), Series(), /*causal=*/true);

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

[[nodiscard]] TrainOptions make_optimizer_options_(const TFTTrainOptions& options) {
  TrainOptions out{};
  out.optimizer = options.optimizer;
  out.learning_rate = options.learning_rate;
  out.grad_clip = options.grad_clip;
  out.fail_on_non_finite = options.fail_on_non_finite;
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

void TemporalFusionTransformer::learn(const TFTBatch& train, const TFTTrainOptions& options) {
  PLUSS_CALL_GUARD();
  if (options.val_ratio <= 0.0) {
    learn(train, TFTBatch{}, options);
    return;
  }

  const auto n = train.encoder_inputs.sizes()[0];
  std::mt19937_64 rng(options.seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  if (options.shuffle) std::ranges::shuffle(order, rng);

  const auto val_n = static_cast<std::int64_t>(static_cast<double>(n) * options.val_ratio);
  const auto train_n = n - val_n;
  PLUSS_CHECK(train_n > 0 && val_n > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: val_ratio leaves an empty split.");

  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(train.encoder_inputs.platform());
  const Series train_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin(), order.begin() + train_n), {train_n}, idx_cfg);
  const Series val_index =
      pluss::make_series(std::vector<std::int64_t>(order.begin() + train_n, order.end()), {val_n}, idx_cfg);

  learn(index_select_batch_(train, train_index), index_select_batch_(train, val_index), options);
}

void TemporalFusionTransformer::learn(const TFTBatch& train, const TFTBatch& val, const TFTTrainOptions& options) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(options.max_epochs > 0 && options.batch_size > 0,
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: max_epochs and batch_size must be "
             "positive.");
  PLUSS_CHECK(train.encoder_inputs.defined() && train.decoder_inputs.defined() && train.target.defined(),
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: encoder_inputs, decoder_inputs and "
             "target are required.");
  if (options_.static_size > 0) {
    PLUSS_CHECK(train.static_inputs.defined(),
               "pluss::ai::ice::models::TemporalFusionTransformer::learn: static_inputs is required when "
               "TFTOptions::static_size > 0.");
  }

  const Platform target_platform = resolve_platform(options.platform);
  impl_->move_to(target_platform);

  const TFTBatch train_on_device = move_batch_(train, target_platform);
  const bool has_val = val.encoder_inputs.defined() && val.target.defined();
  const TFTBatch val_on_device = has_val ? move_batch_(val, target_platform) : TFTBatch{};

  const auto parameters = impl_->all_parameters();
  PLUSS_CHECK(!parameters.empty(),
             "pluss::ai::ice::models::TemporalFusionTransformer::learn: model has no parameters.");
  auto optimizer = ice::make_optimizer(parameters, make_optimizer_options_(options));

  const auto n = train_on_device.encoder_inputs.sizes()[0];
  std::mt19937_64 rng(options.seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(target_platform);

  double best_val_loss = std::numeric_limits<double>::infinity();
  std::int64_t epochs_without_improvement = 0;

  impl_->set_training(true);
  try {
    for (std::int64_t epoch = 0; epoch < options.max_epochs; ++epoch) {
      if (options.shuffle) std::ranges::shuffle(order, rng);
      for (std::int64_t start = 0; start < n; start += options.batch_size) {
        const auto end = std::min(n, start + options.batch_size);
        const Series batch_index =
            pluss::make_series(std::vector<std::int64_t>(order.begin() + start, order.begin() + end),
                               {end - start}, idx_cfg);
        const TFTBatch mini = index_select_batch_(train_on_device, batch_index);
        const TFTForecast forecast = impl_->forward(options_, mini);
        const Series loss = detail::quantile_loss(forecast.quantiles, mini.target, options_.quantiles);

        optimizer->clear();
        loss.reverse();
        optimizer->step();
      }

      if (options.early_stopping && has_val) {
        impl_->set_training(false);
        const NoGradGuard guard(parameters);
        const TFTForecast val_forecast = impl_->forward(options_, val_on_device);
        const double val_loss_value =
            detail::quantile_loss(val_forecast.quantiles, val_on_device.target, options_.quantiles)
                .item<double>();
        impl_->set_training(true);
        if (val_loss_value < best_val_loss - options.min_delta) {
          best_val_loss = val_loss_value;
          epochs_without_improvement = 0;
        } else if (++epochs_without_improvement >= options.patience) {
          break;
        }
      }
    }
  } catch (const std::exception& e) {
    impl_->set_training(false);
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

  const auto parameters = impl_->all_parameters();
  const NoGradGuard guard(parameters);
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
