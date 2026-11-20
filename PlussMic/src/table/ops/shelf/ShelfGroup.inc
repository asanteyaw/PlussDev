ShelfGroupBuilder Shelf::group() const {
  return ShelfGroupBuilder(*this);
}

ShelfGroupBuilder Shelf::group(const ShelfGroupOptions& options) const {
  return ShelfGroupBuilder(*this, options);
}

ShelfGroupBuilder::ShelfGroupBuilder(Shelf value, ShelfGroupOptions options)
    : value_(std::move(value)), options_(options) {}

ShelfGrouped ShelfGroupBuilder::by(const Shelf& key) const {
  return by({key});
}

ShelfGrouped ShelfGroupBuilder::by(std::initializer_list<Shelf> keys) const {
  return by(pluss::msharp::RangeView::Of<Shelf>(keys));
}

ShelfGrouped ShelfGroupBuilder::by(pluss::msharp::RangeView::Of<Shelf> keys) const {
  PLUSS_CHECK(value_.defined(), "Shelf::group: source shelf is undefined");
  std::vector<Shelf> grouped_keys;
  std::vector<std::string> names;
  grouped_keys.reserve(keys.size());
  names.reserve(keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    grouped_keys.push_back(keys.begin()[static_cast<std::ptrdiff_t>(i)]);
    names.push_back(group_key_name_(i, grouped_keys.back()));
  }
  return ShelfGrouped(value_, std::move(grouped_keys), std::move(names), options_);
}

ShelfGrouped::ShelfGrouped(Shelf value,
                             std::vector<Shelf> keys,
                             std::vector<std::string> names,
                             ShelfGroupOptions options)
    : value_(std::move(value)),
      keys_(std::move(keys)),
      names_(std::move(names)),
      options_(options) {}

const detail::GroupStateCache& ShelfGrouped::state_cached_() const {
  if (!state_cache_) {
    state_cache_ = std::make_shared<const detail::GroupStateCache>(
        build_group_state_(
            std::span<const Shelf>(keys_.data(), keys_.size()),
            std::span<const std::string>(names_.data(), names_.size()),
            value_.size(),
            options_,
            "ShelfGrouped"));
  }
  return *state_cache_;
}

DataFrame ShelfGrouped::agg(pluss::msharp::RangeView::Of<ShelfGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "ShelfGrouped::agg: source shelf is undefined");
  PLUSS_CHECK(!keys_.empty(), "ShelfGrouped::agg: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "ShelfGrouped::agg: at least one aggregation is required");

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
                "ShelfGrouped::agg: duplicate output '" + out_name + "'");

    auto out = group_agg_shelf_(state, &value_, spec.op, "ShelfGrouped::agg");
    out_cols.emplace_back(out_name, std::move(out));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame ShelfGrouped::agg(std::initializer_list<ShelfGroupSpec> specs) const {
  return agg(pluss::msharp::RangeView::Of<ShelfGroupSpec>(specs));
}

Shelf ShelfGrouped::transform(GroupOp op) const {
  PLUSS_CHECK(value_.defined(), "ShelfGrouped::transform: source shelf is undefined");
  PLUSS_CHECK(!keys_.empty(), "ShelfGrouped::transform: no grouping keys supplied");

  const auto& state = state_cached_();

  if (state.ngroups == 0) {
    return group_empty_transform_shelf_(value_, op, value_.size());
  }

  auto grouped = group_agg_shelf_(state, &value_, op, "ShelfGrouped::transform");
  return group_broadcast_shelf_(grouped, state);
}

DataFrame ShelfGrouped::transform(pluss::msharp::RangeView::Of<ShelfGroupSpec> specs) const {
  PLUSS_CHECK(value_.defined(), "ShelfGrouped::transform: source shelf is undefined");
  PLUSS_CHECK(!keys_.empty(), "ShelfGrouped::transform: no grouping keys supplied");
  PLUSS_CHECK(specs.size() != 0, "ShelfGrouped::transform: at least one aggregation is required");

  const auto& state = state_cached_();

  std::vector<std::pair<std::string, Shelf>> out_cols;
  out_cols.reserve(specs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(value_.named() ? value_.name() : std::string_view{}, spec.op);
    PLUSS_CHECK(seen.insert(out_name).second,
                "ShelfGrouped::transform: duplicate output '" + out_name + "'");

    if (state.ngroups == 0) {
      out_cols.emplace_back(out_name, group_empty_transform_shelf_(value_, spec.op, value_.size()));
      continue;
    }

    out_cols.emplace_back(
        out_name,
        group_broadcast_shelf_(
            group_agg_shelf_(state, &value_, spec.op, "ShelfGrouped::transform"),
            state));
  }

  return DataFrame(std::move(out_cols), value_.label().values());
}

DataFrame ShelfGrouped::transform(std::initializer_list<ShelfGroupSpec> specs) const {
  return transform(pluss::msharp::RangeView::Of<ShelfGroupSpec>(specs));
}

Shelf ShelfGrouped::filter(const pluss::Series& keep) const {
  return filter(Shelf(keep));
}

Shelf ShelfGrouped::filter(const Shelf& keep) const {
  PLUSS_CHECK(value_.defined(), "ShelfGrouped::filter: source shelf is undefined");
  PLUSS_CHECK(!keys_.empty(), "ShelfGrouped::filter: no grouping keys supplied");

  const auto& state = state_cached_();
  return value_.filter(group_keep_mask_(keep, state, "ShelfGrouped::filter"));
}

DataFrame ShelfGrouped::terminal_(GroupOp op) const {
  return agg({ShelfGroupSpec{.output = {}, .op = op}});
}

DataFrame ShelfGrouped::count() const { return terminal_(GroupOp::Count); }
DataFrame ShelfGrouped::ndistinct() const { return terminal_(GroupOp::Ndistinct); }
DataFrame ShelfGrouped::sum() const { return terminal_(GroupOp::Sum); }
DataFrame ShelfGrouped::mean() const { return terminal_(GroupOp::Mean); }
DataFrame ShelfGrouped::median() const { return terminal_(GroupOp::Median); }
DataFrame ShelfGrouped::min() const { return terminal_(GroupOp::Min); }
DataFrame ShelfGrouped::max() const { return terminal_(GroupOp::Max); }
DataFrame ShelfGrouped::std() const { return terminal_(GroupOp::Std); }
DataFrame ShelfGrouped::var() const { return terminal_(GroupOp::Var); }
DataFrame ShelfGrouped::mode() const { return terminal_(GroupOp::Mode); }
