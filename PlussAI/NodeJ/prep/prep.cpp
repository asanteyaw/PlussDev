#include "pluss/ai/nodej/prep.h"
#include "pluss/table/table.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::ai::nodej::prep {
namespace {

struct DenseMatrix {
  std::vector<double> values{};
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  ObservationLayout layout = ObservationLayout::Rows;
  Platform original_platform = pluss::pCPU;
  ScalarType original_dtype = ScalarType::Float64;
};

[[nodiscard]] Config prep_work_config() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::size_t offset(const DenseMatrix& matrix, std::int64_t sample_idx, std::int64_t feature_idx) {
  if (matrix.layout == ObservationLayout::Rows) {
    return static_cast<std::size_t>(sample_idx * matrix.feature_count + feature_idx);
  }
  return static_cast<std::size_t>(feature_idx * matrix.sample_count + sample_idx);
}

[[nodiscard]] StatusOr<DenseMatrix> to_dense(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseMatrix>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseMatrix>(Status::shape_mismatch(std::string(where) + " requires rank-2 feature matrices."));
  }

  DenseMatrix matrix{};
  matrix.layout = batch.layout;
  matrix.sample_count = batch.sample_count();
  matrix.feature_count = batch.feature_count();
  matrix.original_platform = batch.x.platform();
  matrix.original_dtype = batch.x.scalar_type();

  const Series x64 = batch.x.to(ScalarType::Float64);
  matrix.values = x64.to_vec<double>();
  if (static_cast<std::int64_t>(matrix.values.size()) != matrix.sample_count * matrix.feature_count) {
    return StatusOr<DenseMatrix>(
        Status::shape_mismatch(std::string(where) + " expects contiguous 2D feature storage with matching element count."));
  }
  return StatusOr<DenseMatrix>(std::move(matrix));
}

[[nodiscard]] Series from_dense(const DenseMatrix& matrix, std::vector<double> values, std::int64_t out_feature_count) {
  std::vector<std::int64_t> shape =
      matrix.layout == ObservationLayout::Rows
          ? std::vector<std::int64_t>{matrix.sample_count, out_feature_count}
          : std::vector<std::int64_t>{out_feature_count, matrix.sample_count};
  Series out = pluss::make_series(std::move(values), shape, prep_work_config());
  return out.to(matrix.original_platform, matrix.original_dtype);
}

[[nodiscard]] Series from_dense_like(const DenseMatrix& matrix, std::vector<double> values) {
  return from_dense(matrix, std::move(values), matrix.feature_count);
}

[[nodiscard]] std::vector<double> feature_non_missing(const DenseMatrix& matrix, std::int64_t feature_idx) {
  std::vector<double> values{};
  values.reserve(static_cast<std::size_t>(matrix.sample_count));
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    const double v = matrix.values[offset(matrix, i, feature_idx)];
    if (!std::isnan(v)) values.push_back(v);
  }
  return values;
}

[[nodiscard]] double quantile(std::vector<double> values, double q) {
  std::sort(values.begin(), values.end());
  if (values.size() == 1) return values.front();

  const double pos = q * static_cast<double>(values.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo == hi) return values[lo];
  const double t = pos - static_cast<double>(lo);
  return values[lo] * (1.0 - t) + values[hi] * t;
}

[[nodiscard]] double most_frequent(const std::vector<double>& values, double fallback) {
  if (values.empty()) return fallback;

  std::unordered_map<std::uint64_t, std::int64_t> counts{};
  counts.reserve(values.size());

  for (double value : values) {
    if (value == 0.0) value = 0.0; // normalize signed zero
    const std::uint64_t key = std::bit_cast<std::uint64_t>(value);
    ++counts[key];
  }

  std::int64_t best_count = -1;
  double best_value = fallback;
  for (const auto& [key, count] : counts) {
    const double value = std::bit_cast<double>(key);
    if (count > best_count || (count == best_count && value < best_value)) {
      best_count = count;
      best_value = value;
    }
  }
  return best_value;
}

