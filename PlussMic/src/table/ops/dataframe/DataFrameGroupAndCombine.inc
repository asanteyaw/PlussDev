DataFrame DataFrame::summarize_specs(const std::vector<SummarySpec>& specs, bool render) const {
  PLUSS_CHECK(!specs.empty(), "DataFrame::summarize: no SummarySpec supplied");

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(specs.size());

  std::unordered_set<std::string> names_seen;
  names_seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::summarize: SummarySpec.feature cannot be empty");
    const auto feature_value = features(spec.feature);
    const auto out_name =
        spec.output.empty() ? fmt::format("{}_{}", spec.feature, summary_op_name(spec.op)) : spec.output;
    PLUSS_CHECK(names_seen.insert(out_name).second, "DataFrame::summarize: duplicate output name '" + out_name + "'");

    if (spec.op == SummaryOp::Count) {
      out_cols.emplace_back(out_name, make_i64_slate({non_null_count(feature_value)}));
      continue;
    }
    if (spec.op == SummaryOp::Ndistinct) {
      out_cols.emplace_back(out_name, make_i64_slate({distinct_non_null_count(feature_value)}));
      continue;
    }
    out_cols.emplace_back(out_name, make_f64_slate({numeric_reduce_value(feature_value, spec.op)}));
  }

  (void)render;
  return DataFrame(std::move(out_cols), make_note_slate({"summary"}));
}

DataFrame DataFrame::summarize(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render) const {
  return summarize_specs(std::vector<SummarySpec>(specs.begin(), specs.end()), render);
}

DataFrameView DataFrame::aggregate(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render) const {
  return DataFrameView(summarize(specs, false), render, "aggregate", title());
}

namespace {

std::vector<std::string> contingency_names_(const DataFrame& frame,
                                            pluss::msharp::RangeView::Of<std::string_view> features,
                                            std::string_view which,
                                            std::string_view where) {
  auto names = normalize_name_list(features);
  PLUSS_CHECK(!names.empty(),
              fmt::format("{}: expected at least one {} feature", where, which));
  std::unordered_set<std::string> seen;
  seen.reserve(names.size());
  for (const auto& name : names) {
    PLUSS_CHECK(!name.empty(),
                fmt::format("{}: {} feature names cannot be empty", where, which));
    PLUSS_CHECK(frame.get(name).has_value(),
                fmt::format("{}: unknown {} feature '{}'", where, which, name));
    PLUSS_CHECK(seen.insert(name).second,
                fmt::format("{}: duplicate {} feature '{}'", where, which, name));
  }
  return names;
}

std::string contingency_token_(const Slate& key,
                               std::int64_t row,
                               std::string_view null_label) {
  return detail::valid_at(key, row)
      ? slate_scalar_string_at(key, row)
      : std::string(null_label);
}

std::string contingency_column_name_(std::span<const std::string> names,
                                     std::span<const Slate> keys,
                                     std::int64_t row,
                                     std::string_view separator,
                                     std::string_view null_label) {
  PLUSS_CHECK(!names.empty(), "DataFrame::contingency: expected at least one column feature");
  PLUSS_CHECK(names.size() == keys.size(), "DataFrame::contingency: column key metadata mismatch");
  if (names.size() == 1) {
    return contingency_token_(keys.front(), row, null_label);
  }
  std::string label;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) {
      label += separator;
    }
    label += names[i];
    label += '=';
    label += contingency_token_(keys[i], row, null_label);
  }
  return label;
}

struct ContingencyTensor_ {
  std::vector<std::string> dims{};
  std::vector<Slate> levels{};
  pluss::Series counts = pluss::series::undefined();
};

std::vector<std::string> contingency_names_(
    const DataFrame& frame,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  return contingency_names_(frame, features, "contingency", where);
}

std::int64_t contingency_checked_product_(std::span<const std::int64_t> shape,
                                          std::string_view where) {
  std::int64_t product = 1;
  for (const auto extent : shape) {
    PLUSS_CHECK(extent >= 0, std::string(where) + ": contingency shape cannot contain negative extents");
    if (extent == 0) {
      return 0;
    }
    PLUSS_CHECK(product <= std::numeric_limits<std::int64_t>::max() / extent,
                std::string(where) + ": contingency table is too large to materialize densely");
    product *= extent;
  }
  return product;
}

bool contingency_host_equal_(const SlateValue::HostAny& lhs,
                             const SlateValue::HostAny& rhs) {
  using Kind = SlateValue::HostAny::Kind;
  if (lhs.kind() == Kind::String || rhs.kind() == Kind::String) {
    return lhs.kind() == Kind::String &&
           rhs.kind() == Kind::String &&
           lhs.to_str() == rhs.to_str();
  }
  if (lhs.kind() == Kind::Time || rhs.kind() == Kind::Time) {
    return lhs.kind() == Kind::Time &&
           rhs.kind() == Kind::Time &&
           lhs.to_int() == rhs.to_int();
  }
  const auto lhs_numeric = lhs.kind() == Kind::Bool || lhs.kind() == Kind::Int ||
      lhs.kind() == Kind::UInt || lhs.kind() == Kind::Double;
  const auto rhs_numeric = rhs.kind() == Kind::Bool || rhs.kind() == Kind::Int ||
      rhs.kind() == Kind::UInt || rhs.kind() == Kind::Double;
  if (lhs_numeric && rhs_numeric) {
    if (lhs.kind() != Kind::Double && rhs.kind() != Kind::Double) {
      return lhs.to_int() == rhs.to_int();
    }
    return lhs.to_dbl() == rhs.to_dbl();
  }
  return lhs.to_str() == rhs.to_str();
}

SlateValue::HostAny contingency_level_host_(const Slate& levels, std::int64_t index) {
  const std::array<std::int64_t, 1> positions{index};
  return SlateValue(levels.take(std::span<const std::int64_t>(positions.data(), positions.size()))).host();
}

std::vector<std::string> contingency_filtered_dims_(std::span<const std::string> dims,
                                                    std::span<const std::string> existing) {
  std::unordered_set<std::string> keep(existing.begin(), existing.end());
  std::vector<std::string> out;
  out.reserve(dims.size());
  for (const auto& dim : dims) {
    if (keep.contains(dim)) {
      out.push_back(dim);
    }
  }
  return out;
}

ContingencyTensor_ contingency_build_tensor_(
    const DataFrame& frame,
    const std::vector<std::string>& dims,
    const DataFrame::ContingencyOptions& options,
    std::string_view where) {
  PLUSS_CHECK(!dims.empty(), std::string(where) + ": expected at least one contingency dimension");

  std::vector<Slate> keys;
  keys.reserve(dims.size());
  std::vector<GroupFactorization_> factors;
  factors.reserve(dims.size());
  std::vector<std::int64_t> shape;
  shape.reserve(dims.size());
  std::vector<bool> included(static_cast<std::size_t>(frame.ncases()), true);

  for (const auto& dim : dims) {
    const auto key = frame.features(dim);
    keys.push_back(key);
    auto factor = factor_group_keys_(
        std::span<const Slate>(&keys.back(), 1),
        frame.ncases(),
        SlateGroupOptions{.dropnull = options.dropnull, .sort = options.sort},
        where);
    shape.push_back(factor.ngroups);
    for (std::int64_t row = 0; row < frame.ncases(); ++row) {
      included[static_cast<std::size_t>(row)] =
          included[static_cast<std::size_t>(row)] &&
          factor.group_ids[static_cast<std::size_t>(row)] >= 0;
    }
    factors.push_back(std::move(factor));
  }

  const auto cell_count =
      contingency_checked_product_(std::span<const std::int64_t>(shape.data(), shape.size()), where);
  auto linear_ids = pluss::Series::zeros(
      {frame.ncases()},
      pluss::codem::ScalarType::Int64,
      pluss::codem::Platform::CPU);
  auto linear_work = linear_ids.to(pluss::codem::ScalarType::Float64);

  std::int64_t stride = 1;
  for (std::int64_t axis = static_cast<std::int64_t>(dims.size()) - 1; axis >= 0; --axis) {
    auto ids = group_i64_series_(factors[static_cast<std::size_t>(axis)].broadcast_positions)
        .to(pluss::codem::Platform::CPU)
        .to(pluss::codem::ScalarType::Float64);
    linear_work = linear_work.add(ids.mul(static_cast<double>(stride)));
    const auto extent = shape[static_cast<std::size_t>(axis)];
    PLUSS_CHECK(extent == 0 || stride <= std::numeric_limits<std::int64_t>::max() / std::max<std::int64_t>(1, extent),
                std::string(where) + ": contingency tensor stride overflow");
    stride *= std::max<std::int64_t>(1, extent);
  }
  linear_ids = linear_work.to(pluss::codem::ScalarType::Int64);

  const auto active_mask = bool_series_from_keep(included);
  const auto active_ids = linear_ids.masked_select(active_mask);
  auto counts = pluss::Series::zeros(
      {cell_count},
      pluss::codem::ScalarType::Float64,
      pluss::codem::Platform::CPU);
  if (active_ids.numel() > 0 && cell_count > 0) {
    auto ones = pluss::Series::ones(
        {active_ids.numel()},
        pluss::codem::ScalarType::Float64,
        pluss::codem::Platform::CPU);
    counts = counts.scatter_add(0, active_ids, ones);
  }
  counts = counts.to(pluss::codem::ScalarType::Int64);
  if (dims.empty()) {
    counts = counts.reshape({1});
  } else {
    counts = counts.reshape(std::span<const std::int64_t>(shape.data(), shape.size()));
  }

  ContingencyTensor_ out;
  out.dims = dims;
  out.counts = counts;
  out.levels.reserve(dims.size());
  for (std::size_t axis = 0; axis < dims.size(); ++axis) {
    out.levels.push_back(keys[axis].take(factors[axis].ordered_first_rows));
  }
  return out;
}

} // namespace

