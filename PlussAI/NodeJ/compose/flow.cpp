#include "pluss/ai/nodej/compose.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <iomanip>
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

[[nodiscard]] StatusOr<std::unique_ptr<TransformerBase>> make_transformer_from_spec(TransformKind kind,
                                                                                    const TransformOptions& options) {
  switch (kind) {
    case TransformKind::StandardScaler:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::StandardScaler>(
          std::get<prep::StandardScalerOptions>(options)));
    case TransformKind::MinMaxScaler:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::MinMaxScaler>(
          std::get<prep::MinMaxScalerOptions>(options)));
    case TransformKind::RobustScaler:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::RobustScaler>(
          std::get<prep::RobustScalerOptions>(options)));
    case TransformKind::SimpleImputer:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::SimpleImputer>(
          std::get<prep::SimpleImputerOptions>(options)));
    case TransformKind::NeighborImputer:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::NeighborImputer>(
          std::get<prep::NeighborImputerOptions>(options)));
    case TransformKind::IterativeImputer:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::IterativeImputer>(
          std::get<prep::IterativeImputerOptions>(options)));
    case TransformKind::OneHotEncoder:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::OneHotEncoder>(
          std::get<prep::OneHotEncoderOptions>(options)));
    case TransformKind::FrequencyEncoder:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::FrequencyEncoder>(
          std::get<prep::FrequencyEncoderOptions>(options)));
    case TransformKind::VectorNormalizer:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::VectorNormalizer>(
          std::get<prep::VectorNormalizerOptions>(options)));
    case TransformKind::MaxAbsScaler:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::MaxAbsScaler>(
          std::get<prep::MaxAbsScalerOptions>(options)));
    case TransformKind::QuantileMap:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::QuantileMap>(
          std::get<prep::QuantileMapOptions>(options)));
    case TransformKind::PowerMap:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::PowerMap>(
          std::get<prep::PowerMapOptions>(options)));
    case TransformKind::ThresholdBinarizer:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::ThresholdBinarizer>(
          std::get<prep::ThresholdBinarizerOptions>(options)));
    case TransformKind::PolynomialLift:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::PolynomialLift>(
          std::get<prep::PolynomialLiftOptions>(options)));
    case TransformKind::OrdinalEncoder:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::OrdinalEncoder>(
          std::get<prep::OrdinalEncoderOptions>(options)));
    case TransformKind::LabelEncoder:
      return StatusOr<std::unique_ptr<TransformerBase>>(std::make_unique<prep::LabelEncoder>(
          std::get<prep::LabelEncoderOptions>(options)));
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

bool Flow::has_model() const noexcept { return static_cast<bool>(classifier_) || static_cast<bool>(regressor_); }

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
  out.reserve(transform_specs_.size() * 4);
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    const TransformSpec& spec = transform_specs_[i];
    const std::string base = !spec.stage_key.empty() ? spec.stage_key : ("stage" + std::to_string(i));
    out.push_back(base + "::route_size");
    out.push_back(base + "::remainder");
    out.push_back(base + "::kind");
    out.push_back(base + "::stage_index");
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
  std::int64_t stage_index = -1;
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    if (transform_specs_[i].stage_key == stage || ("stage" + std::to_string(i)) == stage) {
      stage_index = static_cast<std::int64_t>(i);
      break;
    }
  }
  if (stage_index < 0 || stage_index >= static_cast<std::int64_t>(transform_specs_.size())) {
    return StatusOr<double>(Status::invalid_argument("nested_param could not resolve stage key."));
  }
  const TransformSpec& spec = transform_specs_[static_cast<std::size_t>(stage_index)];
  if (param == "route_size") return StatusOr<double>(static_cast<double>(spec.feature_route.size()));
  if (param == "remainder") return StatusOr<double>(spec.remainder == RouteRemainderPolicy::Drop ? 1.0 : 0.0);
  if (param == "kind") return StatusOr<double>(static_cast<double>(static_cast<std::uint8_t>(spec.kind)));
  if (param == "stage_index") return StatusOr<double>(static_cast<double>(stage_index));
  return StatusOr<double>(Status::invalid_argument("nested_param currently supports route_size|remainder|kind|stage_index."));
}

