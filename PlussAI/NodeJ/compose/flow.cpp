#include "pluss/ai/nodej/compose.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pluss::ai::nodej {
namespace {

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

[[nodiscard]] pluss::Config index_config_on(::pluss::Platform platform) {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] Series make_index_series(std::vector<std::int64_t> ids, ::pluss::Platform platform) {
  return pluss::make_series(std::move(ids), {static_cast<std::int64_t>(ids.size())}, index_config_on(platform));
}

[[nodiscard]] std::vector<std::int64_t> route_vec(pluss::msharp::RangeView::Int feature_route) {
  std::vector<std::int64_t> out;
  out.reserve(feature_route.size());
  out.insert(out.end(), feature_route.begin(), feature_route.end());
  return out;
}

[[nodiscard]] const char* transform_kind_name(TransformKind kind) {
  switch (kind) {
    case TransformKind::StandardScaler: return "standard_scaler";
    case TransformKind::MinMaxScaler: return "minmax_scaler";
    case TransformKind::RobustScaler: return "robust_scaler";
    case TransformKind::SimpleImputer: return "simple_imputer";
    case TransformKind::NeighborImputer: return "neighbor_imputer";
    case TransformKind::IterativeImputer: return "iterative_imputer";
    case TransformKind::OneHotEncoder: return "one_hot_encoder";
    case TransformKind::FrequencyEncoder: return "frequency_encoder";
    case TransformKind::VectorNormalizer: return "vector_normalizer";
    case TransformKind::MaxAbsScaler: return "maxabs_scaler";
    case TransformKind::QuantileMap: return "quantile_map";
    case TransformKind::PowerMap: return "power_map";
    case TransformKind::ThresholdBinarizer: return "threshold_binarizer";
    case TransformKind::PolynomialLift: return "polynomial_lift";
    case TransformKind::OrdinalEncoder: return "ordinal_encoder";
    case TransformKind::LabelEncoder: return "label_encoder";
    case TransformKind::BaseNEncoder: return "basen_encoder";
    case TransformKind::HashingEncoder: return "hashing_encoder";
    case TransformKind::TargetEncoder: return "target_encoder";
    case TransformKind::WOEEncoder: return "woe_encoder";
  }
  return "unknown";
}

[[nodiscard]] std::optional<TransformKind> parse_transform_kind(std::string_view text) {
  if (text == "standard_scaler") return TransformKind::StandardScaler;
  if (text == "minmax_scaler") return TransformKind::MinMaxScaler;
  if (text == "robust_scaler") return TransformKind::RobustScaler;
  if (text == "simple_imputer") return TransformKind::SimpleImputer;
  if (text == "neighbor_imputer") return TransformKind::NeighborImputer;
  if (text == "iterative_imputer") return TransformKind::IterativeImputer;
  if (text == "one_hot_encoder") return TransformKind::OneHotEncoder;
  if (text == "frequency_encoder") return TransformKind::FrequencyEncoder;
  if (text == "vector_normalizer") return TransformKind::VectorNormalizer;
  if (text == "maxabs_scaler") return TransformKind::MaxAbsScaler;
  if (text == "quantile_map") return TransformKind::QuantileMap;
  if (text == "power_map") return TransformKind::PowerMap;
  if (text == "threshold_binarizer") return TransformKind::ThresholdBinarizer;
  if (text == "polynomial_lift") return TransformKind::PolynomialLift;
  if (text == "ordinal_encoder") return TransformKind::OrdinalEncoder;
  if (text == "label_encoder") return TransformKind::LabelEncoder;
  if (text == "basen_encoder") return TransformKind::BaseNEncoder;
  if (text == "hashing_encoder") return TransformKind::HashingEncoder;
  if (text == "target_encoder") return TransformKind::TargetEncoder;
  if (text == "woe_encoder") return TransformKind::WOEEncoder;
  return std::nullopt;
}

[[nodiscard]] const char* remainder_name(RouteRemainderPolicy policy) {
  return policy == RouteRemainderPolicy::Drop ? "drop" : "pass";
}

[[nodiscard]] std::optional<RouteRemainderPolicy> parse_remainder(std::string_view text) {
  if (text == "drop") return RouteRemainderPolicy::Drop;
  if (text == "pass") return RouteRemainderPolicy::PassThrough;
  return std::nullopt;
}

template <class T>
[[nodiscard]] std::optional<T> parse_scalar(std::string_view text) {
  if (text.empty()) return std::nullopt;
  std::string tmp(text);
  if constexpr (std::is_same_v<T, bool>) {
    if (tmp == "1" || tmp == "true") return true;
    if (tmp == "0" || tmp == "false") return false;
    return std::nullopt;
  } else if constexpr (std::is_integral_v<T>) {
    T out{};
    auto* begin = tmp.data();
    auto* end = tmp.data() + tmp.size();
    const auto [ptr, ec] = std::from_chars(begin, end, out);
    if (ec != std::errc{} || ptr != end) return std::nullopt;
    return out;
  } else {
    char* end_ptr = nullptr;
    const double value = std::strtod(tmp.c_str(), &end_ptr);
    if (end_ptr == tmp.c_str() || end_ptr != (tmp.c_str() + static_cast<std::ptrdiff_t>(tmp.size()))) {
      return std::nullopt;
    }
    if constexpr (std::is_same_v<T, float>) return static_cast<float>(value);
    return static_cast<T>(value);
  }
}

// std::get<T>(options) below would throw std::bad_variant_access (an
// uncaught exception, not a Status) if a caller's `options` alternative does
// not match `kind` -- e.g. add_named_transform(StandardScaler,
// OneHotEncoderOptions{}). Validate the active alternative first so a
// mismatch is reported the same way every other input error is: a Status.
template <class Options>
[[nodiscard]] StatusOr<Options> require_options(TransformKind kind, const TransformOptions& options) {
  if (!std::holds_alternative<Options>(options)) {
    return StatusOr<Options>(Status::invalid_argument(
        std::string("Flow::add_named_transform: options does not match kind=") +
        transform_kind_name(kind) + "."));
  }
  return StatusOr<Options>(std::get<Options>(options));
}

[[nodiscard]] StatusOr<std::unique_ptr<TransformerBase>> make_transformer_from_spec(TransformKind kind,
                                                                                    const TransformOptions& options) {
  switch (kind) {
    case TransformKind::StandardScaler: {
      StatusOr<prep::StandardScalerOptions> opts = require_options<prep::StandardScalerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::StandardScaler>(opts.value()));
    }
    case TransformKind::MinMaxScaler: {
      StatusOr<prep::MinMaxScalerOptions> opts = require_options<prep::MinMaxScalerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::MinMaxScaler>(opts.value()));
    }
    case TransformKind::RobustScaler: {
      StatusOr<prep::RobustScalerOptions> opts = require_options<prep::RobustScalerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::RobustScaler>(opts.value()));
    }
    case TransformKind::SimpleImputer: {
      StatusOr<prep::SimpleImputerOptions> opts = require_options<prep::SimpleImputerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::SimpleImputer>(opts.value()));
    }
    case TransformKind::NeighborImputer: {
      StatusOr<prep::NeighborImputerOptions> opts = require_options<prep::NeighborImputerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::NeighborImputer>(opts.value()));
    }
    case TransformKind::IterativeImputer: {
      StatusOr<prep::IterativeImputerOptions> opts = require_options<prep::IterativeImputerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::IterativeImputer>(opts.value()));
    }
    case TransformKind::OneHotEncoder: {
      StatusOr<prep::OneHotEncoderOptions> opts = require_options<prep::OneHotEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::OneHotEncoder>(opts.value()));
    }
    case TransformKind::FrequencyEncoder: {
      StatusOr<prep::FrequencyEncoderOptions> opts = require_options<prep::FrequencyEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::FrequencyEncoder>(opts.value()));
    }
    case TransformKind::VectorNormalizer: {
      StatusOr<prep::VectorNormalizerOptions> opts = require_options<prep::VectorNormalizerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::VectorNormalizer>(opts.value()));
    }
    case TransformKind::MaxAbsScaler: {
      StatusOr<prep::MaxAbsScalerOptions> opts = require_options<prep::MaxAbsScalerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::MaxAbsScaler>(opts.value()));
    }
    case TransformKind::QuantileMap: {
      StatusOr<prep::QuantileMapOptions> opts = require_options<prep::QuantileMapOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::QuantileMap>(opts.value()));
    }
    case TransformKind::PowerMap: {
      StatusOr<prep::PowerMapOptions> opts = require_options<prep::PowerMapOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::PowerMap>(opts.value()));
    }
    case TransformKind::ThresholdBinarizer: {
      StatusOr<prep::ThresholdBinarizerOptions> opts = require_options<prep::ThresholdBinarizerOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::ThresholdBinarizer>(opts.value()));
    }
    case TransformKind::PolynomialLift: {
      StatusOr<prep::PolynomialLiftOptions> opts = require_options<prep::PolynomialLiftOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::PolynomialLift>(opts.value()));
    }
    case TransformKind::OrdinalEncoder: {
      StatusOr<prep::OrdinalEncoderOptions> opts = require_options<prep::OrdinalEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::OrdinalEncoder>(opts.value()));
    }
    case TransformKind::LabelEncoder: {
      StatusOr<prep::LabelEncoderOptions> opts = require_options<prep::LabelEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::LabelEncoder>(opts.value()));
    }
    case TransformKind::BaseNEncoder: {
      StatusOr<prep::BaseNEncoderOptions> opts = require_options<prep::BaseNEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::BaseNEncoder>(opts.value()));
    }
    case TransformKind::HashingEncoder: {
      StatusOr<prep::HashingEncoderOptions> opts = require_options<prep::HashingEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::HashingEncoder>(opts.value()));
    }
    case TransformKind::TargetEncoder: {
      StatusOr<prep::TargetEncoderOptions> opts = require_options<prep::TargetEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::TargetEncoder>(opts.value()));
    }
    case TransformKind::WOEEncoder: {
      StatusOr<prep::WOEEncoderOptions> opts = require_options<prep::WOEEncoderOptions>(kind, options);
      if (!opts.ok()) return StatusOr<std::unique_ptr<TransformerBase>>(opts.status());
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::WOEEncoder>(opts.value()));
    }
  }
  return StatusOr<std::unique_ptr<TransformerBase>>(Status::invalid_argument("Unsupported transform kind."));
}