[[nodiscard]] Status validate_range(double low, double high, const char* where) {
  if (!std::isfinite(low) || !std::isfinite(high) || !(high > low)) {
    return Status::invalid_argument(std::string(where) + " requires finite bounds with upper > lower.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_quantiles(double low, double high, const char* where) {
  if (!std::isfinite(low) || !std::isfinite(high) || !(low >= 0.0 && high <= 1.0 && low < high)) {
    return Status::invalid_argument(std::string(where) + " requires quantiles in [0,1] with lower < upper.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_floor(double value, const char* where) {
  if (!std::isfinite(value) || !(value > 0.0)) {
    return Status::invalid_argument(std::string(where) + " requires a positive finite floor.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_max_categories(std::int64_t value, const char* where) {
  if (value < 0) {
    return Status::invalid_argument(std::string(where) + " requires max_categories >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_unit_interval(double value, const char* where) {
  if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
    return Status::invalid_argument(std::string(where) + " requires a finite value in [0,1].");
  }
  return Status::success();
}

[[nodiscard]] std::uint64_t key_for_value(double value) {
  if (value == 0.0) value = 0.0; // normalize signed zero
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t out_offset(ObservationLayout layout,
                                     std::int64_t sample_count,
                                     std::int64_t encoded_features,
                                     std::int64_t sample_idx,
                                     std::int64_t encoded_feature_idx) {
  if (layout == ObservationLayout::Rows) {
    return static_cast<std::size_t>(sample_idx * encoded_features + encoded_feature_idx);
  }
  return static_cast<std::size_t>(encoded_feature_idx * sample_count + sample_idx);
}

[[nodiscard]] Status validate_trained_input(bool trained,
                                            ObservationLayout expected_layout,
                                            std::int64_t expected_features,
                                            const FeatureBatch& batch,
                                            const char* where) {
  if (!trained) return Status::precondition(std::string(where) + " requires learn() before map().");
  Status check = validate_features(batch);
  if (!check.ok()) return check;
  if (batch.layout != expected_layout) {
    return Status::shape_mismatch(std::string(where) + " layout mismatch with fitted transformer.");
  }
  if (batch.feature_count() != expected_features) {
    return Status::shape_mismatch(std::string(where) + " feature count mismatch with fitted transformer.");
  }
  return Status::success();
}

[[nodiscard]] Config index_config_on(::pluss::Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] Series make_index_series(std::vector<std::int64_t> ids, ::pluss::Platform platform) {
  return pluss::make_series(std::move(ids), {static_cast<std::int64_t>(ids.size())}, index_config_on(platform));
}

[[nodiscard]] StatusOr<std::vector<std::int64_t>> normalized_feature_route(const std::vector<std::int64_t>& route,
                                                                           std::int64_t feature_count,
                                                                           std::int64_t stage_index,
                                                                           const char* where) {
  if (route.empty()) {
    return StatusOr<std::vector<std::int64_t>>(
        Status::invalid_argument(std::string(where) + " stage " + std::to_string(stage_index)
                                 + " requires a non-empty feature route."));
  }
  std::vector<std::int64_t> sorted = route;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
    return StatusOr<std::vector<std::int64_t>>(
        Status::invalid_argument(std::string(where) + " stage " + std::to_string(stage_index)
                                 + " route contains duplicate feature indices."));
  }
  for (std::int64_t idx : sorted) {
    if (idx < 0 || idx >= feature_count) {
      return StatusOr<std::vector<std::int64_t>>(
          Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                                 + " route index out of range for current feature count."));
    }
  }
  return StatusOr<std::vector<std::int64_t>>(std::move(sorted));
}

[[nodiscard]] StatusOr<FeatureBatch> select_routed_features(const FeatureBatch& batch,
                                                            const std::vector<std::int64_t>& route,
                                                            std::int64_t stage_index,
                                                            const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  if (route.empty()) {
    return StatusOr<FeatureBatch>(
        Status::invalid_argument(std::string(where) + " stage " + std::to_string(stage_index) + " has an empty route."));
  }

  FeatureBatch out = batch;
  const Series idx = make_index_series(route, batch.x.platform());
  if (batch.layout == ObservationLayout::Rows) {
    out.x = batch.x.index_select(1, idx);
  } else {
    out.x = batch.x.index_select(0, idx);
  }
  check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(
        Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                               + " could not select routed feature subset."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

[[nodiscard]] StatusOr<FeatureBatch> mapped_batch_checked(const Series& mapped,
                                                          ObservationLayout layout,
                                                          std::int64_t expected_samples,
                                                          std::int64_t stage_index,
                                                          const char* where) {
  FeatureBatch out{
      .x = mapped,
      .layout = layout,
  };
  Status check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " emitted invalid feature shape."));
  }
  if (out.sample_count() != expected_samples) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " changed sample count."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

[[nodiscard]] StatusOr<FeatureBatch> single_feature_batch_from_thread(const pluss::table::Thread& thread,
                                                                       const char* where) {
  if (!thread.defined()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument(std::string(where) + " requires a defined Thread."));
  }
  const std::int64_t rows = thread.size();
  if (rows <= 0) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(std::string(where) + " requires at least one row."));
  }

  Series feature = Series::undefined();
  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series: {
      feature = thread.as_series().to(ScalarType::Float64);
      break;
    }
    case pluss::table::ThreadKind::Note: {
      constexpr std::uint64_t fnv_offset = 1469598103934665603ULL;
      constexpr std::uint64_t fnv_prime = 1099511628211ULL;
      std::vector<double> hashed(static_cast<std::size_t>(rows), std::numeric_limits<double>::quiet_NaN());
      for (std::int64_t i = 0; i < rows; ++i) {
        if (!thread.is_valid(i)) continue;
        std::uint64_t h = fnv_offset;
        const std::string_view token = thread.as_note().at(i);
        for (unsigned char ch : token) {
          h ^= static_cast<std::uint64_t>(ch);
          h *= fnv_prime;
        }
        const std::uint32_t lane = static_cast<std::uint32_t>((h ^ (h >> 32)) & 0x7FFFFFFFULL);
        hashed[static_cast<std::size_t>(i)] = static_cast<double>(lane);
      }
      feature = pluss::make_series(std::move(hashed), {rows}, prep_work_config());
      break;
    }
    case pluss::table::ThreadKind::Timeline:
    case pluss::table::ThreadKind::TimeIndex: {
      const pluss::table::Thread casted = thread.cast(pluss::table::ThreadCastTarget::Series);
      feature = casted.as_series().to(ScalarType::Float64);
      break;
    }
    default: {
      return StatusOr<FeatureBatch>(
          Status::invalid_argument(std::string(where) + " supports only Series/Note/Timeline/TimeIndex Thread kinds."));
    }
  }

  if (!feature.defined() || feature.numel() != rows) {
    return StatusOr<FeatureBatch>(
        Status::shape_mismatch(std::string(where) + " expected a 1D-compatible Thread column."));
  }
  feature = feature.reshape({rows});

  if (thread.has_validity_mask()) {
    auto host = feature.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    bool touched = false;
    for (std::int64_t i = 0; i < rows; ++i) {
      if (!thread.is_valid(i)) {
        host[static_cast<std::size_t>(i)] = std::numeric_limits<double>::quiet_NaN();
        touched = true;
      }
    }
    if (touched) {
      const Series fixed = pluss::make_series(std::move(host), {rows}, prep_work_config());
      feature = fixed.to(feature.platform(), ScalarType::Float64);
    }
  }

  FeatureBatch batch{
      .x = feature.reshape({rows, 1}),
      .layout = ObservationLayout::Rows,
  };
  return StatusOr<FeatureBatch>(std::move(batch));
}

[[nodiscard]] Status validate_neighbors(std::int64_t value, const char* where) {
  if (value <= 0) {
    return Status::invalid_argument(std::string(where) + " requires neighbors > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_rounds(std::int64_t value, const char* where) {
  if (value <= 0) {
    return Status::invalid_argument(std::string(where) + " requires rounds > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_grid(std::int64_t value, const char* where) {
  if (value < 2) {
    return Status::invalid_argument(std::string(where) + " requires lambda_grid >= 2.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_estimators(std::int64_t value, const char* where) {
  if (value <= 0) {
    return Status::invalid_argument(std::string(where) + " requires estimators > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_max_samples(std::int64_t value, const char* where) {
  if (value <= 0) {
    return Status::invalid_argument(std::string(where) + " requires max_samples > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_max_features_ratio(double value, const char* where) {
  if (!std::isfinite(value) || !(value > 0.0 && value <= 1.0)) {
    return Status::invalid_argument(std::string(where) + " requires max_features_ratio in (0, 1].");
  }
  return Status::success();
}

[[nodiscard]] Status validate_contamination(double value, const char* where) {
  if (!std::isfinite(value) || !(value > 0.0 && value <= 0.5)) {
    return Status::invalid_argument(std::string(where) + " requires contamination in (0, 0.5].");
  }
  return Status::success();
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
  const std::uint64_t rd_seed = (static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd());
  return splitmix64(rd_seed ^ random.seed);
}

template <class URBG>
void shuffle_indices(std::vector<std::int64_t>& indices, URBG& rng) {
  if (indices.size() < 2) return;
  for (std::size_t i = indices.size(); i > 1; --i) {
    std::uniform_int_distribution<std::size_t> dist(0, i - 1);
    const std::size_t j = dist(rng);
    std::swap(indices[i - 1], indices[j]);
  }
}

class NumpyRandomState final {
 public:
  explicit NumpyRandomState(std::uint32_t seed) : mt_(seed) {}

  [[nodiscard]] std::uint32_t rand_u32() { return mt_(); }

  [[nodiscard]] double rand_double01() {
    // Match NumPy RandomState.random_sample implementation (randomkit rk_double).
    // Consumes two uint32 draws to produce one uniform [0,1) double.
    const std::uint64_t a = static_cast<std::uint64_t>(rand_u32() >> 5);
    const std::uint64_t b = static_cast<std::uint64_t>(rand_u32() >> 6);
    return (static_cast<double>(a) * 67108864.0 + static_cast<double>(b)) * (1.0 / 9007199254740992.0);
  }

  [[nodiscard]] std::int64_t randint_exclusive(std::int64_t high_exclusive) {
    if (high_exclusive <= 0) return 0;
    return static_cast<std::int64_t>(rk_interval_(static_cast<std::uint32_t>(high_exclusive - 1)));
  }

  [[nodiscard]] std::int64_t randint_range(std::int64_t low, std::int64_t high_exclusive) {
    return low + randint_exclusive(high_exclusive - low);
  }

 private:
  [[nodiscard]] std::uint32_t rk_interval_(std::uint32_t max_inclusive) {
    // Match NumPy RandomState randint path (randomkit rk_interval).
    std::uint32_t mask = max_inclusive;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;
    for (;;) {
      const std::uint32_t value = rand_u32() & mask;
      if (value <= max_inclusive) return value;
    }
  }

  std::mt19937 mt_;
};

[[nodiscard]] std::uint32_t sklearn_rand_r(std::uint32_t& seed) {
  // Match sklearn.utils._random.our_rand_r
  constexpr std::uint32_t kDefaultSeed = 1u;
  constexpr std::uint32_t kRandRMax = 2147483647u;
  if (seed == 0u) seed = kDefaultSeed;
  seed ^= static_cast<std::uint32_t>(seed << 13);
  seed ^= static_cast<std::uint32_t>(seed >> 17);
  seed ^= static_cast<std::uint32_t>(seed << 5);
  return seed % (static_cast<std::uint64_t>(kRandRMax) + 1u);
}

[[nodiscard]] std::int64_t sklearn_rand_int(std::int64_t low,
                                            std::int64_t high_exclusive,
                                            std::uint32_t& seed) {
  if (high_exclusive <= low) return low;
  const auto span = static_cast<std::uint64_t>(high_exclusive - low);
  return low + static_cast<std::int64_t>(sklearn_rand_r(seed) % span);
}

[[nodiscard]] double sklearn_rand_uniform(double low, double high, std::uint32_t& seed) {
  constexpr double kRandRMax = 2147483647.0;
  return ((high - low) * static_cast<double>(sklearn_rand_r(seed)) / kRandRMax) + low;
}

[[nodiscard]] std::vector<std::int64_t> random_permutation_prefix(std::int64_t n_population,
                                                                  std::int64_t n_samples,
                                                                  NumpyRandomState& rng) {
  std::vector<std::int64_t> out(static_cast<std::size_t>(n_population));
  std::iota(out.begin(), out.end(), std::int64_t{0});
  for (std::int64_t i = n_population - 1; i > 0; --i) {
    const std::int64_t j = rng.randint_exclusive(i + 1);
    std::swap(out[static_cast<std::size_t>(i)], out[static_cast<std::size_t>(j)]);
  }
  out.resize(static_cast<std::size_t>(n_samples));
  return out;
}

[[nodiscard]] std::vector<std::int64_t> sample_without_replacement(std::int64_t n_population,
                                                                   std::int64_t n_samples,
                                                                   NumpyRandomState& rng) {
  n_samples = std::clamp<std::int64_t>(n_samples, 0, n_population);
  if (n_samples <= 0) return {};
  if (n_samples >= n_population) {
    std::vector<std::int64_t> all(static_cast<std::size_t>(n_population));
    std::iota(all.begin(), all.end(), std::int64_t{0});
    return all;
  }

  // Match sklearn.utils.random.sample_without_replacement(..., method="auto")
  const double ratio = n_population == 0 ? 1.0 : (static_cast<double>(n_samples) / static_cast<double>(n_population));
  if (ratio > 0.01 && ratio < 0.99) {
    return random_permutation_prefix(n_population, n_samples, rng);
  }

  if (ratio < 0.2) {
    std::unordered_set<std::int64_t> selected{};
    selected.reserve(static_cast<std::size_t>(n_samples * 2));
    std::vector<std::int64_t> out{};
    out.reserve(static_cast<std::size_t>(n_samples));
    for (std::int64_t i = 0; i < n_samples; ++i) {
      std::int64_t j = rng.randint_exclusive(n_population);
      while (selected.find(j) != selected.end()) {
        j = rng.randint_exclusive(n_population);
      }
      selected.insert(j);
      out.push_back(j);
    }
    return out;
  }

  // Reservoir sampling branch.
  std::vector<std::int64_t> out(static_cast<std::size_t>(n_samples));
  for (std::int64_t i = 0; i < n_samples; ++i) {
    out[static_cast<std::size_t>(i)] = i;
  }
  for (std::int64_t i = n_samples; i < n_population; ++i) {
    const std::int64_t j = rng.randint_exclusive(i + 1);
    if (j < n_samples) {
      out[static_cast<std::size_t>(j)] = i;
    }
  }
  return out;
}

[[nodiscard]] std::int64_t randint_max_int(NumpyRandomState& rng) {
  // sklearn BaseBagging uses MAX_INT = np.iinfo(np.int32).max as exclusive upper bound.
  constexpr std::int64_t kMaxIntExclusive = 2147483647LL;
  return rng.randint_exclusive(kMaxIntExclusive);
}

[[nodiscard]] double average_isolation_path_length(std::int64_t n) {
  if (n <= 1) return 0.0;
  if (n == 2) return 1.0;
  constexpr double gamma = 0.5772156649015328606; // Euler-Mascheroni
  const double nd = static_cast<double>(n);
  return 2.0 * (std::log(nd - 1.0) + gamma) - (2.0 * (nd - 1.0) / nd);
}

[[nodiscard]] double normal_quantile(double p) {
  // Acklam inverse-normal approximation.
  constexpr double a1 = -3.969683028665376e+01;
  constexpr double a2 = 2.209460984245205e+02;
  constexpr double a3 = -2.759285104469687e+02;
  constexpr double a4 = 1.383577518672690e+02;
  constexpr double a5 = -3.066479806614716e+01;
  constexpr double a6 = 2.506628277459239e+00;

  constexpr double b1 = -5.447609879822406e+01;
  constexpr double b2 = 1.615858368580409e+02;
  constexpr double b3 = -1.556989798598866e+02;
  constexpr double b4 = 6.680131188771972e+01;
  constexpr double b5 = -1.328068155288572e+01;

  constexpr double c1 = -7.784894002430293e-03;
  constexpr double c2 = -3.223964580411365e-01;
  constexpr double c3 = -2.400758277161838e+00;
  constexpr double c4 = -2.549732539343734e+00;
  constexpr double c5 = 4.374664141464968e+00;
  constexpr double c6 = 2.938163982698783e+00;

  constexpr double d1 = 7.784695709041462e-03;
  constexpr double d2 = 3.224671290700398e-01;
  constexpr double d3 = 2.445134137142996e+00;
  constexpr double d4 = 3.754408661907416e+00;

  constexpr double p_low = 0.02425;
  constexpr double p_high = 1.0 - p_low;

  if (p <= 0.0) return -std::numeric_limits<double>::infinity();
  if (p >= 1.0) return std::numeric_limits<double>::infinity();

  if (p < p_low) {
    const double q = std::sqrt(-2.0 * std::log(p));
    return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
           / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
  }
  if (p > p_high) {
    const double q = std::sqrt(-2.0 * std::log(1.0 - p));
    return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
           / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
  }

  const double q = p - 0.5;
  const double r = q * q;
  return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q
         / (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

[[nodiscard]] double yeo_johnson_value(double x, double lambda) {
  if (x >= 0.0) {
    if (std::abs(lambda) <= 1e-12) return std::log1p(x);
    return (std::pow(x + 1.0, lambda) - 1.0) / lambda;
  }
  if (std::abs(lambda - 2.0) <= 1e-12) return -std::log1p(-x);
  return -(std::pow(1.0 - x, 2.0 - lambda) - 1.0) / (2.0 - lambda);
}

[[nodiscard]] double box_cox_value(double x, double lambda) {
  if (std::abs(lambda) <= 1e-12) return std::log(x);
  return (std::pow(x, lambda) - 1.0) / lambda;
}

[[nodiscard]] double feature_cdf(const std::vector<double>& sorted, double value) {
  if (sorted.empty()) return std::numeric_limits<double>::quiet_NaN();
  if (sorted.size() == 1) return 0.5;
  const auto lo_it = std::lower_bound(sorted.begin(), sorted.end(), value);
  const auto hi_it = std::upper_bound(sorted.begin(), sorted.end(), value);
  const std::int64_t lo = static_cast<std::int64_t>(std::distance(sorted.begin(), lo_it));
  const std::int64_t hi = static_cast<std::int64_t>(std::distance(sorted.begin(), hi_it));
  const double rank = 0.5 * static_cast<double>(lo + hi);
  return std::clamp(rank / static_cast<double>(sorted.size()), 0.0, 1.0);
}

[[nodiscard]] bool solve_linear_system(std::vector<double>& a, std::vector<double>& b, std::int64_t n) {
  for (std::int64_t col = 0; col < n; ++col) {
    std::int64_t pivot = col;
    double best = std::abs(a[static_cast<std::size_t>(col * n + col)]);
    for (std::int64_t r = col + 1; r < n; ++r) {
      const double v = std::abs(a[static_cast<std::size_t>(r * n + col)]);
      if (v > best) {
        best = v;
        pivot = r;
      }
    }
    if (!(best > 0.0) || !std::isfinite(best)) return false;
    if (pivot != col) {
      for (std::int64_t c = col; c < n; ++c) {
        std::swap(a[static_cast<std::size_t>(col * n + c)], a[static_cast<std::size_t>(pivot * n + c)]);
      }
      std::swap(b[static_cast<std::size_t>(col)], b[static_cast<std::size_t>(pivot)]);
    }

    const double diag = a[static_cast<std::size_t>(col * n + col)];
    for (std::int64_t r = col + 1; r < n; ++r) {
      const double factor = a[static_cast<std::size_t>(r * n + col)] / diag;
      if (!std::isfinite(factor)) return false;
      a[static_cast<std::size_t>(r * n + col)] = 0.0;
      for (std::int64_t c = col + 1; c < n; ++c) {
        a[static_cast<std::size_t>(r * n + c)] -= factor * a[static_cast<std::size_t>(col * n + c)];
      }
      b[static_cast<std::size_t>(r)] -= factor * b[static_cast<std::size_t>(col)];
    }
  }

  for (std::int64_t i = n - 1; i >= 0; --i) {
    double sum = b[static_cast<std::size_t>(i)];
    for (std::int64_t c = i + 1; c < n; ++c) {
      sum -= a[static_cast<std::size_t>(i * n + c)] * b[static_cast<std::size_t>(c)];
    }
    const double diag = a[static_cast<std::size_t>(i * n + i)];
    if (!(std::abs(diag) > 0.0) || !std::isfinite(diag)) return false;
    b[static_cast<std::size_t>(i)] = sum / diag;
    if (!std::isfinite(b[static_cast<std::size_t>(i)])) return false;
  }
  return true;
}

[[nodiscard]] std::vector<double> fit_ridge_coefficients(const DenseMatrix& matrix,
                                                         const std::vector<double>& target,
                                                         std::int64_t skip_feature,
                                                         const std::vector<std::int64_t>& rows,
                                                         double ridge) {
  const std::int64_t p = matrix.feature_count - 1;
  if (p <= 0 || rows.empty()) return {};
  const std::int64_t d = p + 1; // intercept + p features
  std::vector<double> gram(static_cast<std::size_t>(d * d), 0.0);
  std::vector<double> rhs(static_cast<std::size_t>(d), 0.0);

  for (std::int64_t ridx = 0; ridx < static_cast<std::int64_t>(rows.size()); ++ridx) {
    const std::int64_t i = rows[static_cast<std::size_t>(ridx)];
    std::vector<double> xrow(static_cast<std::size_t>(d), 1.0);
    std::int64_t pos = 1;
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      if (f == skip_feature) continue;
      xrow[static_cast<std::size_t>(pos++)] = matrix.values[offset(matrix, i, f)];
    }
    const double y = target[static_cast<std::size_t>(ridx)];
    for (std::int64_t r = 0; r < d; ++r) {
      rhs[static_cast<std::size_t>(r)] += xrow[static_cast<std::size_t>(r)] * y;
      for (std::int64_t c = 0; c < d; ++c) {
        gram[static_cast<std::size_t>(r * d + c)] += xrow[static_cast<std::size_t>(r)] * xrow[static_cast<std::size_t>(c)];
      }
    }
  }
  for (std::int64_t j = 1; j < d; ++j) {
    gram[static_cast<std::size_t>(j * d + j)] += ridge;
  }

  if (!solve_linear_system(gram, rhs, d)) return {};
  return rhs;
}

[[nodiscard]] double predict_ridge_row(const DenseMatrix& matrix,
                                       std::int64_t row,
                                       std::int64_t skip_feature,
                                       const std::vector<double>& coef,
                                       double intercept) {
  if (coef.empty()) return intercept;
  double pred = intercept;
  std::int64_t pos = 0;
  for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
    if (f == skip_feature) continue;
    pred += coef[static_cast<std::size_t>(pos++)] * matrix.values[offset(matrix, row, f)];
  }
  return pred;
}

void collect_combinations_repetition(std::int64_t start,
                                     std::int64_t depth,
                                     std::int64_t feature_count,
                                     std::vector<std::int64_t>& cur,
                                     std::vector<std::vector<std::int64_t>>& out) {
  if (depth == 0) {
    out.push_back(cur);
    return;
  }
  for (std::int64_t i = start; i < feature_count; ++i) {
    cur.push_back(i);
    collect_combinations_repetition(i, depth - 1, feature_count, cur, out);
    cur.pop_back();
  }
}

void collect_combinations_no_repetition(std::int64_t start,
                                        std::int64_t depth,
                                        std::int64_t feature_count,
                                        std::vector<std::int64_t>& cur,
                                        std::vector<std::vector<std::int64_t>>& out) {
  if (depth == 0) {
    out.push_back(cur);
    return;
  }
  for (std::int64_t i = start; i < feature_count; ++i) {
    cur.push_back(i);
    collect_combinations_no_repetition(i + 1, depth - 1, feature_count, cur, out);
    cur.pop_back();
  }
}

[[nodiscard]] Series state_f64(std::vector<double> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values),
                            {n},
                            prep_work_config());
}

[[nodiscard]] Series state_i64(std::vector<std::int64_t> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values),
                            {n},
                            Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
}

[[nodiscard]] std::vector<double> vec_f64(const Series& state) {
  if (!state.defined()) return {};
  return state.to(pluss::pCPU, ScalarType::Float64).reshape({state.numel()}).to_vec<double>();
}

[[nodiscard]] std::vector<std::int64_t> vec_i64(const Series& state) {
  if (!state.defined()) return {};
  return state.to(pluss::pCPU, ScalarType::Int64).reshape({state.numel()}).to_vec<std::int64_t>();
}

[[nodiscard]] std::vector<double> flatten_nested(const std::vector<std::vector<double>>& nested) {
  std::vector<double> out{};
  std::size_t total = 0;
  for (const auto& row : nested) total += row.size();
  out.reserve(total);
  for (const auto& row : nested) {
    out.insert(out.end(), row.begin(), row.end());
  }
  return out;
}

[[nodiscard]] std::vector<std::int64_t> nested_sizes(const std::vector<std::vector<double>>& nested) {
  std::vector<std::int64_t> out{};
  out.reserve(nested.size());
  for (const auto& row : nested) out.push_back(static_cast<std::int64_t>(row.size()));
  return out;
}

[[nodiscard]] std::vector<std::vector<double>> unflatten_nested(const std::vector<double>& flat,
                                                                const std::vector<std::int64_t>& sizes) {
  std::vector<std::vector<double>> out{};
  out.reserve(sizes.size());
  std::size_t offset_flat = 0;
  for (std::int64_t n : sizes) {
    const std::size_t count = static_cast<std::size_t>(std::max<std::int64_t>(0, n));
    const std::size_t end = std::min(flat.size(), offset_flat + count);
    out.emplace_back(flat.begin() + static_cast<std::ptrdiff_t>(offset_flat),
                     flat.begin() + static_cast<std::ptrdiff_t>(end));
    offset_flat = end;
  }
  return out;
}

[[nodiscard]] std::vector<std::vector<std::int64_t>> unflatten_nested_i64(const std::vector<std::int64_t>& flat,
                                                                           const std::vector<std::int64_t>& sizes) {
  std::vector<std::vector<std::int64_t>> out{};
  out.reserve(sizes.size());
  std::size_t offset_flat = 0;
  for (std::int64_t n : sizes) {
    const std::size_t count = static_cast<std::size_t>(std::max<std::int64_t>(0, n));
    const std::size_t end = std::min(flat.size(), offset_flat + count);
    out.emplace_back(flat.begin() + static_cast<std::ptrdiff_t>(offset_flat),
                     flat.begin() + static_cast<std::ptrdiff_t>(end));
    offset_flat = end;
  }
  return out;
}

} // namespace

FunctionTransformer::FunctionTransformer(SeriesMapFn map_fn, FunctionTransformerOptions options)
    : options_(options), map_fn_(std::move(map_fn)), learned_(map_fn_ != nullptr) {}

bool FunctionTransformer::trained() const noexcept { return map_fn_ != nullptr; }

Status FunctionTransformer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  if (!map_fn_) {
    return Status::invalid_argument("FunctionTransformer::learn requires a valid callable.");
  }
  if (options_.validate_input) {
    Status check = validate_features(batch);
    if (!check.ok()) return check;
  }
  layout_ = batch.layout;
  feature_count_ = batch.feature_count();
  learned_ = true;
  return Status::success();
}

StatusOr<Series> FunctionTransformer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  if (!map_fn_) {
    return StatusOr<Series>(Status::invalid_argument("FunctionTransformer::map requires a valid callable."));
  }
  if (options_.validate_input) {
    Status check = validate_features(batch);
    if (!check.ok()) return StatusOr<Series>(std::move(check));
  }

  StatusOr<Series> mapped_or = map_fn_(batch.x);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());

  FeatureBatch mapped{
      .x = mapped_or.move_value(),
      .layout = batch.layout,
  };
  Status check = validate_features(mapped);
  if (!check.ok()) {
    return StatusOr<Series>(Status::shape_mismatch("FunctionTransformer::map callable emitted invalid feature shape."));
  }
  if (options_.preserve_sample_count && mapped.sample_count() != batch.sample_count()) {
    return StatusOr<Series>(Status::shape_mismatch("FunctionTransformer::map callable changed sample count."));
  }
  return StatusOr<Series>(std::move(mapped.x));
}

StatusOr<Series> FunctionTransformer::learn_map(const FeatureBatch& batch,
                                                const TrainOptions& learn_options,
                                                const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

StatusOr<std::vector<Series>> FunctionTransformer::dump_state() const {
  return StatusOr<std::vector<Series>>(
      Status::not_implemented("FunctionTransformer::dump_state is unavailable for callable-based stages."));
}

Status FunctionTransformer::load_state(const std::vector<Series>& state) {
  (void)state;
  return Status::not_implemented("FunctionTransformer::load_state is unavailable for callable-based stages.");
}

FeatureTransformer::FeatureTransformer(FeatureRemainderPolicy remainder) : remainder_(remainder) {}

bool FeatureTransformer::trained() const noexcept { return trained_; }

Status FeatureTransformer::add_stage(std::unique_ptr<TransformerBase> transformer, std::vector<std::int64_t> feature_route) {
  std::vector<std::unique_ptr<TransformerBase>> pipeline{};
  pipeline.push_back(std::move(transformer));
  return add_pipeline(std::move(pipeline), std::move(feature_route));
}

Status FeatureTransformer::add_pipeline(std::vector<std::unique_ptr<TransformerBase>> transformers,
                                        std::vector<std::int64_t> feature_route) {
  if (transformers.empty()) {
    return Status::invalid_argument("FeatureTransformer::add_pipeline requires at least one transformer.");
  }
  for (std::size_t i = 0; i < transformers.size(); ++i) {
    if (!transformers[i]) {
      return Status::invalid_argument("FeatureTransformer::add_pipeline contains a null transformer.");
    }
  }
  if (feature_route.empty()) {
    return Status::invalid_argument("FeatureTransformer::add_pipeline requires a non-empty feature_route.");
  }
  stages_.push_back(Stage{
      .transformers = std::move(transformers),
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

void FeatureTransformer::clear_stages() noexcept {
  stages_.clear();
  trained_ = false;
  feature_count_ = 0;
  output_feature_count_ = 0;
}

std::int64_t FeatureTransformer::stage_count() const noexcept { return static_cast<std::int64_t>(stages_.size()); }

FeatureRemainderPolicy FeatureTransformer::remainder_policy() const noexcept { return remainder_; }

void FeatureTransformer::set_remainder_policy(FeatureRemainderPolicy policy) noexcept {
  remainder_ = policy;
  trained_ = false;
}

Status FeatureTransformer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  const InferOptions infer_options{
      .layout = batch.layout,
      .missing = options.missing,
      .unknown_categorical = options.unknown_categorical,
  };
  StatusOr<Series> mapped_or = learn_map(batch, options, infer_options);
  if (!mapped_or.ok()) return mapped_or.status();
  return Status::success();
}

StatusOr<Series> FeatureTransformer::map(const FeatureBatch& batch, const InferOptions& options) const {
  if (!trained_) {
    return StatusOr<Series>(Status::precondition("FeatureTransformer::map requires learn() before map()."));
  }
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<Series>(std::move(check));
  if (batch.layout != layout_) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::map layout mismatch with fitted transformer."));
  }
  if (batch.feature_count() != feature_count_) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::map feature count mismatch with fitted transformer."));
  }

  if (stages_.empty()) {
    if (remainder_ == FeatureRemainderPolicy::PassThrough) return StatusOr<Series>(batch.x);
    return StatusOr<Series>(
        Status::invalid_argument("FeatureTransformer::map has no stages and remainder policy is Drop."));
  }

  const std::int64_t expected_samples = batch.sample_count();
  std::vector<Series> outputs{};
  outputs.reserve(stages_.size() + 1);
  std::vector<bool> selected(static_cast<std::size_t>(feature_count_), false);

  for (std::size_t i = 0; i < stages_.size(); ++i) {
    const Stage& stage = stages_[i];
    if (stage.transformers.empty()) {
      return StatusOr<Series>(Status::internal("FeatureTransformer contains an empty stage pipeline."));
    }
    for (std::size_t j = 0; j < stage.transformers.size(); ++j) {
      if (!stage.transformers[j]) {
        return StatusOr<Series>(Status::internal("FeatureTransformer contains a null transformer in stage pipeline."));
      }
    }
    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        stage.feature_route, feature_count_, static_cast<std::int64_t>(i), "FeatureTransformer::map");
    if (!route_or.ok()) return StatusOr<Series>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();
    for (std::int64_t idx : route) selected[static_cast<std::size_t>(idx)] = true;

    StatusOr<FeatureBatch> routed_or = select_routed_features(batch, route, static_cast<std::int64_t>(i), "FeatureTransformer::map");
    if (!routed_or.ok()) return StatusOr<Series>(routed_or.status());

    FeatureBatch branch = routed_or.move_value();
    for (std::size_t j = 0; j < stage.transformers.size(); ++j) {
      StatusOr<Series> mapped_or = stage.transformers[j]->map(branch, options);
      if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
      StatusOr<FeatureBatch> mapped_batch_or = mapped_batch_checked(
          mapped_or.move_value(), batch.layout, expected_samples, static_cast<std::int64_t>(i), "FeatureTransformer::map");
      if (!mapped_batch_or.ok()) return StatusOr<Series>(mapped_batch_or.status());
      branch = mapped_batch_or.move_value();
    }
    outputs.push_back(branch.x);
  }

  if (remainder_ == FeatureRemainderPolicy::PassThrough) {
    std::vector<std::int64_t> remainder{};
    remainder.reserve(static_cast<std::size_t>(feature_count_));
    for (std::int64_t f = 0; f < feature_count_; ++f) {
      if (!selected[static_cast<std::size_t>(f)]) remainder.push_back(f);
    }
    if (!remainder.empty()) {
      StatusOr<FeatureBatch> routed_or =
          select_routed_features(batch, remainder, static_cast<std::int64_t>(stages_.size()), "FeatureTransformer::map");
      if (!routed_or.ok()) return StatusOr<Series>(routed_or.status());
      outputs.push_back(routed_or.move_value().x);
    }
  }

  if (outputs.empty()) {
    return StatusOr<Series>(
        Status::invalid_argument("FeatureTransformer::map produced no output features."));
  }
  Series merged = outputs.front();
  const std::int64_t axis = batch.layout == ObservationLayout::Rows ? 1 : 0;
  for (std::size_t i = 1; i < outputs.size(); ++i) {
    merged = merged.unite({outputs[i]}, axis);
  }

  FeatureBatch merged_batch{
      .x = merged,
      .layout = batch.layout,
  };
  check = validate_features(merged_batch);
  if (!check.ok()) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::map produced invalid feature shape."));
  }
  if (merged_batch.sample_count() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::map changed sample count."));
  }
  if (output_feature_count_ > 0 && merged_batch.feature_count() != output_feature_count_) {
    return StatusOr<Series>(
        Status::shape_mismatch("FeatureTransformer::map output feature count mismatch with fitted transformer."));
  }
  return StatusOr<Series>(std::move(merged_batch.x));
}

StatusOr<Series> FeatureTransformer::learn_map(const FeatureBatch& batch,
                                              const TrainOptions& learn_options,
                                              const InferOptions& infer_options) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<Series>(std::move(check));

  if (stages_.empty()) {
    if (remainder_ == FeatureRemainderPolicy::PassThrough) {
      trained_ = true;
      layout_ = batch.layout;
      feature_count_ = batch.feature_count();
      output_feature_count_ = feature_count_;
      return StatusOr<Series>(batch.x);
    }
    return StatusOr<Series>(
        Status::invalid_argument("FeatureTransformer::learn_map requires at least one stage when remainder policy is Drop."));
  }

  const std::int64_t expected_samples = batch.sample_count();
  std::vector<Series> outputs{};
  outputs.reserve(stages_.size() + 1);
  std::vector<bool> selected(static_cast<std::size_t>(batch.feature_count()), false);

  for (std::size_t i = 0; i < stages_.size(); ++i) {
    Stage& stage = stages_[i];
    if (stage.transformers.empty()) {
      return StatusOr<Series>(Status::internal("FeatureTransformer contains an empty stage pipeline."));
    }
    for (std::size_t j = 0; j < stage.transformers.size(); ++j) {
      if (!stage.transformers[j]) {
        return StatusOr<Series>(Status::internal("FeatureTransformer contains a null transformer in stage pipeline."));
      }
    }
    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        stage.feature_route, batch.feature_count(), static_cast<std::int64_t>(i), "FeatureTransformer::learn_map");
    if (!route_or.ok()) return StatusOr<Series>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();
    stage.feature_route = route;
    for (std::int64_t idx : route) selected[static_cast<std::size_t>(idx)] = true;

    StatusOr<FeatureBatch> routed_or =
        select_routed_features(batch, route, static_cast<std::int64_t>(i), "FeatureTransformer::learn_map");
    if (!routed_or.ok()) return StatusOr<Series>(routed_or.status());

    FeatureBatch branch = routed_or.move_value();
    for (std::size_t j = 0; j < stage.transformers.size(); ++j) {
      StatusOr<Series> mapped_or = stage.transformers[j]->learn_map(branch, learn_options, infer_options);
      if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
      StatusOr<FeatureBatch> mapped_batch_or = mapped_batch_checked(
          mapped_or.move_value(), batch.layout, expected_samples, static_cast<std::int64_t>(i), "FeatureTransformer::learn_map");
      if (!mapped_batch_or.ok()) return StatusOr<Series>(mapped_batch_or.status());
      branch = mapped_batch_or.move_value();
    }
    outputs.push_back(branch.x);
  }

  if (remainder_ == FeatureRemainderPolicy::PassThrough) {
    std::vector<std::int64_t> remainder{};
    remainder.reserve(static_cast<std::size_t>(batch.feature_count()));
    for (std::int64_t f = 0; f < batch.feature_count(); ++f) {
      if (!selected[static_cast<std::size_t>(f)]) remainder.push_back(f);
    }
    if (!remainder.empty()) {
      StatusOr<FeatureBatch> routed_or = select_routed_features(
          batch, remainder, static_cast<std::int64_t>(stages_.size()), "FeatureTransformer::learn_map");
      if (!routed_or.ok()) return StatusOr<Series>(routed_or.status());
      outputs.push_back(routed_or.move_value().x);
    }
  }

  if (outputs.empty()) {
    return StatusOr<Series>(
        Status::invalid_argument("FeatureTransformer::learn_map produced no output features."));
  }
  Series merged = outputs.front();
  const std::int64_t axis = batch.layout == ObservationLayout::Rows ? 1 : 0;
  for (std::size_t i = 1; i < outputs.size(); ++i) {
    merged = merged.unite({outputs[i]}, axis);
  }

  FeatureBatch merged_batch{
      .x = merged,
      .layout = batch.layout,
  };
  check = validate_features(merged_batch);
  if (!check.ok()) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::learn_map produced invalid feature shape."));
  }
  if (merged_batch.sample_count() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch("FeatureTransformer::learn_map changed sample count."));
  }

  trained_ = true;
  layout_ = batch.layout;
  feature_count_ = batch.feature_count();
  output_feature_count_ = merged_batch.feature_count();
  return StatusOr<Series>(std::move(merged_batch.x));
}

