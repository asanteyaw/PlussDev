#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/spatial_index.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

const char* family_name(ModelFamily family) {
  switch (family) {
    case ModelFamily::Linear:
      return "Linear";
    case ModelFamily::Tree:
      return "Tree";
    case ModelFamily::Ensemble:
      return "Ensemble";
    case ModelFamily::Neighbor:
      return "Neighbor";
    case ModelFamily::Kernel:
      return "Kernel";
    case ModelFamily::Bayes:
      return "Bayes";
    case ModelFamily::Cluster:
      return "Cluster";
    case ModelFamily::Decompose:
      return "Decompose";
    case ModelFamily::Detect:
      return "Detect";
  }
  return "Unknown";
}

bool is_phase0_family(ModelFamily family) {
  switch (family) {
    case ModelFamily::Linear:
    case ModelFamily::Tree:
    case ModelFamily::Ensemble:
    case ModelFamily::Neighbor:
    case ModelFamily::Kernel:
    case ModelFamily::Bayes:
      return true;
    case ModelFamily::Cluster:
    case ModelFamily::Decompose:
    case ModelFamily::Detect:
      return false;
  }
  return false;
}

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::int64_t resolved_parallel_workers(std::int64_t tasks) {
  if (tasks <= 1) return 1;
  const std::uint32_t hw = std::thread::hardware_concurrency();
  const std::int64_t cap = hw == 0 ? 1 : static_cast<std::int64_t>(hw);
  return std::max<std::int64_t>(1, std::min<std::int64_t>(tasks, cap));
}

[[nodiscard]] Series state_f64(std::vector<double> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values), {n}, f64_cpu());
}

[[nodiscard]] Series state_i64(std::vector<std::int64_t> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values), {n}, i64_cpu());
}

[[nodiscard]] std::vector<double> vec_f64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>().array().data;
}

[[nodiscard]] std::vector<std::int64_t> vec_i64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).host<std::int64_t>().array().data;
}

[[nodiscard]] Series make_index_series(const std::vector<std::int64_t>& indices, Platform platform) {
  return pluss::make_series(
      indices,
      {static_cast<std::int64_t>(indices.size())},
      Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

[[nodiscard]] bool all_finite(const Series& x) {
  return x.isfinite().all().item<bool>();
}

[[nodiscard]] bool has_nan(const Series& x) {
  return x.isnan().any().item<bool>();
}

struct DenseFeatureMatrix {
  Series x_rows = pluss::series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  Platform original_platform = pluss::pCPU;
  ScalarType original_dtype = ScalarType::Float64;
};

[[nodiscard]] StatusOr<DenseFeatureMatrix> to_dense_rows(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseFeatureMatrix>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseFeatureMatrix>(
        Status::shape_mismatch(std::string(where) + " requires rank-2 feature matrices."));
  }

  DenseFeatureMatrix out{};
  out.sample_count = batch.sample_count();
  out.feature_count = batch.feature_count();
  out.original_platform = batch.x.platform();
  out.original_dtype = batch.x.scalar_type();

  Series x = batch.x.to(ScalarType::Float64);
  if (batch.layout == ObservationLayout::Columns) {
    x = x.transpose(0, 1);
  }
  if (x.sizes().size() != 2 || x.sizes()[0] != out.sample_count || x.sizes()[1] != out.feature_count) {
    return StatusOr<DenseFeatureMatrix>(
        Status::shape_mismatch(std::string(where) + " encountered inconsistent feature matrix shape."));
  }
  out.x_rows = std::move(x);
  return StatusOr<DenseFeatureMatrix>(std::move(out));
}

[[nodiscard]] StatusOr<Series> to_label_vector(const LabeledBatch& batch,
                                               std::int64_t expected_samples,
                                               const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }
  Series y = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y)) {
    return StatusOr<Series>(Status::invalid_argument(
        std::string(where) + " does not support NaN labels; impute/drop missing labels before learn()."));
  }
  return StatusOr<Series>(std::move(y));
}

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::series::ones({x_rows.sizes()[0], 1}, f64_on(x_rows.platform()));
  return ones.unite({x_rows}, 1);
}

[[nodiscard]] StatusOr<Series> solve_ridge_least_squares(const Series& design,
                                                         const Series& labels,
                                                         double lambda,
                                                         bool fit_intercept,
                                                         double pinv_rcond) {
  if (!std::isfinite(lambda) || lambda < 0.0) {
    return StatusOr<Series>(Status::invalid_argument("Linear regression requires finite regularization_strength >= 0."));
  }
  if (!std::isfinite(pinv_rcond) || pinv_rcond <= 0.0) {
    return StatusOr<Series>(Status::invalid_argument("Linear regression requires pinv_rcond > 0."));
  }

  const std::int64_t feature_dim = design.sizes()[1];
  const Platform platform = design.platform();
  const Series xt = design.transpose(0, 1);
  Series gram = xt.matmul(design);
  Series rhs = xt.matmul(labels.reshape({labels.numel(), 1}));

  if (lambda > 0.0) {
    Series ridge_diag = pluss::series::ones({feature_dim}, f64_on(platform)).mul(lambda);
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
      ridge_diag = pluss::index_put(ridge_diag,
                                    std::vector<Series>{idx0},
                                    pluss::series::zeros({1}, f64_on(platform)),
                                    false);
    }
    gram = gram.add(pluss::series::eye(feature_dim, feature_dim, f64_on(platform)).mul(ridge_diag.reshape({1, feature_dim})));
  }

  const Series eye = pluss::series::eye(feature_dim, feature_dim, f64_on(platform));
  const std::array<double, 7> jitters{0.0, 1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2};
  for (double jitter : jitters) {
    try {
      Series g = gram;
      if (jitter > 0.0) g = g.add(eye.mul(jitter));
      const Series chol = pluss::subsp::math::linalg::cholesky(g, false).to(platform, ScalarType::Float64);
      const Series beta = pluss::subsp::math::linalg::cholesky_solve(rhs, chol, false).reshape({feature_dim}).to(platform, ScalarType::Float64);
      if (all_finite(beta)) return StatusOr<Series>(beta);
    } catch (const std::exception&) {
    }
  }

  // Conservative fallback for poorly conditioned design matrices.
  Series x_aug = design;
  Series y_aug = labels.reshape({labels.numel(), 1});
  if (lambda > 0.0) {
    Series penalty = pluss::series::eye(feature_dim, feature_dim, f64_on(platform)).mul(std::sqrt(lambda));
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, penalty.platform());
      penalty = pluss::index_put(penalty, std::vector<Series>{idx0, idx0}, pluss::series::zeros({1}, f64_on(penalty.platform())), false);
    }
    x_aug = x_aug.unite({penalty}, 0);
    y_aug = y_aug.unite({pluss::series::zeros({feature_dim, 1}, f64_on(platform))}, 0);
  }
  const Series beta = pluss::subsp::math::linalg::pinverse(x_aug, pinv_rcond).matmul(y_aug).reshape({feature_dim});
  if (all_finite(beta)) return StatusOr<Series>(beta);
  return StatusOr<Series>(Status::numerical_failure("Linear regression failed to produce finite coefficients."));
}

struct LinearPenalty {
  double l1 = 0.0;
  double l2 = 0.0;
};

[[nodiscard]] LinearPenalty resolve_linear_penalty(const LinearFamilyOptions& options) {
  LinearPenalty out{};
  switch (options.regularization) {
    case RegularizationKind::None:
      break;
    case RegularizationKind::L1:
      out.l1 = options.regularization_strength;
      break;
    case RegularizationKind::L2:
      out.l2 = options.regularization_strength;
      break;
    case RegularizationKind::Elastic:
      out.l1 = options.regularization_strength * options.elastic_l1_ratio;
      out.l2 = options.regularization_strength * (1.0 - options.elastic_l1_ratio);
      break;
  }
  return out;
}

[[nodiscard]] Series apply_l1_prox(const Series& weights,
                                   double shrink,
                                   bool fit_intercept) {
  if (shrink <= 0.0) return weights;
  Series out = weights.sign().mul(weights.abs().sub(shrink).clamp_min(0.0));
  if (fit_intercept) {
    const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, out.platform());
    const Series w0 = weights.index_select(0, idx0).reshape({1});
    out = pluss::index_put(out, std::vector<Series>{idx0}, w0, false);
  }
  return out;
}

[[nodiscard]] double linear_l2_term(const Series& weights, bool fit_intercept, double lambda_l2) {
  if (lambda_l2 <= 0.0) return 0.0;
  Series penalized = weights;
  if (fit_intercept) {
    const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, penalized.platform());
    penalized = pluss::index_put(penalized, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(penalized.platform())), false);
  }
  return 0.5 * lambda_l2 * penalized.square().sum().item<double>();
}

[[nodiscard]] double linear_l1_term(const Series& weights, bool fit_intercept, double lambda_l1) {
  if (lambda_l1 <= 0.0) return 0.0;
  Series penalized = weights;
  if (fit_intercept) {
    const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, penalized.platform());
    penalized = pluss::index_put(penalized, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(penalized.platform())), false);
  }
  return lambda_l1 * penalized.abs().sum().item<double>();
}

[[nodiscard]] StatusOr<Series> solve_elastic_least_squares(const Series& design,
                                                           const Series& labels,
                                                           bool fit_intercept,
                                                           double lambda_l1,
                                                           double lambda_l2,
                                                           std::int64_t max_iterations,
                                                           double tolerance,
                                                           const char* where) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t p = design.sizes()[1];
  if (n <= 0 || p <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty design matrix."));
  }
  if (max_iterations <= 0 || !(tolerance > 0.0)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires positive max_iterations and tolerance."));
  }

  Series weights = pluss::series::zeros({p}, f64_on(design.platform()));
  const Series design_t = design.transpose(0, 1);
  const Series col_sq_sum = design.square().sum(0, false);
  const double max_col_sq = col_sq_sum.max().reshape({1}).host<double>().front();
  double lipschitz = (std::isfinite(max_col_sq) ? max_col_sq : 0.0) / static_cast<double>(n);
  lipschitz += std::max(0.0, lambda_l2);
  if (!(lipschitz > 0.0) || !std::isfinite(lipschitz)) {
    lipschitz = 1.0;
  }
  const double step = 1.0 / lipschitz;

  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const Series pred = design.matmul(weights.reshape({p, 1})).reshape({n});
    const Series residual = pred.sub(labels);
    Series grad = design_t.matmul(residual.reshape({n, 1})).div(static_cast<double>(n)).reshape({p});
    if (lambda_l2 > 0.0) {
      Series ridge = weights;
      if (fit_intercept) {
        const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
        ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
      }
      grad = grad.add(ridge.mul(lambda_l2));
    }
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite gradient."));
    }

    const Series w_step = weights.sub(grad.mul(step));
    const Series w_next = apply_l1_prox(w_step, lambda_l1 * step, fit_intercept);
    if (!all_finite(w_next)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
    }

    const Series next_pred = design.matmul(w_next.reshape({p, 1})).reshape({n});
    const double data_loss = 0.5 * next_pred.sub(labels).square().mean().item<double>();
    const double loss = data_loss + linear_l2_term(w_next, fit_intercept, lambda_l2) + linear_l1_term(w_next, fit_intercept, lambda_l1);
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite objective."));
    }

    const double step_inf = w_next.sub(weights).abs().max().reshape({1}).host<double>().front();
    if (step_inf <= tolerance || std::abs(prev_loss - loss) <= tolerance) {
      weights = w_next;
      break;
    }
    weights = w_next;
    prev_loss = loss;
  }
  return StatusOr<Series>(weights);
}

