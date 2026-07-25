#pragma once

#include "pluss/ai/lymph/common.h"

namespace pluss::ai::lymph {

namespace prep {

class OneHotEncoder;
class FrequencyEncoder;
class OrdinalEncoder;
class LabelEncoder;

using StandardScalerOptions = ::pluss::ai::nodej::prep::StandardScalerOptions;
using MinMaxScalerOptions = ::pluss::ai::nodej::prep::MinMaxScalerOptions;
using RobustScalerOptions = ::pluss::ai::nodej::prep::RobustScalerOptions;
using SimpleImputerStrategy = ::pluss::ai::nodej::prep::SimpleImputerStrategy;
using SimpleImputerOptions = ::pluss::ai::nodej::prep::SimpleImputerOptions;
using CategoryOverflowPolicy = ::pluss::ai::nodej::prep::CategoryOverflowPolicy;
using OneHotEncoderOptions = ::pluss::ai::nodej::prep::OneHotEncoderOptions;
using FrequencyEncoderOptions = ::pluss::ai::nodej::prep::FrequencyEncoderOptions;
using OrdinalEncoderOptions = ::pluss::ai::nodej::prep::OrdinalEncoderOptions;
using LabelEncoderOptions = ::pluss::ai::nodej::prep::LabelEncoderOptions;
using NeighborImputerWeighting = ::pluss::ai::nodej::prep::NeighborImputerWeighting;
using NeighborImputerOptions = ::pluss::ai::nodej::prep::NeighborImputerOptions;
using IterativeImputerOptions = ::pluss::ai::nodej::prep::IterativeImputerOptions;
using VectorNorm = ::pluss::ai::nodej::prep::VectorNorm;
using VectorNormalizerOptions = ::pluss::ai::nodej::prep::VectorNormalizerOptions;
using MaxAbsScalerOptions = ::pluss::ai::nodej::prep::MaxAbsScalerOptions;
using QuantileMapOutput = ::pluss::ai::nodej::prep::QuantileMapOutput;
using QuantileMapOptions = ::pluss::ai::nodej::prep::QuantileMapOptions;
using PowerMapMode = ::pluss::ai::nodej::prep::PowerMapMode;
using PowerMapOptions = ::pluss::ai::nodej::prep::PowerMapOptions;
using ThresholdBinarizerOptions = ::pluss::ai::nodej::prep::ThresholdBinarizerOptions;
using PolynomialLiftOptions = ::pluss::ai::nodej::prep::PolynomialLiftOptions;
using OutlierIsolatorOptions = ::pluss::ai::nodej::prep::OutlierIsolatorOptions;
using FunctionTransformerOptions = ::pluss::ai::nodej::prep::FunctionTransformerOptions;
using FeatureRemainderPolicy = ::pluss::ai::nodej::prep::FeatureRemainderPolicy;
using FunctionTransformer = ::pluss::ai::nodej::prep::FunctionTransformer;
using FeatureTransformer = ::pluss::ai::nodej::prep::FeatureTransformer;

template <class WrapperT>
inline void prep_refresh_encoder_attrs_(WrapperT&) {}

template <class WrapperT>
inline void prep_before_thread_fit_(WrapperT&, const pluss::table::Thread&) {}

[[nodiscard]] inline std::uint64_t prep_key_for_value_(double value) {
  if (value == 0.0) value = 0.0;
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] inline double prep_note_hash_value_(std::string_view token) {
  constexpr std::uint64_t fnv_offset = 1469598103934665603ULL;
  constexpr std::uint64_t fnv_prime = 1099511628211ULL;
  std::uint64_t h = fnv_offset;
  for (unsigned char ch : token) {
    h ^= static_cast<std::uint64_t>(ch);
    h *= fnv_prime;
  }
  const std::uint32_t lane = static_cast<std::uint32_t>((h ^ (h >> 32)) & 0x7FFFFFFFULL);
  return static_cast<double>(lane);
}

[[nodiscard]] inline std::string prep_scalar_to_string_(double value) {
  std::ostringstream oss;
  oss.precision(15);
  oss << value;
  return oss.str();
}

struct SimpleImputerSummary {
  SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean;
  bool learned = false;
  std::int64_t feature_count = 0;
  double constant_value = 0.0;
  double empty_feature_fallback = 0.0;
  std::vector<std::string> feature_names{};
  Series statistics = Series::undefined();
};

struct StandardScalerSummary {
  std::vector<std::string> feature_names{};
  Series mean = Series::undefined();
  Series scale = Series::undefined();
};

struct TransformerSummary {
  std::vector<std::string> feature_names{};
  std::vector<Series> state{};
};

struct OutlierIsolatorSummary {
  bool learned = false;
  std::int64_t feature_count = 0;
  std::int64_t estimators = 0;
  std::int64_t max_samples = 0;
  std::int64_t max_features = 0;
  bool bootstrap = false;
  bool contamination_auto = true;
  double contamination = 0.1;
  double offset = -0.5;
  std::vector<std::string> feature_names{};
};

struct CategoryIndexerOptions {
  std::int64_t max_categories = 256;
  CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent;
  bool include_unknown_bucket = true;
  bool one_based = true;
};

struct CategoryIndexerSummary {
  bool learned = false;
  std::int64_t feature_count = 0;
  bool include_unknown_bucket = true;
  bool one_based = true;
  std::vector<std::string> feature_names{};
  std::vector<std::string> mapped_names{};
  std::vector<std::vector<std::string>> categories{};
};

inline const char* simple_imputer_strategy_name(SimpleImputerStrategy strategy) {
  switch (strategy) {
    case SimpleImputerStrategy::Mean:
      return "mean";
    case SimpleImputerStrategy::Median:
      return "median";
    case SimpleImputerStrategy::MostFrequent:
      return "most_frequent";
    case SimpleImputerStrategy::Constant:
      return "constant";
    default:
      return "unknown";
  }
}

inline void output_render(std::ostream& os, const SimpleImputerSummary& summary) {
  os << "SimpleImputerSummary{strategy=" << simple_imputer_strategy_name(summary.strategy)
     << ", learned=" << (summary.learned ? "true" : "false")
     << ", feature_count=" << summary.feature_count
     << ", constant_value=" << summary.constant_value
     << ", empty_feature_fallback=" << summary.empty_feature_fallback
     << ", feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], statistics=" << summary.statistics << "}";
}

inline void output_render(std::ostream& os, const StandardScalerSummary& summary) {
  os << "StandardScalerSummary{feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], mean=" << summary.mean << ", scale=" << summary.scale << "}";
}

inline void output_render(std::ostream& os, const TransformerSummary& summary) {
  os << "TransformerSummary{feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], state=[";
  for (std::size_t i = 0; i < summary.state.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.state[i];
  }
  os << "]}";
}

inline void output_render(std::ostream& os, const OutlierIsolatorSummary& summary) {
  os << "OutlierIsolatorSummary{learned=" << (summary.learned ? "true" : "false")
     << ", feature_count=" << summary.feature_count
     << ", estimators=" << summary.estimators
     << ", max_samples=" << summary.max_samples
     << ", max_features=" << summary.max_features
     << ", bootstrap=" << (summary.bootstrap ? "true" : "false")
     << ", contamination_auto=" << (summary.contamination_auto ? "true" : "false")
     << ", contamination=" << summary.contamination
     << ", offset=" << summary.offset
     << ", feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "]}";
}

