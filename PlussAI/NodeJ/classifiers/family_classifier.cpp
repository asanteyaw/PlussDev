#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/balance.h"
#include "pluss/ai/nodej/metrics.h"
#include "pluss/ai/nodej/spatial_index.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <future>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
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
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

[[nodiscard]] std::vector<std::int64_t> vec_i64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).to_vec<std::int64_t>();
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

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::series::ones({x_rows.sizes()[0], 1}, f64_on(x_rows.platform()));
  return ones.unite({x_rows}, 1);
}

[[nodiscard]] Status validate_linear_classifier_options(const LinearFamilyOptions& options) {
  if (options.objective != LinearObjective::Logistic && options.objective != LinearObjective::Hinge) {
    return Status::not_implemented("Linear classifier currently supports LinearObjective::Logistic or LinearObjective::Hinge.");
  }
  if (options.regularization != RegularizationKind::None
      && options.regularization != RegularizationKind::L1
      && options.regularization != RegularizationKind::L2
      && options.regularization != RegularizationKind::Elastic) {
    return Status::invalid_argument("Linear classifier received unknown RegularizationKind.");
  }
  if (!std::isfinite(options.regularization_strength) || options.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  if (!std::isfinite(options.elastic_l1_ratio) || options.elastic_l1_ratio < 0.0 || options.elastic_l1_ratio > 1.0) {
    return Status::invalid_argument("LinearFamilyOptions.elastic_l1_ratio must be finite and in [0, 1].");
  }
  return Status::success();
}

[[nodiscard]] Status validate_neighbor_classifier_options(const NeighborFamilyOptions& options) {
  if (options.k <= 0) {
    return Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions.k > 0.");
  }
  if (!std::isfinite(options.radius) || options.radius < 0.0) {
    return Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions.radius to be finite and >= 0.");
  }
  if (options.radius_fallback_k <= 0) {
    return Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions.radius_fallback_k > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_tree_classifier_options(const TreeFamilyOptions& options) {
  if (options.criterion != TreeCriterion::Gini && options.criterion != TreeCriterion::Entropy) {
    return Status::not_implemented("Tree classifier currently supports TreeCriterion::Gini or TreeCriterion::Entropy only.");
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

[[nodiscard]] Status validate_bayes_classifier_options(const BayesFamilyOptions& options) {
  if (!std::isfinite(options.smoothing) || options.smoothing <= 0.0) {
    return Status::invalid_argument("BayesFamilyOptions.smoothing must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] BayesLikelihood resolved_bayes_likelihood(const BayesFamilyOptions& options) {
  if (!options.use_gaussian && options.likelihood == BayesLikelihood::Gaussian) {
    // Legacy option shape exposed only a gaussian toggle.
    return BayesLikelihood::Bernoulli;
  }
  return options.likelihood;
}

[[nodiscard]] Status validate_kernel_classifier_options(const KernelFamilyOptions& options) {
  if (!std::isfinite(options.nu) || options.nu <= 0.0 || options.nu > 1.0) {
    return Status::invalid_argument("KernelFamilyOptions.nu must be finite and in (0, 1].");
  }
  if (options.form == KernelForm::OneClass) {
    return Status::not_implemented("Kernel classifier does not support KernelForm::OneClass. Use TaskKind::Transform.");
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

[[nodiscard]] Status validate_ensemble_classifier_options(const EnsembleFamilyOptions& options) {
  if (options.method != EnsembleMethod::Bagging
      && options.method != EnsembleMethod::RandomForest
      && options.method != EnsembleMethod::GradientBoost
      && options.method != EnsembleMethod::AdaBoost) {
    return Status::not_implemented(
        "Ensemble classifier currently supports EnsembleMethod::Bagging, EnsembleMethod::RandomForest, EnsembleMethod::GradientBoost, and EnsembleMethod::AdaBoost.");
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

[[nodiscard]] Series knn_probability_cpu(const Series& query_rows,
                                         const Series& train_rows,
                                         const Series& y_train,
                                         const Series& train_weight,
                                         std::int64_t k,
                                         bool distance_weighted) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const auto yv = y_train.to(pluss::pCPU, ScalarType::Float64).reshape({y_train.numel()}).contiguous().to_vec<double>();
  const auto wv = train_weight.to(pluss::pCPU, ScalarType::Float64).reshape({train_weight.numel()}).contiguous().to_vec<double>();
  const std::int64_t kk = std::max<std::int64_t>(1, std::min<std::int64_t>(k, nt));
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  constexpr double eps = 1e-12;

  // SpatialIndex (a real KD-tree) replaces the brute-force triple-nested
  // distance loop + per-query max-heap below -- train_rows/query_rows are
  // already validated non-empty rank-2 matrices by the time infer_scores()
  // calls this, so build()/query_k() are not expected to fail here; the
  // all-zero fallback exists only so a violated invariant degrades instead
  // of crashing.
  StatusOr<SpatialIndex> index_or = SpatialIndex::build(train_rows);
  if (!index_or.ok()) return pluss::make_series(out, {nq}, f64_cpu());
  const SpatialIndex index = index_or.move_value();
  StatusOr<KNearestResult> knn_or = index.query_k(query_rows, kk);
  if (!knn_or.ok()) return pluss::make_series(out, {nq}, f64_cpu());
  const KNearestResult knn = knn_or.move_value();
  const auto knn_idx = knn.indices.to(pluss::pCPU, ScalarType::Int64).reshape({knn.indices.numel()}).to_vec<std::int64_t>();
  const auto knn_d2 = knn.distances2.to(pluss::pCPU, ScalarType::Float64).reshape({knn.distances2.numel()}).to_vec<double>();

  for (std::int64_t qi = 0; qi < nq; ++qi) {
    std::vector<std::pair<double, std::int64_t>> best{};
    best.reserve(static_cast<std::size_t>(kk));
    for (std::int64_t j = 0; j < kk; ++j) {
      const std::size_t pos = static_cast<std::size_t>(qi * kk + j);
      best.emplace_back(knn_d2[pos], knn_idx[pos]);
    }

    if (distance_weighted) {
      double zero_sum = 0.0;
      double zero_weight = 0.0;
      for (const auto& [d2, ti] : best) {
        if (d2 <= eps) {
          const double w = wv[static_cast<std::size_t>(ti)];
          zero_sum += w * yv[static_cast<std::size_t>(ti)];
          zero_weight += w;
        }
      }
      if (zero_weight > 0.0) {
        out[static_cast<std::size_t>(qi)] = std::clamp(zero_sum / zero_weight, 0.0, 1.0);
        continue;
      }

      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [d2, ti] : best) {
        const double w = wv[static_cast<std::size_t>(ti)] / std::sqrt(d2 + eps);
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? std::clamp(ysum / wsum, 0.0, 1.0) : 0.0;
    } else {
      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [_, ti] : best) {
        const double w = wv[static_cast<std::size_t>(ti)];
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? std::clamp(ysum / wsum, 0.0, 1.0) : 0.0;
    }
  }

  return pluss::make_series(out, {nq}, f64_cpu());
}

[[nodiscard]] Series radius_probability_cpu(const Series& query_rows,
                                            const Series& train_rows,
                                            const Series& y_train,
                                            const Series& train_weight,
                                            double radius,
                                            std::int64_t fallback_k,
                                            bool distance_weighted) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto yv = y_train.to(pluss::pCPU, ScalarType::Float64).reshape({y_train.numel()}).contiguous().to_vec<double>();
  const auto wv = train_weight.to(pluss::pCPU, ScalarType::Float64).reshape({train_weight.numel()}).contiguous().to_vec<double>();
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
      if (d2 <= radius2) {
        in_radius.emplace_back(d2, ti);
      }
      if (static_cast<std::int64_t>(best.size()) < kk) {
        best.emplace_back(d2, ti);
        if (static_cast<std::int64_t>(best.size()) == kk) {
          std::make_heap(best.begin(), best.end());
        }
      } else if (d2 < best.front().first) {
        std::pop_heap(best.begin(), best.end());
        best.back() = std::pair<double, std::int64_t>{d2, ti};
        std::push_heap(best.begin(), best.end());
      }
    }
    const auto& selected = in_radius.empty() ? best : in_radius;

    if (distance_weighted) {
      double zero_sum = 0.0;
      double zero_weight = 0.0;
      for (const auto& [d2, ti] : selected) {
        if (d2 <= eps) {
          const double w = wv[static_cast<std::size_t>(ti)];
          zero_sum += w * yv[static_cast<std::size_t>(ti)];
          zero_weight += w;
        }
      }
      if (zero_weight > 0.0) {
        out[static_cast<std::size_t>(qi)] = std::clamp(zero_sum / zero_weight, 0.0, 1.0);
        continue;
      }
      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [d2, ti] : selected) {
        const double w = wv[static_cast<std::size_t>(ti)] / std::sqrt(d2 + eps);
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? std::clamp(ysum / wsum, 0.0, 1.0) : 0.0;
    } else {
      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [_, ti] : selected) {
        const double w = wv[static_cast<std::size_t>(ti)];
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? std::clamp(ysum / wsum, 0.0, 1.0) : 0.0;
    }
  }
  return pluss::make_series(out, {nq}, f64_cpu());
}

[[nodiscard]] Series knn_probability_series(const Series& query_rows,
                                            const Series& train_rows,
                                            const Series& y_train,
                                            const Series& train_weight,
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
  const Series weight_train_2d =
      train_weight.reshape({1, sample_count}).expand(std::vector<std::int64_t>{query_count, sample_count});
  const Series weight_knn = weight_train_2d.gather(1, knn_idx).to(ScalarType::Float64);

  if (!distance_weighted) {
    const Series wsum = weight_knn.sum(1, false);
    const Series ysum = y_knn.mul(weight_knn).sum(1, false);
    return wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64)).clamp(0.0, 1.0);
  }

  constexpr double eps = 1e-12;
  const Series d2_knn = dist2.gather(1, knn_idx);
  const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
  const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
  const Series inv_dist = pluss::series::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
  const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist)).mul(weight_knn);
  const Series wsum = weights.sum(1, false);
  const Series ysum = weights.mul(y_knn).sum(1, false);
  return wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64)).clamp(0.0, 1.0);
}

[[nodiscard]] Series radius_probability_series(const Series& query_rows,
                                               const Series& train_rows,
                                               const Series& y_train,
                                               const Series& train_weight,
                                               double radius,
                                               std::int64_t fallback_k,
                                               bool distance_weighted) {
  const std::int64_t sample_count = train_rows.sizes()[0];
  const std::int64_t query_count = query_rows.sizes()[0];
  const Series dist2 = pairwise_distance2(query_rows, train_rows);
  const Series mask = dist2.le(radius * radius).to(ScalarType::Float64);
  const Series in_count = mask.sum(1, false).reshape({query_count});
  const Series fallback = knn_probability_series(query_rows, train_rows, y_train, train_weight, fallback_k, distance_weighted);
  const Series y_train_2d =
      y_train.reshape({1, sample_count}).expand(std::vector<std::int64_t>{query_count, sample_count});
  const Series weight_train_2d =
      train_weight.reshape({1, sample_count}).expand(std::vector<std::int64_t>{query_count, sample_count});

  Series radius_prob = pluss::series::undefined();
  if (distance_weighted) {
    constexpr double eps = 1e-12;
    const Series zero_mask = dist2.le(eps).to(ScalarType::Float64).mul(mask);
    const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
    const Series inv_dist = pluss::series::ones_like(dist2, ScalarType::Float64).div(dist2.sqrt().add(eps));
    const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist).mul(mask)).mul(weight_train_2d);
    const Series wsum = weights.sum(1, false);
    const Series ysum = weights.mul(y_train_2d).sum(1, false);
    radius_prob = wsum.gt(0.0).where(ysum.div(wsum), fallback);
  } else {
    const Series weights = weight_train_2d.mul(mask);
    const Series wsum = weights.sum(1, false);
    const Series ysum = y_train_2d.mul(weights).sum(1, false);
    radius_prob = wsum.gt(0.0).where(ysum.div(wsum), fallback);
  }

  return in_count.gt(0.0).where(radius_prob, fallback).clamp(0.0, 1.0);
}

struct BinaryLabelEncoding {
  Series y01 = pluss::series::undefined();
  double negative = 0.0;
  double positive = 1.0;
  ScalarType label_dtype = ScalarType::Float64;
};

[[nodiscard]] StatusOr<Series> extract_sorted_classes(const LabeledBatch& batch,
                                                      std::int64_t expected_samples,
                                                      const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }

  const Series y64 = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y64)) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }

  const Series classes_raw = y64.unique(true).sort(0, false, false);
  const Series classes = classes_raw.reshape({classes_raw.numel()});
  if (classes.numel() < 2) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " requires at least two distinct classes."));
  }
  return StatusOr<Series>(classes);
}

[[nodiscard]] bool same_sorted_classes(const Series& a, const Series& b) {
  if (a.numel() != b.numel()) return false;
  return a.eq(b).all().item<bool>();
}

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested);
[[nodiscard]] StatusOr<Series> classification_metrics_multiclass(const Series& truth_labels,
                                                                 const Series& pred_labels,
                                                                 const Series& score_matrix,
                                                                 const Series& classes,
                                                                 const std::vector<MetricKind>& requested);

[[nodiscard]] StatusOr<Series> resolve_parameter_bounds(const Series& raw_bounds,
                                                        std::int64_t param_count,
                                                        Platform platform,
                                                        const char* where) {
  if (!raw_bounds.defined()) return StatusOr<Series>(pluss::series::undefined());

  Series bounds = raw_bounds.to(platform, ScalarType::Float64).reshape({raw_bounds.numel()});
  if (bounds.numel() == 1) {
    bounds = bounds.reshape({1}).expand(std::vector<std::int64_t>{param_count}).reshape({param_count});
  } else if (bounds.numel() != param_count) {
    return StatusOr<Series>(Status::shape_mismatch(
        std::string(where) + " bounds must be scalar or length equal to parameter count."));
  }

  if (!all_finite(bounds)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " bounds must be finite."));
  }
  return StatusOr<Series>(std::move(bounds));
}

[[nodiscard]] Series labels_from_scores(const Series& scores,
                                        double negative_label,
                                        double positive_label,
                                        double positive_threshold,
                                        Platform out_platform,
                                        ScalarType out_dtype);
[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               double positive_threshold,
                                               Platform out_platform,
                                               ScalarType out_dtype);

[[nodiscard]] StatusOr<BinaryLabelEncoding> encode_binary_labels(const LabeledBatch& batch,
                                                                 std::int64_t expected_samples,
                                                                 const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<BinaryLabelEncoding>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<BinaryLabelEncoding>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }

  const Series y64 = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y64)) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }

  const Series classes = y64.unique(true).sort(0, false, false);
  if (classes.numel() != 2) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " currently requires exactly two distinct classes."));
  }

  const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cpu());
  const Series idx1 = pluss::make_series(std::vector<std::int64_t>{1}, {1}, i64_cpu());
  const double negative = classes.index({idx0}).reshape({1}).item<double>();
  const double positive = classes.index({idx1}).reshape({1}).item<double>();
  const Series y01 = y64.eq(positive).to(ScalarType::Float64);

  BinaryLabelEncoding out{};
  out.y01 = y01;
  out.negative = negative;
  out.positive = positive;
  out.label_dtype = batch.y.scalar_type();
  return StatusOr<BinaryLabelEncoding>(std::move(out));
}

[[nodiscard]] Status validate_positive_threshold(double threshold, const char* where) {
  if (!std::isfinite(threshold) || threshold < 0.0 || threshold > 1.0) {
    return Status::invalid_argument(std::string(where) + " requires positive_threshold in [0, 1].");
  }
  return Status::success();
}

[[nodiscard]] StatusOr<Series> fit_platt_calibrator(const Series& scores,
                                                    const Series& y01,
                                                    const Series& sample_weights,
                                                    std::int64_t max_iterations,
                                                    double tolerance,
                                                    const char* where) {
  if (scores.numel() != y01.numel() || scores.numel() != sample_weights.numel()) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " expects equal scores/labels/weights lengths."));
  }
  if (scores.numel() == 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty scores."));
  }
  if (max_iterations <= 0 || !(tolerance > 0.0)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires positive iterations and tolerance."));
  }

  const Platform platform = scores.platform();
  Series params = pluss::make_series(std::vector<double>{1.0, 0.0}, {2}, f64_on(platform));
  const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
  const Series idx1 = make_index_series(std::vector<std::int64_t>{1}, platform);
  const double weight_sum = sample_weights.sum().item<double>();
  if (!(weight_sum > 0.0) || !std::isfinite(weight_sum)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires strictly positive total sample weight."));
  }

  constexpr double eps = 1e-12;
  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const double a = params.index_select(0, idx0).reshape({1}).item<double>();
    const double b = params.index_select(0, idx1).reshape({1}).item<double>();

    const Series logits = scores.mul(a).add(b);
    const Series probs = logits.sigmoid();
    const Series err = probs.sub(y01);
    const Series weighted_err = err.mul(sample_weights);
    const Series grad_a = weighted_err.mul(scores).sum().div(weight_sum);
    const Series grad_b = weighted_err.sum().div(weight_sum);
    const Series grad = grad_a.reshape({1}).unite({grad_b.reshape({1})}, 0);
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite calibration gradient."));
    }

    const double lr = 0.05;
    params = params.sub(grad.mul(lr));

    const Series clipped = probs.clamp(eps, 1.0 - eps);
    const Series loss_terms = y01.mul(clipped.log()).add(y01.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0);
    const double loss = loss_terms.mul(sample_weights).sum().div(weight_sum).item<double>();
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite calibration objective."));
    }
    if (std::abs(prev_loss - loss) <= tolerance) {
      break;
    }
    prev_loss = loss;
  }

  if (!all_finite(params)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite calibration parameters."));
  }
  return StatusOr<Series>(params);
}