ContingencyTable DataFrame::contingency(std::string_view row_feature,
                                        std::string_view column_feature) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>({row_feature}),
      pluss::msharp::RangeView::Of<std::string_view>({column_feature}));
}

ContingencyTable DataFrame::contingency(std::string_view row_feature,
                                        std::string_view column_feature,
                                        const ContingencyOptions& options) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>({row_feature}),
      pluss::msharp::RangeView::Of<std::string_view>({column_feature}),
      options);
}

ContingencyTable DataFrame::contingency(
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  return contingency(features, ContingencyOptions{});
}

ContingencyTable DataFrame::contingency(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const ContingencyOptions& options) const {
  const auto dims = contingency_names_(*this, features, "DataFrame::contingency");
  auto tensor = contingency_build_tensor_(*this, dims, options, "DataFrame::contingency");

  std::vector<std::string> default_rows;
  std::vector<std::string> default_cols;
  if (!dims.empty()) {
    default_rows.push_back(dims.front());
    default_cols.insert(default_cols.end(), dims.begin() + 1, dims.end());
  }
  return ContingencyTable(
      std::move(tensor.dims),
      std::move(tensor.levels),
      std::move(tensor.counts),
      options,
      std::move(default_rows),
      std::move(default_cols));
}

ContingencyTable DataFrame::contingency(
    pluss::msharp::RangeView::Of<std::string_view> row_features,
    pluss::msharp::RangeView::Of<std::string_view> column_features) const {
  return contingency(row_features, column_features, ContingencyOptions{});
}

ContingencyTable DataFrame::contingency(
    pluss::msharp::RangeView::Of<std::string_view> row_features,
    pluss::msharp::RangeView::Of<std::string_view> column_features,
    const ContingencyOptions& options) const {
  const auto rows = contingency_names_(*this, row_features, "row", "DataFrame::contingency");
  const auto columns = contingency_names_(*this, column_features, "column", "DataFrame::contingency");
  PLUSS_CHECK(!rows.empty(), "DataFrame::contingency: expected at least one row feature");
  PLUSS_CHECK(!columns.empty(), "DataFrame::contingency: expected at least one column feature");

  std::vector<std::string> dims = rows;
  std::unordered_set<std::string> seen(rows.begin(), rows.end());
  for (const auto& dim : columns) {
    PLUSS_CHECK(seen.insert(dim).second,
                "DataFrame::contingency: the same feature cannot appear on both row and column axes");
    dims.push_back(dim);
  }

  auto tensor = contingency_build_tensor_(*this, dims, options, "DataFrame::contingency");
  return ContingencyTable(
      std::move(tensor.dims),
      std::move(tensor.levels),
      std::move(tensor.counts),
      options,
      rows,
      columns);
}

DataFrame DataFrame::group(pluss::msharp::RangeView::Of<std::string_view> keys,
                           pluss::msharp::RangeView::Of<GroupSpec> specs,
                           bool dropnull_keys,
                           bool render,
                           bool sort_groups) const {
  PLUSS_CHECK(keys.size() != 0, "DataFrame::group: at least one key feature is required");
  PLUSS_CHECK(specs.size() != 0, "DataFrame::group: at least one GroupSpec is required");

  const auto key_names = normalize_name_list(keys);
  std::unordered_set<std::string> key_seen;
  key_seen.reserve(key_names.size());
  std::vector<Slate> key_columns;
  key_columns.reserve(key_names.size());
  for (const auto& key : key_names) {
    PLUSS_CHECK(key_seen.insert(key).second, "DataFrame::group: duplicate key '" + key + "'");
    key_columns.push_back(features(key));
  }

  const auto state = build_group_state_(
      std::span<const Slate>(key_columns.data(), key_columns.size()),
      std::span<const std::string>(key_names.data(), key_names.size()),
      ncases(),
      SlateGroupOptions{.dropnull = dropnull_keys, .sort = sort_groups},
      "DataFrame::group");

  auto out_cols = group_key_columns_(state);

  std::unordered_set<std::string> out_seen(key_names.begin(), key_names.end());
  out_seen.reserve(key_names.size() + specs.size());
  for (const auto& spec : specs) {
    const bool has_feature = !spec.feature.empty();
    PLUSS_CHECK(
        spec.op == GroupOp::Count || has_feature,
        "DataFrame::group: feature is required for non-count aggregations");
    const auto out_name =
        !spec.output.empty()
            ? spec.output
            : group_value_output_name_(spec.feature, spec.op);
    PLUSS_CHECK(
        out_seen.insert(out_name).second,
        "DataFrame::group: duplicate output feature '" + out_name + "'");

    Slate out;
    if (!has_feature) {
      PLUSS_CHECK(spec.op == GroupOp::Count,
                  "DataFrame::group: feature is required for non-count aggregations");
      out = group_count_slate_(state);
    } else {
      const auto value_column = features(spec.feature);
      switch (spec.op) {
        case GroupOp::Count:
          out = group_count_slate_(state, value_column);
          break;
        case GroupOp::Ndistinct:
          out = group_ndistinct_slate_(value_column, state);
          break;
        case GroupOp::Mode:
          out = value_column.kind() == SlateKind::Note
              ? group_note_mode_slate_(value_column, state)
              : group_numeric_slate_(value_column, state, spec.op);
          break;
        case GroupOp::Sum:
        case GroupOp::Mean:
        case GroupOp::Median:
        case GroupOp::Min:
        case GroupOp::Max:
        case GroupOp::Std:
        case GroupOp::Var:
          out = group_numeric_slate_(value_column, state, spec.op);
          break;
      }
    }
    out_cols.emplace_back(out_name, std::move(out));
  }

  (void)render;
  return DataFrame(std::move(out_cols)).title(title());
}

DataFrameView DataFrame::pivot(std::string_view case_feature,
                               std::string_view feature_feature,
                               std::string_view value_feature,
                               bool dropnull_keys,
                               bool render) const {
  const auto dup = group(
      {case_feature, feature_feature},
      {GroupSpec{"__pivot_cell_count__", "", GroupOp::Count}},
      dropnull_keys,
      false);
  const auto counts = dup.features("__pivot_cell_count__").as_series();
  const bool has_duplicate = counts.gt(1).any().item<bool>();
  if (has_duplicate) {
    throw ::pluss::Error(
        "DataFrame::pivot: duplicate case/feature pairs found; use pivot_table with an aggregation op");
  }
  auto out = pivot_table(case_feature, feature_feature, value_feature, GroupOp::Max, dropnull_keys, false);
  return DataFrameView(static_cast<DataFrame>(out), render, "pivot", title());
}

struct PivotResolvedSpec_ {
  std::string feature{};
  std::string group_output{};
  std::string display{};
  DataFrame::GroupOp op{DataFrame::GroupOp::Mean};
};

struct PivotResolvedPlan_ {
  std::vector<std::string> index{};
  std::vector<std::string> columns{};
  std::vector<PivotResolvedSpec_> specs{};
  bool omit_value_prefix{false};
};

std::vector<std::string> pivot_names_(const DataFrame& frame,
                                      const std::vector<std::string>& names,
                                      std::string_view which,
                                      std::string_view where) {
  std::unordered_set<std::string> seen;
  std::vector<std::string> out;
  out.reserve(names.size());
  for (const auto& name : names) {
    PLUSS_CHECK(!name.empty(),
                fmt::format("{}: {} feature names cannot be empty", where, which));
    PLUSS_CHECK(frame.get(name).has_value(),
                fmt::format("{}: unknown {} feature '{}'", where, which, name));
    PLUSS_CHECK(seen.insert(name).second,
                fmt::format("{}: duplicate {} feature '{}'", where, which, name));
    out.push_back(name);
  }
  return out;
}

std::vector<std::string_view> pivot_string_views_(std::span<const std::string> names) {
  std::vector<std::string_view> out;
  out.reserve(names.size());
  for (const auto& name : names) {
    out.push_back(name);
  }
  return out;
}

bool pivot_supports_feature_(const Slate& value, DataFrame::GroupOp op) {
  switch (op) {
    case DataFrame::GroupOp::Count:
    case DataFrame::GroupOp::Ndistinct:
      return true;
    case DataFrame::GroupOp::Mode:
      return value.kind() == SlateKind::Note ||
          (value.kind() == SlateKind::Series && is_numeric_series_type(value.as_series()));
    case DataFrame::GroupOp::Sum:
    case DataFrame::GroupOp::Mean:
    case DataFrame::GroupOp::Median:
    case DataFrame::GroupOp::Min:
    case DataFrame::GroupOp::Max:
    case DataFrame::GroupOp::Std:
    case DataFrame::GroupOp::Var:
      return value.kind() == SlateKind::Series && is_numeric_series_type(value.as_series());
  }
  return false;
}

std::string pivot_default_value_label_(std::string_view feature, DataFrame::GroupOp op) {
  if (feature.empty()) {
    PLUSS_CHECK(op == DataFrame::GroupOp::Count,
                "DataFrame::pivot_table: only Count supports an empty value feature");
    return "count";
  }
  return group_value_output_name_(feature, op);
}

