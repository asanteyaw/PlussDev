DataFrameGroupBuilder DataFrame::group() const {
  return DataFrameGroupBuilder(*this);
}

DataFrameGroupBuilder DataFrame::group(const GroupOptions& options) const {
  return DataFrameGroupBuilder(*this, options);
}

DataFrameGroupBuilder::DataFrameGroupBuilder(DataFrame frame, DataFrame::GroupOptions options)
    : frame_(std::move(frame)), options_(options) {}

DataFrameGrouped DataFrameGroupBuilder::by(std::initializer_list<std::string_view> keys) const {
  return by(pluss::msharp::RangeView::Of<std::string_view>(keys));
}

DataFrameGrouped DataFrameGroupBuilder::by(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  return DataFrameGrouped(frame_, normalize_name_list(keys), options_);
}

DataFrameGrouped DataFrameGroupBuilder::by(const DataFrame::LabelGroup& keys) const {
  return DataFrameGrouped(frame_, keys, options_);
}

DataFrameGrouped::DataFrameGrouped(DataFrame frame,
                                   std::vector<std::string> keys,
                                   DataFrame::GroupOptions options)
    : frame_(std::move(frame)),
      keys_(std::move(keys)),
      options_(options) {}

DataFrameGrouped::DataFrameGrouped(DataFrame frame,
                                   DataFrame::LabelGroup labels,
                                   DataFrame::GroupOptions options)
    : frame_(std::move(frame)),
      labels_(std::move(labels)),
      options_(options) {}

const DataFrame& DataFrameGrouped::keyed_frame_cached_() const {
  if (!keyed_cache_ready_) {
    keyed_frame_cache_ = keyed_frame_(key_names_cache_);
    keyed_cache_ready_ = true;
  }
  return keyed_frame_cache_;
}

const std::vector<std::string>& DataFrameGrouped::key_names_cached_() const {
  (void)keyed_frame_cached_();
  return key_names_cache_;
}

const detail::GroupStateCache& DataFrameGrouped::state_cached_() const {
  if (!state_cache_) {
    const auto& keyed = keyed_frame_cached_();
    const auto& key_names = key_names_cached_();
    std::vector<Thread> key_columns;
    key_columns.reserve(key_names.size());
    for (const auto& key : key_names) {
      key_columns.push_back(keyed.features(key));
    }
    state_cache_ = std::make_shared<const detail::GroupStateCache>(
        build_group_state_(
            std::span<const Thread>(key_columns.data(), key_columns.size()),
            std::span<const std::string>(key_names.data(), key_names.size()),
            keyed.ncases(),
            ThreadGroupOptions{.dropnull = options_.dropnull, .sort = options_.sort},
            "DataFrameGrouped"));
  }
  return *state_cache_;
}

DataFrame DataFrameGrouped::keyed_frame_(std::vector<std::string>& key_names) const {
  if (!labels_.has_value()) {
    key_names = keys_;
    return frame_;
  }

  const auto& keys = *labels_;
  PLUSS_CHECK(frame_.has_caselbls(),
              "DataFrameGrouped(LabelGroup): current DataFrame has no case labels");

  std::vector<std::string> level_names = keys.names;
  if (!keys.label.empty()) {
    PLUSS_CHECK(level_names.empty(),
                "DataFrameGrouped(LabelGroup): use either `label` or `names`, not both");
    level_names.push_back(keys.label);
  }
  PLUSS_CHECK(!level_names.empty(),
              "DataFrameGrouped(LabelGroup): expected at least one case label");

  auto keyed = frame_;
  key_names.clear();
  key_names.reserve(level_names.size());
  for (const auto& level_name : level_names) {
    PLUSS_CHECK(!level_name.empty(),
                "DataFrameGrouped(LabelGroup): case-label name cannot be empty");
    auto temp = "__pluss_case_level_" + level_name;
    std::int64_t suffix = 1;
    while (keyed.get(temp).has_value()) {
      temp = fmt::format("__pluss_case_level_{}_{}", level_name, suffix++);
    }
    auto level_values = keyed.caselbl(level_name);
    if (detail::has_display_labels(level_values)) {
      level_values = detail::clear_display_labels(level_values);
    }
    if (level_values.named()) {
      level_values = level_values.unname();
    }
    set_prepared_feature_(keyed.mut_impl_(), temp, clear_assignment_metadata_(level_values));
    key_names.push_back(std::move(temp));
  }
  return keyed;
}