[[nodiscard]] StatusOr<double> fit_temperature_calibrator(const Series& scores,
                                                          const Series& y01,
                                                          const Series& sample_weights,
                                                          std::int64_t max_iterations,
                                                          double tolerance,
                                                          const char* where) {
  if (scores.numel() != y01.numel() || scores.numel() != sample_weights.numel()) {
    return StatusOr<double>(Status::shape_mismatch(std::string(where) + " expects equal scores/labels/weights lengths."));
  }
  if (scores.numel() == 0) {
    return StatusOr<double>(Status::invalid_argument(std::string(where) + " requires non-empty scores."));
  }
  if (max_iterations <= 0 || !(tolerance > 0.0)) {
    return StatusOr<double>(Status::invalid_argument(std::string(where) + " requires positive iterations and tolerance."));
  }

  const double weight_sum = sample_weights.sum().item<double>();
  if (!(weight_sum > 0.0) || !std::isfinite(weight_sum)) {
    return StatusOr<double>(Status::invalid_argument(std::string(where) + " requires strictly positive total sample weight."));
  }

  constexpr double min_temperature = 1e-6;
  constexpr double max_temperature = 1e6;
  double theta = 0.0; // log(T)
  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const double inv_t = std::exp(-theta);
    const Series logits = scores.mul(inv_t);
    const Series probs = logits.sigmoid();
    const Series err = probs.sub(y01).mul(sample_weights);

    const double grad = err.mul(logits).sum().item<double>() / weight_sum * (-1.0);
    if (!std::isfinite(grad)) {
      return StatusOr<double>(Status::numerical_failure(std::string(where) + " encountered non-finite temperature gradient."));
    }

    theta -= 0.05 * grad;
    theta = std::clamp(theta, std::log(min_temperature), std::log(max_temperature));

    const Series clipped = probs.clamp(1e-12, 1.0 - 1e-12);
    const Series loss_terms = y01.mul(clipped.log()).add(y01.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0);
    const double loss = loss_terms.mul(sample_weights).sum().div(weight_sum).item<double>();
    if (!std::isfinite(loss)) {
      return StatusOr<double>(Status::numerical_failure(std::string(where) + " encountered non-finite temperature objective."));
    }
    if (std::abs(prev_loss - loss) <= tolerance) break;
    prev_loss = loss;
  }

  const double temperature = std::exp(theta);
  if (!std::isfinite(temperature) || temperature < min_temperature || temperature > max_temperature) {
    return StatusOr<double>(Status::numerical_failure(std::string(where) + " produced invalid temperature."));
  }
  return StatusOr<double>(temperature);
}

struct IsotonicCalibratorState {
  std::vector<double> x_thresholds{};
  std::vector<double> y_thresholds{};
};

[[nodiscard]] StatusOr<IsotonicCalibratorState> fit_isotonic_calibrator(const Series& scores,
                                                                        const Series& y01,
                                                                        const Series& sample_weights,
                                                                        const char* where) {
  if (scores.numel() != y01.numel() || scores.numel() != sample_weights.numel()) {
    return StatusOr<IsotonicCalibratorState>(
        Status::shape_mismatch(std::string(where) + " expects equal scores/labels/weights lengths."));
  }
  if (scores.numel() == 0) {
    return StatusOr<IsotonicCalibratorState>(Status::invalid_argument(std::string(where) + " requires non-empty scores."));
  }

  const std::int64_t n = scores.numel();
  const auto xs = scores.to(pluss::pCPU, ScalarType::Float64).reshape({n}).to_vec<double>();
  const auto ys = y01.to(pluss::pCPU, ScalarType::Float64).reshape({n}).to_vec<double>();
  const auto ws = sample_weights.to(pluss::pCPU, ScalarType::Float64).reshape({n}).to_vec<double>();

  std::vector<std::int64_t> order(static_cast<std::size_t>(n), 0);
  std::iota(order.begin(), order.end(), static_cast<std::int64_t>(0));
  std::stable_sort(order.begin(), order.end(), [&](std::int64_t a, std::int64_t b) {
    return xs[static_cast<std::size_t>(a)] < xs[static_cast<std::size_t>(b)];
  });

  std::vector<double> x_unique{};
  std::vector<double> w_unique{};
  std::vector<double> wy_unique{};
  x_unique.reserve(static_cast<std::size_t>(n));
  w_unique.reserve(static_cast<std::size_t>(n));
  wy_unique.reserve(static_cast<std::size_t>(n));

  for (std::int64_t oi = 0; oi < n; ++oi) {
    const std::int64_t idx = order[static_cast<std::size_t>(oi)];
    const double x = xs[static_cast<std::size_t>(idx)];
    const double y = std::clamp(ys[static_cast<std::size_t>(idx)], 0.0, 1.0);
    const double w = ws[static_cast<std::size_t>(idx)];
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || w <= 0.0) {
      return StatusOr<IsotonicCalibratorState>(
          Status::invalid_argument(std::string(where) + " requires finite scores/labels and strictly positive weights."));
    }
    if (!x_unique.empty() && x == x_unique.back()) {
      w_unique.back() += w;
      wy_unique.back() += w * y;
    } else {
      x_unique.push_back(x);
      w_unique.push_back(w);
      wy_unique.push_back(w * y);
    }
  }

  if (x_unique.empty()) {
    return StatusOr<IsotonicCalibratorState>(
        Status::invalid_argument(std::string(where) + " could not construct isotonic support points."));
  }

  struct IsoBlock {
    std::int64_t left = 0;
    std::int64_t right = 0;
    double w = 0.0;
    double wy = 0.0;
  };

  std::vector<IsoBlock> blocks{};
  blocks.reserve(x_unique.size());
  for (std::size_t i = 0; i < x_unique.size(); ++i) {
    blocks.push_back(IsoBlock{
        .left = static_cast<std::int64_t>(i),
        .right = static_cast<std::int64_t>(i),
        .w = w_unique[i],
        .wy = wy_unique[i],
    });
    while (blocks.size() >= 2) {
      const std::size_t m = blocks.size();
      const double prev_mean = blocks[m - 2].wy / blocks[m - 2].w;
      const double last_mean = blocks[m - 1].wy / blocks[m - 1].w;
      if (prev_mean <= last_mean) break;
      blocks[m - 2].right = blocks[m - 1].right;
      blocks[m - 2].w += blocks[m - 1].w;
      blocks[m - 2].wy += blocks[m - 1].wy;
      blocks.pop_back();
    }
  }

  IsotonicCalibratorState out{};
  out.x_thresholds.reserve(x_unique.size() * 2);
  out.y_thresholds.reserve(x_unique.size() * 2);
  for (const IsoBlock& b : blocks) {
    const double yhat = std::clamp(b.wy / b.w, 0.0, 1.0);
    const double xl = x_unique[static_cast<std::size_t>(b.left)];
    const double xr = x_unique[static_cast<std::size_t>(b.right)];
    if (out.x_thresholds.empty() || xl > out.x_thresholds.back()) {
      out.x_thresholds.push_back(xl);
      out.y_thresholds.push_back(yhat);
    } else {
      out.y_thresholds.back() = yhat;
    }
    if (xr > out.x_thresholds.back()) {
      out.x_thresholds.push_back(xr);
      out.y_thresholds.push_back(yhat);
    }
  }

  if (out.x_thresholds.empty() || out.x_thresholds.size() != out.y_thresholds.size()) {
    return StatusOr<IsotonicCalibratorState>(
        Status::numerical_failure(std::string(where) + " produced invalid isotonic thresholds."));
  }
  return StatusOr<IsotonicCalibratorState>(std::move(out));
}

[[nodiscard]] StatusOr<Series> apply_isotonic_calibrator(const Series& scores,
                                                         const std::vector<double>& x_thresholds,
                                                         const std::vector<double>& y_thresholds,
                                                         const char* where) {
  if (x_thresholds.empty() || x_thresholds.size() != y_thresholds.size()) {
    return StatusOr<Series>(Status::precondition(std::string(where) + " isotonic calibrator is not initialized."));
  }
  const std::int64_t n = scores.numel();
  const auto in = scores.to(pluss::pCPU, ScalarType::Float64).reshape({n}).to_vec<double>();
  std::vector<double> out{};
  out.reserve(in.size());

  const std::size_t last = x_thresholds.size() - 1;
  for (double x : in) {
    if (!std::isfinite(x)) {
      out.push_back(std::numeric_limits<double>::quiet_NaN());
      continue;
    }
    if (x <= x_thresholds.front()) {
      out.push_back(y_thresholds.front());
      continue;
    }
    if (x >= x_thresholds[last]) {
      out.push_back(y_thresholds[last]);
      continue;
    }
    const auto it = std::upper_bound(x_thresholds.begin(), x_thresholds.end(), x);
    const std::size_t hi = static_cast<std::size_t>(std::distance(x_thresholds.begin(), it));
    const std::size_t lo = hi - 1;
    const double xl = x_thresholds[lo];
    const double xr = x_thresholds[hi];
    const double yl = y_thresholds[lo];
    const double yr = y_thresholds[hi];
    if (!(xr > xl)) {
      out.push_back(std::clamp(yr, 0.0, 1.0));
      continue;
    }
    const double t = (x - xl) / (xr - xl);
    out.push_back(std::clamp(yl + (yr - yl) * t, 0.0, 1.0));
  }

  return StatusOr<Series>(pluss::make_series(std::move(out), {n}, f64_cpu()).to(scores.platform(), ScalarType::Float64));
}

[[nodiscard]] bool training_balance_requested(const TrainOptions& options) {
  return options.balance.kind != BalanceTargetKind::Keep
      || options.balance.clean != BalanceCleanKind::None;
}

[[nodiscard]] TrainOptions without_training_balance(TrainOptions options) {
  options.balance = BalanceTarget{};
  return options;
}

[[nodiscard]] StatusOr<LabeledBatch> resolve_classification_training_batch(const LabeledBatch& batch,
                                                                           const TrainOptions& options,
                                                                           const char* where) {
  if (!training_balance_requested(options)) return StatusOr<LabeledBatch>(batch);
  StatusOr<LabeledBatch> balanced_or = balance_rows(batch, options.balance, options.random);
  if (!balanced_or.ok()) {
    Status status = balanced_or.status();
    status.message = std::string(where) + " balance: " + status.message;
    return StatusOr<LabeledBatch>(std::move(status));
  }
  return balanced_or;
}

class NeighborClassifier final : public ClassifierBase {
public:
  explicit NeighborClassifier(NeighborFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_neighbor_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "NeighborClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "NeighborClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "NeighborClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "NeighborClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> train_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!train_weight_or.ok()) return train_weight_or.status();
    Series train_weight = train_weight_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    query_mode_ = options_.query_mode;
    radius_ = options_.radius;
    radius_fallback_k_ = std::min<std::int64_t>(options_.radius_fallback_k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborClassifier requires at least one training sample.");
    }

    Series x_train = matrix.x_rows;
    if (options_.standardize) {
      x_train = standardize_fit_rows(x_train, mean_, scale_);
    } else {
      mean_ = pluss::series::undefined();
      scale_ = pluss::series::undefined();
    }
    x_train_ = std::move(x_train);
    y_train_ = labels.y01.to(x_train_.platform(), ScalarType::Float64).reshape({sample_count_});
    train_weight_ = train_weight.to(x_train_.platform(), ScalarType::Float64).reshape({sample_count_});

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "NeighborClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "NeighborClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series x_query = matrix.x_rows;
    if (options_.standardize) {
      x_query = standardize_apply_rows(x_query, mean_, scale_);
    }

    Series probs = pluss::series::undefined();
    if (query_mode_ == NeighborQueryMode::Radius) {
      probs = radius_probability_series(x_query,
                                        x_train_,
                                        y_train_,
                                        train_weight_,
                                        radius_,
                                        radius_fallback_k_,
                                        options_.distance_weighted);
    } else {
      const std::int64_t neighbor_count = std::min<std::int64_t>(k_, sample_count_);
      if (x_query.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
        probs = knn_probability_cpu(x_query, x_train_, y_train_, train_weight_, neighbor_count, options_.distance_weighted);
      } else {
        const Series dist2 = pairwise_distance2(x_query, x_train_);
        const Series sort_idx = dist2.argsort(1, false, true);
        const Series k_cols =
            pluss::series::arange(0, neighbor_count, Config{}.with_dtype(ScalarType::Int64).with_platform(sort_idx.platform()));
        const Series knn_idx = sort_idx.index_select(1, k_cols);
        const Series y_train_2d =
            y_train_.reshape({1, sample_count_}).expand(std::vector<std::int64_t>{matrix.sample_count, sample_count_});
        const Series y_knn = y_train_2d.gather(1, knn_idx);
        const Series train_weight_2d =
            train_weight_.reshape({1, sample_count_}).expand(std::vector<std::int64_t>{matrix.sample_count, sample_count_});
        const Series w_knn = train_weight_2d.gather(1, knn_idx).to(ScalarType::Float64);

        if (options_.distance_weighted) {
          constexpr double eps = 1e-12;
          const Series d2_knn = dist2.gather(1, knn_idx);
          const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
          const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
          const Series inv_dist = pluss::series::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
          const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist)).mul(w_knn);
          const Series wsum = weights.sum(1, false);
          const Series ysum = weights.mul(y_knn).sum(1, false);
          probs = wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64)).clamp(0.0, 1.0);
        } else {
          const Series wsum = w_knn.sum(1, false);
          const Series ysum = y_knn.mul(w_knn).sum(1, false);
          probs = wsum.gt(0.0).where(ysum.div(wsum), pluss::series::zeros_like(wsum, ScalarType::Float64)).clamp(0.0, 1.0);
        }
      }
    }

    return StatusOr<Series>(probs.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "NeighborClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  options.positive_threshold,
                                  batch.x.platform(),
                                  label_dtype_));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "NeighborClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("NeighborClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> probs_or = infer_scores(batch.features);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs_or.value(), metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   static_cast<double>(k_),
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                   static_cast<double>(static_cast<std::int64_t>(query_mode_)),
                   radius_,
                   static_cast<double>(radius_fallback_k_),
                   3.0}),
        x_train_,
        y_train_,
        mean_,
        scale_,
        train_weight_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 5) return Status::invalid_argument("NeighborClassifier::load_state expects 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("NeighborClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    k_ = static_cast<std::int64_t>(meta[3]);
    negative_label_ = meta[4];
    positive_label_ = meta[5];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[6]));
    const bool v2_state = meta.size() >= 11 && meta[10] >= 2.0;
    query_mode_ = v2_state && meta.size() >= 8
                      ? static_cast<NeighborQueryMode>(static_cast<std::int64_t>(meta[7]))
                      : NeighborQueryMode::KNearest;
    radius_ = v2_state && meta.size() >= 9 ? meta[8] : options_.radius;
    radius_fallback_k_ = v2_state && meta.size() >= 10
                             ? static_cast<std::int64_t>(meta[9])
                             : std::max<std::int64_t>(1, std::min<std::int64_t>(options_.radius_fallback_k, sample_count_));
    x_train_ = state[1];
    y_train_ = state[2];
    mean_ = state[3];
    scale_ = state[4];
    train_weight_ = state.size() >= 6 ? state[5] : pluss::series::ones({sample_count_}, f64_on(y_train_.platform()));
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
  Series train_weight_ = pluss::series::undefined();
  Series mean_ = pluss::series::undefined();
  Series scale_ = pluss::series::undefined();
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] double binary_class_impurity(double positive_count, double sample_count, TreeCriterion criterion) {
  if (sample_count <= 0.0) return 0.0;
  const double p = std::clamp(positive_count / sample_count, 0.0, 1.0);
  const double q = 1.0 - p;
  switch (criterion) {
    case TreeCriterion::Gini:
      return 1.0 - (p * p + q * q);
    case TreeCriterion::Entropy: {
      constexpr double eps = 1e-12;
      const double pc = std::clamp(p, eps, 1.0);
      const double qc = std::clamp(q, eps, 1.0);
      return -(p > 0.0 ? p * std::log(pc) : 0.0) - (q > 0.0 ? q * std::log(qc) : 0.0);
    }
    default:
      return 0.0;
  }
}

