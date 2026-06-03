#include "pluss/ai/nodej/builders.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Config i64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] Series state_f64(std::vector<double> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values), {n}, f64_on(pluss::pCPU));
}

[[nodiscard]] std::vector<double> vec_f64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

[[nodiscard]] bool all_finite(const Series& x) {
  return x.isfinite().all().item<bool>();
}

[[nodiscard]] bool has_nan(const Series& x) {
  return x.isnan().any().item<bool>();
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::uint64_t compose_seed(const RandomState& random) {
  if (random.deterministic) return random.seed;
  std::random_device rd;
  return splitmix64((static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd()) ^ random.seed);
}

struct DenseFeatureMatrix {
  Series x_rows = Series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  ObservationLayout layout = ObservationLayout::Rows;
  Platform original_platform = pluss::pCPU;
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
  out.layout = batch.layout;
  out.original_platform = batch.x.platform();
  out.x_rows = batch.layout == ObservationLayout::Rows
                   ? batch.x.to(ScalarType::Float64)
                   : batch.x.to(ScalarType::Float64).transpose(0, 1);
  return StatusOr<DenseFeatureMatrix>(std::move(out));
}

[[nodiscard]] Series pairwise_distance2(const Series& query_rows, const Series& train_rows) {
  const Series dot = query_rows.matmul(train_rows.transpose(0, 1));
  const Series q_norm = query_rows.square().sum(1, true);
  const Series t_norm = train_rows.square().sum(1, true).transpose(0, 1);
  return q_norm.add(t_norm).sub(dot.mul(2.0)).relu();
}

[[nodiscard]] Status validate_cluster_options(const ClusterFamilyOptions& options) {
  if (options.method != ClusterMethod::KMeans) {
    return Status::not_implemented("Transformer cluster family currently supports KMeans only.");
  }
  if (options.clusters <= 0) {
    return Status::invalid_argument("ClusterFamilyOptions.clusters must be positive.");
  }
  if (options.max_iterations <= 0) {
    return Status::invalid_argument("ClusterFamilyOptions.max_iterations must be positive.");
  }
  if (options.restarts <= 0) {
    return Status::invalid_argument("ClusterFamilyOptions.restarts must be positive.");
  }
  if (!(options.tolerance > 0.0)) {
    return Status::invalid_argument("ClusterFamilyOptions.tolerance must be positive.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_decompose_options(const DecomposeFamilyOptions& options) {
  if (options.method != DecomposeMethod::Pca) {
    return Status::not_implemented("Transformer decompose family currently supports PCA only.");
  }
  if (options.components == 0) {
    return Status::invalid_argument("DecomposeFamilyOptions.components must be -1 or positive.");
  }
  if (options.max_iterations <= 0) {
    return Status::invalid_argument("DecomposeFamilyOptions.max_iterations must be positive.");
  }
  if (!(options.tolerance > 0.0)) {
    return Status::invalid_argument("DecomposeFamilyOptions.tolerance must be positive.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_kernel_transform_options(const KernelFamilyOptions& options) {
  if (options.form != KernelForm::OneClass) {
    return Status::invalid_argument("Kernel transformer requires KernelFamilyOptions.form = KernelForm::OneClass.");
  }
  if (!std::isfinite(options.nu) || options.nu <= 0.0 || options.nu > 1.0) {
    return Status::invalid_argument("KernelFamilyOptions.nu must be finite and in (0, 1].");
  }
  if (!std::isfinite(options.gamma) || options.gamma < 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.gamma must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] double resolved_gamma(double gamma, std::int64_t feature_count) {
  if (gamma > 0.0 && std::isfinite(gamma)) return gamma;
  return 1.0 / static_cast<double>(std::max<std::int64_t>(1, feature_count));
}

[[nodiscard]] Series kernel_similarity(const Series& query_rows,
                                       const Series& train_rows,
                                       KernelKind kernel,
                                       double gamma) {
  if (kernel == KernelKind::Linear) {
    return query_rows.matmul(train_rows.transpose(0, 1));
  }
  if (kernel == KernelKind::Rbf) {
    return pairwise_distance2(query_rows, train_rows).mul(-gamma).exp();
  }
  if (kernel == KernelKind::Polynomial) {
    return query_rows.matmul(train_rows.transpose(0, 1)).mul(gamma).add(1.0).pow(2.0);
  }
  return query_rows.matmul(train_rows.transpose(0, 1)).mul(gamma).tanh();
}

class KMeansTransformer final : public TransformerBase {
public:
  explicit KMeansTransformer(ClusterFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    Status options_status = validate_cluster_options(options_);
    if (!options_status.ok()) return options_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KMeansTransformer::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "KMeansTransformer::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }
    if (options_.clusters > matrix.sample_count) {
      return Status::invalid_argument("KMeansTransformer::learn requires clusters <= sample count.");
    }

    const std::int64_t n = matrix.sample_count;
    const std::int64_t p = matrix.feature_count;
    const std::int64_t k = options_.clusters;
    const Platform platform = matrix.x_rows.platform();

    std::uint64_t base_seed = compose_seed(options.random);
    if (base_seed == 0) base_seed = 1;

    Series best_centers = Series::undefined();
    double best_inertia = std::numeric_limits<double>::infinity();

    for (std::int64_t restart = 0; restart < options_.restarts; ++restart) {
      std::mt19937_64 rng(splitmix64(base_seed + static_cast<std::uint64_t>(restart)));
      std::vector<std::int64_t> ids(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) ids[static_cast<std::size_t>(i)] = i;
      std::shuffle(ids.begin(), ids.end(), rng);
      ids.resize(static_cast<std::size_t>(k));
      Series center_idx = pluss::make_series(ids, {k}, i64_on(platform));
      Series centers = matrix.x_rows.index_select(0, center_idx).reshape({k, p});
      Series prev_assign = Series::undefined();

      for (std::int64_t iter = 0; iter < options_.max_iterations; ++iter) {
        const Series d2 = pairwise_distance2(matrix.x_rows, centers);
        const Series assign = d2.argmin(1, false).reshape({n}).to(ScalarType::Int64);
        if (prev_assign.defined() && assign.eq(prev_assign).all().item<bool>()) break;
        prev_assign = assign;

        Series next_centers = Series::undefined();
        for (std::int64_t ci = 0; ci < k; ++ci) {
          const Series mask = assign.eq(ci).to(ScalarType::Float64).reshape({n, 1});
          const double count = mask.sum().item<double>();
          Series center_row = Series::undefined();
          if (count > 0.0) {
            center_row = mask.mul(matrix.x_rows).sum(0, false).reshape({1, p}).div(count);
          } else {
            const Series nearest = d2.min(1, false);
            const Series far_idx = nearest.argmax(0, false).reshape({1}).to(ScalarType::Int64);
            center_row = matrix.x_rows.index_select(0, far_idx).reshape({1, p});
          }
          next_centers = next_centers.defined() ? next_centers.unite({center_row}, 0) : center_row;
        }

        const double drift = next_centers.sub(centers).square().mean().sqrt().item<double>();
        centers = next_centers;
        if (drift <= options_.tolerance) break;
      }

      const Series inertia_terms = pairwise_distance2(matrix.x_rows, centers).min(1, false);
      const double inertia = inertia_terms.sum().item<double>();
      if (std::isfinite(inertia) && inertia < best_inertia) {
        best_inertia = inertia;
        best_centers = centers;
      }
    }

    if (!best_centers.defined() || !all_finite(best_centers)) {
      return Status::numerical_failure("KMeansTransformer::learn failed to produce finite cluster centers.");
    }

    centers_ = best_centers.reshape({options_.clusters, matrix.feature_count});
    feature_count_ = matrix.feature_count;
    layout_ = batch.layout;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override {
    (void)options;
    if (!trained_) return StatusOr<Series>(Status::precondition("KMeansTransformer requires learn() before map()."));
    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KMeansTransformer::map");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (matrix.feature_count != feature_count_) {
      return StatusOr<Series>(Status::shape_mismatch("KMeansTransformer::map feature count mismatch with fitted state."));
    }
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument("KMeansTransformer::map does not support NaN features."));
    }

    const Series labels = pairwise_distance2(matrix.x_rows, centers_).argmin(1, false).reshape({matrix.sample_count});
    if (matrix.layout == ObservationLayout::Rows) {
      return StatusOr<Series>(labels.to(matrix.original_platform, ScalarType::Float64).reshape({matrix.sample_count, 1}));
    }
    return StatusOr<Series>(labels.to(matrix.original_platform, ScalarType::Float64).reshape({1, matrix.sample_count}));
  }

  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override {
    Status status = learn(batch, learn_options);
    if (!status.ok()) return StatusOr<Series>(status);
    return map(batch, infer_options);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(static_cast<std::int64_t>(layout_))}),
        centers_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("KMeansTransformer::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 3) return Status::invalid_argument("KMeansTransformer::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[2]));
    centers_ = state[1];
    return Status::success();
  }

private:
  ClusterFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  ObservationLayout layout_{ObservationLayout::Rows};
  Series centers_ = Series::undefined();
};

class KernelOneClassTransformer final : public TransformerBase {
public:
  explicit KernelOneClassTransformer(KernelFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override {
    (void)options;
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    Status option_status = validate_kernel_transform_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelOneClassTransformer::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument("KernelOneClassTransformer::learn does not support NaN features.");
    }

    feature_count_ = matrix.feature_count;
    layout_ = batch.layout;
    gamma_ = resolved_gamma(options_.gamma, feature_count_);
    x_train_ = matrix.x_rows;

    const Series k = kernel_similarity(x_train_, x_train_, options_.kernel, gamma_);
    const Series score = k.mean(1, false).reshape({matrix.sample_count});
    const Series sorted = score.sort(0, false, false).reshape({matrix.sample_count});
    const std::int64_t qidx = std::clamp<std::int64_t>(
        static_cast<std::int64_t>(std::floor(options_.nu * static_cast<double>(matrix.sample_count))) - 1,
        0,
        matrix.sample_count - 1);
    const Series qseries = pluss::make_series(std::vector<std::int64_t>{qidx}, {1}, i64_on(sorted.platform()));
    offset_ = sorted.index_select(0, qseries).reshape({1});
    trained_ = all_finite(offset_);
    if (!trained_) {
      return Status::numerical_failure("KernelOneClassTransformer::learn failed to produce finite offset.");
    }
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override {
    (void)options;
    if (!trained_) {
      return StatusOr<Series>(Status::precondition("KernelOneClassTransformer requires learn() before map()."));
    }
    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelOneClassTransformer::map");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (matrix.feature_count != feature_count_) {
      return StatusOr<Series>(Status::shape_mismatch("KernelOneClassTransformer::map feature count mismatch."));
    }
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument("KernelOneClassTransformer::map does not support NaN features."));
    }
    const Series k = kernel_similarity(matrix.x_rows, x_train_, options_.kernel, gamma_);
    const Series score = k.mean(1, false).reshape({matrix.sample_count});
    const Series ones = pluss::ones_like(score, ScalarType::Float64);
    const Series labels = score.ge(offset_.reshape({1})).where(ones, ones.mul(-1.0));
    if (matrix.layout == ObservationLayout::Rows) {
      return StatusOr<Series>(labels.to(matrix.original_platform, ScalarType::Float64).reshape({matrix.sample_count, 1}));
    }
    return StatusOr<Series>(labels.to(matrix.original_platform, ScalarType::Float64).reshape({1, matrix.sample_count}));
  }

  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override {
    Status status = learn(batch, learn_options);
    if (!status.ok()) return StatusOr<Series>(status);
    return map(batch, infer_options);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(static_cast<std::int64_t>(layout_)),
                   gamma_,
                   0.0}),
        x_train_,
        offset_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("KernelOneClassTransformer::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 5) return Status::invalid_argument("KernelOneClassTransformer::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[2]));
    gamma_ = meta[3];
    x_train_ = state[1];
    offset_ = state.size() > 2
                  ? state[2].to(x_train_.platform(), ScalarType::Float64).reshape({1})
                  : pluss::make_series(std::vector<double>{meta[4]}, {1}, f64_on(x_train_.platform()));
    return Status::success();
  }