[[nodiscard]] std::vector<std::string> split_sv(std::string_view text, char delim) {
  std::vector<std::string> out{};
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t end = text.find(delim, start);
    if (end == std::string_view::npos) {
      out.emplace_back(text.substr(start));
      break;
    }
    out.emplace_back(text.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

[[nodiscard]] std::map<std::string, std::string> parse_option_map(std::string_view text) {
  std::map<std::string, std::string> out{};
  if (text.empty()) return out;
  const std::vector<std::string> parts = split_sv(text, ';');
  for (const std::string& kv : parts) {
    if (kv.empty()) continue;
    const std::size_t eq = kv.find('=');
    if (eq == std::string::npos) continue;
    out.emplace(kv.substr(0, eq), kv.substr(eq + 1));
  }
  return out;
}

[[nodiscard]] std::string scalar_text(double value) {
  std::ostringstream oss;
  oss << std::setprecision(17) << value;
  return oss.str();
}

[[nodiscard]] bool to_flag(double value) {
  return value >= 0.5;
}

[[nodiscard]] StatusOr<std::int64_t> resolve_stage_index(const std::vector<TransformSpec>& specs, std::string_view stage) {
  for (std::size_t i = 0; i < specs.size(); ++i) {
    if (specs[i].stage_key == stage || ("stage" + std::to_string(i)) == stage) {
      return StatusOr<std::int64_t>(static_cast<std::int64_t>(i));
    }
  }
  return StatusOr<std::int64_t>(Status::invalid_argument("could not resolve stage key."));
}

void append_transform_option_param_keys(const TransformOptions& options, const std::string& prefix, std::vector<std::string>& out) {
  std::visit(
      [&](const auto& opt) {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, prep::StandardScalerOptions>) {
          out.push_back(prefix + "with_centering");
          out.push_back(prefix + "with_scaling");
          out.push_back(prefix + "variance_floor");
        } else if constexpr (std::is_same_v<Opt, prep::MinMaxScalerOptions>) {
          out.push_back(prefix + "lower");
          out.push_back(prefix + "upper");
          out.push_back(prefix + "clip");
          out.push_back(prefix + "range_floor");
        } else if constexpr (std::is_same_v<Opt, prep::RobustScalerOptions>) {
          out.push_back(prefix + "with_centering");
          out.push_back(prefix + "with_scaling");
          out.push_back(prefix + "lower_quantile");
          out.push_back(prefix + "upper_quantile");
          out.push_back(prefix + "range_floor");
        } else if constexpr (std::is_same_v<Opt, prep::SimpleImputerOptions>) {
          out.push_back(prefix + "strategy");
          out.push_back(prefix + "constant_value");
          out.push_back(prefix + "empty_feature_fallback");
        } else if constexpr (std::is_same_v<Opt, prep::OneHotEncoderOptions>) {
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
          out.push_back(prefix + "include_unknown_bucket");
        } else if constexpr (std::is_same_v<Opt, prep::FrequencyEncoderOptions>) {
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
          out.push_back(prefix + "unknown_frequency");
        } else if constexpr (std::is_same_v<Opt, prep::OrdinalEncoderOptions>) {
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
          out.push_back(prefix + "include_unknown_bucket");
          out.push_back(prefix + "one_based");
        } else if constexpr (std::is_same_v<Opt, prep::LabelEncoderOptions>) {
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
          out.push_back(prefix + "include_unknown_bucket");
          out.push_back(prefix + "one_based");
        } else if constexpr (std::is_same_v<Opt, prep::NeighborImputerOptions>) {
          out.push_back(prefix + "neighbors");
          out.push_back(prefix + "weighting");
          out.push_back(prefix + "distance_floor");
          out.push_back(prefix + "empty_feature_fallback");
        } else if constexpr (std::is_same_v<Opt, prep::IterativeImputerOptions>) {
          out.push_back(prefix + "rounds");
          out.push_back(prefix + "ridge");
          out.push_back(prefix + "empty_feature_fallback");
        } else if constexpr (std::is_same_v<Opt, prep::VectorNormalizerOptions>) {
          out.push_back(prefix + "norm");
          out.push_back(prefix + "floor");
        } else if constexpr (std::is_same_v<Opt, prep::MaxAbsScalerOptions>) {
          out.push_back(prefix + "floor");
        } else if constexpr (std::is_same_v<Opt, prep::QuantileMapOptions>) {
          out.push_back(prefix + "output");
          out.push_back(prefix + "clip");
        } else if constexpr (std::is_same_v<Opt, prep::PowerMapOptions>) {
          out.push_back(prefix + "mode");
          out.push_back(prefix + "standardize");
          out.push_back(prefix + "lambda");
          out.push_back(prefix + "lambda_is_set");
          out.push_back(prefix + "lambda_min");
          out.push_back(prefix + "lambda_max");
          out.push_back(prefix + "lambda_grid");
          out.push_back(prefix + "variance_floor");
        } else if constexpr (std::is_same_v<Opt, prep::ThresholdBinarizerOptions>) {
          out.push_back(prefix + "threshold");
          out.push_back(prefix + "low_value");
          out.push_back(prefix + "high_value");
        } else if constexpr (std::is_same_v<Opt, prep::PolynomialLiftOptions>) {
          out.push_back(prefix + "degree");
          out.push_back(prefix + "interactions_only");
          out.push_back(prefix + "include_bias");
        } else if constexpr (std::is_same_v<Opt, prep::BaseNEncoderOptions>) {
          out.push_back(prefix + "base");
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
        } else if constexpr (std::is_same_v<Opt, prep::HashingEncoderOptions>) {
          out.push_back(prefix + "n_components");
          out.push_back(prefix + "seed");
        } else if constexpr (std::is_same_v<Opt, prep::TargetEncoderOptions>) {
          out.push_back(prefix + "smoothing");
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
        } else if constexpr (std::is_same_v<Opt, prep::WOEEncoderOptions>) {
          out.push_back(prefix + "regularization");
          out.push_back(prefix + "max_categories");
          out.push_back(prefix + "overflow");
        }
      },
      options);
}

[[nodiscard]] StatusOr<double> transform_option_param(const TransformOptions& options, std::string_view param) {
  return std::visit(
      [&](const auto& opt) -> StatusOr<double> {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, prep::StandardScalerOptions>) {
          if (param == "with_centering") return StatusOr<double>(opt.with_centering ? 1.0 : 0.0);
          if (param == "with_scaling") return StatusOr<double>(opt.with_scaling ? 1.0 : 0.0);
          if (param == "variance_floor") return StatusOr<double>(opt.variance_floor);
        } else if constexpr (std::is_same_v<Opt, prep::MinMaxScalerOptions>) {
          if (param == "lower") return StatusOr<double>(opt.lower);
          if (param == "upper") return StatusOr<double>(opt.upper);
          if (param == "clip") return StatusOr<double>(opt.clip ? 1.0 : 0.0);
          if (param == "range_floor") return StatusOr<double>(opt.range_floor);
        } else if constexpr (std::is_same_v<Opt, prep::RobustScalerOptions>) {
          if (param == "with_centering") return StatusOr<double>(opt.with_centering ? 1.0 : 0.0);
          if (param == "with_scaling") return StatusOr<double>(opt.with_scaling ? 1.0 : 0.0);
          if (param == "lower_quantile") return StatusOr<double>(opt.lower_quantile);
          if (param == "upper_quantile") return StatusOr<double>(opt.upper_quantile);
          if (param == "range_floor") return StatusOr<double>(opt.range_floor);
        } else if constexpr (std::is_same_v<Opt, prep::SimpleImputerOptions>) {
          if (param == "strategy") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.strategy)));
          if (param == "constant_value") return StatusOr<double>(opt.constant_value);
          if (param == "empty_feature_fallback") return StatusOr<double>(opt.empty_feature_fallback);
        } else if constexpr (std::is_same_v<Opt, prep::OneHotEncoderOptions>) {
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
          if (param == "include_unknown_bucket") return StatusOr<double>(opt.include_unknown_bucket ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, prep::FrequencyEncoderOptions>) {
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
          if (param == "unknown_frequency") return StatusOr<double>(opt.unknown_frequency);
        } else if constexpr (std::is_same_v<Opt, prep::OrdinalEncoderOptions>) {
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
          if (param == "include_unknown_bucket") return StatusOr<double>(opt.include_unknown_bucket ? 1.0 : 0.0);
          if (param == "one_based") return StatusOr<double>(opt.one_based ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, prep::LabelEncoderOptions>) {
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
          if (param == "include_unknown_bucket") return StatusOr<double>(opt.include_unknown_bucket ? 1.0 : 0.0);
          if (param == "one_based") return StatusOr<double>(opt.one_based ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, prep::NeighborImputerOptions>) {
          if (param == "neighbors") return StatusOr<double>(static_cast<double>(opt.neighbors));
          if (param == "weighting") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.weighting)));
          if (param == "distance_floor") return StatusOr<double>(opt.distance_floor);
          if (param == "empty_feature_fallback") return StatusOr<double>(opt.empty_feature_fallback);
        } else if constexpr (std::is_same_v<Opt, prep::IterativeImputerOptions>) {
          if (param == "rounds") return StatusOr<double>(static_cast<double>(opt.rounds));
          if (param == "ridge") return StatusOr<double>(opt.ridge);
          if (param == "empty_feature_fallback") return StatusOr<double>(opt.empty_feature_fallback);
        } else if constexpr (std::is_same_v<Opt, prep::VectorNormalizerOptions>) {
          if (param == "norm") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.norm)));
          if (param == "floor") return StatusOr<double>(opt.floor);
        } else if constexpr (std::is_same_v<Opt, prep::MaxAbsScalerOptions>) {
          if (param == "floor") return StatusOr<double>(opt.floor);
        } else if constexpr (std::is_same_v<Opt, prep::QuantileMapOptions>) {
          if (param == "output") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.output)));
          if (param == "clip") return StatusOr<double>(opt.clip);
        } else if constexpr (std::is_same_v<Opt, prep::PowerMapOptions>) {
          if (param == "mode") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.mode)));
          if (param == "standardize") return StatusOr<double>(opt.standardize ? 1.0 : 0.0);
          if (param == "lambda") {
            if (!opt.lambda.has_value()) return StatusOr<double>(Status::invalid_argument("lambda is not set."));
            return StatusOr<double>(opt.lambda.value());
          }
          if (param == "lambda_is_set") return StatusOr<double>(opt.lambda.has_value() ? 1.0 : 0.0);
          if (param == "lambda_min") return StatusOr<double>(opt.lambda_min);
          if (param == "lambda_max") return StatusOr<double>(opt.lambda_max);
          if (param == "lambda_grid") return StatusOr<double>(static_cast<double>(opt.lambda_grid));
          if (param == "variance_floor") return StatusOr<double>(opt.variance_floor);
        } else if constexpr (std::is_same_v<Opt, prep::ThresholdBinarizerOptions>) {
          if (param == "threshold") return StatusOr<double>(opt.threshold);
          if (param == "low_value") return StatusOr<double>(opt.low_value);
          if (param == "high_value") return StatusOr<double>(opt.high_value);
        } else if constexpr (std::is_same_v<Opt, prep::PolynomialLiftOptions>) {
          if (param == "degree") return StatusOr<double>(static_cast<double>(opt.degree));
          if (param == "interactions_only") return StatusOr<double>(opt.interactions_only ? 1.0 : 0.0);
          if (param == "include_bias") return StatusOr<double>(opt.include_bias ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, prep::BaseNEncoderOptions>) {
          if (param == "base") return StatusOr<double>(static_cast<double>(opt.base));
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
        } else if constexpr (std::is_same_v<Opt, prep::HashingEncoderOptions>) {
          if (param == "n_components") return StatusOr<double>(static_cast<double>(opt.n_components));
          if (param == "seed") return StatusOr<double>(static_cast<double>(opt.seed));
        } else if constexpr (std::is_same_v<Opt, prep::TargetEncoderOptions>) {
          if (param == "smoothing") return StatusOr<double>(opt.smoothing);
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
        } else if constexpr (std::is_same_v<Opt, prep::WOEEncoderOptions>) {
          if (param == "regularization") return StatusOr<double>(opt.regularization);
          if (param == "max_categories") return StatusOr<double>(static_cast<double>(opt.max_categories));
          if (param == "overflow") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.overflow)));
        }
        return StatusOr<double>(Status::invalid_argument("unsupported option parameter for this transform."));
      },
      options);
}