[[nodiscard]] StatusOr<Series> solve_robust_least_squares(const Series& design,
                                                          const Series& labels,
                                                          bool fit_intercept,
                                                          double robust_transition,
                                                          double lambda_l1,
                                                          double lambda_l2,
                                                          std::int64_t max_iterations,
                                                          double tolerance,
                                                          const char* where) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t p = design.sizes()[1];
  if (n <= 0 || p <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty design matrix."));
  }
  if (max_iterations <= 0 || !(tolerance > 0.0)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires positive max_iterations and tolerance."));
  }
  if (!(robust_transition > 0.0) || !std::isfinite(robust_transition)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires finite robust_transition > 0."));
  }

  Series weights = pluss::series::zeros({p}, f64_on(design.platform()));
  const Series design_t = design.transpose(0, 1);
  const Series col_sq_sum = design.square().sum(0, false);
  const double max_col_sq = col_sq_sum.max().reshape({1}).host<double>().front();
  // Lipschitz upper bound for Huber gradient in prediction space is 1.
  double lipschitz = (std::isfinite(max_col_sq) ? max_col_sq : 0.0) / static_cast<double>(n);
  lipschitz += std::max(0.0, lambda_l2);
  if (!(lipschitz > 0.0) || !std::isfinite(lipschitz)) {
    lipschitz = 1.0;
  }
  const double step = 1.0 / lipschitz;

  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const Series pred = design.matmul(weights.reshape({p, 1})).reshape({n});
    const Series residual = pred.sub(labels);
    const Series abs_residual = residual.abs();
    const Series huber_grad = abs_residual.le(robust_transition).where(
        residual,
        residual.sign().mul(robust_transition));

    Series grad = design_t.matmul(huber_grad.reshape({n, 1})).div(static_cast<double>(n)).reshape({p});
    if (lambda_l2 > 0.0) {
      Series ridge = weights;
      if (fit_intercept) {
        const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
        ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
      }
      grad = grad.add(ridge.mul(lambda_l2));
    }
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite gradient."));
    }

    const Series w_step = weights.sub(grad.mul(step));
    const Series w_next = apply_l1_prox(w_step, lambda_l1 * step, fit_intercept);
    if (!all_finite(w_next)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
    }

    const Series next_pred = design.matmul(w_next.reshape({p, 1})).reshape({n});
    const Series next_residual = next_pred.sub(labels);
    const Series next_abs = next_residual.abs();
    const Series quadratic = next_abs.clamp_max(robust_transition);
    const Series linear = next_abs.sub(quadratic);
    const Series huber_loss = quadratic.square().mul(0.5).add(linear.mul(robust_transition)).mean();
    const double loss = huber_loss.host<double>().front() + linear_l2_term(w_next, fit_intercept, lambda_l2) + linear_l1_term(w_next, fit_intercept, lambda_l1);
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite objective."));
    }

    const double step_inf = w_next.sub(weights).abs().max().reshape({1}).host<double>().front();
    if (step_inf <= tolerance || std::abs(prev_loss - loss) <= tolerance) {
      weights = w_next;
      break;
    }
    weights = w_next;
    prev_loss = loss;
  }
  return StatusOr<Series>(weights);
}

[[nodiscard]] StatusOr<Series> solve_bernoulli_map(const Series& design,
                                                   const Series& labels01,
                                                   bool fit_intercept,
                                                   double lambda_l2,
                                                   std::int64_t max_iterations,
                                                   double tolerance,
                                                   const char* where) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t p = design.sizes()[1];
  if (n <= 0 || p <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty design matrix."));
  }
  Series y = labels01.to(ScalarType::Float64).reshape({n}).clamp(0.0, 1.0);
  Series weights = pluss::series::zeros({p}, f64_on(design.platform()));
  const Series design_t = design.transpose(0, 1);
  const Series eye = pluss::series::eye(p, p, f64_on(design.platform()));
  const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, design.platform());

  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const Series logits = design.matmul(weights.reshape({p, 1})).reshape({n});
    const Series probs = logits.sigmoid().clamp(1e-9, 1.0 - 1e-9);
    const Series err = probs.sub(y);
    Series grad = design_t.matmul(err.reshape({n, 1})).div(static_cast<double>(n)).reshape({p});
    if (lambda_l2 > 0.0) {
      Series ridge = weights;
      if (fit_intercept) {
        ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
      }
      grad = grad.add(ridge.mul(lambda_l2));
    }
    const Series w_diag = probs.mul(probs.mul(-1.0).add(1.0)).clamp_min(1e-8).reshape({n, 1});
    Series hessian = design_t.matmul(design.mul(w_diag)).div(static_cast<double>(n));
    if (lambda_l2 > 0.0) {
      Series reg = eye.mul(lambda_l2);
      if (fit_intercept) {
        reg = pluss::index_put(reg, std::vector<Series>{idx0, idx0}, pluss::series::zeros({1}, f64_on(reg.platform())), false);
      }
      hessian = hessian.add(reg);
    }
    const Series delta =
        pluss::subsp::math::linalg::pinverse(hessian.add(eye.mul(1e-12)), 1e-12).matmul(grad.reshape({p, 1})).reshape({p});
    const Series next = weights.sub(delta);
    if (!all_finite(next)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
    }
    const Series p_next = design.matmul(next.reshape({p, 1})).reshape({n}).sigmoid().clamp(1e-9, 1.0 - 1e-9);
    const Series bce = y.mul(p_next.log()).add(y.mul(-1.0).add(1.0).mul(p_next.mul(-1.0).add(1.0).log())).mul(-1.0);
    Series ridge = next;
    if (fit_intercept) {
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    const double loss = bce.mean().item<double>() + 0.5 * lambda_l2 * ridge.square().sum().item<double>();
    const double step = delta.abs().max().reshape({1}).host<double>().front();
    weights = next;
    if (step <= tolerance || std::abs(prev_loss - loss) <= tolerance) break;
    prev_loss = loss;
  }
  return StatusOr<Series>(weights);
}

[[nodiscard]] StatusOr<Series> regression_metrics(const Series& truth,
                                                  const Series& prediction,
                                                  const std::vector<MetricKind>& requested,
                                                  double quantile) {
  const Series y = truth.to(ScalarType::Float64).reshape({truth.numel()});
  const Series yhat = prediction.to(ScalarType::Float64).reshape({prediction.numel()});
  if (y.numel() != yhat.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("assess regression metrics require equal truth/prediction lengths."));
  }
  if (y.numel() == 0) {
    return StatusOr<Series>(Status::invalid_argument("assess regression metrics require at least one sample."));
  }
  if (!(quantile > 0.0 && quantile < 1.0)) {
    return StatusOr<Series>(Status::invalid_argument("AssessOptions.regression_quantile must satisfy 0 < q < 1."));
  }

  const Series err = yhat.sub(y);
  const Series residual = y.sub(yhat);
  const Series sse = err.square().sum();
  const Series mse = err.square().mean();
  const Series mae = err.abs().mean();
  const Series neg_indicator = residual.lt(0.0).to(ScalarType::Float64);
  const Series quantile_loss = residual.mul(quantile).sub(residual.mul(neg_indicator)).mean();
  const Series y_centered = y.sub(y.mean());
  const Series sst = y_centered.square().sum();
  const Series zero = pluss::series::zeros_like(sst, ScalarType::Float64);
  const Series one = pluss::series::ones_like(sst, ScalarType::Float64);
  const Series sse_small = sse.le(std::numeric_limits<double>::epsilon()).to(ScalarType::Float64);
  const Series r2 = sst.gt(std::numeric_limits<double>::epsilon())
                        .where(one.sub(sse.div(sst)),
                               sse_small.where(one, zero));

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Mse:
        out.push_back(mse.reshape({1}));
        break;
      case MetricKind::Rmse:
        out.push_back(mse.sqrt().reshape({1}));
        break;
      case MetricKind::Mae:
        out.push_back(mae.reshape({1}));
        break;
      case MetricKind::R2:
        out.push_back(r2.reshape({1}));
        break;
      case MetricKind::QuantileLoss:
        out.push_back(quantile_loss.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported regression metric in assess()."));
    }
  }
  if (out.empty()) {
    return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(y.platform())));
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(y.platform(), ScalarType::Float64));
}