private:
  KernelFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  ObservationLayout layout_{ObservationLayout::Rows};
  double gamma_{1.0};
  Series offset_ = Series::undefined();
  Series x_train_ = Series::undefined();
};

class PcaTransformer final : public TransformerBase {
public:
  explicit PcaTransformer(DecomposeFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    Status options_status = validate_decompose_options(options_);
    if (!options_status.ok()) return options_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "PcaTransformer::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "PcaTransformer::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    const std::int64_t n = matrix.sample_count;
    const std::int64_t p = matrix.feature_count;
    const std::int64_t max_comp = std::min(n, p);
    const std::int64_t k = options_.components < 0 ? max_comp : std::min(options_.components, max_comp);
    if (k <= 0) {
      return Status::invalid_argument("PcaTransformer::learn resolved zero principal components.");
    }

    const Platform platform = matrix.x_rows.platform();
    const Series centered = matrix.x_rows.sub(matrix.x_rows.mean(0, false).reshape({1, p}));
    const double denom = static_cast<double>(std::max<std::int64_t>(1, n - 1));
    Series cov = centered.transpose(0, 1).matmul(centered).div(denom).reshape({p, p});

    std::vector<Series> component_cols{};
    component_cols.reserve(static_cast<std::size_t>(k));
    std::vector<double> eigen_vals{};
    eigen_vals.reserve(static_cast<std::size_t>(k));

    std::uint64_t seed = compose_seed(options.random);
    if (seed == 0) seed = 1;
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> uni(-1.0, 1.0);

    for (std::int64_t ci = 0; ci < k; ++ci) {
      std::vector<double> init(static_cast<std::size_t>(p), 0.0);
      for (double& v : init) v = uni(rng);
      Series v = pluss::make_series(init, {p}, f64_on(platform));
      const double vn = v.square().sum().sqrt().item<double>();
      if (vn <= 0.0 || !std::isfinite(vn)) {
        v = pluss::ones({p}, f64_on(platform)).div(std::sqrt(static_cast<double>(p)));
      } else {
        v = v.div(vn);
      }

      for (std::int64_t iter = 0; iter < options_.max_iterations; ++iter) {
        Series w = cov.matmul(v.reshape({p, 1})).reshape({p});
        const double wn = w.square().sum().sqrt().item<double>();
        if (!(wn > 0.0) || !std::isfinite(wn)) break;
        Series next = w.div(wn);
        const double delta = next.sub(v).abs().max().item<double>();
        v = next;
        if (delta <= options_.tolerance) break;
      }

      const double ev = v.reshape({1, p}).matmul(cov).matmul(v.reshape({p, 1})).reshape({1}).item<double>();
      if (!std::isfinite(ev) || ev < 0.0) {
        return Status::numerical_failure("PcaTransformer::learn encountered non-finite eigenvalue.");
      }
      component_cols.push_back(v.reshape({p, 1}));
      eigen_vals.push_back(ev);
      cov = cov.sub(v.reshape({p, 1}).matmul(v.reshape({1, p})).mul(ev));
    }

    if (component_cols.empty()) {
      return Status::numerical_failure("PcaTransformer::learn failed to extract principal components.");
    }

    Series comps = component_cols.front();
    for (std::size_t i = 1; i < component_cols.size(); ++i) {
      comps = comps.unite({component_cols[i]}, 1);
    }

    components_ = comps.reshape({p, static_cast<std::int64_t>(component_cols.size())});
    variances_ = pluss::make_series(eigen_vals,
                                    {static_cast<std::int64_t>(eigen_vals.size())},
                                    f64_on(platform));
    mean_ = matrix.x_rows.mean(0, false).reshape({p});
    feature_count_ = p;
    component_count_ = static_cast<std::int64_t>(component_cols.size());
    layout_ = batch.layout;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override {
    (void)options;
    if (!trained_) return StatusOr<Series>(Status::precondition("PcaTransformer requires learn() before map()."));
    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "PcaTransformer::map");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (matrix.feature_count != feature_count_) {
      return StatusOr<Series>(Status::shape_mismatch("PcaTransformer::map feature count mismatch with fitted state."));
    }
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument("PcaTransformer::map does not support NaN features."));
    }

    Series projected = matrix.x_rows.sub(mean_.reshape({1, feature_count_})).matmul(components_);
    if (options_.whiten) {
      projected = projected.div(variances_.clamp_min(1e-12).sqrt().reshape({1, component_count_}));
    }
    if (matrix.layout == ObservationLayout::Rows) {
      return StatusOr<Series>(projected.to(matrix.original_platform, ScalarType::Float64));
    }
    return StatusOr<Series>(projected.transpose(0, 1).to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override {
    Status status = learn(batch, learn_options);
    if (!status.ok()) return StatusOr<Series>(status);
    return map(batch, infer_options);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(component_count_),
                   static_cast<double>(static_cast<std::int64_t>(layout_))}),
        mean_,
        components_,
        variances_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 4) return Status::invalid_argument("PcaTransformer::load_state expects 4 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("PcaTransformer::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    component_count_ = static_cast<std::int64_t>(meta[2]);
    layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[3]));
    mean_ = state[1];
    components_ = state[2];
    variances_ = state[3];
    return Status::success();
  }

