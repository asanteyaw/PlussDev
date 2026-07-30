namespace {

struct NexusResolvedJoinSpec {
  std::vector<std::string> left_keys{};
  std::vector<std::string> right_keys{};
  std::string how{};
  std::string validate{};
  std::string right_suffix{};
  std::string result_name{};
};

struct NexusPullField {
  std::string table{};
  std::string feature{};
  std::string token{};
  std::optional<std::string> aggregate{};
};

enum class NexusPullAggregateOp : std::uint8_t {
  Count = 0,
  Ndistinct = 1,
  Sum = 2,
  Mean = 3,
  Min = 4,
  Max = 5,
};

struct NexusPullAggregateSpec {
  enum class Anchor : std::uint8_t {
    Latest = 0,
    Root = 1,
  };

  NexusPullAggregateOp op{NexusPullAggregateOp::Count};
  std::string token{};
  std::optional<std::int64_t> window_value{};
  std::optional<std::string> window_unit{};
  Anchor anchor{Anchor::Latest};
};

struct NexusPullExpandedFrame {
  DataFrame frame{};
  std::unordered_map<std::string, std::string> projected{};
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

std::string pull_field_key(std::string_view table, std::string_view feature) {
  return std::string(table) + "." + std::string(feature);
}

std::string pull_request_key(const NexusPullField& field) {
  auto out = pull_field_key(field.table, field.feature);
  if (field.aggregate.has_value()) {
    out += ".";
    out += *field.aggregate;
  }
  return out;
}

bool is_pull_backtick_quoted(std::string_view token) {
  return token.size() >= 2 && token.front() == '`' && token.back() == '`';
}

std::string unquote_pull_token(std::string_view token, std::string_view where) {
  PLUSS_CHECK(is_pull_backtick_quoted(token),
              std::string(where) + ": malformed quoted field token '" + std::string(token) + "'");
  return std::string(token.substr(1, token.size() - 2));
}

std::string lower_ascii(std::string_view token) {
  std::string out(token);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

std::string join_pull_parts(const std::vector<std::string>& parts,
                            std::size_t begin,
                            std::size_t end) {
  std::string out;
  for (std::size_t i = begin; i < end; ++i) {
    if (!out.empty()) {
      out += ".";
    }
    out += parts[i];
  }
  return out;
}

std::vector<std::string> split_pull_dots(std::string_view token) {
  std::vector<std::string> out;
  std::string current;
  bool quoted = false;
  for (const char ch : token) {
    if (ch == '`') {
      quoted = !quoted;
      current.push_back(ch);
      continue;
    }
    if (ch == '.' && !quoted) {
      out.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  out.push_back(current);
  return out;
}

std::optional<NexusPullAggregateSpec> parse_pull_aggregate(std::string_view token) {
  const auto lower_token = lower_ascii(token);
  auto lower = lower_token;
  NexusPullAggregateSpec::Anchor anchor = NexusPullAggregateSpec::Anchor::Latest;
  constexpr std::string_view root_suffix = "_root";
  if (lower.size() > root_suffix.size() &&
      lower.ends_with(root_suffix)) {
    anchor = NexusPullAggregateSpec::Anchor::Root;
    lower.resize(lower.size() - root_suffix.size());
  }
  if (lower == "count") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Count,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "ndistinct") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Ndistinct,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "sum") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Sum,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "mean") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Mean,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "min") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Min,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "max") {
    return NexusPullAggregateSpec{
        .op = NexusPullAggregateOp::Max,
        .token = std::string(token),
        .anchor = anchor,
    };
  }

  const auto split = lower.rfind('_');
  if (split == std::string::npos || split == 0 || split + 1 >= lower.size()) {
    return std::nullopt;
  }

  NexusPullAggregateSpec out;
  const auto head = lower.substr(0, split);
  if (head == "count") {
    out.op = NexusPullAggregateOp::Count;
  } else if (head == "sum") {
    out.op = NexusPullAggregateOp::Sum;
  } else if (head == "mean") {
    out.op = NexusPullAggregateOp::Mean;
  } else if (head == "min") {
    out.op = NexusPullAggregateOp::Min;
  } else if (head == "max") {
    out.op = NexusPullAggregateOp::Max;
  } else {
    return std::nullopt;
  }

  const auto tail = lower.substr(split + 1);
  std::size_t digits_end = 0;
  while (digits_end < tail.size() && std::isdigit(static_cast<unsigned char>(tail[digits_end]))) {
    ++digits_end;
  }
  if (digits_end == 0 || digits_end == tail.size()) {
    return std::nullopt;
  }
  const auto value_text = tail.substr(0, digits_end);
  const auto unit = tail.substr(digits_end);
  auto value = std::int64_t{0};
  const auto* begin = value_text.data();
  const auto* end = value_text.data() + value_text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || value <= 0) {
    return std::nullopt;
  }
  out.token = std::string(token);
  out.window_value = value;
  out.window_unit = unit;
  out.anchor = anchor;
  return out;
}