PivotResolvedPlan_ pivot_resolve_plan_(const DataFrame& frame,
                                       const DataFrame::PivotTableOptions& options,
                                       std::string_view where) {
  PivotResolvedPlan_ plan;
  plan.index = pivot_names_(frame, options.index, "index", where);
  plan.columns = pivot_names_(frame, options.columns, "column", where);

  std::unordered_set<std::string> key_seen(plan.index.begin(), plan.index.end());
  for (const auto& name : plan.columns) {
    PLUSS_CHECK(key_seen.insert(name).second,
                fmt::format("{}: the same feature cannot appear on both index and column axes ('{}')",
                            where,
                            name));
  }

  PLUSS_CHECK(!plan.index.empty() || !plan.columns.empty(),
              std::string(where) + ": at least one index or column feature is required");
  PLUSS_CHECK(!options.margins || !options.margins_name.empty(),
              std::string(where) + ": margins_name cannot be empty when margins=true");

  if (!options.specs.empty()) {
    plan.specs.reserve(options.specs.size());
    for (std::size_t i = 0; i < options.specs.size(); ++i) {
      const auto& spec = options.specs[i];
      PLUSS_CHECK(spec.op == DataFrame::GroupOp::Count || !spec.feature.empty(),
                  std::string(where) + ": feature is required for non-count pivot value specs");
      if (!spec.feature.empty()) {
        const auto feature = frame.features(spec.feature);
        PLUSS_CHECK(
            pivot_supports_feature_(feature, spec.op),
            fmt::format("{}: feature '{}' does not support pivot aggregation '{}'",
                        where,
                        spec.feature,
                        group_op_token_(spec.op)));
      }
      plan.specs.push_back(PivotResolvedSpec_{
          .feature = spec.feature,
          .group_output = fmt::format("__pivot_v{}", i),
          .display = spec.output.empty()
              ? pivot_default_value_label_(spec.feature, spec.op)
              : spec.output,
          .op = spec.op,
      });
    }
  } else {
    auto ops = options.aggfuncs;
    if (ops.empty()) {
      ops.push_back(DataFrame::GroupOp::Mean);
    }
    std::vector<std::string> values = options.values;
    if (values.empty()) {
      const auto all = frame.features();
      for (const auto& name : all) {
        if (key_seen.contains(name)) {
          continue;
        }
        const auto value = frame.features(name);
        const bool supported = std::all_of(ops.begin(), ops.end(), [&](const auto op) {
          return pivot_supports_feature_(value, op);
        });
        if (supported) {
          values.push_back(name);
        }
      }
      if (values.empty() && ops.size() == 1 && ops.front() == DataFrame::GroupOp::Count) {
        plan.specs.push_back(PivotResolvedSpec_{
            .feature = "",
            .group_output = "__pivot_v0",
            .display = "count",
            .op = DataFrame::GroupOp::Count,
        });
      }
    }
    if (!values.empty()) {
      values = pivot_names_(frame, values, "value", where);
      plan.specs.reserve(values.size() * ops.size());
      std::size_t next = 0;
      for (const auto& feature_name : values) {
        const auto value = frame.features(feature_name);
        for (const auto op : ops) {
          PLUSS_CHECK(
              pivot_supports_feature_(value, op),
              fmt::format("{}: feature '{}' does not support pivot aggregation '{}'",
                          where,
                          feature_name,
                          group_op_token_(op)));
          plan.specs.push_back(PivotResolvedSpec_{
              .feature = feature_name,
              .group_output = fmt::format("__pivot_v{}", next++),
              .display = pivot_default_value_label_(feature_name, op),
              .op = op,
          });
        }
      }
    }
  }

  PLUSS_CHECK(!plan.specs.empty(), std::string(where) + ": no compatible value aggregations resolved");
  plan.omit_value_prefix = plan.specs.size() == 1;
  return plan;
}

Slate pivot_make_note_slate_(const std::vector<std::string>& values,
                               const std::vector<bool>& valid) {
  const bool all_valid = std::all_of(valid.begin(), valid.end(), [](bool v) { return v; });
  return all_valid
      ? make_note_slate(values)
      : Slate(pluss::make_note(values), bool_series_from_keep(valid));
}

Slate pivot_fill_numeric_column_(const Slate& source,
                                  const SlateValue::HostAny& fill_value,
                                  bool integral_fill) {
  auto valid = detail::validity_mask(source)->to_vec<bool>();
  if (integral_fill) {
    auto values = source.as_series().to_vec<std::int64_t>();
    const auto fill = fill_value.to_int();
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (!valid[i]) {
        values[i] = fill;
        valid[i] = true;
      }
    }
    return make_i64_slate(values, valid);
  }
  auto values = source.as_series().to_vec<double>();
  const auto fill = fill_value.to_dbl();
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!valid[i]) {
      values[i] = fill;
      valid[i] = true;
    }
  }
  return make_f64_slate(values, valid);
}

Slate pivot_fill_note_column_(const Slate& source, const SlateValue::HostAny& fill_value) {
  const auto raw = source.as_note().vector();
  std::vector<std::string> values;
  values.reserve(raw.size());
  for (const auto token : raw) {
    values.emplace_back(token);
  }
  auto valid = detail::validity_mask(source)->to_vec<bool>();
  const auto fill = fill_value.to_str();
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!valid[i]) {
      values[i] = fill;
      valid[i] = true;
    }
  }
  return pivot_make_note_slate_(values, valid);
}

Slate pivot_make_duration_slate_(const std::vector<std::int64_t>& values,
                                   const std::vector<bool>& valid) {
  const auto duration =
      pluss::duration::nanos(values, {static_cast<std::int64_t>(values.size())});
  const bool all_valid =
      std::all_of(valid.begin(), valid.end(), [](const bool value) { return value; });
  return all_valid ? Slate(duration) : Slate(duration, bool_series_from_keep(valid));
}

Slate pivot_fill_duration_column_(
    const Slate& source,
    const SlateValue::HostAny& fill_value) {
  PLUSS_CHECK(fill_value.kind() == SlateValue::HostAny::Kind::Duration,
              "DataFrame::pivot_table: Duration output requires a Duration fill_value");
  const auto fill = fill_value.to_int();
  PLUSS_CHECK(fill != pluss::Duration::kNaT,
              "DataFrame::pivot_table: Duration fill_value cannot be NaT");
  auto values = source.as_duration().host();
  auto valid = detail::validity_mask(source)->to_vec<bool>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!valid[i]) {
      values[i] = fill;
      valid[i] = true;
    }
  }
  return pivot_make_duration_slate_(values, valid);
}

Slate pivot_apply_fill_(const Slate& source,
                         const std::optional<SlateValue::HostAny>& fill_value,
                         bool integral_fill) {
  if (!fill_value.has_value() || !detail::has_validity_mask(source)) {
    return source;
  }
  switch (source.kind()) {
    case SlateKind::Series:
      return pivot_fill_numeric_column_(source, *fill_value, integral_fill);
    case SlateKind::Note:
      return pivot_fill_note_column_(source, *fill_value);
    case SlateKind::Timeline:
      PLUSS_FAIL("DataFrame::pivot_table: fill_value does not support timeline-like pivot outputs");
    case SlateKind::Duration:
      return pivot_fill_duration_column_(source, *fill_value);
  }
  PLUSS_FAIL("DataFrame::pivot_table: unsupported pivot fill dispatch");
}

std::string pivot_level_key_(const Slate& key, std::int64_t row) {
  if (!detail::valid_at(key, row)) {
    return "0;";
  }
  const auto token = slate_scalar_string_at(key, row);
  return fmt::format("1{}:{};", token.size(), token);
}

std::string pivot_combo_key_(std::span<const Slate> keys, std::int64_t row) {
  std::string out;
  for (const auto& key : keys) {
    out += pivot_level_key_(key, row);
  }
  return out;
}

std::string pivot_feature_level_display_(const Slate& key, std::int64_t row) {
  return detail::valid_at(key, row) ? slate_scalar_string_at(key, row) : std::string("<NULL>");
}

std::string pivot_column_display_(std::span<const std::string> names,
                                  std::span<const Slate> keys,
                                  std::int64_t row,
                                  std::string_view separator) {
  PLUSS_CHECK(names.size() == keys.size(),
              "DataFrame::pivot_table: column metadata mismatch while building display labels");
  if (keys.empty()) {
    return "";
  }
  if (keys.size() == 1) {
    return pivot_feature_level_display_(keys.front(), row);
  }
  std::string label;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i != 0) {
      label += separator;
    }
    label += names[i];
    label += '=';
    label += pivot_feature_level_display_(keys[i], row);
  }
  return label;
}

DataFrame pivot_group_optional_keys_(const DataFrame& frame,
                                     std::span<const std::string> keys,
                                     std::span<const PivotResolvedSpec_> specs,
                                     bool dropna,
                                     bool sort,
                                     std::string_view where) {
  (void)where;
  std::vector<DataFrame::GroupSpec> group_specs;
  group_specs.reserve(specs.size());
  for (const auto& spec : specs) {
    group_specs.push_back(DataFrame::GroupSpec{spec.group_output, spec.feature, spec.op});
  }

  DataFrame::GroupOptions group_options;
  group_options.dropnull = dropna;
  group_options.sort = sort;

  if (!keys.empty()) {
    return frame.group(group_options)
        .by(keys)
        .agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(
            std::span<const DataFrame::GroupSpec>(group_specs.data(), group_specs.size())));
  }

  const std::string synthetic_name = "__pivot_all__";
  std::vector<std::pair<std::string, Slate>> columns;
  const auto names = frame.features();
  columns.reserve(names.size() + 1);
  for (const auto& name : names) {
    columns.emplace_back(name, frame.features(name));
  }
  columns.emplace_back(
      synthetic_name,
      make_note_slate(std::vector<std::string>(static_cast<std::size_t>(frame.ncases()), "__all__")));
  auto keyed = DataFrame(std::move(columns), frame.label().values()).title(frame.title());
  return keyed.group(group_options)
      .by({synthetic_name})
      .agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec>(
          std::span<const DataFrame::GroupSpec>(group_specs.data(), group_specs.size())));
}

DataFrameView DataFrame::pivot_table(std::initializer_list<std::string_view> index_features,
                                     std::initializer_list<std::string_view> column_features,
                                     std::initializer_list<std::string_view> value_features,
                                     GroupOp op,
                                     bool dropna,
                                     bool render,
                                     bool sort) const {
  return pivot_table(
      pluss::msharp::RangeView::Of<std::string_view>(index_features),
      pluss::msharp::RangeView::Of<std::string_view>(column_features),
      pluss::msharp::RangeView::Of<std::string_view>(value_features),
      op,
      dropna,
      render,
      sort);
}

