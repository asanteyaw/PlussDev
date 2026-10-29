#pragma once

// Table-agnostic link logic shared by Nexus and Locus: vocabulary mappings,
// oriented / direct relationship lookup, join-spec resolution, and cached
// path/plan search. Engines keep their own tables; they ask this module which
// tables to join, on which keys, in which order, under which contract.

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "relational/Types.h"

namespace pluss::relational {

// -- vocabulary ---------------------------------------------------------------

// The contract as seen from the other end of the link.
RelationshipCardinality reverse_cardinality(RelationshipCardinality cardinality);
// "one_to_one" | "one_to_many" | "many_to_one" | "many_to_many".
std::string cardinality_validate_token(RelationshipCardinality cardinality);
// The how-vocabulary of DataFrame::unify: "common", "keep_left", "unite", ...
std::string join_how_unify_token(JoinHow how);
// "inner" | "left" | "right" | "outer" | "cross" | "left_only" | "right_only".
std::string join_how_token(JoinHow how);

std::string stable_join_result_name(std::string_view left, std::string_view right, JoinHow how);
std::string stable_plan_result_name(std::string_view root, const std::vector<JoinPlanStep>& steps);

// -- link lookup ----------------------------------------------------------------

using RelationshipIndex = std::unordered_map<std::string, std::int64_t>;

// The named relationship, which must connect `left` and `right` in one
// direction or the other; `reversed` is true when it is declared right->left.
// `index` (name -> position in `relationships`) makes the lookup O(1) when the
// caller keeps one; without it the list is scanned.
std::pair<const Relationship&, bool> require_oriented_relationship(
    const std::vector<Relationship>& relationships,
    std::string_view left,
    std::string_view right,
    std::string_view relationship_name,
    std::string_view where,
    const RelationshipIndex* index = nullptr);

// Key pairs oriented left->right for the requested direction.
std::vector<std::pair<std::string, std::string>> oriented_relationship_keys(const Relationship& rel,
                                                                             bool reversed);

void validate_join_key_pairs(const std::vector<std::pair<std::string, std::string>>& keys,
                             std::string_view where);

// The one relationship that directly connects two tables. Never guesses: no
// link and several links are both errors (the latter asks for an explicit name).
std::string require_direct_relationship_name(const std::vector<Relationship>& relationships,
                                             std::string_view left,
                                             std::string_view right,
                                             std::string_view where);

// -- join-spec resolution -----------------------------------------------------

struct ResolvedJoin {
  JoinHow how{JoinHow::Inner};
  std::vector<std::string> left_keys{};
  std::vector<std::string> right_keys{};
  std::optional<RelationshipCardinality> validate{};  // contract to enforce, if any
  std::string right_suffix{};
  std::string result_name{};
  std::optional<std::string> relationship_name{};     // the link that supplied the keys
};

// Resolve a JoinSpec between two tables: keys (explicit, or from the named
// link), the cardinality contract (explicit, or the named link's, flipped when
// walked backwards), suffix and result name. Does not look at table contents:
// the engine checks that the key columns exist.
// With `discover_link`, a spec that names neither keys nor a relationship uses
// the single direct link between the two tables (see require_direct_...).
ResolvedJoin resolve_join(const std::vector<Relationship>& relationships,
                          std::string_view left,
                          std::string_view right,
                          const JoinSpec& spec,
                          std::string_view where,
                          bool discover_link = false,
                          const RelationshipIndex* index = nullptr);

// -- paths and plans ----------------------------------------------------------

struct LinkEdge {
  std::string next{};
  std::string relationship_name{};
  bool reversed{false};
};

using LinkAdjacency = std::unordered_map<std::string, std::vector<LinkEdge>>;

// The plan that walks a path hop by hop (left joins unless told otherwise).
JoinPlan plan_from_path(const JoinPath& path, JoinHow how);

// Adjacency, path and plan results for one set of relationships, computed
// lazily and reused until clear(). Thread-safe for concurrent const use.
class LinkCache {
public:
  LinkCache() = default;
  LinkCache(const LinkCache&) = delete;
  LinkCache& operator=(const LinkCache&) = delete;

  // Forget everything (call after tables or relationships change).
  void clear();

  // table -> outgoing edges, in relationship declaration order. Every table in
  // `tables` has an entry. The reference stays valid until clear().
  const LinkAdjacency& adjacency(const std::vector<std::string>& tables,
                                 const std::vector<Relationship>& relationships) const;

  // Fewest-hop path (ties broken by declaration order). Errors when none.
  JoinPath path(const std::vector<std::string>& tables,
                const std::vector<Relationship>& relationships,
                std::string_view from,
                std::string_view to,
                std::string_view where) const;
  // Same search, but "no path" is an empty result instead of an error.
  std::optional<JoinPath> try_path(const std::vector<std::string>& tables,
                                   const std::vector<Relationship>& relationships,
                                   std::string_view from,
                                   std::string_view to) const;

  JoinPlan plan(const std::vector<std::string>& tables,
                const std::vector<Relationship>& relationships,
                std::string_view from,
                std::string_view to,
                JoinHow how,
                std::string_view where) const;

private:
  const LinkAdjacency& adjacency_locked_(const std::vector<std::string>& tables,
                                         const std::vector<Relationship>& relationships) const;
  std::optional<JoinPath> search_locked_(const std::vector<std::string>& tables,
                                         const std::vector<Relationship>& relationships,
                                         std::string_view from,
                                         std::string_view to) const;

  mutable std::mutex mutex_{};
  mutable bool adjacency_built_{false};
  mutable LinkAdjacency adjacency_{};
  mutable std::unordered_map<std::string, std::optional<JoinPath>> paths_{};
  mutable std::unordered_map<std::string, JoinPlan> plans_{};
};

} // namespace pluss::relational
