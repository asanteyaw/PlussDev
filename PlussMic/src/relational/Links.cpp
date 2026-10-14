#include "relational/Links.h"

#include <algorithm>
#include <unordered_set>

#include "CodeM/core/Errors.h"

namespace pluss::relational {

namespace {

constexpr char kSep = '\x1d';

std::string pair_key(std::string_view a, std::string_view b) {
  std::string out(a);
  out.push_back(kSep);
  out += b;
  return out;
}

}  // namespace

// -- vocabulary ---------------------------------------------------------------

RelationshipCardinality reverse_cardinality(RelationshipCardinality cardinality) {
  switch (cardinality) {
    case RelationshipCardinality::OneToOne:
      return RelationshipCardinality::OneToOne;
    case RelationshipCardinality::OneToMany:
      return RelationshipCardinality::ManyToOne;
    case RelationshipCardinality::ManyToOne:
      return RelationshipCardinality::OneToMany;
    case RelationshipCardinality::ManyToMany:
      return RelationshipCardinality::ManyToMany;
  }
  return RelationshipCardinality::ManyToMany;
}

std::string cardinality_validate_token(RelationshipCardinality cardinality) {
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

std::string join_how_unify_token(JoinHow how) {
  switch (how) {
    case JoinHow::Inner:
      return "common";
    case JoinHow::Left:
      return "keep_left";
    case JoinHow::Right:
      return "keep_right";
    case JoinHow::Outer:
      return "unite";
    case JoinHow::Cross:
      return "cross";
    case JoinHow::LeftOnly:
      return "left_only";
    case JoinHow::RightOnly:
      return "right_only";
  }
  return "common";
}

std::string join_how_token(JoinHow how) {
  switch (how) {
    case JoinHow::Inner:
      return "inner";
    case JoinHow::Left:
      return "left";
    case JoinHow::Right:
      return "right";
    case JoinHow::Outer:
      return "outer";
    case JoinHow::Cross:
      return "cross";
    case JoinHow::LeftOnly:
      return "left_only";
    case JoinHow::RightOnly:
      return "right_only";
  }
  return "inner";
}

std::string stable_join_result_name(std::string_view left, std::string_view right, JoinHow how) {
  (void)how;
  return std::string(left) + "_" + std::string(right);
}

std::string stable_plan_result_name(std::string_view root, const std::vector<JoinPlanStep>& steps) {
  if (steps.empty()) {
    return std::string(root);
  }
  return std::string(root) + "_" + steps.back().right_table;
}

// -- link lookup ----------------------------------------------------------------

std::pair<const Relationship&, bool> require_oriented_relationship(
    const std::vector<Relationship>& relationships,
    std::string_view left,
    std::string_view right,
    std::string_view relationship_name,
    std::string_view where,
    const RelationshipIndex* index) {
  const Relationship* found = nullptr;
  if (index != nullptr) {
    const auto it = index->find(std::string(relationship_name));
    if (it != index->end()) found = &relationships[static_cast<std::size_t>(it->second)];
  } else {
    for (const auto& rel : relationships) {
      if (rel.name == relationship_name) {
        found = &rel;
        break;
      }
    }
  }
  PLUSS_CHECK(found != nullptr,
              std::string(where) + ": unknown relationship '" + std::string(relationship_name) + "'");
  const auto& rel = *found;
  if (rel.left_table == left && rel.right_table == right) {
    return {rel, false};
  }
  if (rel.left_table == right && rel.right_table == left) {
    return {rel, true};
  }
  PLUSS_FAIL(std::string(where) + ": relationship '" + std::string(relationship_name) +
             "' does not connect '" + std::string(left) + "' and '" + std::string(right) + "'");
}

std::vector<std::pair<std::string, std::string>> oriented_relationship_keys(const Relationship& rel,
                                                                             bool reversed) {
  if (!reversed) {
    return rel.keys;
  }
  std::vector<std::pair<std::string, std::string>> out;
  out.reserve(rel.keys.size());
  for (const auto& [left_key, right_key] : rel.keys) {
    out.emplace_back(right_key, left_key);
  }
  return out;
}

void validate_join_key_pairs(const std::vector<std::pair<std::string, std::string>>& keys,
                             std::string_view where) {
  PLUSS_CHECK(!keys.empty(), std::string(where) + ": join keys cannot be empty");
  std::unordered_set<std::string> seen;
  seen.reserve(keys.size() * 2 + 1);
  for (const auto& [left_key, right_key] : keys) {
    PLUSS_CHECK(!left_key.empty() && !right_key.empty(),
                std::string(where) + ": join key pairs cannot contain empty names");
    const auto signature = left_key + std::string("\n") + right_key;
    PLUSS_CHECK(seen.insert(signature).second,
                std::string(where) + ": duplicate join key pair '" + left_key + "' -> '" + right_key + "'");
  }
}

std::string require_direct_relationship_name(const std::vector<Relationship>& relationships,
                                             std::string_view left,
                                             std::string_view right,
                                             std::string_view where) {
  std::optional<std::string> found;
  for (const auto& rel : relationships) {
    const bool matches = (rel.left_table == left && rel.right_table == right) ||
                         (rel.left_table == right && rel.right_table == left);
    if (!matches) {
      continue;
    }
    PLUSS_CHECK(!found.has_value(),
                std::string(where) + ": multiple direct relationships connect '" + std::string(left) +
                    "' and '" + std::string(right) + "'. Specify JoinSpec.relationship_name explicitly");
    found = rel.name;
  }
  PLUSS_CHECK(found.has_value(),
              std::string(where) + ": no direct relationship connects '" + std::string(left) + "' and '" +
                  std::string(right) + "'");
  return *found;
}

// -- join-spec resolution -----------------------------------------------------

ResolvedJoin resolve_join(const std::vector<Relationship>& relationships,
                          std::string_view left,
                          std::string_view right,
                          const JoinSpec& spec,
                          std::string_view where,
                          bool discover_link,
                          const RelationshipIndex* index) {
  ResolvedJoin out;
  out.how = spec.how;
  out.right_suffix = spec.right_suffix.empty() ? std::string("_") + std::string(right) : spec.right_suffix;
  out.result_name = spec.result_name.value_or(stable_join_result_name(left, right, spec.how));

  std::optional<RelationshipCardinality> validate = spec.validate;
  std::vector<std::pair<std::string, std::string>> key_pairs = spec.keys;
  std::optional<std::string> relationship_name = spec.relationship_name;

  if (discover_link && !relationship_name.has_value() && key_pairs.empty() && spec.how != JoinHow::Cross) {
    relationship_name = require_direct_relationship_name(relationships, left, right, where);
  }

  if (relationship_name.has_value()) {
    const auto [rel, reversed] =
        require_oriented_relationship(relationships, left, right, *relationship_name, where, index);
    if (key_pairs.empty()) {
      key_pairs = oriented_relationship_keys(rel, reversed);
    }
    if (!validate.has_value()) {
      validate = reversed ? reverse_cardinality(rel.cardinality) : rel.cardinality;
    }
    out.relationship_name = *relationship_name;
  }

  if (spec.how == JoinHow::Cross) {
    PLUSS_CHECK(key_pairs.empty(), std::string(where) + ": cross join does not accept join keys");
    PLUSS_CHECK(!spec.relationship_name.has_value(),
                std::string(where) + ": cross join cannot be relationship-driven");
    out.validate.reset();
    return out;
  }

  validate_join_key_pairs(key_pairs, where);
  out.left_keys.reserve(key_pairs.size());
  out.right_keys.reserve(key_pairs.size());
  for (const auto& [left_key, right_key] : key_pairs) {
    out.left_keys.push_back(left_key);
    out.right_keys.push_back(right_key);
  }
  out.validate = validate;
  return out;
}

// -- paths and plans ----------------------------------------------------------

JoinPlan plan_from_path(const JoinPath& path, JoinHow how) {
  JoinPlan out;
  out.root_table = path.from;
  out.steps.reserve(path.steps.size());
  for (const auto& step : path.steps) {
    JoinSpec spec;
    spec.how = how;
    spec.right_suffix = std::string("_") + step.to_table;
    // A cross join uses the route only to choose the sequence of catalogs. It
    // must not carry relationship keys or a relationship binding into the
    // resolver, because Cartesian matching is intentionally keyless.
    if (how != JoinHow::Cross) spec.relationship_name = step.relationship_name;
    out.steps.push_back(JoinPlanStep{
        .left_table = step.from_table,
        .right_table = step.to_table,
        .spec = std::move(spec),
    });
  }
  out.result_name = stable_plan_result_name(out.root_table, out.steps);
  return out;
}

void LinkCache::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  adjacency_.clear();
  adjacency_built_ = false;
  paths_.clear();
  plans_.clear();
}

const LinkAdjacency& LinkCache::adjacency_locked_(const std::vector<std::string>& tables,
                                                   const std::vector<Relationship>& relationships) const {
  if (!adjacency_built_) {
    adjacency_.clear();
    adjacency_.reserve(tables.size() * 2 + 1);
    for (const auto& key : tables) {
      adjacency_.try_emplace(key);
    }
    for (const auto& rel : relationships) {
      adjacency_[rel.left_table].push_back(LinkEdge{rel.right_table, rel.name, false});
      adjacency_[rel.right_table].push_back(LinkEdge{rel.left_table, rel.name, true});
    }
    adjacency_built_ = true;
  }
  return adjacency_;
}

const LinkAdjacency& LinkCache::adjacency(const std::vector<std::string>& tables,
                                          const std::vector<Relationship>& relationships) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return adjacency_locked_(tables, relationships);
}

