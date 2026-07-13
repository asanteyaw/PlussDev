DataFrame DataFrame::summarize_specs(const std::vector<SummarySpec>& specs, bool render) const {
  PLUSS_CHECK(!specs.empty(), "DataFrame::summarize: no SummarySpec supplied");

  std::vector<std::pair<std::string, Thread>> out_cols;
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
      out_cols.emplace_back(out_name, make_i64_thread({non_null_count(feature_value)}));
      continue;
    }
    if (spec.op == SummaryOp::Nunique) {
      out_cols.emplace_back(out_name, make_i64_thread({distinct_non_null_count(feature_value)}));
      continue;
    }
    out_cols.emplace_back(out_name, make_f64_thread({numeric_reduce_value(feature_value, spec.op)}));
  }

  (void)render;
  return DataFrame(std::move(out_cols), make_note_thread({"summary"}));
}

DataFrame DataFrame::summarize(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render) const {
  return summarize_specs(std::vector<SummarySpec>(specs.begin(), specs.end()), render);
}

DataFrameView DataFrame::aggregate(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render) const {
  return DataFrameView(summarize(specs, false), render, "aggregate", title());
}

DataFrame DataFrame::group(pluss::msharp::RangeView::Of<std::string_view> keys,
                           pluss::msharp::RangeView::Of<GroupSpec> specs,
                           bool dropna_keys,
                           bool render) const {
  PLUSS_CHECK(keys.size() != 0, "DataFrame::group: at least one key feature is required");
  PLUSS_CHECK(specs.size() != 0, "DataFrame::group: at least one GroupSpec is required");

  const auto key_names = normalize_name_list(keys);
  std::unordered_set<std::string> key_seen;
  key_seen.reserve(key_names.size());
  std::vector<Thread> key_columns;
  key_columns.reserve(key_names.size());
  for (const auto& key : key_names) {
    PLUSS_CHECK(key_seen.insert(key).second, "DataFrame::group: duplicate key '" + key + "'");
    key_columns.push_back(features(key));
  }

  auto composite_keys = make_unify_keys(key_columns, ncases());
  std::vector<bool> included(static_cast<std::size_t>(ncases()), true);
  if (dropna_keys) {
    for (const auto& key : key_columns) {
      if (!key.has_validity_mask()) {
        continue;
      }
      const auto valid = key.validity_mask()->to_vec<bool>();
      std::transform(
          included.begin(),
          included.end(),
          valid.begin(),
          included.begin(),
          std::logical_and<bool>{});
    }
  }

  std::vector<std::int64_t> group_ids(static_cast<std::size_t>(ncases()), -1);
  std::vector<std::int64_t> first_rows;
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, ncases() / 8)));
  for (std::int64_t row = 0; row < ncases(); ++row) {
    if (!included[static_cast<std::size_t>(row)]) {
      continue;
    }
    const auto [found, inserted] = index_by_key.emplace(
        composite_keys[static_cast<std::size_t>(row)],
        first_rows.size());
    if (inserted) {
      first_rows.push_back(row);
    }
    group_ids[static_cast<std::size_t>(row)] =
        static_cast<std::int64_t>(found->second);
  }

  const auto group_count = static_cast<std::int64_t>(first_rows.size());
  auto group_ids_cpu = pluss::Series::empty(
      {ncases()},
      pluss::codem::ScalarType::Int64,
      pluss::codem::Platform::CPU);
  if (!group_ids.empty()) {
    std::memcpy(
        group_ids_cpu.raw_data(),
        group_ids.data(),
        group_ids.size() * sizeof(std::int64_t));
  }
  const auto included_cpu =
      bool_mask_from_values(included, pluss::codem::Platform::CPU);

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(key_columns.size() + specs.size());
  for (std::size_t key = 0; key < key_columns.size(); ++key) {
    out_cols.emplace_back(
        key_names[key],
        key_columns[key].take(first_rows));
  }

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
            : (spec.op == GroupOp::Count && !has_feature
                   ? std::string("count")
                   : fmt::format("{}_{}", spec.feature, group_op_name(spec.op)));
    PLUSS_CHECK(
        out_seen.insert(out_name).second,
        "DataFrame::group: duplicate output feature '" + out_name + "'");

    Thread value_column;
    if (has_feature) {
      value_column = features(spec.feature);
    }

    if (spec.op == GroupOp::Nunique) {
      const auto parts = unify_key_parts(value_column);
      const auto valid = value_column.has_validity_mask()
          ? value_column.validity_mask()->to_vec<bool>()
          : std::vector<bool>(static_cast<std::size_t>(ncases()), true);
      std::vector<std::int64_t> counts(
          static_cast<std::size_t>(group_count),
          0);
      std::unordered_set<std::string> observed;
      observed.reserve(static_cast<std::size_t>(ncases()));
      for (std::int64_t row = 0; row < ncases(); ++row) {
        if (!included[static_cast<std::size_t>(row)] ||
            !valid[static_cast<std::size_t>(row)]) {
          continue;
        }
        std::string key(
            reinterpret_cast<const char*>(
                &group_ids[static_cast<std::size_t>(row)]),
            sizeof(std::int64_t));
        key.append(parts[static_cast<std::size_t>(row)]);
        if (observed.insert(std::move(key)).second) {
          ++counts[static_cast<std::size_t>(
              group_ids[static_cast<std::size_t>(row)])];
        }
      }
      out_cols.emplace_back(out_name, make_i64_thread(counts));
      continue;
    }

    pluss::codem::Platform platform = pluss::codem::Platform::CPU;
    if (has_feature && value_column.kind() == ThreadKind::Series) {
      platform = value_column.as_series().platform();
    }
    auto active = platform == pluss::codem::Platform::CPU
        ? included_cpu
        : included_cpu.to(platform);
    if (has_feature && value_column.has_validity_mask()) {
      auto valid = *value_column.validity_mask();
      if (valid.platform() != platform) {
        valid = valid.to(platform);
      }
      active = active.logical_and(valid);
    }
    auto ids = platform == pluss::codem::Platform::CPU
        ? group_ids_cpu
        : group_ids_cpu.to(platform);
    ids = ids.masked_select(active);

    const auto compute_dtype =
        platform == pluss::codem::Platform::CPU
            ? pluss::codem::ScalarType::Float64
            : pluss::codem::ScalarType::Float32;
    auto counts = pluss::Series::zeros(
        {group_count},
        compute_dtype,
        platform);
    auto ones = pluss::Series::ones(
        {ids.numel()},
        compute_dtype,
        platform);
    counts = counts.scatter_add(0, ids, ones);

    if (spec.op == GroupOp::Count) {
      out_cols.emplace_back(
          out_name,
          Thread(counts.to(pluss::codem::ScalarType::Int64)));
      continue;
    }

    PLUSS_CHECK(
        value_column.kind() == ThreadKind::Series &&
            is_numeric_series_type(value_column.as_series()),
        "DataFrame::group: numeric aggregations require real numeric Series features");
    auto values = value_column.as_series().to(compute_dtype).masked_select(active);
    auto sums = pluss::Series::zeros(
        {group_count},
        compute_dtype,
        platform).scatter_add(0, ids, values);
    const auto zero_counts = pluss::Series::zeros(
        {group_count},
        compute_dtype,
        platform);
    const auto any_value = counts.gt(zero_counts);

    pluss::Series result;
    pluss::Series result_validity = any_value;
    switch (spec.op) {
      case GroupOp::Sum:
        result = sums;
        break;
      case GroupOp::Mean:
        result = sums / counts;
        break;
      case GroupOp::Min:
        result = pluss::Series::full(
            {group_count},
            std::numeric_limits<double>::infinity(),
            compute_dtype,
            platform).scatter_reduce(0, ids, values, "amin", true);
        break;
      case GroupOp::Max:
        result = pluss::Series::full(
            {group_count},
            -std::numeric_limits<double>::infinity(),
            compute_dtype,
            platform).scatter_reduce(0, ids, values, "amax", true);
        break;
      case GroupOp::Std:
      case GroupOp::Var: {
        auto squared_sums = pluss::Series::zeros(
            {group_count},
            compute_dtype,
            platform).scatter_add(0, ids, values * values);
        result = (
            (squared_sums - (sums * sums / counts)) /
            (counts - 1.0)).clamp_min(0.0);
        const auto one_counts = pluss::Series::ones(
            {group_count},
            compute_dtype,
            platform);
        result_validity = counts.gt(one_counts);
        if (spec.op == GroupOp::Std) {
          result = result.sqrt();
        }
        break;
      }
      case GroupOp::Count:
      case GroupOp::Nunique:
        PLUSS_FAIL("DataFrame::group: internal aggregation dispatch failure");
    }
    out_cols.emplace_back(
        out_name,
        Thread(result, result_validity));
  }

  (void)render;
  return DataFrame(std::move(out_cols)).title(title());
}

