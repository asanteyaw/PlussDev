#include "pluss/ai/nodej/sparse_runtime.h"
#include "pluss/ai/nodej/split.h"

#include "pluss/codem/series_factory.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] bool finite_all(const std::vector<double>& v) {
  for (double x : v) {
    if (!std::isfinite(x)) return false;
  }
  return true;
}

[[nodiscard]] double sigmoid(double x) {
  if (x >= 0.0) {
    const double e = std::exp(-x);
    return 1.0 / (1.0 + e);
  }
  const double e = std::exp(x);
  return e / (1.0 + e);
}

[[nodiscard]] std::vector<double> series_f64(const Series& y) {
  return y.to(pluss::pCPU, ScalarType::Float64).reshape({y.numel()}).to_vec<double>();
}

[[nodiscard]] double sparse_row_linear_score(const SparseCsrMatrix& csr,
                                             std::int64_t row,
                                             const std::vector<double>& weights,
                                             std::int64_t class_offset) {
  const std::int64_t stride = csr.cols + 1;
  const std::int64_t b = csr.row_ptr[static_cast<std::size_t>(row)];
  const std::int64_t e = csr.row_ptr[static_cast<std::size_t>(row + 1)];
  double z = weights[static_cast<std::size_t>(class_offset * stride)];
  for (std::int64_t p = b; p < e; ++p) {
    const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
    z += weights[static_cast<std::size_t>(class_offset * stride + c + 1)] * csr.values[static_cast<std::size_t>(p)];
  }
  return z;
}

[[nodiscard]] StatusOr<std::vector<double>> train_logistic_binary_weights(const SparseCsrMatrix& csr,
                                                                          const std::vector<double>& y01,
                                                                          const TrainOptions& options,
                                                                          const LinearFamilyOptions& linear_options,
                                                                          const char* where) {
  if (static_cast<std::int64_t>(y01.size()) != csr.rows) {
    return StatusOr<std::vector<double>>(Status::shape_mismatch(std::string(where) + " label count must match sparse rows."));
  }
  std::vector<double> weights(static_cast<std::size_t>(csr.cols + 1), 0.0); // bias + weights
  const double lr = 0.05;
  const double l2 = linear_options.regularization == RegularizationKind::L2 ? linear_options.regularization_strength : 0.0;
  double prev_loss = std::numeric_limits<double>::infinity();

  for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
    std::vector<double> grad(static_cast<std::size_t>(csr.cols + 1), 0.0);
    double loss_sum = 0.0;
    for (std::int64_t r = 0; r < csr.rows; ++r) {
      const std::int64_t b = csr.row_ptr[static_cast<std::size_t>(r)];
      const std::int64_t e = csr.row_ptr[static_cast<std::size_t>(r + 1)];
      double z = weights[0];
      for (std::int64_t p = b; p < e; ++p) {
        const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
        z += weights[static_cast<std::size_t>(c + 1)] * csr.values[static_cast<std::size_t>(p)];
      }
      const double p1 = sigmoid(z);
      const double yi = y01[static_cast<std::size_t>(r)];
      const double err = p1 - yi;
      grad[0] += err;
      for (std::int64_t p = b; p < e; ++p) {
        const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
        grad[static_cast<std::size_t>(c + 1)] += err * csr.values[static_cast<std::size_t>(p)];
      }
      loss_sum += -(yi * std::log(std::max(p1, 1e-12)) + (1.0 - yi) * std::log(std::max(1.0 - p1, 1e-12)));
    }
    const double n = static_cast<double>(csr.rows);
    for (std::size_t j = 0; j < grad.size(); ++j) {
      grad[j] /= n;
      if (j > 0 && l2 > 0.0) grad[j] += l2 * weights[j];
      weights[j] -= lr * grad[j];
    }
    const double loss = loss_sum / n;
    if (!std::isfinite(loss)) {
      return StatusOr<std::vector<double>>(Status::numerical_failure(std::string(where) + " encountered non-finite objective."));
    }
    if (std::abs(prev_loss - loss) <= options.tolerance) break;
    prev_loss = loss;
  }

  if (!finite_all(weights)) {
    return StatusOr<std::vector<double>>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
  }
  return StatusOr<std::vector<double>>(std::move(weights));
}

[[nodiscard]] std::vector<double> sorted_unique_values(std::vector<double> values) {
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

[[nodiscard]] std::vector<MetricKind> default_sparse_metrics(TaskKind task) {
  if (task == TaskKind::Classification) return {MetricKind::Accuracy};
  return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
}

[[nodiscard]] bool metric_prefers_max(MetricKind metric) {
  switch (metric) {
    case MetricKind::Accuracy:
    case MetricKind::BalancedAccuracy:
    case MetricKind::Precision:
    case MetricKind::Recall:
    case MetricKind::F1:
    case MetricKind::RocAuc:
    case MetricKind::R2:
      return true;
    case MetricKind::LogLoss:
    case MetricKind::Mse:
    case MetricKind::Rmse:
    case MetricKind::Mae:
    case MetricKind::QuantileLoss:
      return false;
  }
  return true;
}

[[nodiscard]] bool maximize_sparse_objective(SparseObjectivePolicy policy, MetricKind metric) {
  if (policy == SparseObjectivePolicy::Maximize) return true;
  if (policy == SparseObjectivePolicy::Minimize) return false;
  return metric_prefers_max(metric);
}

[[nodiscard]] SplitOptions resolved_sparse_split_defaults(TaskKind task, SplitOptions split) {
  if (!split.reliability_defaults) return split;
  if (task != TaskKind::Classification) return split;
  if (!split.stratified && (split.plan == CVPlanKind::Holdout || split.plan == CVPlanKind::KFold)) {
    split.stratified = true;
  }
  return split;
}

[[nodiscard]] StatusOr<SplitPlan> make_sparse_cross_plan(TaskKind task,
                                                         const Series& y,
                                                         std::int64_t sample_count,
                                                         const SplitOptions& split) {
  if (split.plan == CVPlanKind::Holdout) {
    StatusOr<IndexSplit> holdout_or =
        (split.stratified && task == TaskKind::Classification)
            ? strat_holdout_split(y, split)
            : holdout_split(sample_count, split);
    if (!holdout_or.ok()) return StatusOr<SplitPlan>(holdout_or.status());
    SplitPlan plan{};
    plan.folds.push_back(holdout_or.move_value());
    return StatusOr<SplitPlan>(std::move(plan));
  }
  if (split.plan == CVPlanKind::KFold) {
    if (split.stratified && task == TaskKind::Classification) return strat_fold_plan(y, split);
    return fold_plan(sample_count, split);
  }
  if (split.plan == CVPlanKind::StratifiedKFold) {
    if (task != TaskKind::Classification) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("sparse cross assess StratifiedKFold requires classification task."));
    }
    return strat_fold_plan(y, split);
  }
  if (split.plan == CVPlanKind::TimeSeries) {
    return time_series_fold_plan(sample_count, split);
  }
  if (split.plan == CVPlanKind::GroupKFold) {
    if (!split.groups.defined()) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("sparse cross assess GroupKFold requires SplitOptions.groups."));
    }
    if (split.groups.numel() != sample_count) {
      return StatusOr<SplitPlan>(
          Status::shape_mismatch("sparse cross assess GroupKFold requires groups length to match sample count."));
    }
    return group_fold_plan(split.groups, split);
  }
  return StatusOr<SplitPlan>(Status::invalid_argument("sparse cross assess received unknown SplitOptions.plan."));
}

[[nodiscard]] std::vector<std::int64_t> series_i64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).to_vec<std::int64_t>();
}

