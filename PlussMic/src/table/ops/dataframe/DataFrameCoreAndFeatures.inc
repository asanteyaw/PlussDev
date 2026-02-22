bool DataFrame::defined() const noexcept { return static_cast<bool>(impl_); }
bool DataFrame::unique() const noexcept { return impl_.unique(); }

const detail::DataFrameImpl& DataFrame::impl_ref_() const {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  return *impl_;
}

detail::DataFrameImpl& DataFrame::mut_impl_() {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

void DataFrame::suppress_next_stream_() const {
  if (!defined()) {
    return;
  }
  impl_ref_().suppress_next_stream_ = true;
}

bool DataFrame::consume_stream_suppression_() const {
  if (!defined()) {
    return false;
  }
  auto& src = impl_ref_();
  if (!src.suppress_next_stream_) {
    return false;
  }
  src.suppress_next_stream_ = false;
  return true;
}

std::int64_t DataFrame::nrows() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::ncols() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }
std::int64_t DataFrame::nfeatures() const { return ncols(); }

const std::vector<std::string>& DataFrame::column_names() const { return impl_ref_().names_; }

std::vector<Thread> DataFrame::columns() const { return impl_ref_().columns_; }
std::vector<Thread> DataFrame::values() const { return columns(); }
const std::vector<std::string>& DataFrame::feature_names() const { return column_names(); }
std::vector<Thread> DataFrame::features() const { return columns(); }

const Thread& DataFrame::cases() const { return impl_ref_().cases_; }

DataFrame DataFrame::cases(std::int64_t row) const {
  const auto idx = normalize_row_index(row, nrows());
  return slice(idx, idx + 1);
}

DataFrame DataFrame::cases(std::pair<std::int64_t, std::int64_t> row_span) const {
  return slice(row_span.first, row_span.second);
}

std::string DataFrame::cases(std::int64_t row, std::size_t feature_idx) const {
  const auto r = normalize_row_index(row, nrows());
  PLUSS_CHECK(feature_idx < static_cast<std::size_t>(ncols()), "DataFrame::cases(row, feature): feature index out of bounds");
  const auto& src = impl_ref_();
  return thread_scalar_string_at(src.columns_[feature_idx], r);
}

std::string DataFrame::cases(std::int64_t row, std::string_view feature_name) const {
  const auto r = normalize_row_index(row, nrows());
  return thread_scalar_string_at(column(feature_name), r);
}

pluss::Series DataFrame::cases_series() const {
  PLUSS_CHECK(cases().kind() == ThreadKind::Series, "DataFrame::cases_series requires a Series-backed case axis");
  return cases().as_series();
}

DataFrame DataFrame::iloc(std::int64_t row) const {
  return cases(row);
}

DataFrame DataFrame::iloc(std::int64_t start, std::int64_t stop) const {
  return slice(start, stop);
}

std::string DataFrame::iat(std::int64_t row, std::size_t feature_idx) const {
  return cases(row, feature_idx);
}

std::string DataFrame::at(std::int64_t row, std::string_view feature_name) const {
  return cases(row, feature_name);
}

std::optional<Thread> DataFrame::get(std::string_view feature_name) const {
  if (!has_feature(feature_name)) {
    return std::nullopt;
  }
  return features(feature_name);
}

DataFrame DataFrame::with_cases(const Thread& case_idx) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases: cases are undefined");
  PLUSS_CHECK(case_idx.size() == src.nrows_, "DataFrame::with_cases: case length mismatch");
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, case_idx));
}

DataFrame& DataFrame::with_cases_(const Thread& case_idx) {
  auto& w = mut_impl_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases_: cases are undefined");
  PLUSS_CHECK(case_idx.size() == w.nrows_, "DataFrame::with_cases_: case length mismatch");
  w.cases_ = case_idx;
  w.validate_and_reindex_();
  return *this;
}

bool DataFrame::has_column(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_idx_.find(owned_name(name)) != src.name_to_idx_.end();
}

bool DataFrame::has_feature(std::string_view name) const { return has_column(name); }

Thread DataFrame::column(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(owned_name(name));
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + owned_name(name) + "'");
  return src.columns_[it->second];
}