std::string pull_alias_piece(std::string_view token) {
  std::string out;
  out.reserve(token.size());
  bool underscore = false;
  for (const char ch : token) {
    if (std::isalnum(static_cast<unsigned char>(ch))) {
      out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
      underscore = false;
      continue;
    }
    if (!underscore && !out.empty()) {
      out.push_back('_');
      underscore = true;
    }
  }
  while (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out.empty() ? "field" : out;
}

std::string normalize_pull_feature_token(std::string_view token, std::string_view where) {
  return is_pull_backtick_quoted(token)
      ? unquote_pull_token(token, where)
      : std::string(token);
}

std::string pull_output_alias(std::string_view root, const NexusPullField& field) {
  if (!field.aggregate.has_value()) {
    return field.table == root
        ? field.feature
        : pull_alias_piece(field.table) + "_" + pull_alias_piece(field.feature);
  }
  return pull_alias_piece(field.table) + "_" + pull_alias_piece(field.feature) + "_" +
      pull_alias_piece(*field.aggregate);
}

std::int64_t find_pull_first_dot(std::string_view token) {
  bool quoted = false;
  for (std::size_t i = 0; i < token.size(); ++i) {
    if (token[i] == '`') {
      quoted = !quoted;
      continue;
    }
    if (token[i] == '.' && !quoted) {
      return static_cast<std::int64_t>(i);
    }
  }
  PLUSS_CHECK(!quoted,
              "Nexus::pull: unmatched backtick in field token '" + std::string(token) + "'");
  return -1;
}

NexusPullField parse_pull_field(std::string_view root,
                                std::string_view token,
                                std::string_view where) {
  PLUSS_CHECK(!token.empty(), std::string(where) + ": field token cannot be empty");
  const auto dot = find_pull_first_dot(token);
  if (dot < 0) {
    const auto feature = is_pull_backtick_quoted(token)
        ? unquote_pull_token(token, where)
        : std::string(token);
    return NexusPullField{
        .table = std::string(root),
        .feature = std::move(feature),
        .token = std::string(token),
    };
  }
  PLUSS_CHECK(dot > 0 && static_cast<std::size_t>(dot + 1) < token.size(),
              std::string(where) + ": malformed qualified field '" + std::string(token) + "'");
  const auto table = std::string(token.substr(0, static_cast<std::size_t>(dot)));
  const auto rest = token.substr(static_cast<std::size_t>(dot + 1));
  PLUSS_CHECK(!table.empty() && !rest.empty(),
              std::string(where) + ": malformed qualified field '" + std::string(token) + "'");

  if (is_pull_backtick_quoted(rest)) {
    return NexusPullField{
        .table = table,
        .feature = unquote_pull_token(rest, where),
        .token = std::string(token),
    };
  }

  const auto parts = split_pull_dots(rest);
  PLUSS_CHECK(!parts.empty(), std::string(where) + ": malformed qualified field '" + std::string(token) + "'");
  if (parts.size() == 1) {
    return NexusPullField{
        .table = table,
        .feature = normalize_pull_feature_token(parts.front(), where),
        .token = std::string(token),
    };
  }

  if (const auto aggregate = parse_pull_aggregate(parts.back()); aggregate.has_value()) {
    const auto feature = normalize_pull_feature_token(
        join_pull_parts(parts, 0, parts.size() - 1), where);
    return NexusPullField{
        .table = table,
        .feature = feature,
        .token = std::string(token),
        .aggregate = aggregate->token,
    };
  }

  return NexusPullField{
      .table = table,
      .feature = normalize_pull_feature_token(join_pull_parts(parts, 0, parts.size()), where),
      .token = std::string(token),
  };
}

std::vector<NexusPullField> parse_pull_fields(std::string_view root,
                                              pluss::msharp::RangeView::Of<std::string_view> fields,
                                              std::string_view where) {
  PLUSS_CHECK(fields.size() > 0, std::string(where) + ": at least one field is required");
  std::vector<NexusPullField> out;
  out.reserve(fields.size());
  std::unordered_set<std::string> seen;
  seen.reserve(fields.size() * 2 + 1);
  for (const auto field : fields) {
    auto parsed = parse_pull_field(root, field, where);
    const auto key = pull_request_key(parsed);
    PLUSS_CHECK(seen.insert(key).second,
                std::string(where) + ": duplicate requested field '" + key + "'");
    if (parsed.aggregate.has_value()) {
      PLUSS_CHECK(parsed.table != root,
                  std::string(where) + ": aggregate field '" + parsed.token +
                      "' must target a related table, not the root table");
      (void)parse_pull_aggregate(*parsed.aggregate);
    }
    out.push_back(std::move(parsed));
  }
  return out;
}

RelationshipCardinality oriented_step_cardinality(const detail::NexusImpl& src,
                                                  const JoinPathStep& step,
                                                  std::string_view where) {
  const auto rel_it = src.rel_name_to_pos_.find(step.relationship_name);
  PLUSS_CHECK(rel_it != src.rel_name_to_pos_.end(),
              std::string(where) + ": unknown relationship '" + step.relationship_name + "'");
  const auto& rel = src.relationships_[static_cast<std::size_t>(rel_it->second)];
  return step.reversed
      ? reverse_relationship_cardinality(rel.cardinality)
      : rel.cardinality;
}

void require_pull_root_preserving_path(const detail::NexusImpl& src,
                                       const JoinPath& path_value,
                                       std::string_view token,
                                       std::string_view where) {
  for (const auto& step : path_value.steps) {
    const auto cardinality = oriented_step_cardinality(src, step, where);
    PLUSS_CHECK(
        cardinality == RelationshipCardinality::ManyToOne ||
            cardinality == RelationshipCardinality::OneToOne,
        std::string(where) + ": requested field '" + std::string(token) +
            "' requires expanding relationship '" + step.relationship_name + "' from '" +
            step.from_table + "' to '" + step.to_table +
            "'. pull preserves root grain and cannot traverse one-to-many or many-to-many paths");
  }
}

JoinPlan merge_pull_plan(const detail::NexusImpl& src,
                         std::string_view root,
                         const std::vector<std::pair<std::string, JoinPath>>& paths,
                         std::string_view where) {
  JoinPlan out;
  out.root_table = std::string(root);
  out.result_name = std::string(root) + "_pull";

  std::unordered_set<std::string> seen_steps;
  seen_steps.reserve(paths.size() * 4 + 1);

  for (const auto& [token, path_value] : paths) {
    require_pull_root_preserving_path(src, path_value, token, where);
    for (const auto& step : path_value.steps) {
      const auto signature = step.relationship_name + std::string("\n") +
          step.from_table + std::string("\n") + step.to_table;
      if (!seen_steps.insert(signature).second) {
        continue;
      }
      JoinSpec spec;
      spec.how = JoinHow::Left;
      spec.relationship_name = step.relationship_name;
      spec.right_suffix = std::string("_") + step.to_table;
      out.steps.push_back(JoinPlanStep{
          .left_table = step.from_table,
          .right_table = step.to_table,
          .spec = std::move(spec),
      });
    }
  }
  return out;
}

std::unordered_map<std::string, std::string> simulate_pull_projection_map(
    const detail::NexusImpl& src,
    std::string_view root,
    const JoinPlan& plan,
    std::string_view where) {
  std::unordered_map<std::string, std::string> out;
  std::vector<std::string> used_names = frame_ref(src, root, where).features();
  out.reserve(used_names.size() * 2 + 8);
  for (const auto& name : used_names) {
    out.emplace(pull_field_key(root, name), name);
  }

  std::unordered_set<std::string> used(used_names.begin(), used_names.end());
  for (const auto& step : plan.steps) {
    const auto right_frame = frame_ref(src, step.right_table, where);
    const auto resolved = resolve_nexus_join_spec(
        src,
        frame_ref(src, step.left_table, where),
        step.left_table,
        right_frame,
        step.right_table,
        step.spec,
        where);

    std::unordered_set<std::string> skip_right_keys;
    for (std::size_t i = 0; i < resolved.left_keys.size(); ++i) {
      if (resolved.left_keys[i] == resolved.right_keys[i]) {
        skip_right_keys.insert(resolved.right_keys[i]);
        out.emplace(
            pull_field_key(step.right_table, resolved.right_keys[i]),
            resolved.left_keys[i]);
      }
    }

    for (const auto& base : right_frame.features()) {
      if (skip_right_keys.find(base) != skip_right_keys.end()) {
        continue;
      }
      std::string out_name = base;
      if (used.find(out_name) != used.end()) {
        out_name = base + resolved.right_suffix;
        std::size_t k = 2;
        while (used.find(out_name) != used.end()) {
          out_name = fmt::format("{}{}_{}", base, resolved.right_suffix, k++);
        }
      }
      used.insert(out_name);
      out.emplace(pull_field_key(step.right_table, base), out_name);
    }
  }
  return out;
}

std::vector<std::pair<std::string, Thread>> materialize_pull_columns(
    const DataFrame& materialized,
    std::string_view root,
    const std::vector<NexusPullField>& fields,
    const std::unordered_map<std::string, std::string>& projected,
    std::string_view where) {
  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(fields.size());
  std::unordered_set<std::string> aliases;
  aliases.reserve(fields.size() * 2 + 1);

  for (const auto& field : fields) {
    PLUSS_CHECK(!field.aggregate.has_value(),
                std::string(where) + ": aggregate fields should be materialized separately");
    const auto projected_it = projected.find(pull_field_key(field.table, field.feature));
    PLUSS_CHECK(projected_it != projected.end(),
                std::string(where) + ": field mapping missing for '" + field.token + "'");
    const auto alias = pull_output_alias(root, field);
    PLUSS_CHECK(aliases.insert(alias).second,
                std::string(where) + ": output alias collision for '" + alias + "'");
    PLUSS_CHECK(materialized.get(projected_it->second).has_value(),
                std::string(where) + ": materialized field missing '" + projected_it->second + "'");
    out_cols.emplace_back(alias, materialized.features(projected_it->second));
  }
  return out_cols;
}

DataFrame::GroupOp pull_group_op(const NexusPullAggregateSpec& aggregate,
                                 std::string_view where) {
  using GroupOp = DataFrame::GroupOp;
  switch (aggregate.op) {
    case NexusPullAggregateOp::Count:
      return GroupOp::Count;
    case NexusPullAggregateOp::Ndistinct:
      return GroupOp::Ndistinct;
    case NexusPullAggregateOp::Sum:
      return GroupOp::Sum;
    case NexusPullAggregateOp::Mean:
      return GroupOp::Mean;
    case NexusPullAggregateOp::Min:
      return GroupOp::Min;
    case NexusPullAggregateOp::Max:
      return GroupOp::Max;
  }
  PLUSS_FAIL(std::string(where) + ": unsupported pull aggregate");
}

std::int64_t pull_window_seconds(const NexusPullAggregateSpec& spec, std::string_view where) {
  PLUSS_CHECK(spec.window_value.has_value() && spec.window_unit.has_value(),
              std::string(where) + ": missing pull window specification");
  const auto value = *spec.window_value;
  const auto unit = lower_ascii(*spec.window_unit);
  if (unit == "s" || unit == "sec" || unit == "secs" || unit == "second" || unit == "seconds") {
    return value;
  }
  if (unit == "m" || unit == "min" || unit == "mins" || unit == "minute" || unit == "minutes") {
    return value * 60LL;
  }
  if (unit == "h" || unit == "hr" || unit == "hrs" || unit == "hour" || unit == "hours") {
    return value * 60LL * 60LL;
  }
  if (unit == "d" || unit == "day" || unit == "days") {
    return value * 24LL * 60LL * 60LL;
  }
  if (unit == "w" || unit == "wk" || unit == "wks" || unit == "week" || unit == "weeks") {
    return value * 7LL * 24LL * 60LL * 60LL;
  }
  PLUSS_FAIL(std::string(where) + ": unsupported pull window unit '" + *spec.window_unit + "'");
}

std::vector<std::string> pull_root_projection_names(const std::vector<std::string>& root_keys,
                                                    std::string_view output_name) {
  auto out = root_keys;
  out.push_back(std::string(output_name));
  return out;
}

Thread materialize_grouped_pull(const DataFrame& root_frame,
                                const std::vector<std::string>& root_keys,
                                const DataFrame& grouped,
                                std::string_view output_name,
                                const NexusPullAggregateSpec& aggregate,
                                std::optional<std::string_view> root_time_column) {
  const auto root_key_views = key_views(root_keys);
  auto joined = root_frame.unify(pluss::msharp::RangeView::Of<std::string_view>(root_key_views),
                                 grouped,
                                 pluss::msharp::RangeView::Of<std::string_view>(root_key_views),
                                 "keep_left",
                                 "_agg",
                                 "many_to_one");
  if (aggregate.op == NexusPullAggregateOp::Count ||
      aggregate.op == NexusPullAggregateOp::Ndistinct) {
    joined = joined.fillnull(0.0, {output_name});
    if (aggregate.window_value.has_value() &&
        aggregate.anchor == NexusPullAggregateSpec::Anchor::Root &&
        root_time_column.has_value()) {
      joined.set(output_name) =
          joined.features(output_name)
              .put(root_frame.features(*root_time_column).isnull(), pluss::null);
    }
  }
  return joined.features(output_name);
}

Thread aggregate_pull_field(const DataFrame& root_frame,
                            const std::vector<std::string>& root_keys,
                            const DataFrame& expanded,
                            std::string_view value_column,
                            std::optional<std::string_view> child_time_column,
                            std::optional<std::string_view> root_time_column,
                            const NexusPullAggregateSpec& aggregate,
                            std::string_view where) {
  const auto output_name = std::string("__pull_agg__");
  const auto root_key_views = key_views(root_keys);
  std::vector<DataFrame::GroupSpec> specs{
      DataFrame::GroupSpec{
          output_name,
          std::string(value_column),
          pull_group_op(aggregate, where),
      },
  };

  DataFrame working = expanded;
  if (aggregate.window_value.has_value()) {
    PLUSS_CHECK(child_time_column.has_value(),
                std::string(where) + ": window aggregate requires a declared time field");
    PLUSS_CHECK(aggregate.op != NexusPullAggregateOp::Ndistinct,
                std::string(where) + ": ndistinct windows are not supported yet");
    const auto duration = std::to_string(*aggregate.window_value) + *aggregate.window_unit;
    TsOptions ts_options{
        .axis = expanded.features(std::string(*child_time_column)),
        .group = TsGroup(expanded.features(pluss::msharp::RangeView::Of<std::string_view>(root_key_views))),
    };
    if (aggregate.anchor == NexusPullAggregateSpec::Anchor::Root) {
      PLUSS_CHECK(root_time_column.has_value(),
                  std::string(where) + ": root-anchored window aggregate requires a declared root time field");
      ts_options.anchor = expanded.features(std::string(*root_time_column));
    }
    const auto windowed =
        expanded.features({std::string(value_column)}).ts(ts_options).window(duration, WindowClosure::Both);

    DataFrame grouped;
    switch (aggregate.op) {
      case NexusPullAggregateOp::Count:
        grouped = windowed.count(value_column);
        break;
      case NexusPullAggregateOp::Sum:
        grouped = windowed.sum(value_column);
        break;
      case NexusPullAggregateOp::Mean:
        grouped = windowed.mean(value_column);
        break;
      case NexusPullAggregateOp::Min:
        grouped = windowed.min(value_column);
        break;
      case NexusPullAggregateOp::Max:
        grouped = windowed.max(value_column);
        break;
      case NexusPullAggregateOp::Ndistinct:
        PLUSS_FAIL(std::string(where) + ": ndistinct windows are not supported yet");
    }

    std::string grouped_value_feature;
    for (const auto& name : grouped.features()) {
      if (std::find(root_keys.begin(), root_keys.end(), name) == root_keys.end()) {
        grouped_value_feature = name;
        break;
      }
    }
    PLUSS_CHECK(!grouped_value_feature.empty(),
                std::string(where) + ": grouped window aggregate did not emit a value feature");
    if (grouped_value_feature != output_name) {
      grouped = grouped.rename(grouped_value_feature, output_name);
    }
    return materialize_grouped_pull(
        root_frame, root_keys, grouped, output_name, aggregate, root_time_column);
  }

  const auto grouped = working.group({.dropnull = true})
                           .by(pluss::msharp::RangeView::Of<std::string_view>(root_key_views))
                           .agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
  return materialize_grouped_pull(
      root_frame, root_keys, grouped, output_name, aggregate, root_time_column);
}

} // namespace

