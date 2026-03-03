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
      out_cols.emplace_back(out_name, make_i64_thread({nunique_non_null(feature_value)}));
      continue;
    }
    out_cols.emplace_back(out_name, make_f64_thread({numeric_reduce_value(feature_value, spec.op)}));
  }

  (void)render;
  return DataFrame(std::move(out_cols), make_note_thread({"summary"}));
}

DataFrame DataFrame::summarize(std::initializer_list<SummarySpec> specs, bool render) const {
  return summarize_specs(std::vector<SummarySpec>(specs.begin(), specs.end()), render);
}

DataFrameView DataFrame::aggregate(std::initializer_list<SummarySpec> specs, bool render) const {
  return DataFrameView(summarize(specs, false), render);
}

DataFrame DataFrame::group(std::initializer_list<std::string_view> keys,
                           std::initializer_list<GroupSpec> specs,
                           bool dropna_keys,
                           bool render) const {
  PLUSS_CHECK(keys.size() != 0, "DataFrame::group: at least one key feature is required");
  PLUSS_CHECK(specs.size() != 0, "DataFrame::group: at least one GroupSpec is required");

  const auto key_names = normalize_name_list(keys);
  std::unordered_set<std::string> key_seen;
  key_seen.reserve(key_names.size());
  for (const auto& key : key_names) {
    PLUSS_CHECK(key_seen.insert(key).second, "DataFrame::group: duplicate key '" + key + "'");
  }

  struct KeyInfo {
    std::string name;
    Thread col;
    Thread value_col;
    std::optional<std::vector<bool>> valid;
  };
  std::vector<KeyInfo> key_infos;
  key_infos.reserve(key_names.size());
  for (const auto& key : key_names) {
    auto col = features(key);
    auto value_col = col.has_validity_mask() ? col.clear_validity_mask() : col;
    key_infos.push_back({
        key,
        col,
        value_col,
        col.has_validity_mask() ? std::optional<std::vector<bool>>(col.validity_mask()->to_vec<bool>()) : std::nullopt,
    });
  }

  struct SpecInfo {
    GroupOp op{GroupOp::Count};
    std::string out_name;
    bool has_feature{false};
    Thread col{};
    Thread value_col{};
    std::optional<std::vector<bool>> valid;
  };

  std::vector<SpecInfo> spec_infos;
  spec_infos.reserve(specs.size());
  std::unordered_set<std::string> out_seen(key_names.begin(), key_names.end());
  out_seen.reserve(key_names.size() + specs.size());

  for (const auto& spec : specs) {
    const auto has_feature = !spec.feature.empty();
    if (spec.op != GroupOp::Count) {
      PLUSS_CHECK(has_feature, "DataFrame::group: feature is required for non-count aggregations");
    }

    Thread col;
    Thread value_col;
    std::optional<std::vector<bool>> valid;
    if (has_feature) {
      col = features(spec.feature);
      value_col = col.has_validity_mask() ? col.clear_validity_mask() : col;
      valid = col.has_validity_mask() ? std::optional<std::vector<bool>>(col.validity_mask()->to_vec<bool>())
                                      : std::nullopt;

      if (spec.op == GroupOp::Sum || spec.op == GroupOp::Mean || spec.op == GroupOp::Min || spec.op == GroupOp::Max) {
        PLUSS_CHECK(col.kind() == ThreadKind::Series && is_number_series_type(col.as_series()),
                    "DataFrame::group: numeric aggregations require numeric Series features");
      }
    }

    auto out_name = spec.output;
    if (out_name.empty()) {
      if (spec.op == GroupOp::Count && !has_feature) {
        out_name = "count";
      } else if (has_feature) {
        out_name = fmt::format("{}_{}", spec.feature, group_op_name(spec.op));
      } else {
        out_name = group_op_name(spec.op);
      }
    }
    PLUSS_CHECK(out_seen.insert(out_name).second, "DataFrame::group: duplicate output feature '" + out_name + "'");

    spec_infos.push_back({
        spec.op,
        std::move(out_name),
        has_feature,
        std::move(col),
        std::move(value_col),
        std::move(valid),
    });
  }

  struct AggState {
    std::int64_t count{0};
    double sum{0.0};
    double min{0.0};
    double max{0.0};
    bool has_numeric{false};
    std::unordered_set<std::string> uniq{};
  };

  struct GroupState {
    std::vector<std::string> keys;
    std::vector<AggState> agg;
  };

  std::vector<GroupState> groups;
  groups.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 32)));
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(groups.capacity());

  for (std::int64_t r = 0; r < nrows(); ++r) {
    std::vector<std::string> key_vals(key_infos.size());
    bool skip = false;
    for (std::size_t k = 0; k < key_infos.size(); ++k) {
      const auto& key_info = key_infos[k];
      const bool valid = !key_info.valid.has_value() || (*key_info.valid)[static_cast<std::size_t>(r)];
      if (!valid && dropna_keys) {
        skip = true;
        break;
      }
      key_vals[k] = valid ? thread_scalar_string_at(key_info.value_col, r) : "NA";
    }
    if (skip) {
      continue;
    }

    std::string composite;
    for (std::size_t k = 0; k < key_vals.size(); ++k) {
      if (k) {
        composite.push_back('\x1f');
      }
      composite += key_vals[k];
    }

    const auto [it, inserted] = index_by_key.emplace(composite, groups.size());
    if (inserted) {
      groups.push_back({std::move(key_vals), std::vector<AggState>(spec_infos.size())});
    }
    auto& g = groups[it->second];

    for (std::size_t s = 0; s < spec_infos.size(); ++s) {
      const auto& spec = spec_infos[s];
      auto& agg = g.agg[s];

      if (spec.op == GroupOp::Count && !spec.has_feature) {
        agg.count += 1;
        continue;
      }

      const bool valid = !spec.valid.has_value() || (*spec.valid)[static_cast<std::size_t>(r)];
      if (!valid) {
        continue;
      }

      if (spec.op == GroupOp::Count) {
        agg.count += 1;
        continue;
      }
      if (spec.op == GroupOp::Nunique) {
        agg.uniq.insert(thread_scalar_string_at(spec.value_col, r));
        continue;
      }

      double x = 0.0;
      if (!extract_numeric_value(spec.col.as_series(), r, x)) {
        continue;
      }
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
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(key_infos.size() + spec_infos.size());

  for (std::size_t k = 0; k < key_infos.size(); ++k) {
    std::vector<std::string> col;
    col.reserve(groups.size());
    for (const auto& g : groups) {
      col.push_back(g.keys[k]);
    }
    out_cols.emplace_back(key_infos[k].name, make_note_thread(col));
  }

  for (std::size_t s = 0; s < spec_infos.size(); ++s) {
    const auto& spec = spec_infos[s];
    if (spec.op == GroupOp::Count || spec.op == GroupOp::Nunique) {
      std::vector<std::int64_t> col;
      col.reserve(groups.size());
      for (const auto& g : groups) {
        const auto& agg = g.agg[s];
        if (spec.op == GroupOp::Count) {
          col.push_back(agg.count);
        } else {
          col.push_back(static_cast<std::int64_t>(agg.uniq.size()));
        }
      }
      out_cols.emplace_back(spec.out_name, make_i64_thread(col));
      continue;
    }

    std::vector<double> col;
    col.reserve(groups.size());
    for (const auto& g : groups) {
      const auto& agg = g.agg[s];
      if (spec.op == GroupOp::Sum) {
        col.push_back(agg.count > 0 ? agg.sum : std::numeric_limits<double>::quiet_NaN());
      } else if (spec.op == GroupOp::Mean) {
        col.push_back(agg.count > 0 ? (agg.sum / static_cast<double>(agg.count)) : std::numeric_limits<double>::quiet_NaN());
      } else if (spec.op == GroupOp::Min) {
        col.push_back(agg.has_numeric ? agg.min : std::numeric_limits<double>::quiet_NaN());
      } else {
        col.push_back(agg.has_numeric ? agg.max : std::numeric_limits<double>::quiet_NaN());
      }
    }
    out_cols.emplace_back(spec.out_name, make_f64_thread(col));
  }

  (void)render;
  return DataFrame(std::move(out_cols));
}

DataFrameView DataFrame::group_by(std::initializer_list<std::string_view> keys,
                                  std::initializer_list<GroupSpec> specs,
                                  bool dropna_keys,
                                  bool render) const {
  return DataFrameView(group(keys, specs, dropna_keys, false), render);
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
  const auto counts = dup.features("__pivot_cell_count__").as_series().to_vec<std::int64_t>();
  const bool has_duplicate = std::any_of(counts.begin(), counts.end(), [](std::int64_t v) { return v > 1; });
  PLUSS_CHECK(!has_duplicate,
              "DataFrame::pivot: duplicate case/feature pairs found; use pivot_table with an aggregation op");
  return pivot_table(case_feature, feature_feature, value_feature, GroupOp::Max, dropna_keys, render);
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
  const auto* value_series = (value_col.kind() == ThreadKind::Series) ? &value_col.as_series() : nullptr;

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

  case_to_row.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 64)));
  feat_to_col.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 64)));

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

  for (std::int64_t r = 0; r < nrows(); ++r) {
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

    double x = 0.0;
    if (!extract_numeric_value(*value_series, r, x)) {
      continue;
    }
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
  return DataFrameView(std::move(out), render);
}