Status set_transform_option_param(TransformOptions& options, std::string_view param, double value) {
  return std::visit(
      [&](auto& opt) -> Status {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, prep::StandardScalerOptions>) {
          if (param == "with_centering") { opt.with_centering = to_flag(value); return Status::success(); }
          if (param == "with_scaling") { opt.with_scaling = to_flag(value); return Status::success(); }
          if (param == "variance_floor") { opt.variance_floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::MinMaxScalerOptions>) {
          if (param == "lower") { opt.lower = value; return Status::success(); }
          if (param == "upper") { opt.upper = value; return Status::success(); }
          if (param == "clip") { opt.clip = to_flag(value); return Status::success(); }
          if (param == "range_floor") { opt.range_floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::RobustScalerOptions>) {
          if (param == "with_centering") { opt.with_centering = to_flag(value); return Status::success(); }
          if (param == "with_scaling") { opt.with_scaling = to_flag(value); return Status::success(); }
          if (param == "lower_quantile") { opt.lower_quantile = value; return Status::success(); }
          if (param == "upper_quantile") { opt.upper_quantile = value; return Status::success(); }
          if (param == "range_floor") { opt.range_floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::SimpleImputerOptions>) {
          if (param == "strategy") { opt.strategy = static_cast<prep::SimpleImputerStrategy>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "constant_value") { opt.constant_value = value; return Status::success(); }
          if (param == "empty_feature_fallback") { opt.empty_feature_fallback = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::OneHotEncoderOptions>) {
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "include_unknown_bucket") { opt.include_unknown_bucket = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::FrequencyEncoderOptions>) {
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "unknown_frequency") { opt.unknown_frequency = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::OrdinalEncoderOptions>) {
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "include_unknown_bucket") { opt.include_unknown_bucket = to_flag(value); return Status::success(); }
          if (param == "one_based") { opt.one_based = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::LabelEncoderOptions>) {
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "include_unknown_bucket") { opt.include_unknown_bucket = to_flag(value); return Status::success(); }
          if (param == "one_based") { opt.one_based = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::NeighborImputerOptions>) {
          if (param == "neighbors") { opt.neighbors = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "weighting") { opt.weighting = static_cast<prep::NeighborImputerWeighting>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "distance_floor") { opt.distance_floor = value; return Status::success(); }
          if (param == "empty_feature_fallback") { opt.empty_feature_fallback = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::IterativeImputerOptions>) {
          if (param == "rounds") { opt.rounds = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "ridge") { opt.ridge = value; return Status::success(); }
          if (param == "empty_feature_fallback") { opt.empty_feature_fallback = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::VectorNormalizerOptions>) {
          if (param == "norm") { opt.norm = static_cast<prep::VectorNorm>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "floor") { opt.floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::MaxAbsScalerOptions>) {
          if (param == "floor") { opt.floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::QuantileMapOptions>) {
          if (param == "output") { opt.output = static_cast<prep::QuantileMapOutput>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "clip") { opt.clip = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::PowerMapOptions>) {
          if (param == "mode") { opt.mode = static_cast<prep::PowerMapMode>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (param == "standardize") { opt.standardize = to_flag(value); return Status::success(); }
          if (param == "lambda") { opt.lambda = value; return Status::success(); }
          if (param == "lambda_is_set") { if (!to_flag(value)) opt.lambda.reset(); return Status::success(); }
          if (param == "lambda_min") { opt.lambda_min = value; return Status::success(); }
          if (param == "lambda_max") { opt.lambda_max = value; return Status::success(); }
          if (param == "lambda_grid") { opt.lambda_grid = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "variance_floor") { opt.variance_floor = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::ThresholdBinarizerOptions>) {
          if (param == "threshold") { opt.threshold = value; return Status::success(); }
          if (param == "low_value") { opt.low_value = value; return Status::success(); }
          if (param == "high_value") { opt.high_value = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::PolynomialLiftOptions>) {
          if (param == "degree") { opt.degree = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "interactions_only") { opt.interactions_only = to_flag(value); return Status::success(); }
          if (param == "include_bias") { opt.include_bias = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::BaseNEncoderOptions>) {
          if (param == "base") { opt.base = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::HashingEncoderOptions>) {
          if (param == "n_components") { opt.n_components = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "seed") { opt.seed = static_cast<std::uint64_t>(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::TargetEncoderOptions>) {
          if (param == "smoothing") { opt.smoothing = value; return Status::success(); }
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, prep::WOEEncoderOptions>) {
          if (param == "regularization") { opt.regularization = value; return Status::success(); }
          if (param == "max_categories") { opt.max_categories = static_cast<std::int64_t>(value); return Status::success(); }
          if (param == "overflow") { opt.overflow = static_cast<prep::CategoryOverflowPolicy>(static_cast<std::uint8_t>(value)); return Status::success(); }
        }
        return Status::invalid_argument("unsupported option parameter for this transform.");
      },
      options);
}

[[nodiscard]] std::string serialize_transform_options(const TransformOptions& options) {
  std::vector<std::string> keys{};
  append_transform_option_param_keys(options, "", keys);
  std::ostringstream oss;
  bool first = true;
  for (const std::string& key : keys) {
    StatusOr<double> value_or = transform_option_param(options, key);
    if (!value_or.ok()) continue;
    if (!first) oss << ';';
    first = false;
    oss << key << "=" << scalar_text(value_or.value());
  }
  return oss.str();
}

[[nodiscard]] StatusOr<TransformOptions> parse_transform_options(TransformKind kind, std::string_view text) {
  TransformOptions options{};
  switch (kind) {
    case TransformKind::StandardScaler: options = prep::StandardScalerOptions{}; break;
    case TransformKind::MinMaxScaler: options = prep::MinMaxScalerOptions{}; break;
    case TransformKind::RobustScaler: options = prep::RobustScalerOptions{}; break;
    case TransformKind::SimpleImputer: options = prep::SimpleImputerOptions{}; break;
    case TransformKind::NeighborImputer: options = prep::NeighborImputerOptions{}; break;
    case TransformKind::IterativeImputer: options = prep::IterativeImputerOptions{}; break;
    case TransformKind::OneHotEncoder: options = prep::OneHotEncoderOptions{}; break;
    case TransformKind::FrequencyEncoder: options = prep::FrequencyEncoderOptions{}; break;
    case TransformKind::VectorNormalizer: options = prep::VectorNormalizerOptions{}; break;
    case TransformKind::MaxAbsScaler: options = prep::MaxAbsScalerOptions{}; break;
    case TransformKind::QuantileMap: options = prep::QuantileMapOptions{}; break;
    case TransformKind::PowerMap: options = prep::PowerMapOptions{}; break;
    case TransformKind::ThresholdBinarizer: options = prep::ThresholdBinarizerOptions{}; break;
    case TransformKind::PolynomialLift: options = prep::PolynomialLiftOptions{}; break;
    case TransformKind::OrdinalEncoder: options = prep::OrdinalEncoderOptions{}; break;
    case TransformKind::LabelEncoder: options = prep::LabelEncoderOptions{}; break;
    case TransformKind::BaseNEncoder: options = prep::BaseNEncoderOptions{}; break;
    case TransformKind::HashingEncoder: options = prep::HashingEncoderOptions{}; break;
    case TransformKind::TargetEncoder: options = prep::TargetEncoderOptions{}; break;
    case TransformKind::WOEEncoder: options = prep::WOEEncoderOptions{}; break;
  }
  if (text.empty()) return StatusOr<TransformOptions>(options);
  const std::map<std::string, std::string> kv = parse_option_map(text);
  for (const auto& [k, v] : kv) {
    const std::optional<double> parsed = parse_scalar<double>(v);
    if (!parsed.has_value()) {
      return StatusOr<TransformOptions>(Status::invalid_argument("Flow::load_blueprint option parse error for key '" + k + "'."));
    }
    Status status = set_transform_option_param(options, k, parsed.value());
    if (!status.ok()) {
      return StatusOr<TransformOptions>(Status::invalid_argument("Flow::load_blueprint unsupported option key '" + k + "'."));
    }
  }
  return StatusOr<TransformOptions>(std::move(options));
}

void append_model_option_param_keys(const ModelSpec& spec, std::vector<std::string>& out) {
  out.push_back("model::task");
  out.push_back("model::family");
  std::visit(
      [&](const auto& opt) {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, LinearFamilyOptions>) {
          out.push_back("model::opt.objective");
          out.push_back("model::opt.regularization");
          out.push_back("model::opt.regularization_strength");
          out.push_back("model::opt.elastic_l1_ratio");
          out.push_back("model::opt.robust_transition");
          out.push_back("model::opt.fit_intercept");
        } else if constexpr (std::is_same_v<Opt, TreeFamilyOptions>) {
          out.push_back("model::opt.criterion");
          out.push_back("model::opt.max_depth");
          out.push_back("model::opt.min_samples_leaf");
          out.push_back("model::opt.min_samples_split");
        } else if constexpr (std::is_same_v<Opt, EnsembleFamilyOptions>) {
          out.push_back("model::opt.method");
          out.push_back("model::opt.learners");
          out.push_back("model::opt.subsample");
          out.push_back("model::opt.learning_rate");
        } else if constexpr (std::is_same_v<Opt, NeighborFamilyOptions>) {
          out.push_back("model::opt.k");
          out.push_back("model::opt.query_mode");
          out.push_back("model::opt.radius");
          out.push_back("model::opt.radius_fallback_k");
          out.push_back("model::opt.distance_weighted");
          out.push_back("model::opt.standardize");
        } else if constexpr (std::is_same_v<Opt, KernelFamilyOptions>) {
          out.push_back("model::opt.kernel");
          out.push_back("model::opt.form");
          out.push_back("model::opt.c");
          out.push_back("model::opt.gamma");
          out.push_back("model::opt.epsilon");
          out.push_back("model::opt.nu");
        } else if constexpr (std::is_same_v<Opt, BayesFamilyOptions>) {
          out.push_back("model::opt.likelihood");
          out.push_back("model::opt.use_gaussian");
          out.push_back("model::opt.smoothing");
        } else if constexpr (std::is_same_v<Opt, ClusterFamilyOptions>) {
          out.push_back("model::opt.method");
          out.push_back("model::opt.clusters");
          out.push_back("model::opt.max_iterations");
          out.push_back("model::opt.restarts");
          out.push_back("model::opt.tolerance");
        } else if constexpr (std::is_same_v<Opt, DecomposeFamilyOptions>) {
          out.push_back("model::opt.method");
          out.push_back("model::opt.components");
          out.push_back("model::opt.whiten");
          out.push_back("model::opt.max_iterations");
          out.push_back("model::opt.tolerance");
        } else if constexpr (std::is_same_v<Opt, DetectFamilyOptions>) {
          out.push_back("model::opt.method");
          out.push_back("model::opt.estimators");
          out.push_back("model::opt.max_samples");
          out.push_back("model::opt.max_samples_auto");
          out.push_back("model::opt.max_features_ratio");
          out.push_back("model::opt.bootstrap");
          out.push_back("model::opt.contamination_auto");
          out.push_back("model::opt.contamination");
        }
      },
      spec.options);
}

[[nodiscard]] StatusOr<double> model_param(const ModelSpec& spec, std::string_view param) {
  if (param == "task") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(spec.task)));
  if (param == "family") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(spec.family)));
  if (!param.starts_with("opt.")) return StatusOr<double>(Status::invalid_argument("unsupported model param key."));
  const std::string key(param.substr(4));
  return std::visit(
      [&](const auto& opt) -> StatusOr<double> {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, LinearFamilyOptions>) {
          if (key == "objective") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.objective)));
          if (key == "regularization") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.regularization)));
          if (key == "regularization_strength") return StatusOr<double>(opt.regularization_strength);
          if (key == "elastic_l1_ratio") return StatusOr<double>(opt.elastic_l1_ratio);
          if (key == "robust_transition") return StatusOr<double>(opt.robust_transition);
          if (key == "fit_intercept") return StatusOr<double>(opt.fit_intercept ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, TreeFamilyOptions>) {
          if (key == "criterion") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.criterion)));
          if (key == "max_depth") return StatusOr<double>(static_cast<double>(opt.max_depth));
          if (key == "min_samples_leaf") return StatusOr<double>(static_cast<double>(opt.min_samples_leaf));
          if (key == "min_samples_split") return StatusOr<double>(static_cast<double>(opt.min_samples_split));
        } else if constexpr (std::is_same_v<Opt, EnsembleFamilyOptions>) {
          if (key == "method") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.method)));
          if (key == "learners") return StatusOr<double>(static_cast<double>(opt.learners));
          if (key == "subsample") return StatusOr<double>(opt.subsample);
          if (key == "learning_rate") return StatusOr<double>(opt.learning_rate);
        } else if constexpr (std::is_same_v<Opt, NeighborFamilyOptions>) {
          if (key == "k") return StatusOr<double>(static_cast<double>(opt.k));
          if (key == "query_mode") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.query_mode)));
          if (key == "radius") return StatusOr<double>(opt.radius);
          if (key == "radius_fallback_k") return StatusOr<double>(static_cast<double>(opt.radius_fallback_k));
          if (key == "distance_weighted") return StatusOr<double>(opt.distance_weighted ? 1.0 : 0.0);
          if (key == "standardize") return StatusOr<double>(opt.standardize ? 1.0 : 0.0);
        } else if constexpr (std::is_same_v<Opt, KernelFamilyOptions>) {
          if (key == "kernel") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.kernel)));
          if (key == "form") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.form)));
          if (key == "c") return StatusOr<double>(opt.c);
          if (key == "gamma") return StatusOr<double>(opt.gamma);
          if (key == "epsilon") return StatusOr<double>(opt.epsilon);
          if (key == "nu") return StatusOr<double>(opt.nu);
        } else if constexpr (std::is_same_v<Opt, BayesFamilyOptions>) {
          if (key == "likelihood") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.likelihood)));
          if (key == "use_gaussian") return StatusOr<double>(opt.use_gaussian ? 1.0 : 0.0);
          if (key == "smoothing") return StatusOr<double>(opt.smoothing);
        } else if constexpr (std::is_same_v<Opt, ClusterFamilyOptions>) {
          if (key == "method") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.method)));
          if (key == "clusters") return StatusOr<double>(static_cast<double>(opt.clusters));
          if (key == "max_iterations") return StatusOr<double>(static_cast<double>(opt.max_iterations));
          if (key == "restarts") return StatusOr<double>(static_cast<double>(opt.restarts));
          if (key == "tolerance") return StatusOr<double>(opt.tolerance);
        } else if constexpr (std::is_same_v<Opt, DecomposeFamilyOptions>) {
          if (key == "method") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.method)));
          if (key == "components") return StatusOr<double>(static_cast<double>(opt.components));
          if (key == "whiten") return StatusOr<double>(opt.whiten ? 1.0 : 0.0);
          if (key == "max_iterations") return StatusOr<double>(static_cast<double>(opt.max_iterations));
          if (key == "tolerance") return StatusOr<double>(opt.tolerance);
        } else if constexpr (std::is_same_v<Opt, DetectFamilyOptions>) {
          if (key == "method") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(opt.method)));
          if (key == "estimators") return StatusOr<double>(static_cast<double>(opt.isolation.estimators));
          if (key == "max_samples") return StatusOr<double>(static_cast<double>(opt.isolation.max_samples));
          if (key == "max_samples_auto") return StatusOr<double>(opt.isolation.max_samples_auto ? 1.0 : 0.0);
          if (key == "max_features_ratio") return StatusOr<double>(opt.isolation.max_features_ratio);
          if (key == "bootstrap") return StatusOr<double>(opt.isolation.bootstrap ? 1.0 : 0.0);
          if (key == "contamination_auto") return StatusOr<double>(opt.isolation.contamination_auto ? 1.0 : 0.0);
          if (key == "contamination") return StatusOr<double>(opt.isolation.contamination);
        }
        return StatusOr<double>(Status::invalid_argument("unsupported model option parameter."));
      },
      spec.options);
}