StatusOr<std::vector<Series>> FeatureTransformer::dump_state() const {
  std::vector<Series> state{};
  state.reserve(1 + stages_.size() * 3);
  state.push_back(state_f64({
      trained_ ? 1.0 : 0.0,
      static_cast<double>(static_cast<std::int64_t>(layout_)),
      static_cast<double>(feature_count_),
      static_cast<double>(output_feature_count_),
      static_cast<double>(static_cast<std::int64_t>(remainder_)),
      static_cast<double>(stage_count()),
  }));

  for (std::size_t i = 0; i < stages_.size(); ++i) {
    const Stage& stage = stages_[i];
    if (stage.transformers.empty()) {
      return StatusOr<std::vector<Series>>(Status::internal("FeatureTransformer contains an empty stage pipeline."));
    }
    state.push_back(state_i64(stage.feature_route));
    state.push_back(state_i64({static_cast<std::int64_t>(stage.transformers.size())}));
    for (std::size_t j = 0; j < stage.transformers.size(); ++j) {
      if (!stage.transformers[j]) {
        return StatusOr<std::vector<Series>>(Status::internal("FeatureTransformer contains a null transformer in stage pipeline."));
      }
      StatusOr<std::vector<Series>> stage_state_or = stage.transformers[j]->dump_state();
      if (!stage_state_or.ok()) {
        return StatusOr<std::vector<Series>>(
            Status::invalid_argument("FeatureTransformer::dump_state failed at stage "
                                     + std::to_string(i) + " pipeline index " + std::to_string(j)
                                     + ": " + stage_state_or.status().message));
      }
      const std::vector<Series> stage_state = stage_state_or.move_value();
      state.push_back(state_i64({static_cast<std::int64_t>(stage_state.size())}));
      state.insert(state.end(), stage_state.begin(), stage_state.end());
    }
  }
  return StatusOr<std::vector<Series>>(std::move(state));
}

