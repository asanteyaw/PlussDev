ThreadGroupBuilder Thread::group() const {
  return ThreadGroupBuilder(*this);
}

ThreadGroupBuilder Thread::group(const ThreadGroupOptions& options) const {
  return ThreadGroupBuilder(*this, options);
}

ThreadGroupBuilder::ThreadGroupBuilder(Thread value, ThreadGroupOptions options)
    : value_(std::move(value)), options_(options) {}

ThreadGrouped ThreadGroupBuilder::by(const Thread& key) const {
  return by({key});
}

ThreadGrouped ThreadGroupBuilder::by(std::initializer_list<Thread> keys) const {
  return by(pluss::msharp::RangeView::Of<Thread>(keys));
}

ThreadGrouped ThreadGroupBuilder::by(pluss::msharp::RangeView::Of<Thread> keys) const {
  PLUSS_CHECK(value_.defined(), "Thread::group: source thread is undefined");
  std::vector<Thread> grouped_keys;
  std::vector<std::string> names;
  grouped_keys.reserve(keys.size());
  names.reserve(keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    grouped_keys.push_back(keys.begin()[static_cast<std::ptrdiff_t>(i)]);
    names.push_back(group_key_name_(i, grouped_keys.back()));
  }
  return ThreadGrouped(value_, std::move(grouped_keys), std::move(names), options_);
}

ThreadGrouped::ThreadGrouped(Thread value,
                             std::vector<Thread> keys,
                             std::vector<std::string> names,
                             ThreadGroupOptions options)
    : value_(std::move(value)),
      keys_(std::move(keys)),
      names_(std::move(names)),
      options_(options) {}

const detail::GroupStateCache& ThreadGrouped::state_cached_() const {
  if (!state_cache_) {
    state_cache_ = std::make_shared<const detail::GroupStateCache>(
        build_group_state_(
            std::span<const Thread>(keys_.data(), keys_.size()),
            std::span<const std::string>(names_.data(), names_.size()),
            value_.size(),
            options_,
            "ThreadGrouped"));
  }
  return *state_cache_;
}

DataFrame ThreadGrouped::agg(pluss::msharp::RangeView::Of<ThreadGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "ThreadGrouped::agg: source thread is undefined");
  PLUSS_CHECK(!keys_.empty(), "ThreadGrouped::agg: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "ThreadGrouped::agg: at least one aggregation is required");

  const auto& state = state_cached_();

  auto out_cols = group_key_columns_(state);
  std::unordered_set<std::string> seen;
  seen.reserve(out_cols.size() + specs.size());
  for (const auto& [name, _] : out_cols) {
    seen.insert(name);
  }

  for (const auto& spec : specs) {
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(value_.named() ? value_.name() : std::string_view{}, spec.op);
    PLUSS_CHECK(seen.insert(out_name).second,
                "ThreadGrouped::agg: duplicate output '" + out_name + "'");

    auto out = group_agg_thread_(state, &value_, spec.op, "ThreadGrouped::agg");
    out_cols.emplace_back(out_name, std::move(out));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame ThreadGrouped::agg(std::initializer_list<ThreadGroupSpec> specs) const {
  return agg(pluss::msharp::RangeView::Of<ThreadGroupSpec>(specs));
}

Thread ThreadGrouped::transform(GroupOp op) const {
  PLUSS_CHECK(value_.defined(), "ThreadGrouped::transform: source thread is undefined");
  PLUSS_CHECK(!keys_.empty(), "ThreadGrouped::transform: no grouping keys supplied");

  const auto& state = state_cached_();

  if (state.ngroups == 0) {
    return group_empty_transform_thread_(value_, op, value_.size());
  }

  auto grouped = group_agg_thread_(state, &value_, op, "ThreadGrouped::transform");
  return group_broadcast_thread_(grouped, state);
}

DataFrame ThreadGrouped::transform(pluss::msharp::RangeView::Of<ThreadGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "ThreadGrouped::transform: source thread is undefined");
  PLUSS_CHECK(!keys_.empty(), "ThreadGrouped::transform: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "ThreadGrouped::transform: at least one aggregation is required");

  const auto& state = state_cached_();

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(specs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(value_.named() ? value_.name() : std::string_view{}, spec.op);
    PLUSS_CHECK(seen.insert(out_name).second,
                "ThreadGrouped::transform: duplicate output '" + out_name + "'");

    if (state.ngroups == 0) {
      out_cols.emplace_back(out_name, group_empty_transform_thread_(value_, spec.op, value_.size()));
      continue;
    }

    out_cols.emplace_back(
        out_name,
        group_broadcast_thread_(
            group_agg_thread_(state, &value_, spec.op, "ThreadGrouped::transform"),
            state));
  }

  return DataFrame(std::move(out_cols), value_.label().values());
}

DataFrame ThreadGrouped::transform(std::initializer_list<ThreadGroupSpec> specs) const {
  return transform(pluss::msharp::RangeView::Of<ThreadGroupSpec>(specs));
}

Thread ThreadGrouped::filter(const pluss::Series& keep) const {
  return filter(Thread(keep));
}

Thread ThreadGrouped::filter(const Thread& keep) const {
  PLUSS_CHECK(value_.defined(), "ThreadGrouped::filter: source thread is undefined");
  PLUSS_CHECK(!keys_.empty(), "ThreadGrouped::filter: no grouping keys supplied");

  const auto& state = state_cached_();
  return value_.filter(group_keep_mask_(keep, state, "ThreadGrouped::filter"));
}

DataFrame ThreadGrouped::terminal_(GroupOp op) const {
  return agg({ThreadGroupSpec{.output = {}, .op = op}});
}

DataFrame ThreadGrouped::count() const { return terminal_(GroupOp::Count); }
DataFrame ThreadGrouped::ndistinct() const { return terminal_(GroupOp::Ndistinct); }
DataFrame ThreadGrouped::sum() const { return terminal_(GroupOp::Sum); }
DataFrame ThreadGrouped::mean() const { return terminal_(GroupOp::Mean); }
DataFrame ThreadGrouped::median() const { return terminal_(GroupOp::Median); }
DataFrame ThreadGrouped::min() const { return terminal_(GroupOp::Min); }
DataFrame ThreadGrouped::max() const { return terminal_(GroupOp::Max); }
DataFrame ThreadGrouped::std() const { return terminal_(GroupOp::Std); }
DataFrame ThreadGrouped::var() const { return terminal_(GroupOp::Var); }
DataFrame ThreadGrouped::mode() const { return terminal_(GroupOp::Mode); }
