namespace {

bool is_time_like_thread_(ThreadKind kind) {
  return kind == ThreadKind::Timeline || kind == ThreadKind::TimeIndex;
}

std::vector<std::size_t> resolve_time_feature_indices_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
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
    pluss::msharp::RangeView::Of<std::string_view> features,
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
    pluss::msharp::RangeView::Of<std::string_view> value_features,
    std::string_view op_key,
    std::string_view where) {
  std::vector<std::size_t> indices;
  const bool allow_any_thread = (op_key == "count" || op_key == "size");
  if (value_features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (i == axis_idx) {
        continue;
      }
      const auto& col = src.columns_[i];
      if (allow_any_thread) {
        indices.push_back(i);
      } else if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
        indices.push_back(i);
      }
    }
    PLUSS_CHECK(!indices.empty(),
                std::string(where) + (allow_any_thread ? ": no value features available for resample"
                                                       : ": no numeric Series features available for resample"));
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
    if (!allow_any_thread) {
      PLUSS_CHECK(col.kind() == ThreadKind::Series,
                  std::string(where) + ": value feature '" + name + "' must be Series");
      PLUSS_CHECK(is_numeric_series_type(col.as_series()),
                  std::string(where) + ": value feature '" + name + "' must be numeric for op '" + std::string(op_key) + "'");
    }
    indices.push_back(it->second);
  }
  return indices;
}

pluss::Series make_int64_series_(const std::vector<std::int64_t>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Int64);
  auto* ptr = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    ptr[i] = values[i];
  }
  return out;
}

pluss::Series make_float64_series_(const std::vector<double>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Float64);
  auto* ptr = reinterpret_cast<double*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    ptr[i] = values[i];
  }
  return out;
}

Thread resample_feature_thread_(const std::vector<std::int64_t>& bin_id,
                                std::int64_t nbins,
                                const Thread& feature,
                                std::string_view op_key,
                                std::string_view where) {
  PLUSS_CHECK(nbins >= 0, std::string(where) + ": invalid bin count");

  if (op_key == "size") {
    std::vector<std::int64_t> counts(static_cast<std::size_t>(nbins), 0);
    for (const auto b : bin_id) {
      if (b >= 0 && b < nbins) {
        counts[static_cast<std::size_t>(b)] += 1;
      }
    }
    return Thread(make_int64_series_(counts));
  }

  const auto valid = feature.notnull().as_series().to_vec<bool>();

  if (op_key == "count") {
    std::vector<std::int64_t> counts(static_cast<std::size_t>(nbins), 0);
    for (std::size_t i = 0; i < bin_id.size(); ++i) {
      const auto b = bin_id[i];
      if (b >= 0 && b < nbins && valid[i]) {
        counts[static_cast<std::size_t>(b)] += 1;
      }
    }
    return Thread(make_int64_series_(counts));
  }

  PLUSS_CHECK(feature.kind() == ThreadKind::Series,
              std::string(where) + ": value feature must be Series for op '" + std::string(op_key) + "'");
  PLUSS_CHECK(is_numeric_series_type(feature.as_series()),
              std::string(where) + ": value feature must be numeric for op '" + std::string(op_key) + "'");

  const auto values = feature.as_series().to(pluss::codem::ScalarType::Float64).contiguous();
  const auto* vp = reinterpret_cast<const double*>(values.raw_data());
  std::vector<std::int64_t> counts(static_cast<std::size_t>(nbins), 0);
  std::vector<double> sums(static_cast<std::size_t>(nbins), 0.0);
  std::vector<double> mins(static_cast<std::size_t>(nbins), std::numeric_limits<double>::quiet_NaN());
  std::vector<double> maxs(static_cast<std::size_t>(nbins), std::numeric_limits<double>::quiet_NaN());
  std::vector<double> firsts(static_cast<std::size_t>(nbins), std::numeric_limits<double>::quiet_NaN());
  std::vector<double> lasts(static_cast<std::size_t>(nbins), std::numeric_limits<double>::quiet_NaN());

  for (std::size_t i = 0; i < bin_id.size(); ++i) {
    const auto b = bin_id[i];
    if (b < 0 || b >= nbins || !valid[i]) {
      continue;
    }
    const auto idx = static_cast<std::size_t>(b);
    const auto v = vp[i];
    if (counts[idx] == 0) {
      mins[idx] = v;
      maxs[idx] = v;
      firsts[idx] = v;
    } else {
      mins[idx] = std::min(mins[idx], v);
      maxs[idx] = std::max(maxs[idx], v);
    }
    lasts[idx] = v;
    sums[idx] += v;
    counts[idx] += 1;
  }

  std::vector<double> out(static_cast<std::size_t>(nbins), std::numeric_limits<double>::quiet_NaN());
  for (std::int64_t b = 0; b < nbins; ++b) {
    const auto idx = static_cast<std::size_t>(b);
    if (op_key == "sum") {
      out[idx] = counts[idx] == 0 ? 0.0 : sums[idx];
    } else if (op_key == "mean") {
      out[idx] = counts[idx] == 0 ? std::numeric_limits<double>::quiet_NaN()
                                  : sums[idx] / static_cast<double>(counts[idx]);
    } else if (op_key == "min") {
      out[idx] = mins[idx];
    } else if (op_key == "max") {
      out[idx] = maxs[idx];
    } else if (op_key == "first") {
      out[idx] = firsts[idx];
    } else if (op_key == "last") {
      out[idx] = lasts[idx];
    } else {
      PLUSS_FAIL(std::string(where) + ": unsupported resample op '" + std::string(op_key) + "'");
    }
  }

  return Thread(make_float64_series_(out));
}