inline void output_render(std::ostream& os, const CategoryIndexerSummary& summary) {
  os << "CategoryIndexerSummary{learned=" << (summary.learned ? "true" : "false")
     << ", feature_count=" << summary.feature_count
     << ", include_unknown_bucket=" << (summary.include_unknown_bucket ? "true" : "false")
     << ", one_based=" << (summary.one_based ? "true" : "false")
     << ", feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], mapped_names=[";
  for (std::size_t i = 0; i < summary.mapped_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.mapped_names[i];
  }
  os << "], categories=[";
  for (std::size_t i = 0; i < summary.categories.size(); ++i) {
    if (i != 0) os << ", ";
    os << "[";
    for (std::size_t j = 0; j < summary.categories[i].size(); ++j) {
      if (j != 0) os << ", ";
      os << summary.categories[i][j];
    }
    os << "]";
  }
  os << "]}";
}

inline std::ostream& operator<<(std::ostream& os, const SimpleImputerSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const StandardScalerSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const TransformerSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const OutlierIsolatorSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const CategoryIndexerSummary& summary) {
  output_render(os, summary);
  return os;
}

class SimpleImputer final {
public:
  explicit SimpleImputer(SimpleImputerOptions options = {})
      : options_(options), core_(options) {}

  SimpleImputer& features(pluss::msharp::RangeView::Of<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }
  SimpleImputer& features(std::initializer_list<std::string_view> names) {
    return features(pluss::msharp::RangeView::Of<std::string_view>(names));
  }

