#pragma once

#include "pluss/ai/nodej/interfaces.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace pluss::table {
class Thread;
}

namespace pluss::ai::nodej::prep {

struct StandardScalerOptions {
  bool with_centering = true;
  bool with_scaling = true;
  double variance_floor = 1e-12;
};

struct MinMaxScalerOptions {
  double lower = 0.0;
  double upper = 1.0;
  bool clip = false;
  double range_floor = 1e-12;
};

struct RobustScalerOptions {
  bool with_centering = true;
  bool with_scaling = true;
  double lower_quantile = 0.25;
  double upper_quantile = 0.75;
  double range_floor = 1e-12;
};

enum class SimpleImputerStrategy : std::uint8_t {
  Mean = 0,
  Median = 1,
  MostFrequent = 2,
  Constant = 3,
};

struct SimpleImputerOptions {
  SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean;
  double constant_value = 0.0;
  double empty_feature_fallback = 0.0;
};

enum class CategoryOverflowPolicy : std::uint8_t {
  Error = 0,
  KeepMostFrequent = 1,
  SkipFeature = 2,
};

struct OneHotEncoderOptions {
  std::int64_t max_categories = 20;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
  bool include_unknown_bucket = true;
};

struct FrequencyEncoderOptions {
  std::int64_t max_categories = 50;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
  double unknown_frequency = 0.0;
};

struct OrdinalEncoderOptions {
  std::int64_t max_categories = 256;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
  bool include_unknown_bucket = true;
  bool one_based = true;
};

struct LabelEncoderOptions {
  std::int64_t max_categories = 256;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
  bool include_unknown_bucket = true;
  bool one_based = false;
};

// Base-N encoding (category_encoders.BaseNEncoder): ordinal-encode categories to
// codes 1..K (0 reserved for unseen / missing), then represent each code in
// radix `base` across a fixed number of digit columns. base == 2 is the popular
// "binary encoder". K categories cost ceil(log_base(K+1)) columns instead of the
// K columns one-hot would spend.
struct BaseNEncoderOptions {
  std::int64_t base = 2;
  std::int64_t max_categories = 1024;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
};

// Feature hashing (the "hashing trick"): map each category value into one of
// `n_components` buckets with a fixed multiplicative hash and emit an
// n_components-wide indicator. Output width is independent of cardinality and no
// vocabulary is stored, so unseen categories are handled for free (at the cost
// of possible collisions). A missing value produces an all-zero row.
struct HashingEncoderOptions {
  std::int64_t n_components = 8;
  std::uint64_t seed = 0;
};

// Target / mean encoding with additive m-estimate smoothing
// (category_encoders.MEstimateEncoder): each category is replaced by
// (sum_y_in_cat + smoothing * prior) / (count_in_cat + smoothing), where `prior`
// is the global target mean. Larger `smoothing` shrinks rare categories toward
// the prior. Supervised: the target is supplied at fit time. Appropriate for
// regression and binary (0/1) targets; one output column per input feature.
struct TargetEncoderOptions {
  double smoothing = 10.0;
  std::int64_t max_categories = 4096;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
};

// Weight-of-evidence encoding (category_encoders.WOEEncoder) for a binary 0/1
// target: each category is replaced by
// ln( ((pos_c + reg)/(P + 2 reg)) / ((neg_c + reg)/(N + 2 reg)) ), with P/N the
// total positives/negatives and `reg` a Laplace term. Unseen categories encode
// to 0 (neutral). Supervised; one output column per input feature.
struct WOEEncoderOptions {
  double regularization = 1.0;
  std::int64_t max_categories = 4096;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
};

enum class NeighborImputerWeighting : std::uint8_t {
  Uniform = 0,
  Distance = 1,
};

struct NeighborImputerOptions {
  std::int64_t neighbors = 5;
  NeighborImputerWeighting weighting = NeighborImputerWeighting::Distance;
  double distance_floor = 1e-12;
  double empty_feature_fallback = 0.0;
};

struct IterativeImputerOptions {
  std::int64_t rounds = 8;
  double ridge = 1e-6;
  double empty_feature_fallback = 0.0;
};

enum class VectorNorm : std::uint8_t {
  L1 = 0,
  L2 = 1,
  Linf = 2,
};

struct VectorNormalizerOptions {
  VectorNorm norm = VectorNorm::L2;
  double floor = 1e-12;
};

struct MaxAbsScalerOptions {
  double floor = 1e-12;
};

enum class QuantileMapOutput : std::uint8_t {
  Uniform = 0,
  Gaussian = 1,
};

struct QuantileMapOptions {
  QuantileMapOutput output = QuantileMapOutput::Uniform;
  double clip = 1e-7;
};

enum class PowerMapMode : std::uint8_t {
  YeoJohnson = 0,
  BoxCox = 1,
};

struct PowerMapOptions {
  PowerMapMode mode = PowerMapMode::YeoJohnson;
  bool standardize = true;
  std::optional<double> lambda = std::nullopt;
  double lambda_min = -5.0;
  double lambda_max = 5.0;
  std::int64_t lambda_grid = 201;
  double variance_floor = 1e-12;
};

struct ThresholdBinarizerOptions {
  double threshold = 0.0;
  double low_value = 0.0;
  double high_value = 1.0;
};

struct PolynomialLiftOptions {
  std::int64_t degree = 2;
  bool interactions_only = false;
  bool include_bias = false;
};

struct OutlierIsolatorOptions {
  std::int64_t estimators = 100;
  std::int64_t max_samples = 256;
  bool max_samples_auto = true;
  double max_features_ratio = 1.0;
  bool bootstrap = false;
  bool contamination_auto = true;
  double contamination = 0.1;
  RandomState random{};
};

class OutlierIsolator final {
public:
  explicit OutlierIsolator(OutlierIsolatorOptions options = {});

