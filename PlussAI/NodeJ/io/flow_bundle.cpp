#include "pluss/ai/nodej/io.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::ai::nodej::io {
namespace {

using pluss::Config;
using pluss::ScalarType;

constexpr const char* k_magic = "PNJBFLW1";

class BinaryWriter {
public:
  explicit BinaryWriter(const std::string& path) : out_(path, std::ios::binary) {}

  [[nodiscard]] bool ok() const { return out_.good(); }

  template <class T>
  void pod(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    out_.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(T)));
  }

  void string(const std::string& value) {
    const std::uint64_t n = static_cast<std::uint64_t>(value.size());
    pod(n);
    if (n > 0) out_.write(value.data(), static_cast<std::streamsize>(n));
  }

  void series(const Series& value) {
    const std::uint8_t defined = value.defined() ? 1U : 0U;
    pod(defined);
    if (!defined) return;
    const std::int64_t rank = static_cast<std::int64_t>(value.sizes().size());
    pod(rank);
    for (std::int64_t i = 0; i < rank; ++i) pod(value.sizes()[static_cast<std::size_t>(i)]);
    const Series host = value.to(pluss::pCPU, ScalarType::Float64).reshape({value.numel()});
    const std::vector<double> data = host.to_vec<double>();
    const std::uint64_t n = static_cast<std::uint64_t>(data.size());
    pod(n);
    if (n > 0) out_.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(n * sizeof(double)));
  }

private:
  std::ofstream out_;
};

class BinaryReader {
public:
  explicit BinaryReader(const std::string& path) : in_(path, std::ios::binary) {}

  [[nodiscard]] bool ok() const { return in_.good(); }

  template <class T>
  bool pod(T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    in_.read(reinterpret_cast<char*>(&value), static_cast<std::streamsize>(sizeof(T)));
    return in_.good();
  }

  bool string(std::string& value) {
    std::uint64_t n = 0;
    if (!pod(n)) return false;
    value.resize(static_cast<std::size_t>(n));
    if (n == 0) return true;
    in_.read(value.data(), static_cast<std::streamsize>(n));
    return in_.good();
  }

  bool series(Series& value) {
    std::uint8_t defined = 0;
    if (!pod(defined)) return false;
    if (!defined) {
      value = Series::undefined();
      return true;
    }
    std::int64_t rank = 0;
    if (!pod(rank) || rank < 0) return false;
    std::vector<std::int64_t> shape(static_cast<std::size_t>(rank), 0);
    for (std::int64_t i = 0; i < rank; ++i) {
      if (!pod(shape[static_cast<std::size_t>(i)])) return false;
    }
    std::uint64_t n = 0;
    if (!pod(n)) return false;
    std::vector<double> data(static_cast<std::size_t>(n), 0.0);
    if (n > 0) {
      in_.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(n * sizeof(double)));
      if (!in_.good()) return false;
    }
    value = pluss::make_series(std::move(data),
                               shape,
                               Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
    return true;
  }

private:
  std::ifstream in_;
};

template <class EnumT>
void write_enum(BinaryWriter& w, EnumT value) {
  const auto raw = static_cast<std::underlying_type_t<EnumT>>(value);
  w.pod(raw);
}

template <class EnumT>
bool read_enum(BinaryReader& r, EnumT& value) {
  std::underlying_type_t<EnumT> raw{};
  if (!r.pod(raw)) return false;
  value = static_cast<EnumT>(raw);
  return true;
}

void write_random_state(BinaryWriter& w, const RandomState& rs) {
  w.pod(rs.seed);
  w.pod(static_cast<std::uint8_t>(rs.deterministic ? 1U : 0U));
}

bool read_random_state(BinaryReader& r, RandomState& rs) {
  std::uint8_t det = 0;
  if (!r.pod(rs.seed) || !r.pod(det)) return false;
  rs.deterministic = det != 0;
  return true;
}

