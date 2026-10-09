SlateGroupBuilder Slate::group() const {
  return SlateGroupBuilder(*this);
}

SlateGroupBuilder Slate::group(const SlateGroupOptions& options) const {
  return SlateGroupBuilder(*this, options);
}

SlateGroupBuilder::SlateGroupBuilder(Slate value, SlateGroupOptions options)
    : value_(std::move(value)), options_(options) {}

SlateGrouped SlateGroupBuilder::by(const Slate& key) const {
  return by({key});
}

SlateGrouped SlateGroupBuilder::by(std::initializer_list<Slate> keys) const {
  return by(pluss::msharp::RangeView::Of<Slate>(keys));
}

SlateGrouped SlateGroupBuilder::by(pluss::msharp::RangeView::Of<Slate> keys) const {
  PLUSS_CHECK(value_.defined(), "Slate::group: source slate is undefined");
  std::vector<Slate> grouped_keys;
  std::vector<std::string> names;
  grouped_keys.reserve(keys.size());
  names.reserve(keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    grouped_keys.push_back(keys.begin()[static_cast<std::ptrdiff_t>(i)]);
    names.push_back(group_key_name_(i, grouped_keys.back()));
  }
  return SlateGrouped(value_, std::move(grouped_keys), std::move(names), options_);
}

SlateGrouped::SlateGrouped(Slate value,
                             std::vector<Slate> keys,
                             std::vector<std::string> names,
                             SlateGroupOptions options)
    : value_(std::move(value)),
      keys_(std::move(keys)),
      names_(std::move(names)),
      options_(options) {}

const detail::GroupStateCache& SlateGrouped::state_cached_() const {
  if (!state_cache_) {
    state_cache_ = std::make_shared<const detail::GroupStateCache>(
        build_group_state_(
            std::span<const Slate>(keys_.data(), keys_.size()),
            std::span<const std::string>(names_.data(), names_.size()),
            value_.size(),
            options_,
            "SlateGrouped"));
  }
  return *state_cache_;
}

DataFrame SlateGrouped::agg(pluss::msharp::RangeView::Of<SlateGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "SlateGrouped::agg: source slate is undefined");
  PLUSS_CHECK(!keys_.empty(), "SlateGrouped::agg: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "SlateGrouped::agg: at least one aggregation is required");

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
                "SlateGrouped::agg: duplicate output '" + out_name + "'");

    auto out = group_agg_slate_(state, &value_, spec.op, "SlateGrouped::agg");
    out_cols.emplace_back(out_name, std::move(out));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame SlateGrouped::agg(std::initializer_list<SlateGroupSpec> specs) const {
  return agg(pluss::msharp::RangeView::Of<SlateGroupSpec>(specs));
}

Slate SlateGrouped::transform(GroupOp op) const {
  PLUSS_CHECK(value_.defined(), "SlateGrouped::transform: source slate is undefined");
  PLUSS_CHECK(!keys_.empty(), "SlateGrouped::transform: no grouping keys supplied");

  const auto& state = state_cached_();

  if (state.ngroups == 0) {
    return group_empty_transform_slate_(value_, op, value_.size());
  }

  auto grouped = group_agg_slate_(state, &value_, op, "SlateGrouped::transform");
  return group_broadcast_slate_(grouped, state);
}

DataFrame SlateGrouped::transform(pluss::msharp::RangeView::Of<SlateGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "SlateGrouped::transform: source slate is undefined");
  PLUSS_CHECK(!keys_.empty(), "SlateGrouped::transform: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "SlateGrouped::transform: at least one aggregation is required");

  const auto& state = state_cached_();

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(specs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(value_.named() ? value_.name() : std::string_view{}, spec.op);
    PLUSS_CHECK(seen.insert(out_name).second,
                "SlateGrouped::transform: duplicate output '" + out_name + "'");

    if (state.ngroups == 0) {
      out_cols.emplace_back(out_name, group_empty_transform_slate_(value_, spec.op, value_.size()));
      continue;
    }

    out_cols.emplace_back(
        out_name,
        group_broadcast_slate_(
            group_agg_slate_(state, &value_, spec.op, "SlateGrouped::transform"),
            state));
  }

  return DataFrame(std::move(out_cols), value_.label().values());
}

DataFrame SlateGrouped::transform(std::initializer_list<SlateGroupSpec> specs) const {
  return transform(pluss::msharp::RangeView::Of<SlateGroupSpec>(specs));
}

Slate SlateGrouped::filter(const pluss::Series& keep) const {
  return filter(Slate(keep));
}

Slate SlateGrouped::filter(const Slate& keep) const {
  PLUSS_CHECK(value_.defined(), "SlateGrouped::filter: source slate is undefined");
  PLUSS_CHECK(!keys_.empty(), "SlateGrouped::filter: no grouping keys supplied");

  const auto& state = state_cached_();
  return value_.filter(group_keep_mask_(keep, state, "SlateGrouped::filter"));
}

DataFrame SlateGrouped::terminal_(GroupOp op) const {
  return agg({SlateGroupSpec{.output = {}, .op = op}});
}

DataFrame SlateGrouped::count() const { return terminal_(GroupOp::Count); }
DataFrame SlateGrouped::ndistinct() const { return terminal_(GroupOp::Ndistinct); }
DataFrame SlateGrouped::sum() const { return terminal_(GroupOp::Sum); }
DataFrame SlateGrouped::mean() const { return terminal_(GroupOp::Mean); }
DataFrame SlateGrouped::median() const { return terminal_(GroupOp::Median); }
DataFrame SlateGrouped::min() const { return terminal_(GroupOp::Min); }
DataFrame SlateGrouped::max() const { return terminal_(GroupOp::Max); }
DataFrame SlateGrouped::std() const { return terminal_(GroupOp::Std); }
DataFrame SlateGrouped::var() const { return terminal_(GroupOp::Var); }
DataFrame SlateGrouped::mode() const { return terminal_(GroupOp::Mode); }