  [[nodiscard]] bool learned() const noexcept;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> score_samples(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_infer(const FeatureBatch& batch,
                                             const TrainOptions& learn_options = {},
                                             const InferOptions& infer_options = {});

  [[nodiscard]] double decision_offset() const noexcept { return offset_; }
  [[nodiscard]] std::int64_t max_samples_used() const noexcept { return max_samples_used_; }
  [[nodiscard]] std::int64_t max_features_used() const noexcept { return max_features_used_; }

private:
  struct IsolationTree {
    std::vector<std::int32_t> split_feature{};
    std::vector<double> split_value{};
    std::vector<std::int32_t> left_child{};
    std::vector<std::int32_t> right_child{};
    std::vector<std::int32_t> node_size{};
    std::vector<std::uint8_t> missing_go_left{};
  };

  [[nodiscard]] Status validate_options_() const;
  [[nodiscard]] StatusOr<std::vector<double>> score_dense_(const Series& x, ObservationLayout layout, const char* where) const;

  OutlierIsolatorOptions options_{};
  bool learned_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::int64_t max_samples_used_{0};
  std::int64_t max_features_used_{0};
  std::int64_t max_depth_{0};
  double offset_{-0.5};
  std::vector<IsolationTree> forest_{};
};

struct FunctionTransformerOptions {
  bool validate_input = true;
  bool preserve_sample_count = true;
};

class FunctionTransformer final : public TransformerBase {
public:
  using SeriesMapFn = std::function<StatusOr<Series>(const Series&)>;

  explicit FunctionTransformer(SeriesMapFn map_fn = {}, FunctionTransformerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  FunctionTransformerOptions options_{};
  SeriesMapFn map_fn_{};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  bool learned_{false};
};

enum class FeatureRemainderPolicy : std::uint8_t {
  Drop = 0,
  PassThrough = 1,
};

class FeatureTransformer final : public TransformerBase {
public:
  explicit FeatureTransformer(FeatureRemainderPolicy remainder = FeatureRemainderPolicy::Drop);

  FeatureTransformer(FeatureTransformer&&) noexcept = default;
  FeatureTransformer& operator=(FeatureTransformer&&) noexcept = default;
  FeatureTransformer(const FeatureTransformer&) = delete;
  FeatureTransformer& operator=(const FeatureTransformer&) = delete;

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

  Status add_stage(std::unique_ptr<TransformerBase> transformer, pluss::msharp::RangeView::Int feature_route);
  Status add_pipeline(std::vector<std::unique_ptr<TransformerBase>> transformers,
                      pluss::msharp::RangeView::Int feature_route);
  void clear_stages() noexcept;
  [[nodiscard]] std::int64_t stage_count() const noexcept;

  [[nodiscard]] FeatureRemainderPolicy remainder_policy() const noexcept;
  void set_remainder_policy(FeatureRemainderPolicy policy) noexcept;

private:
  struct Stage {
    std::vector<std::unique_ptr<TransformerBase>> transformers{};
    std::vector<std::int64_t> feature_route{};
  };