[[nodiscard]] Status validate_linear_regression_options(const LinearFamilyOptions& options) {
  if (options.objective != LinearObjective::LeastSquares
      && options.objective != LinearObjective::RobustLeast) {
    return Status::not_implemented("Linear regressor currently supports LinearObjective::LeastSquares or LinearObjective::RobustLeast.");
  }
  if (options.regularization != RegularizationKind::None
      && options.regularization != RegularizationKind::L1
      && options.regularization != RegularizationKind::L2
      && options.regularization != RegularizationKind::Elastic) {
    return Status::invalid_argument("Linear regressor received unknown RegularizationKind.");
  }
  if (!std::isfinite(options.regularization_strength) || options.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  if (!std::isfinite(options.elastic_l1_ratio) || options.elastic_l1_ratio < 0.0 || options.elastic_l1_ratio > 1.0) {
    return Status::invalid_argument("LinearFamilyOptions.elastic_l1_ratio must be finite and in [0, 1].");
  }
  if (!(options.robust_transition > 0.0) || !std::isfinite(options.robust_transition)) {
    return Status::invalid_argument("LinearFamilyOptions.robust_transition must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_neighbor_regression_options(const NeighborFamilyOptions& options) {
  if (options.k <= 0) {
    return Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions.k > 0.");
  }
  if (!std::isfinite(options.radius) || options.radius < 0.0) {
    return Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions.radius to be finite and >= 0.");
  }
  if (options.radius_fallback_k <= 0) {
    return Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions.radius_fallback_k > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_tree_regression_options(const TreeFamilyOptions& options) {
  if (options.criterion != TreeCriterion::Mse
      && options.criterion != TreeCriterion::Mae) {
    return Status::not_implemented("Tree regressor currently supports TreeCriterion::Mse or TreeCriterion::Mae.");
  }
  if (options.max_depth < -1) {
    return Status::invalid_argument("TreeFamilyOptions.max_depth must be -1 (unlimited) or >= 0.");
  }
  if (options.min_samples_leaf <= 0) {
    return Status::invalid_argument("TreeFamilyOptions.min_samples_leaf must be positive.");
  }
  if (options.min_samples_split < 2) {
    return Status::invalid_argument("TreeFamilyOptions.min_samples_split must be >= 2.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_kernel_regression_options(const KernelFamilyOptions& options) {
  if (!std::isfinite(options.nu) || options.nu <= 0.0 || options.nu > 1.0) {
    return Status::invalid_argument("KernelFamilyOptions.nu must be finite and in (0, 1].");
  }
  if (options.form == KernelForm::OneClass) {
    return Status::not_implemented("Kernel regressor does not support KernelForm::OneClass. Use TaskKind::Transform.");
  }
  if (!std::isfinite(options.c) || options.c <= 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.c must be finite and > 0.");
  }
  if (!std::isfinite(options.gamma) || options.gamma < 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.gamma must be finite and >= 0.");
  }
  if (!std::isfinite(options.epsilon) || options.epsilon < 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.epsilon must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_bayes_regression_options(const BayesFamilyOptions& options) {
  if (!std::isfinite(options.smoothing) || options.smoothing <= 0.0) {
    return Status::invalid_argument("BayesFamilyOptions.smoothing must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] BayesLikelihood resolved_bayes_likelihood(const BayesFamilyOptions& options) {
  if (!options.use_gaussian && options.likelihood == BayesLikelihood::Gaussian) {
    return BayesLikelihood::Bernoulli;
  }
  return options.likelihood;
}

[[nodiscard]] Status validate_ensemble_regression_options(const EnsembleFamilyOptions& options) {
  if (options.method != EnsembleMethod::Bagging
      && options.method != EnsembleMethod::RandomForest
      && options.method != EnsembleMethod::GradientBoost
      && options.method != EnsembleMethod::AdaBoost) {
    return Status::not_implemented(
        "Ensemble regressor currently supports EnsembleMethod::Bagging, EnsembleMethod::RandomForest, EnsembleMethod::GradientBoost, and EnsembleMethod::AdaBoost.");
  }
  if (options.learners <= 0) {
    return Status::invalid_argument("EnsembleFamilyOptions.learners must be positive.");
  }
  if (!std::isfinite(options.subsample) || options.subsample <= 0.0 || options.subsample > 1.0) {
    return Status::invalid_argument("EnsembleFamilyOptions.subsample must be finite and in (0, 1].");
  }
  if (!std::isfinite(options.learning_rate) || options.learning_rate <= 0.0) {
    return Status::invalid_argument("EnsembleFamilyOptions.learning_rate must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] Series standardize_fit_rows(const Series& x_rows, Series& mean, Series& scale) {
  const std::int64_t feature_count = x_rows.sizes()[1];
  mean = x_rows.mean(0, false).reshape({feature_count});
  const Series centered = x_rows.sub(mean.reshape({1, feature_count}));
  scale = centered.square().mean(0, false).reshape({feature_count}).clamp_min(1e-12).sqrt();
  return centered.div(scale.reshape({1, feature_count}));
}

[[nodiscard]] Series standardize_apply_rows(const Series& x_rows, const Series& mean, const Series& scale) {
  const std::int64_t feature_count = x_rows.sizes()[1];
  return x_rows.sub(mean.reshape({1, feature_count})).div(scale.reshape({1, feature_count}));
}

[[nodiscard]] Series pairwise_distance2(const Series& query_rows, const Series& train_rows) {
  const Series dot = query_rows.matmul(train_rows.transpose(0, 1));
  const Series q_norm = query_rows.square().sum(1, true);
  const Series t_norm = train_rows.square().sum(1, true).transpose(0, 1);
  return q_norm.add(t_norm).sub(dot.mul(2.0)).relu();
}

[[nodiscard]] Series knn_predict_cpu(const Series& query_rows,
                                     const Series& train_rows,
                                     const Series& y_train,
                                     std::int64_t k,
                                     bool distance_weighted) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const auto yv = y_train.to(pluss::pCPU, ScalarType::Float64).reshape({y_train.numel()}).contiguous().host<double>().array().data;
  const std::int64_t kk = std::max<std::int64_t>(1, std::min<std::int64_t>(k, nt));
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  constexpr double eps = 1e-12;

  // SpatialIndex (a real KD-tree) replaces the brute-force triple-nested
  // distance loop + per-query max-heap below -- see the identical migration
  // note on family_classifier.cpp's knn_probability_cpu.
  StatusOr<SpatialIndex> index_or = SpatialIndex::build(train_rows);
  if (!index_or.ok()) return pluss::make_series(out, {nq}, f64_cpu());
  const SpatialIndex index = index_or.move_value();
  StatusOr<KNearestResult> knn_or = index.query_k(query_rows, kk);
  if (!knn_or.ok()) return pluss::make_series(out, {nq}, f64_cpu());
  const KNearestResult knn = knn_or.move_value();
  const auto knn_idx = knn.indices.to(pluss::pCPU, ScalarType::Int64).reshape({knn.indices.numel()}).host<std::int64_t>().array().data;
  const auto knn_d2 = knn.distances2.to(pluss::pCPU, ScalarType::Float64).reshape({knn.distances2.numel()}).host<double>().array().data;

  for (std::int64_t qi = 0; qi < nq; ++qi) {
    std::vector<std::pair<double, std::int64_t>> best{};
    best.reserve(static_cast<std::size_t>(kk));
    for (std::int64_t j = 0; j < kk; ++j) {
      const std::size_t pos = static_cast<std::size_t>(qi * kk + j);
      best.emplace_back(knn_d2[pos], knn_idx[pos]);
    }

    if (distance_weighted) {
      double zero_sum = 0.0;
      double zero_count = 0.0;
      for (const auto& [d2, ti] : best) {
        if (d2 <= eps) {
          zero_sum += yv[static_cast<std::size_t>(ti)];
          zero_count += 1.0;
        }
      }
      if (zero_count > 0.0) {
        out[static_cast<std::size_t>(qi)] = zero_sum / zero_count;
        continue;
      }

      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [d2, ti] : best) {
        const double w = 1.0 / std::sqrt(d2 + eps);
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? ysum / wsum : 0.0;
    } else {
      double sum = 0.0;
      for (const auto& [_, ti] : best) {
        sum += yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = sum / static_cast<double>(kk);
    }
  }

  return pluss::make_series(out, {nq}, f64_cpu());
}

[[nodiscard]] Series radius_predict_cpu(const Series& query_rows,
                                        const Series& train_rows,
                                        const Series& y_train,
                                        double radius,
                                        std::int64_t fallback_k,
                                        bool distance_weighted) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
  const auto yv = y_train.to(pluss::pCPU, ScalarType::Float64).reshape({y_train.numel()}).contiguous().host<double>().array().data;
  const std::int64_t kk = std::max<std::int64_t>(1, std::min<std::int64_t>(fallback_k, nt));
  const double radius2 = radius * radius;
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  constexpr double eps = 1e-12;

  for (std::int64_t qi = 0; qi < nq; ++qi) {
    const std::size_t qbase = static_cast<std::size_t>(qi * d);
    std::vector<std::pair<double, std::int64_t>> in_radius{};
    in_radius.reserve(static_cast<std::size_t>(nt));
    std::vector<std::pair<double, std::int64_t>> best{};
    best.reserve(static_cast<std::size_t>(kk));
    for (std::int64_t ti = 0; ti < nt; ++ti) {
      const std::size_t tbase = static_cast<std::size_t>(ti * d);
      double d2 = 0.0;
      for (std::int64_t f = 0; f < d; ++f) {
        const double dv = qv[qbase + static_cast<std::size_t>(f)] - tv[tbase + static_cast<std::size_t>(f)];
        d2 += dv * dv;
      }
      if (d2 <= radius2) in_radius.emplace_back(d2, ti);
      if (static_cast<std::int64_t>(best.size()) < kk) {
        best.emplace_back(d2, ti);
        if (static_cast<std::int64_t>(best.size()) == kk) std::make_heap(best.begin(), best.end());
      } else if (d2 < best.front().first) {
        std::pop_heap(best.begin(), best.end());
        best.back() = std::pair<double, std::int64_t>{d2, ti};
        std::push_heap(best.begin(), best.end());
      }
    }
    const auto& selected = in_radius.empty() ? best : in_radius;
    if (distance_weighted) {
      double zero_sum = 0.0;
      double zero_count = 0.0;
      for (const auto& [d2, ti] : selected) {
        if (d2 <= eps) {
          zero_sum += yv[static_cast<std::size_t>(ti)];
          zero_count += 1.0;
        }
      }
      if (zero_count > 0.0) {
        out[static_cast<std::size_t>(qi)] = zero_sum / zero_count;
        continue;
      }
      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [d2, ti] : selected) {
        const double w = 1.0 / std::sqrt(d2 + eps);
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? ysum / wsum : 0.0;
    } else {
      double sum = 0.0;
      for (const auto& [_, ti] : selected) sum += yv[static_cast<std::size_t>(ti)];
      out[static_cast<std::size_t>(qi)] = selected.empty() ? 0.0 : sum / static_cast<double>(selected.size());
    }
  }
  return pluss::make_series(out, {nq}, f64_cpu());
}

[[nodiscard]] Series knn_predict_series(const Series& query_rows,
                                        const Series& train_rows,
                                        const Series& y_train,
                                        std::int64_t k,
                                        bool distance_weighted) {
  const std::int64_t sample_count = train_rows.sizes()[0];
  const std::int64_t query_count = query_rows.sizes()[0];
  const std::int64_t neighbor_count = std::max<std::int64_t>(1, std::min<std::int64_t>(k, sample_count));
  const Series dist2 = pairwise_distance2(query_rows, train_rows);
  const Series sort_idx = dist2.argsort(1, false, true);
  const Series k_cols =
      pluss::series::arange(0, neighbor_count, Config{}.with_dtype(ScalarType::Int64).with_platform(sort_idx.platform()));
  const Series knn_idx = sort_idx.index_select(1, k_cols);
  const Series y_train_2d =
      y_train.reshape({1, sample_count}).expand(std::vector<std::int64_t>{query_count, sample_count});
  const Series y_knn = y_train_2d.gather(1, knn_idx);

  if (!distance_weighted) {
    return y_knn.mean(1, false);
  }

  constexpr double eps = 1e-12;
  const Series d2_knn = dist2.gather(1, knn_idx);
  const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
  const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
  const Series inv_dist = pluss::series::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
  const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist));
  const Series wsum = weights.sum(1, false);
  const Series ysum = weights.mul(y_knn).sum(1, false);
  return wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64));
}

[[nodiscard]] Series radius_predict_series(const Series& query_rows,
                                           const Series& train_rows,
                                           const Series& y_train,
                                           double radius,
                                           std::int64_t fallback_k,
                                           bool distance_weighted) {
  const std::int64_t sample_count = train_rows.sizes()[0];
  const std::int64_t query_count = query_rows.sizes()[0];
  const Series dist2 = pairwise_distance2(query_rows, train_rows);
  const Series mask = dist2.le(radius * radius).to(ScalarType::Float64);
  const Series in_count = mask.sum(1, false).reshape({query_count});
  const Series fallback = knn_predict_series(query_rows, train_rows, y_train, fallback_k, distance_weighted);
  const Series y_train_2d =
      y_train.reshape({1, sample_count}).expand(std::vector<std::int64_t>{query_count, sample_count});

  Series radius_pred = pluss::series::undefined();
  if (distance_weighted) {
    constexpr double eps = 1e-12;
    const Series zero_mask = dist2.le(eps).to(ScalarType::Float64).mul(mask);
    const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
    const Series inv_dist = pluss::series::ones_like(dist2, ScalarType::Float64).div(dist2.sqrt().add(eps));
    const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist).mul(mask));
    const Series wsum = weights.sum(1, false);
    const Series ysum = weights.mul(y_train_2d).sum(1, false);
    radius_pred = wsum.gt(0.0).where(ysum.div(wsum), fallback);
  } else {
    const Series ysum = y_train_2d.mul(mask).sum(1, false);
    radius_pred = in_count.gt(0.0).where(ysum.div(in_count.clamp_min(1.0)), fallback);
  }

  return in_count.gt(0.0).where(radius_pred, fallback);
}

class NeighborRegressor final : public RegressorBase {
public:
  explicit NeighborRegressor(NeighborFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_neighbor_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "NeighborRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "NeighborRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "NeighborRegressor::learn");
    if (!y_or.ok()) return y_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    query_mode_ = options_.query_mode;
    radius_ = options_.radius;
    radius_fallback_k_ = std::min<std::int64_t>(options_.radius_fallback_k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborRegressor requires at least one training sample.");
    }

    Series x_train = matrix.x_rows;
    if (options_.standardize) {
      x_train = standardize_fit_rows(x_train, mean_, scale_);
    } else {
      mean_ = pluss::series::undefined();
      scale_ = pluss::series::undefined();
    }
    x_train_ = std::move(x_train);
    y_train_ = y_or.value().reshape({sample_count_});

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "NeighborRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "NeighborRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series x_query = matrix.x_rows;
    if (options_.standardize) {
      x_query = standardize_apply_rows(x_query, mean_, scale_);
    }

    Series pred = pluss::series::undefined();
    if (query_mode_ == NeighborQueryMode::Radius) {
      pred = radius_predict_series(x_query,
                                   x_train_,
                                   y_train_,
                                   radius_,
                                   radius_fallback_k_,
                                   options_.distance_weighted);
    } else {
      const std::int64_t neighbor_count = std::min<std::int64_t>(k_, sample_count_);
      if (x_query.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
        pred = knn_predict_cpu(x_query, x_train_, y_train_, neighbor_count, options_.distance_weighted);
      } else {
        const Series dist2 = pairwise_distance2(x_query, x_train_);
        const Series sort_idx = dist2.argsort(1, false, true);
        const Series k_cols =
            pluss::series::arange(0, neighbor_count, Config{}.with_dtype(ScalarType::Int64).with_platform(sort_idx.platform()));
        const Series knn_idx = sort_idx.index_select(1, k_cols);
        const Series y_train_2d =
            y_train_.reshape({1, sample_count_}).expand(std::vector<std::int64_t>{matrix.sample_count, sample_count_});
        const Series y_knn = y_train_2d.gather(1, knn_idx);

        if (options_.distance_weighted) {
          constexpr double eps = 1e-12;
          const Series d2_knn = dist2.gather(1, knn_idx);
          const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
          const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
          const Series inv_dist = pluss::series::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
          const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist));
          const Series wsum = weights.sum(1, false);
          const Series ysum = weights.mul(y_knn).sum(1, false);
          pred = wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64));
        } else {
          pred = y_knn.mean(1, false);
        }
      }
    }

    return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("NeighborRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(sample_count_),
                   static_cast<double>(k_),
                   static_cast<double>(static_cast<std::int64_t>(query_mode_)),
                   radius_,
                   static_cast<double>(radius_fallback_k_),
                   2.0}),
        x_train_,
        y_train_,
        mean_,
        scale_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 5) return Status::invalid_argument("NeighborRegressor::load_state expects 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("NeighborRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    sample_count_ = static_cast<std::int64_t>(meta[2]);
    k_ = static_cast<std::int64_t>(meta[3]);
    const bool v2_state = meta.size() >= 9 && meta[8] >= 2.0;
    query_mode_ = v2_state && meta.size() >= 5
                      ? static_cast<NeighborQueryMode>(static_cast<std::int64_t>(meta[4]))
                      : NeighborQueryMode::KNearest;
    radius_ = v2_state && meta.size() >= 6 ? meta[5] : options_.radius;
    radius_fallback_k_ = v2_state && meta.size() >= 7
                             ? static_cast<std::int64_t>(meta[6])
                             : std::max<std::int64_t>(1, std::min<std::int64_t>(options_.radius_fallback_k, sample_count_));
    x_train_ = state[1];
    y_train_ = state[2];
    mean_ = state[3];
    scale_ = state[4];
    return Status::success();
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  NeighborFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::int64_t k_{1};
  NeighborQueryMode query_mode_{NeighborQueryMode::KNearest};
  double radius_{1.0};
  std::int64_t radius_fallback_k_{1};
  Series x_train_ = pluss::series::undefined();
  Series y_train_ = pluss::series::undefined();
  Series mean_ = pluss::series::undefined();
  Series scale_ = pluss::series::undefined();
};

class TreeRegressor final : public RegressorBase {
public:
  explicit TreeRegressor(TreeFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_tree_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "TreeRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "TreeRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "TreeRegressor::learn currently has a CPU-only implementation. Provide CPU features for tree learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "TreeRegressor::learn");
    if (!y_or.ok()) return y_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    x_train_host_ = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
    y_train_host_ = y_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({sample_count_}).contiguous().host<double>().array().data;

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeRegressor::learn failed to build any tree nodes.");
    }

    x_train_host_.clear();
    y_train_host_.clear();

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "TreeRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "TreeRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "TreeRegressor::infer currently has a CPU-only implementation. Provide CPU features for tree inference."));
    }

    std::vector<double> pred{};
    Status pred_status = predict_host_rows(
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data,
        matrix.sample_count,
        matrix.feature_count,
        pred);
    if (!pred_status.ok()) return StatusOr<Series>(std::move(pred_status));

    Series out = pluss::make_series(pred, {static_cast<std::int64_t>(pred.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  Status predict_host_rows(const std::vector<double>& x_rows,
                           std::int64_t sample_count,
                           std::int64_t feature_count,
                           std::vector<double>& out) const {
    if (!trained_) return Status::precondition("TreeRegressor must be learned before host prediction.");
    if (feature_count != feature_count_) return Status::shape_mismatch("TreeRegressor host prediction feature mismatch.");
    const std::size_t expected = static_cast<std::size_t>(sample_count * feature_count);
    if (x_rows.size() != expected) {
      return Status::shape_mismatch("TreeRegressor host prediction row-major size mismatch.");
    }
    out.assign(static_cast<std::size_t>(sample_count), 0.0);
    for (std::int64_t i = 0; i < sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_rows[static_cast<std::size_t>(i * feature_count + node.feature)];
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      out[static_cast<std::size_t>(i)] = nodes_[static_cast<std::size_t>(node_idx)].leaf_value;
    }
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("TreeRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<double> flat{};
    flat.reserve(nodes_.size() * 6);
    for (const auto& node : nodes_) {
      flat.push_back(node.is_leaf ? 1.0 : 0.0);
      flat.push_back(static_cast<double>(node.feature));
      flat.push_back(node.threshold);
      flat.push_back(static_cast<double>(node.left));
      flat.push_back(static_cast<double>(node.right));
      flat.push_back(node.leaf_value);
    }
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(sample_count_),
                   static_cast<double>(nodes_.size())}),
        state_f64(std::move(flat)),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("TreeRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("TreeRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    sample_count_ = static_cast<std::int64_t>(meta[2]);
    const std::size_t node_count = static_cast<std::size_t>(std::max<double>(0.0, meta[3]));
    const auto flat = vec_f64(state[1]);
    nodes_.assign(node_count, TreeNode{});
    for (std::size_t i = 0; i < node_count; ++i) {
      const std::size_t base = i * 6;
      if (base + 5 >= flat.size()) break;
      TreeNode node{};
      node.is_leaf = flat[base] > 0.5;
      node.feature = static_cast<std::int64_t>(flat[base + 1]);
      node.threshold = flat[base + 2];
      node.left = static_cast<std::int64_t>(flat[base + 3]);
      node.right = static_cast<std::int64_t>(flat[base + 4]);
      node.leaf_value = flat[base + 5];
      nodes_[i] = node;
    }
    x_train_host_.clear();
    y_train_host_.clear();
    return Status::success();
  }

private:
  struct TreeNode {
    bool is_leaf = true;
    std::int64_t feature = -1;
    double threshold = 0.0;
    std::int64_t left = -1;
    std::int64_t right = -1;
    double leaf_value = 0.0;
  };

  struct SplitCandidate {
    bool valid = false;
    std::int64_t feature = -1;
    double threshold = 0.0;
    double gain = 0.0;
  };

  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  [[nodiscard]] double x_train_at(std::int64_t row, std::int64_t feature) const {
    return x_train_host_[static_cast<std::size_t>(row * feature_count_ + feature)];
  }

  [[nodiscard]] double y_train_at(std::int64_t row) const {
    return y_train_host_[static_cast<std::size_t>(row)];
  }

  [[nodiscard]] double median_of_values(std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    const std::size_t n = values.size();
    const std::size_t mid = n / 2;
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid), values.end());
    const double upper = values[mid];
    if ((n & 1U) != 0U) return upper;
    const double lower = *std::max_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid));
    return 0.5 * (lower + upper);
  }

  [[nodiscard]] std::pair<double, double> median_and_sad_of_values(std::vector<double> values) const {
    const double median = median_of_values(values);
    double sad = 0.0;
    for (double v : values) sad += std::abs(v - median);
    return {median, sad};
  }

  [[nodiscard]] std::pair<double, double> leaf_value_and_impurity(const std::vector<std::int64_t>& indices) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    if (n <= 0) return {0.0, 0.0};
    if (options_.criterion == TreeCriterion::Mse) {
      double sum = 0.0;
      double sumsq = 0.0;
      for (std::int64_t idx : indices) {
        const double yi = y_train_at(idx);
        sum += yi;
        sumsq += yi * yi;
      }
      const double mean = sum / static_cast<double>(n);
      const double sse = sumsq - (sum * sum) / static_cast<double>(n);
      const double impurity = std::max(0.0, sse / static_cast<double>(n));
      return {mean, impurity};
    }

    std::vector<double> y_values{};
    y_values.reserve(static_cast<std::size_t>(n));
    for (std::int64_t idx : indices) y_values.push_back(y_train_at(idx));
    const auto [median, sad] = median_and_sad_of_values(std::move(y_values));
    return {median, sad / static_cast<double>(n)};
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    const auto [leaf_value, parent_impurity] = leaf_value_and_impurity(indices);

    TreeNode node{};
    node.leaf_value = leaf_value;

    const std::int64_t node_index = static_cast<std::int64_t>(nodes_.size());
    nodes_.push_back(node);

    const bool depth_limited = options_.max_depth >= 0 && depth >= options_.max_depth;
    const bool size_limited = n < options_.min_samples_split || n < (2 * options_.min_samples_leaf);
    constexpr double impurity_floor = 1e-14;
    if (depth_limited || size_limited || parent_impurity <= impurity_floor) {
      return node_index;
    }

    SplitCandidate best = find_best_split(indices, parent_impurity);
    if (!best.valid || best.gain <= impurity_floor) {
      return node_index;
    }

    std::vector<std::int64_t> left{};
    std::vector<std::int64_t> right{};
    left.reserve(indices.size());
    right.reserve(indices.size());
    for (std::int64_t idx : indices) {
      if (x_train_at(idx, best.feature) < best.threshold) {
        left.push_back(idx);
      } else {
        right.push_back(idx);
      }
    }
    if (left.empty() || right.empty()) {
      return node_index;
    }

    TreeNode& split_node = nodes_[static_cast<std::size_t>(node_index)];
    split_node.is_leaf = false;
    split_node.feature = best.feature;
    split_node.threshold = best.threshold;
    split_node.left = build_node(left, depth + 1);
    split_node.right = build_node(right, depth + 1);
    return node_index;
  }

  [[nodiscard]] bool split_better(const SplitCandidate& candidate, const SplitCandidate& incumbent) const {
    if (!candidate.valid) return false;
    if (!incumbent.valid) return true;
    const bool better_gain = candidate.gain > incumbent.gain + 1e-15;
    const bool tie_break = std::abs(candidate.gain - incumbent.gain) <= 1e-15
                           && (candidate.feature < incumbent.feature
                               || (candidate.feature == incumbent.feature && candidate.threshold < incumbent.threshold));
    return better_gain || tie_break;
  }

  [[nodiscard]] SplitCandidate find_best_split_for_feature(const std::vector<std::int64_t>& indices,
                                                           double parent_impurity,
                                                           std::int64_t feature) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    SplitCandidate best{};
    best.feature = feature;

    std::vector<std::pair<double, double>> values{};
    values.reserve(static_cast<std::size_t>(n));
    double total_sum = 0.0;
    double total_sumsq = 0.0;
    for (std::int64_t idx : indices) {
      const double yi = y_train_at(idx);
      if (options_.criterion == TreeCriterion::Mse) {
        total_sum += yi;
        total_sumsq += yi * yi;
      }
      values.emplace_back(x_train_at(idx, feature), yi);
    }
    std::sort(values.begin(), values.end(),
              [](const auto& a, const auto& b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second < b.second;
              });
    if (values.front().first == values.back().first) return best;

    if (options_.criterion == TreeCriterion::Mse) {
      double left_sum = 0.0;
      double left_sumsq = 0.0;
      for (std::int64_t i = 0; i < n - 1; ++i) {
        const double yi = values[static_cast<std::size_t>(i)].second;
        left_sum += yi;
        left_sumsq += yi * yi;
        const std::int64_t left_n = i + 1;
        const std::int64_t right_n = n - left_n;
        if (left_n < options_.min_samples_leaf || right_n < options_.min_samples_leaf) continue;

        const double xi = values[static_cast<std::size_t>(i)].first;
        const double xj = values[static_cast<std::size_t>(i + 1)].first;
        if (xi == xj) continue;

        const double right_sum = total_sum - left_sum;
        const double right_sumsq = total_sumsq - left_sumsq;
        const double left_sse = left_sumsq - (left_sum * left_sum) / static_cast<double>(left_n);
        const double right_sse = right_sumsq - (right_sum * right_sum) / static_cast<double>(right_n);
        const double weighted_impurity = std::max(0.0, (left_sse + right_sse) / static_cast<double>(n));
        const double gain = parent_impurity - weighted_impurity;
        const double threshold = xi + (xj - xi) * 0.5;

        const SplitCandidate candidate{
            .valid = true,
            .feature = feature,
            .threshold = threshold,
            .gain = gain,
        };
        if (split_better(candidate, best)) {
          best = candidate;
        }
      }
    } else {
      std::vector<double> left_values{};
      std::vector<double> right_values{};
      left_values.reserve(static_cast<std::size_t>(n));
      right_values.reserve(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n - 1; ++i) {
        const std::int64_t left_n = i + 1;
        const std::int64_t right_n = n - left_n;
        if (left_n < options_.min_samples_leaf || right_n < options_.min_samples_leaf) continue;

        const double xi = values[static_cast<std::size_t>(i)].first;
        const double xj = values[static_cast<std::size_t>(i + 1)].first;
        if (xi == xj) continue;

        left_values.clear();
        right_values.clear();
        for (std::int64_t j = 0; j <= i; ++j) {
          left_values.push_back(values[static_cast<std::size_t>(j)].second);
        }
        for (std::int64_t j = i + 1; j < n; ++j) {
          right_values.push_back(values[static_cast<std::size_t>(j)].second);
        }

        const auto [left_median, left_sad] = median_and_sad_of_values(std::move(left_values));
        const auto [right_median, right_sad] = median_and_sad_of_values(std::move(right_values));
        (void)left_median;
        (void)right_median;
        const double weighted_impurity = (left_sad + right_sad) / static_cast<double>(n);
        const double gain = parent_impurity - weighted_impurity;
        const double threshold = xi + (xj - xi) * 0.5;

        const SplitCandidate candidate{
            .valid = true,
            .feature = feature,
            .threshold = threshold,
            .gain = gain,
        };
        if (split_better(candidate, best)) {
          best = candidate;
        }
      }
    }
    return best;
  }

  [[nodiscard]] SplitCandidate find_best_split(const std::vector<std::int64_t>& indices, double parent_impurity) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    SplitCandidate best{};
    const bool parallel_feature_scan = feature_count_ >= 8 && n >= 1024;
    const std::int64_t workers = parallel_feature_scan ? resolved_parallel_workers(feature_count_) : 1;
    if (workers <= 1) {
      for (std::int64_t f = 0; f < feature_count_; ++f) {
        const SplitCandidate candidate = find_best_split_for_feature(indices, parent_impurity, f);
        if (split_better(candidate, best)) {
          best = candidate;
        }
      }
      return best;
    }

    const std::int64_t chunk = std::max<std::int64_t>(1, (feature_count_ + workers - 1) / workers);
    std::vector<std::future<SplitCandidate>> futures{};
    for (std::int64_t start = 0; start < feature_count_; start += chunk) {
      const std::int64_t end = std::min(feature_count_, start + chunk);
      futures.emplace_back(std::async(std::launch::async, [this, &indices, parent_impurity, start, end]() {
        SplitCandidate local{};
        for (std::int64_t f = start; f < end; ++f) {
          const SplitCandidate candidate = find_best_split_for_feature(indices, parent_impurity, f);
          if (split_better(candidate, local)) {
            local = candidate;
          }
        }
        return local;
      }));
    }
    for (auto& future : futures) {
      const SplitCandidate candidate = future.get();
      if (split_better(candidate, best)) {
        best = candidate;
      }
    }
    return best;
  }

  TreeFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::vector<double> x_train_host_{};
  std::vector<double> y_train_host_{};
  std::vector<TreeNode> nodes_{};
};

