#pragma once

// Table-agnostic key matching. Engines encode their key columns as integers
// (equal values get equal codes, null/missing keys get kNoKey), call match(),
// and gather their own columns through the returned index pairs. Nothing here
// knows whether a record lives in a DataFrame row or a Catalog stream position.

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "relational/Types.h"

namespace pluss::relational {

// Code for a record whose key is null or missing: it never matches anything.
inline constexpr std::int64_t kNoKey = std::numeric_limits<std::int64_t>::min();

// The key columns of one side: cols[c][i] is the code of record i in key
// column c. Every column has `records` entries.
struct KeyColumns {
  std::vector<const std::int64_t*> cols{};
  std::int64_t records{0};
};

// What a match did to the records of each side. Nothing is lost without being
// counted here.
struct MatchReport {
  std::int64_t left_records{0};
  std::int64_t right_records{0};
  std::int64_t left_unkeyed{0};      // records with a null/missing key (never match)
  std::int64_t right_unkeyed{0};
  std::int64_t pairs{0};             // output pairs, matched or not
  std::int64_t matched_pairs{0};     // pairs with a partner on both sides
  std::int64_t left_unmatched{0};    // records with no partner (unkeyed included)
  std::int64_t right_unmatched{0};
  std::int64_t left_dropped{0};      // records absent from the output
  std::int64_t right_dropped{0};
  // Whether keyed records have pairwise distinct keys. Only measured when the
  // contract or the caller needs it, or when the side was hashed anyway.
  bool left_unique{false};
  bool left_unique_known{false};
  bool right_unique{false};
  bool right_unique_known{false};
};

struct MatchOptions {
  JoinHow how{JoinHow::Inner};
  // Contract to enforce; throws when the data contradicts it.
  std::optional<RelationshipCardinality> validate{};
  // Measure left-key uniqueness even if no contract needs it (costs a hash build).
  bool measure_left_unique{false};
  std::string_view where{"join"};
};

// Index pairs in output order: output row p joins left record left[p] with
// right record right[p]; -1 means that side has no record in the row.
// Order: left records in order, each followed by its matches in right order;
// for Outer/Right, right-only records follow in right order.
struct MatchResult {
  std::vector<std::int64_t> left{};
  std::vector<std::int64_t> right{};
  MatchReport report{};
};

MatchResult match(const KeyColumns& left, const KeyColumns& right, const MatchOptions& options);

// A hashed right-hand side that can be reused by many matches (owns its codes and
// its table). Matching against it is identical to the plain overload; a Right or
// RightOnly join, which hashes the left side instead, ignores the prebuilt table.
class BuiltSide {
public:
  BuiltSide(std::vector<std::vector<std::int64_t>> codes, std::int64_t records);
  KeyColumns view() const;
  std::int64_t records() const;
  std::size_t bytes() const;  // approximate memory held

private:
  friend MatchResult match(const KeyColumns&, const BuiltSide&, const MatchOptions&);
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

MatchResult match(const KeyColumns& left, const BuiltSide& right, const MatchOptions& options);

// Compose a chain of hops without materializing the tables in between.
// (root[k], mid[k]) lists records of the root table and the mid table they
// currently reach; `next` is a match whose left side is the mid table. Every pair
// is replaced by one pair per partner of its mid record, so a path root -> a -> b
// is two matches and one expand. With keep_unmatched, a pair whose mid record has
// no partner (or whose mid is -1) survives as (root, -1); otherwise it is dropped.
// Order is preserved: pairs stay grouped by root, partners in match order.
struct Expansion {
  std::vector<std::int64_t> root{};
  std::vector<std::int64_t> leaf{};
};
Expansion expand(const std::vector<std::int64_t>& root,
                 const std::vector<std::int64_t>& mid,
                 const MatchResult& next,
                 std::int64_t mid_records,
                 bool keep_unmatched);

// Do the keyed records of one side have pairwise distinct keys? Also counts
// the unkeyed records.
struct UniquenessReport {
  bool unique{true};
  std::int64_t keyed{0};
  std::int64_t unkeyed{0};
};
UniquenessReport measure_uniqueness(const KeyColumns& side);

} // namespace pluss::relational
