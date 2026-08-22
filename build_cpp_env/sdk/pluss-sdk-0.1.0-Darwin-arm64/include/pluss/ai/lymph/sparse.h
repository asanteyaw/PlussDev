#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/metrics.h"

namespace pluss::ai::lymph {

namespace sparse {

using Triplet = ::pluss::ai::nodej::SparseTripletMatrix;
using Csr = ::pluss::ai::nodej::SparseCsrMatrix;
using FoldReport = ::pluss::ai::nodej::SparseFoldReport;
using CrossReport = ::pluss::ai::nodej::SparseCrossReport;
using ObjectivePolicy = ::pluss::ai::nodej::SparseObjectivePolicy;
using TuneTrial = ::pluss::ai::nodej::SparseTuneTrial;
using TuneLedger = ::pluss::ai::nodej::SparseTuneLedger;
using TuneReport = ::pluss::ai::nodej::SparseTuneReport;

[[nodiscard]] inline Triplet make_triplet(std::int64_t rows,
                                          std::int64_t cols,
                                          pluss::msharp::RangeView::Int row_indices,
                                          pluss::msharp::RangeView::Int column_indices,
                                          pluss::msharp::RangeView::Of<double> values) {
  return resolve(::pluss::ai::nodej::make_sparse_triplet(rows, cols, row_indices, column_indices, values),
                 "sparse::make_triplet");
}

[[nodiscard]] inline Triplet make_triplet(std::int64_t rows,
                                          std::int64_t cols,
                                          std::initializer_list<std::int64_t> row_indices,
                                          std::initializer_list<std::int64_t> column_indices,
                                          std::initializer_list<double> values) {
  return make_triplet(rows,
                      cols,
                      pluss::msharp::RangeView::Int(row_indices),
                      pluss::msharp::RangeView::Int(column_indices),
                      pluss::msharp::RangeView::Of<double>(values));
}

[[nodiscard]] inline Csr to_csr(const Triplet& sparse) {
  return resolve(::pluss::ai::nodej::sparse_triplet_to_csr(sparse), "sparse::to_csr");
}

class BinaryLinearModel final {
public:
  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return core_.feature_count(); }

  BinaryLinearModel& learn_(const Triplet& x,
                            const Series& y,
                            const TrainOptions& options = {},
                            const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::BinaryLinearModel.learn_");
    return *this;
  }

  BinaryLinearModel& learn_(const Csr& x,
                            const Series& y,
                            const TrainOptions& options = {},
                            const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::BinaryLinearModel.learn_");
    return *this;
  }

  [[nodiscard]] Series infer_scores(const Triplet& x) const {
    return resolve(core_.infer_scores(x), "sparse::BinaryLinearModel.infer_scores");
  }
  [[nodiscard]] Series infer_scores(const Csr& x) const {
    return resolve(core_.infer_scores(x), "sparse::BinaryLinearModel.infer_scores");
  }
  [[nodiscard]] Series infer(const Triplet& x, const InferOptions& options = {}) const {
    return resolve(core_.infer(x, options), "sparse::BinaryLinearModel.infer");
  }
  [[nodiscard]] Series infer(const Csr& x, const InferOptions& options = {}) const {
    return resolve(core_.infer(x, options), "sparse::BinaryLinearModel.infer");
  }
  [[nodiscard]] Series assess_series(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::BinaryLinearModel.assess_series");
  }
  [[nodiscard]] Series assess_series(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::BinaryLinearModel.assess_series");
  }
  [[nodiscard]] MetricReport assess(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Classification, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }
  [[nodiscard]] MetricReport assess(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Classification, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }

  [[nodiscard]] std::vector<Series> dump_state() const { return resolve(core_.dump_state(), "sparse::BinaryLinearModel.dump_state"); }
  BinaryLinearModel& load_state_(pluss::msharp::RangeView::Series state) {
    resolve(core_.load_state(state), "sparse::BinaryLinearModel.load_state_");
    return *this;
  }

private:
  ::pluss::ai::nodej::SparseLinearBinaryClassifier core_{};
};

class LinearRegressorModel final {
public:
  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return core_.feature_count(); }

  LinearRegressorModel& learn_(const Triplet& x,
                               const Series& y,
                               const TrainOptions& options = {},
                               const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::LinearRegressorModel.learn_");
    return *this;
  }

  LinearRegressorModel& learn_(const Csr& x,
                               const Series& y,
                               const TrainOptions& options = {},
                               const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::LinearRegressorModel.learn_");
    return *this;
  }

