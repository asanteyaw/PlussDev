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

NexusTraverse::NexusTraverse(Nexus nexus, std::string root_table)
    : nexus_(std::move(nexus)),
      plan_(JoinPlan{
          .root_table = root_table,
          .result_name = root_table,
          .steps = {},
      }),
      anchor_table_(plan_.root_table) {}

NexusTraverse::NexusTraverse(Nexus nexus,
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

NexusTraverse Nexus::from(std::string_view table) const {
  const auto& src = impl_ref_();
  require_known_table(src, table, "Nexus::from");
  return NexusTraverse(*this, std::string(table));
}

NexusTraverse NexusTraverse::join(std::string_view right) const {
  JoinSpec spec;
  return join(right, spec);
}

NexusTraverse NexusTraverse::join(std::string_view right, const JoinSpec& spec) const {
  const auto& src = nexus_.impl_ref_();
  require_known_table(src, anchor_table_, "NexusTraverse::join");
  require_known_table(src, right, "NexusTraverse::join");

  auto resolved_spec = spec;
  if (resolved_spec.how != JoinHow::Cross &&
      resolved_spec.keys.empty() &&
      !resolved_spec.relationship_name.has_value()) {
    resolved_spec.relationship_name =
        require_direct_relationship_name(src, anchor_table_, right, "NexusTraverse::join");
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
  return NexusTraverse(nexus_, std::move(next_plan), std::string(right), picked_, dropped_, title_);
}

NexusTraverse NexusTraverse::via(std::string_view to, JoinHow how) const {
  const auto subplan = nexus_.plan(anchor_table_, to, how);
  auto next_plan = append_join_plan(plan_, plan_.root_table, subplan.steps);
  return NexusTraverse(nexus_, std::move(next_plan), std::string(to), picked_, dropped_, title_);
}

NexusTraverse NexusTraverse::pick(std::initializer_list<std::string_view> features) const {
  return pick(pluss::msharp::RangeView::Of<std::string_view>(features));
}

NexusTraverse NexusTraverse::pick(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return NexusTraverse(
      nexus_,
      plan_,
      anchor_table_,
      normalize_query_names(features, "NexusTraverse::pick"),
      dropped_,
      title_);
}

NexusTraverse NexusTraverse::drop(std::initializer_list<std::string_view> features) const {
  return drop(pluss::msharp::RangeView::Of<std::string_view>(features));
}

NexusTraverse NexusTraverse::drop(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return NexusTraverse(
      nexus_,
      plan_,
      anchor_table_,
      picked_,
      normalize_query_names(features, "NexusTraverse::drop"),
      title_);
}

NexusTraverse NexusTraverse::name(std::string_view value) const {
  const auto owned = std::string(value);
  PLUSS_CHECK(!owned.empty(), "NexusTraverse::name: title cannot be empty");
  return NexusTraverse(nexus_, plan_, anchor_table_, picked_, dropped_, owned);
}

pluss::table::DataFrame NexusTraverse::collect() const {
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

NexusQuery Nexus::query(pluss::table::DataFrame::SqlOptions options) const {
  return NexusQuery(*this, std::move(options));
}

NexusQuery Nexus::query() const {
  return NexusQuery(*this, {});
}

NexusQuery::NexusQuery(Nexus nexus, pluss::table::DataFrame::SqlOptions options)
    : nexus_(std::move(nexus)),
      options_(std::move(options)) {}

pluss::table::DataFrame NexusQuery::execute(std::string_view statement) const {
  const auto& src = nexus_.impl_ref_();

  auto sql_options = options_;
  sql_options.sources.reserve(sql_options.sources.size() + src.keys_.size());
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    sql_options.sources.push_back(
        pluss::table::DataFrame::SqlOptions::SourceBinding{
            .name = src.keys_[i],
            .frame = &src.frames_[i],
        });
  }

  const auto seed = src.frames_.empty() ? pluss::table::DataFrame{} : src.frames_.front();
  return seed.sql(std::move(sql_options)).execute(statement);
}

NexusQueryPrepared NexusQuery::prepare(std::string_view statement) const {
  return NexusQueryPrepared(nexus_, options_, std::string(statement));
}

NexusQueryPrepared::NexusQueryPrepared(Nexus nexus,
                                       pluss::table::DataFrame::SqlOptions options,
                                       std::string statement_template)
    : nexus_(std::move(nexus)),
      options_(std::move(options)),
      statement_template_(std::move(statement_template)) {}

pluss::table::DataFrame NexusQueryPrepared::execute() const {
  return NexusQuery(nexus_, options_).execute(statement_template_);
}

pluss::table::DataFrame NexusQueryPrepared::execute(
    std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params) const {
  auto opts = options_;
  for (auto& [k, v] : params) {
    opts.params[k] = std::move(v);
  }
  return NexusQuery(nexus_, std::move(opts)).execute(statement_template_);
}

pluss::table::DataFrame NexusQueryPrepared::execute(
    std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params,
    std::unordered_map<std::string, std::string> identifier_params) const {
  auto opts = options_;
  for (auto& [k, v] : params) {
    opts.params[k] = std::move(v);
  }
  for (auto& [k, v] : identifier_params) {
    opts.identifier_params[k] = std::move(v);
  }
  return NexusQuery(nexus_, std::move(opts)).execute(statement_template_);
}
