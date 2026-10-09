#pragma once

#include "pluss/ai/lymph/model.h"

namespace pluss::ai::lymph::models {
namespace detail {

[[nodiscard]] inline bool model_family_is_(const Model& model, ModelFamily family) noexcept {
  return model.has_model_spec() && model.model_spec()->family == family;
}

inline void require_model_family_(const Model& model, ModelFamily family, std::string_view where) {
  if (!model_family_is_(model, family)) {
    ::pluss::throw_error(std::string(where) + " requires a Model from the expected family.");
  }
  if (model.task_kind() != TaskKind::Transform) {
    ::pluss::throw_error(std::string(where) + " requires a transform-style Model.");
  }
}

[[nodiscard]] inline Series transform_state_at_(const Model& model, std::size_t index, std::string_view where) {
  const std::vector<std::vector<Series>> states = model.dump_transform_states();
  if (states.empty()) {
    ::pluss::throw_error(std::string(where) + " requires a learned transform-state model.");
  }
  const std::vector<Series>& state = states.front();
  if (state.size() <= index || !state[index].defined()) {
    ::pluss::throw_error(std::string(where) + " requires a learned model with the requested state.");
  }
  return state[index];
}

[[nodiscard]] inline Series result_index_series_(std::int64_t count, ::pluss::Platform platform) {
  return pluss::series::arange(0, count, 1, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

} // namespace detail

namespace cluster {

class Result final {
 public:
  Result(Model model, Series labels) : model_(std::move(model)), labels_(std::move(labels)) {
    detail::require_model_family_(model_, ModelFamily::Cluster, "cluster::Result");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] const Series& labels() const noexcept { return labels_; }
  [[nodiscard]] Series centers() const { return detail::transform_state_at_(model_, 1, "cluster::Result::centers"); }
  [[nodiscard]] bool trained() const noexcept { return model_.trained(); }

  [[nodiscard]] std::int64_t cluster_count() const {
    const Series c = centers();
    if (c.dim() == 0) return 0;
    return c.sizes().front();
  }

  [[nodiscard]] pluss::table::DataFrame board() const {
    const std::int64_t k = cluster_count();
    const Series flat_labels = labels_.reshape({labels_.numel()});
    Series counts = pluss::series::undefined();
    for (std::int64_t ci = 0; ci < k; ++ci) {
      const Series count = flat_labels.eq(static_cast<double>(ci)).to(ScalarType::Float64).sum().reshape({1});
      counts = counts.defined() ? counts.unite({count}, 0) : count;
    }
    return pluss::table::DataFrame({
        {"cluster", pluss::table::Slate(detail::result_index_series_(k, flat_labels.platform()))},
        {"count", pluss::table::Slate(counts.reshape({k}))},
    });
  }

 private:
  Model model_;
  Series labels_ = pluss::series::undefined();
};

class Runner final {
 public:
  explicit Runner(Model model) : model_(std::move(model)) {
    detail::require_model_family_(model_, ModelFamily::Cluster, "cluster::model");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }

  [[nodiscard]] Result run(const FeatureBatch& batch,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(batch, learn_options, infer_options);
    return Result(std::move(trained), std::move(labels));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(frame, data_options, learn_options, infer_options);
    return Result(std::move(trained), std::move(labels));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& x,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(x, learn_options, infer_options);
    return Result(std::move(trained), std::move(labels));
  }

 private:
  Model model_;
};

[[nodiscard]] inline Runner model(Model model) {
  return Runner(std::move(model));
}

} // namespace cluster

namespace embed {

class Result final {
 public:
  Result(Model model, Series coordinates) : model_(std::move(model)), coordinates_(std::move(coordinates)) {
    detail::require_model_family_(model_, ModelFamily::Decompose, "embed::Result");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] const Series& coordinates() const noexcept { return coordinates_; }
  [[nodiscard]] const Series& mapped() const noexcept { return coordinates_; }
  [[nodiscard]] Series mean() const { return detail::transform_state_at_(model_, 1, "embed::Result::mean"); }
  [[nodiscard]] Series components() const { return detail::transform_state_at_(model_, 2, "embed::Result::components"); }
  [[nodiscard]] Series variances() const { return detail::transform_state_at_(model_, 3, "embed::Result::variances"); }
  [[nodiscard]] bool trained() const noexcept { return model_.trained(); }

  [[nodiscard]] std::int64_t component_count() const {
    const Series v = variances();
    return v.defined() ? v.numel() : 0;
  }

  [[nodiscard]] pluss::table::DataFrame board() const {
    const Series variance = variances().reshape({component_count()});
    const std::int64_t k = variance.numel();
    const Series total = variance.sum().reshape({1}).clamp_min(1e-12);
    const Series ratio = variance.div(total);
    return pluss::table::DataFrame({
        {"component", pluss::table::Slate(detail::result_index_series_(k, variance.platform()))},
        {"variance", pluss::table::Slate(variance)},
        {"variance_ratio", pluss::table::Slate(ratio)},
    });
  }

 private:
  Model model_;
  Series coordinates_ = pluss::series::undefined();
};

class Runner final {
 public:
  explicit Runner(Model model) : model_(std::move(model)) {
    detail::require_model_family_(model_, ModelFamily::Decompose, "embed::model");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }

  [[nodiscard]] Result run(const FeatureBatch& batch,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series coordinates = trained.learn_map(batch, learn_options, infer_options);
    return Result(std::move(trained), std::move(coordinates));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series coordinates = trained.learn_map(frame, data_options, learn_options, infer_options);
    return Result(std::move(trained), std::move(coordinates));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& x,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series coordinates = trained.learn_map(x, learn_options, infer_options);
    return Result(std::move(trained), std::move(coordinates));
  }

 private:
  Model model_;
};

[[nodiscard]] inline Runner model(Model model) {
  return Runner(std::move(model));
}

} // namespace embed

} // namespace pluss::ai::lymph::models
