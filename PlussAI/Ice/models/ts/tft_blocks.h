#pragma once

// Internal building blocks for ice::models::TemporalFusionTransformer. Not a
// public header — only included by files under Ice/models/ts/. Each block
// is an original implementation of a standard, published neural building
// block (a gated residual network, a variable selection network); none of
// it is copied from any reference codebase.
//
// elu/LayerNorm/Embedding used to live here too, but were promoted to
// pluss::ai::nn:: (PlussAI/NodeK/NN/modules/activation/Activation.h,
// normalization/LayerNorm.h, embedding/Embedding.h) once a second consumer
// (ice::models::DeepAR) needed them -- see ICE_BLUEPRINT.md's
// centralization note. This header now only holds pieces that are
// genuinely specific to TFT's own architecture.

#include "pluss/ai/ice/models/ts/temporal_fusion_transformer.h"
#include "pluss/nn.h"

#include <utility>
#include <vector>

namespace pluss::ai::ice::models::detail {

struct GatedResidualNetworkOptions {
  std::int64_t input_size = 0;
  std::int64_t hidden_size = 0;
  std::int64_t output_size = 0;   // usually == hidden_size; the GRN's own output width.
  std::int64_t context_size = 0;  // 0 = no static-context injection.
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

// The Gated Residual Network (GRN) is TFT's fundamental reusable block:
// a feedforward transform with optional static-context injection, ELU
// nonlinearity, a sigmoid-gated linear unit (GLU) controlling how much of
// the transform passes through, and a residual + LayerNorm wrapper. Every
// other TFT component (variable selection, static enrichment, the
// position-wise feedforward after attention) is built from this one block.
//
//   eta2 = ELU(fc1(a) [+ fc_context(context)])
//   eta1 = fc2(eta2)
//   GLU(eta1) = sigmoid(gate(eta1)) * value(eta1)
//   GRN(a, context) = LayerNorm(skip(a) + GLU(eta1))
//
// where skip(a) is `a` itself when input_size == output_size, otherwise a
// learned linear projection to output_size.
class GatedResidualNetworkImpl final : public pluss::nn::Module {
public:
  explicit GatedResidualNetworkImpl(GatedResidualNetworkOptions options)
      : Module(true),
        options_(options),
        fc1_(options.input_size, options.hidden_size, /*bias=*/true, options.cfg, options.seed + 1),
        fc2_(options.hidden_size, options.hidden_size, /*bias=*/true, options.cfg, options.seed + 2),
        // Constructed regardless of whether context is used, with a
        // harmless dummy input width when it isn't -- only registered and
        // invoked when options.context_size > 0 (see below).
        fc_context_(options.context_size > 0 ? options.context_size : 1, options.hidden_size, /*bias=*/false,
                    options.cfg, options.seed + 3),
        gate_(options.hidden_size, options.output_size, /*bias=*/true, options.cfg, options.seed + 4),
        value_(options.hidden_size, options.output_size, /*bias=*/true, options.cfg, options.seed + 5),
        skip_(options.input_size == options.output_size ? 1 : options.input_size,
              options.input_size == options.output_size ? 1 : options.output_size, /*bias=*/true, options.cfg,
              options.seed + 6),
        norm_(pluss::nn::LayerNormOptions{options.output_size, 1e-5, options.cfg}) {
    this->module_record("fc1", fc1_);
    this->module_record("fc2", fc2_);
    if (options_.context_size > 0) this->module_record("fc_context", fc_context_);
    this->module_record("gate", gate_);
    this->module_record("value", value_);
    if (options_.input_size != options_.output_size) this->module_record("skip", skip_);
    this->module_record("norm", norm_);
  }

  [[nodiscard]] Series forward(const Series& a, const Series& context = Series()) const {
    Series pre_elu = fc1_->forward(a);
    if (options_.context_size > 0 && context.defined()) {
      pre_elu = pre_elu.add(fc_context_->forward(context));
    }
    const Series eta2 = pluss::nn::elu(pre_elu);
    const Series eta1 = fc2_->forward(eta2);
    const Series glu_out = gate_->forward(eta1).sigmoid().mul(value_->forward(eta1));
    const Series skip = (options_.input_size == options_.output_size) ? a : skip_->forward(a);
    return norm_->forward(skip.add(glu_out));
  }
  Series operator()(const Series& a, const Series& context = Series()) const { return forward(a, context); }