Thread DataFrame::features(std::string_view name) { return column(name); }
Thread DataFrame::features(std::string_view name) const { return column(name); }
pluss::Series DataFrame::series_value(std::string_view feature_name) const { return features(feature_name).as_series(); }
pluss::Note DataFrame::note_value(std::string_view feature_name) const { return features(feature_name).as_note(); }
pluss::Timeline DataFrame::timeline_value(std::string_view feature_name) const { return features(feature_name).as_timeline(); }
pluss::TimeIndex DataFrame::timeindex_value(std::string_view feature_name) const { return features(feature_name).as_timeindex(); }

ColumnRef DataFrame::column_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), column(name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const { return column_ref(name); }

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) {
  return select(names);
}

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) const {
  return select(names);
}

DataFrame DataFrame::select(std::initializer_list<std::string_view> names) const {
  const auto picked = normalize_name_list(names);
  const auto& src = impl_ref_();

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(picked.size());
  out_cols.reserve(picked.size());

  for (const auto& n : picked) {
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::select: unknown column '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::select(const std::vector<std::string>& names) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(names.size());
  out_cols.reserve(names.size());

  std::unordered_set<std::string> seen;
  seen.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(!n.empty(), "DataFrame::select(vector): feature name cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::select(vector): duplicate feature '" + n + "'");
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::select(vector): unknown feature '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::series() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != ThreadKind::Series) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::note() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != ThreadKind::Note) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::timeline() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto k = src.columns_[i].kind();
    if (k != ThreadKind::Timeline && k != ThreadKind::TimeIndex) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::timeindex() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != ThreadKind::TimeIndex) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_series(std::initializer_list<std::string_view> features,
                                 std::optional<pluss::codem::ScalarType> dtype) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_series: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_series: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Series, dtype);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_note(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_note: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_note: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Note, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_timeline(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_timeline: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_timeline: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Timeline, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_timeindex(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_timeindex: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_timeindex: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::TimeIndex, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::astype(std::initializer_list<std::string_view> features,
                            pluss::codem::ScalarType dtype) const {
  return cast_series(features, dtype);
}

DataFrame DataFrame::insert(std::int64_t feature_index, std::string name, const Thread& feature_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!name.empty(), "DataFrame::insert: feature name cannot be empty");
  PLUSS_CHECK(feature_value.defined(), "DataFrame::insert: feature is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(feature_value.size() == src.nrows_, "DataFrame::insert: case count mismatch");
  }
  PLUSS_CHECK(src.name_to_idx_.find(name) == src.name_to_idx_.end(),
              "DataFrame::insert: duplicate feature name '" + name + "'");

  const auto pos = std::clamp<std::int64_t>(feature_index, 0, static_cast<std::int64_t>(src.names_.size()));
  auto out_names = src.names_;
  auto out_cols = src.columns_;
  out_names.insert(out_names.begin() + pos, std::move(name));
  out_cols.insert(out_cols.begin() + pos, feature_value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::drop(std::initializer_list<std::string_view> names) const {
  const auto drop_names = normalize_name_list(names);
  const auto& src = impl_ref_();
  std::unordered_set<std::string> drop_set;
  drop_set.reserve(drop_names.size());
  for (const auto& n : drop_names) {
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::drop: unknown column '" + n + "'");
    drop_set.insert(n);
  }

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (drop_set.find(src.names_[i]) == drop_set.end()) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::drop(const std::vector<std::string>& names) const {
  const auto& src = impl_ref_();
  std::unordered_set<std::string> drop_set;
  drop_set.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(!n.empty(), "DataFrame::drop(vector): feature name cannot be empty");
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::drop(vector): unknown feature '" + n + "'");
    drop_set.insert(n);
  }

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (drop_set.find(src.names_[i]) == drop_set.end()) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::reorder(std::initializer_list<std::string_view> names) const {
  const auto ordered = normalize_name_list(names);
  const auto& src = impl_ref_();

  PLUSS_CHECK(ordered.size() == src.names_.size(), "DataFrame::reorder requires the full column set");
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::reorder contains duplicate column name '" + n + "'");
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::reorder unknown column '" + n + "'");
  }
  return select(names);
}

DataFrame DataFrame::add_prefix(std::string_view prefix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(fmt::format("{}{}", prefix, name));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::add_suffix(std::string_view suffix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(fmt::format("{}{}", name, suffix));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::arrange(std::initializer_list<std::string_view> features,
                             bool ascending,
                             bool nulls_last) const {
  std::vector<ArrangeSpec> specs;
  specs.reserve(features.size());
  for (const auto name : features) {
    specs.push_back({owned_name(name), ascending, nulls_last});
  }
  return arrange_impl_frame(*this, specs);
}

DataFrame DataFrame::arrange(std::initializer_list<ArrangeSpec> specs) const {
  return arrange_impl_frame(*this, std::vector<ArrangeSpec>(specs.begin(), specs.end()));
}

DataFrame DataFrame::sort_values(std::initializer_list<std::string_view> features,
                                 bool ascending,
                                 bool nulls_last) const {
  return arrange(features, ascending, nulls_last);
}

DataFrame DataFrame::sort_values(std::initializer_list<ArrangeSpec> specs) const {
  return arrange(specs);
}

DataFrame DataFrame::sort(std::string_view by_feature, bool ascending, bool nulls_last) const {
  return sort_values({ArrangeSpec{owned_name(by_feature), ascending, nulls_last}});
}

DataFrame DataFrame::sort_by(const std::vector<std::string>& features,
                             const std::vector<bool>& ascending,
                             bool nulls_last) const {
  PLUSS_CHECK(!features.empty(), "DataFrame::sort_by: at least one feature is required");
  PLUSS_CHECK(ascending.empty() || ascending.size() == features.size(),
              "DataFrame::sort_by: ascending size must be 0 or equal to features size");

  std::vector<ArrangeSpec> specs;
  specs.reserve(features.size());
  for (std::size_t i = 0; i < features.size(); ++i) {
    const auto asc = ascending.empty() ? true : ascending[i];
    specs.push_back({features[i], asc, nulls_last});
  }
  return arrange_impl_frame(*this, specs);
}

DataFrame DataFrame::sort_cases(bool ascending, bool nulls_last) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows()));
  std::iota(idx.begin(), idx.end(), 0);
  const auto& case_axis = cases();
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    const auto cmp = case_axis.compare_rows(a, b, nulls_last);
    return ascending ? (cmp < 0) : (cmp > 0);
  });
  return take(idx);
}

DataFrame DataFrame::top(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::top: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return sort_values({ArrangeSpec{owned_name(feature_name), false, true}})
      .head(n, false);
}

DataFrame DataFrame::bottom(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::bottom: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return sort_values({ArrangeSpec{owned_name(feature_name), true, true}})
      .head(n, false);
}

pluss::Series DataFrame::repeated_cases(std::initializer_list<std::string_view> features) const {
  const auto& src = impl_ref_();
  std::vector<std::size_t> feature_idx;
  if (features.size() == 0) {
    feature_idx.resize(src.columns_.size());
    std::iota(feature_idx.begin(), feature_idx.end(), static_cast<std::size_t>(0));
  } else {
    std::unordered_set<std::size_t> seen;
    seen.reserve(features.size());
    for (const auto f : features) {
      const auto it = src.name_to_idx_.find(owned_name(f));
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::repeated_cases: unknown feature '" + owned_name(f) + "'");
      if (seen.insert(it->second).second) {
        feature_idx.push_back(it->second);
      }
    }
  }

  auto out = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* mask = reinterpret_cast<bool*>(out.raw_data());

  std::unordered_set<std::string> seen_keys;
  seen_keys.reserve(static_cast<std::size_t>(nrows()));
  for (std::int64_t r = 0; r < src.nrows_; ++r) {
    std::string key;
    key.reserve(feature_idx.size() * 16);
    for (const auto idx : feature_idx) {
      const auto& col = src.columns_[idx];
      if (!col.is_valid(r)) {
        key.append("\x1D", 1);
      } else {
        key += thread_scalar_string_at(col, r);
      }
      key.append("\x1E", 1);
    }
    const auto inserted = seen_keys.insert(std::move(key)).second;
    mask[r] = !inserted;
  }
  return out;
}

DataFrame DataFrame::unique_cases(std::initializer_list<std::string_view> features) const {
  const auto dup = repeated_cases(features).to_vec<bool>();
  auto keep = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(keep.raw_data());
  for (std::int64_t i = 0; i < nrows(); ++i) {
    p[i] = !dup[static_cast<std::size_t>(i)];
  }
  return filter(keep);
}

DataFrame DataFrame::set_cases(std::string_view feature_name, bool drop_feature) const {
  auto out = with_cases(features(feature_name));
  if (drop_feature) {
    out = out.drop({feature_name});
  }
  return out;
}

DataFrame DataFrame::reset_cases(std::string case_feature) const {
  auto out = with_column(case_feature, cases());
  return out.with_cases(default_cases(out.nrows()));
}

DataFrame DataFrame::align_cases(const Thread& new_cases) const {
  PLUSS_CHECK(new_cases.defined(), "DataFrame::align_cases: new cases are undefined");
  std::unordered_map<std::string, std::int64_t> old_pos;
  old_pos.reserve(static_cast<std::size_t>(nrows()));
  for (std::int64_t i = 0; i < nrows(); ++i) {
    old_pos.emplace(thread_scalar_string_at(cases(), i), i);
  }

  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(new_cases.size()));
  for (std::int64_t i = 0; i < new_cases.size(); ++i) {
    const auto key = thread_scalar_string_at(new_cases, i);
    const auto it = old_pos.find(key);
    PLUSS_CHECK(it != old_pos.end(),
                "DataFrame::align_cases: case label not found '" + key + "'");
    indices.push_back(it->second);
  }
  return take(indices).with_cases(new_cases);
}