[[nodiscard]] SparseCsrMatrix csr_slice_rows(const SparseCsrMatrix& src,
                                             const std::vector<std::int64_t>& row_ids) {
  SparseCsrMatrix out{};
  out.rows = static_cast<std::int64_t>(row_ids.size());
  out.cols = src.cols;
  out.row_ptr.assign(static_cast<std::size_t>(out.rows + 1), 0);
  std::int64_t nnz = 0;
  for (std::int64_t rid : row_ids) {
    nnz += src.row_ptr[static_cast<std::size_t>(rid + 1)] - src.row_ptr[static_cast<std::size_t>(rid)];
  }
  out.col_ind.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, nnz)));
  out.values.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, nnz)));
  for (std::size_t i = 0; i < row_ids.size(); ++i) {
    const std::int64_t rid = row_ids[i];
    const std::int64_t b = src.row_ptr[static_cast<std::size_t>(rid)];
    const std::int64_t e = src.row_ptr[static_cast<std::size_t>(rid + 1)];
    out.row_ptr[i + 1] = out.row_ptr[i] + (e - b);
    for (std::int64_t p = b; p < e; ++p) {
      out.col_ind.push_back(src.col_ind[static_cast<std::size_t>(p)]);
      out.values.push_back(src.values[static_cast<std::size_t>(p)]);
    }
  }
  return out;
}

[[nodiscard]] Status with_sparse_fold_context(Status status,
                                              std::int64_t fold_index,
                                              std::int64_t fold_count,
                                              std::int64_t train_samples,
                                              std::int64_t test_samples) {
  if (status.ok()) return status;
  const std::string prefix = "sparse cross assess fold "
                             + std::to_string(fold_index + 1) + "/" + std::to_string(fold_count)
                             + " (train=" + std::to_string(train_samples)
                             + ", test=" + std::to_string(test_samples) + "): ";
  status.message = prefix + status.message;
  return status;
}

} // namespace

StatusOr<SparseTripletMatrix> make_sparse_triplet(std::int64_t rows,
                                                  std::int64_t cols,
                                                  pluss::msharp::RangeView::Int row_indices,
                                                  pluss::msharp::RangeView::Int column_indices,
                                                  pluss::msharp::RangeView::Of<double> values) {
  if (rows <= 0 || cols <= 0) {
    return StatusOr<SparseTripletMatrix>(Status::invalid_argument("make_sparse_triplet requires rows>0 and cols>0."));
  }
  if (row_indices.size() != column_indices.size() || row_indices.size() != values.size()) {
    return StatusOr<SparseTripletMatrix>(Status::shape_mismatch("make_sparse_triplet index/value lengths must match."));
  }
  std::vector<std::int64_t> r{};
  std::vector<std::int64_t> c{};
  std::vector<double> v{};
  r.reserve(row_indices.size());
  c.reserve(column_indices.size());
  v.reserve(values.size());
  for (std::size_t i = 0; i < row_indices.size(); ++i) {
    const std::int64_t ri = row_indices[i];
    const std::int64_t ci = column_indices[i];
    const double vi = values[i];
    if (ri < 0 || ri >= rows || ci < 0 || ci >= cols) {
      return StatusOr<SparseTripletMatrix>(Status::shape_mismatch("make_sparse_triplet encountered out-of-range row/column index."));
    }
    if (!std::isfinite(vi)) {
      return StatusOr<SparseTripletMatrix>(Status::invalid_argument("make_sparse_triplet requires finite values."));
    }
    r.push_back(ri);
    c.push_back(ci);
    v.push_back(vi);
  }
  return StatusOr<SparseTripletMatrix>(SparseTripletMatrix{
      .row_indices = pluss::make_series(std::move(r), {static_cast<std::int64_t>(row_indices.size())}, i64_cpu()),
      .column_indices = pluss::make_series(std::move(c), {static_cast<std::int64_t>(column_indices.size())}, i64_cpu()),
      .values = pluss::make_series(std::move(v), {static_cast<std::int64_t>(values.size())}, f64_cpu()),
      .rows = rows,
      .cols = cols,
  });
}

StatusOr<SparseCsrMatrix> sparse_triplet_to_csr(const SparseTripletMatrix& sparse) {
  if (sparse.rows <= 0 || sparse.cols <= 0) {
    return StatusOr<SparseCsrMatrix>(Status::shape_mismatch("sparse_triplet_to_csr requires positive rows/cols."));
  }
  if (!sparse.row_indices.defined() || !sparse.column_indices.defined() || !sparse.values.defined()) {
    return StatusOr<SparseCsrMatrix>(Status::invalid_argument("sparse_triplet_to_csr requires defined triplet tensors."));
  }
  const std::int64_t nnz = sparse.values.numel();
  if (sparse.row_indices.numel() != nnz || sparse.column_indices.numel() != nnz) {
    return StatusOr<SparseCsrMatrix>(Status::shape_mismatch("sparse_triplet_to_csr index/value sizes must match."));
  }

  const auto rows = sparse.row_indices.to(pluss::pCPU, ScalarType::Int64).reshape({nnz}).to_vec<std::int64_t>();
  const auto cols = sparse.column_indices.to(pluss::pCPU, ScalarType::Int64).reshape({nnz}).to_vec<std::int64_t>();
  const auto vals = sparse.values.to(pluss::pCPU, ScalarType::Float64).reshape({nnz}).to_vec<double>();
  for (std::int64_t i = 0; i < nnz; ++i) {
    if (rows[static_cast<std::size_t>(i)] < 0 || rows[static_cast<std::size_t>(i)] >= sparse.rows
        || cols[static_cast<std::size_t>(i)] < 0 || cols[static_cast<std::size_t>(i)] >= sparse.cols) {
      return StatusOr<SparseCsrMatrix>(Status::shape_mismatch("sparse_triplet_to_csr found out-of-range row/column index."));
    }
    if (!std::isfinite(vals[static_cast<std::size_t>(i)])) {
      return StatusOr<SparseCsrMatrix>(Status::invalid_argument("sparse_triplet_to_csr requires finite values."));
    }
  }

  std::vector<std::int64_t> order(static_cast<std::size_t>(nnz), 0);
  std::iota(order.begin(), order.end(), static_cast<std::int64_t>(0));
  std::stable_sort(order.begin(), order.end(), [&](std::int64_t a, std::int64_t b) {
    const std::int64_t ra = rows[static_cast<std::size_t>(a)];
    const std::int64_t rb = rows[static_cast<std::size_t>(b)];
    if (ra != rb) return ra < rb;
    return cols[static_cast<std::size_t>(a)] < cols[static_cast<std::size_t>(b)];
  });

  SparseCsrMatrix out{};
  out.rows = sparse.rows;
  out.cols = sparse.cols;
  out.row_ptr.assign(static_cast<std::size_t>(out.rows + 1), 0);
  out.col_ind.resize(static_cast<std::size_t>(nnz));
  out.values.resize(static_cast<std::size_t>(nnz));

  for (std::int64_t pos = 0; pos < nnz; ++pos) {
    const std::int64_t oi = order[static_cast<std::size_t>(pos)];
    const std::int64_t r = rows[static_cast<std::size_t>(oi)];
    out.row_ptr[static_cast<std::size_t>(r + 1)] += 1;
    out.col_ind[static_cast<std::size_t>(pos)] = cols[static_cast<std::size_t>(oi)];
    out.values[static_cast<std::size_t>(pos)] = vals[static_cast<std::size_t>(oi)];
  }
  for (std::int64_t r = 0; r < out.rows; ++r) {
    out.row_ptr[static_cast<std::size_t>(r + 1)] += out.row_ptr[static_cast<std::size_t>(r)];
  }
  return StatusOr<SparseCsrMatrix>(std::move(out));
}

