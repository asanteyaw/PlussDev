#pragma once

// Text forms of the relational types (links, table metadata, join plans, store
// kinds), used by every persisted workspace. Pure string logic; shared by Nexus
// and Locus so a saved link or plan reads back identically whichever wrote it.

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "relational/Types.h"

namespace pluss::relational {

inline constexpr char kInnerSep = '\x1f';  // between items of one cell
inline constexpr char kOuterSep = '\x1e';  // between groups of items in one cell

std::string store_kind_string(StoreKind kind);
StoreKind parse_store_kind(std::string_view text);
std::string join_compact(const std::vector<std::string>& values, char sep);
std::vector<std::string> split_compact(std::string_view text, char sep);
std::string encode_candidate_keys(const std::vector<std::vector<std::string>>& keys);
std::vector<std::vector<std::string>> decode_candidate_keys(std::string_view text);
std::string encode_relationship_keys(const std::vector<std::pair<std::string, std::string>>& keys);
std::vector<std::pair<std::string, std::string>> decode_relationship_keys(std::string_view text);
std::string case_label_policy_string(CaseLabelPolicy policy);
CaseLabelPolicy parse_case_label_policy(std::string_view text);
std::string relationship_cardinality_string(RelationshipCardinality cardinality);
RelationshipCardinality parse_relationship_cardinality(std::string_view text);
RelationshipValidationStatus parse_relationship_status(std::string_view text);
std::string validation_status_string(RelationshipValidationStatus status);
JoinHow parse_join_how(std::string_view text);
std::string optional_text(const std::optional<std::string>& value);
std::optional<std::string> parse_optional_text(std::string_view text);

} // namespace pluss::relational
