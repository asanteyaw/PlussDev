#pragma once

#include "../Module.h"

#include <cmath>
#include <utility>
#include <vector>

namespace pluss::ai::nn {

struct EmbeddingOptions {
  std::int64_t cardinality = 0; // category codes must be in [0, cardinality).
  std::int64_t hidden_size = 0; // embedding width.
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

// A plain learned embedding table: one row per category, looked up by
// index. Promoted out of ice::models::TemporalFusionTransformer's
// originally-private building blocks (Ice/models/ts/tft_blocks.h) once a
// second consumer needed it -- see ICE_BLUEPRINT.md's centralization note.
// Follows the same allocate-then-register-then-fill-in-place construction
// pattern as the engine's other built-in modules (Fusion, Conv, ...):
// weight_ is allocated via Series::empty() and registered atomically, then
// reset_parameters() only ever mutates it in place via copy_(), so it can
// never end up registered-but-untracked.
class PLUSS_API EmbeddingImpl final : public Module {
public:
  explicit EmbeddingImpl(EmbeddingOptions options) : Module(true), options_(options) {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    weight_ = Series::empty({options_.cardinality, options_.hidden_size}, cfg.dtype, cfg.platform);
    this->parameter_record("weight", weight_);
    reset_parameters();
  }

  // Re-randomizes the embedding table in place. Never reassigns weight_,
  // so it's safe to call any number of times after construction --
  // registration and tracks_slope_ (set once, above) are never disturbed.
  void reset_parameters() {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const double bound = 1.0 / std::sqrt(static_cast<double>(options_.hidden_size));
    weight_.copy_(::pluss::detail::nn::uniform_(weight_.sizes(), -bound, bound, cfg, options_.seed));
  }

  // `index`: any shape, integer-valued category codes in [0, cardinality).
  // Returns `index`'s shape with an extra trailing hidden_size dimension.
  [[nodiscard]] Series forward(const Series& index) const {
    const Series flat_index = index.reshape({index.numel()}).to(ScalarType::Int64);
    const Series looked_up = weight_.index_select(0, flat_index); // [numel, hidden_size]
    std::vector<std::int64_t> out_shape(index.sizes().begin(), index.sizes().end());
    out_shape.push_back(options_.hidden_size);
    return looked_up.reshape(out_shape);
  }
  Series operator()(const Series& index) const { return forward(index); }
  EmbeddingImpl* operator->() noexcept { return this; }
  const EmbeddingImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  EmbeddingOptions& options() noexcept { return options_; }
  const EmbeddingOptions& options() const noexcept { return options_; }

private:
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  EmbeddingOptions options_;
  Series weight_ = pluss::series::undefined();
};
PLUSS_MODULE_IMPL(Embedding, EmbeddingImpl);

} // namespace pluss::ai::nn