private:
  DecomposeFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  std::int64_t component_count_{0};
  ObservationLayout layout_{ObservationLayout::Rows};
  Series mean_ = Series::undefined();
  Series components_ = Series::undefined();
  Series variances_ = Series::undefined();
};

} // namespace

StatusOr<std::unique_ptr<TransformerBase>> make_transformer(const ModelSpec& spec) {
  if (spec.task != TaskKind::Transform) {
    return StatusOr<std::unique_ptr<TransformerBase>>(
        Status::invalid_argument("make_transformer requires ModelSpec.task = TaskKind::Transform."));
  }
  if (spec.family == ModelFamily::Cluster) {
    const auto* options = std::get_if<ClusterFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<TransformerBase>>(
          Status::invalid_argument("Cluster transformer requires ClusterFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<KMeansTransformer>(*options));
  }
  if (spec.family == ModelFamily::Decompose) {
    const auto* options = std::get_if<DecomposeFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<TransformerBase>>(
          Status::invalid_argument("Decompose transformer requires DecomposeFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<PcaTransformer>(*options));
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<TransformerBase>>(
          Status::invalid_argument("Kernel transformer requires KernelFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<KernelOneClassTransformer>(*options));
  }
  return StatusOr<std::unique_ptr<TransformerBase>>(
      Status::invalid_argument("Unsupported transformer family; expected Cluster, Decompose, or Kernel(one-class)."));
}

} // namespace pluss::ai::nodej