class TreeClassifier final : public ClassifierBase {
public:
  explicit TreeClassifier(TreeFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_tree_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "TreeClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "TreeClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "TreeClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "TreeClassifier::learn currently has a CPU-only implementation. Provide CPU features for tree learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "TreeClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> train_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!train_weight_or.ok()) return train_weight_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    x_train_host_ = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    y_train_host_ = labels.y01.to(pluss::pCPU, ScalarType::Float64).reshape({sample_count_}).contiguous().to_vec<double>();
    sample_weight_host_ =
        train_weight_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({sample_count_}).contiguous().to_vec<double>();

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeClassifier::learn failed to build any tree nodes.");
    }

    x_train_host_.clear();
    y_train_host_.clear();
    sample_weight_host_.clear();

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "TreeClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "TreeClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "TreeClassifier::infer_scores currently has a CPU-only implementation. Provide CPU features for tree inference."));
    }

    std::vector<double> probs{};
    Status pred_status = predict_prob_host_rows(
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>(),
        matrix.sample_count,
        matrix.feature_count,
        probs);
    if (!pred_status.ok()) return StatusOr<Series>(std::move(pred_status));

    Series out = pluss::make_series(probs, {static_cast<std::int64_t>(probs.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  Status predict_prob_host_rows(const std::vector<double>& x_rows,
                                std::int64_t sample_count,
                                std::int64_t feature_count,
                                std::vector<double>& out) const {
    if (!trained_) return Status::precondition("TreeClassifier must be learned before host prediction.");
    if (feature_count != feature_count_) return Status::shape_mismatch("TreeClassifier host prediction feature mismatch.");
    const std::size_t expected = static_cast<std::size_t>(sample_count * feature_count);
    if (x_rows.size() != expected) {
      return Status::shape_mismatch("TreeClassifier host prediction row-major size mismatch.");
    }
    out.assign(static_cast<std::size_t>(sample_count), 0.0);
    for (std::int64_t i = 0; i < sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_rows[static_cast<std::size_t>(i * feature_count + node.feature)];
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      out[static_cast<std::size_t>(i)] =
          std::clamp(nodes_[static_cast<std::size_t>(node_idx)].leaf_positive_probability, 0.0, 1.0);
    }
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "TreeClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  options.positive_threshold,
                                  batch.x.platform(),
                                  label_dtype_));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "TreeClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("TreeClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> probs_or = infer_scores(batch.features);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs_or.value(), metrics);
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
      flat.push_back(node.leaf_positive_probability);
    }
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                   static_cast<double>(nodes_.size())}),
        state_f64(std::move(flat)),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("TreeClassifier::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("TreeClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    negative_label_ = meta[3];
    positive_label_ = meta[4];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[5]));
    const std::size_t node_count = static_cast<std::size_t>(std::max<double>(0.0, meta[6]));
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
      node.leaf_positive_probability = flat[base + 5];
      nodes_[i] = node;
    }
    x_train_host_.clear();
    y_train_host_.clear();
    sample_weight_host_.clear();
    return Status::success();
  }

private:
  struct TreeNode {
    bool is_leaf = true;
    std::int64_t feature = -1;
    double threshold = 0.0;
    std::int64_t left = -1;
    std::int64_t right = -1;
    double leaf_positive_probability = 0.0;
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

  [[nodiscard]] double sample_weight_at(std::int64_t row) const {
    return sample_weight_host_[static_cast<std::size_t>(row)];
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    double positive_weight = 0.0;
    double total_weight = 0.0;
    for (std::int64_t idx : indices) {
      const double wi = sample_weight_at(idx);
      positive_weight += wi * y_train_at(idx);
      total_weight += wi;
    }
    if (!(total_weight > 0.0) || !std::isfinite(total_weight)) {
      total_weight = static_cast<double>(std::max<std::int64_t>(n, 1));
      positive_weight = 0.0;
      for (std::int64_t idx : indices) positive_weight += y_train_at(idx);
    }

    const double parent_impurity = binary_class_impurity(positive_weight, total_weight, options_.criterion);
    TreeNode node{};
    node.leaf_positive_probability = std::clamp(positive_weight / total_weight, 0.0, 1.0);

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

    struct WeightedFeatureValue {
      double x = 0.0;
      double weighted_y = 0.0;
      double weight = 1.0;
    };

    std::vector<WeightedFeatureValue> values{};
    values.reserve(static_cast<std::size_t>(n));
    double total_positive_weight = 0.0;
    double total_weight = 0.0;
    for (std::int64_t idx : indices) {
      const double yi = y_train_at(idx);
      const double wi = sample_weight_at(idx);
      total_positive_weight += wi * yi;
      total_weight += wi;
      values.push_back(WeightedFeatureValue{
          .x = x_train_at(idx, feature),
          .weighted_y = yi * wi,
          .weight = wi,
      });
    }
    if (!(total_weight > 0.0) || !std::isfinite(total_weight)) return best;
    std::sort(values.begin(), values.end(),
              [](const auto& a, const auto& b) {
                if (a.x != b.x) return a.x < b.x;
                return a.weighted_y < b.weighted_y;
              });
    if (values.front().x == values.back().x) return best;

    double left_positive_weight = 0.0;
    double left_weight = 0.0;
    for (std::int64_t i = 0; i < n - 1; ++i) {
      left_positive_weight += values[static_cast<std::size_t>(i)].weighted_y;
      left_weight += values[static_cast<std::size_t>(i)].weight;
      const std::int64_t left_n = i + 1;
      const std::int64_t right_n = n - left_n;
      if (left_n < options_.min_samples_leaf || right_n < options_.min_samples_leaf) continue;
      const double xi = values[static_cast<std::size_t>(i)].x;
      const double xj = values[static_cast<std::size_t>(i + 1)].x;
      if (xi == xj) continue;

      const double right_positive_weight = total_positive_weight - left_positive_weight;
      const double right_weight = total_weight - left_weight;
      if (!(left_weight > 0.0) || !(right_weight > 0.0)) continue;
      const double left_impurity = binary_class_impurity(left_positive_weight, left_weight, options_.criterion);
      const double right_impurity = binary_class_impurity(right_positive_weight, right_weight, options_.criterion);
      const double weighted_impurity =
          (left_weight * left_impurity + right_weight * right_impurity) / total_weight;
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
  std::vector<double> sample_weight_host_{};
  std::vector<TreeNode> nodes_{};
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class EnsembleClassifier final : public ClassifierBase {
public:
  explicit EnsembleClassifier(EnsembleFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_ensemble_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "EnsembleClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "EnsembleClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "EnsembleClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "EnsembleClassifier::learn currently has a CPU-only implementation. Provide CPU features for ensemble learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "EnsembleClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> train_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!train_weight_or.ok()) return train_weight_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    method_ = options_.method;
    learning_rate_ = options_.learning_rate;
    initial_score_ = 0.0;
    gb_newton_leaf_updates_ = false;
    const std::vector<double> x_train_host =
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    const std::vector<double> y_train_host =
        labels.y01.reshape({sample_count_}).to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    std::vector<double> base_sample_weights =
        train_weight_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({sample_count_}).contiguous().to_vec<double>();
    double base_weight_sum = std::accumulate(base_sample_weights.begin(), base_sample_weights.end(), 0.0);
    if (!(base_weight_sum > 0.0) || !std::isfinite(base_weight_sum)) {
      return Status::invalid_argument("EnsembleClassifier::learn requires strictly positive resolved sample weights.");
    }
    for (double& w : base_sample_weights) w /= base_weight_sum;

    TrainOptions member_options = fit_options;
    member_options.sample_weights = pluss::series::undefined();
    member_options.class_weight = ClassWeightMode::None;
    member_options.class_weights = pluss::series::undefined();

    const std::int64_t learner_count = options_.learners;
    const std::int64_t subsample_count =
        std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(options_.subsample * sample_count_)));
    const std::int64_t mtry = options_.method == EnsembleMethod::RandomForest
                                  ? std::max<std::int64_t>(1, static_cast<std::int64_t>(std::sqrt(static_cast<double>(feature_count_))))
                                  : feature_count_;

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(learner_count));
    std::uint64_t base_seed = options.random.seed;
    if (!options.random.deterministic) {
      std::random_device rd;
      base_seed = (static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd());
    }

    auto train_bagged_member = [&](std::int64_t learner_index) -> StatusOr<EnsembleMember> {
      std::mt19937_64 rng(splitmix64(base_seed + static_cast<std::uint64_t>(learner_index)));
      std::discrete_distribution<std::int64_t> row_dist(base_sample_weights.begin(), base_sample_weights.end());

      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      if (mtry < feature_count_) {
        std::shuffle(feature_ids.begin(), feature_ids.end(), rng);
        feature_ids.resize(static_cast<std::size_t>(mtry));
        std::sort(feature_ids.begin(), feature_ids.end());
      }

      const std::int64_t learner_features = static_cast<std::int64_t>(feature_ids.size());
      constexpr std::int64_t max_attempts = 16;
      for (std::int64_t attempt = 0; attempt < max_attempts; ++attempt) {
        std::vector<std::int64_t> row_ids(static_cast<std::size_t>(subsample_count), 0);
        std::vector<double> y_sub_host(static_cast<std::size_t>(subsample_count), 0.0);
        double positive_count = 0.0;
        for (std::int64_t i = 0; i < subsample_count; ++i) {
          const std::int64_t row = row_dist(rng);
          row_ids[static_cast<std::size_t>(i)] = row;
          const double yv = y_train_host[static_cast<std::size_t>(row)];
          y_sub_host[static_cast<std::size_t>(i)] = yv;
          positive_count += yv;
        }
        if (positive_count <= 0.0 || positive_count >= static_cast<double>(subsample_count)) {
          continue;
        }

        std::vector<double> x_sub_host = project_rows_with_sampling(
            x_train_host, row_ids, feature_count_, feature_ids, learner_features);

        const Series x_sub = pluss::make_series(std::move(x_sub_host), {subsample_count, learner_features}, f64_cpu());
        const Series y_sub = pluss::make_series(std::move(y_sub_host), {subsample_count}, f64_cpu());
        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_sub, .layout = ObservationLayout::Rows},
            .y = y_sub,
        };

        auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{});
        Status tree_status = tree->learn(sub_batch, member_options);
        if (!tree_status.ok()) continue;
        EnsembleMember member{};
        member.classifier_tree = std::move(tree);
        member.feature_ids = std::move(feature_ids);
        member.weight = 1.0;
        member.flavor = LearnerFlavor::ClassifierTree;
        return StatusOr<EnsembleMember>(std::move(member));
      }

      return StatusOr<EnsembleMember>(Status::internal(
          "EnsembleClassifier::learn could not draw a class-diverse bootstrap sample for learner "
          + std::to_string(learner_index) + "."));
    };

    if (options_.method == EnsembleMethod::GradientBoost) {
      constexpr double eps = 1e-12;
      auto logit = [eps](double p) -> double {
        const double pc = std::clamp(p, eps, 1.0 - eps);
        return std::log(pc / (1.0 - pc));
      };

      const double mean_label =
          std::inner_product(y_train_host.begin(), y_train_host.end(), base_sample_weights.begin(), 0.0);
      initial_score_ = logit(mean_label);
      std::vector<double> running_scores(static_cast<std::size_t>(sample_count_), initial_score_);
      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      const std::int64_t learner_features = feature_count_;
      std::mt19937_64 rng(splitmix64(base_seed));
      gb_newton_leaf_updates_ = true;

      for (std::int64_t l = 0; l < learner_count; ++l) {
        std::vector<std::int64_t> row_ids =
            draw_stochastic_boost_rows(rng, base_sample_weights, sample_count_, subsample_count);
        std::vector<double> residual_targets(static_cast<std::size_t>(subsample_count), 0.0);
        for (std::int64_t i = 0; i < subsample_count; ++i) {
          const std::int64_t row = row_ids[static_cast<std::size_t>(i)];
          const double prob = sigmoid_score(running_scores[static_cast<std::size_t>(row)]);
          residual_targets[static_cast<std::size_t>(i)] =
              y_train_host[static_cast<std::size_t>(row)] - prob;
        }

        std::vector<double> x_sub_host = project_rows_with_sampling(
            x_train_host, row_ids, feature_count_, feature_ids, learner_features);

        const Series x_sub = pluss::make_series(std::move(x_sub_host), {subsample_count, learner_features}, f64_cpu());
        const Series y_sub = pluss::make_series(std::move(residual_targets), {subsample_count}, f64_cpu());
        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_sub, .layout = ObservationLayout::Rows},
            .y = y_sub,
        };

        StatusOr<std::unique_ptr<RegressorBase>> tree_or = make_regressor(ModelSpec{
            .task = TaskKind::Regression,
            .family = ModelFamily::Tree,
            .options = TreeFamilyOptions{.criterion = TreeCriterion::Mse},
        });
        if (!tree_or.ok()) return tree_or.status();
        std::unique_ptr<RegressorBase> tree = tree_or.move_value();
        Status tree_status = tree->learn(sub_batch, member_options);
        if (!tree_status.ok()) {
          return Status::internal(
              "EnsembleClassifier::learn failed while fitting gradient-boost tree for learner "
              + std::to_string(l) + ".");
        }

        StatusOr<std::vector<Series>> tree_state_or = tree->dump_state();
        if (!tree_state_or.ok()) return tree_state_or.status();
        std::vector<Series> tree_state = tree_state_or.move_value();
        std::vector<double> stage_update{};
        Status update_status = apply_binomial_newton_leaf_updates(tree_state,
                                                                  x_train_host,
                                                                  y_train_host,
                                                                  base_sample_weights,
                                                                  running_scores,
                                                                  row_ids,
                                                                  sample_count_,
                                                                  feature_count_,
                                                                  stage_update);
        if (!update_status.ok()) return update_status;
        Status load_status = tree->load_state(pluss::msharp::RangeView::Series(tree_state));
        if (!load_status.ok()) return load_status;

        for (std::int64_t i = 0; i < sample_count_; ++i) {
          running_scores[static_cast<std::size_t>(i)] += learning_rate_ * stage_update[static_cast<std::size_t>(i)];
        }

        EnsembleMember member{};
        member.regressor_tree = std::move(tree);
        member.feature_ids = feature_ids;
        member.weight = learning_rate_;
        member.flavor = LearnerFlavor::RegressorTree;
        learners_.push_back(std::move(member));
      }
    } else if (options_.method == EnsembleMethod::AdaBoost) {
      std::vector<double> sample_weights = base_sample_weights;
      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      const std::int64_t learner_features = feature_count_;
      std::mt19937_64 rng(splitmix64(base_seed));

      for (std::int64_t l = 0; l < learner_count; ++l) {
        constexpr std::int64_t max_attempts = 8;
        bool built = false;
        for (std::int64_t attempt = 0; attempt < max_attempts && !built; ++attempt) {
          std::vector<std::int64_t> row_ids =
              draw_weighted_bootstrap_rows(rng, sample_weights, sample_count_);
          std::vector<double> y_sub_host(static_cast<std::size_t>(sample_count_), 0.0);
          for (std::int64_t i = 0; i < sample_count_; ++i) {
            const std::int64_t row = row_ids[static_cast<std::size_t>(i)];
            y_sub_host[static_cast<std::size_t>(i)] = y_train_host[static_cast<std::size_t>(row)];
          }
          std::vector<double> x_sub_host = project_rows_with_sampling(
              x_train_host, row_ids, feature_count_, feature_ids, learner_features);
          const Series x_sub = pluss::make_series(std::move(x_sub_host), {sample_count_, learner_features}, f64_cpu());
          const Series y_sub = pluss::make_series(std::move(y_sub_host), {sample_count_}, f64_cpu());
          const LabeledBatch sub_batch{
              .features = FeatureBatch{.x = x_sub, .layout = ObservationLayout::Rows},
              .y = y_sub,
          };

          auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{});
          Status tree_status = tree->learn(sub_batch, member_options);
          if (!tree_status.ok()) continue;

          std::vector<double> probs{};
          Status pred_status = tree->predict_prob_host_rows(x_train_host, sample_count_, feature_count_, probs);
          if (!pred_status.ok()) return pred_status;

          double weighted_error = 0.0;
          for (std::int64_t i = 0; i < sample_count_; ++i) {
            const bool pred_pos = probs[static_cast<std::size_t>(i)] >= 0.5;
            const bool true_pos = y_train_host[static_cast<std::size_t>(i)] >= 0.5;
            if (pred_pos != true_pos) {
              weighted_error += sample_weights[static_cast<std::size_t>(i)];
            }
          }
          weighted_error = std::clamp(weighted_error, 1e-12, 1.0 - 1e-12);
          if (weighted_error >= 0.5) continue;

          const double alpha = 0.5 * std::log((1.0 - weighted_error) / weighted_error) * learning_rate_;
          double norm = 0.0;
          for (std::int64_t i = 0; i < sample_count_; ++i) {
            const double yi = y_train_host[static_cast<std::size_t>(i)] >= 0.5 ? 1.0 : -1.0;
            const double pi = probs[static_cast<std::size_t>(i)] >= 0.5 ? 1.0 : -1.0;
            sample_weights[static_cast<std::size_t>(i)] *= std::exp(-alpha * yi * pi);
            norm += sample_weights[static_cast<std::size_t>(i)];
          }
          if (!(norm > 0.0) || !std::isfinite(norm)) {
            return Status::numerical_failure("EnsembleClassifier::learn AdaBoost weight normalization failed.");
          }
          for (double& w : sample_weights) w /= norm;

          EnsembleMember member{};
          member.classifier_tree = std::move(tree);
          member.feature_ids = feature_ids;
          member.weight = alpha;
          member.flavor = LearnerFlavor::ClassifierTree;
          learners_.push_back(std::move(member));
          built = true;
        }
        if (!built) {
          return Status::internal(
              "EnsembleClassifier::learn failed while fitting adaboost tree for learner "
              + std::to_string(l) + ".");
        }
      }
    } else {
      gb_newton_leaf_updates_ = false;
      const std::int64_t workers = resolved_parallel_workers(learner_count);
      if (workers <= 1) {
        for (std::int64_t l = 0; l < learner_count; ++l) {
          StatusOr<EnsembleMember> member_or = train_bagged_member(l);
          if (!member_or.ok()) return member_or.status();
          learners_.push_back(member_or.move_value());
        }
      } else {
        for (std::int64_t start = 0; start < learner_count; start += workers) {
          const std::int64_t end = std::min(learner_count, start + workers);
          std::vector<std::future<StatusOr<EnsembleMember>>> futures{};
          futures.reserve(static_cast<std::size_t>(end - start));
          for (std::int64_t l = start; l < end; ++l) {
            futures.emplace_back(std::async(std::launch::async, [&, l]() { return train_bagged_member(l); }));
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
      return Status::internal("EnsembleClassifier::learn did not build any learners.");
    }

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "EnsembleClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "EnsembleClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "EnsembleClassifier::infer_scores currently has a CPU-only implementation. Provide CPU features for ensemble inference."));
    }

    const std::vector<double> x_host = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    std::vector<double> scores_sum(static_cast<std::size_t>(matrix.sample_count), 0.0);
    if (method_ == EnsembleMethod::GradientBoost) {
      std::fill(scores_sum.begin(), scores_sum.end(), initial_score_);
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
      std::vector<double> member_values{};
      if (member.flavor == LearnerFlavor::RegressorTree) {
        if (!member.regressor_tree) {
          return StatusOr<std::vector<double>>(
              Status::internal("EnsembleClassifier::infer_scores missing regressor tree member."));
        }
        const Series x_series = pluss::make_series(*x_input, {matrix.sample_count, infer_feature_count}, f64_cpu());
        const FeatureBatch fb{.x = x_series, .layout = ObservationLayout::Rows};
        StatusOr<Series> pred_or = member.regressor_tree->infer(fb);
        if (!pred_or.ok()) return StatusOr<std::vector<double>>(pred_or.status());
        member_values =
            pred_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({matrix.sample_count}).contiguous().to_vec<double>();
        for (double& value : member_values) {
          value *= member.weight;
        }
        return StatusOr<std::vector<double>>(std::move(member_values));
      }
      if (!member.classifier_tree) {
        return StatusOr<std::vector<double>>(
            Status::internal("EnsembleClassifier::infer_scores missing classifier tree member."));
      }
      Status member_status = member.classifier_tree->predict_prob_host_rows(
          *x_input, matrix.sample_count, infer_feature_count, member_values);
      if (!member_status.ok()) return StatusOr<std::vector<double>>(member_status);
      if (method_ == EnsembleMethod::GradientBoost) {
        constexpr double eps = 1e-12;
        for (double& value : member_values) {
          const double p = std::clamp(value, eps, 1.0 - eps);
          value = member.weight * std::clamp(std::log(p / (1.0 - p)), -8.0, 8.0);
        }
      } else if (method_ == EnsembleMethod::AdaBoost) {
        for (double& value : member_values) {
          value = value >= 0.5 ? member.weight : -member.weight;
        }
      }
      return StatusOr<std::vector<double>>(std::move(member_values));
    };

    const std::int64_t workers = resolved_parallel_workers(static_cast<std::int64_t>(learners_.size()));
    if (workers <= 1) {
      for (std::size_t mi = 0; mi < learners_.size(); ++mi) {
        StatusOr<std::vector<double>> member_scores_or = infer_member(mi);
        if (!member_scores_or.ok()) return StatusOr<Series>(member_scores_or.status());
        const auto& member_scores = member_scores_or.value();
        for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
          scores_sum[static_cast<std::size_t>(i)] += member_scores[static_cast<std::size_t>(i)];
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
          StatusOr<std::vector<double>> member_scores_or = futures[mi - start].get();
          if (!member_scores_or.ok()) return StatusOr<Series>(member_scores_or.status());
          const auto& member_scores = member_scores_or.value();
          for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
            scores_sum[static_cast<std::size_t>(i)] += member_scores[static_cast<std::size_t>(i)];
          }
        }
      }
    }

    if (method_ == EnsembleMethod::GradientBoost || method_ == EnsembleMethod::AdaBoost) {
      const Series scores = pluss::make_series(scores_sum, {matrix.sample_count}, f64_cpu());
      return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
    }

    const double denom = static_cast<double>(learners_.size());
    constexpr double eps = 1e-12;
    for (double& v : scores_sum) {
      v /= denom;
      v = std::clamp(v, eps, 1.0 - eps);
    }
    const Series probs = pluss::make_series(scores_sum, {matrix.sample_count}, f64_cpu());
    const Series scores = probs.log().sub(probs.mul(-1.0).add(1.0).log());
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "EnsembleClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "EnsembleClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("EnsembleClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + learners_.size() * 5);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             static_cast<double>(sample_count_),
                             static_cast<double>(feature_count_),
                             negative_label_,
                             positive_label_,
                             static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                             static_cast<double>(learners_.size()),
                             static_cast<double>(static_cast<std::int64_t>(method_)),
                             initial_score_,
                             learning_rate_,
                             3.0,
                             gb_newton_leaf_updates_ ? 1.0 : 0.0}));
    for (const auto& member : learners_) {
      out.push_back(state_i64(member.feature_ids));
      out.push_back(state_f64({member.weight, static_cast<double>(static_cast<std::int64_t>(member.flavor))}));
      StatusOr<std::vector<Series>> tree_state_or =
          member.flavor == LearnerFlavor::RegressorTree
              ? (member.regressor_tree
                     ? member.regressor_tree->dump_state()
                     : StatusOr<std::vector<Series>>(Status::internal("EnsembleClassifier::dump_state missing regressor tree member.")))
              : (member.classifier_tree
                     ? member.classifier_tree->dump_state()
                     : StatusOr<std::vector<Series>>(Status::internal("EnsembleClassifier::dump_state missing classifier tree member.")));
      if (!tree_state_or.ok()) return StatusOr<std::vector<Series>>(tree_state_or.status());
      const auto& tree_state = tree_state_or.value();
      if (tree_state.empty()) {
        out.push_back(state_i64({0}));
        continue;
      }
      out.push_back(state_i64({static_cast<std::int64_t>(tree_state.size())}));
      out.insert(out.end(), tree_state.begin(), tree_state.end());
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("EnsembleClassifier::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("EnsembleClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    negative_label_ = meta[3];
    positive_label_ = meta[4];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[5]));
    const std::int64_t learner_count = static_cast<std::int64_t>(meta[6]);
    method_ = meta.size() >= 8
                  ? static_cast<EnsembleMethod>(static_cast<std::int64_t>(meta[7]))
                  : EnsembleMethod::Bagging;
    initial_score_ = meta.size() >= 9 ? meta[8] : 0.0;
    learning_rate_ = meta.size() >= 10 ? meta[9] : options_.learning_rate;
    const double state_version = meta.size() >= 11 ? meta[10] : 1.0;
    const bool v2_state = state_version >= 2.0;
    const bool v3_state = state_version >= 3.0;
    gb_newton_leaf_updates_ =
        meta.size() >= 12 ? (meta[11] > 0.5) : false;

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, learner_count)));
    std::size_t cursor = 1;
    for (std::int64_t li = 0; li < learner_count; ++li) {
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state truncated state.");
      EnsembleMember member{};
      member.feature_ids = vec_i64(state[cursor++]);
      member.weight = 1.0;
      member.flavor = LearnerFlavor::ClassifierTree;
      if (v2_state) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state missing member weight.");
        const auto weight_vec = vec_f64(state[cursor++]);
        member.weight = weight_vec.empty() ? 1.0 : weight_vec.front();
        if (v3_state && weight_vec.size() >= 2) {
          member.flavor = static_cast<LearnerFlavor>(static_cast<std::int64_t>(weight_vec[1]));
        }
      } else if (method_ == EnsembleMethod::GradientBoost) {
        member.weight = learning_rate_;
      }
      if (method_ == EnsembleMethod::GradientBoost && !v3_state) {
        member.flavor = gb_newton_leaf_updates_ ? LearnerFlavor::RegressorTree : LearnerFlavor::ClassifierTree;
      }
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state missing tree state size.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t tree_items = size_vec.empty() ? 0 : size_vec.front();
      if (tree_items <= 0) {
        learners_.push_back(std::move(member));
        continue;
      }
      std::vector<Series> tree_state{};
      tree_state.reserve(static_cast<std::size_t>(tree_items));
      for (std::int64_t ti = 0; ti < tree_items; ++ti) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state truncated tree state.");
        tree_state.push_back(state[cursor++]);
      }
      if (member.flavor == LearnerFlavor::RegressorTree) {
        StatusOr<std::unique_ptr<RegressorBase>> tree_or = make_regressor(ModelSpec{
            .task = TaskKind::Regression,
            .family = ModelFamily::Tree,
            .options = TreeFamilyOptions{.criterion = TreeCriterion::Mse},
        });
        if (!tree_or.ok()) return tree_or.status();
        std::unique_ptr<RegressorBase> tree = tree_or.move_value();
        Status load_status = tree->load_state(pluss::msharp::RangeView::Series(tree_state));
        if (!load_status.ok()) return load_status;
        member.regressor_tree = std::move(tree);
      } else {
        auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{.criterion = TreeCriterion::Gini});
        Status load_status = tree->load_state(pluss::msharp::RangeView::Series(tree_state));
        if (!load_status.ok()) return load_status;
        member.classifier_tree = std::move(tree);
      }
      learners_.push_back(std::move(member));
    }
    return Status::success();
  }

