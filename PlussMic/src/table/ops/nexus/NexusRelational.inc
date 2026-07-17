namespace {

struct NexusResolvedJoinSpec {
  std::vector<std::string> left_keys{};
  std::vector<std::string> right_keys{};
  std::string how{};
  std::string validate{};
  std::string right_suffix{};
  std::string result_name{};
};

RelationshipCardinality reverse_relationship_cardinality(RelationshipCardinality cardinality) {
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

std::string join_how_string(JoinHow how) {
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

std::string relationship_cardinality_validate_string(RelationshipCardinality cardinality) {
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

std::string stable_join_result_name(std::string_view left,
                                    std::string_view right,
                                    JoinHow how) {
  (void)how;
  return std::string(left) + "_" + std::string(right);
}

std::string stable_plan_result_name(std::string_view root,
                                    const std::vector<JoinPlanStep>& steps) {
  if (steps.empty()) {
    return std::string(root);
  }
  return std::string(root) + "_" + steps.back().right_table;
}

std::pair<const Relationship&, bool> require_oriented_relationship(const detail::NexusImpl& src,
                                                                   std::string_view left,
                                                                   std::string_view right,
                                                                   std::string_view relationship_name,
                                                                   std::string_view where) {
  const auto rel_it = src.rel_name_to_pos_.find(std::string(relationship_name));
  PLUSS_CHECK(rel_it != src.rel_name_to_pos_.end(),
              std::string(where) + ": unknown relationship '" + std::string(relationship_name) + "'");
  const auto& rel = src.relationships_[static_cast<std::size_t>(rel_it->second)];
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

std::string require_direct_relationship_name(const detail::NexusImpl& src,
                                             std::string_view left,
                                             std::string_view right,
                                             std::string_view where) {
  std::optional<std::string> found;
  for (const auto& rel : src.relationships_) {
    const bool matches =
        (rel.left_table == left && rel.right_table == right) ||
        (rel.left_table == right && rel.right_table == left);
    if (!matches) {
      continue;
    }
    PLUSS_CHECK(!found.has_value(),
                std::string(where) + ": multiple direct relationships connect '" +
                    std::string(left) + "' and '" + std::string(right) +
                    "'. Specify JoinSpec.relationship_name explicitly");
    found = rel.name;
  }
  PLUSS_CHECK(found.has_value(),
              std::string(where) + ": no direct relationship connects '" +
                  std::string(left) + "' and '" + std::string(right) + "'");
  return *found;
}

NexusResolvedJoinSpec resolve_nexus_join_spec(const detail::NexusImpl& src,
                                              const DataFrame& left_frame,
                                              std::string_view left,
                                              const DataFrame& right_frame,
                                              std::string_view right,
                                              const JoinSpec& spec,
                                              std::string_view where) {
  NexusResolvedJoinSpec out;
  out.how = join_how_string(spec.how);
  out.right_suffix = spec.right_suffix.empty()
      ? std::string("_") + std::string(right)
      : spec.right_suffix;
  out.result_name = spec.result_name.value_or(stable_join_result_name(left, right, spec.how));

  std::optional<RelationshipCardinality> validate = spec.validate;
  std::vector<std::pair<std::string, std::string>> key_pairs = spec.keys;

  if (spec.relationship_name.has_value()) {
    const auto [rel, reversed] = require_oriented_relationship(
        src, left, right, *spec.relationship_name, where);
    if (key_pairs.empty()) {
      key_pairs = oriented_relationship_keys(rel, reversed);
    }
    if (!validate.has_value()) {
      validate = reversed
          ? reverse_relationship_cardinality(rel.cardinality)
          : rel.cardinality;
    }
  }

  if (spec.how == JoinHow::Cross) {
    PLUSS_CHECK(key_pairs.empty(),
                std::string(where) + ": cross join does not accept join keys");
    PLUSS_CHECK(!spec.relationship_name.has_value(),
                std::string(where) + ": cross join cannot be relationship-driven");
    out.validate.clear();
    return out;
  }

  validate_join_key_pairs(key_pairs, where);
  out.left_keys.reserve(key_pairs.size());
  out.right_keys.reserve(key_pairs.size());
  for (const auto& [left_key, right_key] : key_pairs) {
    out.left_keys.push_back(left_key);
    out.right_keys.push_back(right_key);
  }
  require_frame_features(left_frame, out.left_keys, where);
  require_frame_features(right_frame, out.right_keys, where);

  if (validate.has_value()) {
    out.validate = relationship_cardinality_validate_string(*validate);
  }
  return out;
}

std::vector<std::string_view> key_views(const std::vector<std::string>& keys) {
  std::vector<std::string_view> out;
  out.reserve(keys.size());
  for (const auto& key : keys) {
    out.push_back(key);
  }
  return out;
}

} // namespace

DataFrame Nexus::join(std::string_view left, std::string_view right, const JoinSpec& spec) const {
  const auto& src = impl_ref_();
  require_known_table(src, left, "Nexus::join");
  require_known_table(src, right, "Nexus::join");

  const auto left_frame = frame(left);
  const auto right_frame = frame(right);
  const auto resolved = resolve_nexus_join_spec(
      src, left_frame, left, right_frame, right, spec, "Nexus::join");

  if (spec.how == JoinHow::Cross) {
    return left_frame.unify({}, right_frame, {}, resolved.how, resolved.right_suffix)
        .title(resolved.result_name);
  }

  const auto left_views = key_views(resolved.left_keys);
  const auto right_views = key_views(resolved.right_keys);
  return left_frame.unify(pluss::msharp::RangeView::Of<std::string_view>(left_views),
                          right_frame,
                          pluss::msharp::RangeView::Of<std::string_view>(right_views),
                          resolved.how,
                          resolved.right_suffix,
                          resolved.validate)
      .title(resolved.result_name);
}

JoinPath Nexus::path(std::string_view from, std::string_view to) const {
  const auto& src = impl_ref_();
  require_known_table(src, from, "Nexus::path");
  require_known_table(src, to, "Nexus::path");

  const auto cache_key = nexus_path_cache_key(from, to);
  if (const auto it = src.path_cache_.find(cache_key); it != src.path_cache_.end()) {
    return it->second;
  }

  JoinPath out;
  out.from = std::string(from);
  out.to = std::string(to);
  if (from == to) {
    src.path_cache_.emplace(std::move(cache_key), out);
    return out;
  }

  struct Trace {
    std::string prev{};
    std::string relationship_name{};
    bool reversed{false};
    bool seen{false};
  };

  std::unordered_map<std::string, Trace> trace;
  trace.reserve(src.keys_.size() * 2 + 1);
  trace.emplace(std::string(from), Trace{"", "", false, true});
  const auto& adjacency = nexus_relationship_adjacency(src);

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

  PLUSS_CHECK(found,
              "Nexus::path: no relationship path from '" + std::string(from) + "' to '" + std::string(to) + "'");

  std::vector<JoinPathStep> steps;
  auto current = std::string(to);
  while (current != from) {
    const auto it = trace.find(current);
    PLUSS_CHECK(it != trace.end() && it->second.seen,
                "Nexus::path: internal path reconstruction failure");
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
  src.path_cache_.emplace(std::move(cache_key), out);
  return out;
}

JoinPlan Nexus::plan(const JoinPath& path_value, JoinHow how) const {
  JoinPlan out;
  out.root_table = path_value.from;
  out.steps.reserve(path_value.steps.size());
  for (const auto& step : path_value.steps) {
    JoinSpec spec;
    spec.how = how;
    spec.right_suffix = std::string("_") + step.to_table;
    spec.relationship_name = step.relationship_name;
    out.steps.push_back(JoinPlanStep{
        .left_table = step.from_table,
        .right_table = step.to_table,
        .spec = std::move(spec),
    });
  }
  out.result_name = stable_plan_result_name(out.root_table, out.steps);
  return out;
}

JoinPlan Nexus::plan(std::string_view from, std::string_view to, JoinHow how) const {
  return plan(path(from, to), how);
}

DataFrame Nexus::materialize(const JoinPlan& plan_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!plan_value.root_table.empty(), "Nexus::materialize: root_table cannot be empty");
  require_known_table(src, plan_value.root_table, "Nexus::materialize");

  auto current = frame(plan_value.root_table);
  if (plan_value.steps.empty()) {
    const auto result_name = plan_value.result_name.empty()
        ? plan_value.root_table
        : plan_value.result_name;
    return current.title(result_name);
  }

  for (const auto& step : plan_value.steps) {
    require_known_table(src, step.right_table, "Nexus::materialize");
    const auto right_frame = frame(step.right_table);
    const auto resolved = resolve_nexus_join_spec(
        src, current, step.left_table, right_frame, step.right_table, step.spec, "Nexus::materialize");

    if (step.spec.how == JoinHow::Cross) {
      current = current.unify({}, right_frame, {}, resolved.how, resolved.right_suffix);
      continue;
    }

    const auto left_views = key_views(resolved.left_keys);
    const auto right_views = key_views(resolved.right_keys);
    current = current.unify(pluss::msharp::RangeView::Of<std::string_view>(left_views),
                            right_frame,
                            pluss::msharp::RangeView::Of<std::string_view>(right_views),
                            resolved.how,
                            resolved.right_suffix,
                            resolved.validate);
  }

  const auto result_name = plan_value.result_name.empty()
      ? stable_plan_result_name(plan_value.root_table, plan_value.steps)
      : plan_value.result_name;
  return current.title(result_name);
}