  FeatureRemainderPolicy remainder_{FeatureRemainderPolicy::Drop};
  std::vector<Stage> stages_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::int64_t output_feature_count_{0};
};

class StandardScaler final : public TransformerBase {
public:
  explicit StandardScaler(StandardScalerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  StandardScalerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> center_{};
  std::vector<double> scale_{};
};

class MinMaxScaler final : public TransformerBase {
public:
  explicit MinMaxScaler(MinMaxScalerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  MinMaxScalerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> data_min_{};
  std::vector<double> data_max_{};
};

class RobustScaler final : public TransformerBase {
public:
  explicit RobustScaler(RobustScalerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  RobustScalerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> center_{};
  std::vector<double> scale_{};
};

class SimpleImputer final : public TransformerBase {
public:
  explicit SimpleImputer(SimpleImputerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  SimpleImputerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> fill_values_{};
};

class NeighborImputer final : public TransformerBase {
public:
  explicit NeighborImputer(NeighborImputerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  NeighborImputerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> train_values_{};
  std::int64_t train_samples_{0};
  std::vector<double> fallback_{};
};

class IterativeImputer final : public TransformerBase {
public:
  explicit IterativeImputer(IterativeImputerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  IterativeImputerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> fallback_{};
  std::vector<std::vector<double>> coefs_{};
  std::vector<double> intercept_{};
};

class VectorNormalizer final : public TransformerBase {
public:
  explicit VectorNormalizer(VectorNormalizerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  VectorNormalizerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
};

class MaxAbsScaler final : public TransformerBase {
public:
  explicit MaxAbsScaler(MaxAbsScalerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  MaxAbsScalerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> scale_{};
};

class QuantileMap final : public TransformerBase {
public:
  explicit QuantileMap(QuantileMapOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  QuantileMapOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<double>> sorted_{};
};

class PowerMap final : public TransformerBase {
public:
  explicit PowerMap(PowerMapOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  PowerMapOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> lambda_{};
  std::vector<double> center_{};
  std::vector<double> scale_{};
};

class ThresholdBinarizer final : public TransformerBase {
public:
  explicit ThresholdBinarizer(ThresholdBinarizerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  ThresholdBinarizerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
};

class PolynomialLift final : public TransformerBase {
public:
  explicit PolynomialLift(PolynomialLiftOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  PolynomialLiftOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::int64_t>> terms_{};
  std::int64_t out_feature_count_{0};
};

class OneHotEncoder final : public TransformerBase {
public:
  explicit OneHotEncoder(OneHotEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  OneHotEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::int64_t encoded_feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::int64_t> feature_offsets_{};
  std::vector<std::int64_t> feature_widths_{};
};

class FrequencyEncoder final : public TransformerBase {
public:
  explicit FrequencyEncoder(FrequencyEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  FrequencyEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::vector<double>> category_freqs_{};
};

class OrdinalEncoder final : public TransformerBase {
public:
  explicit OrdinalEncoder(OrdinalEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  OrdinalEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
};

class LabelEncoder final : public TransformerBase {
public:
  explicit LabelEncoder(LabelEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  LabelEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
};

class BaseNEncoder final : public TransformerBase {
public:
  explicit BaseNEncoder(BaseNEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  BaseNEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::int64_t out_feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::int64_t> digit_counts_{};
};

class HashingEncoder final : public TransformerBase {
public:
  explicit HashingEncoder(HashingEncoderOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  Status learn(const pluss::table::Thread& feature, const TrainOptions& options = {});
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<Series> learn_map(const pluss::table::Thread& feature,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(pluss::msharp::RangeView::Series state) override;

private:
  HashingEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
};

// Shared machinery for the supervised categorical encoders (target / woe).
class SupervisedEncoderBase : public TransformerBase {
public:
  [[nodiscard]] bool trained() const noexcept final { return trained_; }
  [[nodiscard]] bool needs_target() const noexcept final { return true; }

  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) final;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) final;
  Status learn_supervised(const FeatureBatch& batch,
                          const Series& target,
                          const TrainOptions& options = {}) final;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const final;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const final;
  Status load_state(pluss::msharp::RangeView::Series state) final;

protected:
  [[nodiscard]] virtual const char* encoder_name_() const noexcept = 0;
  [[nodiscard]] virtual std::int64_t max_categories_() const noexcept = 0;
  [[nodiscard]] virtual CategoryOverflowPolicy overflow_() const noexcept = 0;
  // Fill `values_out[f]` with one encoded value per category key, and
  // `unknown_out[f]` with the value an unseen category receives.
  virtual Status fit_feature_values_(const std::vector<double>& column,
                                     const std::vector<double>& target,
                                     const std::vector<std::uint64_t>& keys,
                                     std::vector<double>& values_out,
                                     double& unknown_out) = 0;

  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::vector<double>> category_values_{};
  std::vector<double> unknown_values_{};
};

class TargetEncoder final : public SupervisedEncoderBase {
public:
  explicit TargetEncoder(TargetEncoderOptions options = {}) : options_(options) {}

protected:
  [[nodiscard]] const char* encoder_name_() const noexcept override { return "TargetEncoder"; }
  [[nodiscard]] std::int64_t max_categories_() const noexcept override { return options_.max_categories; }
  [[nodiscard]] CategoryOverflowPolicy overflow_() const noexcept override { return options_.overflow; }
  Status fit_feature_values_(const std::vector<double>& column,
                             const std::vector<double>& target,
                             const std::vector<std::uint64_t>& keys,
                             std::vector<double>& values_out,
                             double& unknown_out) override;

private:
  TargetEncoderOptions options_{};
};

class WOEEncoder final : public SupervisedEncoderBase {
public:
  explicit WOEEncoder(WOEEncoderOptions options = {}) : options_(options) {}

protected:
  [[nodiscard]] const char* encoder_name_() const noexcept override { return "WOEEncoder"; }
  [[nodiscard]] std::int64_t max_categories_() const noexcept override { return options_.max_categories; }
  [[nodiscard]] CategoryOverflowPolicy overflow_() const noexcept override { return options_.overflow; }
  Status fit_feature_values_(const std::vector<double>& column,
                             const std::vector<double>& target,
                             const std::vector<std::uint64_t>& keys,
                             std::vector<double>& values_out,
                             double& unknown_out) override;

private:
  WOEEncoderOptions options_{};
};

} // namespace pluss::ai::nodej::prep