void write_train_options(BinaryWriter& w, const TrainOptions& options) {
  write_enum(w, options.layout);
  write_enum(w, options.missing);
  write_enum(w, options.unknown_categorical);
  w.pod(options.max_iterations);
  w.pod(options.tolerance);
  w.pod(static_cast<std::uint8_t>(options.warm_start ? 1U : 0U));
  w.pod(static_cast<std::uint8_t>(options.track_diagnostics ? 1U : 0U));
  write_random_state(w, options.random);
  w.series(options.sample_weights);
  write_enum(w, options.class_weight);
  w.series(options.class_weights);
  write_enum(w, options.calibration);
  w.pod(options.calibration_iterations);
  w.pod(options.calibration_tolerance);
  w.series(options.parameter_lower_bounds);
  w.series(options.parameter_upper_bounds);
}

bool read_train_options(BinaryReader& r, TrainOptions& options) {
  std::uint8_t warm = 0;
  std::uint8_t diag = 0;
  if (!read_enum(r, options.layout)
      || !read_enum(r, options.missing)
      || !read_enum(r, options.unknown_categorical)
      || !r.pod(options.max_iterations)
      || !r.pod(options.tolerance)
      || !r.pod(warm)
      || !r.pod(diag)
      || !read_random_state(r, options.random)
      || !r.series(options.sample_weights)
      || !read_enum(r, options.class_weight)
      || !r.series(options.class_weights)
      || !read_enum(r, options.calibration)
      || !r.pod(options.calibration_iterations)
      || !r.pod(options.calibration_tolerance)
      || !r.series(options.parameter_lower_bounds)
      || !r.series(options.parameter_upper_bounds)) {
    return false;
  }
  options.warm_start = warm != 0;
  options.track_diagnostics = diag != 0;
  return true;
}

void write_feature_batch(BinaryWriter& w, const FeatureBatch& batch) {
  w.series(batch.x);
  write_enum(w, batch.layout);
}

bool read_feature_batch(BinaryReader& r, FeatureBatch& batch) {
  return r.series(batch.x) && read_enum(r, batch.layout);
}

void write_labeled_batch(BinaryWriter& w, const LabeledBatch& batch) {
  write_feature_batch(w, batch.features);
  w.series(batch.y);
  w.series(batch.sample_weights);
}

bool read_labeled_batch(BinaryReader& r, LabeledBatch& batch) {
  return read_feature_batch(r, batch.features)
         && r.series(batch.y)
         && r.series(batch.sample_weights);
}

void write_series_vector(BinaryWriter& w, const std::vector<Series>& values) {
  const std::uint64_t n = static_cast<std::uint64_t>(values.size());
  w.pod(n);
  for (const Series& value : values) {
    w.series(value);
  }
}

bool read_series_vector(BinaryReader& r, std::vector<Series>& values) {
  std::uint64_t n = 0;
  if (!r.pod(n)) return false;
  values.resize(static_cast<std::size_t>(n));
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!r.series(values[i])) return false;
  }
  return true;
}

void write_family_options(BinaryWriter& w, const ModelSpec& spec) {
  if (spec.family == ModelFamily::Linear) {
    const auto& o = std::get<LinearFamilyOptions>(spec.options);
    write_enum(w, o.objective);
    write_enum(w, o.regularization);
    w.pod(o.regularization_strength);
    w.pod(static_cast<std::uint8_t>(o.fit_intercept ? 1U : 0U));
    return;
  }
  if (spec.family == ModelFamily::Tree) {
    const auto& o = std::get<TreeFamilyOptions>(spec.options);
    write_enum(w, o.criterion);
    w.pod(o.max_depth);
    w.pod(o.min_samples_leaf);
    w.pod(o.min_samples_split);
    return;
  }
  if (spec.family == ModelFamily::Ensemble) {
    const auto& o = std::get<EnsembleFamilyOptions>(spec.options);
    write_enum(w, o.method);
    w.pod(o.learners);
    w.pod(o.subsample);
    return;
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto& o = std::get<NeighborFamilyOptions>(spec.options);
    w.pod(o.k);
    w.pod(static_cast<std::uint8_t>(o.distance_weighted ? 1U : 0U));
    w.pod(static_cast<std::uint8_t>(o.standardize ? 1U : 0U));
    return;
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto& o = std::get<KernelFamilyOptions>(spec.options);
    write_enum(w, o.kernel);
    w.pod(o.c);
    w.pod(o.gamma);
    w.pod(o.epsilon);
    return;
  }
  if (spec.family == ModelFamily::Bayes) {
    const auto& o = std::get<BayesFamilyOptions>(spec.options);
    w.pod(static_cast<std::uint8_t>(o.use_gaussian ? 1U : 0U));
    w.pod(o.smoothing);
    return;
  }
  if (spec.family == ModelFamily::Cluster) {
    const auto& o = std::get<ClusterFamilyOptions>(spec.options);
    write_enum(w, o.method);
    w.pod(o.clusters);
    w.pod(o.max_iterations);
    w.pod(o.restarts);
    w.pod(o.tolerance);
    return;
  }
  const auto& o = std::get<DecomposeFamilyOptions>(spec.options);
  write_enum(w, o.method);
  w.pod(o.components);
  w.pod(static_cast<std::uint8_t>(o.whiten ? 1U : 0U));
  w.pod(o.max_iterations);
  w.pod(o.tolerance);
}