DataFrame Nexus::join(std::string_view left, std::string_view right, const JoinSpec& spec) const {
  const auto& src = impl_ref_();
  require_known_table(src, left, "Nexus::join");
  require_known_table(src, right, "Nexus::join");

  const auto& left_frame = frame_ref(src, left, "Nexus::join");
  const auto& right_frame = frame_ref(src, right, "Nexus::join");
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

DataFrame Nexus::pull(std::string_view root,
                      std::initializer_list<std::string_view> fields) const {
  return pull(root, pluss::msharp::RangeView::Of<std::string_view>(fields));
}

DataFrame Nexus::pull(std::string_view root,
                      pluss::msharp::RangeView::Of<std::string_view> fields) const {
  const auto& src = impl_ref_();
  require_known_table(src, root, "Nexus::pull");
  const auto parsed = parse_pull_fields(root, fields, "Nexus::pull");
  std::vector<NexusPullField> plain_fields;
  std::vector<NexusPullField> aggregate_fields;
  plain_fields.reserve(parsed.size());
  aggregate_fields.reserve(parsed.size());
  for (const auto& field : parsed) {
    require_known_table(src, field.table, "Nexus::pull");
    PLUSS_CHECK(frame_ref(src, field.table, "Nexus::pull").get(field.feature).has_value(),
                "Nexus::pull: unknown feature '" + field.feature + "' on table '" + field.table + "'");
    if (field.aggregate.has_value()) {
      aggregate_fields.push_back(field);
      continue;
    }
    plain_fields.push_back(field);
  }

  std::unordered_map<std::string, Thread> pulled_columns;
  pulled_columns.reserve(parsed.size() * 2 + 1);

  if (!plain_fields.empty()) {
    std::vector<std::pair<std::string, JoinPath>> paths;
    paths.reserve(plain_fields.size());
    for (const auto& field : plain_fields) {
      if (field.table != root) {
        paths.emplace_back(field.token, path(root, field.table));
      }
    }
    const auto plan_value = merge_pull_plan(src, root, paths, "Nexus::pull");
    const auto projected = simulate_pull_projection_map(src, root, plan_value, "Nexus::pull");
    const auto materialized = materialize(plan_value).title(std::string(root) + "_pull");
    for (auto& [name, column] :
         materialize_pull_columns(materialized, root, plain_fields, projected, "Nexus::pull")) {
      pulled_columns.emplace(std::move(name), std::move(column));
    }
  }

  if (!aggregate_fields.empty()) {
    const auto& root_frame = frame_ref(src, root, "Nexus::pull");
    const auto root_keys = default_table_keys(src, root, "Nexus::pull");
    const auto& root_profile =
        cached_key_audit_profile(src, root_frame, root, root_keys, "Nexus::pull");
    PLUSS_CHECK(key_profile_valid(root_profile),
                "Nexus::pull: root table '" + std::string(root) +
                    "' must have a complete, unique declared primary key before aggregate pulls");
    std::unordered_map<std::string, NexusPullExpandedFrame> cache;
    cache.reserve(aggregate_fields.size() * 2 + 1);

    for (const auto& field : aggregate_fields) {
      const auto aggregate = parse_pull_aggregate(*field.aggregate);
      PLUSS_CHECK(aggregate.has_value(),
                  "Nexus::pull: unsupported aggregate token '" + *field.aggregate + "'");

      const auto cache_key = std::string(root) + "\n" + field.table;
      if (cache.find(cache_key) == cache.end()) {
        auto plan_value = plan(root, field.table, JoinHow::Left);
        plan_value.result_name = std::string(root) + "_" + field.table;
        cache.emplace(cache_key,
                      NexusPullExpandedFrame{
                          .frame = materialize(plan_value),
                          .projected = simulate_pull_projection_map(src, root, plan_value, "Nexus::pull"),
                      });
      }

      const auto& expanded = cache.at(cache_key);
      const auto projected_it = expanded.projected.find(pull_field_key(field.table, field.feature));
      PLUSS_CHECK(projected_it != expanded.projected.end(),
                  "Nexus::pull: field mapping missing for aggregate '" + field.token + "'");

      std::optional<std::string_view> child_time_column = std::nullopt;
      std::optional<std::string_view> root_time_column = std::nullopt;
      if (aggregate->window_value.has_value()) {
        const auto meta_it = src.table_meta_.find(field.table);
        PLUSS_CHECK(meta_it != src.table_meta_.end() && meta_it->second.time_field.has_value(),
                    "Nexus::pull: window aggregate '" + field.token +
                        "' requires a declared time_field on table '" + field.table + "'");
        const auto time_key = pull_field_key(field.table, *meta_it->second.time_field);
        const auto time_it = expanded.projected.find(time_key);
        PLUSS_CHECK(time_it != expanded.projected.end(),
                    "Nexus::pull: time field mapping missing for '" + field.table + "'");
        child_time_column = time_it->second;
        if (aggregate->anchor == NexusPullAggregateSpec::Anchor::Root) {
          const auto root_meta_it = src.table_meta_.find(std::string(root));
          PLUSS_CHECK(root_meta_it != src.table_meta_.end() &&
                          root_meta_it->second.time_field.has_value(),
                      "Nexus::pull: root-anchored window aggregate '" + field.token +
                          "' requires a declared time_field on root table '" + std::string(root) + "'");
          root_time_column = *root_meta_it->second.time_field;
          PLUSS_CHECK(root_frame.get(*root_time_column).has_value(),
                      std::string("Nexus::pull: root time field '") + std::string(*root_time_column) +
                          "' is missing on root table '" + std::string(root) + "'");
        }
      }

      const auto alias = pull_output_alias(root, field);
      PLUSS_CHECK(pulled_columns.find(alias) == pulled_columns.end(),
                  "Nexus::pull: output alias collision for '" + alias + "'");
      pulled_columns.emplace(
          alias,
          aggregate_pull_field(root_frame,
                               root_keys,
                               expanded.frame,
                               projected_it->second,
                               child_time_column,
                               root_time_column,
                               *aggregate,
                               "Nexus::pull"));
    }
  }

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(parsed.size());
  for (const auto& field : parsed) {
    const auto alias = pull_output_alias(root, field);
    const auto it = pulled_columns.find(alias);
    PLUSS_CHECK(it != pulled_columns.end(),
                "Nexus::pull: internal projection assembly failure for '" + alias + "'");
    out.emplace_back(alias, it->second);
  }
  return DataFrame(std::move(out)).title(std::string(root) + "_pull");
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
  const auto& src = impl_ref_();
  require_known_table(src, from, "Nexus::plan");
  require_known_table(src, to, "Nexus::plan");

  const auto cache_key = nexus_plan_cache_key(from, to, how);
  if (const auto it = src.plan_cache_.find(cache_key); it != src.plan_cache_.end()) {
    return it->second;
  }
  auto computed = plan(path(from, to), how);
  auto [it, _] = src.plan_cache_.emplace(std::move(cache_key), computed);
  return it->second;
}

DataFrame Nexus::materialize(const JoinPlan& plan_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!plan_value.root_table.empty(), "Nexus::materialize: root_table cannot be empty");
  require_known_table(src, plan_value.root_table, "Nexus::materialize");

  auto current = frame_ref(src, plan_value.root_table, "Nexus::materialize");
  if (plan_value.steps.empty()) {
    const auto result_name = plan_value.result_name.empty()
        ? plan_value.root_table
        : plan_value.result_name;
    return current.title(result_name);
  }

  for (const auto& step : plan_value.steps) {
    require_known_table(src, step.right_table, "Nexus::materialize");
    const auto& right_frame = frame_ref(src, step.right_table, "Nexus::materialize");
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
