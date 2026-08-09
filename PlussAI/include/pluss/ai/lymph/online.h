#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/metrics.h"

namespace pluss::ai::lymph {


class OnlineLinearClassifier final {
public:
  explicit OnlineLinearClassifier(OnlineLinearOptions options = {})
      : core_(resolve(::pluss::ai::nodej::online::make_online_linear_classifier(options), "online.make_online_linear_classifier")) {}

  explicit OnlineLinearClassifier(::pluss::ai::nodej::online::OnlineLinearClassifier core)
      : core_(std::move(core)) {}

  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t seen_samples() const noexcept { return core_.seen_samples(); }

  OnlineLinearClassifier& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.learn(batch, options), "online.learn");
    return *this;
  }

  OnlineLinearClassifier& learn_(const pluss::table::DataFrame& x,
                                 const Series& y,
                                 const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  OnlineLinearClassifier& learn_(const pluss::table::DataFrame& x,
                                 const pluss::table::Thread& y,
                                 const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  OnlineLinearClassifier& advance_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.advance(batch, options), "online.advance");
    return *this;
  }

  OnlineLinearClassifier& advance_(const pluss::table::DataFrame& x,
                                   const Series& y,
                                   const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.advance_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.advance_(x,y)");
    feature_names_ = names;
    return advance_(batch, options);
  }

  OnlineLinearClassifier& advance_(const pluss::table::DataFrame& x,
                                   const pluss::table::Thread& y,
                                   const TrainOptions& options = {}) {
    return advance_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer_scores(batch, options), "online.infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x,
                                    const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer_scores(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer_scores(x)");
    return infer_scores(batch, options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer(batch, options), "online.infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer(x)");
    return infer(batch, options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(core_.assess(batch, options), "online.assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.assess_series(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.assess_series(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(batch, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] const ::pluss::ai::nodej::online::OnlineLinearClassifier& core() const noexcept { return core_; }
  [[nodiscard]] ::pluss::ai::nodej::online::OnlineLinearClassifier& core() noexcept { return core_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(pluss::msharp::RangeView::Of<std::string> feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = copy_range_values(feature_names);
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.feataxis().series().features();
    if (names.empty()) {
      ::pluss::throw_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static FeatureBatch feature_from_x_(const pluss::table::DataFrame& x,
                                                    pluss::msharp::RangeView::Of<std::string> names,
                                                    const char* where) {
    return resolve(feature_batch_from_dataframe(x, default_data_options_(names)), std::string(where) + ".feature_batch_from_dataframe");
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     pluss::msharp::RangeView::Of<std::string> names,
                                                     const char* where) {
    const FeatureBatch x_batch = feature_from_x_(x, names, where);
    LabeledBatch batch{
        .features = x_batch,
        .y = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()}),
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  ::pluss::ai::nodej::online::OnlineLinearClassifier core_{};
  std::vector<std::string> feature_names_{};
};

class OnlineLinearRegressor final {
public:
  explicit OnlineLinearRegressor(OnlineLinearOptions options = {})
      : core_(resolve(::pluss::ai::nodej::online::make_online_linear_regressor(options), "online.make_online_linear_regressor")) {}

  explicit OnlineLinearRegressor(::pluss::ai::nodej::online::OnlineLinearRegressor core)
      : core_(std::move(core)) {}

  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t seen_samples() const noexcept { return core_.seen_samples(); }

  OnlineLinearRegressor& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.learn(batch, options), "online.learn");
    return *this;
  }

  OnlineLinearRegressor& learn_(const pluss::table::DataFrame& x,
                                const Series& y,
                                const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  OnlineLinearRegressor& learn_(const pluss::table::DataFrame& x,
                                const pluss::table::Thread& y,
                                const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  OnlineLinearRegressor& advance_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.advance(batch, options), "online.advance");
    return *this;
  }

  OnlineLinearRegressor& advance_(const pluss::table::DataFrame& x,
                                  const Series& y,
                                  const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.advance_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.advance_(x,y)");
    feature_names_ = names;
    return advance_(batch, options);
  }

  OnlineLinearRegressor& advance_(const pluss::table::DataFrame& x,
                                  const pluss::table::Thread& y,
                                  const TrainOptions& options = {}) {
    return advance_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer(batch, options), "online.infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer(x)");
    return infer(batch, options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(core_.assess(batch, options), "online.assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.assess_series(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.assess_series(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(batch, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] const ::pluss::ai::nodej::online::OnlineLinearRegressor& core() const noexcept { return core_; }
  [[nodiscard]] ::pluss::ai::nodej::online::OnlineLinearRegressor& core() noexcept { return core_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(pluss::msharp::RangeView::Of<std::string> feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = copy_range_values(feature_names);
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.feataxis().series().features();
    if (names.empty()) {
      ::pluss::throw_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static FeatureBatch feature_from_x_(const pluss::table::DataFrame& x,
                                                    pluss::msharp::RangeView::Of<std::string> names,
                                                    const char* where) {
    return resolve(feature_batch_from_dataframe(x, default_data_options_(names)), std::string(where) + ".feature_batch_from_dataframe");
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     pluss::msharp::RangeView::Of<std::string> names,
                                                     const char* where) {
    const FeatureBatch x_batch = feature_from_x_(x, names, where);
    LabeledBatch batch{
        .features = x_batch,
        .y = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()}),
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  ::pluss::ai::nodej::online::OnlineLinearRegressor core_{};
  std::vector<std::string> feature_names_{};
};

namespace models {

[[nodiscard]] inline OnlineLinearClassifier online_linear_classifier(OnlineLinearOptions options = {}) {
  return OnlineLinearClassifier(options);
}

[[nodiscard]] inline OnlineLinearRegressor online_linear_regressor(OnlineLinearOptions options = {}) {
  return OnlineLinearRegressor(options);
}

} // namespace models

} // namespace pluss::ai::lymph
