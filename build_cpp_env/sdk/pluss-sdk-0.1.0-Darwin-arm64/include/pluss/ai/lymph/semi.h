#pragma once

#include "pluss/ai/lymph/model.h"

#include <limits>

namespace pluss::ai::lymph::models::semi {

enum class SelectKind : std::uint8_t {
  Confidence = 0,
  TopK = 1,
};

enum class StopReason : std::uint8_t {
  None = 0,
  AllLabeled = 1,
  NoChange = 2,
  MaxRounds = 3,
};

struct Options {
  double unlabeled_label = -1.0;
  SelectKind select = SelectKind::Confidence;
  double confidence = 0.75;
  std::int64_t top_k = 10;
  // Negative means keep going until all rows are labeled or no row changes.
  std::int64_t max_rounds = 10;
  bool final_refit = true;
};

namespace detail {

inline void require_classifier_(const Model& model, std::string_view where) {
  if (model.task_kind() != TaskKind::Classification) {
    ::pluss::throw_error(std::string(where) + " requires a classification Model.");
  }
}

inline void validate_options_(const Options& options) {
  if (!std::isfinite(options.unlabeled_label)) {
    ::pluss::throw_error("semi::Options.unlabeled_label must be finite.");
  }
  if (!std::isfinite(options.confidence) || options.confidence < 0.0 || options.confidence >= 1.0) {
    ::pluss::throw_error("semi::Options.confidence must be finite and in [0, 1).");
  }
  if (options.top_k <= 0) {
    ::pluss::throw_error("semi::Options.top_k must be positive.");
  }
  if (options.max_rounds < -1) {
    ::pluss::throw_error("semi::Options.max_rounds must be -1 or non-negative.");
  }
}

[[nodiscard]] inline Series index_series_(std::int64_t n, ::pluss::Platform platform) {
  return pluss::series::arange(0, n, 1, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

[[nodiscard]] inline LabeledBatch batch_with_labels_(const LabeledBatch& batch, const Series& labels) {
  LabeledBatch out = batch;
  out.y = labels.reshape({labels.numel()}).to(batch.y.platform(), batch.y.scalar_type());
  return out;
}

[[nodiscard]] inline Series score_confidence_(const Series& raw_scores) {
  if (!raw_scores.defined()) {
    ::pluss::throw_error("semi::model(...).run requires classifiers that expose scores.");
  }
  if (raw_scores.dim() == 2 && raw_scores.sizes().size() == 2 && raw_scores.sizes()[1] > 1) {
    const std::int64_t n = raw_scores.sizes()[0];
    const std::int64_t c = raw_scores.sizes()[1];
    const Series scores = raw_scores.to(ScalarType::Float64).reshape({n, c});
    const Series shifted = scores.sub(scores.amax(1, true));
    const Series expv = shifted.exp();
    return expv.div(expv.sum(1, true).clamp_min(1e-12)).amax(1, false).reshape({n});
  }

  const Series scores = raw_scores.to(ScalarType::Float64).reshape({raw_scores.numel()});
  const double lo = scores.amin().reshape({1}).item<double>();
  const double hi = scores.amax().reshape({1}).item<double>();
  const Series prob = (std::isfinite(lo) && std::isfinite(hi) && lo >= 0.0 && hi <= 1.0)
                          ? scores
                          : scores.sigmoid();
  return prob.maximum(prob.mul(-1.0).add(1.0)).reshape({scores.numel()});
}

[[nodiscard]] inline LabeledBatch slice_labeled_(const LabeledBatch& batch, const Series& indices, std::string_view where) {
  return resolve(::pluss::ai::nodej::slice_labeled_batch(batch, indices), where);
}

[[nodiscard]] inline FeatureBatch slice_features_(const FeatureBatch& batch, const Series& indices, std::string_view where) {
  return resolve(::pluss::ai::nodej::slice_feature_batch(batch, indices), where);
}

} // namespace detail

class Result final {
 public:
  Result(Model model,
         Series labels,
         Series labeled_round,
         std::int64_t rounds,
         StopReason stop)
      : model_(std::move(model)),
        labels_(std::move(labels)),
        labeled_round_(std::move(labeled_round)),
        rounds_(rounds),
        stop_(stop) {}

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] const Series& labels() const noexcept { return labels_; }
  [[nodiscard]] const Series& labeled_round() const noexcept { return labeled_round_; }
  [[nodiscard]] std::int64_t rounds() const noexcept { return rounds_; }
  [[nodiscard]] StopReason stop() const noexcept { return stop_; }

  [[nodiscard]] std::int64_t sample_count() const noexcept { return labels_.numel(); }

  [[nodiscard]] std::int64_t labeled_count() const {
    return labeled_round_.reshape({labeled_round_.numel()}).ge(0.0).to(ScalarType::Int64).sum().item<std::int64_t>();
  }

  [[nodiscard]] std::int64_t pseudo_labeled_count() const {
    return labeled_round_.reshape({labeled_round_.numel()}).gt(0.0).to(ScalarType::Int64).sum().item<std::int64_t>();
  }

  [[nodiscard]] pluss::table::DataFrame board() const {
    const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
    return pluss::table::DataFrame({
        {"metric", pluss::table::Thread(pluss::make_note({"sample_count", "labeled_count", "pseudo_labeled_count", "rounds"}))},
        {"value", pluss::table::Thread(pluss::make_series(std::vector<double>{
                                                static_cast<double>(sample_count()),
                                                static_cast<double>(labeled_count()),
                                                static_cast<double>(pseudo_labeled_count()),
                                                static_cast<double>(rounds_),
                                            },
                                            {4},
                                            cfg))},
    });
  }

 private:
  Model model_;
  Series labels_ = Series::undefined();
  Series labeled_round_ = Series::undefined();
  std::int64_t rounds_{0};
  StopReason stop_{StopReason::None};
};

class Runner final {
 public:
  explicit Runner(Model model) : model_(std::move(model)) {
    detail::require_classifier_(model_, "semi::model");
  }