private:
  enum class LearnerFlavor : std::int64_t {
    ClassifierTree = 0,
    RegressorTree = 1,
  };

  struct EnsembleMember {
    std::unique_ptr<TreeClassifier> classifier_tree{};
    std::unique_ptr<RegressorBase> regressor_tree{};
    std::vector<std::int64_t> feature_ids{};
    double weight{1.0};
    LearnerFlavor flavor{LearnerFlavor::ClassifierTree};
  };

  [[nodiscard]] static bool is_full_route(const std::vector<std::int64_t>& feature_ids,
                                          std::int64_t feature_count) {
    if (static_cast<std::int64_t>(feature_ids.size()) != feature_count) return false;
    for (std::int64_t i = 0; i < feature_count; ++i) {
      if (feature_ids[static_cast<std::size_t>(i)] != i) return false;
    }
    return true;
  }

  [[nodiscard]] static std::vector<double> project_rows_with_sampling(const std::vector<double>& x_host,
                                                                      const std::vector<std::int64_t>& row_ids,
                                                                      std::int64_t feature_count,
                                                                      const std::vector<std::int64_t>& feature_ids,
                                                                      std::int64_t learner_features) {
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

  [[nodiscard]] static std::vector<std::int64_t> draw_stochastic_boost_rows(std::mt19937_64& rng,
                                                                             const std::vector<double>& sample_weights,
                                                                             std::int64_t sample_count,
                                                                             std::int64_t subsample_count) {
    if (subsample_count >= sample_count) {
      std::vector<std::int64_t> rows(static_cast<std::size_t>(sample_count), 0);
      std::iota(rows.begin(), rows.end(), static_cast<std::int64_t>(0));
      return rows;
    }
    std::vector<std::int64_t> rows(static_cast<std::size_t>(subsample_count), 0);
    std::discrete_distribution<std::int64_t> dist(sample_weights.begin(), sample_weights.end());
    for (std::int64_t i = 0; i < subsample_count; ++i) {
      rows[static_cast<std::size_t>(i)] = dist(rng);
    }
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

  struct FlatTreeNode {
    bool is_leaf{true};
    std::int64_t feature{-1};
    double threshold{0.0};
    std::int64_t left{-1};
    std::int64_t right{-1};
    double leaf_value{0.0};
  };

  [[nodiscard]] static double sigmoid_score(double x) {
    if (x >= 0.0) {
      const double z = std::exp(-x);
      return 1.0 / (1.0 + z);
    }
    const double z = std::exp(x);
    return z / (1.0 + z);
  }

  [[nodiscard]] static StatusOr<std::vector<FlatTreeNode>> parse_flat_regression_tree(const std::vector<double>& flat) {
    if (flat.size() % 6 != 0) {
      return StatusOr<std::vector<FlatTreeNode>>(
          Status::invalid_argument("EnsembleClassifier::GradientBoost tree state is malformed."));
    }
    const std::size_t node_count = flat.size() / 6;
    std::vector<FlatTreeNode> nodes(node_count);
    for (std::size_t i = 0; i < node_count; ++i) {
      const std::size_t base = i * 6;
      nodes[i].is_leaf = flat[base] > 0.5;
      nodes[i].feature = static_cast<std::int64_t>(flat[base + 1]);
      nodes[i].threshold = flat[base + 2];
      nodes[i].left = static_cast<std::int64_t>(flat[base + 3]);
      nodes[i].right = static_cast<std::int64_t>(flat[base + 4]);
      nodes[i].leaf_value = flat[base + 5];
    }
    return StatusOr<std::vector<FlatTreeNode>>(std::move(nodes));
  }

  [[nodiscard]] static Status apply_binomial_newton_leaf_updates(std::vector<Series>& tree_state,
                                                                 const std::vector<double>& x_host,
                                                                 const std::vector<double>& y01,
                                                                 const std::vector<double>& sample_weights,
                                                                 const std::vector<double>& running_scores,
                                                                 const std::vector<std::int64_t>& inbag_rows,
                                                                 std::int64_t sample_count,
                                                                 std::int64_t feature_count,
                                                                 std::vector<double>& full_stage_update) {
    if (tree_state.size() < 2) {
      return Status::invalid_argument(
          "EnsembleClassifier::GradientBoost requires a tree dump with metadata + nodes.");
    }
    std::vector<double> flat = vec_f64(tree_state[1]);
    StatusOr<std::vector<FlatTreeNode>> nodes_or = parse_flat_regression_tree(flat);
    if (!nodes_or.ok()) return nodes_or.status();
    std::vector<FlatTreeNode> nodes = nodes_or.move_value();
    if (nodes.empty()) {
      return Status::invalid_argument("EnsembleClassifier::GradientBoost received an empty tree.");
    }

    std::vector<std::int64_t> leaf_for_row(static_cast<std::size_t>(sample_count), 0);
    for (std::int64_t row = 0; row < sample_count; ++row) {
      std::int64_t node_idx = 0;
      while (!nodes[static_cast<std::size_t>(node_idx)].is_leaf) {
        const FlatTreeNode& node = nodes[static_cast<std::size_t>(node_idx)];
        if (node.feature < 0 || node.feature >= feature_count
            || node.left < 0 || node.right < 0
            || node.left >= static_cast<std::int64_t>(nodes.size())
            || node.right >= static_cast<std::int64_t>(nodes.size())) {
          return Status::invalid_argument("EnsembleClassifier::GradientBoost tree contains invalid split indices.");
        }
        const double xv = x_host[static_cast<std::size_t>(row * feature_count + node.feature)];
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      leaf_for_row[static_cast<std::size_t>(row)] = node_idx;
    }

    constexpr double hessian_floor = 1e-150;
    std::vector<double> numerator(nodes.size(), 0.0);
    std::vector<double> denominator(nodes.size(), 0.0);
    for (std::int64_t row : inbag_rows) {
      if (row < 0 || row >= sample_count) {
        return Status::invalid_argument("EnsembleClassifier::GradientBoost in-bag row index is out of range.");
      }
      const std::size_t r = static_cast<std::size_t>(row);
      const std::size_t leaf = static_cast<std::size_t>(leaf_for_row[r]);
      const double prob = sigmoid_score(running_scores[r]);
      const double wi = sample_weights[r];
      numerator[leaf] += wi * (y01[r] - prob);
      denominator[leaf] += wi * prob * (1.0 - prob);
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
      if (!nodes[i].is_leaf) continue;
      const double denom = denominator[i];
      nodes[i].leaf_value = std::abs(denom) < hessian_floor ? 0.0 : (numerator[i] / denom);
      flat[i * 6 + 5] = nodes[i].leaf_value;
    }

    tree_state[1] = state_f64(std::move(flat));
    full_stage_update.assign(static_cast<std::size_t>(sample_count), 0.0);
    for (std::int64_t row = 0; row < sample_count; ++row) {
      const std::size_t leaf = static_cast<std::size_t>(leaf_for_row[static_cast<std::size_t>(row)]);
      full_stage_update[static_cast<std::size_t>(row)] = nodes[leaf].leaf_value;
    }
    return Status::success();
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
  double initial_score_{0.0};
  double learning_rate_{0.1};
  bool gb_newton_leaf_updates_{false};
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] Series labels_from_scores(const Series& scores,
                                        double negative_label,
                                        double positive_label,
                                        double positive_threshold,
                                        Platform out_platform,
                                        ScalarType out_dtype) {
  const double t = std::clamp(positive_threshold, 1e-9, 1.0 - 1e-9);
  const double score_threshold = std::log(t / (1.0 - t));
  const Series mask = scores.ge(score_threshold).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               double positive_threshold,
                                               Platform out_platform,
                                               ScalarType out_dtype) {
  const Series mask = probs.ge(positive_threshold).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

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
  double rs = r.square().sum().reshape({1}).item<double>();
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
    const double denom = p.mul(ap).sum().reshape({1}).item<double>();
    if (!std::isfinite(denom) || std::abs(denom) <= eps) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered unstable CG denominator."));
    }
    const double alpha = rs / denom;
    x = x.add(p.mul(alpha));
    r = r.sub(ap.mul(alpha));
    const double rs_new = r.square().sum().reshape({1}).item<double>();
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

[[nodiscard]] Series kernel_scores_cpu(const Series& query_rows,
                                       const Series& train_rows,
                                       const Series& alpha,
                                       KernelKind kind,
                                       double gamma) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto av = alpha.to(pluss::pCPU, ScalarType::Float64).reshape({alpha.numel()}).contiguous().to_vec<double>();
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

class BayesClassifier final : public ClassifierBase {
public:
  explicit BayesClassifier(BayesFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_bayes_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "BayesClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "BayesClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "BayesClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "BayesClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> train_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!train_weight_or.ok()) return train_weight_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    likelihood_ = resolved_bayes_likelihood(options_);
    const Series x = matrix.x_rows;
    const Series y01 = labels.y01.to(x.platform(), ScalarType::Float64).reshape({sample_count_});
    const Series sample_weight = train_weight_or.value().to(x.platform(), ScalarType::Float64).reshape({sample_count_});
    const Series ycol = y01.reshape({sample_count_, 1});
    const Series one = pluss::series::ones({sample_count_, 1}, Config{}.with_dtype(ScalarType::Float64).with_platform(x.platform()));
    const Series wcol = sample_weight.reshape({sample_count_, 1});
    const Series mask1 = ycol.mul(wcol);
    const Series mask0 = one.sub(ycol).mul(wcol);

    const double n1 = mask1.sum().item<double>();
    const double n0 = mask0.sum().item<double>();
    if (n0 <= 0.0 || n1 <= 0.0) {
      return Status::invalid_argument("BayesClassifier::learn requires both classes to be present in training data.");
    }

    mean0_ = pluss::series::undefined();
    mean1_ = pluss::series::undefined();
    var0_ = pluss::series::undefined();
    var1_ = pluss::series::undefined();
    log_prob0_ = pluss::series::undefined();
    log_prob1_ = pluss::series::undefined();
    log_inv_prob0_ = pluss::series::undefined();
    log_inv_prob1_ = pluss::series::undefined();

    if (likelihood_ == BayesLikelihood::Gaussian) {
      mean0_ = x.mul(mask0).sum(0, false).div(n0).reshape({feature_count_});
      mean1_ = x.mul(mask1).sum(0, false).div(n1).reshape({feature_count_});

      const Series d0 = x.sub(mean0_.reshape({1, feature_count_}));
      const Series d1 = x.sub(mean1_.reshape({1, feature_count_}));
      var0_ = d0.square().mul(mask0).sum(0, false).div(n0).add(options_.smoothing).reshape({feature_count_});
      var1_ = d1.square().mul(mask1).sum(0, false).div(n1).add(options_.smoothing).reshape({feature_count_});
      if (!all_finite(var0_) || var0_.le(0.0).any().item<bool>()) {
        return Status::numerical_failure("BayesClassifier::learn produced invalid variance for class 0.");
      }
      if (!all_finite(var1_) || var1_.le(0.0).any().item<bool>()) {
        return Status::numerical_failure("BayesClassifier::learn produced invalid variance for class 1.");
      }
    } else if (likelihood_ == BayesLikelihood::Bernoulli) {
      const Series xb = x.gt(0.0).to(ScalarType::Float64);
      const Series alpha = pluss::series::ones({1}, f64_on(x.platform())).mul(options_.smoothing);
      const Series denom0 = pluss::series::ones({1}, f64_on(x.platform())).mul(n0).add(alpha.mul(2.0));
      const Series denom1 = pluss::series::ones({1}, f64_on(x.platform())).mul(n1).add(alpha.mul(2.0));
      const Series p0 = xb.mul(mask0).sum(0, false).add(alpha).div(denom0).reshape({feature_count_}).clamp(1e-12, 1.0 - 1e-12);
      const Series p1 = xb.mul(mask1).sum(0, false).add(alpha).div(denom1).reshape({feature_count_}).clamp(1e-12, 1.0 - 1e-12);
      log_prob0_ = p0.log();
      log_prob1_ = p1.log();
      log_inv_prob0_ = p0.mul(-1.0).add(1.0).log();
      log_inv_prob1_ = p1.mul(-1.0).add(1.0).log();
    } else if (likelihood_ == BayesLikelihood::Multinomial) {
      if (x.lt(0.0).any().item<bool>()) {
        return Status::invalid_argument("BayesClassifier::learn Multinomial likelihood expects non-negative features.");
      }
      const Series x_nonneg = x;
      const Series alpha = pluss::series::ones({1}, f64_on(x.platform())).mul(options_.smoothing);
      const Series count0 = x_nonneg.mul(mask0).sum(0, false);
      const Series count1 = x_nonneg.mul(mask1).sum(0, false);
      const Series total0 = count0.sum().add(alpha.mul(static_cast<double>(feature_count_)));
      const Series total1 = count1.sum().add(alpha.mul(static_cast<double>(feature_count_)));
      const Series p0 = count0.add(alpha).div(total0).reshape({feature_count_}).clamp_min(1e-12);
      const Series p1 = count1.add(alpha).div(total1).reshape({feature_count_}).clamp_min(1e-12);
      log_prob0_ = p0.log();
      log_prob1_ = p1.log();
    } else if (likelihood_ == BayesLikelihood::Complement) {
      if (x.lt(0.0).any().item<bool>()) {
        return Status::invalid_argument("BayesClassifier::learn Complement likelihood expects non-negative features.");
      }
      const Series x_nonneg = x;
      const Series alpha = pluss::series::ones({1}, f64_on(x.platform())).mul(options_.smoothing);
      const Series count0 = x_nonneg.mul(mask0).sum(0, false).reshape({feature_count_});
      const Series count1 = x_nonneg.mul(mask1).sum(0, false).reshape({feature_count_});
      const Series comp0 = count1.add(alpha);
      const Series comp1 = count0.add(alpha);
      const Series p0 = comp0.div(comp0.sum()).clamp_min(1e-12);
      const Series p1 = comp1.div(comp1.sum()).clamp_min(1e-12);
      log_prob0_ = p0.log().mul(-1.0);
      log_prob1_ = p1.log().mul(-1.0);
    } else {
      return Status::invalid_argument("BayesClassifier::learn encountered unsupported BayesLikelihood.");
    }

    const double total_weight = n0 + n1;
    prior0_ = n0 / total_weight;
    prior1_ = n1 / total_weight;
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "BayesClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "BayesClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    const Series x = matrix.x_rows;
    Series logp0 = pluss::series::undefined();
    Series logp1 = pluss::series::undefined();

    if (likelihood_ == BayesLikelihood::Gaussian) {
      constexpr double two_pi = 6.28318530717958647692;
      const Series mu0 = mean0_.reshape({1, feature_count_});
      const Series mu1 = mean1_.reshape({1, feature_count_});
      const Series v0 = var0_.reshape({1, feature_count_});
      const Series v1 = var1_.reshape({1, feature_count_});

      const Series d0 = x.sub(mu0);
      const Series d1 = x.sub(mu1);
      const Series q0 = d0.square().div(v0).sum(1, false);
      const Series q1 = d1.square().div(v1).sum(1, false);
      const Series c0 = v0.mul(two_pi).log().sum(1, false).mul(-0.5).add(std::log(prior0_));
      const Series c1 = v1.mul(two_pi).log().sum(1, false).mul(-0.5).add(std::log(prior1_));
      logp0 = q0.mul(-0.5).add(c0);
      logp1 = q1.mul(-0.5).add(c1);
    } else if (likelihood_ == BayesLikelihood::Bernoulli) {
      const Series xb = x.gt(0.0).to(ScalarType::Float64);
      const Series one = pluss::series::ones({1, feature_count_}, f64_on(x.platform()));
      logp0 = xb.mul(log_prob0_.reshape({1, feature_count_}))
                  .add(one.sub(xb).mul(log_inv_prob0_.reshape({1, feature_count_})))
                  .sum(1, false)
                  .add(std::log(prior0_));
      logp1 = xb.mul(log_prob1_.reshape({1, feature_count_}))
                  .add(one.sub(xb).mul(log_inv_prob1_.reshape({1, feature_count_})))
                  .sum(1, false)
                  .add(std::log(prior1_));
    } else if (likelihood_ == BayesLikelihood::Multinomial) {
      if (x.lt(0.0).any().item<bool>()) {
        return StatusOr<Series>(
            Status::invalid_argument("BayesClassifier::infer_scores Multinomial likelihood expects non-negative features."));
      }
      const Series x_nonneg = x;
      logp0 = x_nonneg.mul(log_prob0_.reshape({1, feature_count_})).sum(1, false).add(std::log(prior0_));
      logp1 = x_nonneg.mul(log_prob1_.reshape({1, feature_count_})).sum(1, false).add(std::log(prior1_));
    } else if (likelihood_ == BayesLikelihood::Complement) {
      if (x.lt(0.0).any().item<bool>()) {
        return StatusOr<Series>(
            Status::invalid_argument("BayesClassifier::infer_scores Complement likelihood expects non-negative features."));
      }
      const Series x_nonneg = x;
      logp0 = x_nonneg.mul(log_prob0_.reshape({1, feature_count_})).sum(1, false);
      logp1 = x_nonneg.mul(log_prob1_.reshape({1, feature_count_})).sum(1, false);
    } else {
      return StatusOr<Series>(Status::invalid_argument("BayesClassifier::infer_scores encountered unsupported BayesLikelihood."));
    }

    const Series scores = logp1.sub(logp0);
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "BayesClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "BayesClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("BayesClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   prior0_,
                   prior1_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                   static_cast<double>(static_cast<std::int64_t>(likelihood_))}),
        mean0_,
        mean1_,
        var0_,
        var1_,
        log_prob0_,
        log_prob1_,
        log_inv_prob0_,
        log_inv_prob1_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 5) return Status::invalid_argument("BayesClassifier::load_state expects at least 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("BayesClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    prior0_ = meta[3];
    prior1_ = meta[4];
    negative_label_ = meta[5];
    positive_label_ = meta[6];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
    if (meta.size() >= 9) {
      likelihood_ = static_cast<BayesLikelihood>(static_cast<std::int64_t>(meta[8]));
    } else {
      likelihood_ = BayesLikelihood::Gaussian;
    }
    mean0_ = state[1];
    mean1_ = state[2];
    var0_ = state[3];
    var1_ = state[4];
    log_prob0_ = state.size() > 5 ? state[5] : pluss::series::undefined();
    log_prob1_ = state.size() > 6 ? state[6] : pluss::series::undefined();
    log_inv_prob0_ = state.size() > 7 ? state[7] : pluss::series::undefined();
    log_inv_prob1_ = state.size() > 8 ? state[8] : pluss::series::undefined();
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
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  Series mean0_ = pluss::series::undefined();
  Series mean1_ = pluss::series::undefined();
  Series var0_ = pluss::series::undefined();
  Series var1_ = pluss::series::undefined();
  Series log_prob0_ = pluss::series::undefined();
  Series log_prob1_ = pluss::series::undefined();
  Series log_inv_prob0_ = pluss::series::undefined();
  Series log_inv_prob1_ = pluss::series::undefined();
  double prior0_{0.5};
  double prior1_{0.5};
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
  BayesLikelihood likelihood_{BayesLikelihood::Gaussian};
};

class KernelClassifier final : public ClassifierBase {
public:
  explicit KernelClassifier(KernelFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_kernel_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "KernelClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "KernelClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "KernelClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "KernelClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> train_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!train_weight_or.ok()) return train_weight_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    gamma_ = resolved_gamma(options_.gamma, feature_count_);
    const double nu_scale = options_.form == KernelForm::Nu ? std::max(1e-6, options_.nu) : 1.0;
    lambda_ = 1.0 / (options_.c * nu_scale);
    x_train_ = matrix.x_rows;
    const Series sample_weight = train_weight_or.value().to(x_train_.platform(), ScalarType::Float64).reshape({sample_count_});
    const Series sqrt_weight = sample_weight.sqrt().reshape({sample_count_});
    const Series y_pm1 = labels.y01.to(x_train_.platform(), ScalarType::Float64).mul(2.0).sub(1.0).reshape({sample_count_, 1});
    const Series k_series = kernel_matrix(x_train_, x_train_, options_.kernel, gamma_);
    const Series weighted_kernel = k_series.mul(sqrt_weight.reshape({sample_count_, 1}))
                                      .mul(sqrt_weight.reshape({1, sample_count_}));
    const Series weighted_rhs = y_pm1.reshape({sample_count_}).mul(sqrt_weight);
    StatusOr<Series> beta_or = solve_kernel_dual_cg(weighted_kernel,
                                                     weighted_rhs,
                                                     lambda_,
                                                     options.max_iterations,
                                                     options.tolerance,
                                                     "KernelClassifier::learn");
    Series alpha = pluss::series::undefined();
    if (beta_or.ok()) {
      alpha = beta_or.move_value().mul(sqrt_weight).reshape({sample_count_});
    } else {
      // Fallback for indefinite kernels (for example sigmoid/polynomial edge cases).
      Series shifted =
          weighted_kernel.add(pluss::series::eye(sample_count_, sample_count_, f64_cpu()).to(x_train_.platform()).mul(lambda_));
      const Series beta = pluss::subsp::math::linalg::pinverse(shifted, 1e-12)
                              .matmul(weighted_rhs.reshape({sample_count_, 1}))
                              .reshape({sample_count_});
      alpha = beta.mul(sqrt_weight).reshape({sample_count_});
    }
    if (!all_finite(alpha)) {
      return Status::numerical_failure("KernelClassifier::learn produced non-finite dual coefficients.");
    }

    alpha_ = alpha;
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "KernelClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series scores = pluss::series::undefined();
    if (matrix.x_rows.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
      scores = kernel_scores_cpu(matrix.x_rows, x_train_, alpha_, options_.kernel, gamma_);
    } else {
      const Series k_query = kernel_matrix(matrix.x_rows, x_train_, options_.kernel, gamma_);
      scores = k_query.matmul(alpha_.reshape({sample_count_, 1})).reshape({matrix.sample_count});
    }
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "KernelClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "KernelClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("KernelClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   gamma_,
                   lambda_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        x_train_,
        alpha_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 3) return Status::invalid_argument("KernelClassifier::load_state expects 3 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("KernelClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    gamma_ = meta[3];
    lambda_ = meta[4];
    negative_label_ = meta[5];
    positive_label_ = meta[6];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

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

[[nodiscard]] double linear_l1_term(const Series& weights,
                                    bool fit_intercept,
                                    double lambda_l1) {
  if (lambda_l1 <= 0.0) return 0.0;
  Series penalized = weights;
  if (fit_intercept) {
    const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, penalized.platform());
    penalized = pluss::index_put(penalized, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(penalized.platform())), false);
  }
  return lambda_l1 * penalized.abs().sum().item<double>();
}

[[nodiscard]] Series logistic_gradient(const Series& design,
                                       const Series& y01,
                                       const Series& sample_weight,
                                       const Series& weights,
                                       bool fit_intercept,
                                       double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series probs = logits.sigmoid();
  const Series error = probs.sub(y01).mul(sample_weight);
  const double denom = sample_weight.sum().item<double>();
  Series grad = design.transpose(0, 1).matmul(error.reshape({n, 1})).div(denom).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] double logistic_objective(const Series& design,
                                        const Series& y01,
                                        const Series& sample_weight,
                                        const Series& weights,
                                        bool fit_intercept,
                                        double lambda_l2,
                                        double lambda_l1) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series loss_terms = logits.softplus().sub(y01.mul(logits));
  const double denom = sample_weight.sum().item<double>();
  double loss = loss_terms.mul(sample_weight).sum().div(denom).item<double>();

  if (lambda_l2 > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    loss += 0.5 * lambda_l2 * ridge.square().sum().item<double>();
  }
  loss += linear_l1_term(weights, fit_intercept, lambda_l1);
  return loss;
}

[[nodiscard]] StatusOr<Series> solve_spd_system_with_jitter(const Series& hessian,
                                                            const Series& rhs,
                                                            const char* where) {
  const std::int64_t p = hessian.sizes()[0];
  const Platform platform = hessian.platform();
  const Series eye = pluss::series::eye(p, p, f64_on(platform));
  const std::array<double, 7> jitters{0.0, 1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2};
  for (double jitter : jitters) {
    try {
      Series h = hessian;
      if (jitter > 0.0) h = h.add(eye.mul(jitter));
      const Series chol = pluss::subsp::math::linalg::cholesky(h, false).to(platform, ScalarType::Float64);
      const Series delta =
          pluss::subsp::math::linalg::cholesky_solve(rhs.reshape({p, 1}), chol, false).reshape({p}).to(platform, ScalarType::Float64);
      if (all_finite(delta)) return StatusOr<Series>(delta);
    } catch (const std::exception&) {
    }
  }
  return StatusOr<Series>(Status::numerical_failure(std::string(where) + " failed to solve SPD system."));
}

[[nodiscard]] StatusOr<Series> solve_logistic_newton(const Series& design,
                                                     const Series& y01,
                                                     const Series& sample_weight,
                                                     bool fit_intercept,
                                                     double lambda,
                                                     std::int64_t max_iterations,
                                                     double tolerance,
                                                     const char* where) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t p = design.sizes()[1];
  const Platform platform = design.platform();
  const double denom = sample_weight.sum().item<double>();
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires strictly positive sample weight sum."));
  }

  Series weights = pluss::series::zeros({p}, f64_on(platform));
  Series ridge_diag = pluss::series::zeros({p}, f64_on(platform));
  if (lambda > 0.0) {
    ridge_diag = pluss::series::ones({p}, f64_on(platform)).mul(lambda);
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
      ridge_diag = pluss::index_put(ridge_diag,
                                    std::vector<Series>{idx0},
                                    pluss::series::zeros({1}, f64_on(platform)),
                                    false);
    }
  }
  const Series eye = pluss::series::eye(p, p, f64_on(platform));
  double prev_loss = std::numeric_limits<double>::infinity();

  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const Series logits = design.matmul(weights.reshape({p, 1})).reshape({n});
    const Series probs = logits.sigmoid();
    const Series err = probs.sub(y01).mul(sample_weight);
    Series grad = design.transpose(0, 1).matmul(err.reshape({n, 1})).div(denom).reshape({p});
    if (lambda > 0.0) {
      grad = grad.add(weights.mul(ridge_diag));
    }
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite gradient."));
    }

    const Series r = probs.mul(probs.mul(-1.0).add(1.0)).mul(sample_weight).clamp_min(1e-12);
    Series hessian = design
                         .transpose(0, 1)
                         .matmul(design.mul(r.reshape({n, 1})))
                         .div(denom);
    if (lambda > 0.0) {
      hessian = hessian.add(eye.mul(ridge_diag.reshape({1, p})));
    }
    if (!all_finite(hessian)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite Hessian."));
    }

    StatusOr<Series> delta_or = solve_spd_system_with_jitter(hessian, grad, where);
    if (!delta_or.ok()) return StatusOr<Series>(delta_or.status());
    const Series delta = delta_or.move_value();
    weights = weights.sub(delta);
    if (!all_finite(weights)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
    }

    const double loss = logistic_objective(design, y01, sample_weight, weights, fit_intercept, lambda, 0.0);
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite objective."));
    }
    const double step_inf = delta.abs().max().reshape({1}).item<double>();
    if (step_inf <= tolerance || std::abs(prev_loss - loss) <= tolerance) {
      break;
    }
    prev_loss = loss;
  }

  return StatusOr<Series>(weights);
}

