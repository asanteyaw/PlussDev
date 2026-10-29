#pragma once

// Table-agnostic relational vocabulary shared by Nexus (DataFrames) and Locus
// (Catalogs): link declarations, cardinality contracts, join/pull specs, plans,
// table metadata and storage options. Nothing here knows what a table is.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "loader/Config.h"

namespace pluss::relational {

enum class CaseLabelPolicy : std::uint8_t {
  Unknown = 0,
  ImplicitRange = 1,
  ExternalAxis = 2,
  FeatureDerived = 3,
  Structured = 4,
};

enum class RelationshipCardinality : std::uint8_t {
  OneToOne = 0,
  OneToMany = 1,
  ManyToOne = 2,
  ManyToMany = 3,
};

enum class RelationshipValidationStatus : std::uint8_t {
  Declared = 0,
  Validated = 1,
  Warning = 2,
  Invalid = 3,
};

enum class RelationshipSide : std::uint8_t {
  Left = 0,
  Right = 1,
};

enum class JoinHow : std::uint8_t {
  Inner = 0,
  Left = 1,
  Right = 2,
  Outer = 3,
  Cross = 4,
  LeftOnly = 5,
  RightOnly = 6,
};

enum class StoreKind : std::uint8_t {
  Csv = 0,
  Json = 1,
  Parquet = 2,
};

enum class PullOp : std::uint8_t {
  Take = 0,
  Count = 1,
  Ndistinct = 2,
  Sum = 3,
  Mean = 4,
  Median = 5,
  Min = 6,
  Max = 7,
  Std = 8,
  Var = 9,
};

struct TableMeta {
  std::optional<std::string> grain{};
  std::vector<std::string> primary_key{};
  std::vector<std::vector<std::string>> candidate_keys{};
  std::optional<std::string> time_field{};
  CaseLabelPolicy case_labels{CaseLabelPolicy::Unknown};
  std::optional<std::string> source{};
  std::optional<std::string> provenance{};
  std::optional<std::string> note{};
};

struct Relationship {
  std::string name{};
  std::string left_table{};
  std::string right_table{};
  std::vector<std::pair<std::string, std::string>> keys{};
  RelationshipCardinality cardinality{RelationshipCardinality::ManyToOne};
  bool left_optional{false};
  bool right_optional{false};
  double confidence{1.0};
  RelationshipValidationStatus status{RelationshipValidationStatus::Declared};
  std::optional<std::string> note{};
};

struct JoinSpec {
  std::vector<std::pair<std::string, std::string>> keys{};
  JoinHow how{JoinHow::Inner};
  std::string right_suffix{};
  std::optional<RelationshipCardinality> validate{};
  std::optional<std::string> relationship_name{};
  std::optional<std::string> result_name{};
};

struct JoinPathStep {
  std::string relationship_name{};
  std::string from_table{};
  std::string to_table{};
  bool reversed{false};
};

struct JoinPath {
  std::string from{};
  std::string to{};
  std::vector<JoinPathStep> steps{};
};

struct JoinPlanStep {
  std::string left_table{};
  std::string right_table{};
  JoinSpec spec{};
};

struct JoinPlan {
  std::string root_table{};
  std::string result_name{};
  std::vector<JoinPlanStep> steps{};
};

struct StoreOptions {
  StoreKind kind{StoreKind::Parquet};
  std::string manifest{"manifest.csv"};
  std::string schema{"schema.csv"};
  std::string graph{"graph.csv"};
  std::string views{"views.csv"};
  std::string tables{"tables"};
};

struct LoadOptions {
  std::vector<std::string> load{};
  std::vector<std::string> ignore{};
  std::vector<std::string> formats{"csv", "txt", "json", "parquet"};
  bool recursive{false};
  bool keep_extension{false};
  bool parallel{true};
  std::optional<std::size_t> max_workers{};
  pluss::loader::Config config{};
};

struct PullWindow {
  std::int64_t value{0};
  std::string unit{};
  bool anchor_root{false};
};

struct PullSpec {
  std::string source{};
  PullOp op{PullOp::Take};
  std::optional<std::string> alias{};
  std::optional<PullWindow> window{};
  bool prefer_feature_alias{false};

  PullSpec named(std::string_view value) const;
  PullSpec windowed(std::int64_t value, std::string_view unit) const;
  PullSpec root_windowed(std::int64_t value, std::string_view unit) const;
};

} // namespace pluss::relational