std::optional<JoinPath> LinkCache::search_locked_(const std::vector<std::string>& tables,
                                                  const std::vector<Relationship>& relationships,
                                                  std::string_view from,
                                                  std::string_view to) const {
  JoinPath out;
  out.from = std::string(from);
  out.to = std::string(to);
  if (from == to) {
    return out;
  }

  struct Trace {
    std::string prev{};
    std::string relationship_name{};
    bool reversed{false};
    bool seen{false};
  };

  std::unordered_map<std::string, Trace> trace;
  trace.reserve(tables.size() * 2 + 1);
  trace.emplace(std::string(from), Trace{"", "", false, true});
  const auto& adjacency = adjacency_locked_(tables, relationships);

  std::vector<std::string> frontier{std::string(from)};
  std::size_t head = 0;
  bool found = false;

  while (head < frontier.size() && !found) {
    const auto current = frontier[head++];
    const auto adj_it = adjacency.find(current);
    if (adj_it == adjacency.end()) {
      continue;
    }
    for (const auto& edge : adj_it->second) {
      const auto& next = edge.next;
      if (trace.find(next) != trace.end()) {
        continue;
      }
      trace.emplace(next, Trace{current, edge.relationship_name, edge.reversed, true});
      frontier.push_back(next);
      if (next == to) {
        found = true;
        break;
      }
    }
  }
  if (!found) {
    return std::nullopt;
  }

  std::vector<JoinPathStep> steps;
  auto current = std::string(to);
  while (current != from) {
    const auto it = trace.find(current);
    PLUSS_CHECK(it != trace.end() && it->second.seen, "path reconstruction failure");
    steps.push_back(JoinPathStep{
        .relationship_name = it->second.relationship_name,
        .from_table = it->second.prev,
        .to_table = current,
        .reversed = it->second.reversed,
    });
    current = it->second.prev;
  }
  std::reverse(steps.begin(), steps.end());
  out.steps = std::move(steps);
  return out;
}