DataFrame DataFrame::combine(const DataFrame& other) const {
  return combine(other, CombineSpec{});
}

DataFrame DataFrame::combine(const DataFrame& other, const CombineSpec& spec) const {
  PLUSS_CHECK(nrows() == other.nrows(),
              "DataFrame::combine: case count mismatch between left and right frames");

  const auto& left = impl_ref_();
  const auto& right = other.impl_ref_();

  std::vector<std::string> out_names = left.names_;
  std::vector<Thread> out_cols = left.columns_;

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

  for (std::size_t i = 0; i < right.names_.size(); ++i) {
    const auto& right_name = right.names_[i];
    const auto& right_col = right.columns_[i];

    const auto it = out_index.find(right_name);
    if (spec.op == CombineOp::Update) {
      if (it == out_index.end()) {
        continue;
      }
      if (!spec.overwrite) {
        continue;
      }
      out_cols[it->second] = right_col;
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
  rows.reserve(static_cast<std::size_t>(grouped.nrows()));
  for (std::int64_t i = 0; i < grouped.nrows(); ++i) {
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
  return DataFrameView(std::move(out), render);
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
  rows.reserve(static_cast<std::size_t>(grouped.nrows()));
  for (std::int64_t i = 0; i < grouped.nrows(); ++i) {
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
  return DataFrameView(std::move(out), render);
}