bool read_family_options(BinaryReader& r, ModelSpec& spec) {
  if (spec.family == ModelFamily::Linear) {
    LinearFamilyOptions o{};
    std::uint8_t fit = 0;
    if (!read_enum(r, o.objective) || !read_enum(r, o.regularization) || !r.pod(o.regularization_strength) || !r.pod(fit)) return false;
    o.fit_intercept = fit != 0;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Tree) {
    TreeFamilyOptions o{};
    if (!read_enum(r, o.criterion) || !r.pod(o.max_depth) || !r.pod(o.min_samples_leaf) || !r.pod(o.min_samples_split)) return false;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Ensemble) {
    EnsembleFamilyOptions o{};
    if (!read_enum(r, o.method) || !r.pod(o.learners) || !r.pod(o.subsample)) return false;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Neighbor) {
    NeighborFamilyOptions o{};
    std::uint8_t dw = 0;
    std::uint8_t stdize = 0;
    if (!r.pod(o.k) || !r.pod(dw) || !r.pod(stdize)) return false;
    o.distance_weighted = dw != 0;
    o.standardize = stdize != 0;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Kernel) {
    KernelFamilyOptions o{};
    if (!read_enum(r, o.kernel) || !r.pod(o.c) || !r.pod(o.gamma) || !r.pod(o.epsilon)) return false;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Bayes) {
    BayesFamilyOptions o{};
    std::uint8_t use = 0;
    if (!r.pod(use) || !r.pod(o.smoothing)) return false;
    o.use_gaussian = use != 0;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Cluster) {
    ClusterFamilyOptions o{};
    if (!read_enum(r, o.method) || !r.pod(o.clusters) || !r.pod(o.max_iterations) || !r.pod(o.restarts) || !r.pod(o.tolerance)) return false;
    spec.options = o;
    return true;
  }
  if (spec.family == ModelFamily::Decompose) {
    DecomposeFamilyOptions o{};
    std::uint8_t whiten = 0;
    if (!read_enum(r, o.method) || !r.pod(o.components) || !r.pod(whiten) || !r.pod(o.max_iterations) || !r.pod(o.tolerance)) return false;
    o.whiten = whiten != 0;
    spec.options = o;
    return true;
  }
  return false;
}

void write_model_spec(BinaryWriter& w, const ModelSpec& spec) {
  write_enum(w, spec.task);
  write_enum(w, spec.family);
  write_family_options(w, spec);
}

bool read_model_spec(BinaryReader& r, ModelSpec& spec) {
  if (!read_enum(r, spec.task) || !read_enum(r, spec.family)) return false;
  return read_family_options(r, spec);
}