std::optional<JoinPath> LinkCache::try_path(const std::vector<std::string>& tables,
                                            const std::vector<Relationship>& relationships,
                                            std::string_view from,
                                            std::string_view to) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto key = pair_key(from, to);
  if (const auto it = paths_.find(key); it != paths_.end()) {
    return it->second;
  }
  auto computed = search_locked_(tables, relationships, from, to);
  paths_.emplace(key, computed);
  return computed;
}

JoinPath LinkCache::path(const std::vector<std::string>& tables,
                         const std::vector<Relationship>& relationships,
                         std::string_view from,
                         std::string_view to,
                         std::string_view where) const {
  auto found = try_path(tables, relationships, from, to);
  PLUSS_CHECK(found.has_value(),
              std::string(where) + ": no relationship path from '" + std::string(from) + "' to '" +
                  std::string(to) + "'");
  return *found;
}

JoinPlan LinkCache::plan(const std::vector<std::string>& tables,
                         const std::vector<Relationship>& relationships,
                         std::string_view from,
                         std::string_view to,
                         JoinHow how,
                         std::string_view where) const {
  auto key = pair_key(from, to);
  key.push_back(kSep);
  key += std::to_string(static_cast<int>(how));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (const auto it = plans_.find(key); it != plans_.end()) {
      return it->second;
    }
  }
  auto computed = plan_from_path(path(tables, relationships, from, to, where), how);
  std::lock_guard<std::mutex> lock(mutex_);
  return plans_.emplace(std::move(key), std::move(computed)).first->second;
}

} // namespace pluss::relational