class BayesRegressor final : public RegressorBase {
public:
  explicit BayesRegressor(BayesFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_bayes_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "BayesRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "BayesRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "BayesRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    Series y = y_or.move_value().to(ScalarType::Float64).reshape({matrix.sample_count});
    likelihood_ = resolved_bayes_likelihood(options_);
    Series design = prepend_intercept(matrix.x_rows);
    StatusOr<Series> beta_or = [&]() -> StatusOr<Series> {
      if (likelihood_ == BayesLikelihood::Gaussian) {
        return solve_ridge_least_squares(design, y, options_.smoothing, true, pinv_rcond_);
      }
      if (likelihood_ == BayesLikelihood::Bernoulli) {
        if (!all_finite(y) || y.lt(0.0).any().item<bool>() || y.gt(1.0).any().item<bool>()) {
          return StatusOr<Series>(Status::invalid_argument(
              "BayesRegressor::learn Bernoulli likelihood expects labels in [0, 1]."));
        }
        return solve_bernoulli_map(design,
                                   y,
                                   true,
                                   options_.smoothing,
                                   options.max_iterations,
                                   options.tolerance,
                                   "BayesRegressor::learn");
      }
      if (likelihood_ == BayesLikelihood::Multinomial) {
        if (matrix.x_rows.lt(0.0).any().item<bool>()) {
          return StatusOr<Series>(
              Status::invalid_argument("BayesRegressor::learn Multinomial likelihood expects non-negative features."));
        }
        if (!all_finite(y) || y.lt(0.0).any().item<bool>()) {
          return StatusOr<Series>(
              Status::invalid_argument("BayesRegressor::learn Multinomial likelihood expects finite non-negative labels."));
        }
        const Series y_log = y.add(1.0).log();
        return solve_ridge_least_squares(design, y_log, options_.smoothing, true, pinv_rcond_);
      }
      if (likelihood_ == BayesLikelihood::Complement) {
        return StatusOr<Series>(
            Status::not_implemented("BayesRegressor::learn does not support Complement likelihood; use classification."));
      }
      return StatusOr<Series>(Status::invalid_argument("BayesRegressor::learn encountered unsupported BayesLikelihood."));
    }();
    if (!beta_or.ok()) return beta_or.status();

    beta_ = beta_or.move_value();
    feature_count_ = matrix.feature_count;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "BayesRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(
          Status::invalid_argument("BayesRegressor::infer does not support NaN features; apply the same fitted imputer used for learn()."));
    }

    Series design = prepend_intercept(matrix.x_rows);
    if (likelihood_ == BayesLikelihood::Multinomial && matrix.x_rows.lt(0.0).any().item<bool>()) {
      return StatusOr<Series>(
          Status::invalid_argument("BayesRegressor::infer Multinomial likelihood expects non-negative features."));
    }
    Series pred = design.matmul(beta_.reshape({beta_.numel(), 1})).reshape({matrix.sample_count});
    if (likelihood_ == BayesLikelihood::Bernoulli) {
      pred = pred.sigmoid();
    } else if (likelihood_ == BayesLikelihood::Multinomial) {
      pred = pred.exp().sub(1.0).clamp_min(0.0);
    }
    if (!all_finite(pred)) {
      return StatusOr<Series>(Status::numerical_failure("BayesRegressor::infer produced non-finite predictions."));
    }
    return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("BayesRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   pinv_rcond_,
                   static_cast<double>(static_cast<std::int64_t>(likelihood_))}),
        beta_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("BayesRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 3) return Status::invalid_argument("BayesRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    pinv_rcond_ = meta[2];
    likelihood_ = meta.size() >= 4
                      ? static_cast<BayesLikelihood>(static_cast<std::int64_t>(meta[3]))
                      : BayesLikelihood::Gaussian;
    beta_ = state[1];
    return Status::success();
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  BayesFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series beta_ = pluss::series::undefined();
  double pinv_rcond_{1e-12};
  BayesLikelihood likelihood_{BayesLikelihood::Gaussian};
};