DataFrameView DataFrame::pivot_table(pluss::msharp::RangeView::Of<std::string_view> index_features,
                                     pluss::msharp::RangeView::Of<std::string_view> column_features,
                                     pluss::msharp::RangeView::Of<std::string_view> value_features,
                                     GroupOp op,
                                     bool dropna,
                                     bool render,
                                     bool sort) const {
  PivotTableOptions options;
  options.index = normalize_name_list(index_features);
  options.columns = normalize_name_list(column_features);
  options.values = normalize_name_list(value_features);
  options.aggfuncs = {op};
  options.dropna = dropna;
  options.render = render;
  options.sort = sort;
  return pivot_table(options);
}

DataFrameView DataFrame::pivot_table(std::string_view case_feature,
                                     std::string_view feature_feature,
                                     std::string_view value_feature,
                                     GroupOp op,
                                     bool dropnull_keys,
                                     bool render) const {
  return pivot_table(
      pluss::msharp::RangeView::Of<std::string_view>({case_feature}),
      pluss::msharp::RangeView::Of<std::string_view>({feature_feature}),
      pluss::msharp::RangeView::Of<std::string_view>({value_feature}),
      op,
      dropnull_keys,
      render,
      true);
}

DataFrameView DataFrame::pivot_table(const PivotTableOptions& options) const {
  const auto plan = pivot_resolve_plan_(*this, options, "DataFrame::pivot_table");

  std::vector<std::string> group_keys = plan.index;
  group_keys.insert(group_keys.end(), plan.columns.begin(), plan.columns.end());
  auto grouped = pivot_group_optional_keys_(*this,
                                            std::span<const std::string>(group_keys.data(), group_keys.size()),
                                            std::span<const PivotResolvedSpec_>(plan.specs.data(), plan.specs.size()),
                                            options.dropna,
                                            options.sort,
                                            "DataFrame::pivot_table");

  std::vector<Slate> row_key_slates;
  std::vector<Slate> column_key_slates;
  row_key_slates.reserve(plan.index.size());
  column_key_slates.reserve(plan.columns.size());
  for (const auto& name : plan.index) {
    row_key_slates.push_back(grouped.features(name));
  }
  for (const auto& name : plan.columns) {
    column_key_slates.push_back(grouped.features(name));
  }

  std::unordered_map<std::string, std::size_t> row_map;
  std::unordered_map<std::string, std::size_t> col_map;
  std::vector<std::int64_t> row_first_rows;
  std::vector<std::int64_t> col_first_rows;
  std::vector<std::string> row_combo_keys;
  std::vector<std::string> col_combo_keys;
  std::vector<std::size_t> row_positions(static_cast<std::size_t>(grouped.ncases()), 0);
  std::vector<std::size_t> col_positions(static_cast<std::size_t>(grouped.ncases()), 0);
  row_map.reserve(static_cast<std::size_t>(std::max<std::int64_t>(1, grouped.ncases())));
  col_map.reserve(static_cast<std::size_t>(std::max<std::int64_t>(1, grouped.ncases())));

  if (grouped.ncases() == 0) {
    row_map.emplace("", 0);
    row_combo_keys.push_back("");
    if (plan.columns.empty()) {
      col_map.emplace("", 0);
      col_combo_keys.push_back("");
    }
  }

  for (std::int64_t row = 0; row < grouped.ncases(); ++row) {
    const auto row_key = plan.index.empty()
        ? std::string()
        : pivot_combo_key_(std::span<const Slate>(row_key_slates.data(), row_key_slates.size()), row);
    const auto [row_it, row_inserted] = row_map.emplace(row_key, row_map.size());
    if (row_inserted) {
      row_first_rows.push_back(row);
      row_combo_keys.push_back(row_key);
    }
    row_positions[static_cast<std::size_t>(row)] = row_it->second;

    const auto col_key = plan.columns.empty()
        ? std::string()
        : pivot_combo_key_(std::span<const Slate>(column_key_slates.data(), column_key_slates.size()), row);
    const auto [col_it, col_inserted] = col_map.emplace(col_key, col_map.size());
    if (col_inserted) {
      col_first_rows.push_back(row);
      col_combo_keys.push_back(col_key);
    }
    col_positions[static_cast<std::size_t>(row)] = col_it->second;
  }

  if (plan.columns.empty() && col_first_rows.empty()) {
    col_first_rows.push_back(0);
    col_combo_keys.push_back("");
  }

  auto column_combo_label = [&](std::size_t combo) {
    if (plan.columns.empty()) {
      return std::string{};
    }
    return pivot_column_display_(
        std::span<const std::string>(plan.columns.data(), plan.columns.size()),
        std::span<const Slate>(column_key_slates.data(), column_key_slates.size()),
        col_first_rows[combo],
        options.column_separator);
  };

  std::unordered_set<std::string> used_names(plan.index.begin(), plan.index.end());
  auto make_unique_name = [&](std::string base) {
    if (base.empty()) {
      base = "value";
    }
    if (used_names.insert(base).second) {
      return base;
    }
    std::string candidate;
    for (std::int64_t suffix = 1;; ++suffix) {
      candidate = fmt::format("{}_{}", base, suffix);
      if (used_names.insert(candidate).second) {
        return candidate;
      }
    }
  };

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(plan.index.size() + plan.specs.size() * std::max<std::size_t>(1, col_first_rows.size()));

  const auto out_rows = std::max<std::size_t>(row_first_rows.size(), row_map.empty() ? 0 : row_map.size());
  const auto out_col_groups = std::max<std::size_t>(col_first_rows.size(), col_map.empty() ? 0 : col_map.size());
  const bool add_margin_col = options.margins && !plan.columns.empty();
  const bool add_margin_row = options.margins && !plan.index.empty();
  const auto final_rows = out_rows + (add_margin_row ? 1 : 0);

  std::optional<DataFrame> row_totals = std::nullopt;
  std::optional<DataFrame> col_totals = std::nullopt;
  std::optional<DataFrame> grand_totals = std::nullopt;
  std::unordered_map<std::string, std::int64_t> row_total_source_row;
  std::unordered_map<std::string, std::int64_t> col_total_source_row;

  if (options.margins) {
    if (add_margin_col) {
      row_totals = pivot_group_optional_keys_(
          *this,
          std::span<const std::string>(plan.index.data(), plan.index.size()),
          std::span<const PivotResolvedSpec_>(plan.specs.data(), plan.specs.size()),
          options.dropna,
          options.sort,
          "DataFrame::pivot_table");
      std::vector<Slate> total_keys;
      total_keys.reserve(plan.index.size());
      for (const auto& name : plan.index) {
        total_keys.push_back(row_totals->features(name));
      }
      for (std::int64_t row = 0; row < row_totals->ncases(); ++row) {
        const auto key = plan.index.empty()
            ? std::string()
            : pivot_combo_key_(std::span<const Slate>(total_keys.data(), total_keys.size()), row);
        row_total_source_row.emplace(key, row);
      }
    }
    if (add_margin_row && !plan.columns.empty()) {
      col_totals = pivot_group_optional_keys_(
          *this,
          std::span<const std::string>(plan.columns.data(), plan.columns.size()),
          std::span<const PivotResolvedSpec_>(plan.specs.data(), plan.specs.size()),
          options.dropna,
          options.sort,
          "DataFrame::pivot_table");
      std::vector<Slate> total_keys;
      total_keys.reserve(plan.columns.size());
      for (const auto& name : plan.columns) {
        total_keys.push_back(col_totals->features(name));
      }
      for (std::int64_t row = 0; row < col_totals->ncases(); ++row) {
        const auto key = plan.columns.empty()
            ? std::string()
            : pivot_combo_key_(std::span<const Slate>(total_keys.data(), total_keys.size()), row);
        col_total_source_row.emplace(key, row);
      }
    }
    if (add_margin_row || add_margin_col) {
      grand_totals = pivot_group_optional_keys_(
          *this,
          {},
          std::span<const PivotResolvedSpec_>(plan.specs.data(), plan.specs.size()),
          options.dropna,
          options.sort,
          "DataFrame::pivot_table");
    }
  }

  for (std::size_t key_idx = 0; key_idx < plan.index.size(); ++key_idx) {
    const auto source = grouped.features(plan.index[key_idx]);
    std::vector<std::string> values(final_rows, "");
    std::vector<bool> valid(final_rows, false);
    for (std::size_t row_idx = 0; row_idx < out_rows; ++row_idx) {
      const auto src_row = row_first_rows[row_idx];
      if (!detail::valid_at(source, src_row)) {
        continue;
      }
      values[row_idx] = slate_scalar_string_at(source, src_row);
      valid[row_idx] = true;
    }
    if (add_margin_row) {
      values[out_rows] = key_idx == 0 ? options.margins_name : "";
      valid[out_rows] = true;
    }
    out_cols.emplace_back(plan.index[key_idx], pivot_make_note_slate_(values, valid));
  }

  for (const auto& spec : plan.specs) {
    const auto source = grouped.features(spec.group_output);
    const bool integral_output =
        spec.op == GroupOp::Count || spec.op == GroupOp::Ndistinct;
    for (std::size_t combo = 0; combo < std::max<std::size_t>(1, out_col_groups); ++combo) {
      const auto output_name = [&]() {
        if (plan.columns.empty()) {
          return make_unique_name(spec.display);
        }
        const auto combo_label = column_combo_label(combo);
        if (plan.omit_value_prefix) {
          return make_unique_name(combo_label);
        }
        return make_unique_name(spec.display + options.value_separator + combo_label);
      }();

      if (source.kind() == SlateKind::Note) {
        std::vector<std::string> values(final_rows, "");
        std::vector<bool> valid(final_rows, false);
        for (std::int64_t row = 0; row < grouped.ncases(); ++row) {
          if (col_positions[static_cast<std::size_t>(row)] != combo) {
            continue;
          }
          const auto out_row = row_positions[static_cast<std::size_t>(row)];
          if (!detail::valid_at(source, row)) {
            continue;
          }
          values[out_row] = slate_scalar_string_at(source, row);
          valid[out_row] = true;
        }
        if (add_margin_row) {
          if (plan.columns.empty()) {
            if (grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
              values[out_rows] = slate_scalar_string_at(grand_totals->features(spec.group_output), 0);
              valid[out_rows] = true;
            }
          } else {
            const auto it = col_total_source_row.find(col_combo_keys[combo]);
            if (it != col_total_source_row.end()) {
              const auto total_row = it->second;
              const auto total_source = col_totals->features(spec.group_output);
              if (detail::valid_at(total_source, total_row)) {
                values[out_rows] = slate_scalar_string_at(total_source, total_row);
                valid[out_rows] = true;
              }
            }
          }
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(pivot_make_note_slate_(values, valid), options.fill_value, false));
      } else if (source.kind() == SlateKind::Duration) {
        std::vector<std::int64_t> values(final_rows, 0);
        std::vector<bool> valid(final_rows, false);
        const auto source_values = source.as_duration().host();
        for (std::int64_t row = 0; row < grouped.ncases(); ++row) {
          if (col_positions[static_cast<std::size_t>(row)] != combo ||
              !detail::valid_at(source, row)) {
            continue;
          }
          const auto value = source_values[static_cast<std::size_t>(row)];
          if (value == pluss::Duration::kNaT) {
            continue;
          }
          const auto out_row = row_positions[static_cast<std::size_t>(row)];
          values[out_row] = value;
          valid[out_row] = true;
        }
        if (add_margin_row) {
          const auto total_source = plan.columns.empty()
                                        ? grand_totals->features(spec.group_output)
                                        : col_totals->features(spec.group_output);
          std::optional<std::int64_t> total_row;
          if (plan.columns.empty()) {
            if (grand_totals->ncases() > 0) {
              total_row = 0;
            }
          } else if (const auto it = col_total_source_row.find(col_combo_keys[combo]);
                     it != col_total_source_row.end()) {
            total_row = it->second;
          }
          if (total_row.has_value() && detail::valid_at(total_source, *total_row)) {
            const auto value = total_source.as_duration().host()[
                static_cast<std::size_t>(*total_row)];
            if (value != pluss::Duration::kNaT) {
              values[out_rows] = value;
              valid[out_rows] = true;
            }
          }
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(pivot_make_duration_slate_(values, valid),
                              options.fill_value,
                              false));
      } else if (integral_output) {
        std::vector<std::int64_t> values(final_rows, 0);
        std::vector<bool> valid(final_rows, false);
        const auto source_values = source.as_series().to_vec<std::int64_t>();
        for (std::int64_t row = 0; row < grouped.ncases(); ++row) {
          if (col_positions[static_cast<std::size_t>(row)] != combo || !detail::valid_at(source, row)) {
            continue;
          }
          const auto out_row = row_positions[static_cast<std::size_t>(row)];
          values[out_row] = source_values[static_cast<std::size_t>(row)];
          valid[out_row] = true;
        }
        if (add_margin_row) {
          if (plan.columns.empty()) {
            if (grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
              values[out_rows] = grand_totals->features(spec.group_output).as_series().to_vec<std::int64_t>().front();
              valid[out_rows] = true;
            }
          } else {
            const auto it = col_total_source_row.find(col_combo_keys[combo]);
            if (it != col_total_source_row.end()) {
              const auto total_row = it->second;
              const auto total_source = col_totals->features(spec.group_output);
              if (detail::valid_at(total_source, total_row)) {
                values[out_rows] = total_source.as_series().to_vec<std::int64_t>()[static_cast<std::size_t>(total_row)];
                valid[out_rows] = true;
              }
            }
          }
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(make_i64_slate(values, valid), options.fill_value, true));
      } else {
        std::vector<double> values(final_rows, std::numeric_limits<double>::quiet_NaN());
        std::vector<bool> valid(final_rows, false);
        const auto source_values = source.as_series().to_vec<double>();
        for (std::int64_t row = 0; row < grouped.ncases(); ++row) {
          if (col_positions[static_cast<std::size_t>(row)] != combo || !detail::valid_at(source, row)) {
            continue;
          }
          const auto out_row = row_positions[static_cast<std::size_t>(row)];
          values[out_row] = source_values[static_cast<std::size_t>(row)];
          valid[out_row] = true;
        }
        if (add_margin_row) {
          if (plan.columns.empty()) {
            if (grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
              values[out_rows] = grand_totals->features(spec.group_output).as_series().to_vec<double>().front();
              valid[out_rows] = true;
            }
          } else {
            const auto it = col_total_source_row.find(col_combo_keys[combo]);
            if (it != col_total_source_row.end()) {
              const auto total_row = it->second;
              const auto total_source = col_totals->features(spec.group_output);
              if (detail::valid_at(total_source, total_row)) {
                values[out_rows] = total_source.as_series().to_vec<double>()[static_cast<std::size_t>(total_row)];
                valid[out_rows] = true;
              }
            }
          }
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(make_f64_slate(values, valid), options.fill_value, false));
      }
    }

    if (add_margin_col) {
      const auto output_name = plan.omit_value_prefix
          ? make_unique_name(options.margins_name)
          : make_unique_name(spec.display + options.value_separator + options.margins_name);

      const auto total_source = row_totals->features(spec.group_output);
      if (total_source.kind() == SlateKind::Note) {
        std::vector<std::string> values(final_rows, "");
        std::vector<bool> valid(final_rows, false);
        for (std::size_t row_idx = 0; row_idx < out_rows; ++row_idx) {
          const auto it = row_total_source_row.find(row_combo_keys[row_idx]);
          if (it == row_total_source_row.end()) {
            continue;
          }
          if (!detail::valid_at(total_source, it->second)) {
            continue;
          }
          values[row_idx] = slate_scalar_string_at(total_source, it->second);
          valid[row_idx] = true;
        }
        if (add_margin_row && grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
          values[out_rows] = slate_scalar_string_at(grand_totals->features(spec.group_output), 0);
          valid[out_rows] = true;
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(pivot_make_note_slate_(values, valid), options.fill_value, false));
      } else if (total_source.kind() == SlateKind::Duration) {
        std::vector<std::int64_t> values(final_rows, 0);
        std::vector<bool> valid(final_rows, false);
        const auto total_values = total_source.as_duration().host();
        for (std::size_t row_idx = 0; row_idx < out_rows; ++row_idx) {
          const auto it = row_total_source_row.find(row_combo_keys[row_idx]);
          if (it == row_total_source_row.end() ||
              !detail::valid_at(total_source, it->second)) {
            continue;
          }
          const auto value = total_values[static_cast<std::size_t>(it->second)];
          if (value == pluss::Duration::kNaT) {
            continue;
          }
          values[row_idx] = value;
          valid[row_idx] = true;
        }
        if (add_margin_row && grand_totals->ncases() > 0) {
          const auto total = grand_totals->features(spec.group_output);
          if (detail::valid_at(total, 0)) {
            const auto value = total.as_duration().host().front();
            if (value != pluss::Duration::kNaT) {
              values[out_rows] = value;
              valid[out_rows] = true;
            }
          }
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(pivot_make_duration_slate_(values, valid),
                              options.fill_value,
                              false));
      } else if (integral_output) {
        std::vector<std::int64_t> values(final_rows, 0);
        std::vector<bool> valid(final_rows, false);
        const auto total_values = total_source.as_series().to_vec<std::int64_t>();
        for (std::size_t row_idx = 0; row_idx < out_rows; ++row_idx) {
          const auto it = row_total_source_row.find(row_combo_keys[row_idx]);
          if (it == row_total_source_row.end() || !detail::valid_at(total_source, it->second)) {
            continue;
          }
          values[row_idx] = total_values[static_cast<std::size_t>(it->second)];
          valid[row_idx] = true;
        }
        if (add_margin_row && grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
          values[out_rows] = grand_totals->features(spec.group_output).as_series().to_vec<std::int64_t>().front();
          valid[out_rows] = true;
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(make_i64_slate(values, valid), options.fill_value, true));
      } else {
        std::vector<double> values(final_rows, std::numeric_limits<double>::quiet_NaN());
        std::vector<bool> valid(final_rows, false);
        const auto total_values = total_source.as_series().to_vec<double>();
        for (std::size_t row_idx = 0; row_idx < out_rows; ++row_idx) {
          const auto it = row_total_source_row.find(row_combo_keys[row_idx]);
          if (it == row_total_source_row.end() || !detail::valid_at(total_source, it->second)) {
            continue;
          }
          values[row_idx] = total_values[static_cast<std::size_t>(it->second)];
          valid[row_idx] = true;
        }
        if (add_margin_row && grand_totals->ncases() > 0 && detail::valid_at(grand_totals->features(spec.group_output), 0)) {
          values[out_rows] = grand_totals->features(spec.group_output).as_series().to_vec<double>().front();
          valid[out_rows] = true;
        }
        out_cols.emplace_back(
            output_name,
            pivot_apply_fill_(make_f64_slate(values, valid), options.fill_value, false));
      }
    }
  }

  DataFrame out(std::move(out_cols));
  if (!plan.index.empty()) {
    DataFrame::SetLabelOptions set_options;
    set_options.features = plan.index;
    set_options.drop = true;
    set_options.separator = options.index_separator;
    out = out.label().set(set_options);
  }

  return DataFrameView(std::move(out).title(title()), options.render, "pivot_table", title());
}