Status FeatureTransformer::load_state(const std::vector<Series>& state) {
  if (state.empty()) {
    return Status::invalid_argument("FeatureTransformer::load_state expects at least metadata tensor.");
  }
  const std::vector<double> meta = vec_f64(state.front());
  if (meta.size() < 6) {
    return Status::invalid_argument("FeatureTransformer::load_state invalid metadata.");
  }
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  output_feature_count_ = static_cast<std::int64_t>(meta[3]);
  remainder_ = static_cast<FeatureRemainderPolicy>(static_cast<std::int64_t>(meta[4]));
  const std::int64_t expected_stages = static_cast<std::int64_t>(meta[5]);
  if (expected_stages != static_cast<std::int64_t>(stages_.size())) {
    return Status::invalid_argument(
        "FeatureTransformer::load_state stage count mismatch. Ensure the same stage graph is constructed before load_state().");
  }

  std::size_t cursor = 1;
  for (std::size_t i = 0; i < stages_.size(); ++i) {
    Stage& stage = stages_[i];
    if (stage.transformers.empty()) {
      return Status::internal("FeatureTransformer contains an empty stage pipeline.");
    }
    if (cursor >= state.size()) {
      return Status::invalid_argument("FeatureTransformer::load_state truncated while reading stage route.");
    }
    stage.feature_route = vec_i64(state[cursor++]);
    if (cursor >= state.size()) {
      return Status::invalid_argument("FeatureTransformer::load_state truncated while reading stage pipeline size.");
    }
    const std::vector<std::int64_t> pipeline_meta = vec_i64(state[cursor++]);
    if (pipeline_meta.empty() || pipeline_meta.front() < 0) {
      return Status::invalid_argument("FeatureTransformer::load_state encountered invalid stage pipeline size.");
    }
    const std::size_t pipeline_count = static_cast<std::size_t>(pipeline_meta.front());
    if (pipeline_count != stage.transformers.size()) {
      return Status::invalid_argument("FeatureTransformer::load_state stage pipeline size mismatch.");
    }
    for (std::size_t j = 0; j < pipeline_count; ++j) {
      if (!stage.transformers[j]) {
        return Status::internal("FeatureTransformer contains a null transformer in stage pipeline.");
      }
      if (cursor >= state.size()) {
        return Status::invalid_argument("FeatureTransformer::load_state truncated while reading transformer state size.");
      }
      const std::vector<std::int64_t> count_meta = vec_i64(state[cursor++]);
      if (count_meta.empty() || count_meta.front() < 0) {
        return Status::invalid_argument("FeatureTransformer::load_state encountered invalid transformer state size.");
      }
      const std::size_t state_count = static_cast<std::size_t>(count_meta.front());
      if (cursor + state_count > state.size()) {
        return Status::invalid_argument("FeatureTransformer::load_state truncated while reading transformer state payload.");
      }
      std::vector<Series> payload{};
      payload.reserve(state_count);
      for (std::size_t k = 0; k < state_count; ++k) {
        payload.push_back(state[cursor++]);
      }
      Status s = stage.transformers[j]->load_state(payload);
      if (!s.ok()) {
        return Status::invalid_argument("FeatureTransformer::load_state failed at stage "
                                        + std::to_string(i) + " pipeline index " + std::to_string(j)
                                        + ": " + s.message);
      }
    }
  }
  if (cursor != state.size()) {
    return Status::invalid_argument("FeatureTransformer::load_state contains trailing state tensors.");
  }
  if (!trained_) {
    feature_count_ = 0;
    output_feature_count_ = 0;
  }
  return Status::success();
}

StandardScaler::StandardScaler(StandardScalerOptions options) : options_(options) {}

bool StandardScaler::trained() const noexcept { return trained_; }

Status StandardScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status floor_status = validate_floor(options_.variance_floor, "StandardScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "StandardScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  center_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    double sum = 0.0;
    std::int64_t count = 0;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      sum += v;
      ++count;
    }

    double mean = 0.0;
    double stddev = 1.0;
    if (count > 0) {
      mean = sum / static_cast<double>(count);
      double var = 0.0;
      for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
        const double v = matrix.values[offset(matrix, i, f)];
        if (std::isnan(v)) continue;
        const double d = v - mean;
        var += d * d;
      }
      var /= static_cast<double>(count);
      stddev = std::sqrt(var);
      if (!std::isfinite(stddev) || !(stddev > options_.variance_floor)) stddev = 1.0;
    }

    center_[static_cast<std::size_t>(f)] = options_.with_centering ? mean : 0.0;
    scale_[static_cast<std::size_t>(f)] = options_.with_scaling ? stddev : 1.0;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> StandardScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "StandardScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "StandardScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      const double center = center_[static_cast<std::size_t>(f)];
      double denom = scale_[static_cast<std::size_t>(f)];
      if (!std::isfinite(denom) || !(std::abs(denom) > options_.variance_floor)) denom = 1.0;
      out[idx] = (v - center) / denom;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> StandardScaler::learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options,
                                           const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

MinMaxScaler::MinMaxScaler(MinMaxScalerOptions options) : options_(options) {}

bool MinMaxScaler::trained() const noexcept { return trained_; }

Status MinMaxScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status range_status = validate_range(options_.lower, options_.upper, "MinMaxScaler");
  if (!range_status.ok()) return range_status;
  Status floor_status = validate_floor(options_.range_floor, "MinMaxScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MinMaxScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  data_min_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  data_max_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    double minv = std::numeric_limits<double>::infinity();
    double maxv = -std::numeric_limits<double>::infinity();
    bool seen = false;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      minv = std::min(minv, v);
      maxv = std::max(maxv, v);
      seen = true;
    }
    if (!seen) {
      minv = 0.0;
      maxv = 1.0;
    }
    data_min_[static_cast<std::size_t>(f)] = minv;
    data_max_[static_cast<std::size_t>(f)] = maxv;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> MinMaxScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "MinMaxScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MinMaxScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  const double out_span = options_.upper - options_.lower;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      const double minv = data_min_[static_cast<std::size_t>(f)];
      const double maxv = data_max_[static_cast<std::size_t>(f)];
      const double den = maxv - minv;
      double mapped = options_.lower;
      if (std::abs(den) >= options_.range_floor) {
        mapped = ((v - minv) / den) * out_span + options_.lower;
      }
      if (options_.clip) {
        mapped = std::clamp(mapped, options_.lower, options_.upper);
      }
      out[idx] = mapped;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> MinMaxScaler::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

RobustScaler::RobustScaler(RobustScalerOptions options) : options_(options) {}

bool RobustScaler::trained() const noexcept { return trained_; }

Status RobustScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status q_status = validate_quantiles(options_.lower_quantile, options_.upper_quantile, "RobustScaler");
  if (!q_status.ok()) return q_status;
  Status floor_status = validate_floor(options_.range_floor, "RobustScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "RobustScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  center_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::vector<double> values = feature_non_missing(matrix, f);
    double med = 0.0;
    double iqr = 1.0;
    if (!values.empty()) {
      med = quantile(values, 0.5);
      const double q_low = quantile(values, options_.lower_quantile);
      const double q_high = quantile(values, options_.upper_quantile);
      iqr = q_high - q_low;
      if (!(std::abs(iqr) >= options_.range_floor)) iqr = 1.0;
    }

    center_[static_cast<std::size_t>(f)] = options_.with_centering ? med : 0.0;
    scale_[static_cast<std::size_t>(f)] = options_.with_scaling ? iqr : 1.0;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> RobustScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "RobustScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "RobustScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      out[idx] = (v - center_[static_cast<std::size_t>(f)]) / scale_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> RobustScaler::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

SimpleImputer::SimpleImputer(SimpleImputerOptions options) : options_(options) {}

bool SimpleImputer::trained() const noexcept { return trained_; }

Status SimpleImputer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "SimpleImputer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  fill_values_.assign(static_cast<std::size_t>(feature_count_), options_.constant_value);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::vector<double> values = feature_non_missing(matrix, f);
    double fill = options_.empty_feature_fallback;
    switch (options_.strategy) {
      case SimpleImputerStrategy::Mean: {
        if (!values.empty()) {
          const double sum = std::accumulate(values.begin(), values.end(), 0.0);
          fill = sum / static_cast<double>(values.size());
        }
        break;
      }
      case SimpleImputerStrategy::Median: {
        if (!values.empty()) fill = quantile(std::move(values), 0.5);
        break;
      }
      case SimpleImputerStrategy::MostFrequent: {
        fill = most_frequent(values, options_.empty_feature_fallback);
        break;
      }
      case SimpleImputerStrategy::Constant: {
        fill = options_.constant_value;
        break;
      }
    }
    fill_values_[static_cast<std::size_t>(f)] = fill;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> SimpleImputer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "SimpleImputer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "SimpleImputer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(out[idx])) out[idx] = fill_values_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> SimpleImputer::learn_map(const FeatureBatch& batch,
                                          const TrainOptions& learn_options,
                                          const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

NeighborImputer::NeighborImputer(NeighborImputerOptions options) : options_(options) {}

bool NeighborImputer::trained() const noexcept { return trained_; }

Status NeighborImputer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status k_status = validate_neighbors(options_.neighbors, "NeighborImputer");
  if (!k_status.ok()) return k_status;
  Status floor_status = validate_floor(options_.distance_floor, "NeighborImputer");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "NeighborImputer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  train_samples_ = matrix.sample_count;
  train_values_ = matrix.values;
  fallback_.assign(static_cast<std::size_t>(feature_count_), options_.empty_feature_fallback);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    const std::vector<double> values = feature_non_missing(matrix, f);
    if (values.empty()) {
      fallback_[static_cast<std::size_t>(f)] = options_.empty_feature_fallback;
      continue;
    }
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    fallback_[static_cast<std::size_t>(f)] = sum / static_cast<double>(values.size());
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> NeighborImputer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "NeighborImputer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "NeighborImputer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix query = matrix_or.move_value();

  DenseMatrix train{};
  train.values = train_values_;
  train.sample_count = train_samples_;
  train.feature_count = feature_count_;
  train.layout = layout_;

  std::vector<double> out = query.values;
  for (std::int64_t i = 0; i < query.sample_count; ++i) {
    for (std::int64_t f = 0; f < query.feature_count; ++f) {
      const std::size_t qidx = offset(query, i, f);
      if (!std::isnan(out[qidx])) continue;

      std::vector<std::pair<double, double>> candidates{}; // (distance2, value)
      candidates.reserve(static_cast<std::size_t>(train.sample_count));
      for (std::int64_t j = 0; j < train.sample_count; ++j) {
        const double target = train.values[offset(train, j, f)];
        if (std::isnan(target)) continue;

        double d2 = 0.0;
        std::int64_t overlap = 0;
        for (std::int64_t k = 0; k < query.feature_count; ++k) {
          if (k == f) continue;
          const double qv = out[offset(query, i, k)];
          const double tv = train.values[offset(train, j, k)];
          if (std::isnan(qv) || std::isnan(tv)) continue;
          const double d = qv - tv;
          d2 += d * d;
          ++overlap;
        }
        if (overlap == 0) continue;
        candidates.emplace_back(d2, target);
      }

      if (candidates.empty()) {
        out[qidx] = fallback_[static_cast<std::size_t>(f)];
        continue;
      }

      const std::int64_t k = std::min<std::int64_t>(options_.neighbors, static_cast<std::int64_t>(candidates.size()));
      std::partial_sort(
          candidates.begin(), candidates.begin() + k, candidates.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

      double num = 0.0;
      double den = 0.0;
      if (options_.weighting == NeighborImputerWeighting::Uniform) {
        for (std::int64_t t = 0; t < k; ++t) num += candidates[static_cast<std::size_t>(t)].second;
        den = static_cast<double>(k);
      } else {
        bool had_exact = false;
        for (std::int64_t t = 0; t < k; ++t) {
          const double d2 = candidates[static_cast<std::size_t>(t)].first;
          const double y = candidates[static_cast<std::size_t>(t)].second;
          if (d2 <= options_.distance_floor) {
            num += y;
            den += 1.0;
            had_exact = true;
          } else if (!had_exact) {
            const double w = 1.0 / std::sqrt(d2 + options_.distance_floor);
            num += w * y;
            den += w;
          }
        }
      }
      out[qidx] = (den > 0.0) ? (num / den) : fallback_[static_cast<std::size_t>(f)];
    }
  }

  return StatusOr<Series>(from_dense_like(query, std::move(out)));
}

StatusOr<Series> NeighborImputer::learn_map(const FeatureBatch& batch,
                                            const TrainOptions& learn_options,
                                            const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

IterativeImputer::IterativeImputer(IterativeImputerOptions options) : options_(options) {}

bool IterativeImputer::trained() const noexcept { return trained_; }

Status IterativeImputer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status rounds_status = validate_rounds(options_.rounds, "IterativeImputer");
  if (!rounds_status.ok()) return rounds_status;
  if (!std::isfinite(options_.ridge) || options_.ridge < 0.0) {
    return Status::invalid_argument("IterativeImputer requires finite ridge >= 0.");
  }

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "IterativeImputer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  fallback_.assign(static_cast<std::size_t>(feature_count_), options_.empty_feature_fallback);
  std::vector<bool> missing(static_cast<std::size_t>(matrix.values.size()), false);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::vector<double> values = feature_non_missing(matrix, f);
    double fill = options_.empty_feature_fallback;
    if (!values.empty()) {
      fill = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    }
    fallback_[static_cast<std::size_t>(f)] = fill;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(matrix.values[idx])) {
        matrix.values[idx] = fill;
        missing[idx] = true;
      }
    }
  }

  coefs_.assign(static_cast<std::size_t>(feature_count_), std::vector<double>{});
  intercept_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  if (feature_count_ <= 1) {
    trained_ = true;
    return Status::success();
  }

  for (std::int64_t round = 0; round < options_.rounds; ++round) {
    for (std::int64_t f = 0; f < feature_count_; ++f) {
      std::vector<std::int64_t> rows{};
      std::vector<double> target{};
      rows.reserve(static_cast<std::size_t>(matrix.sample_count));
      target.reserve(static_cast<std::size_t>(matrix.sample_count));

      for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
        const std::size_t idx = offset(matrix, i, f);
        if (missing[idx]) continue;
        rows.push_back(i);
        target.push_back(matrix.values[idx]);
      }
      if (rows.empty()) continue;
      std::vector<double> coef_full = fit_ridge_coefficients(matrix, target, f, rows, options_.ridge);
      if (coef_full.empty()) continue;

      intercept_[static_cast<std::size_t>(f)] = coef_full.front();
      coefs_[static_cast<std::size_t>(f)] =
          std::vector<double>(coef_full.begin() + 1, coef_full.end());

      for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
        const std::size_t idx = offset(matrix, i, f);
        if (!missing[idx]) continue;
        matrix.values[idx] = predict_ridge_row(
            matrix, i, f, coefs_[static_cast<std::size_t>(f)], intercept_[static_cast<std::size_t>(f)]);
      }
    }
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> IterativeImputer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "IterativeImputer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "IterativeImputer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  std::vector<bool> missing(static_cast<std::size_t>(matrix.values.size()), false);

  for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(matrix.values[idx])) {
        matrix.values[idx] = fallback_[static_cast<std::size_t>(f)];
        missing[idx] = true;
      }
    }
  }

  if (feature_count_ > 1) {
    for (std::int64_t round = 0; round < options_.rounds; ++round) {
      for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
        const auto& coef = coefs_[static_cast<std::size_t>(f)];
        if (coef.empty()) continue;
        for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
          const std::size_t idx = offset(matrix, i, f);
          if (!missing[idx]) continue;
          matrix.values[idx] = predict_ridge_row(matrix, i, f, coef, intercept_[static_cast<std::size_t>(f)]);
        }
      }
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(matrix.values)));
}