  SimpleImputer& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,
                                                  const TrainOptions& learn_options = {},
                                                  const InferOptions& infer_options = {});

  [[nodiscard]] SimpleImputerSummary summary() const {
    return SimpleImputerSummary{
        .strategy = options_.strategy,
        .learned = learned_,
        .feature_count = static_cast<std::int64_t>(feature_names_.size()),
        .constant_value = options_.constant_value,
        .empty_feature_fallback = options_.empty_feature_fallback,
        .feature_names = feature_names_,
        .statistics = statistics_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  SimpleImputerOptions options_{};
  std::vector<std::string> feature_names_{};
  Series statistics_ = Series::undefined();

private:
  [[nodiscard]] std::vector<std::string> resolved_feature_names_(const pluss::table::DataFrame& frame, const char* where) const;
  [[nodiscard]] static DataFrameBatchOptions batch_options_for_(pluss::msharp::RangeView::Of<std::string> names);
  [[nodiscard]] static pluss::table::DataFrame merge_mapped_(const pluss::table::DataFrame& frame,
                                                             pluss::msharp::RangeView::Of<std::string> names,
                                                             const Series& mapped,
                                                             const char* where);
  void refresh_statistics_();
  ::pluss::ai::nodej::prep::SimpleImputer core_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

class StandardScaler final {
public:
  explicit StandardScaler(StandardScalerOptions options = {})
      : options_(options), core_(options) {}

  StandardScaler& features(pluss::msharp::RangeView::Of<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }
  StandardScaler& features(std::initializer_list<std::string_view> names) {
    return features(pluss::msharp::RangeView::Of<std::string_view>(names));
  }

  StandardScaler& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,
                                                  const TrainOptions& learn_options = {},
                                                  const InferOptions& infer_options = {});

  [[nodiscard]] StandardScalerSummary summary() const {
    return StandardScalerSummary{
        .feature_names = feature_names_,
        .mean = mean_,
        .scale = scale_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  StandardScalerOptions options_{};
  std::vector<std::string> feature_names_{};
  Series mean_ = Series::undefined();
  Series scale_ = Series::undefined();

private:
  [[nodiscard]] std::vector<std::string> resolved_feature_names_(const pluss::table::DataFrame& frame, const char* where) const;
  [[nodiscard]] static DataFrameBatchOptions batch_options_for_(pluss::msharp::RangeView::Of<std::string> names);
  [[nodiscard]] static pluss::table::DataFrame merge_mapped_(const pluss::table::DataFrame& frame,
                                                             pluss::msharp::RangeView::Of<std::string> names,
                                                             const Series& mapped,
                                                             const char* where);
  void refresh_statistics_();
  ::pluss::ai::nodej::prep::StandardScaler core_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

class OutlierIsolator final {
public:
  explicit OutlierIsolator(OutlierIsolatorOptions options = {})
      : options_(options), core_(options) {}

  OutlierIsolator& features(pluss::msharp::RangeView::Of<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }
  OutlierIsolator& features(std::initializer_list<std::string_view> names) {
    return features(pluss::msharp::RangeView::Of<std::string_view>(names));
  }

  OutlierIsolator& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  [[nodiscard]] pluss::table::Thread infer(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::Thread score_samples(const pluss::table::DataFrame& frame,
                                                   const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::Thread learn_infer(const pluss::table::DataFrame& frame,
                                                 const TrainOptions& learn_options = {},
                                                 const InferOptions& infer_options = {});
  [[nodiscard]] pluss::table::DataFrame filter_inliers(const pluss::table::DataFrame& frame,
                                                       const InferOptions& options = {}) const;

  [[nodiscard]] OutlierIsolatorSummary summary() const {
    return OutlierIsolatorSummary{
        .learned = learned_,
        .feature_count = static_cast<std::int64_t>(feature_names_.size()),
        .estimators = options_.estimators,
        .max_samples = core_.max_samples_used(),
        .max_features = core_.max_features_used(),
        .bootstrap = options_.bootstrap,
        .contamination_auto = options_.contamination_auto,
        .contamination = options_.contamination,
        .offset = core_.decision_offset(),
        .feature_names = feature_names_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  OutlierIsolatorOptions options_{};
  std::vector<std::string> feature_names_{};

private:
  [[nodiscard]] std::vector<std::string> resolved_feature_names_(const pluss::table::DataFrame& frame, const char* where) const;
  [[nodiscard]] static DataFrameBatchOptions batch_options_for_(pluss::msharp::RangeView::Of<std::string> names);
  ::pluss::ai::nodej::prep::OutlierIsolator core_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

#define PLUSS_NODEJ_DECLARE_PREP_WRAPPER(WrapperName, OptionsType)                              \
  class WrapperName final {                                                                      \
  public:                                                                                        \
    explicit WrapperName(OptionsType options = {})                                               \
        : options_(options), core_(options) {}                                                   \
                                                                                                 \
    WrapperName& features(pluss::msharp::RangeView::Of<std::string_view> names) {               \
      feature_names_.clear();                                                                    \
      feature_names_.reserve(names.size());                                                      \
      for (auto name : names) {                                                                  \
        const std::string owned(name);                                                           \
        if (!owned.empty()) feature_names_.push_back(owned);                                     \
      }                                                                                          \
      feature_names_pinned_ = !feature_names_.empty();                                           \
      return *this;                                                                              \
    }                                                                                            \
    WrapperName& features(std::initializer_list<std::string_view> names) {                      \
      return features(pluss::msharp::RangeView::Of<std::string_view>(names));                   \
    }                                                                                            \
                                                                                                 \
    WrapperName& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {}); \
    [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame,             \
                                              const InferOptions& options = {}) const;           \
    [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,       \
                                                    const TrainOptions& learn_options = {},      \
                                                    const InferOptions& infer_options = {});     \
                                                                                                 \
    [[nodiscard]] TransformerSummary summary() const {                                            \
      return TransformerSummary{                                                                  \
          .feature_names = feature_names_,                                                       \
          .state = state_,                                                                       \
      };                                                                                         \
    }                                                                                            \
                                                                                                 \
    [[nodiscard]] bool learned() const noexcept { return learned_; }                             \
    OptionsType options_{};                                                                      \
    std::vector<std::string> feature_names_{};                                                   \
    std::vector<Series> state_{};                                                                \
                                                                                                 \
  private:                                                                                       \
    [[nodiscard]] std::vector<std::string> resolved_feature_names_(                              \
        const pluss::table::DataFrame& frame,                                                    \
        const char* where) const;                                                                \
    [[nodiscard]] static DataFrameBatchOptions batch_options_for_(pluss::msharp::RangeView::Of<std::string> names); \
    [[nodiscard]] static pluss::table::DataFrame merge_mapped_(                                  \
        const pluss::table::DataFrame& frame,                                                    \
        pluss::msharp::RangeView::Of<std::string> names,                                                   \
        const Series& mapped,                                                                    \
        const char* where);                                                                      \
    void refresh_state_();                                                                       \
    ::pluss::ai::nodej::prep::WrapperName core_{};                                               \
    bool feature_names_pinned_{false};                                                           \
    bool learned_{false};                                                                        \
  };

#define PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD(WrapperName, OptionsType)                  \
  class WrapperName final {                                                                      \
  public:                                                                                        \
    explicit WrapperName(OptionsType options = {})                                               \
        : options_(options), core_(options) {}                                                   \
                                                                                                 \
    WrapperName& features(pluss::msharp::RangeView::Of<std::string_view> names) {               \
      feature_names_.clear();                                                                    \
      feature_names_.reserve(names.size());                                                      \
      for (auto name : names) {                                                                  \
        const std::string owned(name);                                                           \
        if (!owned.empty()) feature_names_.push_back(owned);                                     \
      }                                                                                          \
      feature_names_pinned_ = !feature_names_.empty();                                           \
      return *this;                                                                              \
    }                                                                                            \
    WrapperName& features(std::initializer_list<std::string_view> names) {                      \
      return features(pluss::msharp::RangeView::Of<std::string_view>(names));                   \
    }                                                                                            \
                                                                                                 \
    WrapperName& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {}); \
    [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame,             \
                                              const InferOptions& options = {}) const;           \
    [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,       \
                                                    const TrainOptions& learn_options = {},      \
                                                    const InferOptions& infer_options = {});     \
                                                                                                 \
    WrapperName& learn(const pluss::table::Thread& feature, const TrainOptions& options = {});  \
    [[nodiscard]] Series map(const pluss::table::Thread& feature,                                \
                             const InferOptions& options = {}) const;                            \
    [[nodiscard]] Series learn_map(const pluss::table::Thread& feature,                          \
                                   const TrainOptions& learn_options = {},                       \
                                   const InferOptions& infer_options = {});                      \
                                                                                                 \
    [[nodiscard]] TransformerSummary summary() const {                                            \
      return TransformerSummary{                                                                  \
          .feature_names = feature_names_,                                                       \
          .state = state_,                                                                       \
      };                                                                                         \
    }                                                                                            \
                                                                                                 \
    [[nodiscard]] bool learned() const noexcept { return learned_; }                             \
    OptionsType options_{};                                                                      \
    std::vector<std::string> feature_names_{};                                                   \
    std::vector<Series> state_{};                                                                \
    std::vector<std::vector<std::string>> categories_{};                                         \
    std::vector<std::vector<double>> category_keys_{};                                           \
    std::vector<std::vector<double>> frequencies_{};                                             \
    std::vector<std::string> mapped_names_{};                                                    \
                                                                                                 \
  private:                                                                                       \
    [[nodiscard]] std::vector<std::string> resolved_feature_names_(                              \
        const pluss::table::DataFrame& frame,                                                    \
        const char* where) const;                                                                \
    [[nodiscard]] static DataFrameBatchOptions batch_options_for_(pluss::msharp::RangeView::Of<std::string> names); \
    [[nodiscard]] static pluss::table::DataFrame merge_mapped_(                                  \
        const pluss::table::DataFrame& frame,                                                    \
        pluss::msharp::RangeView::Of<std::string> names,                                                   \
        const Series& mapped,                                                                    \
        const char* where);                                                                      \
    void refresh_state_();                                                                       \
    friend void prep_refresh_encoder_attrs_(WrapperName& wrapper);                               \
    friend void prep_before_thread_fit_(WrapperName& wrapper, const pluss::table::Thread& feature); \
    ::pluss::ai::nodej::prep::WrapperName core_{};                                               \
    std::unordered_map<std::uint64_t, std::string> category_labels_by_key_{};                   \
    bool feature_names_pinned_{false};                                                           \
    bool learned_{false};                                                                        \
  };

PLUSS_NODEJ_DECLARE_PREP_WRAPPER(MinMaxScaler, MinMaxScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(RobustScaler, RobustScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(NeighborImputer, NeighborImputerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(IterativeImputer, IterativeImputerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD(OneHotEncoder, OneHotEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD(FrequencyEncoder, FrequencyEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD(OrdinalEncoder, OrdinalEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD(LabelEncoder, LabelEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(VectorNormalizer, VectorNormalizerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(MaxAbsScaler, MaxAbsScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(QuantileMap, QuantileMapOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(PowerMap, PowerMapOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(ThresholdBinarizer, ThresholdBinarizerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(PolynomialLift, PolynomialLiftOptions)

inline void prep_refresh_encoder_attrs_(OneHotEncoder& wrapper);
inline void prep_refresh_encoder_attrs_(FrequencyEncoder& wrapper);
inline void prep_refresh_encoder_attrs_(OrdinalEncoder& wrapper);
inline void prep_refresh_encoder_attrs_(LabelEncoder& wrapper);
inline void prep_before_thread_fit_(OneHotEncoder& wrapper, const pluss::table::Thread& feature);
inline void prep_before_thread_fit_(FrequencyEncoder& wrapper, const pluss::table::Thread& feature);
inline void prep_before_thread_fit_(OrdinalEncoder& wrapper, const pluss::table::Thread& feature);
inline void prep_before_thread_fit_(LabelEncoder& wrapper, const pluss::table::Thread& feature);

#undef PLUSS_NODEJ_DECLARE_PREP_WRAPPER
#undef PLUSS_NODEJ_DECLARE_PREP_WRAPPER_WITH_THREAD

class CategoryIndexer final {
public:
  explicit CategoryIndexer(CategoryIndexerOptions options = {})
      : options_(options) {}

  CategoryIndexer& features(pluss::msharp::RangeView::Of<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }
  CategoryIndexer& features(std::initializer_list<std::string_view> names) {
    return features(pluss::msharp::RangeView::Of<std::string_view>(names));
  }

  CategoryIndexer& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  CategoryIndexer& learn(const pluss::table::Thread& feature, const TrainOptions& options = {});

  [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] Series map(const pluss::table::Thread& feature, const InferOptions& options = {}) const;

  [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,
                                                  const TrainOptions& learn_options = {},
                                                  const InferOptions& infer_options = {});
  [[nodiscard]] Series learn_map(const pluss::table::Thread& feature,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {});

  [[nodiscard]] CategoryIndexerSummary summary() const {
    return CategoryIndexerSummary{
        .learned = learned_,
        .feature_count = static_cast<std::int64_t>(feature_names_.size()),
        .include_unknown_bucket = options_.include_unknown_bucket,
        .one_based = options_.one_based,
        .feature_names = feature_names_,
        .mapped_names = mapped_names_,
        .categories = categories_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  CategoryIndexerOptions options_{};
  std::vector<std::string> feature_names_{};
  std::vector<std::string> mapped_names_{};
  std::vector<std::vector<std::string>> categories_{};

private:
  [[nodiscard]] static OneHotEncoderOptions to_one_hot_options_(const CategoryIndexerOptions& options);
  [[nodiscard]] static Series encode_feature_(const OneHotEncoder& encoder,
                                              const pluss::table::Thread& feature,
                                              bool one_based,
                                              const InferOptions& options,
                                              const char* where);
  void refresh_from_encoders_();

  std::vector<OneHotEncoder> encoders_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

[[nodiscard]] inline SimpleImputer simple_imputer(SimpleImputerOptions options = {}) { return SimpleImputer(options); }
[[nodiscard]] inline StandardScaler standard_scaler(StandardScalerOptions options = {}) { return StandardScaler(options); }
[[nodiscard]] inline MinMaxScaler minmax_scaler(MinMaxScalerOptions options = {}) { return MinMaxScaler(options); }
[[nodiscard]] inline RobustScaler robust_scaler(RobustScalerOptions options = {}) { return RobustScaler(options); }
[[nodiscard]] inline NeighborImputer neighbor_imputer(NeighborImputerOptions options = {}) { return NeighborImputer(options); }
[[nodiscard]] inline IterativeImputer iterative_imputer(IterativeImputerOptions options = {}) { return IterativeImputer(options); }
[[nodiscard]] inline OneHotEncoder one_hot_encoder(OneHotEncoderOptions options = {}) { return OneHotEncoder(options); }
[[nodiscard]] inline FrequencyEncoder frequency_encoder(FrequencyEncoderOptions options = {}) { return FrequencyEncoder(options); }
[[nodiscard]] inline OrdinalEncoder ordinal_encoder(OrdinalEncoderOptions options = {}) { return OrdinalEncoder(options); }
[[nodiscard]] inline LabelEncoder label_encoder(LabelEncoderOptions options = {}) { return LabelEncoder(options); }
[[nodiscard]] inline VectorNormalizer vector_normalizer(VectorNormalizerOptions options = {}) { return VectorNormalizer(options); }
[[nodiscard]] inline MaxAbsScaler maxabs_scaler(MaxAbsScalerOptions options = {}) { return MaxAbsScaler(options); }
[[nodiscard]] inline QuantileMap quantile_map(QuantileMapOptions options = {}) { return QuantileMap(options); }
[[nodiscard]] inline PowerMap power_map(PowerMapOptions options = {}) { return PowerMap(options); }
[[nodiscard]] inline ThresholdBinarizer threshold_binarizer(ThresholdBinarizerOptions options = {}) {
  return ThresholdBinarizer(options);
}
[[nodiscard]] inline PolynomialLift polynomial_lift(PolynomialLiftOptions options = {}) { return PolynomialLift(options); }
[[nodiscard]] inline CategoryIndexer category_indexer(CategoryIndexerOptions options = {}) { return CategoryIndexer(options); }
[[nodiscard]] inline OutlierIsolator outlier_isolator(OutlierIsolatorOptions options = {}) {
  return OutlierIsolator(options);
}

} // namespace prep

} // namespace pluss::ai::lymph