pluss::Series bool_series_from_thread_mask_(
    const Thread& bool_mask,
    std::int64_t expected_size,
    std::string_view where) {
  PLUSS_CHECK(bool_mask.kind() == ThreadKind::Series, std::string(where) + ": mask must be a Series thread");
  const auto& mask = bool_mask.as_series();
  validate_bool_mask(mask, expected_size, where);
  return mask;
}

} // namespace

DataFrameView DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    cols.emplace_back(src.names_[i], src.columns_[i].head(clipped, /*render=*/false));
  }
  auto out = DataFrame(std::move(cols), src.cases_.head(clipped, /*render=*/false).clear_display_labels())
                 .title(title());
  return DataFrameView(std::move(out), render, "head", title());
}

DataFrameView DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    cols.emplace_back(src.names_[i], src.columns_[i].tail(clipped, /*render=*/false));
  }
  auto out = DataFrame(std::move(cols), src.cases_.tail(clipped, /*render=*/false).clear_display_labels())
                 .title(title());
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

DataFrame DataFrame::reshuffle(int axis, std::optional<std::uint64_t> random_state) const {
  PLUSS_CHECK(axis == 0 || axis == 1, "DataFrame::reshuffle: axis must be 0 (rows) or 1 (features)");
  const auto& src = impl_ref_();
  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));

  if (axis == 0) {
    std::vector<std::int64_t> idx(static_cast<std::size_t>(src.nrows_));
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);
    return take(idx);
  }

  if (src.columns_.empty()) {
    return *this;
  }

  std::vector<std::size_t> order(src.columns_.size());
  std::iota(order.begin(), order.end(), 0);
  std::shuffle(order.begin(), order.end(), rng);

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(order.size());
  out_cols.reserve(order.size());
  for (const auto idx : order) {
    out_names.push_back(src.names_[idx]);
    out_cols.push_back(src.columns_[idx]);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::shift(std::int64_t periods) const {
  return shift({}, periods);
}

DataFrame DataFrame::shift(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t periods) const {
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

DataFrame DataFrame::take(pluss::msharp::RangeView::Int indices) const {
  const auto& src = impl_ref_();
  if (indices.size() == static_cast<std::size_t>(src.nrows_)) {
    bool is_identity = true;
    for (std::size_t i = 0; i < indices.size(); ++i) {
      if (indices[i] != static_cast<std::int64_t>(i)) {
        is_identity = false;
        break;
      }
    }
    if (is_identity) {
      return *this;
    }
  }
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

DataFrame DataFrame::where(const Thread& bool_mask) const {
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

DataFrame DataFrame::filter(const Thread& bool_mask) const {
  return filter(bool_series_from_thread_mask_(bool_mask, ncases(), "DataFrame::filter"));
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

DataFrame DataFrame::assign(pluss::msharp::RangeView::Of<std::pair<std::string, Thread>> assignments) const {
  DataFrame out = *this;
  for (const auto& assignment : assignments) {
    PLUSS_CHECK(!assignment.first.empty(), "DataFrame::assign: feature name cannot be empty");
    out = out.set_features(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::assign(pluss::msharp::RangeView::Of<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::assign: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::assign: assign expression is empty");
    out = out.set_features(spec.feature, spec.expr(out));
  }
  return out;
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

DataFrame DataFrame::transform(std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  const std::array<DeriveSpec, 1> spec{{DeriveSpec{owned_name(feature), fn}}};
  return assign(pluss::msharp::RangeView::Of<DeriveSpec>(spec));
}

DataFrame DataFrame::transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  const std::array<TransformSpec, 1> spec{{TransformSpec{owned_name(feature), fn}}};
  return transform(pluss::msharp::RangeView::Of<TransformSpec>(spec));
}

DataFrame DataFrame::transform(pluss::msharp::RangeView::Of<TransformSpec> specs) const {
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

DataFrame DataFrame::put(std::string_view feature, const Thread& bool_mask, const Thread& value) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put: feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put: unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, src.nrows_, "DataFrame::put");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::put(std::string_view feature, const Thread& bool_mask, double value) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(double): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(double): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, src.nrows_, "DataFrame::put(double)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::put(std::string_view feature, const Thread& bool_mask, std::string_view value) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(string): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(string): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, src.nrows_, "DataFrame::put(string)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::put(std::string_view feature, const Thread& bool_mask, pluss::null_t value) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(null): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(null): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, src.nrows_, "DataFrame::put(null)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame& DataFrame::put_(std::string_view feature, const Thread& bool_mask, const Thread& value) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_: feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_: unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, w.nrows_, "DataFrame::put_");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Thread& bool_mask, double value) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(double): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(double): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, w.nrows_, "DataFrame::put_(double)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Thread& bool_mask, std::string_view value) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(string): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(string): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, w.nrows_, "DataFrame::put_(string)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Thread& bool_mask, pluss::null_t value) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(null): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(null): unknown feature '" + name + "'");
  const auto mask = bool_series_from_thread_mask_(bool_mask, w.nrows_, "DataFrame::put_(null)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame DataFrame::tz_convert(pluss::msharp::RangeView::Of<std::string_view> features, std::string_view tz_name) const {
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

DataFrame DataFrame::tz_localize(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::to_utc(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return tz_convert(features, "UTC");
}

DataFrame DataFrame::shift_time(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::floor_time(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::ceil_time(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::round_time(pluss::msharp::RangeView::Of<std::string_view> features,
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
                                  pluss::msharp::RangeView::Of<std::string_view> value_features,
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
  const auto value_indices = resolve_resample_value_indices_(src, axis_idx, value_features, op_key, "DataFrame::resample");
  const auto bins = ti.resample_bins(freq_key, step);
  const auto bin_id = bins.bin_id.to_vec<std::int64_t>();
  const auto nbins = bins.labels.numel();

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(value_indices.size());
  out_cols.reserve(value_indices.size());

  for (const auto idx : value_indices) {
    const auto& name = src.names_[idx];
    const auto& feature = src.columns_[idx];
    out_names.push_back(name);
    out_cols.emplace_back(resample_feature_thread_(bin_id, nbins, feature, op_key, "DataFrame::resample"));
  }

  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), Thread(bins.labels)));
  return DataFrameView(std::move(out), render, "resample", title());
}