StatusOr<Series> IterativeImputer::learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

VectorNormalizer::VectorNormalizer(VectorNormalizerOptions options) : options_(options) {}

bool VectorNormalizer::trained() const noexcept { return trained_; }

Status VectorNormalizer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status floor_status = validate_floor(options_.floor, "VectorNormalizer");
  if (!floor_status.ok()) return floor_status;
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "VectorNormalizer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  trained_ = true;
  return Status::success();
}

StatusOr<Series> VectorNormalizer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "VectorNormalizer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "VectorNormalizer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    double norm = 0.0;
    if (options_.norm == VectorNorm::L1) {
      for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
        const double v = out[offset(matrix, i, f)];
        if (std::isnan(v)) continue;
        norm += std::abs(v);
      }
    } else if (options_.norm == VectorNorm::L2) {
      for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
        const double v = out[offset(matrix, i, f)];
        if (std::isnan(v)) continue;
        norm += v * v;
      }
      norm = std::sqrt(norm);
    } else {
      for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
        const double v = out[offset(matrix, i, f)];
        if (std::isnan(v)) continue;
        norm = std::max(norm, std::abs(v));
      }
    }
    if (!(norm >= options_.floor)) norm = 1.0;
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(out[idx])) continue;
      out[idx] /= norm;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> VectorNormalizer::learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

MaxAbsScaler::MaxAbsScaler(MaxAbsScalerOptions options) : options_(options) {}

bool MaxAbsScaler::trained() const noexcept { return trained_; }

Status MaxAbsScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status floor_status = validate_floor(options_.floor, "MaxAbsScaler");
  if (!floor_status.ok()) return floor_status;
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MaxAbsScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    double maxabs = 0.0;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      maxabs = std::max(maxabs, std::abs(v));
    }
    if (!(maxabs >= options_.floor)) maxabs = 1.0;
    scale_[static_cast<std::size_t>(f)] = maxabs;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> MaxAbsScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "MaxAbsScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MaxAbsScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  std::vector<double> out = matrix.values;

  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(out[idx])) continue;
      out[idx] /= scale_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> MaxAbsScaler::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

QuantileMap::QuantileMap(QuantileMapOptions options) : options_(options) {}

bool QuantileMap::trained() const noexcept { return trained_; }

Status QuantileMap::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status clip_status = validate_unit_interval(options_.clip, "QuantileMap");
  if (!clip_status.ok()) return clip_status;
  if (!(options_.clip < 0.5)) {
    return Status::invalid_argument("QuantileMap requires clip < 0.5.");
  }
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "QuantileMap::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  sorted_.assign(static_cast<std::size_t>(feature_count_), {});

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    auto values = feature_non_missing(matrix, f);
    std::sort(values.begin(), values.end());
    sorted_[static_cast<std::size_t>(f)] = std::move(values);
  }
  trained_ = true;
  return Status::success();
}

StatusOr<Series> QuantileMap::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "QuantileMap::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "QuantileMap::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  std::vector<double> out = matrix.values;

  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      const auto& sorted = sorted_[static_cast<std::size_t>(f)];
      if (sorted.empty()) {
        out[idx] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      double p = feature_cdf(sorted, v);
      p = std::clamp(p, options_.clip, 1.0 - options_.clip);
      out[idx] = (options_.output == QuantileMapOutput::Uniform) ? p : normal_quantile(p);
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> QuantileMap::learn_map(const FeatureBatch& batch,
                                        const TrainOptions& learn_options,
                                        const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

PowerMap::PowerMap(PowerMapOptions options) : options_(options) {}

bool PowerMap::trained() const noexcept { return trained_; }

Status PowerMap::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status floor_status = validate_floor(options_.variance_floor, "PowerMap");
  if (!floor_status.ok()) return floor_status;
  Status grid_status = validate_grid(options_.lambda_grid, "PowerMap");
  if (!grid_status.ok()) return grid_status;
  if (!(options_.lambda_max > options_.lambda_min) || !std::isfinite(options_.lambda_min) || !std::isfinite(options_.lambda_max)) {
    return Status::invalid_argument("PowerMap requires finite lambda_min < lambda_max.");
  }
  if (options_.lambda.has_value() && !std::isfinite(*options_.lambda)) {
    return Status::invalid_argument("PowerMap requires finite lambda when explicitly provided.");
  }

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "PowerMap::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  lambda_.assign(static_cast<std::size_t>(feature_count_), 1.0);
  center_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    const std::vector<double> values = feature_non_missing(matrix, f);
    if (values.empty()) continue;
    if (options_.mode == PowerMapMode::BoxCox) {
      for (double v : values) {
        if (!(v > 0.0)) {
          return Status::invalid_argument("PowerMap(BoxCox) requires strictly positive observed values.");
        }
      }
    }

    double best_lambda = options_.lambda.has_value() ? *options_.lambda : 1.0;
    if (!options_.lambda.has_value()) {
      double best_score = -std::numeric_limits<double>::infinity();
      const double step = (options_.lambda_max - options_.lambda_min) / static_cast<double>(options_.lambda_grid - 1);
      const double sum_pos_log = std::accumulate(values.begin(), values.end(), 0.0, [](double acc, double v) {
        return acc + (v >= 0.0 ? std::log1p(v) : 0.0);
      });
      const double sum_neg_log = std::accumulate(values.begin(), values.end(), 0.0, [](double acc, double v) {
        return acc + (v < 0.0 ? std::log1p(-v) : 0.0);
      });
      const double sum_box_log = std::accumulate(values.begin(), values.end(), 0.0, [](double acc, double v) {
        return acc + std::log(v);
      });

      for (std::int64_t gi = 0; gi < options_.lambda_grid; ++gi) {
        const double l = options_.lambda_min + step * static_cast<double>(gi);
        std::vector<double> transformed{};
        transformed.reserve(values.size());
        for (double v : values) {
          transformed.push_back(options_.mode == PowerMapMode::YeoJohnson ? yeo_johnson_value(v, l) : box_cox_value(v, l));
        }
        const double mean = std::accumulate(transformed.begin(), transformed.end(), 0.0) / static_cast<double>(transformed.size());
        double var = 0.0;
        for (double v : transformed) {
          const double d = v - mean;
          var += d * d;
        }
        var /= static_cast<double>(transformed.size());
        if (!(var > 0.0) || !std::isfinite(var)) continue;
        double score = -0.5 * static_cast<double>(transformed.size()) * std::log(var);
        if (options_.mode == PowerMapMode::YeoJohnson) {
          score += (l - 1.0) * (sum_pos_log - sum_neg_log);
        } else {
          score += (l - 1.0) * sum_box_log;
        }
        if (score > best_score) {
          best_score = score;
          best_lambda = l;
        }
      }
    }
    lambda_[static_cast<std::size_t>(f)] = best_lambda;

    if (options_.standardize) {
      double sum = 0.0;
      std::int64_t count = 0;
      for (double v : values) {
        sum += options_.mode == PowerMapMode::YeoJohnson ? yeo_johnson_value(v, best_lambda) : box_cox_value(v, best_lambda);
        ++count;
      }
      double mean = 0.0;
      double stddev = 1.0;
      if (count > 0) {
        mean = sum / static_cast<double>(count);
        double var = 0.0;
        for (double v : values) {
          const double y = options_.mode == PowerMapMode::YeoJohnson ? yeo_johnson_value(v, best_lambda) : box_cox_value(v, best_lambda);
          const double d = y - mean;
          var += d * d;
        }
        var /= static_cast<double>(count);
        stddev = std::sqrt(var);
        if (!(stddev >= options_.variance_floor)) stddev = 1.0;
      }
      center_[static_cast<std::size_t>(f)] = mean;
      scale_[static_cast<std::size_t>(f)] = stddev;
    }
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> PowerMap::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "PowerMap::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "PowerMap::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  std::vector<double> out = matrix.values;

  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      if (options_.mode == PowerMapMode::BoxCox && !(v > 0.0)) {
        return StatusOr<Series>(Status::invalid_argument("PowerMap(BoxCox) map() saw non-positive values."));
      }
      const double l = lambda_[static_cast<std::size_t>(f)];
      double y = options_.mode == PowerMapMode::YeoJohnson ? yeo_johnson_value(v, l) : box_cox_value(v, l);
      if (options_.standardize) {
        y = (y - center_[static_cast<std::size_t>(f)]) / scale_[static_cast<std::size_t>(f)];
      }
      out[idx] = y;
    }
  }

  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> PowerMap::learn_map(const FeatureBatch& batch,
                                     const TrainOptions& learn_options,
                                     const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

ThresholdBinarizer::ThresholdBinarizer(ThresholdBinarizerOptions options) : options_(options) {}

bool ThresholdBinarizer::trained() const noexcept { return trained_; }

Status ThresholdBinarizer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  if (!std::isfinite(options_.threshold) || !std::isfinite(options_.low_value) || !std::isfinite(options_.high_value)) {
    return Status::invalid_argument("ThresholdBinarizer requires finite threshold/low/high values.");
  }
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "ThresholdBinarizer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  trained_ = true;
  return Status::success();
}

StatusOr<Series> ThresholdBinarizer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "ThresholdBinarizer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "ThresholdBinarizer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  std::vector<double> out = matrix.values;
  for (double& v : out) {
    if (std::isnan(v)) continue;
    v = (v > options_.threshold) ? options_.high_value : options_.low_value;
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> ThresholdBinarizer::learn_map(const FeatureBatch& batch,
                                               const TrainOptions& learn_options,
                                               const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

PolynomialLift::PolynomialLift(PolynomialLiftOptions options) : options_(options) {}

bool PolynomialLift::trained() const noexcept { return trained_; }

Status PolynomialLift::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  if (options_.degree < 1) {
    return Status::invalid_argument("PolynomialLift requires degree >= 1.");
  }
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "PolynomialLift::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  terms_.clear();
  out_feature_count_ = options_.include_bias ? 1 : 0;

  std::vector<std::int64_t> cur{};
  for (std::int64_t d = 1; d <= options_.degree; ++d) {
    if (options_.interactions_only) {
      collect_combinations_no_repetition(0, d, feature_count_, cur, terms_);
    } else {
      collect_combinations_repetition(0, d, feature_count_, cur, terms_);
    }
  }
  out_feature_count_ += static_cast<std::int64_t>(terms_.size());
  trained_ = true;
  return Status::success();
}

StatusOr<Series> PolynomialLift::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "PolynomialLift::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "PolynomialLift::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * out_feature_count_), 0.0);
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    std::int64_t out_col = 0;
    if (options_.include_bias) {
      out[out_offset(matrix.layout, matrix.sample_count, out_feature_count_, i, out_col++)] = 1.0;
    }
    for (const auto& term : terms_) {
      double v = 1.0;
      bool has_nan = false;
      for (std::int64_t f : term) {
        const double x = matrix.values[offset(matrix, i, f)];
        if (std::isnan(x)) {
          has_nan = true;
          break;
        }
        v *= x;
      }
      out[out_offset(matrix.layout, matrix.sample_count, out_feature_count_, i, out_col++)] =
          has_nan ? std::numeric_limits<double>::quiet_NaN() : v;
    }
  }

  return StatusOr<Series>(from_dense(matrix, std::move(out), out_feature_count_));
}

StatusOr<Series> PolynomialLift::learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options,
                                           const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

OneHotEncoder::OneHotEncoder(OneHotEncoderOptions options) : options_(options) {}

