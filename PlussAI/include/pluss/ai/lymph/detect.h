#pragma once

#include "pluss/ai/lymph/model.h"

#include <limits>

namespace pluss::ai::lymph::models::detect {
namespace detail {

inline void require_anomaly_model_(const Model& model, std::string_view where) {
  if (!model.has_model_spec() || model.task_kind() != TaskKind::Transform) {
    ::pluss::throw_error(std::string(where) + " requires a transform-style anomaly Model.");
  }
  const ModelFamily family = model.model_spec()->family;
  if (family == ModelFamily::Detect) return;
  if (family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&model.model_spec()->options);
    if (options != nullptr && options->form == KernelForm::OneClass) return;
  }
  ::pluss::throw_error(std::string(where) + " requires a detector Model such as models::one_class_svm or models::isolation_detector.");
}

[[nodiscard]] inline double model_threshold_(const Model& model) {
  std::vector<std::vector<Series>> states{};
  try {
    states = model.dump_transform_states();
  } catch (const ::pluss::Error&) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (states.empty() || states.front().size() < 3 || !states.front()[2].defined()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return states.front()[2].to(pluss::pCPU, ScalarType::Float64).reshape({states.front()[2].numel()}).item<double>();
}

[[nodiscard]] inline DataFrameBatchOptions default_data_options_(pluss::msharp::RangeView::Of<std::string> names) {
  DataFrameBatchOptions options{};
  options.feature_names = copy_range_values(names);
  options.layout = ObservationLayout::Rows;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

} // namespace detail

class Result final {
 public:
  Result(std::string method, Series labels, Series scores, double threshold, std::optional<Model> model = std::nullopt)
      : method_(std::move(method)),
        labels_(std::move(labels)),
        scores_(std::move(scores)),
        threshold_(threshold),
        model_(std::move(model)) {}

  [[nodiscard]] const std::string& method() const noexcept { return method_; }
  [[nodiscard]] bool has_model() const noexcept { return model_.has_value(); }
  [[nodiscard]] const Model& model() const {
    if (!model_.has_value()) {
      ::pluss::throw_error("detect::Result::model: this result was not produced by a Model-backed detector.");
    }
    return *model_;
  }
  [[nodiscard]] const Series& labels() const noexcept { return labels_; }
  [[nodiscard]] const Series& scores() const {
    if (!scores_.defined()) {
      ::pluss::throw_error("detect::Result::scores: this detector did not expose continuous scores.");
    }
    return scores_;
  }
  [[nodiscard]] bool has_scores() const noexcept { return scores_.defined(); }
  [[nodiscard]] double threshold() const noexcept { return threshold_; }
  [[nodiscard]] std::int64_t sample_count() const noexcept { return labels_.numel(); }

  [[nodiscard]] Series flagged() const {
    return labels_.reshape({labels_.numel()}).eq(-1.0);
  }

  [[nodiscard]] std::int64_t flagged_count() const {
    return flagged().to(ScalarType::Int64).sum().item<std::int64_t>();
  }

  [[nodiscard]] double flagged_rate() const {
    if (sample_count() == 0) return 0.0;
    return static_cast<double>(flagged_count()) / static_cast<double>(sample_count());
  }

  [[nodiscard]] pluss::table::DataFrame board() const {
    const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
    return pluss::table::DataFrame({
        {"metric", pluss::table::Thread(pluss::make_note({"sample_count", "flagged_count", "flagged_rate", "threshold"}))},
        {"value", pluss::table::Thread(pluss::make_series(std::vector<double>{
                                                static_cast<double>(sample_count()),
                                                static_cast<double>(flagged_count()),
                                                flagged_rate(),
                                                threshold_,
                                            },
                                            {4},
                                            cfg))},
    });
  }

 private:
  std::string method_{};
  Series labels_ = pluss::series::undefined();
  Series scores_ = pluss::series::undefined();
  double threshold_ = std::numeric_limits<double>::quiet_NaN();
  std::optional<Model> model_{std::nullopt};
};

class ModelRunner final {
 public:
  explicit ModelRunner(Model model) : model_(std::move(model)) {
    detail::require_anomaly_model_(model_, "detect::model");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }

  [[nodiscard]] Result run(const FeatureBatch& batch,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(batch, learn_options, infer_options);
    return Result("model", std::move(labels), pluss::series::undefined(), detail::model_threshold_(trained), std::move(trained));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(frame, data_options, learn_options, infer_options);
    return Result("model", std::move(labels), pluss::series::undefined(), detail::model_threshold_(trained), std::move(trained));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& x,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {}) const {
    Model trained = model_;
    Series labels = trained.learn_map(x, learn_options, infer_options);
    return Result("model", std::move(labels), pluss::series::undefined(), detail::model_threshold_(trained), std::move(trained));
  }

 private:
  Model model_;
};

[[nodiscard]] inline ModelRunner model(Model model) {
  return ModelRunner(std::move(model));
}

} // namespace pluss::ai::lymph::models::detect