ContingencyTable::SliceSpec::SliceSpec(std::string_view dim_name, std::string_view level)
    : dim(dim_name),
      value(SlateValue::HostAny::string(std::string(level))) {}

ContingencyTable::SliceSpec::SliceSpec(std::string_view dim_name, const char* level)
    : SliceSpec(dim_name, std::string_view{level != nullptr ? level : ""}) {}

ContingencyTable::SliceSpec::SliceSpec(std::string_view dim_name, bool level)
    : dim(dim_name),
      value(SlateValue::HostAny::boolean(level)) {}

ContingencyTable::SliceSpec::SliceSpec(std::string_view dim_name, std::int64_t level)
    : dim(dim_name),
      value(SlateValue::HostAny::integer(level)) {}

ContingencyTable::SliceSpec::SliceSpec(std::string_view dim_name, double level)
    : dim(dim_name),
      value(SlateValue::HostAny::floating(level)) {}

ContingencyTable::ContingencyTable() = default;

ContingencyTable::ContingencyTable(std::vector<std::string> dims,
                                   std::vector<Slate> levels,
                                   pluss::Series counts,
                                   DataFrame::ContingencyOptions options,
                                   std::vector<std::string> default_row_dims,
                                   std::vector<std::string> default_column_dims,
                                   Normalize normalize,
                                   std::vector<std::string> conditioned_dims)
    : dims_(std::move(dims)),
      levels_(std::move(levels)),
      counts_(std::move(counts)),
      options_(std::move(options)),
      default_row_dims_hint_(std::move(default_row_dims)),
      default_column_dims_hint_(std::move(default_column_dims)),
      normalize_(normalize),
      conditioned_dims_(std::move(conditioned_dims)) {}