bool OneHotEncoder::trained() const noexcept { return trained_; }

Status OneHotEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "OneHotEncoder");
  if (!max_cat_status.ok()) return max_cat_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OneHotEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_.assign(static_cast<std::size_t>(feature_count_), {});
  feature_offsets_.assign(static_cast<std::size_t>(feature_count_), 0);
  feature_widths_.assign(static_cast<std::size_t>(feature_count_), 0);
  encoded_feature_count_ = 0;

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::unordered_map<std::uint64_t, std::int64_t> counts{};
    counts.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      ++counts[key_for_value(v)];
    }

    std::vector<std::pair<std::uint64_t, std::int64_t>> ranked(counts.begin(), counts.end());
    const std::size_t cap = options_.max_categories > 0 ? static_cast<std::size_t>(options_.max_categories) : ranked.size();

    bool active = true;
    if (ranked.size() > cap) {
      switch (options_.overflow) {
        case CategoryOverflowPolicy::Error: {
          return Status::invalid_argument("OneHotEncoder observed feature cardinality above max_categories.");
        }
        case CategoryOverflowPolicy::SkipFeature: {
          active = false;
          ranked.clear();
          break;
        }
        case CategoryOverflowPolicy::KeepMostFrequent: {
          std::stable_sort(ranked.begin(),
                           ranked.end(),
                           [](const auto& lhs, const auto& rhs) {
                             if (lhs.second != rhs.second) return lhs.second > rhs.second;
                             return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
                           });
          ranked.resize(cap);
          break;
        }
      }
    }

    std::vector<std::uint64_t> keys{};
    keys.reserve(ranked.size());
    for (const auto& [key, _count] : ranked) keys.push_back(key);
    std::sort(keys.begin(), keys.end(), [](std::uint64_t a, std::uint64_t b) {
      return std::bit_cast<double>(a) < std::bit_cast<double>(b);
    });

    if (!active) keys.clear();
    category_keys_[static_cast<std::size_t>(f)] = std::move(keys);

    std::int64_t width = static_cast<std::int64_t>(category_keys_[static_cast<std::size_t>(f)].size());
    if (active && options_.include_unknown_bucket) ++width;
    feature_offsets_[static_cast<std::size_t>(f)] = encoded_feature_count_;
    feature_widths_[static_cast<std::size_t>(f)] = width;
    encoded_feature_count_ += width;
  }

  if (encoded_feature_count_ <= 0) {
    return Status::invalid_argument("OneHotEncoder produced zero output features; adjust category options.");
  }

  trained_ = true;
  return Status::success();
}

Status OneHotEncoder::learn(const pluss::table::Thread& feature, const TrainOptions& options) {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "OneHotEncoder::learn(Thread)");
  if (!batch_or.ok()) return batch_or.status();
  return learn(batch_or.value(), options);
}

StatusOr<Series> OneHotEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "OneHotEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OneHotEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * encoded_feature_count_), 0.0);
  for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
    const auto& keys = category_keys_[static_cast<std::size_t>(f)];
    const std::int64_t start = feature_offsets_[static_cast<std::size_t>(f)];
    const std::int64_t width = feature_widths_[static_cast<std::size_t>(f)];
    const std::int64_t unknown_idx = options_.include_unknown_bucket ? start + static_cast<std::int64_t>(keys.size()) : -1;
    if (width <= 0) continue;

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      bool unknown = std::isnan(v);
      std::int64_t local_idx = -1;
      if (!unknown) {
        const std::uint64_t key = key_for_value(v);
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          unknown = true;
        } else {
          local_idx = static_cast<std::int64_t>(std::distance(keys.begin(), it));
        }
      }

      if (unknown) {
        switch (options.unknown_categorical) {
          case UnknownCategoryPolicy::Error: {
            return StatusOr<Series>(Status::invalid_argument("OneHotEncoder saw unknown category during map()."));
          }
          case UnknownCategoryPolicy::MissingBucket: {
            if (unknown_idx >= 0) {
              out[out_offset(matrix.layout, matrix.sample_count, encoded_feature_count_, i, unknown_idx)] = 1.0;
            }
            break;
          }
          case UnknownCategoryPolicy::Ignore: {
            break;
          }
        }
        continue;
      }

      const std::int64_t encoded_idx = start + local_idx;
      out[out_offset(matrix.layout, matrix.sample_count, encoded_feature_count_, i, encoded_idx)] = 1.0;
    }
  }

  return StatusOr<Series>(from_dense(matrix, std::move(out), encoded_feature_count_));
}

StatusOr<Series> OneHotEncoder::map(const pluss::table::Thread& feature, const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "OneHotEncoder::map(Thread)");
  if (!batch_or.ok()) return StatusOr<Series>(batch_or.status());
  return map(batch_or.value(), options);
}

StatusOr<Series> OneHotEncoder::learn_map(const FeatureBatch& batch,
                                          const TrainOptions& learn_options,
                                          const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

StatusOr<Series> OneHotEncoder::learn_map(const pluss::table::Thread& feature,
                                          const TrainOptions& learn_options,
                                          const InferOptions& infer_options) {
  Status s = learn(feature, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(feature, infer_options);
}

FrequencyEncoder::FrequencyEncoder(FrequencyEncoderOptions options) : options_(options) {}

bool FrequencyEncoder::trained() const noexcept { return trained_; }

Status FrequencyEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "FrequencyEncoder");
  if (!max_cat_status.ok()) return max_cat_status;
  Status unknown_status = validate_unit_interval(options_.unknown_frequency, "FrequencyEncoder");
  if (!unknown_status.ok()) return unknown_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "FrequencyEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_.assign(static_cast<std::size_t>(feature_count_), {});
  category_freqs_.assign(static_cast<std::size_t>(feature_count_), {});

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::unordered_map<std::uint64_t, std::int64_t> counts{};
    counts.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      ++counts[key_for_value(v)];
    }

    std::vector<std::pair<std::uint64_t, std::int64_t>> ranked(counts.begin(), counts.end());
    const std::size_t cap = options_.max_categories > 0 ? static_cast<std::size_t>(options_.max_categories) : ranked.size();
    if (ranked.size() > cap) {
      switch (options_.overflow) {
        case CategoryOverflowPolicy::Error: {
          return Status::invalid_argument("FrequencyEncoder observed feature cardinality above max_categories.");
        }
        case CategoryOverflowPolicy::SkipFeature: {
          ranked.clear();
          break;
        }
        case CategoryOverflowPolicy::KeepMostFrequent: {
          std::stable_sort(ranked.begin(),
                           ranked.end(),
                           [](const auto& lhs, const auto& rhs) {
                             if (lhs.second != rhs.second) return lhs.second > rhs.second;
                             return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
                           });
          ranked.resize(cap);
          break;
        }
      }
    }

    const double total = static_cast<double>(std::accumulate(
        ranked.begin(),
        ranked.end(),
        std::int64_t{0},
        [](std::int64_t acc, const std::pair<std::uint64_t, std::int64_t>& item) { return acc + item.second; }));

    std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
      return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
    });

    auto& keys = category_keys_[static_cast<std::size_t>(f)];
    auto& freqs = category_freqs_[static_cast<std::size_t>(f)];
    keys.reserve(ranked.size());
    freqs.reserve(ranked.size());
    for (const auto& [key, count] : ranked) {
      keys.push_back(key);
      freqs.push_back(total > 0.0 ? static_cast<double>(count) / total : 0.0);
    }
  }

  trained_ = true;
  return Status::success();
}

Status FrequencyEncoder::learn(const pluss::table::Thread& feature, const TrainOptions& options) {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "FrequencyEncoder::learn(Thread)");
  if (!batch_or.ok()) return batch_or.status();
  return learn(batch_or.value(), options);
}

StatusOr<Series> FrequencyEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "FrequencyEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "FrequencyEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * matrix.feature_count), 0.0);
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t out_idx = offset(matrix, i, f);
      const auto& keys = category_keys_[static_cast<std::size_t>(f)];
      const auto& freqs = category_freqs_[static_cast<std::size_t>(f)];

      const double v = matrix.values[offset(matrix, i, f)];
      bool unknown = std::isnan(v);
      double mapped = 0.0;
      if (!unknown) {
        const std::uint64_t key = key_for_value(v);
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          unknown = true;
        } else {
          const std::size_t idx = static_cast<std::size_t>(std::distance(keys.begin(), it));
          mapped = freqs[idx];
        }
      }

      if (unknown) {
        switch (options.unknown_categorical) {
          case UnknownCategoryPolicy::Error: {
            return StatusOr<Series>(Status::invalid_argument("FrequencyEncoder saw unknown category during map()."));
          }
          case UnknownCategoryPolicy::MissingBucket: {
            mapped = options_.unknown_frequency;
            break;
          }
          case UnknownCategoryPolicy::Ignore: {
            mapped = 0.0;
            break;
          }
        }
      }
      out[out_idx] = mapped;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> FrequencyEncoder::map(const pluss::table::Thread& feature, const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "FrequencyEncoder::map(Thread)");
  if (!batch_or.ok()) return StatusOr<Series>(batch_or.status());
  return map(batch_or.value(), options);
}

StatusOr<Series> FrequencyEncoder::learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

StatusOr<Series> FrequencyEncoder::learn_map(const pluss::table::Thread& feature,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status s = learn(feature, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(feature, infer_options);
}

namespace {

template <class EncoderOptions>
[[nodiscard]] std::vector<std::uint64_t> fit_category_keys_for_feature(const DenseMatrix& matrix,
                                                                        std::int64_t feature_idx,
                                                                        const EncoderOptions& options,
                                                                        bool* active_out,
                                                                        Status* status_out,
                                                                        const char* where) {
  if (active_out) *active_out = true;
  if (status_out) *status_out = Status::success();

  std::unordered_map<std::uint64_t, std::int64_t> counts{};
  counts.reserve(static_cast<std::size_t>(matrix.sample_count));
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    const double v = matrix.values[offset(matrix, i, feature_idx)];
    if (std::isnan(v)) continue;
    ++counts[key_for_value(v)];
  }

  std::vector<std::pair<std::uint64_t, std::int64_t>> ranked(counts.begin(), counts.end());
  const std::size_t cap = options.max_categories > 0 ? static_cast<std::size_t>(options.max_categories) : ranked.size();
  if (ranked.size() > cap) {
    switch (options.overflow) {
      case CategoryOverflowPolicy::Error: {
        if (status_out) {
          *status_out = Status::invalid_argument(std::string(where) + " observed feature cardinality above max_categories.");
        }
        return {};
      }
      case CategoryOverflowPolicy::SkipFeature: {
        if (active_out) *active_out = false;
        ranked.clear();
        break;
      }
      case CategoryOverflowPolicy::KeepMostFrequent: {
        std::stable_sort(ranked.begin(),
                         ranked.end(),
                         [](const auto& lhs, const auto& rhs) {
                           if (lhs.second != rhs.second) return lhs.second > rhs.second;
                           return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
                         });
        ranked.resize(cap);
        break;
      }
    }
  }

  std::vector<std::uint64_t> keys{};
  keys.reserve(ranked.size());
  for (const auto& [key, _count] : ranked) keys.push_back(key);
  std::sort(keys.begin(), keys.end(), [](std::uint64_t a, std::uint64_t b) {
    return std::bit_cast<double>(a) < std::bit_cast<double>(b);
  });
  return keys;
}

template <class EncoderOptions>
[[nodiscard]] StatusOr<std::vector<std::vector<std::uint64_t>>> fit_category_keys(const DenseMatrix& matrix,
                                                                                   const EncoderOptions& options,
                                                                                   const char* where) {
  std::vector<std::vector<std::uint64_t>> keys(static_cast<std::size_t>(matrix.feature_count));
  for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
    bool active = true;
    Status status = Status::success();
    auto fitted = fit_category_keys_for_feature(matrix, f, options, &active, &status, where);
    if (!status.ok()) return StatusOr<std::vector<std::vector<std::uint64_t>>>(std::move(status));
    if (!active) fitted.clear();
    keys[static_cast<std::size_t>(f)] = std::move(fitted);
  }
  return StatusOr<std::vector<std::vector<std::uint64_t>>>(std::move(keys));
}

template <class EncoderOptions>
[[nodiscard]] StatusOr<Series> map_index_like(const DenseMatrix& matrix,
                                              const std::vector<std::vector<std::uint64_t>>& category_keys,
                                              const EncoderOptions& options,
                                              const InferOptions& infer_options,
                                              const char* where) {
  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * matrix.feature_count),
                          std::numeric_limits<double>::quiet_NaN());
  const double base = options.one_based ? 1.0 : 0.0;

  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const auto& keys = category_keys[static_cast<std::size_t>(f)];
      const double v = matrix.values[offset(matrix, i, f)];
      bool unknown = std::isnan(v);
      std::int64_t index = -1;

      if (!unknown) {
        const std::uint64_t key = key_for_value(v);
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          unknown = true;
        } else {
          index = static_cast<std::int64_t>(std::distance(keys.begin(), it));
        }
      }

      double mapped = std::numeric_limits<double>::quiet_NaN();
      if (!unknown) {
        mapped = base + static_cast<double>(index);
      } else {
        switch (infer_options.unknown_categorical) {
          case UnknownCategoryPolicy::Error:
            return StatusOr<Series>(Status::invalid_argument(std::string(where) + " saw unknown category during map()."));
          case UnknownCategoryPolicy::MissingBucket:
            if (options.include_unknown_bucket) {
              mapped = base + static_cast<double>(keys.size());
            }
            break;
          case UnknownCategoryPolicy::Ignore:
            break;
        }
      }
      out[offset(matrix, i, f)] = mapped;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

} // namespace

OrdinalEncoder::OrdinalEncoder(OrdinalEncoderOptions options) : options_(options) {}

bool OrdinalEncoder::trained() const noexcept { return trained_; }

Status OrdinalEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "OrdinalEncoder");
  if (!max_cat_status.ok()) return max_cat_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OrdinalEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  auto keys_or = fit_category_keys(matrix, options_, "OrdinalEncoder::learn");
  if (!keys_or.ok()) return keys_or.status();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_ = keys_or.move_value();
  trained_ = true;
  return Status::success();
}

Status OrdinalEncoder::learn(const pluss::table::Thread& feature, const TrainOptions& options) {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "OrdinalEncoder::learn(Thread)");
  if (!batch_or.ok()) return batch_or.status();
  return learn(batch_or.value(), options);
}

StatusOr<Series> OrdinalEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "OrdinalEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OrdinalEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  return map_index_like(matrix_or.value(), category_keys_, options_, options, "OrdinalEncoder::map");
}

