namespace {

bool is_time_like_thread_(ThreadKind kind) {
  return kind == ThreadKind::Timeline || kind == ThreadKind::TimeIndex;
}

std::vector<std::size_t> resolve_time_feature_indices_(
    const detail::DataFrameImpl& src,
    std::initializer_list<std::string_view> features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (is_time_like_thread_(src.columns_[i].kind())) {
        indices.push_back(i);
      }
    }
    PLUSS_CHECK(!indices.empty(), std::string(where) + ": no Timeline/TimeIndex features found");
    return indices;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  indices.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + name + "'");
    const auto k = src.columns_[it->second].kind();
    PLUSS_CHECK(is_time_like_thread_(k),
                std::string(where) + ": feature '" + name + "' is not Timeline/TimeIndex");
    indices.push_back(it->second);
  }
  return indices;
}

std::vector<std::size_t> resolve_shift_feature_indices_(
    const detail::DataFrameImpl& src,
    std::initializer_list<std::string_view> features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      indices.push_back(i);
    }
    return indices;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  indices.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + name + "'");
    indices.push_back(it->second);
  }
  return indices;
}

pluss::TimeIndex time_axis_to_index_(const Thread& axis, std::string_view where) {
  if (axis.kind() == ThreadKind::TimeIndex) {
    return axis.as_timeindex();
  }
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline, std::string(where) + ": axis feature must be Timeline/TimeIndex");
  return axis.as_timeline().to_timeindex(false);
}

std::vector<std::size_t> resolve_resample_value_indices_(
    const detail::DataFrameImpl& src,
    std::size_t axis_idx,
    std::initializer_list<std::string_view> value_features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (value_features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (i == axis_idx) {
        continue;
      }
      const auto& col = src.columns_[i];
      if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
        indices.push_back(i);
      }
    }
    PLUSS_CHECK(!indices.empty(), std::string(where) + ": no numeric Series features available for resample");
    return indices;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(value_features.size());
  indices.reserve(value_features.size());
  for (const auto name_sv : value_features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": value feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate value feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown value feature '" + name + "'");
    PLUSS_CHECK(it->second != axis_idx, std::string(where) + ": value feature cannot also be the time axis");
    const auto& col = src.columns_[it->second];
    PLUSS_CHECK(col.kind() == ThreadKind::Series, std::string(where) + ": value feature '" + name + "' must be Series");
    indices.push_back(it->second);
  }
  return indices;
}

} // namespace

DataFrameView DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  auto out = slice(0, clipped);
  return DataFrameView(std::move(out), render, "head", title());
}

DataFrameView DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  auto out = slice(ncases() - clipped, ncases());
  return DataFrameView(std::move(out), render, "tail", title());
}

DataFrame DataFrame::sample(std::int64_t n, std::optional<std::uint64_t> random_state) const {
  const auto total = ncases();
  PLUSS_CHECK(total >= 0, "DataFrame::sample: invalid case count");
  PLUSS_CHECK(n >= 0, "DataFrame::sample: n must be >= 0");
  const auto draw = std::min<std::int64_t>(n, total);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(total));
  std::iota(idx.begin(), idx.end(), 0);

  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(draw));
  return take(idx);
}

DataFrame DataFrame::shift(std::int64_t periods) const {
  return shift({}, periods);
}

DataFrame DataFrame::shift(std::initializer_list<std::string_view> features, std::int64_t periods) const {
  const auto& src = impl_ref_();
  if (src.columns_.empty() || src.nrows_ == 0 || periods == 0) {
    return *this;
  }

  const auto indices = resolve_shift_feature_indices_(src, features, "DataFrame::shift");
  if (indices.empty()) {
    return *this;
  }

  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    out_cols[idx] = src.columns_[idx].shift(periods);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, ncases());
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    idx.push_back(i);
  }
  return take(idx);
}