[[nodiscard]] Series hinge_gradient(const Series& design,
                                    const Series& y01,
                                    const Series& sample_weight,
                                    const Series& weights,
                                    bool fit_intercept,
                                    double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  const Series active = margin.gt(0.0).to(ScalarType::Float64);
  const Series coeff = ypm.mul(active).mul(-1.0).mul(sample_weight);
  const double denom = sample_weight.sum().item<double>();
  Series grad = design.transpose(0, 1).matmul(coeff.reshape({n, 1})).div(denom).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] double hinge_objective(const Series& design,
                                     const Series& y01,
                                     const Series& sample_weight,
                                     const Series& weights,
                                     bool fit_intercept,
                                     double lambda_l2,
                                     double lambda_l1) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  const double denom = sample_weight.sum().item<double>();
  double loss = margin.relu().mul(sample_weight).sum().div(denom).item<double>();

  if (lambda_l2 > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    loss += 0.5 * lambda_l2 * ridge.square().sum().item<double>();
  }
  loss += linear_l1_term(weights, fit_intercept, lambda_l1);
  return loss;
}

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested) {
  const Series y = truth01.to(ScalarType::Float64).reshape({truth01.numel()});
  const Series yh = labels01.to(ScalarType::Float64).reshape({labels01.numel()});
  const Series p = probs.to(ScalarType::Float64).reshape({probs.numel()});

  if (y.numel() != yh.numel() || y.numel() != p.numel() || y.numel() == 0) {
    return StatusOr<Series>(Status::shape_mismatch("assess classification metrics require equal non-empty vectors."));
  }

  constexpr double eps = 1e-12;
  const Series yb = y.ge(0.5).to(ScalarType::Float64);
  const Series yhb = yh.ge(0.5).to(ScalarType::Float64);
  const Series clipped = p.clamp(eps, 1.0 - eps);
  const Series one = pluss::series::ones({1}, f64_on(y.platform()));
  const Series zero = pluss::series::zeros({1}, f64_on(y.platform()));

  const Series tp = yb.mul(yhb).sum();
  const Series tn = yb.mul(-1.0).add(1.0).mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series fp = yb.mul(-1.0).add(1.0).mul(yhb).sum();
  const Series fn = yb.mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series accuracy = yb.eq(yhb).to(ScalarType::Float64).mean();
  const Series tpr_den = tp.add(fn);
  const Series tnr_den = tn.add(fp);
  const Series tpr_mask = tpr_den.gt(0.0).to(ScalarType::Float64);
  const Series tnr_mask = tnr_den.gt(0.0).to(ScalarType::Float64);
  const Series tpr = tp.div(tpr_den.clamp_min(1.0)).mul(tpr_mask);
  const Series tnr = tn.div(tnr_den.clamp_min(1.0)).mul(tnr_mask);
  const Series bal_accuracy = tpr.add(tnr).mul(0.5);
  const Series geometric_mean = tpr.mul(tnr).clamp_min(0.0).sqrt();
  const Series balance_quality = geometric_mean.square().mul(tpr.sub(tnr).mul(0.1).add(1.0)).clamp_min(0.0);
  const Series precision_den = tp.add(fp);
  const Series precision_mask = precision_den.gt(0.0).to(ScalarType::Float64);
  const Series precision = tp.div(precision_den.clamp_min(1.0)).mul(precision_mask);
  const Series recall = tpr;
  const Series f1_den = precision.add(recall);
  const Series f1_mask = f1_den.gt(0.0).to(ScalarType::Float64);
  const Series f1 = precision.mul(recall).mul(2.0).div(f1_den.clamp_min(1.0)).mul(f1_mask);
  const Series logloss = yb.mul(clipped.log()).add(yb.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0).mean();

  const Series pos_count = yb.sum();
  const Series neg_count = yb.mul(-1.0).add(1.0).sum();
  const Series has_both = pos_count.gt(0.0).to(ScalarType::Float64).mul(neg_count.gt(0.0).to(ScalarType::Float64));
  const Series pos_safe = pos_count.clamp_min(1.0);
  const Series neg_safe = neg_count.clamp_min(1.0);
  const std::int64_t n = p.numel();
  const Platform platform = p.platform();
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const Series order = p.argsort(0, true, true);
  const Series p_sorted = p.index_select(0, order);
  const Series y_sorted = yb.index_select(0, order);
  const Series one_vec = pluss::series::ones({n}, f64_on(platform));
  const Series tp_cum = y_sorted.cumsum(0, false);
  const Series fp_cum = one_vec.sub(y_sorted).cumsum(0, false);

  Series idx_next = pluss::series::arange(1, n, i64_cfg);
  if (idx_next.numel() == 0) {
    idx_next = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cfg);
  }
  Series p_next = p_sorted.index_select(0, idx_next).reshape({idx_next.numel()});
  if (n == 1) {
    p_next = p_sorted.add(1.0);
  } else {
    p_next = p_next.unite({p_sorted.reshape({n}).index_select(0, pluss::series::arange(n - 1, n, i64_cfg)).add(1.0)}, 0);
  }
  const Series boundary = p_sorted.ne(p_next).reshape({n});
  const Series tpr_curve = tp_cum.div(pos_safe);
  const Series fpr_curve = fp_cum.div(neg_safe);
  const Series tpr_u = tpr_curve.masked_select(boundary);
  const Series fpr_u = fpr_curve.masked_select(boundary);
  const std::int64_t m = fpr_u.numel();
  const Series tpr_prev = (m > 1)
                              ? pluss::series::zeros({1}, f64_on(platform)).unite(
                                    {tpr_u.index_select(0, pluss::series::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::series::zeros({1}, f64_on(platform));
  const Series fpr_prev = (m > 1)
                              ? pluss::series::zeros({1}, f64_on(platform)).unite(
                                    {fpr_u.index_select(0, pluss::series::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::series::zeros({1}, f64_on(platform));
  const Series roc_auc_raw = fpr_u.sub(fpr_prev).mul(tpr_u.add(tpr_prev)).mul(0.5).sum().clamp(0.0, 1.0).reshape({1});
  const Series roc_auc = roc_auc_raw.mul(has_both).add(one.mul(0.5).mul(one.sub(has_both)));

  // Brier score: mean squared error of the positive-class probability.
  const Series brier = p.sub(yb).square().mean().reshape({1});
  // Gini coefficient: 2 * AUC - 1.
  const Series gini = roc_auc.mul(2.0).sub(1.0).reshape({1});
  // Average precision via the shared threshold_curve step-sum.
  bool wants_ap = false;
  for (MetricKind metric : requested) {
    if (metric == MetricKind::AveragePrecision) wants_ap = true;
  }
  Series avg_precision = pluss::series::zeros({1}, f64_on(platform));
  if (wants_ap) {
    StatusOr<double> ap_or = average_precision(p, yb);
    if (!ap_or.ok()) return StatusOr<Series>(ap_or.status());
    avg_precision = pluss::make_series(std::vector<double>{ap_or.value()}, {1}, f64_on(platform));
  }
  // Matthews correlation coefficient from the 2x2 counts; 0 when the denominator vanishes.
  const Series mcc_num = tp.mul(tn).sub(fp.mul(fn));
  const Series mcc_den = tp.add(fp).mul(tp.add(fn)).mul(tn.add(fp)).mul(tn.add(fn)).clamp_min(0.0).sqrt();
  const Series mcc_mask = mcc_den.gt(0.0).to(ScalarType::Float64);
  const Series mcc = mcc_num.div(mcc_den.clamp_min(1.0)).mul(mcc_mask).reshape({1});
  // Unweighted Cohen's kappa; 0 when 1 - p_e vanishes.
  const Series n_total = tp.add(tn).add(fp).add(fn);
  const Series kappa_pe =
      tp.add(fp).mul(tp.add(fn)).add(tn.add(fn).mul(tn.add(fp))).div(n_total.mul(n_total).clamp_min(1.0));
  const Series kappa_den = kappa_pe.mul(-1.0).add(1.0);
  const Series kappa_mask = kappa_den.gt(0.0).to(ScalarType::Float64);
  const Series cohen_kappa = accuracy.reshape({1})
                                 .sub(kappa_pe)
                                 .div(kappa_den.add(kappa_mask.mul(-1.0).add(1.0)))
                                 .mul(kappa_mask)
                                 .reshape({1});

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy.reshape({1}));
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(bal_accuracy.reshape({1}));
        break;
      case MetricKind::Precision:
        out.push_back(precision.reshape({1}));
        break;
      case MetricKind::Recall:
      case MetricKind::Sensitivity:
        out.push_back(recall.reshape({1}));
        break;
      case MetricKind::F1:
        out.push_back(f1.reshape({1}));
        break;
      case MetricKind::Specificity:
        out.push_back(tnr.reshape({1}));
        break;
      case MetricKind::GeometricMean:
        out.push_back(geometric_mean.reshape({1}));
        break;
      case MetricKind::BalanceQuality:
        out.push_back(balance_quality.reshape({1}));
        break;
      case MetricKind::RocAuc:
        out.push_back(roc_auc.reshape({1}));
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss.reshape({1}));
        break;
      case MetricKind::Mcc:
        out.push_back(mcc.reshape({1}));
        break;
      case MetricKind::CohenKappa:
        out.push_back(cohen_kappa.reshape({1}));
        break;
      case MetricKind::Gini:
        out.push_back(gini.reshape({1}));
        break;
      case MetricKind::AveragePrecision:
        out.push_back(avg_precision.reshape({1}));
        break;
      case MetricKind::Brier:
        out.push_back(brier.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported classification metric in assess()."));
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

[[nodiscard]] StatusOr<Series> ovr_auc_binary01(const Series& truth01,
                                                const Series& positive_scores,
                                                const char* where) {
  const Series yb = truth01.to(ScalarType::Float64).reshape({truth01.numel()});
  const Series p = positive_scores.to(ScalarType::Float64).reshape({positive_scores.numel()});
  if (yb.numel() == 0 || yb.numel() != p.numel()) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " requires equal non-empty vectors."));
  }

  // Sort + cumulative-sum curve shared with threshold_curve()
  // (NodeJ/metrics/threshold_curve.cpp) instead of re-deriving it here.
  StatusOr<detail::BinaryScoreCurve> curve_or = detail::binary_score_curve(p, yb);
  if (!curve_or.ok()) return StatusOr<Series>(curve_or.status());
  const detail::BinaryScoreCurve curve = curve_or.move_value();

  const Series has_both = curve.positive_count.gt(0.0).to(ScalarType::Float64)
                              .mul(curve.negative_count.gt(0.0).to(ScalarType::Float64));
  const Series pos_safe = curve.positive_count.clamp_min(1.0);
  const Series neg_safe = curve.negative_count.clamp_min(1.0);
  const Platform platform = p.platform();
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);

  const Series tpr_curve = curve.true_positive_cumulative.div(pos_safe);
  const Series fpr_curve = curve.false_positive_cumulative.div(neg_safe);
  const Series tpr_u = tpr_curve.masked_select(curve.is_threshold_boundary);
  const Series fpr_u = fpr_curve.masked_select(curve.is_threshold_boundary);
  const std::int64_t m = fpr_u.numel();
  const Series tpr_prev = (m > 1)
                              ? pluss::series::zeros({1}, f64_on(platform)).unite(
                                    {tpr_u.index_select(0, pluss::series::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::series::zeros({1}, f64_on(platform));
  const Series fpr_prev = (m > 1)
                              ? pluss::series::zeros({1}, f64_on(platform)).unite(
                                    {fpr_u.index_select(0, pluss::series::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::series::zeros({1}, f64_on(platform));
  const Series auc_raw = fpr_u.sub(fpr_prev).mul(tpr_u.add(tpr_prev)).mul(0.5).sum().clamp(0.0, 1.0).reshape({1});
  const Series half = pluss::series::ones({1}, f64_on(platform)).mul(0.5);
  const Series auc = auc_raw.mul(has_both).add(half.mul(pluss::series::ones({1}, f64_on(platform)).sub(has_both)));
  return StatusOr<Series>(auc.reshape({1}));
}

// One-vs-rest average precision for a binary 0/1 truth vector and its
// positive-class scores -- delegates to the shared nodej::average_precision
// (NodeJ/metrics/threshold_curve.cpp), wrapping the scalar into a [1] Series to
// match ovr_auc_binary01's shape.
[[nodiscard]] StatusOr<Series> ovr_average_precision_binary01(const Series& truth01,
                                                             const Series& positive_scores,
                                                             const char* where) {
  const Series yb = truth01.to(ScalarType::Float64).reshape({truth01.numel()});
  const Series p = positive_scores.to(ScalarType::Float64).reshape({positive_scores.numel()});
  if (yb.numel() == 0 || yb.numel() != p.numel()) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " requires equal non-empty vectors."));
  }
  StatusOr<double> ap_or = average_precision(p, yb);
  if (!ap_or.ok()) return StatusOr<Series>(ap_or.status());
  return StatusOr<Series>(
      pluss::make_series(std::vector<double>{ap_or.value()}, {1}, f64_on(p.platform())));
}

[[nodiscard]] StatusOr<Series> classification_metrics_multiclass(const Series& truth_labels,
                                                                 const Series& pred_labels,
                                                                 const Series& score_matrix,
                                                                 const Series& classes,
                                                                 const std::vector<MetricKind>& requested) {
  const Series y = truth_labels.to(ScalarType::Float64).reshape({truth_labels.numel()});
  const Series yh = pred_labels.to(ScalarType::Float64).reshape({pred_labels.numel()});
  const Series cls = classes.to(ScalarType::Float64).reshape({classes.numel()});
  const std::int64_t n = y.numel();
  const std::int64_t k = cls.numel();
  const Platform platform = y.platform();
  if (n == 0 || yh.numel() != n || k < 2) {
    return StatusOr<Series>(Status::shape_mismatch("multiclass assess requires non-empty equal-size label vectors."));
  }

  Series logits = score_matrix.to(platform, ScalarType::Float64);
  if (logits.dim() != 2 || logits.sizes()[0] != n || logits.sizes()[1] != k) {
    return StatusOr<Series>(Status::shape_mismatch("multiclass assess expects score matrix shape [samples, classes]."));
  }

  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const Series k_scalar = pluss::series::ones({1}, f64_on(platform)).mul(static_cast<double>(k));
  Series precision_sum = pluss::series::zeros({1}, f64_on(platform));
  Series recall_sum = pluss::series::zeros({1}, f64_on(platform));
  Series f1_sum = pluss::series::zeros({1}, f64_on(platform));
  Series specificity_sum = pluss::series::zeros({1}, f64_on(platform));
  Series geometric_sum = pluss::series::zeros({1}, f64_on(platform));
  Series balance_quality_sum = pluss::series::zeros({1}, f64_on(platform));
  // Marginal-sum accumulators for MCC / Cohen's kappa: c (total correct),
  // Sum p_k*t_k, Sum p_k^2, Sum t_k^2, where p_k = predicted count for class k,
  // t_k = true count for class k.
  Series correct_sum = pluss::series::zeros({1}, f64_on(platform));
  Series pk_tk_sum = pluss::series::zeros({1}, f64_on(platform));
  Series pk_pk_sum = pluss::series::zeros({1}, f64_on(platform));
  Series tk_tk_sum = pluss::series::zeros({1}, f64_on(platform));
  const Series n_scalar = pluss::series::ones({1}, f64_on(platform)).mul(static_cast<double>(n));

  for (std::int64_t ci = 0; ci < k; ++ci) {
    const Series idx = pluss::series::arange(ci, ci + 1, i64_cfg);
    const Series cls_value = cls.index_select(0, idx).reshape({1});
    const Series y_c = y.eq(cls_value).to(ScalarType::Float64);
    const Series yh_c = yh.eq(cls_value).to(ScalarType::Float64);
    const Series tp = y_c.mul(yh_c).sum();
    const Series fp = y_c.mul(-1.0).add(1.0).mul(yh_c).sum();
    const Series fn = y_c.mul(yh_c.mul(-1.0).add(1.0)).sum();
    const Series tn = n_scalar.sub(tp.add(fp).add(fn));
    const Series precision_den = tp.add(fp);
    const Series recall_den = tp.add(fn);
    const Series specificity_den = tn.add(fp);
    const Series precision_mask = precision_den.gt(0.0).to(ScalarType::Float64);
    const Series recall_mask = recall_den.gt(0.0).to(ScalarType::Float64);
    const Series specificity_mask = specificity_den.gt(0.0).to(ScalarType::Float64);
    const Series precision = tp.div(precision_den.clamp_min(1.0)).mul(precision_mask);
    const Series recall = tp.div(recall_den.clamp_min(1.0)).mul(recall_mask);
    const Series specificity = tn.div(specificity_den.clamp_min(1.0)).mul(specificity_mask);
    const Series f1_den = precision.add(recall);
    const Series f1_mask = f1_den.gt(0.0).to(ScalarType::Float64);
    const Series f1 = precision.mul(recall).mul(2.0).div(f1_den.clamp_min(1.0)).mul(f1_mask);
    const Series geometric = recall.mul(specificity).clamp_min(0.0).sqrt();
    const Series balance_quality = geometric.square().mul(recall.sub(specificity).mul(0.1).add(1.0)).clamp_min(0.0);
    precision_sum = precision_sum.add(precision.reshape({1}));
    recall_sum = recall_sum.add(recall.reshape({1}));
    f1_sum = f1_sum.add(f1.reshape({1}));
    specificity_sum = specificity_sum.add(specificity.reshape({1}));
    geometric_sum = geometric_sum.add(geometric.reshape({1}));
    balance_quality_sum = balance_quality_sum.add(balance_quality.reshape({1}));
    const Series pk = tp.add(fp).reshape({1});  // predicted count for this class
    const Series tk = tp.add(fn).reshape({1});  // true count for this class
    correct_sum = correct_sum.add(tp.reshape({1}));
    pk_tk_sum = pk_tk_sum.add(pk.mul(tk));
    pk_pk_sum = pk_pk_sum.add(pk.mul(pk));
    tk_tk_sum = tk_tk_sum.add(tk.mul(tk));
  }

  const Series accuracy = y.eq(yh).to(ScalarType::Float64).mean().reshape({1});
  const Series macro_precision = precision_sum.div(k_scalar).reshape({1});
  const Series macro_recall = recall_sum.div(k_scalar).reshape({1});
  const Series macro_f1 = f1_sum.div(k_scalar).reshape({1});
  const Series macro_specificity = specificity_sum.div(k_scalar).reshape({1});
  const Series macro_geometric = geometric_sum.div(k_scalar).reshape({1});
  const Series macro_balance_quality = balance_quality_sum.div(k_scalar).reshape({1});
  const Series balanced_accuracy = macro_recall.reshape({1});

  constexpr double eps = 1e-12;
  const Series probs = logits.softmax(1);
  Series one_hot = pluss::series::undefined();
  for (std::int64_t ci = 0; ci < k; ++ci) {
    const Series idx = pluss::series::arange(ci, ci + 1, i64_cfg);
    const Series cls_value = cls.index_select(0, idx).reshape({1});
    const Series col = y.eq(cls_value).to(ScalarType::Float64).reshape({n, 1});
    one_hot = one_hot.defined() ? one_hot.unite({col}, 1) : col;
  }
  const Series logloss = probs.mul(one_hot).sum(1, false).clamp(eps, 1.0).log().mul(-1.0).mean().reshape({1});

  // Multiclass Brier: mean over samples of Sum_k (p_ik - y_ik)^2.
  const Series brier = probs.sub(one_hot).square().sum(1, false).mean().reshape({1});

  // MCC and unweighted Cohen's kappa from the class marginal sums accumulated
  // above. c = correct_sum, s = n, p.t = pk_tk_sum, p.p = pk_pk_sum,
  // t.t = tk_tk_sum.
  const Series s_sq = n_scalar.mul(n_scalar).reshape({1});
  const Series mcc_num = correct_sum.mul(n_scalar).sub(pk_tk_sum).reshape({1});
  const Series mcc_den = s_sq.sub(pk_pk_sum).mul(s_sq.sub(tk_tk_sum)).clamp_min(0.0).sqrt().reshape({1});
  const Series mcc_mask = mcc_den.gt(0.0).to(ScalarType::Float64);
  const Series mcc = mcc_num.div(mcc_den.add(mcc_mask.mul(-1.0).add(1.0))).mul(mcc_mask).reshape({1});
  const Series kappa_pe = pk_tk_sum.div(s_sq.clamp_min(1.0)).reshape({1});
  const Series kappa_den = kappa_pe.mul(-1.0).add(1.0);
  const Series kappa_mask = kappa_den.gt(0.0).to(ScalarType::Float64);
  const Series cohen_kappa = accuracy.sub(kappa_pe)
                                 .div(kappa_den.add(kappa_mask.mul(-1.0).add(1.0)))
                                 .mul(kappa_mask)
                                 .reshape({1});

  bool needs_ovr_auc = false;
  bool needs_ovr_ap = false;
  for (MetricKind metric : requested) {
    if (metric == MetricKind::RocAuc || metric == MetricKind::Gini) needs_ovr_auc = true;
    if (metric == MetricKind::AveragePrecision) needs_ovr_ap = true;
  }
  Series macro_roc_auc = pluss::series::undefined();
  Series macro_average_precision = pluss::series::undefined();
  if (needs_ovr_auc || needs_ovr_ap) {
    Series auc_sum = pluss::series::zeros({1}, f64_on(platform));
    Series ap_sum = pluss::series::zeros({1}, f64_on(platform));
    for (std::int64_t ci = 0; ci < k; ++ci) {
      const Series idx = pluss::series::arange(ci, ci + 1, i64_cfg);
      const Series cls_value = cls.index_select(0, idx).reshape({1});
      const Series y01 = y.eq(cls_value).to(ScalarType::Float64).reshape({n});
      const Series score_col = probs.index_select(1, idx).reshape({n});
      if (needs_ovr_auc) {
        StatusOr<Series> auc_or = ovr_auc_binary01(y01, score_col, "classification_metrics_multiclass");
        if (!auc_or.ok()) return StatusOr<Series>(auc_or.status());
        auc_sum = auc_sum.add(auc_or.value().reshape({1}));
      }
      if (needs_ovr_ap) {
        StatusOr<Series> ap_or =
            ovr_average_precision_binary01(y01, score_col, "classification_metrics_multiclass");
        if (!ap_or.ok()) return StatusOr<Series>(ap_or.status());
        ap_sum = ap_sum.add(ap_or.value().reshape({1}));
      }
    }
    if (needs_ovr_auc) macro_roc_auc = auc_sum.div(k_scalar).reshape({1});
    if (needs_ovr_ap) macro_average_precision = ap_sum.div(k_scalar).reshape({1});
  }
  const Series gini =
      needs_ovr_auc ? macro_roc_auc.mul(2.0).sub(1.0).reshape({1}) : pluss::series::undefined();

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy);
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(balanced_accuracy);
        break;
      case MetricKind::Precision:
        out.push_back(macro_precision);
        break;
      case MetricKind::Recall:
      case MetricKind::Sensitivity:
        out.push_back(macro_recall);
        break;
      case MetricKind::F1:
        out.push_back(macro_f1);
        break;
      case MetricKind::Specificity:
        out.push_back(macro_specificity);
        break;
      case MetricKind::GeometricMean:
        out.push_back(macro_geometric);
        break;
      case MetricKind::BalanceQuality:
        out.push_back(macro_balance_quality);
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss);
        break;
      case MetricKind::RocAuc:
        out.push_back(macro_roc_auc);
        break;
      case MetricKind::Mcc:
        out.push_back(mcc);
        break;
      case MetricKind::CohenKappa:
        out.push_back(cohen_kappa);
        break;
      case MetricKind::Gini:
        out.push_back(gini);
        break;
      case MetricKind::AveragePrecision:
        out.push_back(macro_average_precision);
        break;
      case MetricKind::Brier:
        out.push_back(brier);
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported classification metric in multiclass assess()."));
    }
  }

  if (out.empty()) {
    return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(platform)));
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(platform, ScalarType::Float64));
}

