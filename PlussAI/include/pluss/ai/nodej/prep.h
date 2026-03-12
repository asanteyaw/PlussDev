#pragma once

#include "pluss/ai/nodej/interfaces.h"

#include <cstdint>
#include <optional>
#include <vector>

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  Status load_state(const std::vector<Series>& state) override;

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
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(const std::vector<Series>& state) override;

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
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override;
  Status load_state(const std::vector<Series>& state) override;

private:
  FrequencyEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::vector<double>> category_freqs_{};
};

} // namespace pluss::ai::nodej::prep
