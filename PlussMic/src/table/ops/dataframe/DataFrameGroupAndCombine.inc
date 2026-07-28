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
    if (spec.op == SummaryOp::Ndistinct) {
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
                           bool dropnull_keys,
                           bool render,
                           bool sort_groups) const {
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

  const auto state = build_group_state_(
      std::span<const Thread>(key_columns.data(), key_columns.size()),
      std::span<const std::string>(key_names.data(), key_names.size()),
      ncases(),
      ThreadGroupOptions{.dropnull = dropnull_keys, .sort = sort_groups},
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

    Thread out;
    if (!has_feature) {
      PLUSS_CHECK(spec.op == GroupOp::Count,
                  "DataFrame::group: feature is required for non-count aggregations");
      out = group_count_thread_(state);
    } else {
      const auto value_column = features(spec.feature);
      switch (spec.op) {
        case GroupOp::Count:
          out = group_count_thread_(state, value_column);
          break;
        case GroupOp::Ndistinct:
          out = group_ndistinct_thread_(value_column, state);
          break;
        case GroupOp::Mode:
          out = value_column.kind() == ThreadKind::Note
              ? group_note_mode_thread_(value_column, state)
              : group_numeric_thread_(value_column, state, spec.op);
          break;
        case GroupOp::Sum:
        case GroupOp::Mean:
        case GroupOp::Median:
        case GroupOp::Min:
        case GroupOp::Max:
        case GroupOp::Std:
        case GroupOp::Var:
          out = group_numeric_thread_(value_column, state, spec.op);
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

double median_from_values_(std::vector<double>& values) {
  PLUSS_CHECK(!values.empty(), "DataFrame::pivot_table median: empty value group");
  const auto n = values.size();
  const auto mid = n / 2;
  std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid), values.end());
  const auto upper = values[mid];
  if (n % 2 != 0) {
    return upper;
  }
  std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid - 1), values.end());
  return (values[mid - 1] + upper) / 2.0;
}

DataFrameView DataFrame::pivot_table(std::string_view case_feature,
                                     std::string_view feature_feature,
                                     std::string_view value_feature,
                                     GroupOp op,
                                     bool dropnull_keys,
                                     bool render) const {
  PLUSS_CHECK(op == GroupOp::Count || op == GroupOp::Ndistinct || op == GroupOp::Sum || op == GroupOp::Mean ||
                  op == GroupOp::Median || op == GroupOp::Min || op == GroupOp::Max,
              "DataFrame::pivot_table: unsupported aggregation op");

  const auto case_col = features(case_feature);
  const auto feat_col = features(feature_feature);
  const auto value_col = features(value_feature);

  const bool needs_numeric = (op != GroupOp::Count && op != GroupOp::Ndistinct);
  if (needs_numeric) {
    PLUSS_CHECK(value_col.kind() == ThreadKind::Series && is_number_series_type(value_col.as_series()),
                "DataFrame::pivot_table: numeric aggregations require a numeric Series value feature");
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
    std::vector<double> values{};
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
    if (dropnull_keys && (!case_valid || !feat_valid)) {
      continue;
    }

    const auto case_key = case_valid ? thread_scalar_string_at(case_vals, r) : std::string("NULL");
    const auto feat_key = feat_valid ? thread_scalar_string_at(feat_vals, r) : std::string("NULL");
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
    if (op == GroupOp::Ndistinct) {
      agg.uniq.insert(thread_scalar_string_at(value_vals, r));
      continue;
    }

    PLUSS_CHECK(value_numeric_ptr != nullptr,
                "DataFrame::pivot_table: numeric cache unavailable for numeric aggregation");
    const double x = value_numeric_ptr[r];
    agg.count += 1;
    agg.sum += x;
    if (op == GroupOp::Median) {
      agg.values.push_back(x);
    }
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
    if (op == GroupOp::Count || op == GroupOp::Ndistinct) {
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
      } else if (op == GroupOp::Median && !agg.values.empty()) {
        auto values = agg.values;
        vals[r] = median_from_values_(values);
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