DataFrame DataFrameGrouped::agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::agg: only Flat layout is currently supported");
  PLUSS_CHECK(specs.size() != 0, "DataFrameGrouped::agg: at least one GroupSpec is required");

  const auto& keyed = keyed_frame_cached_();
  const auto& key_names = key_names_cached_();
  const auto& state = state_cached_();

  auto out_cols = group_key_columns_(state);
  std::unordered_set<std::string> out_seen(key_names.begin(), key_names.end());
  out_seen.reserve(key_names.size() + specs.size());
  for (const auto& spec : specs) {
    const bool has_feature = !spec.feature.empty();
    PLUSS_CHECK(
        spec.op == DataFrame::GroupOp::Count || has_feature,
        "DataFrameGrouped::agg: feature is required for non-count aggregations");
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(spec.feature, spec.op);
    PLUSS_CHECK(
        out_seen.insert(out_name).second,
        "DataFrameGrouped::agg: duplicate output feature '" + out_name + "'");
    std::optional<Thread> value_storage = std::nullopt;
    if (has_feature) {
      value_storage = keyed.features(spec.feature);
    }
    out_cols.emplace_back(
        out_name,
        group_agg_thread_(
            state,
            value_storage ? &*value_storage : nullptr,
            spec.op,
            "DataFrameGrouped::agg"));
  }

  auto grouped = DataFrame(std::move(out_cols)).title(frame_.title());
  if (!labels_.has_value()) {
    return grouped;
  }

  const auto& keys = *labels_;
  std::vector<std::string> level_names = keys.names;
  if (!keys.label.empty()) {
    level_names.push_back(keys.label);
  }
  std::vector<std::pair<std::string_view, std::string_view>> renames;
  renames.reserve(level_names.size());
  for (std::size_t i = 0; i < level_names.size(); ++i) {
    renames.emplace_back(key_names[i], level_names[i]);
  }
  return grouped.rename(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(
          std::span<const std::pair<std::string_view, std::string_view>>(renames.data(), renames.size())));
}