Status SparseLinearBinaryClassifier::learn(const SparseTripletMatrix& x,
                                           const Series& y,
                                           const TrainOptions& options,
                                           const LinearFamilyOptions& linear_options) {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return csr_or.status();
  return learn(csr_or.value(), y, options, linear_options);
}

Status SparseLinearBinaryClassifier::learn(const SparseCsrMatrix& csr,
                                           const Series& y,
                                           const TrainOptions& options,
                                           const LinearFamilyOptions& linear_options) {
  if (options.max_iterations <= 0) return Status::invalid_argument("SparseLinearBinaryClassifier requires max_iterations > 0.");
  if (!(options.tolerance > 0.0)) return Status::invalid_argument("SparseLinearBinaryClassifier requires tolerance > 0.");
  if (!y.defined()) return Status::invalid_argument("SparseLinearBinaryClassifier requires defined labels.");
  if (y.numel() != csr.rows) return Status::shape_mismatch("SparseLinearBinaryClassifier label count must match sparse rows.");

  std::vector<double> yv = series_f64(y);
  if (!finite_all(yv)) return Status::invalid_argument("SparseLinearBinaryClassifier requires finite labels.");
  double lo = yv[0];
  double hi = yv[0];
  for (double v : yv) {
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  if (lo == hi) return Status::invalid_argument("SparseLinearBinaryClassifier requires two label classes.");
  std::vector<double> y01(static_cast<std::size_t>(csr.rows), 0.0);
  for (std::int64_t i = 0; i < csr.rows; ++i) {
    const double v = yv[static_cast<std::size_t>(i)];
    y01[static_cast<std::size_t>(i)] = (std::abs(v - hi) <= std::abs(v - lo)) ? 1.0 : 0.0;
  }

  feature_count_ = csr.cols;
  StatusOr<std::vector<double>> trained_or =
      train_logistic_binary_weights(csr, y01, options, linear_options, "SparseLinearBinaryClassifier::learn");
  if (!trained_or.ok()) return trained_or.status();
  weights_ = trained_or.move_value();
  trained_ = true;
  return Status::success();
}

StatusOr<std::vector<double>> SparseLinearBinaryClassifier::infer_scores_cpu_(const SparseCsrMatrix& csr) const {
  if (!trained_) {
    return StatusOr<std::vector<double>>(Status::precondition("SparseLinearBinaryClassifier requires learn() before infer()."));
  }
  if (csr.cols != feature_count_) {
    return StatusOr<std::vector<double>>(Status::shape_mismatch("SparseLinearBinaryClassifier feature count mismatch."));
  }
  std::vector<double> out(static_cast<std::size_t>(csr.rows), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    const std::int64_t b = csr.row_ptr[static_cast<std::size_t>(r)];
    const std::int64_t e = csr.row_ptr[static_cast<std::size_t>(r + 1)];
    double z = weights_[0];
    for (std::int64_t p = b; p < e; ++p) {
      const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
      z += weights_[static_cast<std::size_t>(c + 1)] * csr.values[static_cast<std::size_t>(p)];
    }
    out[static_cast<std::size_t>(r)] = z;
  }
  return StatusOr<std::vector<double>>(std::move(out));
}

StatusOr<Series> SparseLinearBinaryClassifier::infer_scores(const SparseTripletMatrix& x) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return infer_scores(csr_or.value());
}

StatusOr<Series> SparseLinearBinaryClassifier::infer_scores(const SparseCsrMatrix& csr) const {
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  return StatusOr<Series>(
      pluss::make_series(std::move(scores_or.value()), {csr.rows}, f64_cpu()));
}

StatusOr<Series> SparseLinearBinaryClassifier::infer(const SparseTripletMatrix& x, const InferOptions& options) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return infer(csr_or.value(), options);
}

StatusOr<Series> SparseLinearBinaryClassifier::infer(const SparseCsrMatrix& csr, const InferOptions& options) const {
  if (!(options.positive_threshold > 0.0) || !(options.positive_threshold < 1.0)) {
    return StatusOr<Series>(Status::invalid_argument("SparseLinearBinaryClassifier positive_threshold must be in (0,1)."));
  }
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  const auto& scores = scores_or.value();
  std::vector<double> pred(scores.size(), 0.0);
  for (std::size_t i = 0; i < scores.size(); ++i) {
    pred[i] = sigmoid(scores[i]) >= options.positive_threshold ? 1.0 : 0.0;
  }
  return StatusOr<Series>(pluss::make_series(std::move(pred), {static_cast<std::int64_t>(scores.size())}, f64_cpu()));
}

StatusOr<Series> SparseLinearBinaryClassifier::assess(const SparseTripletMatrix& x,
                                                      const Series& y,
                                                      const AssessOptions& options) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return assess(csr_or.value(), y, options);
}

StatusOr<Series> SparseLinearBinaryClassifier::assess(const SparseCsrMatrix& csr,
                                                      const Series& y,
                                                      const AssessOptions& options) const {
  if (!y.defined()) return StatusOr<Series>(Status::invalid_argument("SparseLinearBinaryClassifier::assess requires labels."));
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  const auto& scores = scores_or.value();
  const auto truth_raw = series_f64(y);
  if (truth_raw.size() != scores.size()) {
    return StatusOr<Series>(Status::shape_mismatch("SparseLinearBinaryClassifier::assess label count mismatch."));
  }
  std::vector<double> truth(truth_raw.size(), 0.0);
  for (std::size_t i = 0; i < truth_raw.size(); ++i) truth[i] = truth_raw[i] > 0.0 ? 1.0 : 0.0;
  std::vector<MetricKind> metrics = options.metrics;
  if (metrics.empty()) metrics = {MetricKind::Accuracy};

  std::int64_t tp = 0, tn = 0, fp = 0, fn = 0;
  double logloss = 0.0;
  std::vector<double> probs(scores.size(), 0.0);
  for (std::size_t i = 0; i < scores.size(); ++i) {
    const double p = sigmoid(scores[i]);
    probs[i] = p;
    const double y01 = truth[i];
    const bool pred1 = p >= options.positive_threshold;
    logloss += -(y01 * std::log(std::max(p, 1e-12)) + (1.0 - y01) * std::log(std::max(1.0 - p, 1e-12)));
    if (pred1 && y01 > 0.5) ++tp;
    else if (pred1 && y01 <= 0.5) ++fp;
    else if (!pred1 && y01 > 0.5) ++fn;
    else ++tn;
  }
  const double n = static_cast<double>(scores.size());
  logloss /= std::max(1.0, n);
  const double acc = static_cast<double>(tp + tn) / std::max<std::int64_t>(1, static_cast<std::int64_t>(scores.size()));
  const double prec = tp + fp > 0 ? static_cast<double>(tp) / static_cast<double>(tp + fp) : 0.0;
  const double rec = tp + fn > 0 ? static_cast<double>(tp) / static_cast<double>(tp + fn) : 0.0;
  const double f1 = (prec + rec) > 0.0 ? 2.0 * prec * rec / (prec + rec) : 0.0;
  const double tpr = rec;
  const double tnr = tn + fp > 0 ? static_cast<double>(tn) / static_cast<double>(tn + fp) : 0.0;
  const double bacc = 0.5 * (tpr + tnr);

  std::vector<double> out{};
  out.reserve(metrics.size());
  for (MetricKind m : metrics) {
    switch (m) {
      case MetricKind::Accuracy: out.push_back(acc); break;
      case MetricKind::BalancedAccuracy: out.push_back(bacc); break;
      case MetricKind::Precision: out.push_back(prec); break;
      case MetricKind::Recall: out.push_back(rec); break;
      case MetricKind::F1: out.push_back(f1); break;
      case MetricKind::LogLoss: out.push_back(logloss); break;
      default:
        return StatusOr<Series>(Status::not_implemented(
            "SparseLinearBinaryClassifier::assess currently supports Accuracy/BalancedAccuracy/Precision/Recall/F1/LogLoss."));
    }
  }
  return StatusOr<Series>(pluss::make_series(std::move(out), {static_cast<std::int64_t>(metrics.size())}, f64_cpu()));
}