Status Flow::set_nested_param(std::string_view path, double value) {
  const std::vector<std::string> parts = split_sv(path, ':');
  if (parts.size() != 3 || parts[1] != "") {
    return Status::invalid_argument("set_nested_param path must be '<stage>::<param>'.");
  }
  const std::string stage = parts[0];
  const std::string param = parts[2];
  std::int64_t stage_index = -1;
  for (std::size_t i = 0; i < transform_specs_.size(); ++i) {
    if (transform_specs_[i].stage_key == stage || ("stage" + std::to_string(i)) == stage) {
      stage_index = static_cast<std::int64_t>(i);
      break;
    }
  }
  if (stage_index < 0 || stage_index >= static_cast<std::int64_t>(transform_specs_.size())) {
    return Status::invalid_argument("set_nested_param could not resolve stage key.");
  }
  if (param == "remainder") {
    return set_stage_remainder(stage_index, value >= 0.5 ? RouteRemainderPolicy::Drop : RouteRemainderPolicy::PassThrough);
  }
  return Status::invalid_argument("set_nested_param currently supports '<stage>::remainder' only.");
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
  oss << "pluss_flow_blueprint_v1\n";
  for (const auto& spec : transform_specs_) {
    oss << transform_kind_name(spec.kind) << "|"
        << (spec.stage_key.empty() ? "-" : spec.stage_key) << "|"
        << remainder_name(spec.remainder) << "|";
    for (std::size_t i = 0; i < spec.feature_route.size(); ++i) {
      if (i != 0) oss << ",";
      oss << spec.feature_route[i];
    }
    oss << "\n";
  }
  return StatusOr<std::string>(oss.str());
}

Status Flow::load_blueprint(std::string_view text) {
  std::istringstream iss{std::string(text)};
  std::string line{};
  if (!std::getline(iss, line)) {
    return Status::invalid_argument("Flow::load_blueprint requires a non-empty blueprint.");
  }
  if (line != "pluss_flow_blueprint_v1") {
    return Status::invalid_argument("Flow::load_blueprint invalid blueprint header.");
  }
  clear_transforms();
  while (std::getline(iss, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> fields = split_sv(line, '|');
    if (fields.size() != 4) return Status::invalid_argument("Flow::load_blueprint malformed stage line.");
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
    TransformOptions options{};
    switch (kind.value()) {
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
    }
    Status status = add_named_transform(kind.value(),
                                        options,
                                        fields[1] == "-" ? std::string{} : fields[1],
                                        pluss::msharp::RangeView::Int(route),
                                        remainder.value());
    if (!status.ok()) return status;
  }
  return Status::success();
}

Status Flow::add_named_transform(TransformKind kind,
                                 const TransformOptions& options,
                                 std::string stage_key,
                                 pluss::msharp::RangeView::Int feature_route,
                                 RouteRemainderPolicy remainder) {
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
  transforms_.push_back(std::move(transformer));
  has_opaque_transforms_ = true;
  trained_ = false;
  return Status::success();
}

void Flow::clear_transforms() noexcept {
  transforms_.clear();
  transform_specs_.clear();
  has_opaque_transforms_ = false;
  trained_ = false;
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
  return Status::invalid_argument("Flow::set_model supports classification or regression ModelSpec tasks only.");
}

Status Flow::set_classifier(std::unique_ptr<ClassifierBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_classifier requires a non-null model.");
  regressor_.reset();
  classifier_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

Status Flow::set_regressor(std::unique_ptr<RegressorBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_regressor requires a non-null model.");
  classifier_.reset();
  regressor_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

void Flow::clear_model() noexcept {
  classifier_.reset();
  regressor_.reset();
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

StatusOr<FeatureBatch> Flow::apply_learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
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
      StatusOr<Series> mapped_or = transforms_[i]->learn_map(current, learn_options, infer_options);
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

    StatusOr<Series> mapped_or = transforms_[i]->learn_map(routed_or.value(), learn_options, infer_options);
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
  StatusOr<FeatureBatch> mapped_or = apply_learn_map(batch.features, options, infer_options);
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
