#pragma once

#include "pluss/ai/nodej/text.h"
#include "pluss/ai/nodej/options.h"
#include "pluss/ai/nodej/families.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <vector>

namespace pluss::ai::nodej {

struct SparseCsrMatrix {
  std::vector<std::int64_t> row_ptr{};
  std::vector<std::int64_t> col_ind{};
  std::vector<double> values{};
  std::int64_t rows = 0;
  std::int64_t cols = 0;
};

StatusOr<SparseTripletMatrix> make_sparse_triplet(std::int64_t rows,
                                                  std::int64_t cols,
                                                  pluss::msharp::RangeView::Int row_indices,
                                                  pluss::msharp::RangeView::Int column_indices,
                                                  pluss::msharp::RangeView::Of<double> values);
inline StatusOr<SparseTripletMatrix> make_sparse_triplet(
    std::int64_t rows,
    std::int64_t cols,
    std::initializer_list<std::int64_t> row_indices,
    std::initializer_list<std::int64_t> column_indices,
    std::initializer_list<double> values) {
  return make_sparse_triplet(rows,
                             cols,
                             pluss::msharp::RangeView::Int(row_indices),
                             pluss::msharp::RangeView::Int(column_indices),
                             pluss::msharp::RangeView::Of<double>(values));
}

StatusOr<SparseCsrMatrix> sparse_triplet_to_csr(const SparseTripletMatrix& sparse);

class SparseLinearBinaryClassifier final {
public:
  SparseLinearBinaryClassifier() = default;

  [[nodiscard]] bool trained() const noexcept { return trained_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return feature_count_; }

  Status learn(const SparseTripletMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});
  Status learn(const SparseCsrMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});

  [[nodiscard]] StatusOr<Series> infer_scores(const SparseTripletMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer_scores(const SparseCsrMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseTripletMatrix& x, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseCsrMatrix& x, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseTripletMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseCsrMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const;
  Status load_state(pluss::msharp::RangeView::Series state);

private:
  [[nodiscard]] StatusOr<std::vector<double>> infer_scores_cpu_(const SparseCsrMatrix& csr) const;

  bool trained_{false};
  std::int64_t feature_count_{0};
  std::vector<double> weights_{};
};

class SparseLinearRegressor final {
public:
  SparseLinearRegressor() = default;

  [[nodiscard]] bool trained() const noexcept { return trained_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return feature_count_; }

  Status learn(const SparseTripletMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});
  Status learn(const SparseCsrMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});

  [[nodiscard]] StatusOr<Series> infer(const SparseTripletMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseCsrMatrix& x) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseTripletMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseCsrMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const;
  Status load_state(pluss::msharp::RangeView::Series state);

private:
  [[nodiscard]] StatusOr<std::vector<double>> infer_cpu_(const SparseCsrMatrix& csr) const;

  bool trained_{false};
  std::int64_t feature_count_{0};
  std::vector<double> weights_{};
};

class SparseLinearMultiClassifier final {
public:
  SparseLinearMultiClassifier() = default;

  [[nodiscard]] bool trained() const noexcept { return trained_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return feature_count_; }
  [[nodiscard]] std::int64_t class_count() const noexcept {
    return static_cast<std::int64_t>(class_values_.size());
  }

  Status learn(const SparseTripletMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});
  Status learn(const SparseCsrMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});

  [[nodiscard]] StatusOr<Series> infer_scores(const SparseTripletMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer_scores(const SparseCsrMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseTripletMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseCsrMatrix& x) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseTripletMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseCsrMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const;
  Status load_state(pluss::msharp::RangeView::Series state);

private:
  [[nodiscard]] StatusOr<std::vector<double>> infer_scores_cpu_(const SparseCsrMatrix& csr) const;

  bool trained_{false};
  std::int64_t feature_count_{0};
  std::vector<double> class_values_{};
  std::vector<double> weights_{}; // flattened [class0 (bias+f), class1 (bias+f), ...]
};

struct SparseFoldReport {
  Series metrics = Series::undefined();
  Series train_metrics = Series::undefined();
  Series train_indices = Series::undefined();
  Series test_indices = Series::undefined();
  double fit_time_seconds = 0.0;
  double score_time_seconds = 0.0;
};

struct SparseCrossReport {
  std::vector<SparseFoldReport> folds{};
  std::vector<MetricKind> metrics{};
  Series mean_metrics = Series::undefined();
  Series std_metrics = Series::undefined();
  Series mean_train_metrics = Series::undefined();
  Series std_train_metrics = Series::undefined();
  Series fit_time_seconds = Series::undefined();
  Series score_time_seconds = Series::undefined();
};

enum class SparseObjectivePolicy : std::uint8_t {
  Auto = 0,
  Minimize = 1,
  Maximize = 2,
};

struct SparseTuneTrial {
  LinearFamilyOptions options{};
  Status status = Status::success();
  SparseCrossReport assess{};
  double objective = std::numeric_limits<double>::quiet_NaN();
};

struct SparseTuneReport {
  std::vector<SparseTuneTrial> trials{};
  std::int64_t best_index = -1;
  MetricKind objective_metric = MetricKind::Accuracy;
  SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto;
  std::vector<std::int64_t> objective_rank{};
};