class LinearClassifier final : public ClassifierBase {
public:
  explicit LinearClassifier(LinearFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");
    if ((options.calibration == ScoreCalibration::Platt || options.calibration == ScoreCalibration::Temperature)
        && (options.calibration_iterations <= 0 || !(options.calibration_tolerance > 0.0))) {
      return Status::invalid_argument(
          "LinearClassifier::learn with selected calibration requires positive calibration iterations and tolerance.");
    }

    Status option_status = validate_linear_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "LinearClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "LinearClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "LinearClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "LinearClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();
    StatusOr<Series> sample_weight_or = resolve_training_weights(labels.y01, fit_batch.sample_weights, fit_options);
    if (!sample_weight_or.ok()) return sample_weight_or.status();
    const Series sample_weight = sample_weight_or.move_value();

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const std::int64_t param_count = design.sizes()[1];
    Series weights = pluss::series::zeros({param_count}, f64_on(design.platform()));
    const LinearPenalty penalty = resolve_linear_penalty(options_);

    optim::AdamState adam = optim::adam_state_like(std::vector<Series>{weights});
    optim::AdamOptions adam_opt{};
    adam_opt.learning_rate = 0.05;
    adam_opt.weight_decay = 0.0;

    const bool use_parameter_bounds = options.parameter_lower_bounds.defined() || options.parameter_upper_bounds.defined();
    optim::SeptimaConstraints septima_constraints{};
    optim::SeptimaOptions septima_options{};
    optim::SeptimaSolveOptions septima_solve_options{};

    if (use_parameter_bounds && penalty.l1 > 0.0) {
      return Status::not_implemented(
          "LinearClassifier::learn currently does not support L1/Elastic regularization when parameter bounds are enabled.");
    }

    if (use_parameter_bounds) {
      StatusOr<Series> lb_or =
          resolve_parameter_bounds(options.parameter_lower_bounds, param_count, design.platform(), "LinearClassifier::learn lower");
      if (!lb_or.ok()) return lb_or.status();
      StatusOr<Series> ub_or =
          resolve_parameter_bounds(options.parameter_upper_bounds, param_count, design.platform(), "LinearClassifier::learn upper");
      if (!ub_or.ok()) return ub_or.status();

      Series lower = lb_or.move_value();
      Series upper = ub_or.move_value();
      if (lower.defined() && upper.defined() && !lower.le(upper).all().item<bool>()) {
        return Status::invalid_argument("LinearClassifier::learn lower bounds must be <= upper bounds.");
      }

      if (lower.defined()) septima_constraints.lbs = {lower};
      if (upper.defined()) septima_constraints.ubs = {upper};
      septima_options.adam = adam_opt;
      septima_options.project_bounds = true;
      septima_options.enable_constraint_autodiff = false;
      septima_solve_options.step = septima_options;
      septima_solve_options.max_iterations = 1;
      septima_solve_options.max_no_progress_iterations = 0;
      septima_solve_options.objective_tolerance = 0.0;
      septima_solve_options.gradient_tolerance = 0.0;
      septima_solve_options.step_tolerance = 0.0;
      septima_solve_options.constraint_tolerance = 0.0;
    }

    if (options_.objective == LinearObjective::Logistic && !use_parameter_bounds && penalty.l1 <= 0.0) {
      StatusOr<Series> fast_weights_or =
          solve_logistic_newton(design,
                                labels.y01,
                                sample_weight,
                                options_.fit_intercept,
                                penalty.l2,
                                options.max_iterations,
                                options.tolerance,
                                "LinearClassifier::learn");
      if (!fast_weights_or.ok()) return fast_weights_or.status();
      weights = fast_weights_or.move_value();
    } else {
      double prev_loss = std::numeric_limits<double>::infinity();
      for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
        Series grad = pluss::series::undefined();
        if (options_.objective == LinearObjective::Logistic) {
          grad = logistic_gradient(design, labels.y01, sample_weight, weights, options_.fit_intercept, penalty.l2);
        } else {
          grad = hinge_gradient(design, labels.y01, sample_weight, weights, options_.fit_intercept, penalty.l2);
        }
        if (!all_finite(grad)) {
          return Status::numerical_failure("LinearClassifier::learn encountered non-finite gradient.");
        }

        try {
          if (use_parameter_bounds) {
            const auto fit = optim::septima(
                [&](const optim::Trace& w) -> optim::Trace {
                  return optim::sum(w * optim::fixed(grad));
                },
                weights,
                septima_solve_options,
                septima_constraints);
            weights = fit.params.front();
          } else {
            std::vector<Series> params{weights};
            optim::adam_step_inplace(params, std::vector<Series>{grad}, adam, adam_opt);
            weights = params[0];
          }
        } catch (const std::exception& ex) {
          return Status::numerical_failure(std::string("LinearClassifier::learn optimizer failure: ") + ex.what());
        }
        if (penalty.l1 > 0.0) {
          weights = apply_l1_prox(weights, penalty.l1 * adam_opt.learning_rate, options_.fit_intercept);
        }

        const double loss = options_.objective == LinearObjective::Logistic
                                ? logistic_objective(design,
                                                     labels.y01,
                                                     sample_weight,
                                                     weights,
                                                     options_.fit_intercept,
                                                     penalty.l2,
                                                     penalty.l1)
                                : hinge_objective(design,
                                                  labels.y01,
                                                  sample_weight,
                                                  weights,
                                                  options_.fit_intercept,
                                                  penalty.l2,
                                                  penalty.l1);
        if (!std::isfinite(loss)) {
          return Status::numerical_failure("LinearClassifier::learn encountered non-finite objective.");
        }
        if (std::abs(prev_loss - loss) <= options.tolerance) {
          break;
        }
        prev_loss = loss;
      }
    }