DataFrame DataFrame::join(const DataFrame& other, std::string collision_suffix) const {
  CombineSpec spec;
  spec.op = CombineOp::Features;
  spec.collision_suffix = std::move(collision_suffix);
  return combine(other, spec);
}

DataFrame DataFrame::update(const DataFrame& other, bool overwrite) const {
  CombineSpec spec;
  spec.op = CombineOp::Update;
  spec.overwrite = overwrite;
  return combine(other, spec);
}

DataFrame DataFrame::with_column(std::string name, const Thread& column_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column: input column is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == src.nrows_, "DataFrame::with_column: row count mismatch");
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  const auto it = src.name_to_idx_.find(name);
  if (it == src.name_to_idx_.end()) {
    out_names.push_back(std::move(name));
    out_cols.push_back(column_value);
  } else {
    out_cols[it->second] = column_value;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame& DataFrame::with_column_(std::string name, const Thread& column_value) {
  auto& w = mut_impl_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column_: input column is undefined");
  if (!w.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == w.nrows_, "DataFrame::with_column_: row count mismatch");
  }

  const auto it = w.name_to_idx_.find(name);
  if (it == w.name_to_idx_.end()) {
    w.names_.push_back(std::move(name));
    w.columns_.push_back(column_value);
  } else {
    w.columns_[it->second] = column_value;
  }
  w.validate_and_reindex_();
  return *this;
}

