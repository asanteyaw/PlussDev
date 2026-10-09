namespace {

struct NexusResolvedJoinSpec {
  std::vector<std::string> left_keys{};
  std::vector<std::string> right_keys{};
  std::string how{};
  std::string validate{};
  std::string right_suffix{};
  std::string result_name{};
};

// The pull grammar is shared with Locus (relational/Pull.h).
using NexusPullField = relational::PullField;
using NexusPullAggregateOp = relational::PullAggregateOp;
using NexusPullAggregateSpec = relational::PullAggregate;
using relational::parse_pull_aggregate;
using relational::parse_pull_field;
using relational::parse_pull_fields;
using relational::parse_pull_spec;
using relational::pull_field_key;
using relational::pull_output_alias;
using relational::pull_request_key;
using relational::pull_window_seconds;

struct NexusPullExpandedFrame {
  pluss::table::DataFrame frame{};
  std::unordered_map<std::string, std::string> projected{};
};

// Link vocabulary, lookup and join-spec resolution are shared with Locus
// (relational/Links.h); these keep the Nexus call sites unchanged.
RelationshipCardinality reverse_relationship_cardinality(RelationshipCardinality cardinality) {
  return relational::reverse_cardinality(cardinality);
}

std::string join_how_string(JoinHow how) { return relational::join_how_unify_token(how); }

std::string relationship_cardinality_validate_string(RelationshipCardinality cardinality) {
  return relational::cardinality_validate_token(cardinality);
}

using relational::join_how_token;
using relational::oriented_relationship_keys;
using relational::stable_join_result_name;
using relational::stable_plan_result_name;
using relational::validate_join_key_pairs;

std::pair<const Relationship&, bool> require_oriented_relationship(const detail::NexusImpl& src,
                                                                   std::string_view left,
                                                                   std::string_view right,
                                                                   std::string_view relationship_name,
                                                                   std::string_view where) {
  return relational::require_oriented_relationship(
      src.relationships_, left, right, relationship_name, where, &src.rel_name_to_pos_);
}

std::string require_direct_relationship_name(const detail::NexusImpl& src,
                                             std::string_view left,
                                             std::string_view right,
                                             std::string_view where) {
  return relational::require_direct_relationship_name(src.relationships_, left, right, where);
}

NexusResolvedJoinSpec resolve_nexus_join_spec(const detail::NexusImpl& src,
                                              const pluss::table::DataFrame& left_frame,
                                              std::string_view left,
                                              const pluss::table::DataFrame& right_frame,
                                              std::string_view right,
                                              const JoinSpec& spec,
                                              std::string_view where) {
  const auto resolved =
      relational::resolve_join(src.relationships_, left, right, spec, where, false, &src.rel_name_to_pos_);
  NexusResolvedJoinSpec out;
  out.how = join_how_string(resolved.how);
  out.right_suffix = resolved.right_suffix;
  out.result_name = resolved.result_name;
  if (spec.how == JoinHow::Cross) {
    out.validate.clear();
    return out;
  }
  out.left_keys = resolved.left_keys;
  out.right_keys = resolved.right_keys;
  require_frame_features(left_frame, out.left_keys, where);
  require_frame_features(right_frame, out.right_keys, where);
  if (resolved.validate.has_value()) {
    out.validate = relationship_cardinality_validate_string(*resolved.validate);
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

std::vector<std::pair<std::string, pluss::table::Slate>> materialize_pull_columns(
    const pluss::table::DataFrame& materialized,
    std::string_view root,
    const std::vector<NexusPullField>& fields,
    const std::unordered_map<std::string, std::string>& projected,
    std::string_view where) {
  std::vector<std::pair<std::string, pluss::table::Slate>> out_cols;
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

pluss::table::DataFrame::GroupOp pull_group_op(const NexusPullAggregateSpec& aggregate,
                                 std::string_view where) {
  using GroupOp = pluss::table::DataFrame::GroupOp;
  switch (aggregate.op) {
    case NexusPullAggregateOp::Count:
      return GroupOp::Count;
    case NexusPullAggregateOp::Ndistinct:
      return GroupOp::Ndistinct;
    case NexusPullAggregateOp::Sum:
      return GroupOp::Sum;
    case NexusPullAggregateOp::Mean:
      return GroupOp::Mean;
    case NexusPullAggregateOp::Median:
      return GroupOp::Median;
    case NexusPullAggregateOp::Min:
      return GroupOp::Min;
    case NexusPullAggregateOp::Max:
      return GroupOp::Max;
    case NexusPullAggregateOp::Std:
      return GroupOp::Std;
    case NexusPullAggregateOp::Var:
      return GroupOp::Var;
  }
  PLUSS_FAIL(std::string(where) + ": unsupported pull aggregate");
}

std::vector<std::string> pull_root_projection_names(const std::vector<std::string>& root_keys,
                                                    std::string_view output_name) {
  auto out = root_keys;
  out.push_back(std::string(output_name));
  return out;
}

pluss::table::Slate materialize_grouped_pull(const pluss::table::DataFrame& root_frame,
                                const std::vector<std::string>& root_keys,
                                const pluss::table::DataFrame& grouped,
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
              .put(root_frame.features(*root_time_column).isnull(), pluss::series::null);
    }
  }
  return joined.features(output_name);
}

pluss::table::Slate aggregate_pull_field(const pluss::table::DataFrame& root_frame,
                            const std::vector<std::string>& root_keys,
                            const pluss::table::DataFrame& expanded,
                            std::string_view value_column,
                            std::optional<std::string_view> child_time_column,
                            std::optional<std::string_view> root_time_column,
                            const NexusPullAggregateSpec& aggregate,
                            std::string_view where) {
  const auto output_name = std::string("__pull_agg__");
  const auto root_key_views = key_views(root_keys);
  std::vector<pluss::table::DataFrame::GroupSpec> specs{
      pluss::table::DataFrame::GroupSpec{
          output_name,
          std::string(value_column),
          pull_group_op(aggregate, where),
      },
  };

  pluss::table::DataFrame working = expanded;
  if (aggregate.window_value.has_value()) {
    PLUSS_CHECK(child_time_column.has_value(),
                std::string(where) + ": window aggregate requires a declared time field");
    PLUSS_CHECK(aggregate.op != NexusPullAggregateOp::Ndistinct,
                std::string(where) + ": ndistinct windows are not supported yet");
    const auto duration = std::to_string(*aggregate.window_value) + *aggregate.window_unit;
    pluss::table::TsOptions ts_options{
        .axis = expanded.features(std::string(*child_time_column)),
        .group = pluss::table::TsGroup(
            expanded.features(pluss::msharp::RangeView::Of<std::string_view>(root_key_views))),
    };
    if (aggregate.anchor == NexusPullAggregateSpec::Anchor::Root) {
      PLUSS_CHECK(root_time_column.has_value(),
                  std::string(where) + ": root-anchored window aggregate requires a declared root time field");
      ts_options.anchor = expanded.features(std::string(*root_time_column));
    }
    const auto windowed =
        expanded.features({std::string(value_column)})
            .ts(ts_options)
            .window(duration, pluss::table::WindowClosure::Both);

    pluss::table::DataFrame grouped;
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
      case NexusPullAggregateOp::Median:
        PLUSS_FAIL(std::string(where) + ": median windows are not supported yet");
      case NexusPullAggregateOp::Min:
        grouped = windowed.min(value_column);
        break;
      case NexusPullAggregateOp::Max:
        grouped = windowed.max(value_column);
        break;
      case NexusPullAggregateOp::Std:
        PLUSS_FAIL(std::string(where) + ": std windows are not supported yet");
      case NexusPullAggregateOp::Var:
        PLUSS_FAIL(std::string(where) + ": var windows are not supported yet");
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
                           .agg(pluss::msharp::RangeView::Of<pluss::table::DataFrame::GroupSpec>(specs));
  return materialize_grouped_pull(
      root_frame, root_keys, grouped, output_name, aggregate, root_time_column);
}

} // namespace

pluss::table::DataFrame Nexus::join(std::string_view left, std::string_view right, const JoinSpec& spec) const {
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

pluss::table::DataFrame nexus_pull_impl(const detail::NexusImpl& src,
                                        const Nexus& nexus,
                                        std::string_view root,
                                        const std::vector<NexusPullField>& parsed,
                                        std::string_view where) {
  std::vector<NexusPullField> plain_fields;
  std::vector<NexusPullField> aggregate_fields;
  plain_fields.reserve(parsed.size());
  aggregate_fields.reserve(parsed.size());
  for (const auto& field : parsed) {
    require_known_table(src, field.table, where);
    PLUSS_CHECK(frame_ref(src, field.table, where).get(field.feature).has_value(),
                std::string(where) + ": unknown feature '" + field.feature +
                    "' on table '" + field.table + "'");
    if (field.aggregate.has_value()) {
      aggregate_fields.push_back(field);
      continue;
    }
    plain_fields.push_back(field);
  }

  std::unordered_map<std::string, pluss::table::Slate> pulled_columns;
  pulled_columns.reserve(parsed.size() * 2 + 1);

  if (!plain_fields.empty()) {
    std::vector<std::pair<std::string, JoinPath>> paths;
    paths.reserve(plain_fields.size());
    for (const auto& field : plain_fields) {
      if (field.table != root) {
        paths.emplace_back(field.token, nexus.path(root, field.table));
      }
    }
    const auto plan_value = merge_pull_plan(src, root, paths, where);
    const auto projected = simulate_pull_projection_map(src, root, plan_value, where);
    const auto materialized = nexus.materialize(plan_value).title(std::string(root) + "_pull");
    for (auto& [name, column] :
         materialize_pull_columns(materialized, root, plain_fields, projected, where)) {
      pulled_columns.emplace(std::move(name), std::move(column));
    }
  }

  if (!aggregate_fields.empty()) {
    const auto& root_frame = frame_ref(src, root, where);
    const auto root_keys = default_table_keys(src, root, where);
    const auto& root_profile =
        cached_key_audit_profile(src, root_frame, root, root_keys, where);
    PLUSS_CHECK(key_profile_valid(root_profile),
                std::string(where) + ": root table '" + std::string(root) +
                    "' must have a complete, unique declared primary key before aggregate pulls");
    std::unordered_map<std::string, NexusPullExpandedFrame> cache;
    cache.reserve(aggregate_fields.size() * 2 + 1);

    for (const auto& field : aggregate_fields) {
      const auto aggregate = parse_pull_aggregate(*field.aggregate);
      PLUSS_CHECK(aggregate.has_value(),
                  std::string(where) + ": unsupported aggregate token '" + *field.aggregate + "'");

      const auto cache_key = std::string(root) + "\n" + field.table;
      if (cache.find(cache_key) == cache.end()) {
        auto plan_value = nexus.plan(root, field.table, JoinHow::Left);
        plan_value.result_name = std::string(root) + "_" + field.table;
        cache.emplace(cache_key,
                      NexusPullExpandedFrame{
                          .frame = nexus.materialize(plan_value),
                          .projected = simulate_pull_projection_map(src, root, plan_value, where),
                      });
      }

      const auto& expanded = cache.at(cache_key);
      const auto projected_it = expanded.projected.find(pull_field_key(field.table, field.feature));
      PLUSS_CHECK(projected_it != expanded.projected.end(),
                  std::string(where) + ": field mapping missing for aggregate '" + field.token + "'");

      std::optional<std::string_view> child_time_column = std::nullopt;
      std::optional<std::string_view> root_time_column = std::nullopt;
      if (aggregate->window_value.has_value()) {
        const auto meta_it = src.table_meta_.find(field.table);
        PLUSS_CHECK(meta_it != src.table_meta_.end() && meta_it->second.time_field.has_value(),
                    std::string(where) + ": window aggregate '" + field.token +
                        "' requires a declared time_field on table '" + field.table + "'");
        const auto time_key = pull_field_key(field.table, *meta_it->second.time_field);
        const auto time_it = expanded.projected.find(time_key);
        PLUSS_CHECK(time_it != expanded.projected.end(),
                    std::string(where) + ": time field mapping missing for '" + field.table + "'");
        child_time_column = time_it->second;
        if (aggregate->anchor == NexusPullAggregateSpec::Anchor::Root) {
          const auto root_meta_it = src.table_meta_.find(std::string(root));
          PLUSS_CHECK(root_meta_it != src.table_meta_.end() &&
                          root_meta_it->second.time_field.has_value(),
                      std::string(where) + ": root-anchored window aggregate '" + field.token +
                          "' requires a declared time_field on root table '" + std::string(root) + "'");
          root_time_column = *root_meta_it->second.time_field;
          PLUSS_CHECK(root_frame.get(*root_time_column).has_value(),
                      std::string(where) + ": root time field '" + std::string(*root_time_column) +
                          "' is missing on root table '" + std::string(root) + "'");
        }
      }

      const auto alias = pull_output_alias(root, field);
      PLUSS_CHECK(pulled_columns.find(alias) == pulled_columns.end(),
                  std::string(where) + ": output alias collision for '" + alias + "'");
      pulled_columns.emplace(
          alias,
          aggregate_pull_field(root_frame,
                               root_keys,
                               expanded.frame,
                               projected_it->second,
                               child_time_column,
                               root_time_column,
                               *aggregate,
                               where));
    }
  }

  std::vector<std::pair<std::string, pluss::table::Slate>> out;
  out.reserve(parsed.size());
  for (const auto& field : parsed) {
    const auto alias = pull_output_alias(root, field);
    const auto it = pulled_columns.find(alias);
    PLUSS_CHECK(it != pulled_columns.end(),
                std::string(where) + ": internal projection assembly failure for '" + alias + "'");
    out.emplace_back(alias, it->second);
  }
  return pluss::table::DataFrame(std::move(out)).title(std::string(root) + "_pull");
}

NexusPull Nexus::pull(std::string_view root) const {
  const auto& src = impl_ref_();
  require_known_table(src, root, "Nexus::pull");
  return NexusPull(*this, std::string(root));
}

pluss::table::DataFrame Nexus::pull(std::string_view root,
                      std::initializer_list<std::string_view> fields) const {
  return pull(root, pluss::msharp::RangeView::Of<std::string_view>(fields));
}

pluss::table::DataFrame Nexus::pull(std::string_view root,
                      pluss::msharp::RangeView::Of<std::string_view> fields) const {
  const auto& src = impl_ref_();
  require_known_table(src, root, "Nexus::pull");
  return nexus_pull_impl(src, *this, root, parse_pull_fields(root, fields, "Nexus::pull"), "Nexus::pull");
}

pluss::table::DataFrame Nexus::pull(std::string_view root,
                      std::initializer_list<PullSpec> fields) const {
  return pull(root, std::vector<PullSpec>(fields));
}

pluss::table::DataFrame Nexus::pull(std::string_view root,
                      const std::vector<PullSpec>& fields) const {
  const auto& src = impl_ref_();
  require_known_table(src, root, "Nexus::pull");
  return nexus_pull_impl(src, *this, root, parse_pull_fields(root, fields, "Nexus::pull"), "Nexus::pull");
}

JoinPath Nexus::path(std::string_view from, std::string_view to) const {
  const auto& src = impl_ref_();
  require_known_table(src, from, "Nexus::path");
  require_known_table(src, to, "Nexus::path");
  return src.link_cache_.path(src.keys_, src.relationships_, from, to, "Nexus::path");
}

JoinPlan Nexus::plan(const JoinPath& path_value, JoinHow how) const {
  return relational::plan_from_path(path_value, how);
}

JoinPlan Nexus::plan(std::string_view from, std::string_view to, JoinHow how) const {
  const auto& src = impl_ref_();
  require_known_table(src, from, "Nexus::plan");
  require_known_table(src, to, "Nexus::plan");
  return src.link_cache_.plan(src.keys_, src.relationships_, from, to, how, "Nexus::path");
}

pluss::table::DataFrame Nexus::materialize(const JoinPlan& plan_value) const {
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
