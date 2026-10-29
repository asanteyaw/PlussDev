#pragma once

// Join vocabulary of the kindle stack. Catalog owns the engine (Catalog::join,
// join_detailed, audit_keys); Locus and anything above it resolve links and
// delegate here. Raggedness is decided by a RaggedPolicy (Relational class
// defaults): keep every record, nulls where a row has no partner, losses
// counted in the report.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kindle/Ragged.h"
#include "relational/Match.h"
#include "relational/Types.h"

namespace pluss::kindle {

class Catalog;
struct CatalogJoinResult;
struct CatalogMatch;

struct CatalogJoinSpec {
  relational::JoinHow how{relational::JoinHow::Inner};
  std::vector<std::string> left_keys{};
  std::vector<std::string> right_keys{};
  // Contract to enforce between the two sides' keys (throws when contradicted).
  std::optional<relational::RelationshipCardinality> validate{};
  // Appended to a right column whose name the left side already has.
  std::string right_suffix{"_right"};
};

struct CatalogJoinOptions {
  RaggedPolicy ragged{RaggedPolicy::defaults(OpClass::Relational)};
  // A left key and its right partner that share a name become one column, but
  // only when the output cannot contain right-only records (so no value is lost).
  bool coalesce_keys{true};
};

// What a join did to the records of both sides. Nothing is lost uncounted.
struct CatalogJoinReport {
  relational::MatchReport match{};
  std::int64_t output_records{0};   // rows in the output
  std::int64_t trimmed_left{0};     // indices cut by the policy's domain
  std::int64_t trimmed_right{0};
  std::string summary() const;
};

// Key uniqueness and orphan counts for a pair of key sets.
struct CatalogKeyAudit {
  std::int64_t left_records{0};
  std::int64_t right_records{0};
  bool left_unique{false};
  bool right_unique{false};
  std::int64_t left_unkeyed{0};    // records with a null or missing key
  std::int64_t right_unkeyed{0};
  std::int64_t left_orphans{0};    // records with no partner (unkeyed included)
  std::int64_t right_orphans{0};
};

} // namespace pluss::kindle
