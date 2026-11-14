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

DataFrameGrouped DataFrameGroupBuilder::by(std::string_view key) const {
  return by(pluss::msharp::RangeView::Of<std::string_view>({key}));
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

namespace {

const char* group_terminal_name_(DataFrame::GroupOp op) {
  switch (op) {
    case DataFrame::GroupOp::Count:
      return "count";
    case DataFrame::GroupOp::Ndistinct:
      return "ndistinct";
    case DataFrame::GroupOp::Sum:
      return "sum";
    case DataFrame::GroupOp::Mean:
      return "mean";
    case DataFrame::GroupOp::Median:
      return "median";
    case DataFrame::GroupOp::Min:
      return "min";
    case DataFrame::GroupOp::Max:
      return "max";
    case DataFrame::GroupOp::Std:
      return "std";
    case DataFrame::GroupOp::Var:
      return "var";
    case DataFrame::GroupOp::Mode:
      return "mode";
  }
  return "group";
}

}

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
    std::vector<Slate> key_columns;
    key_columns.reserve(key_names.size());
    for (const auto& key : key_names) {
      key_columns.push_back(keyed.features(key));
    }
    state_cache_ = std::make_shared<const detail::GroupStateCache>(
        build_group_state_(
            std::span<const Slate>(key_columns.data(), key_columns.size()),
            std::span<const std::string>(key_names.data(), key_names.size()),
            keyed.ncases(),
            SlateGroupOptions{.dropnull = options_.dropnull, .sort = options_.sort},
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
      temp = pluss::tils::OutputEngine::format("__pluss_case_level_{}_{}", level_name, suffix++);
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
    std::optional<Slate> value_storage = std::nullopt;
    if (has_feature) {
      value_storage = keyed.features(spec.feature);
    }
    out_cols.emplace_back(
        out_name,
        group_agg_slate_(
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

  std::vector<std::pair<std::string, Slate>> out_cols;
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

    std::optional<Slate> value_storage = std::nullopt;
    if (has_feature) {
      value_storage = keyed.features(spec.feature);
    }
    if (state.ngroups == 0) {
      if (!value_storage.has_value()) {
        value_storage = Slate(pluss::make_series(std::vector<std::int64_t>(keyed.ncases(), 0)));
      }
      out_cols.emplace_back(
          out_name,
          group_empty_transform_slate_(*value_storage, spec.op, keyed.ncases()));
      continue;
    }
    out_cols.emplace_back(
        out_name,
        group_broadcast_slate_(
            group_agg_slate_(
                state,
                value_storage ? &*value_storage : nullptr,
                spec.op,
                "DataFrameGrouped::transform"),
            state));
  }

  return DataFrame(std::move(out_cols), keyed.labels().values()).title(frame_.title());
}

DataFrame DataFrameGrouped::transform(std::initializer_list<DataFrame::GroupSpec> specs) const {
  return transform(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::filter(const pluss::Series& keep) const {
  return filter(Slate(keep));
}

DataFrame DataFrameGrouped::filter(const Slate& keep) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::filter: only Flat layout is currently supported");

  const auto& state = state_cached_();
  return frame_.filter(group_keep_mask_(keep, state, "DataFrameGrouped::filter"));
}

std::vector<DataFrame::GroupSpec> DataFrameGrouped::value_specs_(DataFrame::GroupOp op) const {
  const auto& key_names = key_names_cached_();
  std::unordered_set<std::string> key_set(key_names.begin(), key_names.end());
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

std::vector<DataFrame::GroupSpec> DataFrameGrouped::selected_value_specs_(
    DataFrame::GroupOp op,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& key_names = key_names_cached_();
  std::unordered_set<std::string> key_set(key_names.begin(), key_names.end());
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());

  std::vector<DataFrame::GroupSpec> specs;
  specs.reserve(features.size());
  const auto where = std::string("DataFrameGrouped::") + group_terminal_name_(op);
  for (const auto feature_sv : features) {
    const auto feature = owned_name(feature_sv);
    PLUSS_CHECK(!feature.empty(), where + ": feature name cannot be empty");
    PLUSS_CHECK(frame_.get(feature).has_value(), where + ": unknown feature '" + feature + "'");
    PLUSS_CHECK(!key_set.contains(feature), where + ": feature '" + feature + "' is a grouping key");
    if (!seen.insert(feature).second) {
      continue;
    }
    specs.push_back(DataFrame::GroupSpec{
        .output = {},
        .feature = feature,
        .op = op,
    });
  }

  PLUSS_CHECK(!specs.empty(), where + ": at least one value feature is required");
  return specs;
}

DataFrame DataFrameGrouped::terminal_(DataFrame::GroupOp op) const {
  const auto specs = value_specs_(op);
  return agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::terminal_(
    DataFrame::GroupOp op,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto specs = selected_value_specs_(op, features);
  return agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(specs));
}

DataFrame DataFrameGrouped::count() const { return terminal_(DataFrame::GroupOp::Count); }
DataFrame DataFrameGrouped::count(std::string_view feature) const {
  return count(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::count(std::initializer_list<std::string_view> features) const {
  return count(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::count(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Count, features);
}

DataFrame DataFrameGrouped::ndistinct() const { return terminal_(DataFrame::GroupOp::Ndistinct); }
DataFrame DataFrameGrouped::ndistinct(std::string_view feature) const {
  return ndistinct(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::ndistinct(std::initializer_list<std::string_view> features) const {
  return ndistinct(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::ndistinct(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Ndistinct, features);
}

DataFrame DataFrameGrouped::sum() const { return terminal_(DataFrame::GroupOp::Sum); }
DataFrame DataFrameGrouped::sum(std::string_view feature) const {
  return sum(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::sum(std::initializer_list<std::string_view> features) const {
  return sum(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::sum(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Sum, features);
}

DataFrame DataFrameGrouped::mean() const { return terminal_(DataFrame::GroupOp::Mean); }
DataFrame DataFrameGrouped::mean(std::string_view feature) const {
  return mean(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::mean(std::initializer_list<std::string_view> features) const {
  return mean(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::mean(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Mean, features);
}

DataFrame DataFrameGrouped::median() const { return terminal_(DataFrame::GroupOp::Median); }
DataFrame DataFrameGrouped::median(std::string_view feature) const {
  return median(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::median(std::initializer_list<std::string_view> features) const {
  return median(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::median(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Median, features);
}

DataFrame DataFrameGrouped::min() const { return terminal_(DataFrame::GroupOp::Min); }
DataFrame DataFrameGrouped::min(std::string_view feature) const {
  return min(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::min(std::initializer_list<std::string_view> features) const {
  return min(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::min(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Min, features);
}

DataFrame DataFrameGrouped::max() const { return terminal_(DataFrame::GroupOp::Max); }
DataFrame DataFrameGrouped::max(std::string_view feature) const {
  return max(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::max(std::initializer_list<std::string_view> features) const {
  return max(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::max(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Max, features);
}

DataFrame DataFrameGrouped::std() const { return terminal_(DataFrame::GroupOp::Std); }
DataFrame DataFrameGrouped::std(std::string_view feature) const {
  return std(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::std(std::initializer_list<std::string_view> features) const {
  return std(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::std(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Std, features);
}

DataFrame DataFrameGrouped::var() const { return terminal_(DataFrame::GroupOp::Var); }
DataFrame DataFrameGrouped::var(std::string_view feature) const {
  return var(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::var(std::initializer_list<std::string_view> features) const {
  return var(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::var(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Var, features);
}

DataFrame DataFrameGrouped::mode() const { return terminal_(DataFrame::GroupOp::Mode); }
DataFrame DataFrameGrouped::mode(std::string_view feature) const {
  return mode(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}
DataFrame DataFrameGrouped::mode(std::initializer_list<std::string_view> features) const {
  return mode(pluss::msharp::RangeView::Of<std::string_view>(features));
}
DataFrame DataFrameGrouped::mode(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return terminal_(DataFrame::GroupOp::Mode, features);
}

DataFrame DataFrameGrouped::yield() const
{
    PLUSS_CHECK(
        options_.layout == DataFrame::GroupLayout::Flat,
        "DataFrameGrouped::yield: only Flat layout is currently supported");

    const auto& state = state_cached_();
    return frame_.take(state.grouped_rows).title(frame_.title());
}

DataFrame DataFrameGrouped::mode_ties(std::string_view feature, std::string_view output) const {
  PLUSS_CHECK(options_.layout == DataFrame::GroupLayout::Flat,
              "DataFrameGrouped::mode_ties: only Flat layout is currently supported");
  PLUSS_CHECK(!feature.empty(), "DataFrameGrouped::mode_ties: feature is required");

  const auto& keyed = keyed_frame_cached_();
  const auto& state = state_cached_();
  const auto value = keyed.features(feature);
  const bool is_note = value.kind() == SlateKind::Note;

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
                                        : pluss::tils::OutputEngine::format("{}_mode_ties", feature);
  const bool any_null = std::any_of(
      valid_out.begin(), valid_out.end(), [](bool v) { return !v; });

  Slate value_col = is_note
      ? (any_null ? group_note_slate_(note_out, valid_out) : group_note_slate_(note_out))
      : (any_null ? group_f64_slate_(numeric_out, valid_out) : group_f64_slate_(numeric_out));
  out_cols.emplace_back(out_name, std::move(value_col));

  return DataFrame(std::move(out_cols)).title(frame_.title());
}