  [[nodiscard]] Series infer(const Triplet& x) const {
    return resolve(core_.infer(x), "sparse::LinearRegressorModel.infer");
  }
  [[nodiscard]] Series infer(const Csr& x) const {
    return resolve(core_.infer(x), "sparse::LinearRegressorModel.infer");
  }
  [[nodiscard]] Series assess_series(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::LinearRegressorModel.assess_series");
  }
  [[nodiscard]] Series assess_series(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::LinearRegressorModel.assess_series");
  }
  [[nodiscard]] MetricReport assess(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Regression, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }
  [[nodiscard]] MetricReport assess(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Regression, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }

  [[nodiscard]] std::vector<Series> dump_state() const {
    return resolve(core_.dump_state(), "sparse::LinearRegressorModel.dump_state");
  }
  LinearRegressorModel& load_state_(pluss::msharp::RangeView::Series state) {
    resolve(core_.load_state(state), "sparse::LinearRegressorModel.load_state_");
    return *this;
  }

private:
  ::pluss::ai::nodej::SparseLinearRegressor core_{};
};

class MultiLinearModel final {
public:
  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return core_.feature_count(); }
  [[nodiscard]] std::int64_t class_count() const noexcept { return core_.class_count(); }

  MultiLinearModel& learn_(const Triplet& x,
                           const Series& y,
                           const TrainOptions& options = {},
                           const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::MultiLinearModel.learn_");
    return *this;
  }

  MultiLinearModel& learn_(const Csr& x,
                           const Series& y,
                           const TrainOptions& options = {},
                           const LinearFamilyOptions& linear_options = {}) {
    resolve(core_.learn(x, y, options, linear_options), "sparse::MultiLinearModel.learn_");
    return *this;
  }

  [[nodiscard]] Series infer_scores(const Triplet& x) const {
    return resolve(core_.infer_scores(x), "sparse::MultiLinearModel.infer_scores");
  }
  [[nodiscard]] Series infer_scores(const Csr& x) const {
    return resolve(core_.infer_scores(x), "sparse::MultiLinearModel.infer_scores");
  }
  [[nodiscard]] Series infer(const Triplet& x) const {
    return resolve(core_.infer(x), "sparse::MultiLinearModel.infer");
  }
  [[nodiscard]] Series infer(const Csr& x) const {
    return resolve(core_.infer(x), "sparse::MultiLinearModel.infer");
  }
  [[nodiscard]] Series assess_series(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::MultiLinearModel.assess_series");
  }
  [[nodiscard]] Series assess_series(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    return resolve(core_.assess(x, y, options), "sparse::MultiLinearModel.assess_series");
  }
  [[nodiscard]] MetricReport assess(const Triplet& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Classification, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }
  [[nodiscard]] MetricReport assess(const Csr& x, const Series& y, const AssessOptions& options = {}) const {
    const auto metrics = default_assess_metrics_(TaskKind::Classification, options);
    return metric_report_from_series_(assess_series(x, y, options), metrics);
  }

  [[nodiscard]] std::vector<Series> dump_state() const {
    return resolve(core_.dump_state(), "sparse::MultiLinearModel.dump_state");
  }
  MultiLinearModel& load_state_(pluss::msharp::RangeView::Series state) {
    resolve(core_.load_state(state), "sparse::MultiLinearModel.load_state_");
    return *this;
  }

private:
  ::pluss::ai::nodej::SparseLinearMultiClassifier core_{};
};

[[nodiscard]] inline CrossReport cross_assess_binary(const Triplet& x,
                                                     const Series& y,
                                                     const SplitOptions& split = {},
                                                     const TrainOptions& train_options = {},
                                                     const AssessOptions& assess_options = {},
                                                     const LinearFamilyOptions& linear_options = {}) {
  return resolve(::pluss::ai::nodej::cross_assess_sparse_binary(x, y, split, train_options, assess_options, linear_options),
                 "sparse::cross_assess_binary");
}

[[nodiscard]] inline CrossReport cross_assess_regression(const Triplet& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         const LinearFamilyOptions& linear_options = {}) {
  return resolve(
      ::pluss::ai::nodej::cross_assess_sparse_regression(x, y, split, train_options, assess_options, linear_options),
      "sparse::cross_assess_regression");
}