StatusOr<SparseCrossReport> cross_assess_sparse_binary(const SparseTripletMatrix& x,
                                                       const Series& y,
                                                       const SplitOptions& split = {},
                                                       const TrainOptions& train_options = {},
                                                       const AssessOptions& assess_options = {},
                                                       const LinearFamilyOptions& linear_options = {});

StatusOr<SparseCrossReport> cross_assess_sparse_regression(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split = {},
                                                           const TrainOptions& train_options = {},
                                                           const AssessOptions& assess_options = {},
                                                           const LinearFamilyOptions& linear_options = {});

StatusOr<SparseCrossReport> cross_assess_sparse_multiclass(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split = {},
                                                           const TrainOptions& train_options = {},
                                                           const AssessOptions& assess_options = {},
                                                           const LinearFamilyOptions& linear_options = {});

StatusOr<SparseCrossReport> assess_sparse_binary_folds(const SparseTripletMatrix& x,
                                                       const Series& y,
                                                       const SplitOptions& split = {},
                                                       const TrainOptions& train_options = {},
                                                       const AssessOptions& assess_options = {},
                                                       const LinearFamilyOptions& linear_options = {});

StatusOr<SparseCrossReport> assess_sparse_regression_folds(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split = {},
                                                           const TrainOptions& train_options = {},
                                                           const AssessOptions& assess_options = {},
                                                           const LinearFamilyOptions& linear_options = {});

StatusOr<SparseCrossReport> assess_sparse_multiclass_folds(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split = {},
                                                           const TrainOptions& train_options = {},
                                                           const AssessOptions& assess_options = {},
                                                           const LinearFamilyOptions& linear_options = {});

StatusOr<SparseTuneReport> sweep_sparse_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                               const SparseTripletMatrix& x,
                                               const Series& y,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

StatusOr<SparseTuneReport> sweep_sparse_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                   const SparseTripletMatrix& x,
                                                   const Series& y,
                                                   const SplitOptions& split = {},
                                                   const TrainOptions& train_options = {},
                                                   const AssessOptions& assess_options = {},
                                                   MetricKind objective_metric = MetricKind::Mse,
                                                   SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

StatusOr<SparseTuneReport> sweep_sparse_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                   const SparseTripletMatrix& x,
                                                   const Series& y,
                                                   const SplitOptions& split = {},
                                                   const TrainOptions& train_options = {},
                                                   const AssessOptions& assess_options = {},
                                                   MetricKind objective_metric = MetricKind::Accuracy,
                                                   SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

StatusOr<SparseTuneReport> tune_sparse_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                              const SparseTripletMatrix& x,
                                              const Series& y,
                                              const SplitOptions& split = {},
                                              const TrainOptions& train_options = {},
                                              const AssessOptions& assess_options = {},
                                              MetricKind objective_metric = MetricKind::Accuracy,
                                              SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

inline StatusOr<SparseTuneReport> tune_sparse_binary(std::initializer_list<LinearFamilyOptions> candidates,
                                                     const SparseTripletMatrix& x,
                                                     const Series& y,
                                                     const SplitOptions& split = {},
                                                     const TrainOptions& train_options = {},
                                                     const AssessOptions& assess_options = {},
                                                     MetricKind objective_metric = MetricKind::Accuracy,
                                                     SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto) {
  return tune_sparse_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                            x,
                            y,
                            split,
                            train_options,
                            assess_options,
                            objective_metric,
                            objective_policy);
}

StatusOr<SparseTuneReport> tune_sparse_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                  const SparseTripletMatrix& x,
                                                  const Series& y,
                                                  const SplitOptions& split = {},
                                                  const TrainOptions& train_options = {},
                                                  const AssessOptions& assess_options = {},
                                                  MetricKind objective_metric = MetricKind::Mse,
                                                  SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

inline StatusOr<SparseTuneReport> tune_sparse_regression(std::initializer_list<LinearFamilyOptions> candidates,
                                                         const SparseTripletMatrix& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         MetricKind objective_metric = MetricKind::Mse,
                                                         SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto) {
  return tune_sparse_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                                x,
                                y,
                                split,
                                train_options,
                                assess_options,
                                objective_metric,
                                objective_policy);
}

StatusOr<SparseTuneReport> tune_sparse_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                  const SparseTripletMatrix& x,
                                                  const Series& y,
                                                  const SplitOptions& split = {},
                                                  const TrainOptions& train_options = {},
                                                  const AssessOptions& assess_options = {},
                                                  MetricKind objective_metric = MetricKind::Accuracy,
                                                  SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto);

inline StatusOr<SparseTuneReport> tune_sparse_multiclass(std::initializer_list<LinearFamilyOptions> candidates,
                                                         const SparseTripletMatrix& x,
                                                         const Series& y,
                                                         const SplitOptions& split = {},
                                                         const TrainOptions& train_options = {},
                                                         const AssessOptions& assess_options = {},
                                                         MetricKind objective_metric = MetricKind::Accuracy,
                                                         SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto) {
  return tune_sparse_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions>(candidates),
                                x,
                                y,
                                split,
                                train_options,
                                assess_options,
                                objective_metric,
                                objective_policy);
}

} // namespace pluss::ai::nodej