Status set_model_param(ModelSpec& spec, std::string_view param, double value) {
  if (param == "task") {
    spec.task = static_cast<TaskKind>(static_cast<std::uint8_t>(value));
    return Status::success();
  }
  if (param == "family") {
    spec.family = static_cast<ModelFamily>(static_cast<std::uint8_t>(value));
    switch (spec.family) {
      case ModelFamily::Linear: spec.options = LinearFamilyOptions{}; break;
      case ModelFamily::Tree: spec.options = TreeFamilyOptions{}; break;
      case ModelFamily::Ensemble: spec.options = EnsembleFamilyOptions{}; break;
      case ModelFamily::Neighbor: spec.options = NeighborFamilyOptions{}; break;
      case ModelFamily::Kernel: spec.options = KernelFamilyOptions{}; break;
      case ModelFamily::Bayes: spec.options = BayesFamilyOptions{}; break;
      case ModelFamily::Cluster: spec.options = ClusterFamilyOptions{}; break;
      case ModelFamily::Decompose: spec.options = DecomposeFamilyOptions{}; break;
      case ModelFamily::Detect: spec.options = DetectFamilyOptions{}; break;
    }
    return Status::success();
  }
  if (!param.starts_with("opt.")) return Status::invalid_argument("unsupported model param key.");
  const std::string key(param.substr(4));
  return std::visit(
      [&](auto& opt) -> Status {
        using Opt = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<Opt, LinearFamilyOptions>) {
          if (key == "objective") { opt.objective = static_cast<LinearObjective>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "regularization") { opt.regularization = static_cast<RegularizationKind>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "regularization_strength") { opt.regularization_strength = value; return Status::success(); }
          if (key == "elastic_l1_ratio") { opt.elastic_l1_ratio = value; return Status::success(); }
          if (key == "robust_transition") { opt.robust_transition = value; return Status::success(); }
          if (key == "fit_intercept") { opt.fit_intercept = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, TreeFamilyOptions>) {
          if (key == "criterion") { opt.criterion = static_cast<TreeCriterion>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "max_depth") { opt.max_depth = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "min_samples_leaf") { opt.min_samples_leaf = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "min_samples_split") { opt.min_samples_split = static_cast<std::int64_t>(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, EnsembleFamilyOptions>) {
          if (key == "method") { opt.method = static_cast<EnsembleMethod>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "learners") { opt.learners = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "subsample") { opt.subsample = value; return Status::success(); }
          if (key == "learning_rate") { opt.learning_rate = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, NeighborFamilyOptions>) {
          if (key == "k") { opt.k = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "query_mode") { opt.query_mode = static_cast<NeighborQueryMode>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "radius") { opt.radius = value; return Status::success(); }
          if (key == "radius_fallback_k") { opt.radius_fallback_k = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "distance_weighted") { opt.distance_weighted = to_flag(value); return Status::success(); }
          if (key == "standardize") { opt.standardize = to_flag(value); return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, KernelFamilyOptions>) {
          if (key == "kernel") { opt.kernel = static_cast<KernelKind>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "form") { opt.form = static_cast<KernelForm>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "c") { opt.c = value; return Status::success(); }
          if (key == "gamma") { opt.gamma = value; return Status::success(); }
          if (key == "epsilon") { opt.epsilon = value; return Status::success(); }
          if (key == "nu") { opt.nu = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, BayesFamilyOptions>) {
          if (key == "likelihood") { opt.likelihood = static_cast<BayesLikelihood>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "use_gaussian") { opt.use_gaussian = to_flag(value); return Status::success(); }
          if (key == "smoothing") { opt.smoothing = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, ClusterFamilyOptions>) {
          if (key == "method") { opt.method = static_cast<ClusterMethod>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "clusters") { opt.clusters = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "max_iterations") { opt.max_iterations = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "restarts") { opt.restarts = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "tolerance") { opt.tolerance = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, DecomposeFamilyOptions>) {
          if (key == "method") { opt.method = static_cast<DecomposeMethod>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "components") { opt.components = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "whiten") { opt.whiten = to_flag(value); return Status::success(); }
          if (key == "max_iterations") { opt.max_iterations = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "tolerance") { opt.tolerance = value; return Status::success(); }
        } else if constexpr (std::is_same_v<Opt, DetectFamilyOptions>) {
          if (key == "method") { opt.method = static_cast<DetectMethod>(static_cast<std::uint8_t>(value)); return Status::success(); }
          if (key == "estimators") { opt.isolation.estimators = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "max_samples") { opt.isolation.max_samples = static_cast<std::int64_t>(value); return Status::success(); }
          if (key == "max_samples_auto") { opt.isolation.max_samples_auto = to_flag(value); return Status::success(); }
          if (key == "max_features_ratio") { opt.isolation.max_features_ratio = value; return Status::success(); }
          if (key == "bootstrap") { opt.isolation.bootstrap = to_flag(value); return Status::success(); }
          if (key == "contamination_auto") { opt.isolation.contamination_auto = to_flag(value); return Status::success(); }
          if (key == "contamination") { opt.isolation.contamination = value; return Status::success(); }
        }
        return Status::invalid_argument("unsupported model option parameter.");
      },
      spec.options);
}

[[nodiscard]] StatusOr<std::vector<std::int64_t>> normalized_feature_route(const std::vector<std::int64_t>& route,
                                                                           std::int64_t feature_count,
                                                                           std::int64_t stage_index,
                                                                           const char* where) {
  if (route.empty()) return StatusOr<std::vector<std::int64_t>>(std::vector<std::int64_t>{});
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
  if (route.empty()) return StatusOr<FeatureBatch>(batch);

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

[[nodiscard]] StatusOr<FeatureBatch> merge_routed_features(const FeatureBatch& original,
                                                           const FeatureBatch& routed_mapped,
                                                           const std::vector<std::int64_t>& route,
                                                           RouteRemainderPolicy remainder,
                                                           std::int64_t stage_index,
                                                           const char* where) {
  Status check = validate_features(original);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  check = validate_features(routed_mapped);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  if (original.layout != routed_mapped.layout) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " route merge layout mismatch."));
  }
  if (original.sample_count() != routed_mapped.sample_count()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " route merge sample count mismatch."));
  }
  if (route.empty()) return StatusOr<FeatureBatch>(routed_mapped);
  if (remainder == RouteRemainderPolicy::Drop) {
    return StatusOr<FeatureBatch>(routed_mapped);
  }

  const std::int64_t feature_count = original.feature_count();
  if (static_cast<std::int64_t>(route.size()) == feature_count) {
    return StatusOr<FeatureBatch>(routed_mapped);
  }

  std::vector<bool> selected(static_cast<std::size_t>(feature_count), false);
  for (std::int64_t idx : route) selected[static_cast<std::size_t>(idx)] = true;

  std::vector<Series> parts{};
  parts.reserve(1 + route.size());
  auto push_slice = [&](const std::vector<std::int64_t>& ids) {
    if (ids.empty()) return;
    const Series idx = make_index_series(ids, original.x.platform());
    const Series slab =
        original.layout == ObservationLayout::Rows ? original.x.index_select(1, idx) : original.x.index_select(0, idx);
    parts.push_back(slab);
  };

  bool inserted = false;
  std::vector<std::int64_t> carry{};
  carry.reserve(static_cast<std::size_t>(feature_count));
  for (std::int64_t f = 0; f < feature_count; ++f) {
    if (selected[static_cast<std::size_t>(f)]) {
      if (!inserted) {
        push_slice(carry);
        carry.clear();
        parts.push_back(routed_mapped.x);
        inserted = true;
      }
      continue;
    }
    carry.push_back(f);
  }
  push_slice(carry);

  if (parts.empty()) {
    return StatusOr<FeatureBatch>(Status::internal(std::string(where) + " stage " + std::to_string(stage_index)
                                                   + " route merge produced no output parts."));
  }

  Series merged = parts.front();
  const std::int64_t axis = original.layout == ObservationLayout::Rows ? 1 : 0;
  for (std::size_t i = 1; i < parts.size(); ++i) {
    merged = merged.unite({parts[i]}, axis);
  }

  FeatureBatch out{
      .x = merged,
      .layout = original.layout,
  };
  check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                                                         + " route merge emitted invalid feature shape."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

} // namespace

bool Flow::trained() const noexcept { return trained_; }

bool Flow::has_model() const noexcept {
  return static_cast<bool>(classifier_) || static_cast<bool>(regressor_) || has_transform_model_;
}

TaskKind Flow::task_kind() const noexcept {
  if (classifier_) return TaskKind::Classification;
  if (regressor_) return TaskKind::Regression;
  return TaskKind::Transform;
}

std::int64_t Flow::stage_count() const noexcept { return static_cast<std::int64_t>(transforms_.size()); }

bool Flow::has_model_spec() const noexcept { return model_spec_.has_value(); }

bool Flow::has_opaque_transforms() const noexcept { return has_opaque_transforms_; }

const std::optional<ModelSpec>& Flow::model_spec() const noexcept { return model_spec_; }

const std::vector<TransformSpec>& Flow::transform_specs() const noexcept { return transform_specs_; }

StatusOr<FlowGraphInfo> Flow::graph_info() const {
  FlowGraphInfo info{};
  info.task = task_kind();
  info.trained = trained_;
  info.has_model = has_model();
  info.has_opaque_transforms = has_opaque_transforms_;
  info.stages.reserve(transform_specs_.size());
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    const TransformSpec& spec = transform_specs_[i];
    info.stages.push_back(FlowStageInfo{
        .index = static_cast<std::int64_t>(i),
        .stage_key = spec.stage_key,
        .kind = spec.kind,
        .routed_feature_count = static_cast<std::int64_t>(spec.feature_route.size()),
        .remainder = spec.remainder,
        .route_active = !spec.feature_route.empty(),
    });
  }
  return StatusOr<FlowGraphInfo>(std::move(info));
}

std::vector<std::string> Flow::nested_param_keys() const {
  std::vector<std::string> out{};
  out.reserve(transform_specs_.size() * 16 + 24);
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    const TransformSpec& spec = transform_specs_[i];
    const std::string base = !spec.stage_key.empty() ? spec.stage_key : ("stage" + std::to_string(i));
    out.push_back(base + "::route_size");
    out.push_back(base + "::remainder");
    out.push_back(base + "::kind");
    out.push_back(base + "::stage_index");
    append_transform_option_param_keys(spec.options, base + "::opt.", out);
  }
  if (model_spec_.has_value()) {
    append_model_option_param_keys(model_spec_.value(), out);
  }
  return out;
}

StatusOr<double> Flow::nested_param(std::string_view path) const {
  const std::vector<std::string> parts = split_sv(path, ':');
  if (parts.size() != 3 || parts[1] != "") {
    return StatusOr<double>(Status::invalid_argument("nested_param path must be '<stage>::<param>'."));
  }
  const std::string stage = parts[0];
  const std::string param = parts[2];
  if (stage == "model") {
    if (!model_spec_.has_value()) {
      return StatusOr<double>(Status::precondition("nested_param model::* requires Flow configured via set_model(ModelSpec)."));
    }
    return model_param(model_spec_.value(), param);
  }

  StatusOr<std::int64_t> stage_index_or = resolve_stage_index(transform_specs_, stage);
  if (!stage_index_or.ok()) {
    return StatusOr<double>(Status::invalid_argument("nested_param could not resolve stage key."));
  }
  const std::int64_t stage_index = stage_index_or.value();
  const TransformSpec& spec = transform_specs_[static_cast<std::size_t>(stage_index)];
  if (param == "route_size") return StatusOr<double>(static_cast<double>(spec.feature_route.size()));
  if (param == "remainder") return StatusOr<double>(spec.remainder == RouteRemainderPolicy::Drop ? 1.0 : 0.0);
  if (param == "kind") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(spec.kind)));
  if (param == "stage_index") return StatusOr<double>(static_cast<double>(stage_index));
  if (param.size() > 4 && param.rfind("opt.", 0) == 0) {
    return transform_option_param(spec.options, std::string_view(param).substr(4));
  }
  return StatusOr<double>(Status::invalid_argument("nested_param supports route metadata, model parameters, and stage opt.* parameters."));
}

