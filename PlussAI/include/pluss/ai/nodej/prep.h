#pragma once

#include "pluss/ai/nodej/interfaces.h"

#include <cstdint>
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

class StandardScaler final : public TransformerBase {
public:
  explicit StandardScaler(StandardScalerOptions options = {});

  [[nodiscard]] bool trained() const noexcept override;
  Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) override;
  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const override;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {}) override;

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

private:
  SimpleImputerOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<double> fill_values_{};
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

private:
  FrequencyEncoderOptions options_{};
  bool trained_{false};
  ObservationLayout layout_{ObservationLayout::Rows};
  std::int64_t feature_count_{0};
  std::vector<std::vector<std::uint64_t>> category_keys_{};
  std::vector<std::vector<double>> category_freqs_{};
};

} // namespace pluss::ai::nodej::prep