DataFrameView DataFrame::group_by(pluss::msharp::RangeView::Of<std::string_view> keys,
                                  pluss::msharp::RangeView::Of<GroupSpec> specs,
                                  bool dropna_keys,
                                  bool render) const {
  return DataFrameView(group(keys, specs, dropna_keys, false), render, "group_by", title());
}

DataFrameView DataFrame::group_by(std::initializer_list<std::string_view> keys,
                                  pluss::msharp::RangeView::Of<GroupSpec> specs,
                                  bool dropna_keys,
                                  bool render) const {
  return group_by(pluss::msharp::RangeView::Of<std::string_view>(keys),
                  specs,
                  dropna_keys,
                  render);
}

DataFrameView DataFrame::group_by(const CaseGroup& keys,
                                  pluss::msharp::RangeView::Of<GroupSpec> specs,
                                  bool dropna_keys,
                                  bool render) const {
  PLUSS_CHECK(has_levels(), "DataFrame::group_by(CaseGroup): current DataFrame has no case levels");
  std::vector<std::string> level_names = keys.levels;
  if (!keys.level.empty()) {
    PLUSS_CHECK(level_names.empty(),
                "DataFrame::group_by(CaseGroup): use either `level` or `levels`, not both");
    level_names.push_back(keys.level);
  }
  PLUSS_CHECK(!level_names.empty(),
              "DataFrame::group_by(CaseGroup): expected at least one case level");

  auto keyed = *this;
  std::vector<std::string> temp_names;
  temp_names.reserve(level_names.size());
  for (const auto& level_name : level_names) {
    PLUSS_CHECK(!level_name.empty(),
                "DataFrame::group_by(CaseGroup): level name cannot be empty");
    auto temp = "__pluss_case_level_" + level_name;
    std::int64_t suffix = 1;
    while (keyed.has_feature(temp)) {
      temp = fmt::format("__pluss_case_level_{}_{}", level_name, suffix++);
    }
    auto level_values = level(level_name);
    if (detail::has_display_labels(level_values)) {
      level_values = detail::clear_display_labels(level_values);
    }
    if (level_values.has_display_name()) {
      level_values = level_values.clear_display_name();
    }
    set_prepared_feature_(keyed.mut_impl_(), temp, clear_assignment_metadata_(level_values));
    temp_names.push_back(std::move(temp));
  }

  std::vector<std::string_view> temp_views;
  temp_views.reserve(temp_names.size());
  for (const auto& temp : temp_names) {
    temp_views.push_back(temp);
  }
  auto grouped = keyed.group(
      pluss::msharp::RangeView::Of<std::string_view>(
          std::span<const std::string_view>(temp_views.data(), temp_views.size())),
      specs,
      dropna_keys,
      false);

  std::vector<std::pair<std::string_view, std::string_view>> renames;
  renames.reserve(level_names.size());
  for (std::size_t i = 0; i < level_names.size(); ++i) {
    renames.emplace_back(temp_names[i], level_names[i]);
  }
  grouped = grouped.rename(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(
      std::span<const std::pair<std::string_view, std::string_view>>(renames.data(), renames.size())));
  return DataFrameView(std::move(grouped), render, "group_by", title());
}