StatusOr<std::vector<Series>> SparseLinearBinaryClassifier::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      pluss::make_series(std::vector<double>{trained_ ? 1.0 : 0.0, static_cast<double>(feature_count_)}, {2}, f64_cpu()),
      pluss::make_series(weights_, {static_cast<std::int64_t>(weights_.size())}, f64_cpu()),
  });
}

Status SparseLinearBinaryClassifier::load_state(pluss::msharp::RangeView::Series state) {
  if (state.size() < 2) return Status::invalid_argument("SparseLinearBinaryClassifier::load_state expects 2 tensors.");
  const std::vector<double> meta = state[0].to(pluss::pCPU, ScalarType::Float64).reshape({state[0].numel()}).to_vec<double>();
  if (meta.size() < 2) return Status::invalid_argument("SparseLinearBinaryClassifier::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  feature_count_ = static_cast<std::int64_t>(meta[1]);
  weights_ = state[1].to(pluss::pCPU, ScalarType::Float64).reshape({state[1].numel()}).to_vec<double>();
  if (static_cast<std::int64_t>(weights_.size()) != feature_count_ + 1) {
    return Status::shape_mismatch("SparseLinearBinaryClassifier::load_state coefficient size mismatch.");
  }
  return Status::success();
}

Status SparseLinearRegressor::learn(const SparseTripletMatrix& x,
                                    const Series& y,
                                    const TrainOptions& options,
                                    const LinearFamilyOptions& linear_options) {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return csr_or.status();
  return learn(csr_or.value(), y, options, linear_options);
}

Status SparseLinearRegressor::learn(const SparseCsrMatrix& csr,
                                    const Series& y,
                                    const TrainOptions& options,
                                    const LinearFamilyOptions& linear_options) {
  if (options.max_iterations <= 0) return Status::invalid_argument("SparseLinearRegressor requires max_iterations > 0.");
  if (!(options.tolerance > 0.0)) return Status::invalid_argument("SparseLinearRegressor requires tolerance > 0.");
  if (!y.defined()) return Status::invalid_argument("SparseLinearRegressor requires defined targets.");
  if (y.numel() != csr.rows) return Status::shape_mismatch("SparseLinearRegressor target count must match sparse rows.");
  const std::vector<double> yv = series_f64(y);
  if (!finite_all(yv)) return Status::invalid_argument("SparseLinearRegressor requires finite targets.");

  feature_count_ = csr.cols;
  weights_.assign(static_cast<std::size_t>(feature_count_ + 1), 0.0);
  const double lr = 0.05;
  const double l2 = linear_options.regularization == RegularizationKind::L2 ? linear_options.regularization_strength : 0.0;
  double prev_loss = std::numeric_limits<double>::infinity();

  for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
    std::vector<double> grad(static_cast<std::size_t>(feature_count_ + 1), 0.0);
    double loss_sum = 0.0;
    for (std::int64_t r = 0; r < csr.rows; ++r) {
      const std::int64_t b = csr.row_ptr[static_cast<std::size_t>(r)];
      const std::int64_t e = csr.row_ptr[static_cast<std::size_t>(r + 1)];
      double pred = weights_[0];
      for (std::int64_t p = b; p < e; ++p) {
        const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
        pred += weights_[static_cast<std::size_t>(c + 1)] * csr.values[static_cast<std::size_t>(p)];
      }
      const double err = pred - yv[static_cast<std::size_t>(r)];
      grad[0] += err;
      for (std::int64_t p = b; p < e; ++p) {
        const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
        grad[static_cast<std::size_t>(c + 1)] += err * csr.values[static_cast<std::size_t>(p)];
      }
      loss_sum += 0.5 * err * err;
    }
    const double n = static_cast<double>(csr.rows);
    for (std::size_t j = 0; j < grad.size(); ++j) {
      grad[j] /= n;
      if (j > 0 && l2 > 0.0) grad[j] += l2 * weights_[j];
      weights_[j] -= lr * grad[j];
    }
    const double loss = loss_sum / n;
    if (!std::isfinite(loss)) return Status::numerical_failure("SparseLinearRegressor encountered non-finite objective.");
    if (std::abs(prev_loss - loss) <= options.tolerance) break;
    prev_loss = loss;
  }

  if (!finite_all(weights_)) {
    return Status::numerical_failure("SparseLinearRegressor produced non-finite coefficients.");
  }
  trained_ = true;
  return Status::success();
}

StatusOr<std::vector<double>> SparseLinearRegressor::infer_cpu_(const SparseCsrMatrix& csr) const {
  if (!trained_) return StatusOr<std::vector<double>>(Status::precondition("SparseLinearRegressor requires learn() before infer()."));
  if (csr.cols != feature_count_) return StatusOr<std::vector<double>>(Status::shape_mismatch("SparseLinearRegressor feature count mismatch."));
  std::vector<double> out(static_cast<std::size_t>(csr.rows), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    out[static_cast<std::size_t>(r)] = sparse_row_linear_score(csr, r, weights_, 0);
  }
  return StatusOr<std::vector<double>>(std::move(out));
}

StatusOr<Series> SparseLinearRegressor::infer(const SparseTripletMatrix& x) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return infer(csr_or.value());
}

StatusOr<Series> SparseLinearRegressor::infer(const SparseCsrMatrix& csr) const {
  StatusOr<std::vector<double>> pred_or = infer_cpu_(csr);
  if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
  return StatusOr<Series>(pluss::make_series(std::move(pred_or.value()), {csr.rows}, f64_cpu()));
}

StatusOr<Series> SparseLinearRegressor::assess(const SparseTripletMatrix& x,
                                               const Series& y,
                                               const AssessOptions& options) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return assess(csr_or.value(), y, options);
}

StatusOr<Series> SparseLinearRegressor::assess(const SparseCsrMatrix& csr,
                                               const Series& y,
                                               const AssessOptions& options) const {
  if (!y.defined()) return StatusOr<Series>(Status::invalid_argument("SparseLinearRegressor::assess requires targets."));
  StatusOr<std::vector<double>> pred_or = infer_cpu_(csr);
  if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
  const std::vector<double> truth = series_f64(y);
  const std::vector<double>& pred = pred_or.value();
  if (truth.size() != pred.size()) return StatusOr<Series>(Status::shape_mismatch("SparseLinearRegressor::assess target count mismatch."));

  double mse = 0.0;
  double mae = 0.0;
  double mean = 0.0;
  for (double v : truth) mean += v;
  mean /= std::max<std::size_t>(truth.size(), 1);
  double ss_tot = 0.0;
  double ss_res = 0.0;
  for (std::size_t i = 0; i < pred.size(); ++i) {
    const double err = pred[i] - truth[i];
    mse += err * err;
    mae += std::abs(err);
    ss_res += err * err;
    const double d = truth[i] - mean;
    ss_tot += d * d;
  }
  const double n = static_cast<double>(std::max<std::size_t>(pred.size(), 1));
  mse /= n;
  mae /= n;
  const double rmse = std::sqrt(std::max(0.0, mse));
  const double r2 = ss_tot > 0.0 ? 1.0 - ss_res / ss_tot : (ss_res <= 1e-12 ? 1.0 : 0.0);

  std::vector<MetricKind> metrics = options.metrics;
  if (metrics.empty()) metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  std::vector<double> out{};
  out.reserve(metrics.size());
  for (MetricKind m : metrics) {
    switch (m) {
      case MetricKind::Mse: out.push_back(mse); break;
      case MetricKind::Rmse: out.push_back(rmse); break;
      case MetricKind::Mae: out.push_back(mae); break;
      case MetricKind::R2: out.push_back(r2); break;
      default:
        return StatusOr<Series>(Status::not_implemented(
            "SparseLinearRegressor::assess currently supports Mse/Rmse/Mae/R2."));
    }
  }
  return StatusOr<Series>(pluss::make_series(std::move(out), {static_cast<std::int64_t>(metrics.size())}, f64_cpu()));
}