DataFrame DataFrame::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.take(indices));
  }
  auto out_case = src.cases_.take(indices);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::where(const pluss::Series& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, ncases(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.cases_.filter(bool_mask);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::query(const pluss::Series& bool_mask) const {
  return where(bool_mask);
}

DataFrame DataFrame::query(const QueryFn& predicate) const {
  PLUSS_CHECK(static_cast<bool>(predicate), "DataFrame::query: predicate is empty");
  return where(predicate(*this));
}

DataFrame DataFrame::query(std::string_view expression) const {
  const auto text = owned_name(expression);
  PLUSS_CHECK(!text.empty(), "DataFrame::query: expression cannot be empty");
  auto parser = QueryParser(*this, tokenize_query(text));
  auto eval = parser.parse();

  auto mask = pluss::Series::zeros({ncases()}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < ncases(); ++i) {
    keep[i] = eval(i);
  }
  return where(mask);
}

DataFrame DataFrame::derive(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  DataFrame out = *this;
  for (const auto& assignment : assignments) {
    PLUSS_CHECK(!assignment.first.empty(), "DataFrame::derive: feature name cannot be empty");
    out = out.with_column(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::derive(std::initializer_list<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::derive: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::derive: derive expression is empty");
    out = out.with_column(spec.feature, spec.expr(out));
  }
  return out;
}

DataFrame DataFrame::assign(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  return derive(assignments);
}

DataFrame DataFrame::assign(std::initializer_list<DeriveSpec> specs) const {
  return derive(specs);
}

DataFrame DataFrame::transform(const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform: transform function is empty");
  const auto& src = impl_ref_();

  auto out_cols = pluss::transmit([&](const Thread& feature) {
    auto transformed = fn(feature);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Thread");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    return transformed;
  }, src.columns_);

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  return transform({TransformSpec{owned_name(feature), fn}});
}

DataFrame DataFrame::transform(std::initializer_list<TransformSpec> specs) const {
  PLUSS_CHECK(specs.size() > 0, "DataFrame::transform: at least one TransformSpec is required");
  const auto& src = impl_ref_();

  auto out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::transform: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::transform: transform expression is empty");
    PLUSS_CHECK(seen.insert(spec.feature).second,
                "DataFrame::transform: duplicate transform target '" + spec.feature + "'");

    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::transform: unknown feature '" + spec.feature + "'");

    auto transformed = spec.expr(out_cols[it->second]);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Thread");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    out_cols[it->second] = std::move(transformed);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::tz_convert(std::initializer_list<std::string_view> features, std::string_view tz_name) const {
  const auto tz = owned_name(tz_name);
  PLUSS_CHECK(!tz.empty(), "DataFrame::tz_convert: tz_name cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::tz_convert");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().tz_convert(tz), col.validity_mask());
    } else {
      out_cols[idx] = Thread(col.as_timeindex().tz_convert(tz), col.validity_mask());
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::tz_localize(std::initializer_list<std::string_view> features,
                                 std::string_view tz_name,
                                 int fold,
                                 std::string_view nonexistent) const {
  const auto tz = owned_name(tz_name);
  const auto nonexistent_mode = owned_name(nonexistent);
  PLUSS_CHECK(!tz.empty(), "DataFrame::tz_localize: tz_name cannot be empty");
  PLUSS_CHECK(!nonexistent_mode.empty(), "DataFrame::tz_localize: nonexistent mode cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::tz_localize");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().tz_localize(tz, fold, nonexistent_mode), col.validity_mask());
    } else {
      out_cols[idx] = Thread(col.as_timeindex().tz_localize(tz, fold, nonexistent_mode), col.validity_mask());
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::to_utc(std::initializer_list<std::string_view> features) const {
  return tz_convert(features, "UTC");
}

DataFrame DataFrame::shift_time(std::initializer_list<std::string_view> features,
                                int periods,
                                std::string_view freq) const {
  const auto freq_owned = owned_name(freq);
  PLUSS_CHECK(!freq_owned.empty(), "DataFrame::shift_time: freq cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::shift_time");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().shift(periods, freq_owned), col.validity_mask());
      continue;
    }
    const auto shifted_timeline = col.as_timeindex().timeline().shift(periods, freq_owned);
    out_cols[idx] = Thread(shifted_timeline.to_timeindex(false), col.validity_mask());
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::floor_time(std::initializer_list<std::string_view> features,
                                std::string_view unit) const {
  const auto unit_owned = owned_name(unit);
  PLUSS_CHECK(!unit_owned.empty(), "DataFrame::floor_time: unit cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::floor_time");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().floor(unit_owned), col.validity_mask());
    } else {
      out_cols[idx] = Thread(col.as_timeindex().floor(unit_owned), col.validity_mask());
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::ceil_time(std::initializer_list<std::string_view> features,
                               std::string_view unit) const {
  const auto unit_owned = owned_name(unit);
  PLUSS_CHECK(!unit_owned.empty(), "DataFrame::ceil_time: unit cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::ceil_time");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().ceil(unit_owned), col.validity_mask());
    } else {
      out_cols[idx] = Thread(col.as_timeindex().ceil(unit_owned), col.validity_mask());
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::round_time(std::initializer_list<std::string_view> features,
                                std::string_view unit) const {
  const auto unit_owned = owned_name(unit);
  PLUSS_CHECK(!unit_owned.empty(), "DataFrame::round_time: unit cannot be empty");
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, features, "DataFrame::round_time");
  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    if (col.kind() == ThreadKind::Timeline) {
      out_cols[idx] = Thread(col.as_timeline().round(unit_owned), col.validity_mask());
    } else {
      out_cols[idx] = Thread(col.as_timeindex().round(unit_owned), col.validity_mask());
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrameView DataFrame::resample(std::string_view time_feature,
                                  std::initializer_list<std::string_view> value_features,
                                  std::string_view freq,
                                  std::string_view op,
                                  std::int64_t step,
                                  bool render) const {
  const auto axis_feature = owned_name(time_feature);
  const auto freq_key = owned_name(freq);
  auto op_key = lower_copy(owned_name(op));
  PLUSS_CHECK(!axis_feature.empty(), "DataFrame::resample: time_feature cannot be empty");
  PLUSS_CHECK(!freq_key.empty(), "DataFrame::resample: freq cannot be empty");
  PLUSS_CHECK(step > 0, "DataFrame::resample: step must be positive");
  PLUSS_CHECK(!op_key.empty(), "DataFrame::resample: op cannot be empty");
  PLUSS_CHECK(op_key != "ohlc", "DataFrame::resample: op='ohlc' is not supported in 1D Thread output");

  const auto& src = impl_ref_();
  const auto axis_it = src.name_to_idx_.find(axis_feature);
  PLUSS_CHECK(axis_it != src.name_to_idx_.end(), "DataFrame::resample: unknown time_feature '" + axis_feature + "'");
  const auto axis_idx = axis_it->second;
  const auto& axis_col = src.columns_[axis_idx];
  PLUSS_CHECK(is_time_like_thread_(axis_col.kind()),
              "DataFrame::resample: time_feature '" + axis_feature + "' must be Timeline/TimeIndex");

  const auto ti = time_axis_to_index_(axis_col, "DataFrame::resample");
  const auto value_indices = resolve_resample_value_indices_(src, axis_idx, value_features, "DataFrame::resample");

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(value_indices.size());
  out_cols.reserve(value_indices.size());

  for (const auto idx : value_indices) {
    const auto& name = src.names_[idx];
    const auto& feature = src.columns_[idx];
    PLUSS_CHECK(feature.kind() == ThreadKind::Series, "DataFrame::resample: value feature must be Series");
    if (op_key == "count" || op_key == "size") {
      out_names.push_back(name);
      out_cols.emplace_back(ti.resample_count(freq_key, step));
      continue;
    }
    PLUSS_CHECK(is_numeric_series_type(feature.as_series()),
                "DataFrame::resample: value feature '" + name + "' must be numeric for op '" + op_key + "'");
    out_names.push_back(name);
    out_cols.emplace_back(ti.resample_agg(feature.as_series(), freq_key, op_key, step));
  }

  auto bins = ti.resample_bins(freq_key, step);
  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), Thread(bins.labels)));
  return DataFrameView(std::move(out), render, "resample", title());
}