    if (!all_finite(weights)) {
      return Status::numerical_failure("LinearClassifier::learn produced non-finite coefficients.");
    }

    calibrated_scores_ = false;
    calibration_kind_ = CalibrationKind::None;
    calibration_a_ = 1.0;
    calibration_b_ = 0.0;
    calibration_temperature_ = 1.0;
    isotonic_x_.clear();
    isotonic_y_.clear();
    if (options.calibration == ScoreCalibration::Platt) {
      const Series train_scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({matrix.sample_count});
      StatusOr<Series> params_or = fit_platt_calibrator(train_scores,
                                                        labels.y01,
                                                        sample_weight,
                                                        options.calibration_iterations,
                                                        options.calibration_tolerance,
                                                        "LinearClassifier::learn");
      if (!params_or.ok()) return params_or.status();
      const Series params = params_or.move_value();
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, params.platform());
      const Series idx1 = make_index_series(std::vector<std::int64_t>{1}, params.platform());
      calibration_a_ = params.index_select(0, idx0).reshape({1}).item<double>();
      calibration_b_ = params.index_select(0, idx1).reshape({1}).item<double>();
      calibrated_scores_ = true;
      calibration_kind_ = CalibrationKind::Platt;
    } else if (options.calibration == ScoreCalibration::Isotonic) {
      const Series train_scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({matrix.sample_count});
      StatusOr<IsotonicCalibratorState> iso_or =
          fit_isotonic_calibrator(train_scores, labels.y01, sample_weight, "LinearClassifier::learn");
      if (!iso_or.ok()) return iso_or.status();
      isotonic_x_ = std::move(iso_or.value().x_thresholds);
      isotonic_y_ = std::move(iso_or.value().y_thresholds);
      calibrated_scores_ = true;
      calibration_kind_ = CalibrationKind::Isotonic;
    } else if (options.calibration == ScoreCalibration::Temperature) {
      const Series train_scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({matrix.sample_count});
      StatusOr<double> temp_or = fit_temperature_calibrator(train_scores,
                                                            labels.y01,
                                                            sample_weight,
                                                            options.calibration_iterations,
                                                            options.calibration_tolerance,
                                                            "LinearClassifier::learn");
      if (!temp_or.ok()) return temp_or.status();
      calibration_temperature_ = temp_or.value();
      calibrated_scores_ = true;
      calibration_kind_ = CalibrationKind::Temperature;
    }

    weights_ = std::move(weights);
    feature_count_ = matrix.feature_count;
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "LinearClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "LinearClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const Series scores = design.matmul(weights_.reshape({weights_.numel(), 1})).reshape({matrix.sample_count});
    if (!all_finite(scores)) {
      return StatusOr<Series>(Status::numerical_failure("LinearClassifier::infer_scores produced non-finite scores."));
    }
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "LinearClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    StatusOr<Series> probs_or = probabilities_from_scores(scores_or.value().reshape({scores_or.value().numel()}));
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    const Series probs = probs_or.move_value();

    const Series out =
        labels_from_probabilities(probs, negative_label_, positive_label_, options.positive_threshold, batch.x.platform(), label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "LinearClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("LinearClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    StatusOr<Series> probs_or = probabilities_from_scores(scores_or.value().reshape({scores_or.value().numel()}));
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    const Series probs = probs_or.move_value();

    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "LinearClassifier::assess");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    const Series pred01 = probs.ge(options.positive_threshold).to(ScalarType::Float64).reshape({probs.numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs, metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   calibrated_scores_ ? 1.0 : 0.0,
                   static_cast<double>(static_cast<std::int64_t>(calibration_kind_)),
                   calibration_a_,
                   calibration_b_,
                   calibration_temperature_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        weights_,
    };
    if (!isotonic_x_.empty() && isotonic_x_.size() == isotonic_y_.size()) {
      out.push_back(pluss::make_series(isotonic_x_, {static_cast<std::int64_t>(isotonic_x_.size())}, f64_cpu()));
      out.push_back(pluss::make_series(isotonic_y_, {static_cast<std::int64_t>(isotonic_y_.size())}, f64_cpu()));
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("LinearClassifier::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("LinearClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    calibrated_scores_ = meta[2] > 0.5;
    if (meta.size() >= 10) {
      calibration_kind_ = static_cast<CalibrationKind>(static_cast<std::int64_t>(meta[3]));
      calibration_a_ = meta[4];
      calibration_b_ = meta[5];
      calibration_temperature_ = meta[6];
      negative_label_ = meta[7];
      positive_label_ = meta[8];
      label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[9]));
    } else {
      // Backward compatibility with previous [trained, features, calibrated, a, b, neg, pos, dtype] metadata.
      calibration_kind_ = calibrated_scores_ ? CalibrationKind::Platt : CalibrationKind::None;
      calibration_a_ = meta[3];
      calibration_b_ = meta[4];
      calibration_temperature_ = 1.0;
      negative_label_ = meta[5];
      positive_label_ = meta[6];
      label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
    }
    weights_ = state[1];
    isotonic_x_.clear();
    isotonic_y_.clear();
    if (state.size() >= 4 && calibration_kind_ == CalibrationKind::Isotonic) {
      isotonic_x_ = vec_f64(state[2]);
      isotonic_y_ = vec_f64(state[3]);
      if (isotonic_x_.empty() || isotonic_x_.size() != isotonic_y_.size()) {
        return Status::invalid_argument("LinearClassifier::load_state invalid isotonic calibration state.");
      }
    }
    if (calibration_kind_ == CalibrationKind::Temperature) {
      calibration_temperature_ = std::clamp(calibration_temperature_, 1e-6, 1e6);
    }
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

  [[nodiscard]] StatusOr<Series> probabilities_from_scores(const Series& scores) const {
    if (!calibrated_scores_ || calibration_kind_ == CalibrationKind::None) {
      return StatusOr<Series>(scores.sigmoid());
    }
    if (calibration_kind_ == CalibrationKind::Platt) {
      return StatusOr<Series>(scores.mul(calibration_a_).add(calibration_b_).sigmoid());
    }
    if (calibration_kind_ == CalibrationKind::Temperature) {
      const double t = std::clamp(calibration_temperature_, 1e-6, 1e6);
      return StatusOr<Series>(scores.div(t).sigmoid());
    }
    return apply_isotonic_calibrator(scores, isotonic_x_, isotonic_y_, "LinearClassifier::probabilities_from_scores");
  }

  enum class CalibrationKind : std::int64_t {
    None = 0,
    Platt = 1,
    Isotonic = 2,
    Temperature = 3,
  };

  LinearFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series weights_ = pluss::series::undefined();
  bool calibrated_scores_{false};
  CalibrationKind calibration_kind_{CalibrationKind::None};
  double calibration_a_{1.0};
  double calibration_b_{0.0};
  double calibration_temperature_{1.0};
  std::vector<double> isotonic_x_{};
  std::vector<double> isotonic_y_{};
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class FamilyClassifier final : public ClassifierBase {
public:
  explicit FamilyClassifier(ModelFamily family) : family_(family) {}

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
        Status::not_implemented(std::string(family_name(family_)) + " classifier infer() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch);
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));
    return StatusOr<Series>(Status::not_implemented(std::string(family_name(family_))
                                                    + " classifier infer_scores() is scheduled after Phase 0."));
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
        Status::not_implemented(std::string(family_name(family_)) + " classifier assess() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(features_)}),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("FamilyClassifier::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 2) return Status::invalid_argument("FamilyClassifier::load_state invalid metadata.");
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

[[nodiscard]] StatusOr<std::unique_ptr<ClassifierBase>> make_binary_classifier_impl(const ModelSpec& spec) {
  if (spec.family == ModelFamily::Linear) {
    const auto* options = std::get_if<LinearFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Linear classifier requires LinearFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<LinearClassifier>(*options));
  }
  if (spec.family == ModelFamily::Tree) {
    const auto* options = std::get_if<TreeFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Tree classifier requires TreeFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<TreeClassifier>(*options));
  }
  if (spec.family == ModelFamily::Ensemble) {
    const auto* options = std::get_if<EnsembleFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Ensemble classifier requires EnsembleFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<EnsembleClassifier>(*options));
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<NeighborClassifier>(*options));
  }
  if (spec.family == ModelFamily::Bayes) {
    const auto* options = std::get_if<BayesFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Bayes classifier requires BayesFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<BayesClassifier>(*options));
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Kernel classifier requires KernelFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<KernelClassifier>(*options));
  }
  return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<FamilyClassifier>(spec.family));
}