StatusOr<std::vector<Series>> SparseLinearRegressor::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      pluss::make_series(std::vector<double>{trained_ ? 1.0 : 0.0, static_cast<double>(feature_count_)}, {2}, f64_cpu()),
      pluss::make_series(weights_, {static_cast<std::int64_t>(weights_.size())}, f64_cpu()),
  });
}

Status SparseLinearRegressor::load_state(pluss::msharp::RangeView::Series state) {
  if (state.size() < 2) return Status::invalid_argument("SparseLinearRegressor::load_state expects 2 tensors.");
  const std::vector<double> meta = state[0].to(pluss::pCPU, ScalarType::Float64).reshape({state[0].numel()}).to_vec<double>();
  if (meta.size() < 2) return Status::invalid_argument("SparseLinearRegressor::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  feature_count_ = static_cast<std::int64_t>(meta[1]);
  weights_ = state[1].to(pluss::pCPU, ScalarType::Float64).reshape({state[1].numel()}).to_vec<double>();
  if (static_cast<std::int64_t>(weights_.size()) != feature_count_ + 1) {
    return Status::shape_mismatch("SparseLinearRegressor::load_state coefficient size mismatch.");
  }
  return Status::success();
}

Status SparseLinearMultiClassifier::learn(const SparseTripletMatrix& x,
                                          const Series& y,
                                          const TrainOptions& options,
                                          const LinearFamilyOptions& linear_options) {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return csr_or.status();
  return learn(csr_or.value(), y, options, linear_options);
}

Status SparseLinearMultiClassifier::learn(const SparseCsrMatrix& csr,
                                          const Series& y,
                                          const TrainOptions& options,
                                          const LinearFamilyOptions& linear_options) {
  if (options.max_iterations <= 0) return Status::invalid_argument("SparseLinearMultiClassifier requires max_iterations > 0.");
  if (!(options.tolerance > 0.0)) return Status::invalid_argument("SparseLinearMultiClassifier requires tolerance > 0.");
  if (!y.defined()) return Status::invalid_argument("SparseLinearMultiClassifier requires defined labels.");
  if (y.numel() != csr.rows) return Status::shape_mismatch("SparseLinearMultiClassifier label count must match sparse rows.");
  const std::vector<double> yv = series_f64(y);
  if (!finite_all(yv)) return Status::invalid_argument("SparseLinearMultiClassifier requires finite labels.");

  class_values_ = sorted_unique_values(yv);
  if (class_values_.size() < 2) {
    return Status::invalid_argument("SparseLinearMultiClassifier requires at least two distinct classes.");
  }

  feature_count_ = csr.cols;
  const std::int64_t stride = feature_count_ + 1;
  weights_.assign(static_cast<std::size_t>(stride * static_cast<std::int64_t>(class_values_.size())), 0.0);

  for (std::size_t ci = 0; ci < class_values_.size(); ++ci) {
    std::vector<double> y01(static_cast<std::size_t>(csr.rows), 0.0);
    const double cls = class_values_[ci];
    for (std::int64_t r = 0; r < csr.rows; ++r) {
      y01[static_cast<std::size_t>(r)] = yv[static_cast<std::size_t>(r)] == cls ? 1.0 : 0.0;
    }
    StatusOr<std::vector<double>> cls_w_or =
        train_logistic_binary_weights(csr, y01, options, linear_options, "SparseLinearMultiClassifier::learn");
    if (!cls_w_or.ok()) return cls_w_or.status();
    const std::vector<double>& cls_w = cls_w_or.value();
    for (std::int64_t j = 0; j < stride; ++j) {
      weights_[static_cast<std::size_t>(static_cast<std::int64_t>(ci) * stride + j)] = cls_w[static_cast<std::size_t>(j)];
    }
  }

  trained_ = true;
  return Status::success();
}

StatusOr<std::vector<double>> SparseLinearMultiClassifier::infer_scores_cpu_(const SparseCsrMatrix& csr) const {
  if (!trained_) {
    return StatusOr<std::vector<double>>(Status::precondition("SparseLinearMultiClassifier requires learn() before infer()."));
  }
  if (csr.cols != feature_count_) {
    return StatusOr<std::vector<double>>(Status::shape_mismatch("SparseLinearMultiClassifier feature count mismatch."));
  }
  const std::int64_t cls_count = static_cast<std::int64_t>(class_values_.size());
  std::vector<double> out(static_cast<std::size_t>(csr.rows * cls_count), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    for (std::int64_t ci = 0; ci < cls_count; ++ci) {
      out[static_cast<std::size_t>(r * cls_count + ci)] = sparse_row_linear_score(csr, r, weights_, ci);
    }
  }
  return StatusOr<std::vector<double>>(std::move(out));
}

StatusOr<Series> SparseLinearMultiClassifier::infer_scores(const SparseTripletMatrix& x) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return infer_scores(csr_or.value());
}

StatusOr<Series> SparseLinearMultiClassifier::infer_scores(const SparseCsrMatrix& csr) const {
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  return StatusOr<Series>(pluss::make_series(std::move(scores_or.value()),
                                             {csr.rows, static_cast<std::int64_t>(class_values_.size())},
                                             f64_cpu()));
}

StatusOr<Series> SparseLinearMultiClassifier::infer(const SparseTripletMatrix& x) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return infer(csr_or.value());
}

StatusOr<Series> SparseLinearMultiClassifier::infer(const SparseCsrMatrix& csr) const {
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  const std::vector<double>& scores = scores_or.value();
  const std::int64_t cls_count = static_cast<std::int64_t>(class_values_.size());
  std::vector<double> pred(static_cast<std::size_t>(csr.rows), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    std::int64_t best = 0;
    double best_score = scores[static_cast<std::size_t>(r * cls_count)];
    for (std::int64_t ci = 1; ci < cls_count; ++ci) {
      const double s = scores[static_cast<std::size_t>(r * cls_count + ci)];
      if (s > best_score) {
        best_score = s;
        best = ci;
      }
    }
    pred[static_cast<std::size_t>(r)] = class_values_[static_cast<std::size_t>(best)];
  }
  return StatusOr<Series>(pluss::make_series(std::move(pred), {csr.rows}, f64_cpu()));
}

StatusOr<Series> SparseLinearMultiClassifier::assess(const SparseTripletMatrix& x,
                                                     const Series& y,
                                                     const AssessOptions& options) const {
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  return assess(csr_or.value(), y, options);
}