std::int64_t ContingencyTable::ndim() const {
  return static_cast<std::int64_t>(dims_.size());
}

std::vector<std::int64_t> ContingencyTable::shape() const {
  std::vector<std::int64_t> out;
  out.reserve(levels_.size());
  for (const auto& level : levels_) {
    out.push_back(level.size());
  }
  return out;
}

std::vector<std::string> ContingencyTable::dims() const {
  return dims_;
}

std::size_t ContingencyTable::dim_index_(std::string_view dim, std::string_view where) const {
  const auto it = std::find(dims_.begin(), dims_.end(), owned_name(dim));
  PLUSS_CHECK(it != dims_.end(),
              fmt::format("{}: unknown contingency dimension '{}'", where, dim));
  return static_cast<std::size_t>(std::distance(dims_.begin(), it));
}

std::vector<std::size_t> ContingencyTable::dim_indices_(
    pluss::msharp::RangeView::Of<std::string_view> dims,
    std::string_view where) const {
  const auto names = normalize_name_list(dims);
  std::unordered_set<std::string> seen;
  std::vector<std::size_t> out;
  out.reserve(names.size());
  for (const auto& name : names) {
    PLUSS_CHECK(seen.insert(name).second,
                fmt::format("{}: duplicate dimension '{}'", where, name));
    out.push_back(dim_index_(name, where));
  }
  return out;
}

std::vector<std::string> ContingencyTable::default_row_dims_() const {
  auto rows = contingency_filtered_dims_(
      std::span<const std::string>(default_row_dims_hint_.data(), default_row_dims_hint_.size()),
      std::span<const std::string>(dims_.data(), dims_.size()));
  if (!rows.empty()) {
    return rows;
  }
  if (dims_.empty()) {
    return {};
  }
  return {dims_.front()};
}

std::vector<std::string> ContingencyTable::default_column_dims_() const {
  const auto rows = default_row_dims_();
  std::unordered_set<std::string> assigned(rows.begin(), rows.end());
  auto columns = contingency_filtered_dims_(
      std::span<const std::string>(default_column_dims_hint_.data(), default_column_dims_hint_.size()),
      std::span<const std::string>(dims_.data(), dims_.size()));
  if (!columns.empty()) {
    std::vector<std::string> filtered;
    filtered.reserve(columns.size());
    for (const auto& dim : columns) {
      if (!assigned.contains(dim)) {
        filtered.push_back(dim);
      }
    }
    if (!filtered.empty()) {
      return filtered;
    }
  }
  std::vector<std::string> out;
  out.reserve(dims_.size());
  for (const auto& dim : dims_) {
    if (!assigned.contains(dim)) {
      out.push_back(dim);
    }
  }
  return out;
}

std::vector<std::string> ContingencyTable::render_column_dims_(
    pluss::msharp::RangeView::Of<std::string_view> row_dims,
    pluss::msharp::RangeView::Of<std::string_view> column_dims) const {
  const auto rows = normalize_name_list(row_dims);
  const auto columns = normalize_name_list(column_dims);
  std::unordered_set<std::string> assigned;
  for (const auto& dim : rows) {
    PLUSS_CHECK(assigned.insert(dim).second,
                "ContingencyTable::to_frame: duplicate row dimension '" + dim + "'");
    (void)dim_index_(dim, "ContingencyTable::to_frame");
  }
  std::vector<std::string> out;
  out.reserve(dims_.size());
  for (const auto& dim : columns) {
    PLUSS_CHECK(!assigned.contains(dim),
                "ContingencyTable::to_frame: a dimension cannot be both a row and column dimension");
    PLUSS_CHECK(assigned.insert(dim).second,
                "ContingencyTable::to_frame: duplicate column dimension '" + dim + "'");
    (void)dim_index_(dim, "ContingencyTable::to_frame");
    out.push_back(dim);
  }
  for (const auto& dim : dims_) {
    if (!assigned.contains(dim)) {
      out.push_back(dim);
    }
  }
  return out;
}

std::int64_t ContingencyTable::level_index_(std::size_t dim_index,
                                            const SlateValue::HostAny& value,
                                            std::string_view where) const {
  const auto& levels = levels_.at(dim_index);
  for (std::int64_t index = 0; index < levels.size(); ++index) {
    if (contingency_host_equal_(contingency_level_host_(levels, index), value)) {
      return index;
    }
  }
  PLUSS_FAIL(fmt::format("{}: level '{}' was not found in dimension '{}'",
                         where,
                         value.to_str(),
                         dims_.at(dim_index)));
}

std::vector<std::string> ContingencyTable::conditioned_dims_after_drop_(std::size_t dim_index) const {
  std::vector<std::string> out;
  out.reserve(conditioned_dims_.size());
  for (const auto& dim : conditioned_dims_) {
    if (dim != dims_.at(dim_index)) {
      out.push_back(dim);
    }
  }
  return out;
}

ContingencyTable ContingencyTable::slice_at_(std::size_t dim_index, std::int64_t level_index) const {
  auto dims = dims_;
  auto levels = levels_;
  auto counts = counts_;
  counts = counts.select(static_cast<std::int64_t>(dim_index), level_index);
  dims.erase(dims.begin() + static_cast<std::ptrdiff_t>(dim_index));
  levels.erase(levels.begin() + static_cast<std::ptrdiff_t>(dim_index));
  auto rows = default_row_dims_hint_;
  auto cols = default_column_dims_hint_;
  rows.erase(std::remove(rows.begin(), rows.end(), dims_.at(dim_index)), rows.end());
  cols.erase(std::remove(cols.begin(), cols.end(), dims_.at(dim_index)), cols.end());
  return ContingencyTable(
      std::move(dims),
      std::move(levels),
      std::move(counts),
      options_,
      std::move(rows),
      std::move(cols),
      normalize_,
      conditioned_dims_after_drop_(dim_index));
}

pluss::Series ContingencyTable::materialized_values_() const {
  if (!counts_.defined()) {
    return pluss::series::undefined();
  }
  if (normalize_ == Normalize::None) {
    return counts_;
  }
  auto values = counts_.to(pluss::codem::ScalarType::Float64);
  if (values.numel() == 0) {
    return values;
  }
  if (normalize_ == Normalize::All) {
    const auto denom = values.sum().item<double>();
    if (denom == 0.0) {
      return pluss::Series::zeros(values.sizes(), pluss::codem::ScalarType::Float64, values.platform());
    }
    return values.div(denom);
  }

  std::unordered_set<std::string> conditioned(conditioned_dims_.begin(), conditioned_dims_.end());
  auto denom = values;
  for (std::int64_t axis = static_cast<std::int64_t>(dims_.size()) - 1; axis >= 0; --axis) {
    if (!conditioned.contains(dims_[static_cast<std::size_t>(axis)])) {
      denom = denom.sum(axis, true);
    }
  }
  auto expanded = denom.expand(values.sizes(), true);
  auto out = values.div(expanded.clamp_min(1.0));
  return out.masked_fill(expanded.eq(0.0), 0.0);
}

ContingencyTable ContingencyTable::freq() const {
  auto out = *this;
  out.normalize_ = Normalize::None;
  out.conditioned_dims_.clear();
  return out;
}

ContingencyTable ContingencyTable::share() const {
  auto out = *this;
  out.normalize_ = Normalize::All;
  out.conditioned_dims_.clear();
  return out;
}

ContingencyTable ContingencyTable::slice(std::string_view dim, std::string_view value) const {
  return slice({SliceSpec(dim, value)});
}

ContingencyTable ContingencyTable::slice(std::string_view dim, const char* value) const {
  return slice({SliceSpec(dim, value)});
}

ContingencyTable ContingencyTable::slice(std::string_view dim, bool value) const {
  return slice({SliceSpec(dim, value)});
}

ContingencyTable ContingencyTable::slice(std::string_view dim, std::int64_t value) const {
  return slice({SliceSpec(dim, value)});
}

ContingencyTable ContingencyTable::slice(std::string_view dim, double value) const {
  return slice({SliceSpec(dim, value)});
}

ContingencyTable ContingencyTable::slice(std::initializer_list<SliceSpec> specs) const {
  auto out = *this;
  for (const auto& spec : specs) {
    const auto axis = out.dim_index_(spec.dim, "ContingencyTable::slice");
    const auto level = out.level_index_(axis, spec.value, "ContingencyTable::slice");
    out = out.slice_at_(axis, level);
  }
  return out;
}

ContingencyTable ContingencyTable::marginalize(std::string_view dim) const {
  return marginalize(pluss::msharp::RangeView::Of<std::string_view>({dim}));
}

ContingencyTable ContingencyTable::marginalize(
    std::initializer_list<std::string_view> dims) const {
  return marginalize(pluss::msharp::RangeView::Of<std::string_view>(dims));
}