class EnsembleRegressor final : public RegressorBase {
public:
  explicit EnsembleRegressor(EnsembleFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_ensemble_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "EnsembleRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "EnsembleRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "EnsembleRegressor::learn currently has a CPU-only implementation. Provide CPU features for ensemble learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "EnsembleRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    const Series y_train = y_or.value().reshape({matrix.sample_count});
    const std::vector<double> x_train_host =
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
    const std::vector<double> y_train_host =
        y_train.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    method_ = options_.method;
    learning_rate_ = options_.learning_rate;
    initial_prediction_ = 0.0;

    const std::int64_t learner_count = options_.learners;
    const std::int64_t subsample_count =
        std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(options_.subsample * sample_count_)));
    const std::int64_t mtry = options_.method == EnsembleMethod::RandomForest
                                  ? std::max<std::int64_t>(1, feature_count_ / 3)
                                  : feature_count_;

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(learner_count));
    std::uint64_t base_seed = options.random.seed;
    if (!options.random.deterministic) {
      std::random_device rd;
      base_seed = (static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd());
    }

    auto train_bagged_member = [&](std::int64_t learner_index,
                                   const std::vector<double>& targets) -> StatusOr<EnsembleMember> {
      std::mt19937_64 rng(splitmix64(base_seed + static_cast<std::uint64_t>(learner_index)));
      std::uniform_int_distribution<std::int64_t> row_dist(0, sample_count_ - 1);

      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      if (mtry < feature_count_) {
        std::shuffle(feature_ids.begin(), feature_ids.end(), rng);
        feature_ids.resize(static_cast<std::size_t>(mtry));
        std::sort(feature_ids.begin(), feature_ids.end());
      }

      const std::int64_t learner_features = static_cast<std::int64_t>(feature_ids.size());
      std::vector<std::int64_t> row_ids(static_cast<std::size_t>(subsample_count), 0);
      for (std::int64_t i = 0; i < subsample_count; ++i) {
        row_ids[static_cast<std::size_t>(i)] = row_dist(rng);
      }

      std::vector<double> x_sub_host = project_rows_with_sampling(
          x_train_host, row_ids, sample_count_, feature_count_, feature_ids, learner_features);
      std::vector<double> y_sub_host = gather_rows(targets, row_ids);

      const Series x_series = pluss::make_series(std::move(x_sub_host), {subsample_count, learner_features}, f64_cpu());
      const Series y_series = pluss::make_series(std::move(y_sub_host), {subsample_count}, f64_cpu());
      const LabeledBatch sub_batch{
          .features = FeatureBatch{.x = x_series, .layout = ObservationLayout::Rows},
          .y = y_series,
      };

      auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
      });
      Status tree_status = tree->learn(sub_batch, options);
      if (!tree_status.ok()) {
        return StatusOr<EnsembleMember>(Status::internal(
            "EnsembleRegressor::learn failed while fitting base tree for learner "
            + std::to_string(learner_index) + "."));
      }

      EnsembleMember member{};
      member.tree = std::move(tree);
      member.feature_ids = std::move(feature_ids);
      member.weight = 1.0;
      return StatusOr<EnsembleMember>(std::move(member));
    };

    if (options_.method == EnsembleMethod::GradientBoost) {
      initial_prediction_ =
          std::accumulate(y_train_host.begin(), y_train_host.end(), 0.0) / static_cast<double>(sample_count_);
      std::vector<double> running_prediction(static_cast<std::size_t>(sample_count_), initial_prediction_);
      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      const std::int64_t learner_features = feature_count_;
      std::mt19937_64 rng(splitmix64(base_seed));

      for (std::int64_t l = 0; l < learner_count; ++l) {
        std::vector<std::int64_t> row_ids =
            draw_stochastic_boost_rows(rng, sample_count_, subsample_count);
        std::vector<double> residual_targets(static_cast<std::size_t>(subsample_count), 0.0);
        for (std::int64_t i = 0; i < subsample_count; ++i) {
          const std::int64_t row = row_ids[static_cast<std::size_t>(i)];
          residual_targets[static_cast<std::size_t>(i)] =
              y_train_host[static_cast<std::size_t>(row)] - running_prediction[static_cast<std::size_t>(row)];
        }

        std::vector<double> x_sub_host = project_rows_with_sampling(
            x_train_host, row_ids, sample_count_, feature_count_, feature_ids, learner_features);

        const Series x_series = pluss::make_series(std::move(x_sub_host), {subsample_count, learner_features}, f64_cpu());
        const Series y_series = pluss::make_series(std::move(residual_targets), {subsample_count}, f64_cpu());
        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_series, .layout = ObservationLayout::Rows},
            .y = y_series,
        };

        auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
        });
        Status tree_status = tree->learn(sub_batch, options);
        if (!tree_status.ok()) {
          return Status::internal(
              "EnsembleRegressor::learn failed while fitting gradient-boost tree for learner "
              + std::to_string(l) + ".");
        }

        std::vector<double> update_values{};
        Status pred_status = tree->predict_host_rows(x_train_host, sample_count_, feature_count_, update_values);
        if (!pred_status.ok()) return pred_status;

        const double step = learning_rate_;
        for (std::int64_t i = 0; i < sample_count_; ++i) {
          running_prediction[static_cast<std::size_t>(i)] += step * update_values[static_cast<std::size_t>(i)];
        }

        EnsembleMember member{};
        member.tree = std::move(tree);
        member.feature_ids = feature_ids;
        member.weight = step;
        learners_.push_back(std::move(member));

        double residual_mse = 0.0;
        for (std::int64_t i = 0; i < sample_count_; ++i) {
          const double e = y_train_host[static_cast<std::size_t>(i)] - running_prediction[static_cast<std::size_t>(i)];
          residual_mse += e * e;
        }
        residual_mse /= static_cast<double>(sample_count_);
        if (residual_mse <= options.tolerance) {
          break;
        }
      }
    } else if (options_.method == EnsembleMethod::AdaBoost) {
      std::vector<double> sample_weights(static_cast<std::size_t>(sample_count_), 1.0 / static_cast<double>(sample_count_));
      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      const std::int64_t learner_features = feature_count_;
      std::mt19937_64 rng(splitmix64(base_seed));

      for (std::int64_t l = 0; l < learner_count; ++l) {
        std::vector<std::int64_t> row_ids =
            draw_weighted_bootstrap_rows(rng, sample_weights, sample_count_);
        std::vector<double> y_sub_host = gather_rows(y_train_host, row_ids);
        std::vector<double> x_sub_host = project_rows_with_sampling(
            x_train_host, row_ids, sample_count_, feature_count_, feature_ids, learner_features);

        const Series x_series = pluss::make_series(std::move(x_sub_host), {sample_count_, learner_features}, f64_cpu());
        const Series y_series = pluss::make_series(std::move(y_sub_host), {sample_count_}, f64_cpu());
        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_series, .layout = ObservationLayout::Rows},
            .y = y_series,
        };

        auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
        });
        Status tree_status = tree->learn(sub_batch, options);
        if (!tree_status.ok()) {
          return Status::internal(
              "EnsembleRegressor::learn failed while fitting adaboost tree for learner "
              + std::to_string(l) + ".");
        }

        std::vector<double> pred_values{};
        Status pred_status = tree->predict_host_rows(x_train_host, sample_count_, feature_count_, pred_values);
        if (!pred_status.ok()) return pred_status;

        double max_abs_err = 0.0;
        std::vector<double> abs_err(static_cast<std::size_t>(sample_count_), 0.0);
        for (std::int64_t i = 0; i < sample_count_; ++i) {
          const double e = std::abs(y_train_host[static_cast<std::size_t>(i)] - pred_values[static_cast<std::size_t>(i)]);
          abs_err[static_cast<std::size_t>(i)] = e;
          max_abs_err = std::max(max_abs_err, e);
        }
        if (!(max_abs_err > 0.0)) {
          EnsembleMember member{};
          member.tree = std::move(tree);
          member.feature_ids = feature_ids;
          member.weight = learning_rate_;
          learners_.push_back(std::move(member));
          break;
        }

        double weighted_loss = 0.0;
        std::vector<double> loss(static_cast<std::size_t>(sample_count_), 0.0);
        for (std::int64_t i = 0; i < sample_count_; ++i) {
          const double li = std::clamp(abs_err[static_cast<std::size_t>(i)] / max_abs_err, 0.0, 1.0);
          loss[static_cast<std::size_t>(i)] = li;
          weighted_loss += sample_weights[static_cast<std::size_t>(i)] * li;
        }
        weighted_loss = std::clamp(weighted_loss, 1e-12, 1.0 - 1e-12);
        if (weighted_loss >= 0.5) {
          return Status::internal(
              "EnsembleRegressor::learn AdaBoost weighted loss reached >= 0.5; weak learner unsuitable.");
        }
        const double beta = weighted_loss / (1.0 - weighted_loss);
        const double alpha = learning_rate_ * std::log(1.0 / beta);

        double norm = 0.0;
        for (std::int64_t i = 0; i < sample_count_; ++i) {
          sample_weights[static_cast<std::size_t>(i)] *= std::pow(beta, 1.0 - loss[static_cast<std::size_t>(i)]);
          norm += sample_weights[static_cast<std::size_t>(i)];
        }
        if (!(norm > 0.0) || !std::isfinite(norm)) {
          return Status::numerical_failure("EnsembleRegressor::learn AdaBoost weight normalization failed.");
        }
        for (double& w : sample_weights) w /= norm;

        EnsembleMember member{};
        member.tree = std::move(tree);
        member.feature_ids = feature_ids;
        member.weight = alpha;
        learners_.push_back(std::move(member));
      }
    } else {
      const std::int64_t workers = resolved_parallel_workers(learner_count);
      if (workers <= 1) {
        for (std::int64_t l = 0; l < learner_count; ++l) {
          StatusOr<EnsembleMember> member_or = train_bagged_member(l, y_train_host);
          if (!member_or.ok()) return member_or.status();
          learners_.push_back(member_or.move_value());
        }
      } else {
        for (std::int64_t start = 0; start < learner_count; start += workers) {
          const std::int64_t end = std::min(learner_count, start + workers);
          std::vector<std::future<StatusOr<EnsembleMember>>> futures{};
          futures.reserve(static_cast<std::size_t>(end - start));
          for (std::int64_t l = start; l < end; ++l) {
            futures.emplace_back(std::async(std::launch::async, [&, l]() {
              return train_bagged_member(l, y_train_host);
            }));
          }
          for (std::int64_t l = start; l < end; ++l) {
            StatusOr<EnsembleMember> member_or = futures[static_cast<std::size_t>(l - start)].get();
            if (!member_or.ok()) return member_or.status();
            learners_.push_back(member_or.move_value());
          }
        }
      }
    }

    if (learners_.empty()) {
      return Status::internal("EnsembleRegressor::learn did not build any learners.");
    }

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "EnsembleRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "EnsembleRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "EnsembleRegressor::infer currently has a CPU-only implementation. Provide CPU features for ensemble inference."));
    }

    const std::vector<double> x_host = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
    std::vector<double> pred_sum(static_cast<std::size_t>(matrix.sample_count), 0.0);
    if (method_ == EnsembleMethod::GradientBoost) {
      std::fill(pred_sum.begin(), pred_sum.end(), initial_prediction_);
    }

    auto infer_member = [&](std::size_t member_index) -> StatusOr<std::vector<double>> {
      const EnsembleMember& member = learners_[member_index];
      const std::int64_t learner_features = static_cast<std::int64_t>(member.feature_ids.size());
      const bool full_route = is_full_route(member.feature_ids, matrix.feature_count);
      std::vector<double> x_sub_host{};
      const std::vector<double>* x_input = &x_host;
      std::int64_t infer_feature_count = matrix.feature_count;
      if (!full_route) {
        x_sub_host = project_rows_full(
            x_host, matrix.sample_count, matrix.feature_count, member.feature_ids, learner_features);
        x_input = &x_sub_host;
        infer_feature_count = learner_features;
      }

      std::vector<double> member_pred{};
      Status member_status = member.tree->predict_host_rows(*x_input, matrix.sample_count, infer_feature_count, member_pred);
      if (!member_status.ok()) return StatusOr<std::vector<double>>(member_status);
      if (member.weight != 1.0) {
        for (double& value : member_pred) {
          value *= member.weight;
        }
      }
      return StatusOr<std::vector<double>>(std::move(member_pred));
    };

    const std::int64_t workers = resolved_parallel_workers(static_cast<std::int64_t>(learners_.size()));
    if (workers <= 1) {
      for (std::size_t mi = 0; mi < learners_.size(); ++mi) {
        StatusOr<std::vector<double>> member_pred_or = infer_member(mi);
        if (!member_pred_or.ok()) return StatusOr<Series>(member_pred_or.status());
        const auto& member_pred = member_pred_or.value();
        for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
          pred_sum[static_cast<std::size_t>(i)] += member_pred[static_cast<std::size_t>(i)];
        }
      }
    } else {
      for (std::size_t start = 0; start < learners_.size(); start += static_cast<std::size_t>(workers)) {
        const std::size_t end = std::min(learners_.size(), start + static_cast<std::size_t>(workers));
        std::vector<std::future<StatusOr<std::vector<double>>>> futures{};
        futures.reserve(end - start);
        for (std::size_t mi = start; mi < end; ++mi) {
          futures.emplace_back(std::async(std::launch::async, [&, mi]() { return infer_member(mi); }));
        }
        for (std::size_t mi = start; mi < end; ++mi) {
          StatusOr<std::vector<double>> member_pred_or = futures[mi - start].get();
          if (!member_pred_or.ok()) return StatusOr<Series>(member_pred_or.status());
          const auto& member_pred = member_pred_or.value();
          for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
            pred_sum[static_cast<std::size_t>(i)] += member_pred[static_cast<std::size_t>(i)];
          }
        }
      }
    }

    if (method_ != EnsembleMethod::GradientBoost) {
      double denom = 0.0;
      if (method_ == EnsembleMethod::AdaBoost) {
        for (const auto& member : learners_) denom += member.weight;
      } else {
        denom = static_cast<double>(learners_.size());
      }
      denom = std::max(denom, 1e-12);
      for (double& v : pred_sum) v /= denom;
    }
    Series out = pluss::make_series(pred_sum, {matrix.sample_count}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("EnsembleRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + learners_.size() * 5);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             static_cast<double>(sample_count_),
                             static_cast<double>(feature_count_),
                             static_cast<double>(learners_.size()),
                             static_cast<double>(static_cast<std::int64_t>(method_)),
                             initial_prediction_,
                             learning_rate_,
                             2.0}));
    for (const auto& member : learners_) {
      out.push_back(state_i64(member.feature_ids));
      out.push_back(state_f64({member.weight}));
      if (!member.tree) {
        out.push_back(state_i64({0}));
        continue;
      }
      StatusOr<std::vector<Series>> tree_state_or = member.tree->dump_state();
      if (!tree_state_or.ok()) return StatusOr<std::vector<Series>>(tree_state_or.status());
      const auto& tree_state = tree_state_or.value();
      out.push_back(state_i64({static_cast<std::int64_t>(tree_state.size())}));
      out.insert(out.end(), tree_state.begin(), tree_state.end());
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("EnsembleRegressor::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("EnsembleRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    const std::int64_t learner_count = static_cast<std::int64_t>(meta[3]);
    method_ = meta.size() >= 5
                  ? static_cast<EnsembleMethod>(static_cast<std::int64_t>(meta[4]))
                  : EnsembleMethod::Bagging;
    initial_prediction_ = meta.size() >= 6 ? meta[5] : 0.0;
    learning_rate_ = meta.size() >= 7 ? meta[6] : options_.learning_rate;
    const bool v2_state = meta.size() >= 8 && meta[7] >= 2.0;

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, learner_count)));
    std::size_t cursor = 1;
    for (std::int64_t li = 0; li < learner_count; ++li) {
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state truncated state.");
      EnsembleMember member{};
      member.feature_ids = vec_i64(state[cursor++]);
      member.weight = 1.0;
      if (v2_state) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state missing member weight.");
        const auto weight_vec = vec_f64(state[cursor++]);
        member.weight = weight_vec.empty() ? 1.0 : weight_vec.front();
      } else if (method_ == EnsembleMethod::GradientBoost) {
        member.weight = learning_rate_;
      }
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state missing tree state size.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t tree_items = size_vec.empty() ? 0 : size_vec.front();
      if (tree_items <= 0) {
        learners_.push_back(std::move(member));
        continue;
      }
      std::vector<Series> tree_state{};
      tree_state.reserve(static_cast<std::size_t>(tree_items));
      for (std::int64_t ti = 0; ti < tree_items; ++ti) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state truncated tree state.");
        tree_state.push_back(state[cursor++]);
      }
      auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{.criterion = TreeCriterion::Mse});
      Status load_status = tree->load_state(tree_state);
      if (!load_status.ok()) return load_status;
      member.tree = std::move(tree);
      learners_.push_back(std::move(member));
    }
    return Status::success();
  }