Status Flow::set_nested_param(std::string_view path, double value) {
  const std::vector<std::string> parts = split_sv(path, ':');
  if (parts.size() != 3 || parts[1] != "") {
    return Status::invalid_argument("set_nested_param path must be '<stage>::<param>'.");
  }
  const std::string stage = parts[0];
  const std::string param = parts[2];
  if (stage == "model") {
    if (!model_spec_.has_value()) {
      return Status::precondition("set_nested_param model::* requires Flow configured via set_model(ModelSpec).");
    }
    ModelSpec spec = model_spec_.value();
    Status status = set_model_param(spec, param, value);
    if (!status.ok()) return status;
    return set_model(spec);
  }

  StatusOr<std::int64_t> stage_index_or = resolve_stage_index(transform_specs_, stage);
  if (!stage_index_or.ok()) {
    return Status::invalid_argument("set_nested_param could not resolve stage key.");
  }
  const std::int64_t stage_index = stage_index_or.value();
  if (param == "remainder") {
    return set_stage_remainder(stage_index, value >= 0.5 ? RouteRemainderPolicy::Drop : RouteRemainderPolicy::PassThrough);
  }
  if (param == "kind") {
    const TransformKind next_kind = static_cast<TransformKind>(static_cast<std::uint8_t>(value));
    TransformOptions next_options_or{};
    StatusOr<TransformOptions> defaults_or = parse_transform_options(next_kind, "");
    if (!defaults_or.ok()) return defaults_or.status();
    next_options_or = defaults_or.move_value();
    transform_specs_[static_cast<std::size_t>(stage_index)].kind = next_kind;
    transform_specs_[static_cast<std::size_t>(stage_index)].options = std::move(next_options_or);
    StatusOr<std::unique_ptr<TransformerBase>> stage_or = make_transformer_from_spec(
        transform_specs_[static_cast<std::size_t>(stage_index)].kind,
        transform_specs_[static_cast<std::size_t>(stage_index)].options);
    if (!stage_or.ok()) return stage_or.status();
    transforms_[static_cast<std::size_t>(stage_index)] = stage_or.move_value();
    trained_ = false;
    return Status::success();
  }
  if (param.size() > 4 && param.rfind("opt.", 0) == 0) {
    TransformSpec& spec = transform_specs_[static_cast<std::size_t>(stage_index)];
    Status status = set_transform_option_param(spec.options, std::string_view(param).substr(4), value);
    if (!status.ok()) return status;
    StatusOr<std::unique_ptr<TransformerBase>> stage_or = make_transformer_from_spec(spec.kind, spec.options);
    if (!stage_or.ok()) return stage_or.status();
    transforms_[static_cast<std::size_t>(stage_index)] = stage_or.move_value();
    trained_ = false;
    return Status::success();
  }
  return Status::invalid_argument("set_nested_param supports model::* and '<stage>::remainder|kind|opt.*'.");
}