ContingencyTable ContingencyTable::marginalize(
    pluss::msharp::RangeView::Of<std::string_view> dims) const {
  auto axes = dim_indices_(dims, "ContingencyTable::marginalize");
  std::sort(axes.begin(), axes.end());
  std::reverse(axes.begin(), axes.end());

  auto out_dims = dims_;
  auto out_levels = levels_;
  auto out_counts = counts_;
  auto rows = default_row_dims_hint_;
  auto cols = default_column_dims_hint_;
  auto conditioned = conditioned_dims_;

  for (const auto axis : axes) {
    const auto& dim = out_dims[axis];
    out_counts = out_counts.sum(static_cast<std::int64_t>(axis), false);
    out_dims.erase(out_dims.begin() + static_cast<std::ptrdiff_t>(axis));
    out_levels.erase(out_levels.begin() + static_cast<std::ptrdiff_t>(axis));
    rows.erase(std::remove(rows.begin(), rows.end(), dim), rows.end());
    cols.erase(std::remove(cols.begin(), cols.end(), dim), cols.end());
    conditioned.erase(std::remove(conditioned.begin(), conditioned.end(), dim), conditioned.end());
  }

  return ContingencyTable(
      std::move(out_dims),
      std::move(out_levels),
      std::move(out_counts),
      options_,
      std::move(rows),
      std::move(cols),
      normalize_,
      std::move(conditioned));
}

ContingencyTable ContingencyTable::condition(std::string_view dim) const {
  return condition(pluss::msharp::RangeView::Of<std::string_view>({dim}));
}

ContingencyTable ContingencyTable::condition(
    std::initializer_list<std::string_view> dims) const {
  return condition(pluss::msharp::RangeView::Of<std::string_view>(dims));
}

ContingencyTable ContingencyTable::condition(
    pluss::msharp::RangeView::Of<std::string_view> dims) const {
  auto names = normalize_name_list(dims);
  std::unordered_set<std::string> seen;
  for (const auto& name : names) {
    PLUSS_CHECK(seen.insert(name).second,
                "ContingencyTable::condition: duplicate dimension '" + name + "'");
    (void)dim_index_(name, "ContingencyTable::condition");
  }
  auto out = *this;
  out.normalize_ = Normalize::Conditional;
  out.conditioned_dims_ = std::move(names);
  return out;
}

pluss::Series ContingencyTable::to_array() const {
  return materialized_values_();
}

DataFrame ContingencyTable::to_frame() const {
  return to_frame(options_);
}

DataFrame ContingencyTable::to_frame(const DataFrame::ContingencyOptions& options) const {
  const auto rows = default_row_dims_();
  const auto columns = default_column_dims_();
  return detail::with_stack_name_views(rows, columns, [&](auto row_names, auto col_names) {
    return this->to_frame(row_names, col_names, options);
  });
}

DataFrame ContingencyTable::to_frame(
    std::initializer_list<std::string_view> row_dims,
    std::initializer_list<std::string_view> column_dims) const {
  return to_frame(
      pluss::msharp::RangeView::Of<std::string_view>(row_dims),
      pluss::msharp::RangeView::Of<std::string_view>(column_dims),
      options_);
}

DataFrame ContingencyTable::to_frame(
    std::initializer_list<std::string_view> row_dims,
    std::initializer_list<std::string_view> column_dims,
    const DataFrame::ContingencyOptions& options) const {
  return to_frame(
      pluss::msharp::RangeView::Of<std::string_view>(row_dims),
      pluss::msharp::RangeView::Of<std::string_view>(column_dims),
      options);
}

DataFrame ContingencyTable::to_frame(
    pluss::msharp::RangeView::Of<std::string_view> row_dims,
    pluss::msharp::RangeView::Of<std::string_view> column_dims) const {
  return to_frame(row_dims, column_dims, options_);
}

DataFrame ContingencyTable::to_frame(
    pluss::msharp::RangeView::Of<std::string_view> row_dims,
    pluss::msharp::RangeView::Of<std::string_view> column_dims,
    const DataFrame::ContingencyOptions& options) const {
  const auto row_names = normalize_name_list(row_dims);
  const auto col_names = render_column_dims_(row_dims, column_dims);

  std::vector<std::int64_t> row_axes;
  row_axes.reserve(row_names.size());
  for (const auto& name : row_names) {
    row_axes.push_back(static_cast<std::int64_t>(dim_index_(name, "ContingencyTable::to_frame")));
  }
  std::vector<std::int64_t> col_axes;
  col_axes.reserve(col_names.size());
  for (const auto& name : col_names) {
    col_axes.push_back(static_cast<std::int64_t>(dim_index_(name, "ContingencyTable::to_frame")));
  }

  auto values = materialized_values_();
  std::vector<std::int64_t> permute_axes = row_axes;
  permute_axes.insert(permute_axes.end(), col_axes.begin(), col_axes.end());
  if (values.dim() > 0 && !permute_axes.empty()) {
    values = values.permute(std::span<const std::int64_t>(permute_axes.data(), permute_axes.size()));
  }

  const auto shape_now = shape();
  auto product_from_axes = [&](const std::vector<std::int64_t>& axes) -> std::int64_t {
    std::int64_t out = 1;
    for (const auto axis : axes) {
      out *= shape_now[static_cast<std::size_t>(axis)];
    }
    return out;
  };
  const auto row_count = row_axes.empty() ? 1 : product_from_axes(row_axes);
  const auto col_count = col_axes.empty() ? 1 : product_from_axes(col_axes);
  values = values.contiguous().reshape({row_count, col_count});
  const auto matrix = values.to(pluss::codem::Platform::CPU).contiguous();

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(row_names.size() + static_cast<std::size_t>(col_count) + (options.totals ? 1U : 0U));

  const auto extra_row = options.totals ? 1 : 0;
  const auto out_rows = static_cast<std::size_t>(row_count + extra_row);

  for (std::size_t i = 0; i < row_names.size(); ++i) {
    const auto axis = row_axes[i];
    const auto axis_size = shape_now[static_cast<std::size_t>(axis)];
    std::int64_t block = 1;
    for (std::size_t j = i + 1; j < row_axes.size(); ++j) {
      block *= shape_now[static_cast<std::size_t>(row_axes[j])];
    }
    std::vector<std::string> labels(out_rows);
    for (std::int64_t row = 0; row < row_count; ++row) {
      const auto level = axis_size == 0 ? 0 : (row / std::max<std::int64_t>(1, block)) % axis_size;
      labels[static_cast<std::size_t>(row)] = contingency_token_(
          levels_[static_cast<std::size_t>(axis)],
          level,
          options.null_label);
    }
    if (options.totals) {
      labels[out_rows - 1] = i == 0 ? std::string("Total") : std::string{};
    }
    out_cols.emplace_back(row_names[i], make_note_slate(labels));
  }

  std::unordered_set<std::string> used_names(row_names.begin(), row_names.end());
  auto make_unique_name = [&](std::string base) {
    if (base.empty()) {
      base = "value";
    }
    if (used_names.insert(base).second) {
      return base;
    }
    std::size_t suffix = 2;
    while (true) {
      auto candidate = fmt::format("{}_{}", base, suffix++);
      if (used_names.insert(candidate).second) {
        return candidate;
      }
    }
  };

  auto build_column_name = [&](std::int64_t column_index) {
    if (col_axes.empty()) {
      return make_unique_name(normalize_ == Normalize::None ? std::string("freq") : std::string("share"));
    }
    if (col_axes.size() == 1) {
      const auto axis = col_axes.front();
      return make_unique_name(contingency_token_(
          levels_[static_cast<std::size_t>(axis)],
          column_index,
          options.null_label));
    }
    std::vector<std::string> parts;
    parts.reserve(col_axes.size());
    std::int64_t residual = column_index;
    for (std::size_t i = 0; i < col_axes.size(); ++i) {
      std::int64_t block = 1;
      for (std::size_t j = i + 1; j < col_axes.size(); ++j) {
        block *= shape_now[static_cast<std::size_t>(col_axes[j])];
      }
      const auto axis = col_axes[i];
      const auto axis_size = shape_now[static_cast<std::size_t>(axis)];
      const auto level = axis_size == 0 ? 0 : (residual / std::max<std::int64_t>(1, block)) % axis_size;
      parts.push_back(fmt::format("{}={}",
                                  col_names[i],
                                  contingency_token_(levels_[static_cast<std::size_t>(axis)],
                                                     level,
                                                     options.null_label)));
    }
    std::string joined;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i != 0) {
        joined += options.separator;
      }
      joined += parts[i];
    }
    return make_unique_name(std::move(joined));
  };

  if (matrix.scalar_type() == pluss::codem::ScalarType::Int64) {
    const auto* raw = reinterpret_cast<const std::int64_t*>(matrix.raw_data());
    std::vector<std::int64_t> col_totals(static_cast<std::size_t>(col_count), 0);
    std::vector<std::int64_t> row_totals(static_cast<std::size_t>(row_count), 0);
    for (std::int64_t row = 0; row < row_count; ++row) {
      for (std::int64_t col = 0; col < col_count; ++col) {
        const auto value = raw[static_cast<std::size_t>(row * col_count + col)];
        row_totals[static_cast<std::size_t>(row)] += value;
        col_totals[static_cast<std::size_t>(col)] += value;
      }
    }
    const auto grand_total = std::accumulate(row_totals.begin(), row_totals.end(), std::int64_t{0});
    for (std::int64_t col = 0; col < col_count; ++col) {
      std::vector<std::int64_t> values_host(out_rows, 0);
      for (std::int64_t row = 0; row < row_count; ++row) {
        values_host[static_cast<std::size_t>(row)] =
            raw[static_cast<std::size_t>(row * col_count + col)];
      }
      if (options.totals) {
        values_host[out_rows - 1] = col_totals[static_cast<std::size_t>(col)];
      }
      out_cols.emplace_back(build_column_name(col), make_i64_slate(values_host));
    }
    if (options.totals) {
      std::vector<std::int64_t> values_host(out_rows, 0);
      for (std::int64_t row = 0; row < row_count; ++row) {
        values_host[static_cast<std::size_t>(row)] = row_totals[static_cast<std::size_t>(row)];
      }
      values_host[out_rows - 1] = grand_total;
      out_cols.emplace_back(make_unique_name("Total"), make_i64_slate(values_host));
    }
  } else {
    const auto* raw = reinterpret_cast<const double*>(matrix.raw_data());
    std::vector<double> col_totals(static_cast<std::size_t>(col_count), 0.0);
    std::vector<double> row_totals(static_cast<std::size_t>(row_count), 0.0);
    for (std::int64_t row = 0; row < row_count; ++row) {
      for (std::int64_t col = 0; col < col_count; ++col) {
        const auto value = raw[static_cast<std::size_t>(row * col_count + col)];
        row_totals[static_cast<std::size_t>(row)] += value;
        col_totals[static_cast<std::size_t>(col)] += value;
      }
    }
    const auto grand_total = std::accumulate(row_totals.begin(), row_totals.end(), 0.0);
    for (std::int64_t col = 0; col < col_count; ++col) {
      std::vector<double> values_host(out_rows, 0.0);
      for (std::int64_t row = 0; row < row_count; ++row) {
        values_host[static_cast<std::size_t>(row)] =
            raw[static_cast<std::size_t>(row * col_count + col)];
      }
      if (options.totals) {
        values_host[out_rows - 1] = col_totals[static_cast<std::size_t>(col)];
      }
      out_cols.emplace_back(build_column_name(col), make_f64_slate(values_host));
    }
    if (options.totals) {
      std::vector<double> values_host(out_rows, 0.0);
      for (std::int64_t row = 0; row < row_count; ++row) {
        values_host[static_cast<std::size_t>(row)] = row_totals[static_cast<std::size_t>(row)];
      }
      values_host[out_rows - 1] = grand_total;
      out_cols.emplace_back(make_unique_name("Total"), make_f64_slate(values_host));
    }
  }

  return DataFrame(std::move(out_cols));
}