[[nodiscard]] inline CrossReport cross_assess_multiclass(const Triplet& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         const LinearFamilyOptions& linear_options = {}) {
  return resolve(
      ::pluss::ai::nodej::cross_assess_sparse_multiclass(x, y, split, train_options, assess_options, linear_options),
                 "sparse::cross_assess_multiclass");
}

[[nodiscard]] inline CrossReport assess_binary_folds(const Triplet& x,
                                                     const Series& y,
                                                     const SplitOptions& split = {},
                                                     const TrainOptions& train_options = {},
                                                     const AssessOptions& assess_options = {},
                                                     const LinearFamilyOptions& linear_options = {}) {
  return resolve(::pluss::ai::nodej::assess_sparse_binary_folds(x, y, split, train_options, assess_options, linear_options),
                 "sparse::assess_binary_folds");
}

[[nodiscard]] inline CrossReport assess_regression_folds(const Triplet& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         const LinearFamilyOptions& linear_options = {}) {
  return resolve(
      ::pluss::ai::nodej::assess_sparse_regression_folds(x, y, split, train_options, assess_options, linear_options),
      "sparse::assess_regression_folds");
}

[[nodiscard]] inline CrossReport assess_multiclass_folds(const Triplet& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         const LinearFamilyOptions& linear_options = {}) {
  return resolve(
      ::pluss::ai::nodej::assess_sparse_multiclass_folds(x, y, split, train_options, assess_options, linear_options),
      "sparse::assess_multiclass_folds");
}

[[nodiscard]] inline TuneReport sweep_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                             const Triplet& x,
                                             const Series& y,
                                             const SplitOptions& split = {},
                                             const TrainOptions& train_options = {},
                                             const AssessOptions& assess_options = {},
                                             MetricKind objective_metric = MetricKind::Accuracy,
                                             ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::sweep_sparse_binary(candidates,
                                                         x,
                                                         y,
                                                         split,
                                                         train_options,
                                                         assess_options,
                                                         objective_metric,
                                                         objective_policy),
                 "sparse::sweep_binary");
}

[[nodiscard]] inline TuneReport sweep_binary(std::initializer_list<LinearFamilyOptions> candidates,
                                             const Triplet& x,
                                             const Series& y,
                                             const SplitOptions& split = {},
                                             const TrainOptions& train_options = {},
                                             const AssessOptions& assess_options = {},
                                             MetricKind objective_metric = MetricKind::Accuracy,
                                             ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return sweep_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                      x,
                      y,
                      split,
                      train_options,
                      assess_options,
                      objective_metric,
                      objective_policy);
}

[[nodiscard]] inline TuneReport sweep_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                 const Triplet& x,
                                                 const Series& y,
                                                 const SplitOptions& split = {},
                                                 const TrainOptions& train_options = {},
                                                 const AssessOptions& assess_options = {},
                                                 MetricKind objective_metric = MetricKind::Mse,
                                                 ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::sweep_sparse_regression(candidates,
                                                             x,
                                                             y,
                                                             split,
                                                             train_options,
                                                             assess_options,
                                                             objective_metric,
                                                             objective_policy),
                 "sparse::sweep_regression");
}

[[nodiscard]] inline TuneReport sweep_regression(std::initializer_list<LinearFamilyOptions> candidates,
                                                 const Triplet& x,
                                                 const Series& y,
                                                 const SplitOptions& split = {},
                                                 const TrainOptions& train_options = {},
                                                 const AssessOptions& assess_options = {},
                                                 MetricKind objective_metric = MetricKind::Mse,
                                                 ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return sweep_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                          x,
                          y,
                          split,
                          train_options,
                          assess_options,
                          objective_metric,
                          objective_policy);
}

[[nodiscard]] inline TuneReport sweep_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                 const Triplet& x,
                                                 const Series& y,
                                                 const SplitOptions& split = {},
                                                 const TrainOptions& train_options = {},
                                                 const AssessOptions& assess_options = {},
                                                 MetricKind objective_metric = MetricKind::Accuracy,
                                                 ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::sweep_sparse_multiclass(candidates,
                                                             x,
                                                             y,
                                                             split,
                                                             train_options,
                                                             assess_options,
                                                             objective_metric,
                                                             objective_policy),
                 "sparse::sweep_multiclass");
}

[[nodiscard]] inline TuneReport sweep_multiclass(std::initializer_list<LinearFamilyOptions> candidates,
                                                 const Triplet& x,
                                                 const Series& y,
                                                 const SplitOptions& split = {},
                                                 const TrainOptions& train_options = {},
                                                 const AssessOptions& assess_options = {},
                                                 MetricKind objective_metric = MetricKind::Accuracy,
                                                 ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return sweep_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                          x,
                          y,
                          split,
                          train_options,
                          assess_options,
                          objective_metric,
                          objective_policy);
}