DataFrameView DataFrame::pivot(std::string_view case_feature,
                               std::string_view feature_feature,
                               std::string_view value_feature,
                               bool dropna_keys,
                               bool render) const {
  const auto dup = group(
      {case_feature, feature_feature},
      {GroupSpec{"__pivot_cell_count__", "", GroupOp::Count}},
      dropna_keys,
      false);
  const auto counts = dup.features("__pivot_cell_count__").as_series();
  const bool has_duplicate = counts.gt(1).any().item<bool>();
  if (has_duplicate) {
    throw ::pluss::Error(
        "DataFrame::pivot: duplicate case/feature pairs found; use pivot_table with an aggregation op");
  }
  auto out = pivot_table(case_feature, feature_feature, value_feature, GroupOp::Max, dropna_keys, false);
  return DataFrameView(static_cast<DataFrame>(out), render, "pivot", title());
}

DataFrameView DataFrame::pivot_table(std::string_view case_feature,
                                     std::string_view feature_feature,
                                     std::string_view value_feature,
                                     GroupOp op,
                                     bool dropna_keys,
                                     bool render) const {
  PLUSS_CHECK(op == GroupOp::Count || op == GroupOp::Nunique || op == GroupOp::Sum || op == GroupOp::Mean ||
                  op == GroupOp::Min || op == GroupOp::Max,
              "DataFrame::pivot_table: unsupported aggregation op");

  const auto case_col = features(case_feature);
  const auto feat_col = features(feature_feature);
  const auto value_col = features(value_feature);

  const bool needs_numeric = (op != GroupOp::Count && op != GroupOp::Nunique);
  if (needs_numeric) {
    PLUSS_CHECK(value_col.kind() == ThreadKind::Series && is_number_series_type(value_col.as_series()),
                "DataFrame::pivot_table: Sum/Mean/Min/Max require a numeric Series value feature");
  }

  const auto case_vals = case_col.has_validity_mask() ? case_col.clear_validity_mask() : case_col;
  const auto feat_vals = feat_col.has_validity_mask() ? feat_col.clear_validity_mask() : feat_col;
  const auto value_vals = value_col.has_validity_mask() ? value_col.clear_validity_mask() : value_col;
  std::optional<pluss::Series> value_numeric = std::nullopt;
  const double* value_numeric_ptr = nullptr;
  if (needs_numeric) {
    value_numeric = value_col.as_series().to(pluss::codem::ScalarType::Float64);
    value_numeric_ptr = reinterpret_cast<const double*>(value_numeric->raw_data());
  }

  struct PivotAgg {
    std::int64_t count{0};
    double sum{0.0};
    double min{0.0};
    double max{0.0};
    bool has_numeric{false};
    std::unordered_set<std::string> uniq{};
  };

  std::unordered_map<std::string, std::size_t> case_to_row;
  std::unordered_map<std::string, std::size_t> feat_to_col;
  std::vector<std::string> case_labels;
  std::vector<std::string> feat_labels;
  std::vector<std::vector<PivotAgg>> grid;

  case_to_row.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, ncases() / 64)));
  feat_to_col.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, ncases() / 64)));

  auto ensure_case = [&](const std::string& key) -> std::size_t {
    const auto [it, inserted] = case_to_row.emplace(key, case_labels.size());
    if (inserted) {
      case_labels.push_back(key);
      grid.emplace_back(feat_labels.size());
    }
    return it->second;
  };

  auto ensure_feature = [&](const std::string& key) -> std::size_t {
    const auto [it, inserted] = feat_to_col.emplace(key, feat_labels.size());
    if (inserted) {
      feat_labels.push_back(key);
      for (auto& row : grid) {
        row.emplace_back();
      }
    }
    return it->second;
  };
  for (std::int64_t r = 0; r < ncases(); ++r) {
    const bool case_valid = case_col.is_valid(r);
    const bool feat_valid = feat_col.is_valid(r);
    if (dropna_keys && (!case_valid || !feat_valid)) {
      continue;
    }

    const auto case_key = case_valid ? thread_scalar_string_at(case_vals, r) : std::string("NA");
    const auto feat_key = feat_valid ? thread_scalar_string_at(feat_vals, r) : std::string("NA");
    const auto row_idx = ensure_case(case_key);
    const auto col_idx = ensure_feature(feat_key);
    auto& agg = grid[row_idx][col_idx];

    if (!value_col.is_valid(r)) {
      continue;
    }

    if (op == GroupOp::Count) {
      agg.count += 1;
      continue;
    }
    if (op == GroupOp::Nunique) {
      agg.uniq.insert(thread_scalar_string_at(value_vals, r));
      continue;
    }

    PLUSS_CHECK(value_numeric_ptr != nullptr,
                "DataFrame::pivot_table: numeric cache unavailable for numeric aggregation");
    const double x = value_numeric_ptr[r];
    agg.count += 1;
    agg.sum += x;
    if (!agg.has_numeric) {
      agg.min = x;
      agg.max = x;
      agg.has_numeric = true;
    } else {
      agg.min = std::min(agg.min, x);
      agg.max = std::max(agg.max, x);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(feat_labels.size());
  for (std::size_t c = 0; c < feat_labels.size(); ++c) {
    if (op == GroupOp::Count || op == GroupOp::Nunique) {
      std::vector<std::int64_t> vals;
      vals.reserve(case_labels.size());
      for (std::size_t r = 0; r < case_labels.size(); ++r) {
        const auto& agg = grid[r][c];
        vals.push_back(op == GroupOp::Count ? agg.count : static_cast<std::int64_t>(agg.uniq.size()));
      }
      out_cols.emplace_back(feat_labels[c], make_i64_thread(vals));
      continue;
    }

    std::vector<double> vals(case_labels.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<bool> valid(case_labels.size(), false);
    for (std::size_t r = 0; r < case_labels.size(); ++r) {
      const auto& agg = grid[r][c];
      if (op == GroupOp::Sum && agg.count > 0) {
        vals[r] = agg.sum;
        valid[r] = true;
      } else if (op == GroupOp::Mean && agg.count > 0) {
        vals[r] = agg.sum / static_cast<double>(agg.count);
        valid[r] = true;
      } else if (op == GroupOp::Min && agg.has_numeric) {
        vals[r] = agg.min;
        valid[r] = true;
      } else if (op == GroupOp::Max && agg.has_numeric) {
        vals[r] = agg.max;
        valid[r] = true;
      }
    }
    out_cols.emplace_back(feat_labels[c], make_f64_thread(vals, valid));
  }
  auto out = DataFrame(std::move(out_cols), make_note_thread(case_labels));
  return DataFrameView(std::move(out), render, "pivot_table", title());
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
  std::vector<Thread> out_cols;
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
          : Thread(
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

  std::vector<std::pair<std::string, Thread>> pairs;
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

Atlas DataFrame::unsqueeze(const Thread& depth_labels) const {
  PLUSS_CHECK(defined(), "DataFrame::unsqueeze requires a defined DataFrame");
  PLUSS_CHECK(depth_labels.defined(), "DataFrame::unsqueeze: depth labels are undefined");
  PLUSS_CHECK(depth_labels.size() == 1, "DataFrame::unsqueeze: depth labels must have size 1");
  return Atlas({*this}, depth_labels);
}

DataFrameView DataFrame::count_by(std::string_view feature_name,
                                  std::int64_t top_n,
                                  bool dropna,
                                  bool render) const {
  auto grouped = group({feature_name}, {GroupSpec{"count", "", GroupOp::Count}}, dropna, false);
  const auto keys_note = grouped.features(feature_name).as_note();
  const auto counts = grouped.features("count").as_series().to_vec<std::int64_t>();

  std::vector<std::pair<std::string, std::int64_t>> rows;
  rows.reserve(static_cast<std::size_t>(grouped.ncases()));
  for (std::int64_t i = 0; i < grouped.ncases(); ++i) {
    rows.emplace_back(
        std::string(keys_note.at(i)),
        counts[static_cast<std::size_t>(i)]);
  }
  std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) {
      return a.second > b.second;
    }
    return a.first < b.first;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::string> keys;
  std::vector<std::int64_t> vals;
  keys.reserve(rows.size());
  vals.reserve(rows.size());
  for (const auto& [k, c] : rows) {
    keys.push_back(k);
    vals.push_back(c);
  }

  auto out = DataFrame({
      {owned_name(feature_name), make_note_thread(keys)},
      {"count", make_i64_thread(vals)},
  });
  return DataFrameView(std::move(out), render, "count_by", title());
}

DataFrameView DataFrame::mean_by(std::string_view group_feature_name,
                                 std::string_view value_feature_name,
                                 std::int64_t top_n,
                                 std::int64_t min_count,
                                 bool dropna_group,
                                 bool render) const {
  PLUSS_CHECK(min_count >= 1, "DataFrame::mean_by: min_count must be >= 1");
  const auto mean_name = fmt::format("mean_{}", value_feature_name);
  auto grouped = group(
      {group_feature_name},
      {
          GroupSpec{mean_name, owned_name(value_feature_name), GroupOp::Mean},
          GroupSpec{"count", owned_name(value_feature_name), GroupOp::Count},
      },
      dropna_group,
      false);

  struct MeanRow {
    std::string key;
    double mean{0.0};
    std::int64_t count{0};
  };
  const auto groups_note = grouped.features(group_feature_name).as_note();
  const auto grouped_means = grouped.features(mean_name).as_series().to_vec<double>();
  const auto grouped_counts = grouped.features("count").as_series().to_vec<std::int64_t>();

  std::vector<MeanRow> rows;
  rows.reserve(static_cast<std::size_t>(grouped.ncases()));
  for (std::int64_t i = 0; i < grouped.ncases(); ++i) {
    const auto cnt = grouped_counts[static_cast<std::size_t>(i)];
    if (cnt < min_count) {
      continue;
    }
    rows.push_back({
        std::string(groups_note.at(i)),
        grouped_means[static_cast<std::size_t>(i)],
        cnt,
    });
  }

  std::sort(rows.begin(), rows.end(), [](const MeanRow& a, const MeanRow& b) {
    if (a.mean != b.mean) {
      return a.mean > b.mean;
    }
    return a.key < b.key;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::string> out_groups;
  std::vector<double> out_means;
  std::vector<std::int64_t> out_counts;
  out_groups.reserve(rows.size());
  out_means.reserve(rows.size());
  out_counts.reserve(rows.size());
  for (const auto& row : rows) {
    out_groups.push_back(row.key);
    out_means.push_back(row.mean);
    out_counts.push_back(row.count);
  }

  auto out = DataFrame({
      {owned_name(group_feature_name), make_note_thread(out_groups)},
      {fmt::format("mean_{}", value_feature_name), make_f64_thread(out_means)},
      {"count", make_i64_thread(out_counts)},
  });
  return DataFrameView(std::move(out), render, "mean_by", title());
}