  [[nodiscard]] const GatedResidualNetworkOptions& options() const noexcept { return options_; }

private:
  GatedResidualNetworkOptions options_;
  pluss::nn::Fusion fc1_;
  pluss::nn::Fusion fc2_;
  pluss::nn::Fusion fc_context_;
  pluss::nn::Fusion gate_;
  pluss::nn::Fusion value_;
  pluss::nn::Fusion skip_;
  pluss::nn::LayerNorm norm_;
};
PLUSS_MODULE_IMPL(GatedResidualNetwork, GatedResidualNetworkImpl);

struct GatedAddNormOptions {
  std::int64_t size = 0;
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

// The lighter-weight gated skip connection TFT uses at three points (after
// the LSTM stack, after self-attention, after the position-wise
// feedforward): a GLU gate straight on `x` (no fc1/fc2/ELU transform stage
// -- unlike GatedResidualNetwork, there's nothing to nonlinearly transform
// here, just a learned amount of `x` to let through), added to a residual
// and normalized: `LayerNorm(residual + sigmoid(gate(x)) * value(x))`.
// Assumes `x` and `residual` are already the same width (`size`); TFT only
// ever uses this once everything is already projected to hidden_size.
class GatedAddNormImpl final : public pluss::nn::Module {
public:
  explicit GatedAddNormImpl(GatedAddNormOptions options)
      : Module(true),
        options_(options),
        gate_(options.size, options.size, /*bias=*/true, options.cfg, options.seed + 1),
        value_(options.size, options.size, /*bias=*/true, options.cfg, options.seed + 2),
        norm_(pluss::nn::LayerNormOptions{options.size, 1e-5, options.cfg}) {
    this->module_record("gate", gate_);
    this->module_record("value", value_);
    this->module_record("norm", norm_);
  }

  [[nodiscard]] Series forward(const Series& x, const Series& residual) const {
    const Series glu_out = gate_->forward(x).sigmoid().mul(value_->forward(x));
    return norm_->forward(residual.add(glu_out));
  }
  Series operator()(const Series& x, const Series& residual) const { return forward(x, residual); }

private:
  GatedAddNormOptions options_;
  pluss::nn::Fusion gate_;
  pluss::nn::Fusion value_;
  pluss::nn::LayerNorm norm_;
};
PLUSS_MODULE_IMPL(GatedAddNorm, GatedAddNormImpl);

struct VariableSelectionOptions {
  std::int64_t num_vars = 0;
  std::int64_t hidden_size = 0;
  std::int64_t context_size = 0;
  // Which of the [0, num_vars) slots are categorical -- see
  // TFTOptions::CategoricalVariable's identically-shaped public doc comment.
  // Any index not listed here is continuous.
  std::vector<pluss::ai::ice::models::TFTOptions::CategoricalVariable> categoricals = {};
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

// A Variable Selection Network (VSN) takes `num_vars` features (already
// concatenated into one [..., num_vars] tensor -- any number of leading
// batch/time dims is fine, Fusion's matmul broadcasts over them; a
// categorical slot's raw value is its integer category code stored as a
// float), independently transforms each variable into a [..., hidden_size]
// representation -- a small per-variable GRN for continuous variables, a
// learned per-category pluss::nn::Embedding for categorical ones -- learns
// a softmax weighting over which variables matter most at each position
// from the *concatenation* of those hidden_size representations (not the
// raw input values -- feeding a raw categorical code into that weighting
// network would treat an arbitrary category label as an ordinal quantity,
// which is wrong regardless of how the variable is otherwise processed),
// and combines them via the learned weights. Returns both the combined
// [..., hidden_size] representation and the [..., num_vars] selection
// weights (useful for interpretability, matching TFT's own design goal).
class VariableSelectionNetworkImpl final : public pluss::nn::Module {
public:
  explicit VariableSelectionNetworkImpl(VariableSelectionOptions options)
      : Module(true),
        options_(options),
        flatten_grn_(GatedResidualNetworkOptions{
            .input_size = options.num_vars * options.hidden_size,
            .hidden_size = options.hidden_size,
            .output_size = options.num_vars,
            .context_size = options.context_size,
            .cfg = options.cfg,
            .seed = options.seed + 1000,
        }) {
    this->module_record("flatten_grn", flatten_grn_);

    is_categorical_.assign(static_cast<std::size_t>(options_.num_vars), false);
    std::vector<std::int64_t> cardinality(static_cast<std::size_t>(options_.num_vars), 0);
    for (const auto& spec : options_.categoricals) {
      PLUSS_CHECK(spec.variable_index >= 0 && spec.variable_index < options_.num_vars,
                 "pluss::ai::ice::models::detail::VariableSelectionNetwork: categorical variable_index out of range");
      PLUSS_CHECK(spec.cardinality > 0,
                 "pluss::ai::ice::models::detail::VariableSelectionNetwork: categorical cardinality must be > 0");
      is_categorical_[static_cast<std::size_t>(spec.variable_index)] = true;
      cardinality[static_cast<std::size_t>(spec.variable_index)] = spec.cardinality;
    }

    slot_lookup_.assign(static_cast<std::size_t>(options_.num_vars), 0);
    for (std::int64_t i = 0; i < options_.num_vars; ++i) {
      const auto idx = static_cast<std::size_t>(i);
      if (is_categorical_[idx]) {
        slot_lookup_[idx] = static_cast<std::int64_t>(var_embeddings_.size());
        var_embeddings_.emplace_back(pluss::nn::EmbeddingOptions{
            .cardinality = cardinality[idx],
            .hidden_size = options.hidden_size,
            .cfg = options.cfg,
            .seed = options.seed + 3000 + static_cast<std::uint64_t>(i) * 7,
        });
        this->module_record("var_embedding_" + std::to_string(i), var_embeddings_.back());
      } else {
        slot_lookup_[idx] = static_cast<std::int64_t>(var_grns_.size());
        var_grns_.emplace_back(GatedResidualNetworkOptions{
            .input_size = 1,
            .hidden_size = options.hidden_size,
            .output_size = options.hidden_size,
            .context_size = 0,
            .cfg = options.cfg,
            .seed = options.seed + 2000 + static_cast<std::uint64_t>(i) * 7,
        });
        this->module_record("var_grn_" + std::to_string(i), var_grns_.back());
      }
    }
  }