[[nodiscard]] inline TuneReport tune_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                            const Triplet& x,
                                            const Series& y,
                                            const SplitOptions& split = {},
                                            const TrainOptions& train_options = {},
                                            const AssessOptions& assess_options = {},
                                            MetricKind objective_metric = MetricKind::Accuracy,
                                            ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::tune_sparse_binary(candidates,
                                                        x,
                                                        y,
                                                        split,
                                                        train_options,
                                                        assess_options,
                                                        objective_metric,
                                                        objective_policy),
                 "sparse::tune_binary");
}

[[nodiscard]] inline TuneReport tune_binary(std::initializer_list<LinearFamilyOptions> candidates,
                                            const Triplet& x,
                                            const Series& y,
                                            const SplitOptions& split = {},
                                            const TrainOptions& train_options = {},
                                            const AssessOptions& assess_options = {},
                                            MetricKind objective_metric = MetricKind::Accuracy,
                                            ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return tune_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                     x,
                     y,
                     split,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy);
}

[[nodiscard]] inline TuneReport tune_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                const Triplet& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Mse,
                                                ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::tune_sparse_regression(candidates,
                                                            x,
                                                            y,
                                                            split,
                                                            train_options,
                                                            assess_options,
                                                            objective_metric,
                                                            objective_policy),
                 "sparse::tune_regression");
}

[[nodiscard]] inline TuneReport tune_regression(std::initializer_list<LinearFamilyOptions> candidates,
                                                const Triplet& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Mse,
                                                ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return tune_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                         x,
                         y,
                         split,
                         train_options,
                         assess_options,
                         objective_metric,
                         objective_policy);
}

[[nodiscard]] inline TuneReport tune_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                const Triplet& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return resolve(::pluss::ai::nodej::tune_sparse_multiclass(candidates,
                                                            x,
                                                            y,
                                                            split,
                                                            train_options,
                                                            assess_options,
                                                            objective_metric,
                                                            objective_policy),
                 "sparse::tune_multiclass");
}

[[nodiscard]] inline TuneReport tune_multiclass(std::initializer_list<LinearFamilyOptions> candidates,
                                                const Triplet& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                ObjectivePolicy objective_policy = ObjectivePolicy::Auto) {
  return tune_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                         x,
                         y,
                         split,
                         train_options,
                         assess_options,
                         objective_metric,
                         objective_policy);
}

[[nodiscard]] inline bool has_best_trial(const TuneReport& report) {
  return report.best_index >= 0 && report.best_index < static_cast<std::int64_t>(report.trials.size());
}

[[nodiscard]] inline const TuneTrial& best_trial(const TuneReport& report) {
  if (!has_best_trial(report)) {
    ::pluss::throw_error("sparse::best_trial requested but report has no best trial.");
  }
  return report.trials[static_cast<std::size_t>(report.best_index)];
}

[[nodiscard]] inline Series objective_series(const TuneReport& report) {
  if (report.ledger.objective.defined()) return report.ledger.objective;
  std::vector<double> values{};
  values.reserve(report.trials.size());
  for (const auto& trial : report.trials) values.push_back(trial.objective);
  return pluss::make_series(std::move(values),
                            {static_cast<std::int64_t>(report.trials.size())},
                            Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
}

[[nodiscard]] inline Series objective_rank_series(const TuneReport& report) {
  if (report.ledger.objective_rank.defined()) return report.ledger.objective_rank;
  std::vector<std::int64_t> values{};
  values.reserve(report.objective_rank.size());
  for (const auto rank : report.objective_rank) values.push_back(rank);
  return pluss::make_series(std::move(values),
                            {static_cast<std::int64_t>(report.objective_rank.size())},
                            Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
}

[[nodiscard]] inline bool has_ledger(const TuneReport& report) {
  return report.ledger.objective.defined();
}

[[nodiscard]] inline const TuneLedger& tune_ledger(const TuneReport& report) {
  if (!has_ledger(report)) {
    ::pluss::throw_error("sparse::tune_ledger: tune report does not include ledger artifacts.");
  }
  return report.ledger;
}

[[nodiscard]] inline Series encoded_param_table(const TuneReport& report) {
  return tune_ledger(report).encoded_params;
}

} // namespace sparse

} // namespace pluss::ai::lymph