StatusOr<Series> SparseLinearMultiClassifier::assess(const SparseCsrMatrix& csr,
                                                     const Series& y,
                                                     const AssessOptions& options) const {
  if (!y.defined()) return StatusOr<Series>(Status::invalid_argument("SparseLinearMultiClassifier::assess requires labels."));
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  const std::vector<double> truth = series_f64(y);
  if (static_cast<std::int64_t>(truth.size()) != csr.rows) {
    return StatusOr<Series>(Status::shape_mismatch("SparseLinearMultiClassifier::assess label count mismatch."));
  }

  const std::int64_t cls_count = static_cast<std::int64_t>(class_values_.size());
  const std::vector<double>& scores = scores_or.value();
  std::vector<std::int64_t> pred_idx(static_cast<std::size_t>(csr.rows), 0);
  std::vector<std::int64_t> truth_idx(static_cast<std::size_t>(csr.rows), -1);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    std::int64_t best = 0;
    double best_score = scores[static_cast<std::size_t>(r * cls_count)];
    for (std::int64_t ci = 1; ci < cls_count; ++ci) {
      const double s = scores[static_cast<std::size_t>(r * cls_count + ci)];
      if (s > best_score) {
        best_score = s;
        best = ci;
      }
    }
    pred_idx[static_cast<std::size_t>(r)] = best;
    for (std::int64_t ci = 0; ci < cls_count; ++ci) {
      if (truth[static_cast<std::size_t>(r)] == class_values_[static_cast<std::size_t>(ci)]) {
        truth_idx[static_cast<std::size_t>(r)] = ci;
        break;
      }
    }
    if (truth_idx[static_cast<std::size_t>(r)] < 0) {
      return StatusOr<Series>(
          Status::invalid_argument("SparseLinearMultiClassifier::assess labels must match classes observed at learn()."));
    }
  }

  std::vector<MetricKind> metrics = options.metrics;
  if (metrics.empty()) metrics = {MetricKind::Accuracy};
  const double n = static_cast<double>(std::max<std::int64_t>(1, csr.rows));
  double correct = 0.0;
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    if (pred_idx[static_cast<std::size_t>(r)] == truth_idx[static_cast<std::size_t>(r)]) correct += 1.0;
  }
  const double accuracy = correct / n;

  std::vector<double> class_recall(static_cast<std::size_t>(cls_count), 0.0);
  std::vector<double> class_precision(static_cast<std::size_t>(cls_count), 0.0);
  std::vector<double> class_f1(static_cast<std::size_t>(cls_count), 0.0);
  for (std::int64_t ci = 0; ci < cls_count; ++ci) {
    double tp = 0.0, fp = 0.0, fn = 0.0;
    for (std::int64_t r = 0; r < csr.rows; ++r) {
      const bool p = pred_idx[static_cast<std::size_t>(r)] == ci;
      const bool t = truth_idx[static_cast<std::size_t>(r)] == ci;
      if (p && t) tp += 1.0;
      else if (p && !t) fp += 1.0;
      else if (!p && t) fn += 1.0;
    }
    const double prec = tp + fp > 0.0 ? tp / (tp + fp) : 0.0;
    const double rec = tp + fn > 0.0 ? tp / (tp + fn) : 0.0;
    const double f1 = prec + rec > 0.0 ? 2.0 * prec * rec / (prec + rec) : 0.0;
    class_precision[static_cast<std::size_t>(ci)] = prec;
    class_recall[static_cast<std::size_t>(ci)] = rec;
    class_f1[static_cast<std::size_t>(ci)] = f1;
  }
  double macro_precision = 0.0;
  double macro_recall = 0.0;
  double macro_f1 = 0.0;
  for (std::int64_t ci = 0; ci < cls_count; ++ci) {
    macro_precision += class_precision[static_cast<std::size_t>(ci)];
    macro_recall += class_recall[static_cast<std::size_t>(ci)];
    macro_f1 += class_f1[static_cast<std::size_t>(ci)];
  }
  macro_precision /= static_cast<double>(cls_count);
  macro_recall /= static_cast<double>(cls_count);
  macro_f1 /= static_cast<double>(cls_count);

  std::vector<double> out{};
  out.reserve(metrics.size());
  for (MetricKind m : metrics) {
    switch (m) {
      case MetricKind::Accuracy: out.push_back(accuracy); break;
      case MetricKind::BalancedAccuracy: out.push_back(macro_recall); break;
      case MetricKind::Precision: out.push_back(macro_precision); break;
      case MetricKind::Recall: out.push_back(macro_recall); break;
      case MetricKind::F1: out.push_back(macro_f1); break;
      default:
        return StatusOr<Series>(Status::not_implemented(
            "SparseLinearMultiClassifier::assess currently supports Accuracy/BalancedAccuracy/Precision/Recall/F1."));
    }
  }
  return StatusOr<Series>(pluss::make_series(std::move(out), {static_cast<std::int64_t>(metrics.size())}, f64_cpu()));
}

StatusOr<std::vector<Series>> SparseLinearMultiClassifier::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      pluss::make_series(std::vector<double>{trained_ ? 1.0 : 0.0, static_cast<double>(feature_count_)}, {2}, f64_cpu()),
      pluss::make_series(class_values_, {static_cast<std::int64_t>(class_values_.size())}, f64_cpu()),
      pluss::make_series(weights_, {static_cast<std::int64_t>(weights_.size())}, f64_cpu()),
  });
}

