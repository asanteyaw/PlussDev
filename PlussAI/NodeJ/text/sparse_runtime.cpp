#include "pluss/ai/nodej/sparse_runtime.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
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
  if (options.max_iterations <= 0) return Status::invalid_argument("SparseLinearBinaryClassifier requires max_iterations > 0.");
  if (!(options.tolerance > 0.0)) return Status::invalid_argument("SparseLinearBinaryClassifier requires tolerance > 0.");
  if (!y.defined()) return Status::invalid_argument("SparseLinearBinaryClassifier requires defined labels.");

  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return csr_or.status();
  SparseCsrMatrix csr = csr_or.move_value();
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
  weights_.assign(static_cast<std::size_t>(feature_count_ + 1), 0.0); // bias + weights
  const double lr = 0.05;
  const double l2 = linear_options.regularization == RegularizationKind::L2 ? linear_options.regularization_strength : 0.0;
  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
    std::vector<double> grad(static_cast<std::size_t>(feature_count_ + 1), 0.0);
    double loss_sum = 0.0;
    for (std::int64_t r = 0; r < csr.rows; ++r) {
      const std::int64_t b = csr.row_ptr[static_cast<std::size_t>(r)];
      const std::int64_t e = csr.row_ptr[static_cast<std::size_t>(r + 1)];
      double z = weights_[0];
      for (std::int64_t p = b; p < e; ++p) {
        const std::int64_t c = csr.col_ind[static_cast<std::size_t>(p)];
        z += weights_[static_cast<std::size_t>(c + 1)] * csr.values[static_cast<std::size_t>(p)];
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
      if (j > 0 && l2 > 0.0) grad[j] += l2 * weights_[j];
      weights_[j] -= lr * grad[j];
    }
    const double loss = loss_sum / n;
    if (!std::isfinite(loss)) return Status::numerical_failure("SparseLinearBinaryClassifier encountered non-finite objective.");
    if (std::abs(prev_loss - loss) <= options.tolerance) break;
    prev_loss = loss;
  }

  if (!finite_all(weights_)) {
    return Status::numerical_failure("SparseLinearBinaryClassifier produced non-finite coefficients.");
  }
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
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr_or.value());
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  return StatusOr<Series>(
      pluss::make_series(std::move(scores_or.value()), {csr_or.value().rows}, f64_cpu()));
}

StatusOr<Series> SparseLinearBinaryClassifier::infer(const SparseTripletMatrix& x, const InferOptions& options) const {
  if (!(options.positive_threshold > 0.0) || !(options.positive_threshold < 1.0)) {
    return StatusOr<Series>(Status::invalid_argument("SparseLinearBinaryClassifier positive_threshold must be in (0,1)."));
  }
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr_or.value());
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
  if (!y.defined()) return StatusOr<Series>(Status::invalid_argument("SparseLinearBinaryClassifier::assess requires labels."));
  StatusOr<SparseCsrMatrix> csr_or = sparse_triplet_to_csr(x);
  if (!csr_or.ok()) return StatusOr<Series>(csr_or.status());
  StatusOr<std::vector<double>> scores_or = infer_scores_cpu_(csr_or.value());
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

} // namespace pluss::ai::nodej
