bool DataFrame::defined() const noexcept { return static_cast<bool>(impl_); }
bool DataFrame::unique() const noexcept { return impl_.unique(); }
std::int64_t DataFrame::height() const { return ncases(); }

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

std::int64_t DataFrame::ncases() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::nfeats() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }
std::int64_t DataFrame::nfeatures() const { return nfeats(); }

std::vector<Thread> DataFrame::features() const { return impl_ref_().columns_; }
DataFrame::MatrixValues DataFrame::matrix() const {
  DataFrame::MatrixValues out{};
  auto& series_out = std::get<0>(out);
  auto& note_out = std::get<1>(out);
  auto& timeline_out = std::get<2>(out);
  auto& timeindex_out = std::get<3>(out);

  std::vector<pluss::Series> series_cols{};
  std::vector<pluss::Note> note_cols{};
  std::vector<pluss::Timeline> timeline_cols{};

  for (const auto& thread : impl_ref_().columns_) {
    switch (thread.kind()) {
      case ThreadKind::Series:
        series_cols.push_back(thread.as_series());
        break;
      case ThreadKind::Note:
        note_cols.push_back(thread.as_note());
        break;
      case ThreadKind::Timeline:
        timeline_cols.push_back(thread.as_timeline());
        break;
      case ThreadKind::TimeIndex:
        timeindex_out.push_back(thread.as_timeindex());
        break;
    }
  }

  if (!series_cols.empty()) {
    series_out.push_back(pack_series_columns(series_cols, "DataFrame::matrix(series)", /*force_matrix_2d=*/true));
  }

  if (!note_cols.empty()) {
    note_out.push_back(pack_note_columns(note_cols, "DataFrame::matrix(note)", /*force_matrix_2d=*/true));
  }

  if (!timeline_cols.empty()) {
    timeline_out.push_back(
        pack_timeline_columns(timeline_cols, "DataFrame::matrix(timeline)", /*force_matrix_2d=*/true));
  }

  return out;
}
const std::vector<std::string>& DataFrame::feature_names() const { return impl_ref_().names_; }

const Thread& DataFrame::cases() const { return impl_ref_().cases_; }

DataFrame DataFrame::cases(std::int64_t row) const {
  const auto idx = normalize_row_index(row, ncases());
  return slice(idx, idx + 1);
}

DataFrame DataFrame::cases(std::pair<std::int64_t, std::int64_t> row_span) const {
  return slice(row_span.first, row_span.second);
}

std::string DataFrame::cases(std::int64_t row, std::size_t feature_idx) const {
  const auto r = normalize_row_index(row, ncases());
  PLUSS_CHECK(feature_idx < static_cast<std::size_t>(nfeats()), "DataFrame::cases(row, feature): feature index out of bounds");
  const auto& src = impl_ref_();
  return thread_scalar_string_at(src.columns_[feature_idx], r);
}

std::string DataFrame::cases(std::int64_t row, std::string_view feature_name) const {
  const auto r = normalize_row_index(row, ncases());
  return thread_scalar_string_at(features(feature_name), r);
}

pluss::Series DataFrame::cases_series() const {
  PLUSS_CHECK(cases().kind() == ThreadKind::Series, "DataFrame::cases_series requires a Series-backed case axis");
  return cases().as_series();
}

DataFrame DataFrame::cloc(std::int64_t row) const {
  return cases(row);
}

DataFrame DataFrame::cloc(std::int64_t start, std::int64_t stop) const {
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

std::string DataFrame::title() const {
  if (!defined()) {
    return {};
  }
  return impl_ref_().title_;
}

DataFrame DataFrame::title(std::string title) const {
  auto out = *this;
  out.title_(std::move(title));
  return out;
}

DataFrame& DataFrame::title_(std::string title) {
  mut_impl_().title_ = std::move(title);
  return *this;
}

bool DataFrame::has_feature(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_idx_.find(owned_name(name)) != src.name_to_idx_.end();
}

Thread DataFrame::features(std::string_view name) {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'");
  return src.columns_[it->second].with_display_labels(src.cases_).with_display_name(std::move(feature_name));
}

Thread DataFrame::features(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'");
  return src.columns_[it->second].with_display_labels(src.cases_).with_display_name(std::move(feature_name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), features(name));
}

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) {
  return select(names);
}

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) const {
  return select(names);
}

