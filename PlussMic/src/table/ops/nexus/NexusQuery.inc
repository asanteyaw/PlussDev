namespace {

std::vector<std::string> normalize_query_names(std::initializer_list<std::string_view> names,
                                               std::string_view where) {
  return normalize_requested_keys(names, where);
}

std::vector<std::string> normalize_query_names(pluss::msharp::RangeView::Of<std::string_view> names,
                                               std::string_view where) {
  return normalize_requested_keys(names, where);
}

std::vector<std::string_view> query_name_views(const std::vector<std::string>& names) {
  std::vector<std::string_view> out;
  out.reserve(names.size());
  for (const auto& name : names) {
    out.push_back(name);
  }
  return out;
}

JoinPlan append_join_plan(const JoinPlan& base,
                          std::string_view root_table,
                          const std::vector<JoinPlanStep>& extra_steps) {
  JoinPlan out = base;
  if (out.root_table.empty()) {
    out.root_table = std::string(root_table);
  }
  out.steps.reserve(out.steps.size() + extra_steps.size());
  for (const auto& step : extra_steps) {
    out.steps.push_back(step);
  }
  out.result_name = stable_plan_result_name(out.root_table, out.steps);
  return out;
}

} // namespace

NexusQuery::NexusQuery(Nexus nexus, std::string root_table)
    : nexus_(std::move(nexus)),
      plan_(JoinPlan{
          .root_table = root_table,
          .result_name = root_table,
          .steps = {},
      }),
      anchor_table_(plan_.root_table) {}

NexusQuery::NexusQuery(Nexus nexus,
                       JoinPlan plan,
                       std::string anchor_table,
                       std::optional<std::vector<std::string>> picked,
                       std::optional<std::vector<std::string>> dropped,
                       std::optional<std::string> title)
    : nexus_(std::move(nexus)),
      plan_(std::move(plan)),
      anchor_table_(std::move(anchor_table)),
      picked_(std::move(picked)),
      dropped_(std::move(dropped)),
      title_(std::move(title)) {}

NexusQuery Nexus::from(std::string_view table) const {
  const auto& src = impl_ref_();
  require_known_table(src, table, "Nexus::from");
  return NexusQuery(*this, std::string(table));
}

NexusQuery NexusQuery::join(std::string_view right) const {
  JoinSpec spec;
  return join(right, spec);
}

NexusQuery NexusQuery::join(std::string_view right, const JoinSpec& spec) const {
  const auto& src = nexus_.impl_ref_();
  require_known_table(src, anchor_table_, "NexusQuery::join");
  require_known_table(src, right, "NexusQuery::join");

  auto resolved_spec = spec;
  if (resolved_spec.how != JoinHow::Cross &&
      resolved_spec.keys.empty() &&
      !resolved_spec.relationship_name.has_value()) {
    resolved_spec.relationship_name =
        require_direct_relationship_name(src, anchor_table_, right, "NexusQuery::join");
  }
  if (resolved_spec.right_suffix.empty()) {
    resolved_spec.right_suffix = std::string("_") + std::string(right);
  }

  JoinPlanStep step{
      .left_table = anchor_table_,
      .right_table = std::string(right),
      .spec = std::move(resolved_spec),
  };
  auto next_plan = append_join_plan(plan_, plan_.root_table, {step});
  return NexusQuery(nexus_, std::move(next_plan), std::string(right), picked_, dropped_, title_);
}

NexusQuery NexusQuery::via(std::string_view to, JoinHow how) const {
  const auto subplan = nexus_.plan(anchor_table_, to, how);
  auto next_plan = append_join_plan(plan_, plan_.root_table, subplan.steps);
  return NexusQuery(nexus_, std::move(next_plan), std::string(to), picked_, dropped_, title_);
}

NexusQuery NexusQuery::pick(std::initializer_list<std::string_view> features) const {
  return pick(pluss::msharp::RangeView::Of<std::string_view>(features));
}

NexusQuery NexusQuery::pick(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return NexusQuery(
      nexus_,
      plan_,
      anchor_table_,
      normalize_query_names(features, "NexusQuery::pick"),
      dropped_,
      title_);
}

NexusQuery NexusQuery::drop(std::initializer_list<std::string_view> features) const {
  return drop(pluss::msharp::RangeView::Of<std::string_view>(features));
}

NexusQuery NexusQuery::drop(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return NexusQuery(
      nexus_,
      plan_,
      anchor_table_,
      picked_,
      normalize_query_names(features, "NexusQuery::drop"),
      title_);
}

NexusQuery NexusQuery::name(std::string_view value) const {
  const auto owned = std::string(value);
  PLUSS_CHECK(!owned.empty(), "NexusQuery::name: title cannot be empty");
  return NexusQuery(nexus_, plan_, anchor_table_, picked_, dropped_, owned);
}

DataFrame NexusQuery::collect() const {
  auto out = nexus_.materialize(plan_);
  if (picked_.has_value()) {
    const auto picked_views = query_name_views(*picked_);
    out = out.features(pluss::msharp::RangeView::Of<std::string_view>(picked_views));
  }
  if (dropped_.has_value()) {
    const auto dropped_views = query_name_views(*dropped_);
    out = out.drop(pluss::msharp::RangeView::Of<std::string_view>(dropped_views));
  }
  if (title_.has_value()) {
    out = out.title(*title_);
  }
  return out;
}