std::string Flow::flow_summary() const {
  std::ostringstream oss;
  oss << "task=" << static_cast<int>(task_kind())
      << " stages=" << transform_specs_.size()
      << " trained=" << (trained_ ? "1" : "0")
      << " model=" << (has_model() ? "1" : "0");
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    const auto& spec = transform_specs_[i];
    oss << "\n[" << i << "] "
        << (spec.stage_key.empty() ? ("stage" + std::to_string(i)) : spec.stage_key)
        << " kind=" << transform_kind_name(spec.kind)
        << " route=" << spec.feature_route.size()
        << " remainder=" << remainder_name(spec.remainder);
  }
  return oss.str();
}

StatusOr<std::string> Flow::save_blueprint() const {
  if (has_opaque_transforms_) {
    return StatusOr<std::string>(
        Status::invalid_argument("Flow::save_blueprint does not support opaque append_transform stages."));
  }
  std::ostringstream oss;
  oss << "pluss_flow_blueprint_v2\n";
  for (const auto& spec : transform_specs_) {
    oss << transform_kind_name(spec.kind) << "|"
        << (spec.stage_key.empty() ? "-" : spec.stage_key) << "|"
        << remainder_name(spec.remainder) << "|";
    for (std::size_t i = 0; i < spec.feature_route.size(); ++i) {
      if (i != 0) oss << ",";
      oss << spec.feature_route[i];
    }
    oss << "|" << serialize_transform_options(spec.options) << "\n";
  }
  return StatusOr<std::string>(oss.str());
}

