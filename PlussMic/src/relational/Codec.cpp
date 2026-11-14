#include "relational/Codec.h"

#include <algorithm>
#include <cctype>

#include "CodeM/core/Errors.h"

namespace pluss::relational {

namespace {

std::string lower_trimmed(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
  std::string out(text.substr(begin, end - begin));
  for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
  return std::string(text.substr(begin, end - begin));
}

} // namespace

std::string store_kind_string(StoreKind kind) {
  switch (kind) {
    case StoreKind::Csv:
      return "csv";
    case StoreKind::Json:
      return "json";
    case StoreKind::Parquet:
      return "parquet";
  }
  return "json";
}
StoreKind parse_store_kind(std::string_view text) {
  const auto kind = lower_trimmed(text);
  if (kind == "csv") {
    return StoreKind::Csv;
  }
  if (kind == "json") {
    return StoreKind::Json;
  }
  if (kind == "parquet" || kind == "pq") {
    return StoreKind::Parquet;
  }
  static const bool pluss_vocab_1 = pluss::exception::names::define("store kind", {"csv", "json", "pq", "parquet"});
  (void)pluss_vocab_1;
  PLUSS_FAIL("Nexus::load: unsupported store kind '" + std::string(text) + "'");
}
std::string join_compact(const std::vector<std::string>& values, char sep) {
  std::string out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out.push_back(sep);
    }
    out += values[i];
  }
  return out;
}
std::vector<std::string> split_compact(std::string_view text, char sep) {
  std::vector<std::string> out;
  if (text.empty()) {
    return out;
  }
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(sep, start);
    if (pos == std::string_view::npos) {
      out.emplace_back(text.substr(start));
      break;
    }
    out.emplace_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return out;
}
std::string encode_candidate_keys(const std::vector<std::vector<std::string>>& keys) {
  std::vector<std::string> encoded;
  encoded.reserve(keys.size());
  for (const auto& keyset : keys) {
    encoded.push_back(join_compact(keyset, kInnerSep));
  }
  return join_compact(encoded, kOuterSep);
}
std::vector<std::vector<std::string>> decode_candidate_keys(std::string_view text) {
  std::vector<std::vector<std::string>> out;
  for (const auto& keyset : split_compact(text, kOuterSep)) {
    out.push_back(split_compact(keyset, kInnerSep));
  }
  return out;
}
std::string encode_relationship_keys(const std::vector<std::pair<std::string, std::string>>& keys) {
  std::vector<std::string> encoded;
  encoded.reserve(keys.size());
  for (const auto& [left, right] : keys) {
    encoded.push_back(left + std::string(1, kInnerSep) + right);
  }
  return join_compact(encoded, kOuterSep);
}
std::vector<std::pair<std::string, std::string>> decode_relationship_keys(std::string_view text) {
  std::vector<std::pair<std::string, std::string>> out;
  for (const auto& item : split_compact(text, kOuterSep)) {
    const auto parts = split_compact(item, kInnerSep);
    PLUSS_CHECK(parts.size() == 2,
                "Nexus::load: malformed relationship key payload");
    out.emplace_back(parts[0], parts[1]);
  }
  return out;
}
std::string case_label_policy_string(CaseLabelPolicy policy) {
  switch (policy) {
    case CaseLabelPolicy::Unknown:
      return "unknown";
    case CaseLabelPolicy::ImplicitRange:
      return "implicit_range";
    case CaseLabelPolicy::ExternalAxis:
      return "external_axis";
    case CaseLabelPolicy::FeatureDerived:
      return "feature_derived";
    case CaseLabelPolicy::Structured:
      return "structured";
  }
  return "unknown";
}
CaseLabelPolicy parse_case_label_policy(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "unknown") {
    return CaseLabelPolicy::Unknown;
  }
  if (value == "implicit_range") {
    return CaseLabelPolicy::ImplicitRange;
  }
  if (value == "external_axis") {
    return CaseLabelPolicy::ExternalAxis;
  }
  if (value == "feature_derived") {
    return CaseLabelPolicy::FeatureDerived;
  }
  if (value == "structured") {
    return CaseLabelPolicy::Structured;
  }
  static const bool pluss_vocab_2 = pluss::exception::names::define("case-label policy", {"unknown", "implicit_range", "external_axis", "feature_derived", "structured"});
  (void)pluss_vocab_2;
  PLUSS_FAIL("Nexus::load: unsupported case-label policy '" + std::string(text) + "'");
}
std::string relationship_cardinality_string(RelationshipCardinality cardinality) {
  switch (cardinality) {
    case RelationshipCardinality::OneToOne:
      return "one_to_one";
    case RelationshipCardinality::OneToMany:
      return "one_to_many";
    case RelationshipCardinality::ManyToOne:
      return "many_to_one";
    case RelationshipCardinality::ManyToMany:
      return "many_to_many";
  }
  return "many_to_many";
}
RelationshipCardinality parse_relationship_cardinality(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value == "one_to_one") {
    return RelationshipCardinality::OneToOne;
  }
  if (value == "one_to_many") {
    return RelationshipCardinality::OneToMany;
  }
  if (value == "many_to_one") {
    return RelationshipCardinality::ManyToOne;
  }
  if (value.empty() || value == "many_to_many") {
    return RelationshipCardinality::ManyToMany;
  }
  static const bool pluss_vocab_3 = pluss::exception::names::define("relationship cardinality", {"one_to_one", "one_to_many", "many_to_one", "many_to_many"});
  (void)pluss_vocab_3;
  PLUSS_FAIL("Nexus::load: unsupported relationship cardinality '" + std::string(text) + "'");
}
RelationshipValidationStatus parse_relationship_status(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "declared") {
    return RelationshipValidationStatus::Declared;
  }
  if (value == "validated") {
    return RelationshipValidationStatus::Validated;
  }
  if (value == "warning") {
    return RelationshipValidationStatus::Warning;
  }
  if (value == "invalid") {
    return RelationshipValidationStatus::Invalid;
  }
  static const bool pluss_vocab_4 = pluss::exception::names::define("relationship status", {"declared", "validated", "warning", "invalid"});
  (void)pluss_vocab_4;
  PLUSS_FAIL("Nexus::load: unsupported relationship status '" + std::string(text) + "'");
}
JoinHow parse_join_how(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "inner") {
    return JoinHow::Inner;
  }
  if (value == "left") {
    return JoinHow::Left;
  }
  if (value == "right") {
    return JoinHow::Right;
  }
  if (value == "outer") {
    return JoinHow::Outer;
  }
  if (value == "cross") {
    return JoinHow::Cross;
  }
  if (value == "left_only") {
    return JoinHow::LeftOnly;
  }
  if (value == "right_only") {
    return JoinHow::RightOnly;
  }
  static const bool pluss_vocab_5 = pluss::exception::names::define("join kind", {"inner", "left", "right", "outer", "cross", "left_only", "right_only"});
  (void)pluss_vocab_5;
  PLUSS_FAIL("Nexus::load: unsupported join kind '" + std::string(text) + "'");
}
std::string optional_text(const std::optional<std::string>& value) {
  return value.value_or("");
}
std::optional<std::string> parse_optional_text(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  return std::string(text);
}
std::string validation_status_string(RelationshipValidationStatus status) {
  switch (status) {
    case RelationshipValidationStatus::Declared:
      return "declared";
    case RelationshipValidationStatus::Validated:
      return "validated";
    case RelationshipValidationStatus::Warning:
      return "warning";
    case RelationshipValidationStatus::Invalid:
      return "invalid";
  }
  return "declared";
}

} // namespace pluss::relational