Atlas ContingencyTable::facet() const {
  return facet(options_);
}

Atlas ContingencyTable::facet(const DataFrame::ContingencyOptions& options) const {
  const auto rows = default_row_dims_();
  const auto columns = default_column_dims_();
  return detail::with_stack_name_views(rows, columns, [&](auto row_names, auto col_names) {
    return this->facet(row_names, col_names, options);
  });
}

Atlas ContingencyTable::facet(
    std::initializer_list<std::string_view> row_dims,
    std::initializer_list<std::string_view> column_dims) const {
  return facet(
      pluss::msharp::RangeView::Of<std::string_view>(row_dims),
      pluss::msharp::RangeView::Of<std::string_view>(column_dims),
      options_);
}

Atlas ContingencyTable::facet(
    std::initializer_list<std::string_view> row_dims,
    std::initializer_list<std::string_view> column_dims,
    const DataFrame::ContingencyOptions& options) const {
  return facet(
      pluss::msharp::RangeView::Of<std::string_view>(row_dims),
      pluss::msharp::RangeView::Of<std::string_view>(column_dims),
      options);
}

Atlas ContingencyTable::facet(
    pluss::msharp::RangeView::Of<std::string_view> row_dims,
    pluss::msharp::RangeView::Of<std::string_view> column_dims) const {
  return facet(row_dims, column_dims, options_);
}

Atlas ContingencyTable::facet(
    pluss::msharp::RangeView::Of<std::string_view> row_dims,
    pluss::msharp::RangeView::Of<std::string_view> column_dims,
    const DataFrame::ContingencyOptions& options) const {
  const auto row_names = normalize_name_list(row_dims);
  const auto col_names = normalize_name_list(column_dims);
  std::unordered_set<std::string> assigned;
  for (const auto& name : row_names) {
    PLUSS_CHECK(assigned.insert(name).second,
                "ContingencyTable::facet: duplicate row dimension '" + name + "'");
    (void)dim_index_(name, "ContingencyTable::facet");
  }
  for (const auto& name : col_names) {
    PLUSS_CHECK(!assigned.contains(name),
                "ContingencyTable::facet: a dimension cannot be both a row and facet column dimension");
    PLUSS_CHECK(assigned.insert(name).second,
                "ContingencyTable::facet: duplicate column dimension '" + name + "'");
    (void)dim_index_(name, "ContingencyTable::facet");
  }

  std::vector<std::string> facet_dims;
  for (const auto& dim : dims_) {
    if (!assigned.contains(dim)) {
      facet_dims.push_back(dim);
    }
  }
  if (facet_dims.empty()) {
    return Atlas({to_frame(row_dims, column_dims, options)});
  }

  std::vector<std::int64_t> facet_axes;
  facet_axes.reserve(facet_dims.size());
  for (const auto& dim : facet_dims) {
    facet_axes.push_back(static_cast<std::int64_t>(dim_index_(dim, "ContingencyTable::facet")));
  }

  std::int64_t facet_count = 1;
  const auto current_shape = shape();
  for (const auto axis : facet_axes) {
    facet_count *= current_shape[static_cast<std::size_t>(axis)];
  }

  std::vector<DataFrame> frames;
  frames.reserve(static_cast<std::size_t>(facet_count));
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(facet_count));
  for (std::int64_t idx = 0; idx < facet_count; ++idx) {
    auto sliced = *this;
    std::vector<std::string> parts;
    parts.reserve(facet_dims.size());
    for (std::size_t i = 0; i < facet_dims.size(); ++i) {
      std::int64_t block = 1;
      for (std::size_t j = i + 1; j < facet_axes.size(); ++j) {
        block *= current_shape[static_cast<std::size_t>(facet_axes[j])];
      }
      const auto axis = static_cast<std::size_t>(facet_axes[i]);
      const auto axis_size = current_shape[axis];
      const auto level = axis_size == 0 ? 0 : (idx / std::max<std::int64_t>(1, block)) % axis_size;
      parts.push_back(fmt::format("{}={}",
                                  facet_dims[i],
                                  contingency_token_(levels_[axis], level, options.null_label)));
      sliced = sliced.slice_at_(sliced.dim_index_(facet_dims[i], "ContingencyTable::facet"), level);
    }
    std::string joined;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i != 0) {
        joined += options.separator;
      }
      joined += parts[i];
    }
    labels.push_back(std::move(joined));
    frames.push_back(sliced.to_frame(row_dims, column_dims, options));
  }
  return Atlas(std::move(frames), make_note_slate(labels));
}

DataFrame DataFrame::combine(const DataFrame& other) const {
  return combine(other, CombineSpec{});
}

DataFrame DataFrame::combine(const DataFrame& other, const CombineSpec& spec) const {
  FrameMatchOptions match_options;
  match_options.cases = spec.cases;
  match_options.features = FrameMatchRule::Unite;
  const auto matched = match(other, match_options);
  const auto& left = matched.left.impl_ref_();

  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(features().size() + other.features().size());
  out_cols.reserve(features().size() + other.features().size());
  for (const auto& name : features()) {
    out_names.push_back(name);
    out_cols.push_back(matched.left.features(name));
  }

  std::unordered_map<std::string, std::size_t> out_index;
  out_index.reserve(out_names.size());
  for (std::size_t i = 0; i < out_names.size(); ++i) {
    out_index.emplace(out_names[i], i);
  }

  auto next_unique_name = [&](std::string candidate) {
    if (out_index.find(candidate) == out_index.end()) {
      return candidate;
    }
    std::size_t suffix_id = 2;
    while (true) {
      const auto attempt = fmt::format("{}_{}", candidate, suffix_id);
      if (out_index.find(attempt) == out_index.end()) {
        return attempt;
      }
      ++suffix_id;
    }
  };

  for (const auto& right_name : other.features()) {
    const auto right_col = matched.right.features(right_name);

    const auto it = out_index.find(right_name);
    if (spec.op == CombineOp::Update) {
      if (it == out_index.end()) {
        continue;
      }
      const auto& left_col = out_cols[it->second];
      const auto write_mask = spec.overwrite
          ? right_col.notnull()
          : Slate(
                left_col.isnull().as_series().logical_and(
                    right_col.notnull().as_series()));
      out_cols[it->second] = left_col.put(write_mask, right_col);
      continue;
    }

    if (it == out_index.end()) {
      out_index.emplace(right_name, out_names.size());
      out_names.push_back(right_name);
      out_cols.push_back(right_col);
      continue;
    }

    PLUSS_CHECK(!spec.collision_suffix.empty(),
                "DataFrame::combine(features): collision_suffix must be non-empty when duplicate features exist");
    auto candidate = next_unique_name(right_name + spec.collision_suffix);
    out_index.emplace(candidate, out_names.size());
    out_names.push_back(std::move(candidate));
    out_cols.push_back(right_col);
  }

  std::vector<std::pair<std::string, Slate>> pairs;
  pairs.reserve(out_names.size());
  for (std::size_t i = 0; i < out_names.size(); ++i) {
    pairs.emplace_back(out_names[i], out_cols[i]);
  }
  return DataFrame(std::move(pairs), left.cases_);
}

Atlas DataFrame::unsqueeze() const {
  PLUSS_CHECK(defined(), "DataFrame::unsqueeze requires a defined DataFrame");
  return Atlas({*this});
}

Atlas DataFrame::unsqueeze(const Slate& depth_labels) const {
  PLUSS_CHECK(defined(), "DataFrame::unsqueeze requires a defined DataFrame");
  PLUSS_CHECK(depth_labels.defined(), "DataFrame::unsqueeze: depth labels are undefined");
  PLUSS_CHECK(depth_labels.size() == 1, "DataFrame::unsqueeze: depth labels must have size 1");
  return Atlas({*this}, depth_labels);
}