Status Flow::load_blueprint(std::string_view text) {
  std::istringstream iss{std::string(text)};
  std::string line{};
  if (!std::getline(iss, line)) {
    return Status::invalid_argument("Flow::load_blueprint requires a non-empty blueprint.");
  }
  const bool v1 = line == "pluss_flow_blueprint_v1";
  const bool v2 = line == "pluss_flow_blueprint_v2";
  if (!v1 && !v2) {
    return Status::invalid_argument("Flow::load_blueprint invalid blueprint header.");
  }
  clear_transforms();
  while (std::getline(iss, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> fields = split_sv(line, '|');
    if ((!v1 && fields.size() != 5) || (v1 && fields.size() != 4)) {
      return Status::invalid_argument("Flow::load_blueprint malformed stage line.");
    }
    const std::optional<TransformKind> kind = parse_transform_kind(fields[0]);
    if (!kind.has_value()) return Status::invalid_argument("Flow::load_blueprint unknown stage kind.");
    const std::optional<RouteRemainderPolicy> remainder = parse_remainder(fields[2]);
    if (!remainder.has_value()) return Status::invalid_argument("Flow::load_blueprint invalid remainder policy.");
    std::vector<std::int64_t> route{};
    if (!fields[3].empty()) {
      const std::vector<std::string> ids = split_sv(fields[3], ',');
      route.reserve(ids.size());
      for (const std::string& token : ids) {
        const auto parsed = parse_scalar<std::int64_t>(token);
        if (!parsed.has_value()) return Status::invalid_argument("Flow::load_blueprint route parse error.");
        route.push_back(parsed.value());
      }
    }
    const std::string option_blob = v2 ? fields[4] : std::string{};
    StatusOr<TransformOptions> options_or = parse_transform_options(kind.value(), option_blob);
    if (!options_or.ok()) return options_or.status();
    Status status = add_named_transform(kind.value(),
                                        options_or.value(),
                                        fields[1] == "-" ? std::string{} : fields[1],
                                        pluss::msharp::RangeView::Int(route),
                                        remainder.value());
    if (!status.ok()) return status;
  }
  return Status::success();
}

StatusOr<Flow> Flow::clone() const {
  if (has_opaque_transforms_) {
    return StatusOr<Flow>(
        Status::invalid_argument("Flow::clone does not support opaque append_transform stages."));
  }
  if (!model_spec_.has_value() && has_model()) {
    return StatusOr<Flow>(
        Status::invalid_argument("Flow::clone requires a ModelSpec for model-backed flows."));
  }

  // Rebuild transform stages before the model: a TaskKind::Transform
  // ModelSpec's model occupies the tail of transforms_ with no
  // transform_specs_ entry of its own (see set_model()), and
  // add_named_transform() refuses to add anything after it. Stages first,
  // model last is the only order that works for every task.
  Flow out{};
  for (const TransformSpec& spec : transform_specs_) {
    Status status = out.add_named_transform(spec.kind,
                                            spec.options,
                                            spec.stage_key,
                                            pluss::msharp::RangeView::Int(spec.feature_route),
                                            spec.remainder);
    if (!status.ok()) {
      return StatusOr<Flow>(
          Status::invalid_argument("Flow::clone failed to rebuild transform stage: " + status.message));
    }
  }

  if (model_spec_.has_value()) {
    Status status = out.set_model(*model_spec_);
    if (!status.ok()) {
      return StatusOr<Flow>(Status::invalid_argument("Flow::clone failed to rebuild model: " + status.message));
    }
  }

  if (trained_) {
    StatusOr<std::vector<std::vector<Series>>> transform_states_or = dump_transform_states();
    if (!transform_states_or.ok()) return StatusOr<Flow>(transform_states_or.status());
    Status status = out.load_transform_states(transform_states_or.value());
    if (!status.ok()) {
      return StatusOr<Flow>(
          Status::invalid_argument("Flow::clone failed to load transform states: " + status.message));
    }

    if (out.has_model()) {
      StatusOr<std::vector<Series>> model_state_or = dump_model_state();
      if (!model_state_or.ok()) return StatusOr<Flow>(model_state_or.status());
      status = out.load_model_state(model_state_or.value());
      if (!status.ok()) {
        return StatusOr<Flow>(
            Status::invalid_argument("Flow::clone failed to load model state: " + status.message));
      }
    }
    out.mark_trained(true);
  }

  return StatusOr<Flow>(std::move(out));
}

Status Flow::add_named_transform(TransformKind kind,
                                 const TransformOptions& options,
                                 std::string stage_key,
                                 pluss::msharp::RangeView::Int feature_route,
                                 RouteRemainderPolicy remainder) {
  // set_model() with a TaskKind::Transform spec pushes the built transformer
  // straight onto transforms_ without a matching transform_specs_ entry (it
  // is the model, tracked via model_spec_, not a named stage). Pushing a
  // named stage on top of that would land this call's own spec at the wrong
  // index relative to transforms_, silently misrouting it to the model
  // instead of this stage on the next map()/learn_map(). Refuse instead of
  // corrupting the alignment.
  if (transforms_.size() != transform_specs_.size()) {
    return Status::precondition(
        "Flow::add_named_transform cannot add a stage after a TaskKind::Transform model has been set via "
        "set_model(); build every transform stage before attaching a Transform-task model, or attach the "
        "model to a separate Flow.");
  }
  if (!stage_key.empty()) {
    for (const TransformSpec& spec : transform_specs_) {
      if (spec.stage_key == stage_key) {
        return Status::invalid_argument("Flow::add_named_transform requires unique stage_key values.");
      }
    }
  }
  StatusOr<std::unique_ptr<TransformerBase>> stage_or = make_transformer_from_spec(kind, options);
  if (!stage_or.ok()) return stage_or.status();
  transforms_.push_back(stage_or.move_value());
  transform_specs_.push_back(TransformSpec{
      .kind = kind,
      .options = options,
      .feature_route = route_vec(feature_route),
      .remainder = remainder,
      .stage_key = std::move(stage_key),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_column_stage(std::string stage_key,
                              TransformKind kind,
                              const TransformOptions& options,
                              pluss::msharp::RangeView::Int feature_route,
                              RouteRemainderPolicy remainder) {
  return add_named_transform(kind, options, std::move(stage_key), feature_route, remainder);
}

Status Flow::set_stage_key(std::int64_t stage_index, std::string stage_key) {
  if (stage_index < 0 || stage_index >= static_cast<std::int64_t>(transform_specs_.size())) {
    return Status::invalid_argument("Flow::set_stage_key stage index out of range.");
  }
  if (!stage_key.empty()) {
    for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
      if (static_cast<std::int64_t>(i) == stage_index) continue;
      if (transform_specs_[i].stage_key == stage_key) {
        return Status::invalid_argument("Flow::set_stage_key requires unique stage_key values.");
      }
    }
  }
  transform_specs_[static_cast<std::size_t>(stage_index)].stage_key = std::move(stage_key);
  return Status::success();
}

Status Flow::set_stage_route(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route) {
  if (stage_index < 0 || stage_index >= static_cast<std::int64_t>(transform_specs_.size())) {
    return Status::invalid_argument("Flow::set_stage_route stage index out of range.");
  }
  transform_specs_[static_cast<std::size_t>(stage_index)].feature_route = route_vec(feature_route);
  trained_ = false;
  return Status::success();
}

Status Flow::set_stage_remainder(std::int64_t stage_index, RouteRemainderPolicy remainder) {
  if (stage_index < 0 || stage_index >= static_cast<std::int64_t>(transform_specs_.size())) {
    return Status::invalid_argument("Flow::set_stage_remainder stage index out of range.");
  }
  transform_specs_[static_cast<std::size_t>(stage_index)].remainder = remainder;
  trained_ = false;
  return Status::success();
}

Status Flow::append_transform(std::unique_ptr<TransformerBase> transformer) {
  if (!transformer) return Status::invalid_argument("Flow::append_transform requires a non-null transformer.");
  // See the identical guard in add_named_transform(): set_model() with a
  // TaskKind::Transform spec leaves transforms_ one entry ahead of
  // transform_specs_ (the model has no spec entry), so appending here would
  // misalign every later index too.
  if (transforms_.size() != transform_specs_.size()) {
    return Status::precondition(
        "Flow::append_transform cannot add a stage after a TaskKind::Transform model has been set via "
        "set_model(); build every transform stage before attaching a Transform-task model, or attach the "
        "model to a separate Flow.");
  }
  transforms_.push_back(std::move(transformer));
  // Keep transform_specs_ index-aligned with transforms_. An opaque stage
  // (FeatureTransformer / FunctionTransformer) runs on the whole frame and does
  // its own internal routing, so its placeholder spec carries an empty
  // feature_route -- apply_learn_map / apply_map then feed it the full batch.
  // Without this entry, a later add_named_transform() spec would land at this
  // opaque stage's index and its route would be misapplied to the opaque stage.
  // The placeholder `kind` is never used: execution dispatches through the
  // polymorphic transforms_[i] object, and serialization refuses flows that
  // contain opaque stages (see has_opaque_transforms()).
  transform_specs_.push_back(TransformSpec{});
  has_opaque_transforms_ = true;
  trained_ = false;
  return Status::success();
}

void Flow::clear_transforms() noexcept {
  transforms_.clear();
  transform_specs_.clear();
  has_opaque_transforms_ = false;
  trained_ = false;
  // Wiping transforms_ also removes a TaskKind::Transform model, if one was
  // set via set_model() -- it lived there, not in classifier_/regressor_.
  if (has_transform_model_) {
    has_transform_model_ = false;
    model_spec_.reset();
  }
}

Status Flow::add_standard_scaler(prep::StandardScalerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::StandardScaler, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_minmax_scaler(prep::MinMaxScalerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::MinMaxScaler, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_robust_scaler(prep::RobustScalerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::RobustScaler, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_simple_imputer(prep::SimpleImputerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::SimpleImputer, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_neighbor_imputer(prep::NeighborImputerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::NeighborImputer, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_iterative_imputer(prep::IterativeImputerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::IterativeImputer, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_one_hot_encoder(prep::OneHotEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::OneHotEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_frequency_encoder(prep::FrequencyEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::FrequencyEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_vector_normalizer(prep::VectorNormalizerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::VectorNormalizer, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_maxabs_scaler(prep::MaxAbsScalerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::MaxAbsScaler, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_quantile_map(prep::QuantileMapOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::QuantileMap, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_power_map(prep::PowerMapOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::PowerMap, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_threshold_binarizer(prep::ThresholdBinarizerOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(TransformKind::ThresholdBinarizer,
                             TransformOptions{options},
                             std::string{},
                             feature_route,
                             RouteRemainderPolicy::PassThrough);
}

Status Flow::add_polynomial_lift(prep::PolynomialLiftOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::PolynomialLift, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_ordinal_encoder(prep::OrdinalEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::OrdinalEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_label_encoder(prep::LabelEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::LabelEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_basen_encoder(prep::BaseNEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::BaseNEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_hashing_encoder(prep::HashingEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::HashingEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_target_encoder(prep::TargetEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::TargetEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_woe_encoder(prep::WOEEncoderOptions options, pluss::msharp::RangeView::Int feature_route) {
  return add_named_transform(
      TransformKind::WOEEncoder, TransformOptions{options}, std::string{}, feature_route, RouteRemainderPolicy::PassThrough);
}

Status Flow::add_function_transformer(prep::FunctionTransformer transformer) {
  return append_transform(std::make_unique<prep::FunctionTransformer>(std::move(transformer)));
}

Status Flow::add_feature_transformer(prep::FeatureTransformer transformer) {
  return append_transform(std::make_unique<prep::FeatureTransformer>(std::move(transformer)));
}

Status Flow::set_model(const ModelSpec& spec) {
  clear_model();
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> cls_or = make_classifier(spec);
    if (!cls_or.ok()) return cls_or.status();
    classifier_ = cls_or.move_value();
    model_spec_ = spec;
    trained_ = false;
    return Status::success();
  }
  if (spec.task == TaskKind::Regression) {
    StatusOr<std::unique_ptr<RegressorBase>> reg_or = make_regressor(spec);
    if (!reg_or.ok()) return reg_or.status();
    regressor_ = reg_or.move_value();
    model_spec_ = spec;
    trained_ = false;
    return Status::success();
  }
  if (spec.task == TaskKind::Transform) {
    StatusOr<std::unique_ptr<TransformerBase>> transformer_or = make_transformer(spec);
    if (!transformer_or.ok()) return transformer_or.status();
    // clear_model() above already popped any prior Transform-task model, so
    // transforms_ carries no orphan here -- push_back is always a clean
    // replacement, never an accumulation.
    transforms_.push_back(transformer_or.move_value());
    has_transform_model_ = true;
    model_spec_ = spec;
    trained_ = false;
    return Status::success();
  }
  return Status::invalid_argument("Flow::set_model supports classification, regression, or transform ModelSpec tasks.");
}

void Flow::clear_transform_model() noexcept {
  if (has_transform_model_) {
    transforms_.pop_back();
    has_transform_model_ = false;
  }
}

Status Flow::set_classifier(std::unique_ptr<ClassifierBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_classifier requires a non-null model.");
  regressor_.reset();
  clear_transform_model();
  classifier_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

Status Flow::set_regressor(std::unique_ptr<RegressorBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_regressor requires a non-null model.");
  classifier_.reset();
  clear_transform_model();
  regressor_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

void Flow::clear_model() noexcept {
  classifier_.reset();
  regressor_.reset();
  clear_transform_model();
  model_spec_.reset();
  trained_ = false;
}

StatusOr<FeatureBatch> Flow::apply_map(const FeatureBatch& batch, const InferOptions& options) const {
  Status input_status = validate_features(batch);
  if (!input_status.ok()) return StatusOr<FeatureBatch>(std::move(input_status));

  if (transforms_.empty()) return StatusOr<FeatureBatch>(batch);
  const std::int64_t expected_samples = batch.sample_count();
  FeatureBatch current = batch;
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<FeatureBatch>(Status::internal("Flow contains a null transformer stage."));
    }
    const std::vector<std::int64_t>* stage_route = (i < transform_specs_.size()) ? &transform_specs_[i].feature_route : nullptr;
    if (stage_route == nullptr || stage_route->empty()) {
      StatusOr<Series> mapped_or = transforms_[i]->map(current, options);
      if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
      StatusOr<FeatureBatch> next_or =
          mapped_batch_checked(mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::map");
      if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
      current = next_or.move_value();
      continue;
    }

    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        *stage_route, current.feature_count(), static_cast<std::int64_t>(i), "Flow::map");
    if (!route_or.ok()) return StatusOr<FeatureBatch>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();

    StatusOr<FeatureBatch> routed_or = select_routed_features(current, route, static_cast<std::int64_t>(i), "Flow::map");
    if (!routed_or.ok()) return StatusOr<FeatureBatch>(routed_or.status());

    StatusOr<Series> mapped_or = transforms_[i]->map(routed_or.value(), options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> mapped_routed_or = mapped_batch_checked(
        mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::map");
    if (!mapped_routed_or.ok()) return StatusOr<FeatureBatch>(mapped_routed_or.status());

    const RouteRemainderPolicy remainder =
        (i < transform_specs_.size()) ? transform_specs_[i].remainder : RouteRemainderPolicy::PassThrough;
    StatusOr<FeatureBatch> merged_or = merge_routed_features(
        current, mapped_routed_or.value(), route, remainder, static_cast<std::int64_t>(i), "Flow::map");
    if (!merged_or.ok()) return StatusOr<FeatureBatch>(merged_or.status());
    current = merged_or.move_value();
  }
  return StatusOr<FeatureBatch>(std::move(current));
}

namespace {

// One preprocessing stage's fit+apply during Flow::learn. Supervised stages
// (target / weight-of-evidence encoders) are handed the training target; every
// other stage takes the ordinary unsupervised learn_map().
[[nodiscard]] StatusOr<Series> learn_map_stage_(TransformerBase& stage,
                                                const FeatureBatch& batch,
                                                const Series* target,
                                                const TrainOptions& learn_options,
                                                const InferOptions& infer_options) {
  if (stage.needs_target()) {
    if (target == nullptr || !target->defined()) {
      return StatusOr<Series>(Status::invalid_argument(
          "a supervised prep stage (target / woe encoder) needs a training target; "
          "attach it inside Flow::learn(labeled_batch), not Flow::map()/learn_map()."));
    }
    Status fit = stage.learn_supervised(batch, *target, learn_options);
    if (!fit.ok()) return StatusOr<Series>(std::move(fit));
    return stage.map(batch, infer_options);
  }
  return stage.learn_map(batch, learn_options, infer_options);
}

} // namespace

StatusOr<FeatureBatch> Flow::apply_learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options,
                                             const Series* target) {
  Status input_status = validate_features(batch);
  if (!input_status.ok()) return StatusOr<FeatureBatch>(std::move(input_status));

  if (transforms_.empty()) return StatusOr<FeatureBatch>(batch);
  const std::int64_t expected_samples = batch.sample_count();
  FeatureBatch current = batch;
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<FeatureBatch>(Status::internal("Flow contains a null transformer stage."));
    }
    const std::vector<std::int64_t>* stage_route = (i < transform_specs_.size()) ? &transform_specs_[i].feature_route : nullptr;
    if (stage_route == nullptr || stage_route->empty()) {
      StatusOr<Series> mapped_or = learn_map_stage_(*transforms_[i], current, target, learn_options, infer_options);
      if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
      StatusOr<FeatureBatch> next_or = mapped_batch_checked(
          mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::learn");
      if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
      current = next_or.move_value();
      continue;
    }

    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        *stage_route, current.feature_count(), static_cast<std::int64_t>(i), "Flow::learn");
    if (!route_or.ok()) return StatusOr<FeatureBatch>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();

    StatusOr<FeatureBatch> routed_or = select_routed_features(current, route, static_cast<std::int64_t>(i), "Flow::learn");
    if (!routed_or.ok()) return StatusOr<FeatureBatch>(routed_or.status());

    StatusOr<Series> mapped_or = learn_map_stage_(*transforms_[i], routed_or.value(), target, learn_options, infer_options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> mapped_routed_or = mapped_batch_checked(
        mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::learn");
    if (!mapped_routed_or.ok()) return StatusOr<FeatureBatch>(mapped_routed_or.status());

    const RouteRemainderPolicy remainder =
        (i < transform_specs_.size()) ? transform_specs_[i].remainder : RouteRemainderPolicy::PassThrough;
    StatusOr<FeatureBatch> merged_or = merge_routed_features(
        current, mapped_routed_or.value(), route, remainder, static_cast<std::int64_t>(i), "Flow::learn");
    if (!merged_or.ok()) return StatusOr<FeatureBatch>(merged_or.status());
    current = merged_or.move_value();
  }
  return StatusOr<FeatureBatch>(std::move(current));
}

Status Flow::learn(const LabeledBatch& batch, const TrainOptions& options) {
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return data_status;

  const InferOptions infer_options{
      .layout = batch.features.layout,
      .missing = options.missing,
      .unknown_categorical = options.unknown_categorical,
  };
  StatusOr<FeatureBatch> mapped_or = apply_learn_map(batch.features, options, infer_options, &batch.y);
  if (!mapped_or.ok()) return mapped_or.status();

  if (!has_model()) {
    trained_ = true;
    return Status::success();
  }

  const LabeledBatch learned_batch{
      .features = mapped_or.move_value(),
      .y = batch.y,
      .sample_weights = batch.sample_weights,
  };
  Status model_status = classifier_ ? classifier_->learn(learned_batch, options) : regressor_->learn(learned_batch, options);
  if (!model_status.ok()) return model_status;
  trained_ = true;
  return Status::success();
}

Status Flow::learn(const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options) {
  StatusOr<LabeledBatch> batch_or = labeled_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return batch_or.status();
  }
  return learn(batch_or.value(), options);
}

StatusOr<Series> Flow::map(const FeatureBatch& batch, const InferOptions& options) const {
  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return StatusOr<Series>(mapped_or.move_value().x);
}

StatusOr<Series> Flow::learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options,
                                 const InferOptions& infer_options) {
  StatusOr<FeatureBatch> mapped_or = apply_learn_map(batch, learn_options, infer_options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  if (!has_model()) {
    trained_ = true;
  } else {
    trained_ = false;
  }
  return StatusOr<Series>(mapped_or.move_value().x);
}

Status Flow::check_ready_for_infer(const char* where) const {
  if (!has_model()) {
    return Status::precondition(std::string("Flow requires a model before ") + where + "().");
  }
  if (!trained_) {
    return Status::precondition(std::string("Flow must be learned before ") + where + "().");
  }
  return Status::success();
}

StatusOr<Series> Flow::infer(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = check_ready_for_infer("infer");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return classifier_ ? classifier_->infer(mapped_or.value(), options) : regressor_->infer(mapped_or.value(), options);
}

StatusOr<Series> Flow::infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = feature_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return infer(batch_or.value(), options);
}

StatusOr<Series> Flow::infer_scores(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = check_ready_for_infer("infer_scores");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  if (!classifier_) {
    return StatusOr<Series>(Status::invalid_argument("Flow::infer_scores is available for classification flows only."));
  }

  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return classifier_->infer_scores(mapped_or.value(), options);
}

StatusOr<Series> Flow::infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = feature_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return infer_scores(batch_or.value(), options);
}

StatusOr<Series> Flow::assess(const LabeledBatch& batch, const AssessOptions& options) const {
  Status ready = check_ready_for_infer("assess");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

  const InferOptions infer_options{
      .layout = options.layout,
      .missing = MissingPolicy::KeepNaN,
      .unknown_categorical = UnknownCategoryPolicy::Ignore,
  };
  StatusOr<FeatureBatch> mapped_or = apply_map(batch.features, infer_options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());

  const LabeledBatch mapped_batch{
      .features = mapped_or.move_value(),
      .y = batch.y,
      .sample_weights = batch.sample_weights,
  };
  return classifier_ ? classifier_->assess(mapped_batch, options) : regressor_->assess(mapped_batch, options);
}

StatusOr<Series> Flow::assess(const pluss::table::DataFrame& frame,
                              const DataFrameBatchOptions& data_options,
                              const AssessOptions& options) const {
  StatusOr<LabeledBatch> batch_or = labeled_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return assess(batch_or.value(), options);
}

StatusOr<std::vector<std::vector<Series>>> Flow::dump_transform_states() const {
  std::vector<std::vector<Series>> states{};
  states.reserve(transforms_.size());
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<std::vector<std::vector<Series>>>(Status::internal("Flow contains a null transformer stage."));
    }
    StatusOr<std::vector<Series>> state_or = transforms_[i]->dump_state();
    if (!state_or.ok()) {
      return StatusOr<std::vector<std::vector<Series>>>(Status::not_implemented(
          "Flow::dump_transform_states failed at stage " + std::to_string(i) + ": " + state_or.status().message));
    }
    states.push_back(state_or.move_value());
  }
  return StatusOr<std::vector<std::vector<Series>>>(std::move(states));
}

StatusOr<std::vector<Series>> Flow::dump_model_state() const {
  if (!has_model()) return StatusOr<std::vector<Series>>(std::vector<Series>{});
  if (classifier_) return classifier_->dump_state();
  return regressor_->dump_state();
}

Status Flow::load_transform_states(pluss::msharp::RangeView::Of<std::vector<Series>> states) {
  if (states.size() != transforms_.size()) {
    return Status::shape_mismatch("Flow::load_transform_states requires one state per transform stage.");
  }
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) return Status::internal("Flow contains a null transformer stage.");
    Status status = transforms_[i]->load_state(states[i]);
    if (!status.ok()) {
      return Status::invalid_argument("Flow::load_transform_states failed at stage " + std::to_string(i) + ": "
                                      + status.message);
    }
  }
  return Status::success();
}

Status Flow::load_model_state(pluss::msharp::RangeView::Series state) {
  if (!has_model()) {
    if (!state.empty()) {
      return Status::invalid_argument("Flow::load_model_state received model state for a transform-only flow.");
    }
    return Status::success();
  }
  if (state.empty()) {
    return Status::invalid_argument("Flow::load_model_state requires non-empty model state for model flows.");
  }
  return classifier_ ? classifier_->load_state(state) : regressor_->load_state(state);
}

void Flow::mark_trained(bool trained) noexcept { trained_ = trained; }

StatusOr<Flow> make_flow(const ModelSpec& spec) {
  Flow flow{};
  Status status = flow.set_model(spec);
  if (!status.ok()) return StatusOr<Flow>(std::move(status));
  return StatusOr<Flow>(std::move(flow));
}

} // namespace pluss::ai::nodej