StatusOr<Series> OrdinalEncoder::map(const pluss::table::Thread& feature, const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "OrdinalEncoder::map(Thread)");
  if (!batch_or.ok()) return StatusOr<Series>(batch_or.status());
  return map(batch_or.value(), options);
}

StatusOr<Series> OrdinalEncoder::learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options,
                                           const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

StatusOr<Series> OrdinalEncoder::learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options,
                                           const InferOptions& infer_options) {
  Status s = learn(feature, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(feature, infer_options);
}

StatusOr<std::vector<Series>> OrdinalEncoder::dump_state() const {
  std::vector<std::int64_t> sizes{};
  sizes.reserve(category_keys_.size());
  std::vector<double> flat_values{};
  for (const auto& keys : category_keys_) {
    sizes.push_back(static_cast<std::int64_t>(keys.size()));
    for (std::uint64_t key : keys) flat_values.push_back(std::bit_cast<double>(key));
  }
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({
          trained_ ? 1.0 : 0.0,
          static_cast<double>(static_cast<std::int64_t>(layout_)),
          static_cast<double>(feature_count_),
      }),
      state_i64(std::move(sizes)),
      state_f64(std::move(flat_values)),
  });
}

Status OrdinalEncoder::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("OrdinalEncoder::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("OrdinalEncoder::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  const auto nested = unflatten_nested(vec_f64(state[2]), vec_i64(state[1]));
  category_keys_.clear();
  category_keys_.reserve(nested.size());
  for (const auto& row : nested) {
    std::vector<std::uint64_t> keys{};
    keys.reserve(row.size());
    for (double v : row) keys.push_back(key_for_value(v));
    category_keys_.push_back(std::move(keys));
  }
  if (!trained_) category_keys_.clear();
  return Status::success();
}

LabelEncoder::LabelEncoder(LabelEncoderOptions options) : options_(options) {}

bool LabelEncoder::trained() const noexcept { return trained_; }

Status LabelEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "LabelEncoder");
  if (!max_cat_status.ok()) return max_cat_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "LabelEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  auto keys_or = fit_category_keys(matrix, options_, "LabelEncoder::learn");
  if (!keys_or.ok()) return keys_or.status();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_ = keys_or.move_value();
  trained_ = true;
  return Status::success();
}

Status LabelEncoder::learn(const pluss::table::Thread& feature, const TrainOptions& options) {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "LabelEncoder::learn(Thread)");
  if (!batch_or.ok()) return batch_or.status();
  return learn(batch_or.value(), options);
}

StatusOr<Series> LabelEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "LabelEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "LabelEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  return map_index_like(matrix_or.value(), category_keys_, options_, options, "LabelEncoder::map");
}

StatusOr<Series> LabelEncoder::map(const pluss::table::Thread& feature, const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = single_feature_batch_from_thread(feature, "LabelEncoder::map(Thread)");
  if (!batch_or.ok()) return StatusOr<Series>(batch_or.status());
  return map(batch_or.value(), options);
}

StatusOr<Series> LabelEncoder::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

StatusOr<Series> LabelEncoder::learn_map(const pluss::table::Thread& feature,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(feature, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(feature, infer_options);
}

StatusOr<std::vector<Series>> LabelEncoder::dump_state() const {
  std::vector<std::int64_t> sizes{};
  sizes.reserve(category_keys_.size());
  std::vector<double> flat_values{};
  for (const auto& keys : category_keys_) {
    sizes.push_back(static_cast<std::int64_t>(keys.size()));
    for (std::uint64_t key : keys) flat_values.push_back(std::bit_cast<double>(key));
  }
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({
          trained_ ? 1.0 : 0.0,
          static_cast<double>(static_cast<std::int64_t>(layout_)),
          static_cast<double>(feature_count_),
      }),
      state_i64(std::move(sizes)),
      state_f64(std::move(flat_values)),
  });
}

Status LabelEncoder::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("LabelEncoder::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("LabelEncoder::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  const auto nested = unflatten_nested(vec_f64(state[2]), vec_i64(state[1]));
  category_keys_.clear();
  category_keys_.reserve(nested.size());
  for (const auto& row : nested) {
    std::vector<std::uint64_t> keys{};
    keys.reserve(row.size());
    for (double v : row) keys.push_back(key_for_value(v));
    category_keys_.push_back(std::move(keys));
  }
  if (!trained_) category_keys_.clear();
  return Status::success();
}

OutlierIsolator::OutlierIsolator(OutlierIsolatorOptions options) : options_(options) {}

bool OutlierIsolator::learned() const noexcept { return learned_; }

Status OutlierIsolator::validate_options_() const {
  Status estimators_status = validate_estimators(options_.estimators, "OutlierIsolator");
  if (!estimators_status.ok()) return estimators_status;

  if (!options_.max_samples_auto) {
    Status max_samples_status = validate_max_samples(options_.max_samples, "OutlierIsolator");
    if (!max_samples_status.ok()) return max_samples_status;
  }

  Status max_features_status = validate_max_features_ratio(options_.max_features_ratio, "OutlierIsolator");
  if (!max_features_status.ok()) return max_features_status;

  if (!options_.contamination_auto) {
    Status contamination_status = validate_contamination(options_.contamination, "OutlierIsolator");
    if (!contamination_status.ok()) return contamination_status;
  }

  return Status::success();
}

Status OutlierIsolator::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status options_status = validate_options_();
  if (!options_status.ok()) return options_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OutlierIsolator::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();
  for (double& v : matrix.values) {
    if (std::isfinite(v)) v = static_cast<double>(static_cast<float>(v));
  }

  if (matrix.sample_count < 2) {
    return Status::invalid_argument("OutlierIsolator::learn requires at least 2 samples.");
  }

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  max_samples_used_ = options_.max_samples_auto ? std::min<std::int64_t>(256, matrix.sample_count)
                                                : std::min<std::int64_t>(options_.max_samples, matrix.sample_count);
  max_samples_used_ = std::max<std::int64_t>(1, max_samples_used_);

  const auto max_features_raw = static_cast<std::int64_t>(
      std::floor(options_.max_features_ratio * static_cast<double>(feature_count_)));
  max_features_used_ = std::clamp<std::int64_t>(max_features_raw, 1, feature_count_);

  const std::int64_t depth_base = std::max<std::int64_t>(2, max_samples_used_);
  max_depth_ = static_cast<std::int64_t>(std::ceil(std::log2(static_cast<double>(depth_base))));

  NumpyRandomState global_rng(static_cast<std::uint32_t>(compose_seed(options_.random)));
  std::vector<std::int64_t> all_samples(static_cast<std::size_t>(matrix.sample_count));
  std::iota(all_samples.begin(), all_samples.end(), std::int64_t{0});
  std::vector<std::int64_t> all_features(static_cast<std::size_t>(feature_count_));
  std::iota(all_features.begin(), all_features.end(), std::int64_t{0});

  forest_.clear();
  forest_.reserve(static_cast<std::size_t>(options_.estimators));

  for (std::int64_t tree_idx = 0; tree_idx < options_.estimators; ++tree_idx) {
    const std::int64_t tree_seed = randint_max_int(global_rng);
    NumpyRandomState bagging_rng(static_cast<std::uint32_t>(tree_seed));
    NumpyRandomState estimator_rng(static_cast<std::uint32_t>(tree_seed));
    std::uint32_t splitter_seed = static_cast<std::uint32_t>(randint_max_int(estimator_rng));

    std::vector<std::int64_t> sampled_rows{};
    sampled_rows.reserve(static_cast<std::size_t>(max_samples_used_));
    if (options_.bootstrap) {
      for (std::int64_t i = 0; i < max_samples_used_; ++i) {
        sampled_rows.push_back(bagging_rng.randint_exclusive(matrix.sample_count));
      }
    } else {
      sampled_rows = sample_without_replacement(matrix.sample_count, max_samples_used_, bagging_rng);
      std::sort(sampled_rows.begin(), sampled_rows.end());
    }

    std::vector<std::int64_t> feature_subset{};
    if (max_features_used_ >= feature_count_) {
      feature_subset = all_features;
    } else {
      feature_subset = sample_without_replacement(feature_count_, max_features_used_, bagging_rng);
    }

    IsolationTree tree{};
    auto add_leaf = [&tree](std::int64_t n_rows) -> std::int32_t {
      const std::int32_t node = static_cast<std::int32_t>(tree.split_feature.size());
      tree.split_feature.push_back(-1);
      tree.split_value.push_back(0.0);
      tree.left_child.push_back(-1);
      tree.right_child.push_back(-1);
      tree.node_size.push_back(static_cast<std::int32_t>(std::max<std::int64_t>(0, n_rows)));
      tree.missing_go_left.push_back(0);
      return node;
    };

    std::vector<std::int64_t> splitter_features = feature_subset;
    std::vector<std::int64_t> constant_features = feature_subset;
    constexpr std::size_t kPerNodeMaxFeatures = 1; // ExtraTreeRegressor(max_features=1)

    std::function<std::int32_t(const std::vector<std::int64_t>&, std::int64_t, std::size_t)> build =
        [&](const std::vector<std::int64_t>& rows,
            std::int64_t depth,
            std::size_t n_known_constants_initial) -> std::int32_t {
      if (rows.empty()) return add_leaf(0);
      if (depth >= max_depth_ || rows.size() <= 1) {
        return add_leaf(static_cast<std::int64_t>(rows.size()));
      }

      std::int64_t split_feature = -1;
      double split_min = 0.0;
      double split_max = 0.0;
      if (splitter_features.empty()) {
        return add_leaf(static_cast<std::int64_t>(rows.size()));
      }
      n_known_constants_initial = std::min<std::size_t>(n_known_constants_initial, splitter_features.size());

      std::size_t f_i = splitter_features.size();
      std::size_t n_found_constants = 0;
      std::size_t n_drawn_constants = 0;
      std::size_t n_known_constants = n_known_constants_initial;
      std::size_t n_total_constants = n_known_constants;
      std::size_t n_visited_features = 0;

      auto restore_constant_prefix = [&]() {
        for (std::size_t i = 0; i < n_known_constants_initial; ++i) {
          splitter_features[i] = constant_features[i];
        }
        for (std::size_t i = 0; i < n_found_constants; ++i) {
          constant_features[n_known_constants_initial + i] = splitter_features[n_known_constants_initial + i];
        }
      };

      while (f_i > n_total_constants &&
             (n_visited_features < kPerNodeMaxFeatures ||
              n_visited_features <= n_found_constants + n_drawn_constants)) {
        ++n_visited_features;
        std::int64_t f_j_i64 = sklearn_rand_int(
            static_cast<std::int64_t>(n_drawn_constants),
            static_cast<std::int64_t>(f_i - n_found_constants),
            splitter_seed);
        std::size_t f_j = static_cast<std::size_t>(f_j_i64);
        if (f_j < n_known_constants) {
          std::swap(splitter_features[n_drawn_constants], splitter_features[f_j]);
          ++n_drawn_constants;
          continue;
        }
        f_j += n_found_constants;
        const std::int64_t f = splitter_features[f_j];
        double minv = std::numeric_limits<double>::infinity();
        double maxv = -std::numeric_limits<double>::infinity();
        std::int64_t non_missing = 0;
        for (std::int64_t row : rows) {
          const double v = matrix.values[offset(matrix, row, f)];
          if (std::isnan(v)) continue;
          minv = std::min(minv, v);
          maxv = std::max(maxv, v);
          ++non_missing;
        }
        const std::int64_t missing_count = static_cast<std::int64_t>(rows.size()) - non_missing;
        constexpr double kFeatureThreshold = 1e-7;
        const bool all_missing = non_missing == 0;
        const bool effectively_constant_no_missing = (!all_missing) && (maxv <= minv + kFeatureThreshold) && (missing_count == 0);
        if (all_missing || effectively_constant_no_missing) {
          std::swap(splitter_features[f_j], splitter_features[n_total_constants]);
          ++n_found_constants;
          ++n_total_constants;
          continue;
        }
        f_i -= 1;
        std::swap(splitter_features[f_i], splitter_features[f_j]);
        split_feature = f;
        split_min = minv;
        split_max = maxv;
        break;
      }
      if (split_feature < 0) {
        restore_constant_prefix();
        return add_leaf(static_cast<std::int64_t>(rows.size()));
      }

      double split_value = sklearn_rand_uniform(split_min, split_max, splitter_seed);
      if (split_value == split_max) split_value = split_min;
      std::vector<std::int64_t> left_rows{};
      std::vector<std::int64_t> right_rows{};
      std::vector<std::int64_t> missing_rows{};
      left_rows.reserve(rows.size());
      right_rows.reserve(rows.size());
      missing_rows.reserve(rows.size());
      for (std::int64_t row : rows) {
        const double value = matrix.values[offset(matrix, row, split_feature)];
        if (std::isnan(value)) {
          missing_rows.push_back(row);
          continue;
        }
        if (value <= split_value) {
          left_rows.push_back(row);
        } else {
          right_rows.push_back(row);
        }
      }

      std::uint8_t missing_go_left = 0;
      if (!missing_rows.empty()) {
        missing_go_left = static_cast<std::uint8_t>(sklearn_rand_int(0, 2, splitter_seed));
      }
      if (missing_go_left) {
        left_rows.insert(left_rows.end(), missing_rows.begin(), missing_rows.end());
      } else {
        right_rows.insert(right_rows.end(), missing_rows.begin(), missing_rows.end());
      }

      if (left_rows.empty() || right_rows.empty()) {
        restore_constant_prefix();
        return add_leaf(static_cast<std::int64_t>(rows.size()));
      }

      const std::int32_t node = add_leaf(static_cast<std::int64_t>(rows.size()));
      tree.split_feature[static_cast<std::size_t>(node)] = static_cast<std::int32_t>(split_feature);
      tree.split_value[static_cast<std::size_t>(node)] = split_value;
      if (missing_rows.empty()) {
        tree.missing_go_left[static_cast<std::size_t>(node)] =
            static_cast<std::uint8_t>(left_rows.size() > right_rows.size());
      } else {
        tree.missing_go_left[static_cast<std::size_t>(node)] = missing_go_left;
      }
      restore_constant_prefix();
      tree.left_child[static_cast<std::size_t>(node)] = build(left_rows, depth + 1, n_total_constants);
      tree.right_child[static_cast<std::size_t>(node)] = build(right_rows, depth + 1, n_total_constants);
      return node;
    };

    (void)build(sampled_rows, 0, 0);
    forest_.push_back(std::move(tree));
  }

  learned_ = true;
  offset_ = -0.5; // sklearn contamination='auto'
  if (!options_.contamination_auto) {
    StatusOr<std::vector<double>> scores_or = score_dense_(batch.x, batch.layout, "OutlierIsolator::learn");
    if (!scores_or.ok()) {
      learned_ = false;
      forest_.clear();
      return scores_or.status();
    }
    offset_ = quantile(scores_or.move_value(), options_.contamination);
  }

  return Status::success();
}