class OvRClassifier final : public ClassifierBase {
public:
  explicit OvRClassifier(ModelSpec spec) : spec_(std::move(spec)) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    StatusOr<LabeledBatch> fit_batch_or = resolve_classification_training_batch(batch, options, "OvRClassifier::learn");
    if (!fit_batch_or.ok()) return fit_batch_or.status();
    const LabeledBatch& fit_batch = fit_batch_or.value();
    const TrainOptions fit_options = without_training_balance(options);

    StatusOr<Series> classes_or = extract_sorted_classes(fit_batch, fit_batch.features.sample_count(), "OvRClassifier::learn");
    if (!classes_or.ok()) return classes_or.status();
    const Series classes = classes_or.move_value();
    const std::int64_t class_count = classes.numel();
    const std::int64_t sample_count = fit_batch.features.sample_count();
    const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(classes.platform());

    std::vector<std::unique_ptr<ClassifierBase>> models{};
    models.reserve(static_cast<std::size_t>(class_count));

    if (class_count == 2) {
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
      if (!model_or.ok()) return model_or.status();
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      Status learn_status = model->learn(fit_batch, fit_options);
      if (!learn_status.ok()) return learn_status;
      models.push_back(std::move(model));
      multiclass_ = false;
    } else {
      const Series y64 = fit_batch.y.to(ScalarType::Float64).reshape({sample_count});
      for (std::int64_t ci = 0; ci < class_count; ++ci) {
        const Series idx = pluss::series::arange(ci, ci + 1, i64_cfg);
        const Series cls_value = classes.index_select(0, idx).reshape({1});
        const Series y01 = y64.eq(cls_value).to(ScalarType::Float64);
        const LabeledBatch binary_batch{
            .features = fit_batch.features,
            .y = y01,
            .sample_weights = fit_batch.sample_weights,
        };
        StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
        if (!model_or.ok()) return model_or.status();
        std::unique_ptr<ClassifierBase> model = model_or.move_value();
        Status learn_status = model->learn(binary_batch, fit_options);
        if (!learn_status.ok()) return learn_status;
        models.push_back(std::move(model));
      }
      multiclass_ = true;
    }

    models_ = std::move(models);
    classes_ = classes.to(ScalarType::Float64).reshape({class_count});
    label_dtype_ = batch.y.scalar_type();
    feature_count_ = batch.features.feature_count();
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->infer_scores(batch, options);
    }

    const std::int64_t sample_count = batch.sample_count();
    Series scores = pluss::series::undefined();
    for (const std::unique_ptr<ClassifierBase>& model : models_) {
      StatusOr<Series> score_or = model->infer_scores(batch, options);
      if (!score_or.ok()) return StatusOr<Series>(score_or.status());
      const Series col = score_or.value().to(batch.x.platform(), ScalarType::Float64).reshape({sample_count, 1});
      scores = scores.defined() ? scores.unite({col}, 1) : col;
    }
    if (!scores.defined()) {
      return StatusOr<Series>(Status::internal("OvRClassifier::infer_scores has no trained learners."));
    }
    return StatusOr<Series>(scores.reshape({sample_count, static_cast<std::int64_t>(models_.size())}));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->infer(batch, options);
    }

    StatusOr<Series> score_or = infer_scores(batch, options);
    if (!score_or.ok()) return StatusOr<Series>(score_or.status());
    const Series scores = score_or.value().reshape({batch.sample_count(), classes_.numel()});
    const Series idx = scores.argmax(1, false).reshape({batch.sample_count()}).to(ScalarType::Int64);
    const Series labels = classes_.to(scores.platform(), ScalarType::Float64).index_select(0, idx);
    return StatusOr<Series>(labels.to(batch.x.platform(), label_dtype_).reshape({batch.sample_count()}));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->assess(batch, options);
    }

    StatusOr<Series> classes_or = extract_sorted_classes(batch, batch.features.sample_count(), "OvRClassifier::assess");
    if (!classes_or.ok()) return StatusOr<Series>(classes_or.status());
    if (!same_sorted_classes(classes_or.value().to(ScalarType::Float64), classes_)) {
      return StatusOr<Series>(Status::invalid_argument(
          "OvRClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) metrics = {MetricKind::Accuracy};
    return classification_metrics_multiclass(batch.y, pred_or.value(), scores_or.value(), classes_, metrics);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + models_.size() * 2);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             multiclass_ ? 1.0 : 0.0,
                             static_cast<double>(feature_count_),
                             static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                             static_cast<double>(models_.size())}));
    out.push_back(classes_);
    for (const auto& model : models_) {
      if (!model) {
        out.push_back(state_i64({0}));
        continue;
      }
      StatusOr<std::vector<Series>> child_or = model->dump_state();
      if (!child_or.ok()) return StatusOr<std::vector<Series>>(child_or.status());
      const auto& child = child_or.value();
      out.push_back(state_i64({static_cast<std::int64_t>(child.size())}));
      out.insert(out.end(), child.begin(), child.end());
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("OvRClassifier::load_state expects metadata + classes tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 5) return Status::invalid_argument("OvRClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    multiclass_ = meta[1] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[3]));
    const std::int64_t model_count = static_cast<std::int64_t>(meta[4]);
    classes_ = state[1];

    models_.clear();
    models_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, model_count)));
    std::size_t cursor = 2;
    for (std::int64_t mi = 0; mi < model_count; ++mi) {
      if (cursor >= state.size()) return Status::invalid_argument("OvRClassifier::load_state truncated child state.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t child_items = size_vec.empty() ? 0 : size_vec.front();
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
      if (!model_or.ok()) return model_or.status();
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      if (child_items > 0) {
        std::vector<Series> child{};
        child.reserve(static_cast<std::size_t>(child_items));
        for (std::int64_t si = 0; si < child_items; ++si) {
          if (cursor >= state.size()) return Status::invalid_argument("OvRClassifier::load_state truncated child tensors.");
          child.push_back(state[cursor++]);
        }
        Status load_status = model->load_state(child);
        if (!load_status.ok()) return load_status;
      }
      models_.push_back(std::move(model));
    }
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
    if (models_.empty()) {
      return Status::precondition("Estimator must hold at least one trained learner.");
    }
    return Status::success();
  }

  ModelSpec spec_{};
  bool trained_{false};
  bool multiclass_{false};
  std::int64_t feature_count_{0};
  ScalarType label_dtype_{ScalarType::Float64};
  Series classes_ = pluss::series::undefined();
  std::vector<std::unique_ptr<ClassifierBase>> models_{};
};

} // namespace

StatusOr<Series> score_metrics(const Series& scores,
                               const Series& labels,
                               const std::vector<MetricKind>& requested) {
  for (MetricKind metric : requested) {
    if (metric == MetricKind::Mse || metric == MetricKind::Rmse || metric == MetricKind::Mae ||
        metric == MetricKind::R2 || metric == MetricKind::QuantileLoss) {
      return StatusOr<Series>(Status::invalid_argument(
          "score_metrics: regression metric requested on a classification score/label pair."));
    }
  }

  const Platform platform = scores.platform();
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const bool multiclass = scores.dim() == 2 && scores.sizes()[1] >= 2;

  if (!multiclass) {
    const std::int64_t n = labels.numel();
    const Series p = scores.to(platform, ScalarType::Float64).reshape({n});
    const Series y01 = labels.to(platform, ScalarType::Float64).reshape({n});
    // Hard-label metrics (F1, Mcc, ...) score a 0.5-threshold prediction -- the
    // same cutoff the model-backed reports path applies by default.
    const Series pred01 = p.ge(0.5).to(ScalarType::Float64);
    return classification_metrics(y01, pred01, p, requested);
  }

  const std::int64_t n = scores.sizes()[0];
  const std::int64_t k = scores.sizes()[1];
  const Series probs = scores.to(platform, ScalarType::Float64).reshape({n, k});
  const Series y = labels.to(platform, ScalarType::Float64).reshape({n});
  const Series pred = probs.argmax(1, false).to(platform, ScalarType::Float64).reshape({n});
  const Series classes = pluss::series::arange(0, k, i64_cfg).to(platform, ScalarType::Float64);
  // classification_metrics_multiclass runs softmax over its score matrix; hand
  // it log-probabilities so softmax(log(p)) == p and Brier / LogLoss see the
  // caller's actual probabilities rather than a re-normalized copy.
  const Series log_probs = probs.clamp_min(1e-12).log();
  return classification_metrics_multiclass(y, pred, log_probs, classes, requested);
}

StatusOr<std::unique_ptr<ClassifierBase>> make_classifier(const ModelSpec& spec) {
  if (spec.task != TaskKind::Classification) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(
        Status::invalid_argument("make_classifier requires ModelSpec.task = TaskKind::Classification."));
  }
  if (!is_phase0_family(spec.family)) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(Status::invalid_argument("Unsupported classifier family."));
  }
  StatusOr<std::unique_ptr<ClassifierBase>> check_or = make_binary_classifier_impl(spec);
  if (!check_or.ok()) return StatusOr<std::unique_ptr<ClassifierBase>>(check_or.status());
  return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<OvRClassifier>(spec));
}

} // namespace pluss::ai::nodej