private:
  struct EnsembleMember {
    std::unique_ptr<TreeRegressor> tree{};
    std::vector<std::int64_t> feature_ids{};
    double weight{1.0};
  };

  [[nodiscard]] static bool is_full_route(const std::vector<std::int64_t>& feature_ids,
                                          std::int64_t feature_count) {
    if (static_cast<std::int64_t>(feature_ids.size()) != feature_count) return false;
    for (std::int64_t i = 0; i < feature_count; ++i) {
      if (feature_ids[static_cast<std::size_t>(i)] != i) return false;
    }
    return true;
  }

  [[nodiscard]] static std::vector<double> gather_rows(const std::vector<double>& source,
                                                       const std::vector<std::int64_t>& row_ids) {
    std::vector<double> out(static_cast<std::size_t>(row_ids.size()), 0.0);
    for (std::size_t i = 0; i < row_ids.size(); ++i) {
      out[i] = source[static_cast<std::size_t>(row_ids[i])];
    }
    return out;
  }

  [[nodiscard]] static std::vector<std::int64_t> draw_stochastic_boost_rows(std::mt19937_64& rng,
                                                                             std::int64_t sample_count,
                                                                             std::int64_t subsample_count) {
    if (subsample_count >= sample_count) {
      std::vector<std::int64_t> rows(static_cast<std::size_t>(sample_count), 0);
      std::iota(rows.begin(), rows.end(), static_cast<std::int64_t>(0));
      return rows;
    }
    std::vector<std::int64_t> rows(static_cast<std::size_t>(sample_count), 0);
    std::iota(rows.begin(), rows.end(), static_cast<std::int64_t>(0));
    std::shuffle(rows.begin(), rows.end(), rng);
    rows.resize(static_cast<std::size_t>(subsample_count));
    return rows;
  }

  [[nodiscard]] static std::vector<std::int64_t> draw_weighted_bootstrap_rows(
      std::mt19937_64& rng,
      const std::vector<double>& sample_weights,
      std::int64_t sample_count) {
    std::vector<std::int64_t> rows(static_cast<std::size_t>(sample_count), 0);
    std::discrete_distribution<std::int64_t> dist(sample_weights.begin(), sample_weights.end());
    for (std::int64_t i = 0; i < sample_count; ++i) {
      rows[static_cast<std::size_t>(i)] = dist(rng);
    }
    return rows;
  }

  [[nodiscard]] static std::vector<double> project_rows_with_sampling(const std::vector<double>& x_host,
                                                                      const std::vector<std::int64_t>& row_ids,
                                                                      std::int64_t sample_count,
                                                                      std::int64_t feature_count,
                                                                      const std::vector<std::int64_t>& feature_ids,
                                                                      std::int64_t learner_features) {
    (void)sample_count;
    std::vector<double> out(static_cast<std::size_t>(row_ids.size() * learner_features), 0.0);
    if (is_full_route(feature_ids, feature_count)) {
      for (std::int64_t i = 0; i < static_cast<std::int64_t>(row_ids.size()); ++i) {
        const std::size_t in_base = static_cast<std::size_t>(row_ids[static_cast<std::size_t>(i)] * feature_count);
        const std::size_t out_base = static_cast<std::size_t>(i * learner_features);
        std::copy_n(x_host.begin() + static_cast<std::vector<double>::difference_type>(in_base),
                    feature_count,
                    out.begin() + static_cast<std::vector<double>::difference_type>(out_base));
      }
      return out;
    }
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(row_ids.size()); ++i) {
      const std::size_t in_base = static_cast<std::size_t>(row_ids[static_cast<std::size_t>(i)] * feature_count);
      const std::size_t out_base = static_cast<std::size_t>(i * learner_features);
      for (std::int64_t j = 0; j < learner_features; ++j) {
        const std::int64_t f = feature_ids[static_cast<std::size_t>(j)];
        out[out_base + static_cast<std::size_t>(j)] = x_host[in_base + static_cast<std::size_t>(f)];
      }
    }
    return out;
  }

  [[nodiscard]] static std::vector<double> project_rows_full(const std::vector<double>& x_host,
                                                             std::int64_t sample_count,
                                                             std::int64_t feature_count,
                                                             const std::vector<std::int64_t>& feature_ids,
                                                             std::int64_t learner_features) {
    if (is_full_route(feature_ids, feature_count)) return x_host;
    std::vector<double> out(static_cast<std::size_t>(sample_count * learner_features), 0.0);
    for (std::int64_t i = 0; i < sample_count; ++i) {
      const std::size_t x_row_base = static_cast<std::size_t>(i * feature_count);
      const std::size_t out_row_base = static_cast<std::size_t>(i * learner_features);
      for (std::int64_t j = 0; j < learner_features; ++j) {
        const std::int64_t f = feature_ids[static_cast<std::size_t>(j)];
        out[out_row_base + static_cast<std::size_t>(j)] = x_host[x_row_base + static_cast<std::size_t>(f)];
      }
    }
    return out;
  }

  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  EnsembleFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::vector<EnsembleMember> learners_{};
  EnsembleMethod method_{EnsembleMethod::Bagging};
  double initial_prediction_{0.0};
  double learning_rate_{0.1};
};