DataFrame::LetSlot DataFrame::set(std::string_view name) {
  return LetSlot(*this, owned_name(name));
}

DataFrame::LetSlots DataFrame::set(std::initializer_list<std::string_view> names) {
  return LetSlots(*this, normalize_name_list(names));
}

DataFrame DataFrame::set_features(std::string_view name, const Thread& value) const {
  return with_column(owned_name(name), value);
}

DataFrame& DataFrame::set_features_(std::string_view name, const Thread& value) {
  return with_column_(owned_name(name), value);
}

DataFrame DataFrame::set_features(std::initializer_list<std::string_view> names,
                                  std::initializer_list<Thread> values) const {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features: names/value size mismatch");
  DataFrame out = *this;
  auto value_it = values.begin();
  for (const auto name : names) {
    out = out.with_column(owned_name(name), *value_it);
    ++value_it;
  }
  return out;
}

DataFrame& DataFrame::set_features_(std::initializer_list<std::string_view> names,
                                    std::initializer_list<Thread> values) {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features_: names/value size mismatch");
  auto value_it = values.begin();
  for (const auto name : names) {
    with_column_(owned_name(name), *value_it);
    ++value_it;
  }
  return *this;
}

DataFrame DataFrame::rename(std::string_view old_name, std::string new_name) const {
  const auto& src = impl_ref_();
  const auto old_key = owned_name(old_name);
  const auto it = src.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::rename: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(src.name_to_idx_.find(new_name) == src.name_to_idx_.end(), "DataFrame::rename: target name already exists");
  }

  auto out_names = src.names_;
  out_names[it->second] = std::move(new_name);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::rename(const std::unordered_map<std::string, std::string>& mapping) const {
  const auto& src = impl_ref_();
  if (mapping.empty()) {
    return *this;
  }

  std::unordered_set<std::string> target_names;
  target_names.reserve(src.names_.size() * 2);
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());

  for (const auto& name : src.names_) {
    auto it = mapping.find(name);
    std::string next = (it == mapping.end()) ? name : it->second;
    PLUSS_CHECK(!next.empty(), "DataFrame::rename(mapping): target feature name cannot be empty");
    PLUSS_CHECK(target_names.insert(next).second,
                "DataFrame::rename(mapping): duplicate target feature '" + next + "'");
    out_names.push_back(std::move(next));
  }

  for (const auto& [from, _] : mapping) {
    PLUSS_CHECK(src.name_to_idx_.find(from) != src.name_to_idx_.end(),
                "DataFrame::rename(mapping): unknown source feature '" + from + "'");
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_));
}