Status SparseLinearMultiClassifier::load_state(pluss::msharp::RangeView::Series state) {
  if (state.size() < 3) return Status::invalid_argument("SparseLinearMultiClassifier::load_state expects 3 tensors.");
  const std::vector<double> meta = state[0].to(pluss::pCPU, ScalarType::Float64).reshape({state[0].numel()}).to_vec<double>();
  if (meta.size() < 2) return Status::invalid_argument("SparseLinearMultiClassifier::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  feature_count_ = static_cast<std::int64_t>(meta[1]);
  class_values_ = state[1].to(pluss::pCPU, ScalarType::Float64).reshape({state[1].numel()}).to_vec<double>();
  weights_ = state[2].to(pluss::pCPU, ScalarType::Float64).reshape({state[2].numel()}).to_vec<double>();
  const std::int64_t stride = feature_count_ + 1;
  if (class_values_.size() < 2) {
    return Status::shape_mismatch("SparseLinearMultiClassifier::load_state requires at least two classes.");
  }
  if (static_cast<std::int64_t>(weights_.size()) != stride * static_cast<std::int64_t>(class_values_.size())) {
    return Status::shape_mismatch("SparseLinearMultiClassifier::load_state coefficient size mismatch.");
  }
  return Status::success();
}

template <class SparseEstimator>
StatusOr<SparseCrossReport> cross_assess_sparse_impl(const SparseTripletMatrix& x,
                                                     const Series& y,
                                                     TaskKind task,
                                                     const SplitOptions& split,
                                                     const TrainOptions& train_options,
                                                     const AssessOptions& assess_options,
                                                     const LinearFamilyOptions& linear_options) {
  if (!y.defined()) {
    return StatusOr<SparseCrossReport>(Status::invalid_argument("sparse cross assess requires defined labels/targets."));
  }
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<SparseCrossReport>(csr_or.status());
  const SparseCsrMatrix csr = csr_or.move_value();
  if (y.numel() != csr.rows) {
    return StatusOr<SparseCrossReport>(Status::shape_mismatch("sparse cross assess requires y length to match sparse rows."));
  }

  const Series y64 = y.to(ScalarType::Float64).reshape({y.numel()});
  const SplitOptions effective_split = resolved_sparse_split_defaults(task, split);
  StatusOr<SplitPlan> plan_or = make_sparse_cross_plan(task, y64, csr.rows, effective_split);
  if (!plan_or.ok()) return StatusOr<SparseCrossReport>(plan_or.status());
  SplitPlan plan = plan_or.move_value();
  if (plan.folds.empty()) {
    return StatusOr<SparseCrossReport>(Status::internal("sparse cross assess produced an empty fold plan."));
  }

  SparseCrossReport report{};
  report.metrics = assess_options.metrics.empty() ? default_sparse_metrics(task) : assess_options.metrics;
  if (report.metrics.empty()) {
    return StatusOr<SparseCrossReport>(Status::invalid_argument("sparse cross assess requires at least one metric."));
  }
  AssessOptions fold_assess_options = assess_options;
  fold_assess_options.metrics = report.metrics;

  std::vector<double> fit_times{};
  std::vector<double> score_times{};
  std::vector<double> test_metric_values{};
  std::vector<double> train_metric_values{};
  fit_times.reserve(plan.folds.size());
  score_times.reserve(plan.folds.size());
  test_metric_values.reserve(plan.folds.size() * report.metrics.size());
  train_metric_values.reserve(plan.folds.size() * report.metrics.size());
  report.folds.reserve(plan.folds.size());

  for (std::size_t fi = 0; fi < plan.folds.size(); ++fi) {
    const IndexSplit& split_fold = plan.folds[fi];
    const std::vector<std::int64_t> train_ids = series_i64(split_fold.train_indices);
    const std::vector<std::int64_t> test_ids = series_i64(split_fold.test_indices);
    SparseCsrMatrix train_csr = csr_slice_rows(csr, train_ids);
    SparseCsrMatrix test_csr = csr_slice_rows(csr, test_ids);
    const Series train_y = y64.index_select(0, split_fold.train_indices.to(ScalarType::Int64)).reshape({split_fold.train_indices.numel()});
    const Series test_y = y64.index_select(0, split_fold.test_indices.to(ScalarType::Int64)).reshape({split_fold.test_indices.numel()});

    SparseEstimator estimator{};

    const auto fit_start = std::chrono::steady_clock::now();
    const Status fit_status = estimator.learn(train_csr, train_y, train_options, linear_options);
    const auto fit_end = std::chrono::steady_clock::now();
    if (!fit_status.ok()) {
      return StatusOr<SparseCrossReport>(with_sparse_fold_context(fit_status,
                                                                  static_cast<std::int64_t>(fi),
                                                                  static_cast<std::int64_t>(plan.folds.size()),
                                                                  split_fold.train_indices.numel(),
                                                                  split_fold.test_indices.numel()));
    }

    const auto score_start = std::chrono::steady_clock::now();
    StatusOr<Series> test_metrics_or = estimator.assess(test_csr, test_y, fold_assess_options);
    const auto score_end = std::chrono::steady_clock::now();
    if (!test_metrics_or.ok()) {
      return StatusOr<SparseCrossReport>(with_sparse_fold_context(test_metrics_or.status(),
                                                                  static_cast<std::int64_t>(fi),
                                                                  static_cast<std::int64_t>(plan.folds.size()),
                                                                  split_fold.train_indices.numel(),
                                                                  split_fold.test_indices.numel()));
    }
    StatusOr<Series> train_metrics_or = estimator.assess(train_csr, train_y, fold_assess_options);
    if (!train_metrics_or.ok()) {
      return StatusOr<SparseCrossReport>(with_sparse_fold_context(train_metrics_or.status(),
                                                                  static_cast<std::int64_t>(fi),
                                                                  static_cast<std::int64_t>(plan.folds.size()),
                                                                  split_fold.train_indices.numel(),
                                                                  split_fold.test_indices.numel()));
    }

    SparseFoldReport fold{};
    fold.metrics = test_metrics_or.move_value().to(ScalarType::Float64).reshape({static_cast<std::int64_t>(report.metrics.size())});
    fold.train_metrics = train_metrics_or.move_value().to(ScalarType::Float64).reshape({static_cast<std::int64_t>(report.metrics.size())});
    const std::vector<double> test_vec = fold.metrics.to(pluss::pCPU, ScalarType::Float64).reshape({static_cast<std::int64_t>(report.metrics.size())}).to_vec<double>();
    const std::vector<double> train_vec = fold.train_metrics.to(pluss::pCPU, ScalarType::Float64).reshape({static_cast<std::int64_t>(report.metrics.size())}).to_vec<double>();
    test_metric_values.insert(test_metric_values.end(), test_vec.begin(), test_vec.end());
    train_metric_values.insert(train_metric_values.end(), train_vec.begin(), train_vec.end());
    fold.train_indices = split_fold.train_indices;
    fold.test_indices = split_fold.test_indices;
    fold.fit_time_seconds = std::chrono::duration<double>(fit_end - fit_start).count();
    fold.score_time_seconds = std::chrono::duration<double>(score_end - score_start).count();

    fit_times.push_back(fold.fit_time_seconds);
    score_times.push_back(fold.score_time_seconds);
    report.folds.push_back(std::move(fold));
  }

  const std::int64_t fold_count = static_cast<std::int64_t>(report.folds.size());
  const std::int64_t metric_count = static_cast<std::int64_t>(report.metrics.size());
  const Series test_matrix =
      pluss::make_series(std::move(test_metric_values), {fold_count, metric_count}, f64_cpu());
  const Series train_matrix =
      pluss::make_series(std::move(train_metric_values), {fold_count, metric_count}, f64_cpu());

  report.mean_metrics = test_matrix.mean(0, false).reshape({metric_count});
  report.mean_train_metrics = train_matrix.mean(0, false).reshape({metric_count});
  report.std_metrics =
      test_matrix.sub(report.mean_metrics.reshape({1, metric_count})).square().mean(0, false).sqrt().reshape({metric_count});
  report.std_train_metrics =
      train_matrix.sub(report.mean_train_metrics.reshape({1, metric_count})).square().mean(0, false).sqrt().reshape({metric_count});
  report.fit_time_seconds = pluss::make_series(std::move(fit_times), {fold_count}, f64_cpu());
  report.score_time_seconds = pluss::make_series(std::move(score_times), {fold_count}, f64_cpu());
  return StatusOr<SparseCrossReport>(std::move(report));
}

StatusOr<SparseCrossReport> assess_sparse_binary_folds(const SparseTripletMatrix& x,
                                                       const Series& y,
                                                       const SplitOptions& split,
                                                       const TrainOptions& train_options,
                                                       const AssessOptions& assess_options,
                                                       const LinearFamilyOptions& linear_options) {
  return cross_assess_sparse_impl<SparseLinearBinaryClassifier>(
      x, y, TaskKind::Classification, split, train_options, assess_options, linear_options);
}

StatusOr<SparseCrossReport> assess_sparse_regression_folds(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split,
                                                           const TrainOptions& train_options,
                                                           const AssessOptions& assess_options,
                                                           const LinearFamilyOptions& linear_options) {
  return cross_assess_sparse_impl<SparseLinearRegressor>(
      x, y, TaskKind::Regression, split, train_options, assess_options, linear_options);
}

StatusOr<SparseCrossReport> assess_sparse_multiclass_folds(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split,
                                                           const TrainOptions& train_options,
                                                           const AssessOptions& assess_options,
                                                           const LinearFamilyOptions& linear_options) {
  return cross_assess_sparse_impl<SparseLinearMultiClassifier>(
      x, y, TaskKind::Classification, split, train_options, assess_options, linear_options);
}

StatusOr<SparseCrossReport> cross_assess_sparse_binary(const SparseTripletMatrix& x,
                                                       const Series& y,
                                                       const SplitOptions& split,
                                                       const TrainOptions& train_options,
                                                       const AssessOptions& assess_options,
                                                       const LinearFamilyOptions& linear_options) {
  return assess_sparse_binary_folds(x, y, split, train_options, assess_options, linear_options);
}

StatusOr<SparseCrossReport> cross_assess_sparse_regression(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split,
                                                           const TrainOptions& train_options,
                                                           const AssessOptions& assess_options,
                                                           const LinearFamilyOptions& linear_options) {
  return assess_sparse_regression_folds(x, y, split, train_options, assess_options, linear_options);
}

StatusOr<SparseCrossReport> cross_assess_sparse_multiclass(const SparseTripletMatrix& x,
                                                           const Series& y,
                                                           const SplitOptions& split,
                                                           const TrainOptions& train_options,
                                                           const AssessOptions& assess_options,
                                                           const LinearFamilyOptions& linear_options) {
  return assess_sparse_multiclass_folds(x, y, split, train_options, assess_options, linear_options);
}

template <class CrossAssessFn>
StatusOr<SparseTuneReport> sweep_sparse_impl(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                             const SparseTripletMatrix& x,
                                             const Series& y,
                                             const SplitOptions& split,
                                             const TrainOptions& train_options,
                                             const AssessOptions& assess_options,
                                             MetricKind objective_metric,
                                             SparseObjectivePolicy objective_policy,
                                             const CrossAssessFn& cross_assess_fn) {
  if (candidates.empty()) {
    return StatusOr<SparseTuneReport>(Status::invalid_argument("sweep_sparse requires at least one candidate."));
  }

  SparseTuneReport report{};
  report.objective_metric = objective_metric;
  report.objective_policy = objective_policy;
  report.trials.reserve(candidates.size());

  const bool maximize = maximize_sparse_objective(objective_policy, objective_metric);
  bool have_best = false;
  double best_value = maximize ? -std::numeric_limits<double>::infinity()
                               : std::numeric_limits<double>::infinity();
  std::int64_t best_idx = -1;

  for (std::size_t i = 0; i < candidates.size(); ++i) {
    SparseTuneTrial trial{};
    trial.options = candidates[i];
    StatusOr<SparseCrossReport> assess_or =
        cross_assess_fn(x, y, split, train_options, assess_options, trial.options);
    if (!assess_or.ok()) {
      trial.status = assess_or.status();
      report.trials.push_back(std::move(trial));
      continue;
    }
    trial.assess = assess_or.move_value();
    trial.status = Status::success();

    std::int64_t metric_index = -1;
    for (std::size_t mi = 0; mi < trial.assess.metrics.size(); ++mi) {
      if (trial.assess.metrics[mi] == objective_metric) {
        metric_index = static_cast<std::int64_t>(mi);
        break;
      }
    }
    if (metric_index < 0) {
      trial.status = Status::invalid_argument("sweep_sparse objective metric not present in assess metrics.");
      trial.objective = std::numeric_limits<double>::quiet_NaN();
      report.trials.push_back(std::move(trial));
      continue;
    }
    const std::vector<double> mean_metrics =
        trial.assess.mean_metrics.to(pluss::pCPU, ScalarType::Float64).reshape({trial.assess.mean_metrics.numel()}).to_vec<double>();
    trial.objective = mean_metrics[static_cast<std::size_t>(metric_index)];
    report.trials.push_back(std::move(trial));

    const SparseTuneTrial& stored = report.trials.back();
    if (stored.status.ok() && std::isfinite(stored.objective)) {
      if (!have_best || (maximize ? stored.objective > best_value : stored.objective < best_value)) {
        have_best = true;
        best_value = stored.objective;
        best_idx = static_cast<std::int64_t>(i);
      }
    }
  }

  report.best_index = best_idx;

  std::vector<std::int64_t> valid{};
  valid.reserve(report.trials.size());
  report.objective_rank.assign(report.trials.size(), 0);
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    if (report.trials[i].status.ok() && std::isfinite(report.trials[i].objective)) {
      valid.push_back(static_cast<std::int64_t>(i));
    }
  }
  if (valid.empty()) {
    std::fill(report.objective_rank.begin(), report.objective_rank.end(), 1);
    return StatusOr<SparseTuneReport>(std::move(report));
  }

  std::stable_sort(valid.begin(), valid.end(), [&](std::int64_t li, std::int64_t ri) {
    const double lhs = report.trials[static_cast<std::size_t>(li)].objective;
    const double rhs = report.trials[static_cast<std::size_t>(ri)].objective;
    if (lhs == rhs) return false;
    return maximize ? (lhs > rhs) : (lhs < rhs);
  });

  std::int64_t rank = 1;
  double prev = report.trials[static_cast<std::size_t>(valid[0])].objective;
  report.objective_rank[static_cast<std::size_t>(valid[0])] = rank;
  for (std::size_t pos = 1; pos < valid.size(); ++pos) {
    const double value = report.trials[static_cast<std::size_t>(valid[pos])].objective;
    if (value != prev) {
      rank = static_cast<std::int64_t>(pos) + 1;
      prev = value;
    }
    report.objective_rank[static_cast<std::size_t>(valid[pos])] = rank;
  }
  const std::int64_t worst = static_cast<std::int64_t>(valid.size()) + 1;
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    if (!(report.trials[i].status.ok() && std::isfinite(report.trials[i].objective))) {
      report.objective_rank[i] = worst;
    }
  }

  return StatusOr<SparseTuneReport>(std::move(report));
}