[[nodiscard]] double resolved_gamma(double gamma, std::int64_t feature_count) {
  if (gamma > 0.0) return gamma;
  return 1.0 / static_cast<double>(std::max<std::int64_t>(feature_count, 1));
}

[[nodiscard]] double kernel_eval(KernelKind kind,
                                 double gamma,
                                 const double* a,
                                 const double* b,
                                 std::int64_t feature_count) {
  double dot = 0.0;
  double d2 = 0.0;
  for (std::int64_t f = 0; f < feature_count; ++f) {
    const double av = a[static_cast<std::size_t>(f)];
    const double bv = b[static_cast<std::size_t>(f)];
    dot += av * bv;
    const double d = av - bv;
    d2 += d * d;
  }

  switch (kind) {
    case KernelKind::Linear:
      return dot;
    case KernelKind::Rbf:
      return std::exp(-gamma * d2);
    case KernelKind::Polynomial:
      return std::pow(gamma * dot + 1.0, 3.0);
    case KernelKind::Sigmoid:
      return std::tanh(gamma * dot);
  }
  return dot;
}

[[nodiscard]] Series kernel_matrix(const Series& a_rows, const Series& b_rows, KernelKind kind, double gamma) {
  const Series dot = a_rows.matmul(b_rows.transpose(0, 1));
  switch (kind) {
    case KernelKind::Linear:
      return dot;
    case KernelKind::Rbf: {
      const Series a_norm = a_rows.square().sum(1, true);
      const Series b_norm = b_rows.square().sum(1, true).transpose(0, 1);
      const Series d2 = a_norm.add(b_norm).sub(dot.mul(2.0)).relu();
      return d2.mul(-gamma).exp();
    }
    case KernelKind::Polynomial:
      return dot.mul(gamma).add(1.0).pow(3.0);
    case KernelKind::Sigmoid:
      return dot.mul(gamma).tanh();
  }
  return dot;
}