void write_transform_spec(BinaryWriter& w, const TransformSpec& spec) {
  write_enum(w, spec.kind);
  const std::uint64_t route_count = static_cast<std::uint64_t>(spec.feature_route.size());
  w.pod(route_count);
  for (std::int64_t idx : spec.feature_route) {
    w.pod(idx);
  }
  switch (spec.kind) {
    case TransformKind::StandardScaler: {
      const auto& o = std::get<prep::StandardScalerOptions>(spec.options);
      w.pod(static_cast<std::uint8_t>(o.with_centering ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.with_scaling ? 1U : 0U));
      w.pod(o.variance_floor);
      break;
    }
    case TransformKind::MinMaxScaler: {
      const auto& o = std::get<prep::MinMaxScalerOptions>(spec.options);
      w.pod(o.lower);
      w.pod(o.upper);
      w.pod(static_cast<std::uint8_t>(o.clip ? 1U : 0U));
      w.pod(o.range_floor);
      break;
    }
    case TransformKind::RobustScaler: {
      const auto& o = std::get<prep::RobustScalerOptions>(spec.options);
      w.pod(static_cast<std::uint8_t>(o.with_centering ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.with_scaling ? 1U : 0U));
      w.pod(o.lower_quantile);
      w.pod(o.upper_quantile);
      w.pod(o.range_floor);
      break;
    }
    case TransformKind::SimpleImputer: {
      const auto& o = std::get<prep::SimpleImputerOptions>(spec.options);
      write_enum(w, o.strategy);
      w.pod(o.constant_value);
      w.pod(o.empty_feature_fallback);
      break;
    }
    case TransformKind::NeighborImputer: {
      const auto& o = std::get<prep::NeighborImputerOptions>(spec.options);
      w.pod(o.neighbors);
      write_enum(w, o.weighting);
      w.pod(o.distance_floor);
      w.pod(o.empty_feature_fallback);
      break;
    }
    case TransformKind::IterativeImputer: {
      const auto& o = std::get<prep::IterativeImputerOptions>(spec.options);
      w.pod(o.rounds);
      w.pod(o.ridge);
      w.pod(o.empty_feature_fallback);
      break;
    }
    case TransformKind::OneHotEncoder: {
      const auto& o = std::get<prep::OneHotEncoderOptions>(spec.options);
      w.pod(o.max_categories);
      write_enum(w, o.overflow);
      w.pod(static_cast<std::uint8_t>(o.include_unknown_bucket ? 1U : 0U));
      break;
    }
    case TransformKind::FrequencyEncoder: {
      const auto& o = std::get<prep::FrequencyEncoderOptions>(spec.options);
      w.pod(o.max_categories);
      write_enum(w, o.overflow);
      w.pod(o.unknown_frequency);
      break;
    }
    case TransformKind::VectorNormalizer: {
      const auto& o = std::get<prep::VectorNormalizerOptions>(spec.options);
      write_enum(w, o.norm);
      w.pod(o.floor);
      break;
    }
    case TransformKind::MaxAbsScaler: {
      const auto& o = std::get<prep::MaxAbsScalerOptions>(spec.options);
      w.pod(o.floor);
      break;
    }
    case TransformKind::QuantileMap: {
      const auto& o = std::get<prep::QuantileMapOptions>(spec.options);
      write_enum(w, o.output);
      w.pod(o.clip);
      break;
    }
    case TransformKind::PowerMap: {
      const auto& o = std::get<prep::PowerMapOptions>(spec.options);
      write_enum(w, o.mode);
      w.pod(static_cast<std::uint8_t>(o.standardize ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.lambda.has_value() ? 1U : 0U));
      if (o.lambda.has_value()) w.pod(o.lambda.value());
      w.pod(o.lambda_min);
      w.pod(o.lambda_max);
      w.pod(o.lambda_grid);
      w.pod(o.variance_floor);
      break;
    }
    case TransformKind::ThresholdBinarizer: {
      const auto& o = std::get<prep::ThresholdBinarizerOptions>(spec.options);
      w.pod(o.threshold);
      w.pod(o.low_value);
      w.pod(o.high_value);
      break;
    }
    case TransformKind::PolynomialLift: {
      const auto& o = std::get<prep::PolynomialLiftOptions>(spec.options);
      w.pod(o.degree);
      w.pod(static_cast<std::uint8_t>(o.interactions_only ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.include_bias ? 1U : 0U));
      break;
    }
    case TransformKind::OrdinalEncoder: {
      const auto& o = std::get<prep::OrdinalEncoderOptions>(spec.options);
      w.pod(o.max_categories);
      write_enum(w, o.overflow);
      w.pod(static_cast<std::uint8_t>(o.include_unknown_bucket ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.one_based ? 1U : 0U));
      break;
    }
    case TransformKind::LabelEncoder: {
      const auto& o = std::get<prep::LabelEncoderOptions>(spec.options);
      w.pod(o.max_categories);
      write_enum(w, o.overflow);
      w.pod(static_cast<std::uint8_t>(o.include_unknown_bucket ? 1U : 0U));
      w.pod(static_cast<std::uint8_t>(o.one_based ? 1U : 0U));
      break;
    }
  }
}