StatusOr<SparseTuneReport> tune_sparse_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                              const SparseTripletMatrix& x,
                                              const Series& y,
                                              const SplitOptions& split,
                                              const TrainOptions& train_options,
                                              const AssessOptions& assess_options,
                                              MetricKind objective_metric,
                                              SparseObjectivePolicy objective_policy) {
  return sweep_sparse_impl(candidates,
                           x,
                           y,
                           split,
                           train_options,
                           assess_options,
                           objective_metric,
                           objective_policy,
                           assess_sparse_binary_folds);
}

StatusOr<SparseTuneReport> tune_sparse_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                  const SparseTripletMatrix& x,
                                                  const Series& y,
                                                  const SplitOptions& split,
                                                  const TrainOptions& train_options,
                                                  const AssessOptions& assess_options,
                                                  MetricKind objective_metric,
                                                  SparseObjectivePolicy objective_policy) {
  return sweep_sparse_impl(candidates,
                           x,
                           y,
                           split,
                           train_options,
                           assess_options,
                           objective_metric,
                           objective_policy,
                           assess_sparse_regression_folds);
}

StatusOr<SparseTuneReport> tune_sparse_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                  const SparseTripletMatrix& x,
                                                  const Series& y,
                                                  const SplitOptions& split,
                                                  const TrainOptions& train_options,
                                                  const AssessOptions& assess_options,
                                                  MetricKind objective_metric,
                                                  SparseObjectivePolicy objective_policy) {
  return sweep_sparse_impl(candidates,
                           x,
                           y,
                           split,
                           train_options,
                           assess_options,
                           objective_metric,
                           objective_policy,
                           assess_sparse_multiclass_folds);
}

StatusOr<SparseTuneReport> sweep_sparse_binary(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                               const SparseTripletMatrix& x,
                                               const Series& y,
                                               const SplitOptions& split,
                                               const TrainOptions& train_options,
                                               const AssessOptions& assess_options,
                                               MetricKind objective_metric,
                                               SparseObjectivePolicy objective_policy) {
  return tune_sparse_binary(candidates,
                            x,
                            y,
                            split,
                            train_options,
                            assess_options,
                            objective_metric,
                            objective_policy);
}

StatusOr<SparseTuneReport> sweep_sparse_regression(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                   const SparseTripletMatrix& x,
                                                   const Series& y,
                                                   const SplitOptions& split,
                                                   const TrainOptions& train_options,
                                                   const AssessOptions& assess_options,
                                                   MetricKind objective_metric,
                                                   SparseObjectivePolicy objective_policy) {
  return tune_sparse_regression(candidates,
                                x,
                                y,
                                split,
                                train_options,
                                assess_options,
                                objective_metric,
                                objective_policy);
}

StatusOr<SparseTuneReport> sweep_sparse_multiclass(pluss::msharp::RangeView::Of<LinearFamilyOptions> candidates,
                                                   const SparseTripletMatrix& x,
                                                   const Series& y,
                                                   const SplitOptions& split,
                                                   const TrainOptions& train_options,
                                                   const AssessOptions& assess_options,
                                                   MetricKind objective_metric,
                                                   SparseObjectivePolicy objective_policy) {
  return tune_sparse_multiclass(candidates,
                                x,
                                y,
                                split,
                                train_options,
                                assess_options,
                                objective_metric,
                                objective_policy);
}

} // namespace pluss::ai::nodej
