namespace {

JoinPlan normalize_view_plan(const detail::NexusImpl& src,
                             std::string_view name,
                             const JoinPlan& plan,
                             std::string_view where) {
  const auto view_name = std::string(name);
  PLUSS_CHECK(!view_name.empty(), std::string(where) + ": view name cannot be empty");
  PLUSS_CHECK(!plan.root_table.empty(), std::string(where) + ": plan root_table cannot be empty");
  require_known_table(src, plan.root_table, where);

  JoinPlan out = plan;
  out.result_name = view_name;

  auto current = out.root_table;
  for (auto& step : out.steps) {
    require_known_table(src, step.left_table, where);
    require_known_table(src, step.right_table, where);
    PLUSS_CHECK(step.left_table == current,
                std::string(where) + ": step chain is broken at '" + step.left_table + "'");
    if (step.spec.right_suffix.empty()) {
      step.spec.right_suffix = std::string("_") + step.right_table;
    }
    if (step.spec.how != JoinHow::Cross &&
        step.spec.keys.empty() &&
        !step.spec.relationship_name.has_value()) {
      step.spec.relationship_name =
          require_direct_relationship_name(src, step.left_table, step.right_table, where);
    }
    current = step.right_table;
  }
  return out;
}

} // namespace

std::vector<std::string> Nexus::stores() const {
  std::vector<std::string> names;
  names.reserve(impl_ref_().saved_views_.size());
  for (const auto& [name, plan] : impl_ref_().saved_views_) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

bool Nexus::has_store(std::string_view name) const {
  return impl_ref_().saved_views_.find(std::string(name)) != impl_ref_().saved_views_.end();
}

std::optional<JoinPlan> Nexus::store(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.saved_views_.find(std::string(name));
  if (it == src.saved_views_.end()) {
    return std::nullopt;
  }
  return it->second;
}

Nexus Nexus::store(std::string_view name, const JoinPlan& plan) const {
  auto out = *this;
  out.store_(name, plan);
  return out;
}

Nexus& Nexus::store_(std::string_view name, const JoinPlan& plan) {
  auto& dst = mut_impl_();
  dst.saved_views_[std::string(name)] = normalize_view_plan(dst, name, plan, "Nexus::store");
  return *this;
}

pluss::table::DataFrame Nexus::run(std::string_view name) const {
  const auto stored = store(name);
  PLUSS_CHECK(stored.has_value(), "Nexus::run: unknown store '" + std::string(name) + "'");
  return materialize(*stored).title(std::string(name));
}

pluss::table::DataFrame Nexus::run(std::string_view name, const JoinPlan& plan) const {
  const auto normalized = normalize_view_plan(impl_ref_(), name, plan, "Nexus::run");
  return materialize(normalized).title(std::string(name));
}