bool read_transform_spec(BinaryReader& r, TransformSpec& spec) {
  if (!read_enum(r, spec.kind)) return false;
  std::uint64_t route_count = 0;
  if (!r.pod(route_count)) return false;
  spec.feature_route.resize(static_cast<std::size_t>(route_count));
  for (std::uint64_t i = 0; i < route_count; ++i) {
    if (!r.pod(spec.feature_route[static_cast<std::size_t>(i)])) return false;
  }
  switch (spec.kind) {
    case TransformKind::StandardScaler: {
      prep::StandardScalerOptions o{};
      std::uint8_t c = 0;
      std::uint8_t s = 0;
      if (!r.pod(c) || !r.pod(s) || !r.pod(o.variance_floor)) return false;
      o.with_centering = c != 0;
      o.with_scaling = s != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::MinMaxScaler: {
      prep::MinMaxScalerOptions o{};
      std::uint8_t clip = 0;
      if (!r.pod(o.lower) || !r.pod(o.upper) || !r.pod(clip) || !r.pod(o.range_floor)) return false;
      o.clip = clip != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::RobustScaler: {
      prep::RobustScalerOptions o{};
      std::uint8_t c = 0;
      std::uint8_t s = 0;
      if (!r.pod(c) || !r.pod(s) || !r.pod(o.lower_quantile) || !r.pod(o.upper_quantile) || !r.pod(o.range_floor)) return false;
      o.with_centering = c != 0;
      o.with_scaling = s != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::SimpleImputer: {
      prep::SimpleImputerOptions o{};
      if (!read_enum(r, o.strategy) || !r.pod(o.constant_value) || !r.pod(o.empty_feature_fallback)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::NeighborImputer: {
      prep::NeighborImputerOptions o{};
      if (!r.pod(o.neighbors) || !read_enum(r, o.weighting) || !r.pod(o.distance_floor) || !r.pod(o.empty_feature_fallback)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::IterativeImputer: {
      prep::IterativeImputerOptions o{};
      if (!r.pod(o.rounds) || !r.pod(o.ridge) || !r.pod(o.empty_feature_fallback)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::OneHotEncoder: {
      prep::OneHotEncoderOptions o{};
      std::uint8_t include = 0;
      if (!r.pod(o.max_categories) || !read_enum(r, o.overflow) || !r.pod(include)) return false;
      o.include_unknown_bucket = include != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::FrequencyEncoder: {
      prep::FrequencyEncoderOptions o{};
      if (!r.pod(o.max_categories) || !read_enum(r, o.overflow) || !r.pod(o.unknown_frequency)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::VectorNormalizer: {
      prep::VectorNormalizerOptions o{};
      if (!read_enum(r, o.norm) || !r.pod(o.floor)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::MaxAbsScaler: {
      prep::MaxAbsScalerOptions o{};
      if (!r.pod(o.floor)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::QuantileMap: {
      prep::QuantileMapOptions o{};
      if (!read_enum(r, o.output) || !r.pod(o.clip)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::PowerMap: {
      prep::PowerMapOptions o{};
      std::uint8_t standardize = 0;
      std::uint8_t has_lambda = 0;
      if (!read_enum(r, o.mode) || !r.pod(standardize) || !r.pod(has_lambda)) return false;
      o.standardize = standardize != 0;
      if (has_lambda) {
        double lambda = 0.0;
        if (!r.pod(lambda)) return false;
        o.lambda = lambda;
      } else {
        o.lambda = std::nullopt;
      }
      if (!r.pod(o.lambda_min) || !r.pod(o.lambda_max) || !r.pod(o.lambda_grid) || !r.pod(o.variance_floor)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::ThresholdBinarizer: {
      prep::ThresholdBinarizerOptions o{};
      if (!r.pod(o.threshold) || !r.pod(o.low_value) || !r.pod(o.high_value)) return false;
      spec.options = o;
      return true;
    }
    case TransformKind::PolynomialLift: {
      prep::PolynomialLiftOptions o{};
      std::uint8_t inter = 0;
      std::uint8_t bias = 0;
      if (!r.pod(o.degree) || !r.pod(inter) || !r.pod(bias)) return false;
      o.interactions_only = inter != 0;
      o.include_bias = bias != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::OrdinalEncoder: {
      prep::OrdinalEncoderOptions o{};
      std::uint8_t include = 0;
      std::uint8_t one_based = 0;
      if (!r.pod(o.max_categories) || !read_enum(r, o.overflow) || !r.pod(include) || !r.pod(one_based)) return false;
      o.include_unknown_bucket = include != 0;
      o.one_based = one_based != 0;
      spec.options = o;
      return true;
    }
    case TransformKind::LabelEncoder: {
      prep::LabelEncoderOptions o{};
      std::uint8_t include = 0;
      std::uint8_t one_based = 0;
      if (!r.pod(o.max_categories) || !read_enum(r, o.overflow) || !r.pod(include) || !r.pod(one_based)) return false;
      o.include_unknown_bucket = include != 0;
      o.one_based = one_based != 0;
      spec.options = o;
      return true;
    }
  }
  return false;
}

Status add_transform_from_spec(Flow& flow, const TransformSpec& spec) {
  switch (spec.kind) {
    case TransformKind::StandardScaler:
      return flow.add_standard_scaler(std::get<prep::StandardScalerOptions>(spec.options), spec.feature_route);
    case TransformKind::MinMaxScaler:
      return flow.add_minmax_scaler(std::get<prep::MinMaxScalerOptions>(spec.options), spec.feature_route);
    case TransformKind::RobustScaler:
      return flow.add_robust_scaler(std::get<prep::RobustScalerOptions>(spec.options), spec.feature_route);
    case TransformKind::SimpleImputer:
      return flow.add_simple_imputer(std::get<prep::SimpleImputerOptions>(spec.options), spec.feature_route);
    case TransformKind::NeighborImputer:
      return flow.add_neighbor_imputer(std::get<prep::NeighborImputerOptions>(spec.options), spec.feature_route);
    case TransformKind::IterativeImputer:
      return flow.add_iterative_imputer(std::get<prep::IterativeImputerOptions>(spec.options), spec.feature_route);
    case TransformKind::OneHotEncoder:
      return flow.add_one_hot_encoder(std::get<prep::OneHotEncoderOptions>(spec.options), spec.feature_route);
    case TransformKind::FrequencyEncoder:
      return flow.add_frequency_encoder(std::get<prep::FrequencyEncoderOptions>(spec.options), spec.feature_route);
    case TransformKind::VectorNormalizer:
      return flow.add_vector_normalizer(std::get<prep::VectorNormalizerOptions>(spec.options), spec.feature_route);
    case TransformKind::MaxAbsScaler:
      return flow.add_maxabs_scaler(std::get<prep::MaxAbsScalerOptions>(spec.options), spec.feature_route);
    case TransformKind::QuantileMap:
      return flow.add_quantile_map(std::get<prep::QuantileMapOptions>(spec.options), spec.feature_route);
    case TransformKind::PowerMap:
      return flow.add_power_map(std::get<prep::PowerMapOptions>(spec.options), spec.feature_route);
    case TransformKind::ThresholdBinarizer:
      return flow.add_threshold_binarizer(std::get<prep::ThresholdBinarizerOptions>(spec.options), spec.feature_route);
    case TransformKind::PolynomialLift:
      return flow.add_polynomial_lift(std::get<prep::PolynomialLiftOptions>(spec.options), spec.feature_route);
    case TransformKind::OrdinalEncoder:
      return flow.add_ordinal_encoder(std::get<prep::OrdinalEncoderOptions>(spec.options), spec.feature_route);
    case TransformKind::LabelEncoder:
      return flow.add_label_encoder(std::get<prep::LabelEncoderOptions>(spec.options), spec.feature_route);
  }
  return Status::invalid_argument("Unknown TransformKind in flow bundle.");
}

} // namespace

Status save(const std::string& path,
            const Flow& flow,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options,
            FlowBundleMetadata metadata) {
  if (!flow.has_model_spec() || !flow.model_spec().has_value()) {
    return Status::invalid_argument("save requires a Flow configured via set_model(ModelSpec).");
  }
  if (flow.has_opaque_transforms()) {
    return Status::invalid_argument(
        "save cannot serialize opaque transform stages added via append_transform().");
  }
  Status batch_status = validate_labeled(train_batch);
  const bool has_fallback_train = batch_status.ok();

  bool has_snapshot = false;
  std::vector<std::vector<Series>> transform_states{};
  std::vector<Series> model_state{};
  if (flow.trained()) {
    StatusOr<std::vector<std::vector<Series>>> tstates_or = flow.dump_transform_states();
    StatusOr<std::vector<Series>> mstate_or = flow.dump_model_state();
    if (tstates_or.ok() && mstate_or.ok()) {
      transform_states = tstates_or.move_value();
      model_state = mstate_or.move_value();
      has_snapshot = true;
    }
  }

  if (!has_snapshot && !has_fallback_train) {
    return Status::invalid_argument(
        "save requires either fitted-state snapshot support or a valid training batch fallback.");
  }

  if (metadata.created_unix_nanos == 0) {
    metadata.created_unix_nanos = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
  }

  BinaryWriter writer(path);
  if (!writer.ok()) return Status::invalid_argument("save failed to open output path.");
  writer.string(k_magic);
  writer.pod(metadata.schema_version);
  writer.string(metadata.producer);
  writer.string(metadata.producer_version);
  writer.pod(metadata.created_unix_nanos);
  write_model_spec(writer, flow.model_spec().value());
  const std::uint64_t stage_count = static_cast<std::uint64_t>(flow.transform_specs().size());
  writer.pod(stage_count);
  for (const auto& spec : flow.transform_specs()) {
    write_transform_spec(writer, spec);
  }
  writer.pod(static_cast<std::uint8_t>(flow.trained() ? 1U : 0U));
  writer.pod(static_cast<std::uint8_t>(has_snapshot ? 1U : 0U));
  if (has_snapshot) {
    const std::uint64_t ts_count = static_cast<std::uint64_t>(transform_states.size());
    writer.pod(ts_count);
    for (const auto& stage_state : transform_states) {
      write_series_vector(writer, stage_state);
    }
    write_series_vector(writer, model_state);
  }
  writer.pod(static_cast<std::uint8_t>(has_fallback_train ? 1U : 0U));
  write_train_options(writer, train_options);
  if (has_fallback_train) {
    write_labeled_batch(writer, train_batch);
  } else {
    write_labeled_batch(writer, LabeledBatch{});
  }
  if (!writer.ok()) return Status::internal("save failed while writing bundle bytes.");
  return Status::success();
}

StatusOr<FlowBundle> load(const std::string& path) {
  BinaryReader reader(path);
  if (!reader.ok()) return StatusOr<FlowBundle>(Status::invalid_argument("load failed to open input path."));

  std::string magic{};
  if (!reader.string(magic) || magic != k_magic) {
    return StatusOr<FlowBundle>(Status::invalid_argument("load encountered invalid bundle magic."));
  }

  FlowBundle bundle{};
  if (!reader.pod(bundle.metadata.schema_version)
      || !reader.string(bundle.metadata.producer)
      || !reader.string(bundle.metadata.producer_version)
      || !reader.pod(bundle.metadata.created_unix_nanos)) {
    return StatusOr<FlowBundle>(Status::internal("load failed while reading bundle metadata."));
  }
  if (bundle.metadata.schema_version != k_flow_bundle_schema) {
    return StatusOr<FlowBundle>(
        Status::invalid_argument("load schema version mismatch with current reader."));
  }

  if (!read_model_spec(reader, bundle.model)) {
    return StatusOr<FlowBundle>(Status::internal("load failed while reading model spec."));
  }
  std::uint64_t stage_count = 0;
  if (!reader.pod(stage_count)) {
    return StatusOr<FlowBundle>(Status::internal("load failed while reading transform stage count."));
  }
  bundle.transforms.resize(static_cast<std::size_t>(stage_count));
  for (std::size_t i = 0; i < bundle.transforms.size(); ++i) {
    if (!read_transform_spec(reader, bundle.transforms[i])) {
      return StatusOr<FlowBundle>(Status::internal("load failed while reading transform stage."));
    }
  }
  {
    std::uint8_t trained = 0;
    std::uint8_t has_snapshot = 0;
    if (!reader.pod(trained) || !reader.pod(has_snapshot)) {
      return StatusOr<FlowBundle>(Status::internal("load failed while reading snapshot flags."));
    }
    bundle.trained = trained != 0;
    bundle.has_snapshot = has_snapshot != 0;
    if (bundle.has_snapshot) {
      std::uint64_t ts_count = 0;
      if (!reader.pod(ts_count)) {
        return StatusOr<FlowBundle>(Status::internal("load failed while reading transform snapshot count."));
      }
      bundle.transform_states.resize(static_cast<std::size_t>(ts_count));
      for (std::size_t i = 0; i < bundle.transform_states.size(); ++i) {
        if (!read_series_vector(reader, bundle.transform_states[i])) {
          return StatusOr<FlowBundle>(Status::internal("load failed while reading transform snapshot state."));
        }
      }
      if (!read_series_vector(reader, bundle.model_state)) {
        return StatusOr<FlowBundle>(Status::internal("load failed while reading model snapshot state."));
      }
    }
    std::uint8_t has_fallback_train = 0;
    if (!reader.pod(has_fallback_train)) {
      return StatusOr<FlowBundle>(Status::internal("load failed while reading fallback-train flag."));
    }
    if (!read_train_options(reader, bundle.train_options)) {
      return StatusOr<FlowBundle>(Status::internal("load failed while reading train options."));
    }
    if (has_fallback_train != 0) {
      if (!read_labeled_batch(reader, bundle.train_batch)) {
        return StatusOr<FlowBundle>(Status::internal("load failed while reading train batch."));
      }
    } else {
      LabeledBatch ignored{};
      if (!read_labeled_batch(reader, ignored)) {
        return StatusOr<FlowBundle>(Status::internal("load failed while reading placeholder train batch."));
      }
      bundle.train_batch = LabeledBatch{};
    }
  }
  return StatusOr<FlowBundle>(std::move(bundle));
}

StatusOr<Flow> restore(const FlowBundle& bundle) {
  StatusOr<Flow> flow_or = make_flow(bundle.model);
  if (!flow_or.ok()) return StatusOr<Flow>(flow_or.status());
  Flow flow = flow_or.move_value();
  for (const auto& spec : bundle.transforms) {
    Status stage_status = add_transform_from_spec(flow, spec);
    if (!stage_status.ok()) return StatusOr<Flow>(stage_status);
  }

  if (bundle.has_snapshot && bundle.trained) {
    Status tstate = flow.load_transform_states(bundle.transform_states);
    if (tstate.ok()) {
      Status mstate = flow.load_model_state(bundle.model_state);
      if (mstate.ok()) {
        flow.mark_trained(true);
        return StatusOr<Flow>(std::move(flow));
      }
    }
  }

  Status batch_status = validate_labeled(bundle.train_batch);
  if (!batch_status.ok()) {
    return StatusOr<Flow>(Status::invalid_argument(
        "restore could not load fitted snapshots and bundle has no valid train-batch fallback."));
  }
  Status learn_status = flow.learn(bundle.train_batch, bundle.train_options);
  if (!learn_status.ok()) return StatusOr<Flow>(learn_status);
  return StatusOr<Flow>(std::move(flow));
}

StatusOr<Flow> load_and_restore(const std::string& path) {
  StatusOr<FlowBundle> bundle_or = load(path);
  if (!bundle_or.ok()) return StatusOr<Flow>(bundle_or.status());
  return restore(bundle_or.value());
}

} // namespace pluss::ai::nodej::io