DataFrame DataFrame::features(pluss::msharp::RangeView::Of<std::string_view> names) {
  return select(names);
}

DataFrame DataFrame::features(pluss::msharp::RangeView::Of<std::string_view> names) const {
  return select(names);
}

DataFrame DataFrame::features(StackArray<std::string_view> names) {
  return select(pluss::msharp::RangeView::Of<std::string_view>(names.data().values));
}

DataFrame DataFrame::features(StackArray<std::string_view> names) const {
  return select(pluss::msharp::RangeView::Of<std::string_view>(names.data().values));
}

DataFrame DataFrame::select(pluss::msharp::RangeView::Of<std::string_view> names) const {
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

DataFrame DataFrame::select_like(std::string_view needle, bool case_sensitive) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  const auto token = owned_name(needle);
  const auto token_cmp = case_sensitive ? token : lower_copy(token);
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& name = src.names_[i];
    const auto name_cmp = case_sensitive ? name : lower_copy(name);
    if (name_cmp.find(token_cmp) == std::string::npos) {
      continue;
    }
    out_names.push_back(name);
    out_cols.push_back(src.columns_[i]);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::select_regex(std::string_view pattern) const {
  const auto pat = owned_name(pattern);
  std::regex re;
  try {
    re = std::regex(pat, std::regex::ECMAScript);
  } catch (const std::regex_error& e) {
    PLUSS_FAIL("DataFrame::select_regex: invalid regex '" + pat + "': " + std::string(e.what()));
  }

  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& name = src.names_[i];
    if (!std::regex_search(name, re)) {
      continue;
    }
    out_names.push_back(name);
    out_cols.push_back(src.columns_[i]);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::series_features() const {
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

DataFrame DataFrame::note_features() const {
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

DataFrame DataFrame::timeline_features() const {
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

DataFrame DataFrame::cast_series(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::cast_note(pluss::msharp::RangeView::Of<std::string_view> features) const {
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

DataFrame DataFrame::cast_timeline(pluss::msharp::RangeView::Of<std::string_view> features) const {
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

DataFrame DataFrame::cast_timeindex(pluss::msharp::RangeView::Of<std::string_view> features) const {
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

DataFrame DataFrame::astype(pluss::msharp::RangeView::Of<std::string_view> features,
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

DataFrame DataFrame::drop(pluss::msharp::RangeView::Of<std::string_view> names) const {
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

DataFrame DataFrame::reorder(pluss::msharp::RangeView::Of<std::string_view> names) const {
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

DataFrame DataFrame::arrange(pluss::msharp::RangeView::Of<std::string_view> features,
                             bool ascending,
                             bool nulls_last) const {
  std::vector<ArrangeSpec> specs;
  specs.reserve(features.size());
  for (const auto name : features) {
    specs.push_back({owned_name(name), ascending, nulls_last});
  }
  return arrange_impl_frame(*this, specs);
}

DataFrame DataFrame::arrange(pluss::msharp::RangeView::Of<ArrangeSpec> specs) const {
  return arrange_impl_frame(*this, std::vector<ArrangeSpec>(specs.begin(), specs.end()));
}

DataFrame DataFrame::sort_values(pluss::msharp::RangeView::Of<std::string_view> features,
                                 bool ascending,
                                 bool nulls_last) const {
  return arrange(features, ascending, nulls_last);
}

DataFrame DataFrame::sort_values(pluss::msharp::RangeView::Of<ArrangeSpec> specs) const {
  return arrange(specs);
}

DataFrame DataFrame::sort_cases(bool ascending, bool nulls_last) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(ncases()));
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

pluss::Series DataFrame::repeated_cases(pluss::msharp::RangeView::Of<std::string_view> features) const {
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

  auto out = pluss::Series::zeros({ncases()}, pluss::codem::ScalarType::Bool);
  auto* mask = reinterpret_cast<bool*>(out.raw_data());

  std::unordered_set<std::string> seen_keys;
  seen_keys.reserve(static_cast<std::size_t>(ncases()));
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

DataFrame DataFrame::unique_cases(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return filter(repeated_cases(features).logical_not());
}

DataFrame DataFrame::set_cases(std::string_view feature_name, bool drop_feature) const {
  auto out = with_cases(features(feature_name));
  if (drop_feature) {
    out = out.drop({feature_name});
  }
  return out;
}

DataFrame DataFrame::reset_cases(std::string case_feature) const {
  auto out = set_features(case_feature, cases());
  return out.with_cases(default_cases(out.ncases()));
}

DataFrame DataFrame::align_cases(const Thread& new_cases) const {
  PLUSS_CHECK(new_cases.defined(), "DataFrame::align_cases: new cases are undefined");
  std::unordered_map<std::string, std::int64_t> old_pos;
  old_pos.reserve(static_cast<std::size_t>(ncases()));
  for (std::int64_t i = 0; i < ncases(); ++i) {
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

DataFrame::LetSlot DataFrame::set(std::string_view name) {
  return LetSlot(*this, owned_name(name));
}

DataFrame::LetSlots DataFrame::set(pluss::msharp::RangeView::Of<std::string_view> names) {
  return LetSlots(*this, normalize_name_list(names));
}

DataFrame DataFrame::set_features(std::string_view name, const Thread& value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(value.defined(), "DataFrame::set_features: input feature is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(value.size() == src.nrows_, "DataFrame::set_features: row count mismatch");
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  const auto key = owned_name(name);
  const auto it = src.name_to_idx_.find(key);
  if (it == src.name_to_idx_.end()) {
    out_names.push_back(key);
    out_cols.push_back(value);
  } else {
    out_cols[it->second] = value;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame& DataFrame::set_features_(std::string_view name, const Thread& value) {
  auto& w = mut_impl_();
  PLUSS_CHECK(value.defined(), "DataFrame::set_features_: input feature is undefined");
  if (!w.columns_.empty()) {
    PLUSS_CHECK(value.size() == w.nrows_, "DataFrame::set_features_: row count mismatch");
  }

  const auto key = owned_name(name);
  const auto it = w.name_to_idx_.find(key);
  if (it == w.name_to_idx_.end()) {
    w.names_.push_back(key);
    w.columns_.push_back(value);
  } else {
    w.columns_[it->second] = value;
  }
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::set_features(pluss::msharp::RangeView::Of<std::string_view> names,
                                  pluss::msharp::RangeView::Of<Thread> values) const {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features: names/value size mismatch");
  DataFrame out = *this;
  for (std::size_t i = 0; i < names.size(); ++i) {
    out = out.set_features(names[i], values[i]);
  }
  return out;
}

DataFrame& DataFrame::set_features_(pluss::msharp::RangeView::Of<std::string_view> names,
                                    pluss::msharp::RangeView::Of<Thread> values) {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features_: names/value size mismatch");
  for (std::size_t i = 0; i < names.size(); ++i) {
    set_features_(names[i], values[i]);
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
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> renames) const {
  DataFrame out = *this;
  for (const auto& [old_name, new_name] : renames) {
    out = out.rename(old_name, std::string(new_name));
  }
  return out;
}

DataFrame DataFrame::relabel_features(pluss::msharp::RangeView::Of<std::string_view> feature_names) const {
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

DataFrame DataFrame::set_feature_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const {
  return relabel_features(feature_names);
}

DataFrame DataFrame::set_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const {
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
  right_lookup.reserve(static_cast<std::size_t>(right.ncases()));
  for (std::int64_t r = 0; r < right.ncases(); ++r) {
    right_lookup[thread_scalar_string_at(right_col, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < ncases(); ++l) {
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

DataFrame DataFrame::stitch_on(pluss::msharp::RangeView::Of<std::string_view> left_features,
                               const DataFrame& right,
                               pluss::msharp::RangeView::Of<std::string_view> right_features,
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
  right_lookup.reserve(static_cast<std::size_t>(right.ncases()));
  for (std::int64_t r = 0; r < right.ncases(); ++r) {
    right_lookup[make_key(right_cols, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < ncases(); ++l) {
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

DataFrame DataFrame::stitch_on(std::initializer_list<std::string_view> left_features,
                               const DataFrame& right,
                               std::initializer_list<std::string_view> right_features,
                               std::string right_suffix) const {
  return stitch_on(pluss::msharp::RangeView::Of<std::string_view>(left_features),
                   right,
                   pluss::msharp::RangeView::Of<std::string_view>(right_features),
                   std::move(right_suffix));
}