DataFrame& DataFrame::rename_(std::string_view old_name, std::string new_name) {
  auto& w = mut_impl_();
  const auto old_key = owned_name(old_name);
  const auto it = w.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::rename_: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(w.name_to_idx_.find(new_name) == w.name_to_idx_.end(), "DataFrame::rename_: target name already exists");
  }

  w.names_[it->second] = std::move(new_name);
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::rename_features(
    std::initializer_list<std::pair<std::string_view, std::string_view>> renames) const {
  DataFrame out = *this;
  for (const auto& [old_name, new_name] : renames) {
    out = out.rename(old_name, std::string(new_name));
  }
  return out;
}

DataFrame DataFrame::relabel_features(std::initializer_list<std::string_view> feature_names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(feature_names.size() == src.names_.size(),
              "DataFrame::relabel_features: number of names must match feature count");
  std::vector<std::string> out_names;
  out_names.reserve(feature_names.size());
  std::unordered_set<std::string> seen;
  seen.reserve(feature_names.size());
  for (const auto n : feature_names) {
    const auto key = owned_name(n);
    PLUSS_CHECK(!key.empty(), "DataFrame::relabel_features: feature name cannot be empty");
    PLUSS_CHECK(seen.insert(key).second,
                "DataFrame::relabel_features: duplicate feature name '" + key + "'");
    out_names.push_back(key);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::set_feature_axis(std::initializer_list<std::string_view> feature_names) const {
  return relabel_features(feature_names);
}

DataFrame DataFrame::set_axis(std::initializer_list<std::string_view> feature_names) const {
  return set_feature_axis(feature_names);
}

DataFrame DataFrame::conform_to(const DataFrame& other) const {
  const auto& src = impl_ref_();
  const auto ordered = other.feature_names();
  PLUSS_CHECK(ordered.size() == src.names_.size(),
              "DataFrame::conform_to: both frames must have same feature count");

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(ordered.size());
  out_cols.reserve(ordered.size());
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::conform_to: duplicate feature in target '" + n + "'");
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::conform_to: missing feature '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
  return out.align_cases(other.cases());
}

DataFrame DataFrame::reindex_like(const DataFrame& other) const {
  return conform_to(other);
}

DataFrame DataFrame::reindex_cases(const Thread& new_cases) const {
  return align_cases(new_cases);
}

DataFrame DataFrame::stitch_on(std::string_view left_feature,
                               const DataFrame& right,
                               std::string_view right_feature,
                               std::string right_suffix) const {
  const auto left_key = owned_name(left_feature);
  const auto right_key = right_feature.empty() ? left_key : owned_name(right_feature);
  PLUSS_CHECK(!left_key.empty(), "DataFrame::stitch_on: left feature cannot be empty");
  PLUSS_CHECK(!right_key.empty(), "DataFrame::stitch_on: right feature cannot be empty");
  PLUSS_CHECK(has_feature(left_key), "DataFrame::stitch_on: left feature not found '" + left_key + "'");
  PLUSS_CHECK(right.has_feature(right_key), "DataFrame::stitch_on: right feature not found '" + right_key + "'");

  const auto& left_src = impl_ref_();
  const auto& right_src = right.impl_ref_();
  const auto left_col = features(left_key);
  const auto right_col = right.features(right_key);

  std::unordered_map<std::string, std::vector<std::int64_t>> right_lookup;
  right_lookup.reserve(static_cast<std::size_t>(right.nrows()));
  for (std::int64_t r = 0; r < right.nrows(); ++r) {
    right_lookup[thread_scalar_string_at(right_col, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < nrows(); ++l) {
    const auto key = thread_scalar_string_at(left_col, l);
    const auto it = right_lookup.find(key);
    if (it == right_lookup.end()) {
      continue;
    }
    for (const auto rr : it->second) {
      left_rows.push_back(l);
      right_rows.push_back(rr);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(left_src.names_.size() + right_src.names_.size());

  for (std::size_t i = 0; i < left_src.names_.size(); ++i) {
    out_cols.emplace_back(left_src.names_[i], left_src.columns_[i].take(left_rows));
  }

  std::unordered_set<std::string> used_names;
  used_names.reserve(out_cols.size() + right_src.names_.size());
  for (const auto& [name, _] : out_cols) {
    used_names.insert(name);
  }

  for (std::size_t i = 0; i < right_src.names_.size(); ++i) {
    const auto& base = right_src.names_[i];
    if (base == right_key && left_key == right_key) {
      continue;
    }
    std::string out_name = base;
    if (used_names.find(out_name) != used_names.end()) {
      out_name = base + right_suffix;
      std::size_t k = 2;
      while (used_names.find(out_name) != used_names.end()) {
        out_name = fmt::format("{}{}_{}", base, right_suffix, k++);
      }
    }
    used_names.insert(out_name);
    out_cols.emplace_back(std::move(out_name), right_src.columns_[i].take(right_rows));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame DataFrame::stitch_on(std::initializer_list<std::string_view> left_features,
                               const DataFrame& right,
                               std::initializer_list<std::string_view> right_features,
                               std::string right_suffix) const {
  PLUSS_CHECK(left_features.size() > 0, "DataFrame::stitch_on: at least one left key feature is required");
  std::vector<std::string> left_keys = normalize_name_list(left_features);
  std::vector<std::string> right_keys;
  if (right_features.size() == 0) {
    right_keys = left_keys;
  } else {
    right_keys = normalize_name_list(right_features);
  }
  PLUSS_CHECK(left_keys.size() == right_keys.size(),
              "DataFrame::stitch_on: left/right key feature counts must match");

  const auto& left_src = impl_ref_();
  const auto& right_src = right.impl_ref_();

  std::vector<Thread> left_cols;
  std::vector<Thread> right_cols;
  left_cols.reserve(left_keys.size());
  right_cols.reserve(right_keys.size());
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    PLUSS_CHECK(has_feature(left_keys[i]),
                "DataFrame::stitch_on: left key feature not found '" + left_keys[i] + "'");
    PLUSS_CHECK(right.has_feature(right_keys[i]),
                "DataFrame::stitch_on: right key feature not found '" + right_keys[i] + "'");
    left_cols.push_back(features(left_keys[i]));
    right_cols.push_back(right.features(right_keys[i]));
  }

  auto make_key = [](const std::vector<Thread>& key_cols, std::int64_t row) {
    std::string k;
    k.reserve(key_cols.size() * 16);
    for (const auto& c : key_cols) {
      k += thread_scalar_string_at(c, row);
      k.push_back('\x1F');
    }
    return k;
  };

  std::unordered_map<std::string, std::vector<std::int64_t>> right_lookup;
  right_lookup.reserve(static_cast<std::size_t>(right.nrows()));
  for (std::int64_t r = 0; r < right.nrows(); ++r) {
    right_lookup[make_key(right_cols, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < nrows(); ++l) {
    const auto key = make_key(left_cols, l);
    const auto it = right_lookup.find(key);
    if (it == right_lookup.end()) {
      continue;
    }
    for (const auto rr : it->second) {
      left_rows.push_back(l);
      right_rows.push_back(rr);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(left_src.names_.size() + right_src.names_.size());
  for (std::size_t i = 0; i < left_src.names_.size(); ++i) {
    out_cols.emplace_back(left_src.names_[i], left_src.columns_[i].take(left_rows));
  }

  std::unordered_set<std::string> right_key_set(right_keys.begin(), right_keys.end());
  std::unordered_set<std::string> used_names;
  used_names.reserve(out_cols.size() + right_src.names_.size());
  for (const auto& [name, _] : out_cols) {
    used_names.insert(name);
  }

  for (std::size_t i = 0; i < right_src.names_.size(); ++i) {
    const auto& base = right_src.names_[i];
    if (right_key_set.find(base) != right_key_set.end()) {
      continue;
    }
    std::string out_name = base;
    if (used_names.find(out_name) != used_names.end()) {
      out_name = base + right_suffix;
      std::size_t k = 2;
      while (used_names.find(out_name) != used_names.end()) {
        out_name = fmt::format("{}{}_{}", base, right_suffix, k++);
      }
    }
    used_names.insert(out_name);
    out_cols.emplace_back(std::move(out_name), right_src.columns_[i].take(right_rows));
  }

  return DataFrame(std::move(out_cols));
}