StatusOr<std::vector<double>> OutlierIsolator::score_dense_(const Series& x,
                                                            ObservationLayout layout,
                                                            const char* where) const {
  FeatureBatch batch{};
  batch.x = x;
  batch.layout = layout;
  Status ready = validate_trained_input(learned_, layout_, feature_count_, batch, where);
  if (!ready.ok()) return StatusOr<std::vector<double>>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, where);
  if (!matrix_or.ok()) return StatusOr<std::vector<double>>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();
  for (double& v : matrix.values) {
    if (std::isfinite(v)) v = static_cast<double>(static_cast<float>(v));
  }
  if (forest_.empty()) {
    return StatusOr<std::vector<double>>(Status::precondition(std::string(where) + " requires a learned forest."));
  }

  const double norm = std::max(average_isolation_path_length(max_samples_used_), 1e-12);
  std::vector<double> scores(static_cast<std::size_t>(matrix.sample_count), 0.0);

  auto path_length = [&](const IsolationTree& tree, std::int64_t row) -> double {
    if (tree.split_feature.empty()) return 0.0;
    std::int32_t node = 0;
    std::int64_t depth = 0;
    while (node >= 0 && node < static_cast<std::int32_t>(tree.split_feature.size())) {
      const std::int32_t feat = tree.split_feature[static_cast<std::size_t>(node)];
      if (feat < 0 || tree.left_child[static_cast<std::size_t>(node)] < 0 || tree.right_child[static_cast<std::size_t>(node)] < 0) {
        const std::int64_t leaf_size = tree.node_size[static_cast<std::size_t>(node)];
        return static_cast<double>(depth) + average_isolation_path_length(leaf_size);
      }
      const double split = tree.split_value[static_cast<std::size_t>(node)];
      const double value = matrix.values[offset(matrix, row, static_cast<std::int64_t>(feat))];
      if (std::isnan(value)) {
        const bool go_left = tree.missing_go_left[static_cast<std::size_t>(node)] != 0;
        node = go_left ? tree.left_child[static_cast<std::size_t>(node)]
                       : tree.right_child[static_cast<std::size_t>(node)];
      } else {
        node = value <= split ? tree.left_child[static_cast<std::size_t>(node)]
                              : tree.right_child[static_cast<std::size_t>(node)];
      }
      ++depth;
    }
    return static_cast<double>(depth);
  };

  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    double total_path = 0.0;
    for (const auto& tree : forest_) {
      total_path += path_length(tree, i);
    }
    const double avg_path = total_path / static_cast<double>(forest_.size());
    const double anomaly_score = std::exp2(-avg_path / norm);
    scores[static_cast<std::size_t>(i)] = -anomaly_score; // sklearn-like score_samples: lower means more anomalous.
  }

  return StatusOr<std::vector<double>>(std::move(scores));
}

StatusOr<Series> OutlierIsolator::score_samples(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  StatusOr<std::vector<double>> scores_or = score_dense_(batch.x, batch.layout, "OutlierIsolator::score_samples");
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

  std::vector<double> scores = scores_or.move_value();
  Series out = pluss::make_series(
      std::move(scores),
      {batch.sample_count()},
      Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  return StatusOr<Series>(out.to(batch.x.platform(), ScalarType::Float64));
}

StatusOr<Series> OutlierIsolator::infer(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  StatusOr<std::vector<double>> scores_or = score_dense_(batch.x, batch.layout, "OutlierIsolator::infer");
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

  std::vector<double> scores = scores_or.move_value();
  std::vector<std::int64_t> labels(scores.size(), 1);
  for (std::size_t i = 0; i < scores.size(); ++i) {
    const double decision = scores[i] - offset_;
    labels[i] = decision < 0.0 ? -1 : 1;
  }

  Series out = pluss::make_series(
      std::move(labels),
      {batch.sample_count()},
      Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  return StatusOr<Series>(out.to(batch.x.platform(), ScalarType::Int64));
}

StatusOr<Series> OutlierIsolator::learn_infer(const FeatureBatch& batch,
                                              const TrainOptions& learn_options,
                                              const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return infer(batch, infer_options);
}

StatusOr<std::vector<Series>> StandardScaler::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(center_),
      state_f64(scale_),
  });
}

Status StandardScaler::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("StandardScaler::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("StandardScaler::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  center_ = vec_f64(state[1]);
  scale_ = vec_f64(state[2]);
  if (!trained_) {
    center_.clear();
    scale_.clear();
  }
  return Status::success();
}

StatusOr<std::vector<Series>> MinMaxScaler::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(data_min_),
      state_f64(data_max_),
  });
}

Status MinMaxScaler::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("MinMaxScaler::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("MinMaxScaler::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  data_min_ = vec_f64(state[1]);
  data_max_ = vec_f64(state[2]);
  if (!trained_) {
    data_min_.clear();
    data_max_.clear();
  }
  return Status::success();
}

StatusOr<std::vector<Series>> RobustScaler::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(center_),
      state_f64(scale_),
  });
}

Status RobustScaler::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("RobustScaler::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("RobustScaler::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  center_ = vec_f64(state[1]);
  scale_ = vec_f64(state[2]);
  if (!trained_) {
    center_.clear();
    scale_.clear();
  }
  return Status::success();
}

StatusOr<std::vector<Series>> SimpleImputer::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(fill_values_),
  });
}

Status SimpleImputer::load_state(const std::vector<Series>& state) {
  if (state.size() < 2) return Status::invalid_argument("SimpleImputer::load_state expects 2 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("SimpleImputer::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  fill_values_ = vec_f64(state[1]);
  if (!trained_) fill_values_.clear();
  return Status::success();
}

StatusOr<std::vector<Series>> NeighborImputer::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0,
                 static_cast<double>(static_cast<std::int64_t>(layout_)),
                 static_cast<double>(feature_count_),
                 static_cast<double>(train_samples_)}),
      state_f64(train_values_),
      state_f64(fallback_),
  });
}

Status NeighborImputer::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("NeighborImputer::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 4) return Status::invalid_argument("NeighborImputer::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  train_samples_ = static_cast<std::int64_t>(meta[3]);
  train_values_ = vec_f64(state[1]);
  fallback_ = vec_f64(state[2]);
  if (!trained_) {
    train_values_.clear();
    fallback_.clear();
    train_samples_ = 0;
  }
  return Status::success();
}

StatusOr<std::vector<Series>> IterativeImputer::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(fallback_),
      state_f64(intercept_),
      state_i64(nested_sizes(coefs_)),
      state_f64(flatten_nested(coefs_)),
  });
}

Status IterativeImputer::load_state(const std::vector<Series>& state) {
  if (state.size() < 5) return Status::invalid_argument("IterativeImputer::load_state expects 5 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("IterativeImputer::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  fallback_ = vec_f64(state[1]);
  intercept_ = vec_f64(state[2]);
  coefs_ = unflatten_nested(vec_f64(state[4]), vec_i64(state[3]));
  if (!trained_) {
    fallback_.clear();
    intercept_.clear();
    coefs_.clear();
  }
  return Status::success();
}

StatusOr<std::vector<Series>> VectorNormalizer::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
  });
}

Status VectorNormalizer::load_state(const std::vector<Series>& state) {
  if (state.empty()) return Status::invalid_argument("VectorNormalizer::load_state expects metadata tensor.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("VectorNormalizer::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  return Status::success();
}

StatusOr<std::vector<Series>> MaxAbsScaler::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(scale_),
  });
}

Status MaxAbsScaler::load_state(const std::vector<Series>& state) {
  if (state.size() < 2) return Status::invalid_argument("MaxAbsScaler::load_state expects 2 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("MaxAbsScaler::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  scale_ = vec_f64(state[1]);
  if (!trained_) scale_.clear();
  return Status::success();
}

StatusOr<std::vector<Series>> QuantileMap::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_i64(nested_sizes(sorted_)),
      state_f64(flatten_nested(sorted_)),
  });
}

Status QuantileMap::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("QuantileMap::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("QuantileMap::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  sorted_ = unflatten_nested(vec_f64(state[2]), vec_i64(state[1]));
  if (!trained_) sorted_.clear();
  return Status::success();
}

StatusOr<std::vector<Series>> PowerMap::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_f64(lambda_),
      state_f64(center_),
      state_f64(scale_),
  });
}

Status PowerMap::load_state(const std::vector<Series>& state) {
  if (state.size() < 4) return Status::invalid_argument("PowerMap::load_state expects 4 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("PowerMap::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  lambda_ = vec_f64(state[1]);
  center_ = vec_f64(state[2]);
  scale_ = vec_f64(state[3]);
  if (!trained_) {
    lambda_.clear();
    center_.clear();
    scale_.clear();
  }
  return Status::success();
}

StatusOr<std::vector<Series>> ThresholdBinarizer::dump_state() const {
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
  });
}

Status ThresholdBinarizer::load_state(const std::vector<Series>& state) {
  if (state.empty()) return Status::invalid_argument("ThresholdBinarizer::load_state expects metadata tensor.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("ThresholdBinarizer::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  return Status::success();
}

StatusOr<std::vector<Series>> PolynomialLift::dump_state() const {
  std::vector<std::int64_t> sizes{};
  sizes.reserve(terms_.size());
  std::vector<std::int64_t> flat{};
  for (const auto& term : terms_) {
    sizes.push_back(static_cast<std::int64_t>(term.size()));
    flat.insert(flat.end(), term.begin(), term.end());
  }
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0,
                 static_cast<double>(static_cast<std::int64_t>(layout_)),
                 static_cast<double>(feature_count_),
                 static_cast<double>(out_feature_count_)}),
      state_i64(std::move(sizes)),
      state_i64(std::move(flat)),
  });
}

Status PolynomialLift::load_state(const std::vector<Series>& state) {
  if (state.size() < 3) return Status::invalid_argument("PolynomialLift::load_state expects 3 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 4) return Status::invalid_argument("PolynomialLift::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  out_feature_count_ = static_cast<std::int64_t>(meta[3]);
  terms_ = unflatten_nested_i64(vec_i64(state[2]), vec_i64(state[1]));
  if (!trained_) {
    terms_.clear();
    out_feature_count_ = 0;
  }
  return Status::success();
}

StatusOr<std::vector<Series>> OneHotEncoder::dump_state() const {
  std::vector<std::int64_t> sizes{};
  sizes.reserve(category_keys_.size());
  std::vector<double> flat_values{};
  for (const auto& keys : category_keys_) {
    sizes.push_back(static_cast<std::int64_t>(keys.size()));
    for (std::uint64_t key : keys) {
      flat_values.push_back(std::bit_cast<double>(key));
    }
  }
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0,
                 static_cast<double>(static_cast<std::int64_t>(layout_)),
                 static_cast<double>(feature_count_),
                 static_cast<double>(encoded_feature_count_)}),
      state_i64(std::move(sizes)),
      state_f64(std::move(flat_values)),
      state_i64(feature_offsets_),
      state_i64(feature_widths_),
  });
}

Status OneHotEncoder::load_state(const std::vector<Series>& state) {
  if (state.size() < 5) return Status::invalid_argument("OneHotEncoder::load_state expects 5 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 4) return Status::invalid_argument("OneHotEncoder::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  encoded_feature_count_ = static_cast<std::int64_t>(meta[3]);
  const auto sizes = vec_i64(state[1]);
  const auto flat = vec_f64(state[2]);
  const auto nested = unflatten_nested(flat, sizes);
  category_keys_.clear();
  category_keys_.reserve(nested.size());
  for (const auto& row : nested) {
    std::vector<std::uint64_t> keys{};
    keys.reserve(row.size());
    for (double v : row) keys.push_back(key_for_value(v));
    category_keys_.push_back(std::move(keys));
  }
  feature_offsets_ = vec_i64(state[3]);
  feature_widths_ = vec_i64(state[4]);
  if (!trained_) {
    category_keys_.clear();
    feature_offsets_.clear();
    feature_widths_.clear();
    encoded_feature_count_ = 0;
  }
  return Status::success();
}

StatusOr<std::vector<Series>> FrequencyEncoder::dump_state() const {
  std::vector<std::int64_t> key_sizes{};
  key_sizes.reserve(category_keys_.size());
  std::vector<double> flat_keys{};
  for (const auto& keys : category_keys_) {
    key_sizes.push_back(static_cast<std::int64_t>(keys.size()));
    for (std::uint64_t key : keys) flat_keys.push_back(std::bit_cast<double>(key));
  }
  const auto freq_sizes = nested_sizes(category_freqs_);
  const auto flat_freqs = flatten_nested(category_freqs_);
  return StatusOr<std::vector<Series>>(std::vector<Series>{
      state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(static_cast<std::int64_t>(layout_)), static_cast<double>(feature_count_)}),
      state_i64(std::move(key_sizes)),
      state_f64(std::move(flat_keys)),
      state_i64(freq_sizes),
      state_f64(flat_freqs),
  });
}

Status FrequencyEncoder::load_state(const std::vector<Series>& state) {
  if (state.size() < 5) return Status::invalid_argument("FrequencyEncoder::load_state expects 5 state tensors.");
  const auto meta = vec_f64(state[0]);
  if (meta.size() < 3) return Status::invalid_argument("FrequencyEncoder::load_state invalid metadata.");
  trained_ = meta[0] > 0.5;
  layout_ = static_cast<ObservationLayout>(static_cast<std::int64_t>(meta[1]));
  feature_count_ = static_cast<std::int64_t>(meta[2]);
  const auto key_nested = unflatten_nested(vec_f64(state[2]), vec_i64(state[1]));
  category_keys_.clear();
  category_keys_.reserve(key_nested.size());
  for (const auto& row : key_nested) {
    std::vector<std::uint64_t> keys{};
    keys.reserve(row.size());
    for (double v : row) keys.push_back(key_for_value(v));
    category_keys_.push_back(std::move(keys));
  }
  category_freqs_ = unflatten_nested(vec_f64(state[4]), vec_i64(state[3]));
  if (!trained_) {
    category_keys_.clear();
    category_freqs_.clear();
  }
  return Status::success();
}

} // namespace pluss::ai::nodej::prep