[[nodiscard]] StatusOr<Series> solve_kernel_dual_cg(const Series& kernel,
                                                    const Series& rhs,
                                                    double lambda,
                                                    std::int64_t max_iterations,
                                                    double tolerance,
                                                    const char* where) {
  const std::int64_t n = kernel.sizes()[0];
  const Platform platform = kernel.platform();
  const Series eye = pluss::series::eye(n, n, f64_on(platform));
  const Series a = kernel.add(eye.mul(lambda));
  Series x = pluss::series::zeros({n}, f64_on(platform));
  Series b = rhs.to(platform, ScalarType::Float64).reshape({n});
  Series r = b;
  Series p = r;
  double rs = r.square().sum().reshape({1}).host<double>().front();
  if (!std::isfinite(rs)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite residual norm."));
  }
  if (rs <= std::max(1e-24, tolerance * tolerance)) {
    return StatusOr<Series>(x);
  }

  const double rs0 = rs;
  const std::int64_t iters = std::max<std::int64_t>(4, max_iterations);
  constexpr double eps = 1e-18;
  for (std::int64_t iter = 0; iter < iters; ++iter) {
    const Series ap = a.matmul(p.reshape({n, 1})).reshape({n});
    const double denom = p.mul(ap).sum().reshape({1}).host<double>().front();
    if (!std::isfinite(denom) || std::abs(denom) <= eps) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered unstable CG denominator."));
    }
    const double alpha = rs / denom;
    x = x.add(p.mul(alpha));
    r = r.sub(ap.mul(alpha));
    const double rs_new = r.square().sum().reshape({1}).host<double>().front();
    if (!std::isfinite(rs_new)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite CG residual norm."));
    }
    if (std::sqrt(rs_new) <= std::max(tolerance * std::sqrt(std::max(rs0, eps)), 1e-12)) {
      break;
    }
    const double beta = rs_new / std::max(rs, eps);
    p = r.add(p.mul(beta));
    rs = rs_new;
  }
  if (!all_finite(x)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite dual coefficients."));
  }
  return StatusOr<Series>(x);
}

[[nodiscard]] Series kernel_predict_cpu(const Series& query_rows,
                                        const Series& train_rows,
                                        const Series& alpha,
                                        KernelKind kind,
                                        double gamma) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
  const auto av = alpha.to(pluss::pCPU, ScalarType::Float64).reshape({alpha.numel()}).contiguous().host<double>().array().data;
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  for (std::int64_t qi = 0; qi < nq; ++qi) {
    const double* qptr = qv.data() + static_cast<std::size_t>(qi * d);
    double s = 0.0;
    for (std::int64_t ti = 0; ti < nt; ++ti) {
      const double* tptr = tv.data() + static_cast<std::size_t>(ti * d);
      s += kernel_eval(kind, gamma, qptr, tptr, d) * av[static_cast<std::size_t>(ti)];
    }
    out[static_cast<std::size_t>(qi)] = s;
  }
  return pluss::make_series(out, {nq}, f64_cpu());
}

class KernelRegressor final : public RegressorBase {
public:
  explicit KernelRegressor(KernelFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_kernel_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "KernelRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "KernelRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "KernelRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    const Series y = y_or.value().reshape({matrix.sample_count, 1});

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    gamma_ = resolved_gamma(options_.gamma, feature_count_);
    const double nu_scale = options_.form == KernelForm::Nu ? std::max(1e-6, options_.nu) : 1.0;
    lambda_ = 1.0 / (options_.c * nu_scale);
    x_train_ = matrix.x_rows;
    const Series k_series = kernel_matrix(x_train_, x_train_, options_.kernel, gamma_);
    StatusOr<Series> alpha_or = solve_kernel_dual_cg(k_series,
                                                     y.reshape({sample_count_}),
                                                     lambda_,
                                                     options.max_iterations,
                                                     options.tolerance,
                                                     "KernelRegressor::learn");
    Series alpha = pluss::series::undefined();
    if (alpha_or.ok()) {
      alpha = alpha_or.move_value();
    } else {
      // Fallback for indefinite kernels (for example sigmoid/polynomial edge cases).
      Series shifted = k_series.add(pluss::series::eye(sample_count_, sample_count_, f64_cpu()).to(x_train_.platform()).mul(lambda_));
      alpha = pluss::subsp::math::linalg::pinverse(shifted, 1e-12).matmul(y).reshape({sample_count_});
    }
    if (!all_finite(alpha)) {
      return Status::numerical_failure("KernelRegressor::learn produced non-finite dual coefficients.");
    }

    alpha_ = alpha;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "KernelRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series pred = pluss::series::undefined();
    if (matrix.x_rows.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
      pred = kernel_predict_cpu(matrix.x_rows, x_train_, alpha_, options_.kernel, gamma_);
    } else {
      const Series k_query = kernel_matrix(matrix.x_rows, x_train_, options_.kernel, gamma_);
      pred = k_query.matmul(alpha_.reshape({sample_count_, 1})).reshape({matrix.sample_count});
    }
    return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("KernelRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   gamma_,
                   lambda_}),
        x_train_,
        alpha_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 3) return Status::invalid_argument("KernelRegressor::load_state expects 3 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 5) return Status::invalid_argument("KernelRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    gamma_ = meta[3];
    lambda_ = meta[4];
    x_train_ = state[1];
    alpha_ = state[2];
    return Status::success();
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  KernelFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  double gamma_{1.0};
  double lambda_{1.0};
  Series x_train_ = pluss::series::undefined();
  Series alpha_ = pluss::series::undefined();
};

class LinearRegressor final : public RegressorBase {
public:
  explicit LinearRegressor(LinearFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_linear_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "LinearRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "LinearRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "LinearRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    Series y = y_or.move_value();

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const LinearPenalty penalty = resolve_linear_penalty(options_);
    StatusOr<Series> beta_or = StatusOr<Series>(Status::internal("unreachable linear objective path"));
    if (options_.objective == LinearObjective::RobustLeast) {
      beta_or = solve_robust_least_squares(design,
                                           y,
                                           options_.fit_intercept,
                                           options_.robust_transition,
                                           penalty.l1,
                                           penalty.l2,
                                           options.max_iterations,
                                           options.tolerance,
                                           "LinearRegressor::learn");
    } else if (penalty.l1 > 0.0) {
      beta_or = solve_elastic_least_squares(design,
                                            y,
                                            options_.fit_intercept,
                                            penalty.l1,
                                            penalty.l2,
                                            options.max_iterations,
                                            options.tolerance,
                                            "LinearRegressor::learn");
    } else {
      beta_or = solve_ridge_least_squares(design, y, penalty.l2, options_.fit_intercept, pinv_rcond_);
    }
    if (!beta_or.ok()) return beta_or.status();

    beta_ = beta_or.value();
    feature_count_ = matrix.feature_count;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "LinearRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "LinearRegressor::infer does not support NaN features; apply the same fitted imputer used for learn()."));
    }

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const Series pred = design.matmul(beta_.reshape({beta_.numel(), 1})).reshape({matrix.sample_count});
    if (!all_finite(pred)) {
      return StatusOr<Series>(Status::numerical_failure("LinearRegressor::infer produced non-finite predictions."));
    }
    return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("LinearRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(feature_count_), pinv_rcond_}),
        beta_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("LinearRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 3) return Status::invalid_argument("LinearRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    pinv_rcond_ = meta[2];
    beta_ = state[1];
    return Status::success();
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  LinearFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series beta_ = pluss::series::undefined();
  double pinv_rcond_{1e-12};
};

class FamilyRegressor final : public RegressorBase {
public:
  explicit FamilyRegressor(ModelFamily family) : family_(family) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    features_ = batch.features.feature_count();
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch);
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));
    return StatusOr<Series>(
        Status::not_implemented(std::string(family_name(family_)) + " regressor infer() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    (void)options;
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));
    if (!trained_) return StatusOr<Series>(Status::precondition("Estimator must be learned before assess()."));
    if (batch.features.feature_count() != features_) {
      return StatusOr<Series>(Status::shape_mismatch("Feature count mismatch between learn and assess batch."));
    }
    return StatusOr<Series>(
        Status::not_implemented(std::string(family_name(family_)) + " regressor assess() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(features_)}),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("FamilyRegressor::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 2) return Status::invalid_argument("FamilyRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    features_ = static_cast<std::int64_t>(meta[1]);
    return Status::success();
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition("Estimator must be learned before infer().");
    if (batch.feature_count() != features_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  ModelFamily family_;
  bool trained_{false};
  std::int64_t features_{0};
};

} // namespace

StatusOr<std::unique_ptr<RegressorBase>> make_regressor(const ModelSpec& spec) {
  if (spec.task != TaskKind::Regression) {
    return StatusOr<std::unique_ptr<RegressorBase>>(
        Status::invalid_argument("make_regressor requires ModelSpec.task = TaskKind::Regression."));
  }
  if (!is_phase0_family(spec.family)) {
    return StatusOr<std::unique_ptr<RegressorBase>>(Status::invalid_argument("Unsupported regressor family."));
  }
  if (spec.family == ModelFamily::Linear) {
    const auto* options = std::get_if<LinearFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Linear regressor requires LinearFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<LinearRegressor>(*options));
  }
  if (spec.family == ModelFamily::Tree) {
    const auto* options = std::get_if<TreeFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Tree regressor requires TreeFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<TreeRegressor>(*options));
  }
  if (spec.family == ModelFamily::Ensemble) {
    const auto* options = std::get_if<EnsembleFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Ensemble regressor requires EnsembleFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<EnsembleRegressor>(*options));
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<NeighborRegressor>(*options));
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Kernel regressor requires KernelFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<KernelRegressor>(*options));
  }
  if (spec.family == ModelFamily::Bayes) {
    const auto* options = std::get_if<BayesFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Bayes regressor requires BayesFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<BayesRegressor>(*options));
  }
  return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<FamilyRegressor>(spec.family));
}

} // namespace pluss::ai::nodej