  // `flat_vars`: [..., num_vars]. `context`: [..., context_size] or
  // undefined when options.context_size == 0. Returns (combined [...,
  // hidden_size], weights [..., num_vars]).
  [[nodiscard]] std::pair<Series, Series> forward(const Series& flat_vars, const Series& context = Series()) const {
    const std::int64_t last_dim = flat_vars.sizes().size() - 1;

    std::vector<Series> processed{};
    processed.reserve(static_cast<std::size_t>(options_.num_vars));
    for (std::int64_t i = 0; i < options_.num_vars; ++i) {
      const auto idx = static_cast<std::size_t>(i);
      const Series one_var = flat_vars.narrow_copy(last_dim, i, 1); // [..., 1]
      if (is_categorical_[idx]) {
        const Series index = one_var.squeeze(-1); // [...] category codes
        processed.push_back(var_embeddings_[static_cast<std::size_t>(slot_lookup_[idx])]->forward(index));
      } else {
        processed.push_back(var_grns_[static_cast<std::size_t>(slot_lookup_[idx])]->forward(one_var));
      }
    }
    const Series stacked = pluss::pile(processed, -2); // [..., num_vars, hidden_size]

    std::vector<std::int64_t> flat_shape(stacked.sizes().begin(), stacked.sizes().end() - 2);
    flat_shape.push_back(options_.num_vars * options_.hidden_size);
    const Series flattened = stacked.reshape(flat_shape); // [..., num_vars*hidden_size]

    const Series weights = flatten_grn_->forward(flattened, context).softmax(-1); // [..., num_vars]
    const Series weights_expanded = weights.unsqueeze(-1);        // [..., num_vars, 1]
    const Series combined = stacked.mul(weights_expanded).sum(-2, false); // [..., hidden_size]
    return {combined, weights};
  }

private:
  VariableSelectionOptions options_;
  GatedResidualNetwork flatten_grn_;
  std::vector<GatedResidualNetwork> var_grns_;    // one per continuous slot, in slot order
  std::vector<pluss::nn::Embedding> var_embeddings_; // one per categorical slot, in slot order
  std::vector<bool> is_categorical_;              // size num_vars
  std::vector<std::int64_t> slot_lookup_;         // size num_vars; index into var_grns_ or var_embeddings_
};
PLUSS_MODULE_IMPL(VariableSelectionNetwork, VariableSelectionNetworkImpl);

// Pinball (quantile) loss, averaged over every element and every requested
// quantile. `target`: [N, horizon]. `predicted_quantiles`: [N, horizon,
// num_quantiles], in the same order as `quantiles`.
[[nodiscard]] inline Series quantile_loss(const Series& predicted_quantiles,
                                          const Series& target,
                                          const std::vector<double>& quantiles) {
  const Series target_expanded = target.unsqueeze(-1); // [N, horizon, 1], broadcasts against num_quantiles.
  const Series error = target_expanded.sub(predicted_quantiles); // [N, horizon, num_quantiles]

  std::vector<Series> per_quantile{};
  per_quantile.reserve(quantiles.size());
  for (std::size_t i = 0; i < quantiles.size(); ++i) {
    const Series e = error.narrow_copy(error.sizes().size() - 1, static_cast<std::int64_t>(i), 1);
    const double q = quantiles[i];
    // max(q*e, (q-1)*e), same branch-free identity style as pluss::nn::elu.
    // max(q*e, (q-1)*e) = q*e + relu(-e) -- check: e>=0 -> relu(-e)=0,
    // result=q*e, and q*e >= (q-1)*e when e>=0 since they differ by -e<=0.
    // e<0 -> relu(-e)=-e, result=(q-1)*e, and that's >= q*e when e<0 since
    // they differ by -e>0. Both match the max().
    per_quantile.push_back(e.mul(q).add(e.neg().relu()));
  }
  const Series stacked = pluss::unite(per_quantile, -1); // [N, horizon, num_quantiles]
  return stacked.mean();
}

} // namespace pluss::ai::ice::models::detail