  [[nodiscard]] const Model& model() const noexcept { return model_; }

  [[nodiscard]] Result run(const LabeledBatch& batch,
                           Options options = {},
                           const TrainOptions& train_options = {},
                           const InferOptions& infer_options = {}) const {
    detail::validate_options_(options);
    resolve(::pluss::ai::nodej::validate_labeled(batch), "semi::model(...).run.validate_labeled");

    const std::int64_t n = batch.features.sample_count();
    if (n == 0) {
      ::pluss::throw_error("semi::model(...).run requires at least one sample.");
    }

    const ::pluss::Platform label_platform = batch.y.platform();
    const Series all_idx = detail::index_series_(n, label_platform);
    Series labels = batch.y.to(ScalarType::Float64).reshape({n});
    Series has_label = labels.ne(options.unlabeled_label).reshape({n});
    if (has_label.to(ScalarType::Int64).sum().item<std::int64_t>() == 0) {
      ::pluss::throw_error("semi::model(...).run requires at least one initially labeled sample.");
    }

    Series labeled_round = pluss::series::full({n}, -1.0, Config{}.with_dtype(ScalarType::Int64).with_platform(label_platform));
    labeled_round.index_put_({all_idx.masked_select(has_label).to(label_platform, ScalarType::Int64)},
                             pluss::series::zeros({has_label.to(ScalarType::Int64).sum().item<std::int64_t>()},
                                                  Config{}.with_dtype(ScalarType::Int64).with_platform(label_platform)),
                             false);

    Model trained = model_;
    StopReason stop = StopReason::None;
    std::int64_t rounds = 0;

    while (!has_label.all().item<bool>()
           && (options.max_rounds < 0 || rounds < options.max_rounds)) {
      ++rounds;

      const Series labeled_idx = all_idx.masked_select(has_label).to(label_platform, ScalarType::Int64);
      const LabeledBatch learn_batch =
          detail::slice_labeled_(detail::batch_with_labels_(batch, labels), labeled_idx, "semi::model(...).run.labeled_slice");
      trained = model_.learn(learn_batch, train_options);

      const Series missing_mask = has_label.logical_not().reshape({n});
      const Series unlabeled_idx = all_idx.masked_select(missing_mask).to(label_platform, ScalarType::Int64);
      const FeatureBatch query =
          detail::slice_features_(batch.features, unlabeled_idx, "semi::model(...).run.unlabeled_slice");
      const Series predicted = trained.infer(query, infer_options).reshape({unlabeled_idx.numel()});
      const Series confidence = detail::score_confidence_(trained.infer_scores(query, infer_options));

      Series selected_full = Series::undefined();
      Series selected_pred = Series::undefined();
      if (options.select == SelectKind::Confidence) {
        const Series selected_mask = confidence.gt(options.confidence).reshape({confidence.numel()});
        selected_full = unlabeled_idx.masked_select(selected_mask).to(label_platform, ScalarType::Int64);
        selected_pred = predicted.masked_select(selected_mask);
      } else {
        const std::int64_t take_n = std::min<std::int64_t>(options.top_k, confidence.numel());
        const Series order = confidence.argsort(0, true, true).reshape({confidence.numel()});
        const Series take =
            order.index_select(0, detail::index_series_(take_n, order.platform())).reshape({take_n}).to(label_platform, ScalarType::Int64);
        selected_full = unlabeled_idx.index_select(0, take).reshape({take_n});
        selected_pred = predicted.index_select(0, take.to(predicted.platform(), ScalarType::Int64)).reshape({take_n});
      }

      if (selected_full.numel() == 0) {
        stop = StopReason::NoChange;
        break;
      }

      labels.index_put_({selected_full.to(labels.platform(), ScalarType::Int64)},
                        selected_pred.to(labels.platform(), ScalarType::Float64).reshape({selected_full.numel()}),
                        false);
      labeled_round.index_put_({selected_full.to(label_platform, ScalarType::Int64)},
                               pluss::series::full({selected_full.numel()},
                                                   static_cast<double>(rounds),
                                                   Config{}.with_dtype(ScalarType::Int64).with_platform(label_platform)),
                               false);
      has_label = labels.ne(options.unlabeled_label).reshape({n});
    }

    if (has_label.all().item<bool>()) {
      stop = StopReason::AllLabeled;
    } else if (stop == StopReason::None) {
      stop = StopReason::MaxRounds;
    }

    if (options.final_refit || !trained.trained()) {
      const Series final_idx = all_idx.masked_select(has_label).to(label_platform, ScalarType::Int64);
      const LabeledBatch final_batch =
          detail::slice_labeled_(detail::batch_with_labels_(batch, labels), final_idx, "semi::model(...).run.final_slice");
      trained = model_.learn(final_batch, train_options);
    }

    return Result(std::move(trained),
                  labels.to(batch.y.platform(), batch.y.scalar_type()).reshape({n}),
                  labeled_round.reshape({n}),
                  rounds,
                  stop);
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           Options options = {},
                           const TrainOptions& train_options = {},
                           const InferOptions& infer_options = {}) const {
    const LabeledBatch batch =
        resolve(labeled_batch_from_dataframe(frame, data_options), "semi::model(...).run.labeled_batch_from_dataframe");
    return run(batch, options, train_options, infer_options);
  }

 private:
  Model model_;
};

[[nodiscard]] inline Runner model(Model model) {
  return Runner(std::move(model));
}

} // namespace pluss::ai::lymph::models::semi