DataFrame DataFrameGrouped::agg(std::initializer_list<DataFrame::GroupSpec> specs) const {
  return agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::transform(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::transform: only Flat layout is currently supported");
  PLUSS_CHECK(specs.size() != 0, "DataFrameGrouped::transform: at least one GroupSpec is required");

  const auto& keyed = keyed_frame_cached_();
  const auto& state = state_cached_();

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(specs.size());
  std::unordered_set<std::string> out_seen;
  out_seen.reserve(specs.size());

  for (const auto& spec : specs) {
    const bool has_feature = !spec.feature.empty();
    PLUSS_CHECK(
        spec.op == DataFrame::GroupOp::Count || has_feature,
        "DataFrameGrouped::transform: feature is required for non-count aggregations");
    const auto out_name = !spec.output.empty()
        ? spec.output
        : group_value_output_name_(spec.feature, spec.op);
    PLUSS_CHECK(out_seen.insert(out_name).second,
                "DataFrameGrouped::transform: duplicate output feature '" + out_name + "'");

    std::optional<Thread> value_storage = std::nullopt;
    if (has_feature) {
      value_storage = keyed.features(spec.feature);
    }
    if (state.ngroups == 0) {
      if (!value_storage.has_value()) {
        value_storage = Thread(pluss::make_series(std::vector<std::int64_t>(keyed.ncases(), 0)));
      }
      out_cols.emplace_back(
          out_name,
          group_empty_transform_thread_(*value_storage, spec.op, keyed.ncases()));
      continue;
    }
    out_cols.emplace_back(
        out_name,
        group_broadcast_thread_(
            group_agg_thread_(
                state,
                value_storage ? &*value_storage : nullptr,
                spec.op,
                "DataFrameGrouped::transform"),
            state));
  }

  return DataFrame(std::move(out_cols), keyed.label().values()).title(frame_.title());
}

DataFrame DataFrameGrouped::transform(std::initializer_list<DataFrame::GroupSpec> specs) const {
  return transform(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::filter(const pluss::Series& keep) const {
  return filter(Thread(keep));
}

DataFrame DataFrameGrouped::filter(const Thread& keep) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::filter: only Flat layout is currently supported");

  const auto& state = state_cached_();
  return frame_.filter(group_keep_mask_(keep, state, "DataFrameGrouped::filter"));
}

std::vector<DataFrame::GroupSpec> DataFrameGrouped::value_specs_(DataFrame::GroupOp op) const {
  std::unordered_set<std::string> key_set(keys_.begin(), keys_.end());
  std::vector<DataFrame::GroupSpec> specs;
  for (const auto& name : frame_.features()) {
    if (key_set.contains(name)) {
      continue;
    }
    specs.push_back(DataFrame::GroupSpec{
        .output = {},
        .feature = name,
        .op = op,
    });
  }

  if (op == DataFrame::GroupOp::Count && specs.empty()) {
    specs.push_back(DataFrame::GroupSpec{
        .output = "count",
        .feature = {},
        .op = op,
    });
  }

  PLUSS_CHECK(!specs.empty(),
              "DataFrameGrouped: no value features remain after excluding grouping keys");
  return specs;
}

DataFrame DataFrameGrouped::terminal_(DataFrame::GroupOp op) const {
  const auto specs = value_specs_(op);
  return agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::count() const { return terminal_(DataFrame::GroupOp::Count); }
DataFrame DataFrameGrouped::ndistinct() const { return terminal_(DataFrame::GroupOp::Ndistinct); }
DataFrame DataFrameGrouped::sum() const { return terminal_(DataFrame::GroupOp::Sum); }
DataFrame DataFrameGrouped::mean() const { return terminal_(DataFrame::GroupOp::Mean); }
DataFrame DataFrameGrouped::median() const { return terminal_(DataFrame::GroupOp::Median); }
DataFrame DataFrameGrouped::min() const { return terminal_(DataFrame::GroupOp::Min); }
DataFrame DataFrameGrouped::max() const { return terminal_(DataFrame::GroupOp::Max); }
DataFrame DataFrameGrouped::std() const { return terminal_(DataFrame::GroupOp::Std); }
DataFrame DataFrameGrouped::var() const { return terminal_(DataFrame::GroupOp::Var); }
DataFrame DataFrameGrouped::mode() const { return terminal_(DataFrame::GroupOp::Mode); }

DataFrame DataFrameGrouped::mode_ties(std::string_view feature, std::string_view output) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::mode_ties: only Flat layout is currently supported");
  PLUSS_CHECK(!feature.empty(), "DataFrameGrouped::mode_ties: feature is required");

  const auto& keyed = keyed_frame_cached_();
  const auto& state = state_cached_();
  const auto value = keyed.features(feature);
  const bool is_note = value.kind() == ThreadKind::Note;

  const auto note_ties = is_note ? group_note_mode_ties_(value, state)
                                 : std::vector<std::vector<std::string>>{};
  const auto numeric_ties = is_note ? std::vector<std::vector<double>>{}
                                    : group_numeric_mode_ties_(value, state);

  // Walk groups in *output* order, expanding each group into one row per
  // tied value (or exactly one null row if it had no observed values at
  // all) — every other grouped terminal guarantees one row per group, and
  // this keeps that guarantee even though the row *count* is no longer
  // fixed at exactly one.
  std::vector<std::int64_t> expanded_positions;
  std::vector<std::string> note_out;
  std::vector<double> numeric_out;
  std::vector<bool> valid_out;

  for (std::int64_t pos = 0; pos < state.ngroups; ++pos) {
    const auto group_id = state.order[static_cast<std::size_t>(pos)];
    if (is_note) {
      const auto& tied = note_ties[static_cast<std::size_t>(group_id)];
      if (tied.empty()) {
        expanded_positions.push_back(pos);
        note_out.emplace_back();
        valid_out.push_back(false);
      } else {
        for (const auto& candidate : tied) {
          expanded_positions.push_back(pos);
          note_out.push_back(candidate);
          valid_out.push_back(true);
        }
      }
    } else {
      const auto& tied = numeric_ties[static_cast<std::size_t>(group_id)];
      if (tied.empty()) {
        expanded_positions.push_back(pos);
        numeric_out.push_back(0.0);
        valid_out.push_back(false);
      } else {
        for (const auto candidate : tied) {
          expanded_positions.push_back(pos);
          numeric_out.push_back(candidate);
          valid_out.push_back(true);
        }
      }
    }
  }

  auto out_cols = group_key_columns_expanded_(state, expanded_positions);

  const auto out_name = !output.empty() ? std::string(output)
                                        : fmt::format("{}_mode_ties", feature);
  const bool any_null = std::any_of(
      valid_out.begin(), valid_out.end(), [](bool v) { return !v; });

  Thread value_col = is_note
      ? (any_null ? group_note_thread_(note_out, valid_out) : group_note_thread_(note_out))
      : (any_null ? group_f64_thread_(numeric_out, valid_out) : group_f64_thread_(numeric_out));
  out_cols.emplace_back(out_name, std::move(value_col));

  return DataFrame(std::move(out_cols)).title(frame_.title());
}
