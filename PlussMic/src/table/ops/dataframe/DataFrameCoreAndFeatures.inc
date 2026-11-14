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

std::int64_t DataFrame::ncases() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::nfeatures() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }
std::pair<std::int64_t, std::int64_t> DataFrame::shape() const {
  return {ncases(), nfeatures()};
}
// height()/width() are convenience aliases for ncases()/nfeatures() (equivalently,
// shape().first/shape().second): height is the number of cases (rows), width is the
// number of features (columns) — the same convention used by shape().
std::int64_t DataFrame::height() const { return ncases(); }
std::int64_t DataFrame::width() const { return nfeatures(); }
// length() is the total cell count: width() * height() (equivalently
// nfeatures() * ncases()) — not to be confused with
// DataFrameNoteAccessor::length(), the unrelated per-cell text-length accessor
// reached via .nt().length().
std::int64_t DataFrame::length() const { return width() * height(); }

DataFrame::DTypes::DTypes(std::vector<Entry> entries) : entries_(std::move(entries)) {}

std::int64_t DataFrame::DTypes::size() const noexcept {
  return static_cast<std::int64_t>(entries_.size());
}

bool DataFrame::DTypes::empty() const noexcept { return entries_.empty(); }

bool DataFrame::DTypes::contains(std::string_view feature) const noexcept {
  return std::any_of(entries_.begin(), entries_.end(), [&](const Entry& entry) {
    return entry.first == feature;
  });
}

const std::string& DataFrame::DTypes::at(std::string_view feature) const {
  const auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
    return entry.first == feature;
  });
  PLUSS_CHECK(it != entries_.end(), "DataFrame::DTypes::at: unknown feature '" + std::string(feature) + "'", std::string(feature), pluss::exception::map_keys(entries_));
  return it->second;
}

const std::string& DataFrame::DTypes::operator[](std::string_view feature) const {
  return at(feature);
}

const std::vector<DataFrame::DTypes::Entry>& DataFrame::DTypes::entries() const noexcept {
  return entries_;
}

std::vector<std::string> DataFrame::DTypes::payloads() const {
  std::vector<std::string> out;
  out.reserve(entries_.size());
  for (const auto& entry : entries_) {
    out.push_back(entry.second);
  }
  return out;
}

DataFrame::DTypes::const_iterator DataFrame::DTypes::begin() const noexcept {
  return entries_.begin();
}

DataFrame::DTypes::const_iterator DataFrame::DTypes::end() const noexcept {
  return entries_.end();
}

DataFrame::MatrixValues DataFrame::matrix() const {
  DataFrame::MatrixValues out{};
  auto& series_out = std::get<0>(out);
  auto& note_out = std::get<1>(out);
  auto& timeline_out = std::get<2>(out);
  auto& duration_out = std::get<3>(out);

  std::vector<pluss::Series> series_cols{};
  std::vector<pluss::Note> note_cols{};
  std::vector<pluss::Timeline> timeline_cols{};
  std::vector<pluss::Duration> duration_cols{};

  for (const auto& slate : impl_ref_().columns_) {
    switch (slate.kind()) {
      case SlateKind::Series:
        series_cols.push_back(slate.as_series());
        break;
      case SlateKind::Note:
        note_cols.push_back(slate.as_note());
        break;
      case SlateKind::Timeline:
        timeline_cols.push_back(slate.as_timeline());
        break;
      case SlateKind::Duration:
        duration_cols.push_back(slate.as_duration());
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

  if (!duration_cols.empty()) {
    duration_out.push_back(
        pack_duration_columns(duration_cols, "DataFrame::matrix(duration)", /*force_matrix_2d=*/true));
  }

  return out;
}
const std::vector<std::string>& DataFrame::features() const { return impl_ref_().names_; }

DataFrame::DTypes DataFrame::ptypes() const {
  const auto& src = impl_ref_();
  std::vector<DTypes::Entry> entries;
  entries.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    entries.emplace_back(src.names_[i], src.columns_[i].ptype());
  }
  return DTypes(std::move(entries));
}

DataFrame::DTypes DataFrame::dtypes() const {
  const auto& src = impl_ref_();
  std::vector<DTypes::Entry> entries;
  entries.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    entries.emplace_back(src.names_[i], col.ptype() + "<" + col.dtype() + ">");
  }
  return DTypes(std::move(entries));
}

const Slate& DataFrame::case_labels_() const { return impl_ref_().cases_; }

const Slate& DataFrame::caselbl() const { return case_labels_(); }

DataFrameLabelAccessor DataFrame::labels() const {
  return DataFrameLabelAccessor(*this);
}

DataFrameFeatureAxisAccessor DataFrame::feataxis() const {
  return DataFrameFeatureAxisAccessor(*this);
}

DataFrameCaseStudyAccessor DataFrame::case_study() const {
  return DataFrameCaseStudyAccessor(*this);
}

DataFrameCastAccessor DataFrame::cast() const {
  return DataFrameCastAccessor(*this);
}

DataFrameCastAccessor DataFrame::cast(const SlateCastOptions& options) const {
  return DataFrameCastAccessor(*this, options);
}

DataFrame DataFrame::caselbl(const Slate& labels) const {
  return with_cases(labels);
}

DataFrame& DataFrame::caselbl_(const Slate& labels) {
  return with_cases_(labels);
}

namespace {

std::string selector_label_from_scalar_slate_(const Slate& value, std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": selector slate is undefined");
  PLUSS_CHECK(value.size() == 1, std::string(where) + ": selector slate must contain exactly one label");
  return slate_scalar_string_at(value, 0);
}

std::vector<std::string> selector_labels_from_slate_(const Slate& values, std::string_view where) {
  PLUSS_CHECK(values.defined(), std::string(where) + ": selector slate is undefined");
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(values.size()));
  for (std::int64_t i = 0; i < values.size(); ++i) {
    out.push_back(slate_scalar_string_at(values, i));
  }
  return out;
}

std::unordered_map<std::string, std::vector<std::int64_t>> build_case_lookup_(const DataFrame& frame) {
  std::unordered_map<std::string, std::vector<std::int64_t>> lookup;
  lookup.reserve(static_cast<std::size_t>(std::max<std::int64_t>(8, frame.ncases() * 2)));
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    lookup[slate_scalar_string_at(frame.labels().values(), i)].push_back(i);
  }
  return lookup;
}

std::size_t level_index_(const detail::DataFrameImpl& src, std::string_view level, std::string_view where) {
  PLUSS_CHECK(src.case_axis_.defined(), std::string(where) + ": current DataFrame has no case levels");
  const auto name = owned_name(level);
  PLUSS_CHECK(!name.empty(), std::string(where) + ": level name cannot be empty");
  for (std::size_t i = 0; i < src.case_axis_.levels.size(); ++i) {
    if (src.case_axis_.levels[i].name == name) {
      return i;
    }
  }
  PLUSS_FAIL(std::string(where) + ": unknown case level '" + name + "'");
}

std::string case_tuple_key_(const detail::CaseAxis& axis, std::int64_t row) {
  std::string key;
  for (const auto& level : axis.levels) {
    key += detail::valid_at(level.values, row) ? slate_scalar_string_at(level.values, row) : std::string("NULL");
    key.append("\x1E", 1);
  }
  return key;
}

std::vector<std::string> composite_case_labels_(const detail::CaseAxis& axis,
                                                std::int64_t nrows,
                                                std::string_view separator = "\x1F") {
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(nrows));
  for (std::int64_t row = 0; row < nrows; ++row) {
    std::string label;
    for (std::size_t i = 0; i < axis.levels.size(); ++i) {
      if (i > 0) {
        label.append(separator.data(), separator.size());
      }
      const auto& values = axis.levels[i].values;
      label += detail::valid_at(values, row) ? slate_scalar_string_at(values, row) : std::string("NULL");
    }
    labels.push_back(std::move(label));
  }
  return labels;
}

Slate label_key_mask_(const std::vector<std::string>& keys,
                       const Slate& labels,
                       bool all_repeated,
                       bool keep_first,
                       bool keep_last,
                       std::string_view where) {
  PLUSS_CHECK(!(keep_first && keep_last), std::string(where) + ": keep_first and keep_last cannot both be true");
  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_key;
  rows_by_key.reserve(keys.size());
  for (std::int64_t row = 0; row < static_cast<std::int64_t>(keys.size()); ++row) {
    rows_by_key[keys[static_cast<std::size_t>(row)]].push_back(row);
  }
  std::vector<bool> mask(keys.size(), false);
  for (const auto& [_, rows] : rows_by_key) {
    if (rows.size() <= 1) {
      continue;
    }
    if (all_repeated) {
      for (const auto row : rows) {
        mask[static_cast<std::size_t>(row)] = true;
      }
      continue;
    }
    const std::size_t start = keep_first ? 1 : 0;
    const std::size_t stop = keep_last ? rows.size() - 1 : rows.size();
    for (std::size_t i = start; i < stop; ++i) {
      mask[static_cast<std::size_t>(rows[i])] = true;
    }
  }
  return Slate(bool_series_from_keep(mask)).label().set(labels);
}

std::vector<std::string> selected_label_keys_(const detail::DataFrameImpl& src,
                                              const Slate& flat_labels,
                                              pluss::msharp::RangeView::Of<std::string_view> names,
                                              std::string_view where) {
  if (names.size() == 0) {
    std::vector<std::string> keys;
    keys.reserve(static_cast<std::size_t>(flat_labels.size()));
    for (std::int64_t row = 0; row < flat_labels.size(); ++row) {
      keys.push_back(detail::valid_at(flat_labels, row) ? slate_scalar_string_at(flat_labels, row) : std::string{"NULL"});
    }
    return keys;
  }
  PLUSS_CHECK(src.case_axis_.defined(), std::string(where) + ": current DataFrame has no structured labels");
  std::vector<std::size_t> positions;
  positions.reserve(names.size());
  for (const auto name : names) {
    positions.push_back(level_index_(src, name, where));
  }
  std::vector<std::string> keys;
  keys.reserve(static_cast<std::size_t>(src.nrows_));
  for (std::int64_t row = 0; row < src.nrows_; ++row) {
    std::string key;
    for (const auto position : positions) {
      if (!key.empty()) key.append("\x1E", 1);
      const auto& values = src.case_axis_.levels[position].values;
      key += detail::valid_at(values, row) ? slate_scalar_string_at(values, row) : std::string{"NULL"};
    }
    keys.push_back(std::move(key));
  }
  return keys;
}

detail::CaseAxis take_case_axis_(const detail::CaseAxis& axis, pluss::msharp::RangeView::Int indices) {
  detail::CaseAxis out;
  out.levels.reserve(axis.levels.size());
  for (const auto& level : axis.levels) {
    out.levels.push_back({level.name, level.values.take(indices)});
  }
  return out;
}

detail::CaseAxis filter_case_axis_(const detail::CaseAxis& axis, const pluss::Series& mask) {
  detail::CaseAxis out;
  out.levels.reserve(axis.levels.size());
  for (const auto& level : axis.levels) {
    out.levels.push_back({level.name, level.values.filter(mask)});
  }
  return out;
}

std::vector<std::int64_t> resolve_case_positions_(const DataFrame& frame, const detail::CaseSelector& selector) {
  std::vector<std::int64_t> indices;
  switch (selector.kind) {
    case detail::CaseSelector::Kind::PositionScalar:
      indices.push_back(normalize_row_index(selector.first, frame.ncases()));
      return indices;
    case detail::CaseSelector::Kind::PositionSlice: {
      const auto [start, stop] = normalize_slice(selector.first, selector.second, frame.ncases());
      indices.reserve(static_cast<std::size_t>(stop - start));
      for (std::int64_t i = start; i < stop; ++i) {
        indices.push_back(i);
      }
      return indices;
    }
    case detail::CaseSelector::Kind::PositionList:
      indices.reserve(selector.positions.size());
      for (const auto row : selector.positions) {
        indices.push_back(normalize_row_index(row, frame.ncases()));
      }
      return indices;
    case detail::CaseSelector::Kind::Mask:
      PLUSS_FAIL("resolve_case_positions_: mask selectors must be handled by DataFrame::case_study directly");
    case detail::CaseSelector::Kind::LabelScalar:
    case detail::CaseSelector::Kind::LabelList:
    case detail::CaseSelector::Kind::LabelSlice:
      break;
  }

  const auto lookup = build_case_lookup_(frame);
  if (selector.kind == detail::CaseSelector::Kind::LabelScalar) {
    const auto& key = selector.label_values.front();
    const auto it = lookup.find(key);
    PLUSS_CHECK(it != lookup.end(), "DataFrame::case_study: case label not found '" + key + "'", key, pluss::exception::map_keys(lookup));
    return it->second;
  }

  if (selector.kind == detail::CaseSelector::Kind::LabelList) {
    for (const auto& key : selector.label_values) {
      const auto it = lookup.find(key);
      PLUSS_CHECK(it != lookup.end(), "DataFrame::case_study: case label not found '" + key + "'", key, pluss::exception::map_keys(lookup));
      indices.insert(indices.end(), it->second.begin(), it->second.end());
    }
    return indices;
  }

  const auto& start_key = selector.label_values.at(0);
  const auto& stop_key = selector.label_values.at(1);
  const auto start_it = lookup.find(start_key);
  const auto stop_it = lookup.find(stop_key);
  PLUSS_CHECK(start_it != lookup.end(), "DataFrame::case_study: case label not found '" + start_key + "'", start_key, pluss::exception::map_keys(lookup));
  PLUSS_CHECK(stop_it != lookup.end(), "DataFrame::case_study: case label not found '" + stop_key + "'", stop_key, pluss::exception::map_keys(lookup));
  const auto start = start_it->second.front();
  const auto stop = stop_it->second.back();
  if (stop < start) {
    return {};
  }
  indices.reserve(static_cast<std::size_t>(stop - start + 1));
  for (std::int64_t i = start; i <= stop; ++i) {
    indices.push_back(i);
  }
  return indices;
}

std::vector<std::size_t> resolve_feature_positions_(const DataFrame& frame, const detail::FeatureSelector& selector) {
  std::vector<std::size_t> indices;
  switch (selector.kind) {
    case detail::FeatureSelector::Kind::All:
      indices.resize(static_cast<std::size_t>(frame.nfeatures()));
      std::iota(indices.begin(), indices.end(), static_cast<std::size_t>(0));
      return indices;
    case detail::FeatureSelector::Kind::PositionScalar:
      indices.push_back(static_cast<std::size_t>(normalize_row_index(selector.first, frame.nfeatures())));
      return indices;
    case detail::FeatureSelector::Kind::PositionSlice: {
      const auto [start, stop] = normalize_slice(selector.first, selector.second, frame.nfeatures());
      indices.reserve(static_cast<std::size_t>(stop - start));
      for (std::int64_t i = start; i < stop; ++i) {
        indices.push_back(static_cast<std::size_t>(i));
      }
      return indices;
    }
    case detail::FeatureSelector::Kind::PositionList:
      indices.reserve(selector.positions.size());
      for (const auto pos : selector.positions) {
        indices.push_back(static_cast<std::size_t>(normalize_row_index(pos, frame.nfeatures())));
      }
      return indices;
    case detail::FeatureSelector::Kind::NameScalar:
    case detail::FeatureSelector::Kind::NameList:
    case detail::FeatureSelector::Kind::NameSlice:
      break;
  }

  const auto& names = frame.features();
  std::unordered_map<std::string, std::size_t> by_name;
  by_name.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    by_name.emplace(names[i], i);
  }

  if (selector.kind == detail::FeatureSelector::Kind::NameScalar) {
    const auto& key = selector.name_values.front();
    const auto it = by_name.find(key);
    PLUSS_CHECK(it != by_name.end(), "DataFrame::case_study: unknown feature '" + key + "'", key, pluss::exception::map_keys(by_name));
    indices.push_back(it->second);
    return indices;
  }

  if (selector.kind == detail::FeatureSelector::Kind::NameList) {
    indices.reserve(selector.name_values.size());
    for (const auto& key : selector.name_values) {
      const auto it = by_name.find(key);
      PLUSS_CHECK(it != by_name.end(), "DataFrame::case_study: unknown feature '" + key + "'", key, pluss::exception::map_keys(by_name));
      indices.push_back(it->second);
    }
    return indices;
  }

  const auto& start_key = selector.name_values.at(0);
  const auto& stop_key = selector.name_values.at(1);
  const auto start_it = by_name.find(start_key);
  const auto stop_it = by_name.find(stop_key);
  PLUSS_CHECK(start_it != by_name.end(), "DataFrame::case_study: unknown feature '" + start_key + "'", start_key, pluss::exception::map_keys(by_name));
  PLUSS_CHECK(stop_it != by_name.end(), "DataFrame::case_study: unknown feature '" + stop_key + "'", stop_key, pluss::exception::map_keys(by_name));
  const auto start = static_cast<std::int64_t>(start_it->second);
  const auto stop = static_cast<std::int64_t>(stop_it->second);
  if (stop < start) {
    return {};
  }
  indices.reserve(static_cast<std::size_t>(stop - start + 1));
  for (std::int64_t i = start; i <= stop; ++i) {
    indices.push_back(static_cast<std::size_t>(i));
  }
  return indices;
}

DataFrame project_feature_positions_(const DataFrame& frame, pluss::RangeView<std::size_t> indices) {
  const auto& source_names = frame.features();
  std::vector<std::string_view> names;
  names.reserve(indices.size());
  for (const auto idx : indices) {
    PLUSS_CHECK(idx < source_names.size(), "DataFrame::case_study: feature index out of bounds");
    names.push_back(source_names[idx]);
  }
  return frame.features(pluss::msharp::RangeView::Of<std::string_view>(
      std::span<const std::string_view>(names.data(), names.size())));
}

} // namespace

bool DataFrame::has_caselbls() const {
  return impl_ref_().case_axis_.defined();
}

std::vector<std::string> DataFrame::caselbls() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out;
  out.reserve(src.case_axis_.levels.size());
  for (const auto& level : src.case_axis_.levels) {
    out.push_back(level.name);
  }
  return out;
}

Slate DataFrame::caselbl(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.case_axis_.levels[level_index_(src, name, "DataFrame::caselbl")].values;
}

Slate DataFrame::caselbl(std::int64_t position) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::caselbl: current DataFrame has no case labels");
  auto pos = position < 0 ? position + static_cast<std::int64_t>(src.case_axis_.levels.size()) : position;
  PLUSS_CHECK(pos >= 0 && pos < static_cast<std::int64_t>(src.case_axis_.levels.size()),
              "DataFrame::caselbl: case-label position out of bounds");
  return src.case_axis_.levels[static_cast<std::size_t>(pos)].values;
}

DataFrameLabelAccessor::DataFrameLabelAccessor(const DataFrame& frame)
    : frame_(frame) {}

Slate DataFrameLabelAccessor::values() const {
  return frame_.caselbl();
}

DataFrame DataFrameLabelAccessor::set(const Slate& labels) const {
  PLUSS_CHECK(labels.defined(),
              "DataFrameLabelAccessor::set: labels are undefined");
  PLUSS_CHECK(labels.size() == frame_.ncases(),
              "DataFrameLabelAccessor::set: label count must match the frame's case count");
  return frame_.caselbl(labels);
}

DataFrame DataFrameLabelAccessor::set(const pluss::Note& labels) const {
  return set(Slate(labels));
}

bool DataFrameLabelAccessor::has() const {
  return frame_.has_caselbls();
}

std::vector<std::string> DataFrameLabelAccessor::names() const {
  return frame_.caselbls();
}

Slate DataFrameLabelAccessor::part(std::string_view name) const {
  return frame_.caselbl(name);
}

Slate DataFrameLabelAccessor::part(std::int64_t position) const {
  return frame_.caselbl(position);
}

DataFrame DataFrameLabelAccessor::set(std::string_view feature_name, bool drop_feature) const {
  return frame_.set_caselbl(feature_name, drop_feature);
}

DataFrame DataFrameLabelAccessor::set(std::initializer_list<std::string_view> feature_names,
                                      bool drop_features,
                                      std::string_view separator) const {
  return frame_.set_caselbl(feature_names, drop_features, separator);
}

DataFrame DataFrameLabelAccessor::set(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                                      bool drop_features,
                                      std::string_view separator) const {
  return frame_.set_caselbl(feature_names, drop_features, separator);
}

DataFrame DataFrameLabelAccessor::set(const DataFrame::SetLabelOptions& options) const {
  return frame_.set_caselbl(options);
}

DataFrame DataFrameLabelAccessor::reset(std::string case_feature) const {
  return frame_.reset_caselbl(std::move(case_feature));
}

DataFrame DataFrameLabelAccessor::reset(const DataFrame::ResetLabelOptions& options) const {
  return frame_.reset_caselbl(options);
}

DataFrame DataFrameLabelAccessor::sort(bool ascending, bool nulls_last) const {
  return frame_.sort_caselbl(ascending, nulls_last);
}

DataFrame DataFrameLabelAccessor::sort(const DataFrame::SortLabelOptions& options) const {
  return frame_.sort_caselbl(options);
}

DataFrame DataFrameLabelAccessor::align(const Slate& new_label) const {
  return frame_.align_caselbl(new_label);
}

DataFrame DataFrameLabelAccessor::align(const Slate& new_label, std::optional<double> fill) const {
  return frame_.align_caselbl(new_label, fill);
}

Slate DataFrameLabelAccessor::unite(const Slate& other) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().unite(other);
}

Slate DataFrameLabelAccessor::intersect(const Slate& other) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().intersect(other);
}

Slate DataFrameLabelAccessor::diff(const Slate& other) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().diff(other);
}

Slate DataFrameLabelAccessor::symdiff(const Slate& other) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().symdiff(other);
}

Slate DataFrameLabelAccessor::find(const Slate& targets) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().find(targets);
}

Slate DataFrameLabelAccessor::pos(double value) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().pos(value);
}

Slate DataFrameLabelAccessor::pos(std::string_view value) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().pos(value);
}

Slate DataFrameLabelAccessor::slice(std::string_view start, std::string_view stop) const {
  const auto labels = frame_.labels().values();
  return labels.label().set(labels).label().slice(start, stop);
}

DataFrame DataFrameLabelAccessor::rename(pluss::msharp::RangeView::Of<DataFrame::LabelRename> renames) const {
  return frame_.rename_caselbls(renames);
}

DataFrame DataFrameLabelAccessor::rename(std::initializer_list<DataFrame::LabelRename> renames) const {
  return frame_.rename_caselbls(renames);
}

DataFrame DataFrameLabelAccessor::order(pluss::msharp::RangeView::Of<std::string_view> names) const {
  return frame_.order_caselbls(names);
}

DataFrame DataFrameLabelAccessor::order(std::initializer_list<std::string_view> names) const {
  return frame_.order_caselbls(names);
}

DataFrame DataFrameLabelAccessor::names(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto old_names = frame_.caselbls();
  PLUSS_CHECK(names.size() == old_names.size(),
              "DataFrame::labels().names: expected one name per current label level");
  std::vector<DataFrame::LabelRename> renames;
  renames.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    renames.push_back(DataFrame::LabelRename{old_names[i], owned_name(names[i])});
  }
  return frame_.rename_caselbls(renames);
}

DataFrame DataFrameLabelAccessor::names(std::initializer_list<std::string_view> names) const {
  return this->names(pluss::msharp::RangeView::Of<std::string_view>(names));
}

DataFrame DataFrameLabelAccessor::drop(pluss::msharp::RangeView::Of<std::string_view> names) const {
  return frame_.drop_caselbls(names);
}

DataFrame DataFrameLabelAccessor::drop(std::initializer_list<std::string_view> names) const {
  return frame_.drop_caselbls(names);
}

DataFrame DataFrameLabelAccessor::drop(const DataFrame::LabelDropOptions& options) const {
  PLUSS_CHECK(!(options.duplicated && options.repeated),
              "DataFrame::labels().drop: use either duplicated or repeated, not both");
  if (!options.duplicated && !options.repeated) {
    if (options.names.empty()) {
      return frame_;
    }
    std::vector<std::string_view> views;
    views.reserve(options.names.size());
    for (const auto& name : options.names) {
      views.push_back(name);
    }
    return frame_.drop_caselbls(pluss::msharp::RangeView::Of<std::string_view>(
        std::span<const std::string_view>(views.data(), views.size())));
  }
  std::vector<std::string_view> views;
  views.reserve(options.names.size());
  for (const auto& name : options.names) {
    views.push_back(name);
  }
  const auto labels = frame_.labels().values();
  const auto keys = selected_label_keys_(
      frame_.impl_ref_(),
      labels,
      pluss::msharp::RangeView::Of<std::string_view>(std::span<const std::string_view>(views.data(), views.size())),
      "DataFrame::labels().drop");
  auto mask = label_key_mask_(keys, labels, options.repeated, options.keep_first, options.keep_last,
                              "DataFrame::labels().drop");
  return frame_.filter(mask.as_series().logical_not());
}

Slate DataFrameLabelAccessor::duplicated(pluss::msharp::RangeView::Of<std::string_view> names,
                                          bool keep_first,
                                          bool keep_last) const {
  const auto labels = frame_.labels().values();
  const auto keys = selected_label_keys_(frame_.impl_ref_(), labels, names, "DataFrame::labels().duplicated");
  return label_key_mask_(keys, labels, false, keep_first, keep_last, "DataFrame::labels().duplicated")
      .name("duplicated");
}

Slate DataFrameLabelAccessor::duplicated(std::initializer_list<std::string_view> names,
                                          bool keep_first,
                                          bool keep_last) const {
  return duplicated(pluss::msharp::RangeView::Of<std::string_view>(names), keep_first, keep_last);
}

Slate DataFrameLabelAccessor::duplicated(bool keep_first, bool keep_last) const {
  return frame_.labels().values().duplicated(keep_first, keep_last)
      .label().set(frame_.labels().values())
      .name("duplicated");
}

Slate DataFrameLabelAccessor::repeated(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto labels = frame_.labels().values();
  const auto keys = selected_label_keys_(frame_.impl_ref_(), labels, names, "DataFrame::labels().repeated");
  return label_key_mask_(keys, labels, true, false, false, "DataFrame::labels().repeated")
      .name("repeated");
}

Slate DataFrameLabelAccessor::repeated(std::initializer_list<std::string_view> names) const {
  return repeated(pluss::msharp::RangeView::Of<std::string_view>(names));
}

Slate DataFrameLabelAccessor::repeated() const {
  return frame_.labels().values().repeated().label().set(frame_.labels().values()).name("repeated");
}

Slate DataFrameLabelAccessor::distinct(bool dropnull) const {
  return frame_.labels().values().distinct(dropnull).name("distinct");
}

Slate DataFrameLabelAccessor::freq(std::int64_t top_n, bool dropnull) const {
  return detail::freq_count_slate(frame_.labels().values(), top_n, dropnull, "freq");
}

Slate DataFrameLabelAccessor::has(double value) const {
  return frame_.labels().values().has(value).label().set(frame_.labels().values()).name("has");
}

Slate DataFrameLabelAccessor::has(std::string_view value) const {
  return frame_.labels().values().has(value).label().set(frame_.labels().values()).name("has");
}

Slate DataFrameLabelAccessor::has(const Slate& values) const {
  return frame_.labels().values().has(values).label().set(frame_.labels().values()).name("has");
}

Slate DataFrameLabelAccessor::fillnull(double value) const {
  return frame_.labels().values().fillnull(value);
}

Slate DataFrameLabelAccessor::fillnull(std::string_view value) const {
  return frame_.labels().values().fillnull(value);
}

Slate DataFrameLabelAccessor::fillnull(const pluss::Timeline& value) const {
  return frame_.labels().values().fillnull(value);
}

Slate DataFrameLabelAccessor::isnull() const {
  return frame_.labels().values().isnull().label().set(frame_.labels().values()).name("isnull");
}

Slate DataFrameLabelAccessor::notnull() const {
  return frame_.labels().values().notnull().label().set(frame_.labels().values()).name("notnull");
}

DataFrame DataFrameLabelAccessor::map(const std::function<SlateValue(const SlateValue&)>& fn) const {
  const auto& src = frame_.impl_ref_();
  PLUSS_CHECK(!src.case_axis_.defined(),
              "DataFrame::labels().map: structured labels are not supported; map a label part instead");
  return frame_.caselbl(frame_.caselbl().map(fn));
}

Slate DataFrameLabelAccessor::take(pluss::msharp::RangeView::Int indices) const {
  std::vector<std::int64_t> owned;
  owned.reserve(indices.size());
  for (const auto idx : indices) {
    owned.push_back(idx);
  }
  return frame_.labels().values().take(std::span<const std::int64_t>(owned.data(), owned.size()));
}

Slate DataFrameLabelAccessor::slice(std::int64_t start, std::int64_t stop) const {
  return frame_.labels().values().slice(start, stop);
}

DataFrame DataFrameLabelAccessor::swap(std::string_view left, std::string_view right) const {
  const auto current = names();
  const auto lhs = owned_name(left);
  const auto rhs = owned_name(right);
  PLUSS_CHECK(lhs != rhs, "DataFrame::labels().swap: label names must differ");
  std::vector<std::string> ordered = current;
  auto lit = std::find(ordered.begin(), ordered.end(), lhs);
  auto rit = std::find(ordered.begin(), ordered.end(), rhs);
  PLUSS_CHECK(lit != ordered.end(), "DataFrame::labels().swap: unknown label '" + lhs + "'", lhs, pluss::exception::map_keys(ordered));
  PLUSS_CHECK(rit != ordered.end(), "DataFrame::labels().swap: unknown label '" + rhs + "'", rhs, pluss::exception::map_keys(ordered));
  std::iter_swap(lit, rit);
  std::vector<std::string_view> views;
  views.reserve(ordered.size());
  for (const auto& name : ordered) {
    views.push_back(name);
  }
  return frame_.order_caselbls(pluss::msharp::RangeView::Of<std::string_view>(
      std::span<const std::string_view>(views.data(), views.size())));
}

bool DataFrameLabelAccessor::monotonic(bool ascending, bool strict, bool nulls_last) const {
  const auto labels = frame_.labels().values();
  for (std::int64_t row = 1; row < labels.size(); ++row) {
    const auto cmp = detail::compare_rows(labels, row - 1, row, nulls_last);
    if (ascending) {
      if (strict ? cmp >= 0 : cmp > 0) {
        return false;
      }
    } else if (strict ? cmp <= 0 : cmp < 0) {
      return false;
    }
  }
  return true;
}

std::int64_t DataFrameLabelAccessor::depth() const {
  const auto& src = frame_.impl_ref_();
  if (!src.case_axis_.defined()) {
    return monotonic(true, false, true) ? 1 : 0;
  }
  std::int64_t depth = 0;
  for (std::size_t prefix = 1; prefix <= src.case_axis_.levels.size(); ++prefix) {
    bool sorted = true;
    for (std::int64_t row = 1; row < src.nrows_ && sorted; ++row) {
      for (std::size_t level = 0; level < prefix; ++level) {
        const auto cmp = detail::compare_rows(src.case_axis_.levels[level].values, row - 1, row, true);
        if (cmp < 0) {
          break;
        }
        if (cmp > 0) {
          sorted = false;
          break;
        }
      }
    }
    if (!sorted) {
      break;
    }
    depth = static_cast<std::int64_t>(prefix);
  }
  return depth;
}

DataFrame DataFrameLabelAccessor::codes() const {
  const auto& src = frame_.impl_ref_();
  std::vector<std::pair<std::string, Slate>> columns;
  if (!src.case_axis_.defined()) {
    const auto labels = frame_.labels().values();
    std::unordered_map<std::string, std::int64_t> code_by_value;
    std::vector<std::int64_t> codes;
    codes.reserve(static_cast<std::size_t>(labels.size()));
    for (std::int64_t row = 0; row < labels.size(); ++row) {
      const auto key = detail::valid_at(labels, row) ? slate_scalar_string_at(labels, row) : std::string{"NULL"};
      auto [it, inserted] = code_by_value.emplace(key, static_cast<std::int64_t>(code_by_value.size()));
      codes.push_back(it->second);
    }
    columns.emplace_back("label", make_i64_slate(codes));
  } else {
    columns.reserve(src.case_axis_.levels.size());
    for (const auto& level : src.case_axis_.levels) {
      std::unordered_map<std::string, std::int64_t> code_by_value;
      std::vector<std::int64_t> codes;
      codes.reserve(static_cast<std::size_t>(level.values.size()));
      for (std::int64_t row = 0; row < level.values.size(); ++row) {
        const auto key = detail::valid_at(level.values, row) ? slate_scalar_string_at(level.values, row) : std::string{"NULL"};
        auto [it, inserted] = code_by_value.emplace(key, static_cast<std::int64_t>(code_by_value.size()));
        codes.push_back(it->second);
      }
      columns.emplace_back(level.name, make_i64_slate(codes));
    }
  }
  return DataFrame(std::move(columns), frame_.labels().values());
}

DataFrame DataFrameLabelAccessor::levels() const {
  const auto& src = frame_.impl_ref_();
  std::vector<std::pair<std::string, Slate>> columns;
  if (!src.case_axis_.defined()) {
    columns.emplace_back("label", frame_.labels().values().distinct(false).cast().note());
    return DataFrame(std::move(columns));
  }
  columns.reserve(src.case_axis_.levels.size());
  std::size_t max_size = 0;
  std::vector<std::vector<std::string>> level_values;
  level_values.reserve(src.case_axis_.levels.size());
  for (const auto& level : src.case_axis_.levels) {
    auto distinct = level.values.distinct(false);
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(distinct.size()));
    for (std::int64_t row = 0; row < distinct.size(); ++row) {
      values.push_back(detail::valid_at(distinct, row) ? slate_scalar_string_at(distinct, row) : std::string{"NULL"});
    }
    max_size = std::max(max_size, values.size());
    level_values.push_back(std::move(values));
  }
  for (std::size_t i = 0; i < src.case_axis_.levels.size(); ++i) {
    auto values = std::move(level_values[i]);
    values.resize(max_size, "");
    columns.emplace_back(src.case_axis_.levels[i].name, make_note_slate(values));
  }
  return DataFrame(std::move(columns));
}

DataFrame DataFrame::pick_cases_(const CasePick& pick) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::case_study: current DataFrame has no case labels");

  std::vector<std::string> level_names = pick.names;
  std::vector<std::string> values = pick.values;
  if (!pick.label.empty()) {
    PLUSS_CHECK(level_names.empty(), "DataFrame::case_study: use either `label` or `names`, not both");
    level_names.push_back(pick.label);
    values.push_back(pick.value);
  }
  if (!pick.key.empty()) {
    PLUSS_CHECK(level_names.empty(), "DataFrame::case_study: use either `key` or case-label/value selectors, not both");
    PLUSS_CHECK(pick.key.size() == src.case_axis_.levels.size(),
                "DataFrame::case_study: full key length must match case-label count");
    for (const auto& level : src.case_axis_.levels) {
      level_names.push_back(level.name);
    }
    values = pick.key;
  }
  PLUSS_CHECK(!level_names.empty(), "DataFrame::case_study: expected a case-label selector or full key");
  PLUSS_CHECK(level_names.size() == values.size(), "DataFrame::case_study: names/values length mismatch");

  std::vector<std::size_t> level_indices;
  level_indices.reserve(level_names.size());
  for (const auto& name : level_names) {
    level_indices.push_back(level_index_(src, name, "DataFrame::case_study"));
  }

  std::vector<std::int64_t> rows;
  rows.reserve(static_cast<std::size_t>(ncases()));
  for (std::int64_t row = 0; row < ncases(); ++row) {
    bool match = true;
    for (std::size_t i = 0; i < level_indices.size(); ++i) {
      const auto& column = src.case_axis_.levels[level_indices[i]].values;
      const auto observed = detail::valid_at(column, row) ? slate_scalar_string_at(column, row) : std::string("NULL");
      if (observed != values[i]) {
        match = false;
        break;
      }
    }
    if (match) {
      rows.push_back(row);
    }
  }
  PLUSS_CHECK(!rows.empty(), "DataFrame::case_study: no cases matched case level selector");
  auto out = take(rows);
  if (pick.drop) {
    std::vector<std::string_view> drop_views;
    drop_views.reserve(level_names.size());
    for (const auto& name : level_names) {
      drop_views.push_back(name);
    }
    out = out.drop_caselbls(pluss::msharp::RangeView::Of<std::string_view>(
        std::span<const std::string_view>(drop_views.data(), drop_views.size())));
  }
  return out;
}

DataFrame DataFrame::select_cases_(const detail::CaseSelector& rows,
                                  const detail::FeatureSelector& selected_features) const {
  if (rows.kind == detail::CaseSelector::Kind::Mask) {
    auto filtered = filter(rows.mask);
    return project_feature_positions_(filtered, resolve_feature_positions_(filtered, selected_features));
  }
  const auto case_positions = resolve_case_positions_(*this, rows);
  auto out = take(case_positions);
  return project_feature_positions_(out, resolve_feature_positions_(out, selected_features));
}

DataFrameCaseStudyAccessor::DataFrameCaseStudyAccessor(const DataFrame& frame) : frame_(frame) {}

DataFrame DataFrameCaseStudyAccessor::at(std::int64_t row) const {
  return frame_.select_cases_(detail::CaseSelector::at(row), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::slice(std::int64_t start, std::int64_t stop) const {
  return frame_.select_cases_(detail::CaseSelector::slice(start, stop), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::take(std::initializer_list<std::int64_t> rows) const {
  return frame_.select_cases_(detail::CaseSelector::take(rows), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::take(pluss::RangeView<std::int64_t> rows) const {
  return frame_.select_cases_(detail::CaseSelector::take(rows), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(std::string_view value) const {
  return frame_.select_cases_(detail::CaseSelector::label(value), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(std::int64_t value) const {
  return frame_.select_cases_(detail::CaseSelector::label(value), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(double value) const {
  return frame_.select_cases_(detail::CaseSelector::label(value), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(std::initializer_list<std::string_view> values) const {
  return frame_.select_cases_(detail::CaseSelector::labels(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(pluss::RangeView<std::string_view> values) const {
  return frame_.select_cases_(detail::CaseSelector::labels(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(std::initializer_list<std::int64_t> values) const {
  return frame_.select_cases_(detail::CaseSelector::labels(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(pluss::RangeView<std::int64_t> values) const {
  return frame_.select_cases_(detail::CaseSelector::labels(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::labels(const Slate& values) const {
  return frame_.select_cases_(detail::CaseSelector::labels(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::span(std::string_view start, std::string_view stop) const {
  return frame_.select_cases_(detail::CaseSelector::span(start, stop), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::span(std::int64_t start, std::int64_t stop) const {
  return frame_.select_cases_(detail::CaseSelector::span(start, stop), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::span(double start, double stop) const {
  return frame_.select_cases_(detail::CaseSelector::span(start, stop), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::span(const Slate& start, const Slate& stop) const {
  return frame_.select_cases_(detail::CaseSelector::span(start, stop), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::where(const Slate& values) const {
  return frame_.select_cases_(detail::CaseSelector::where(values), detail::FeatureSelector::all());
}

DataFrame DataFrameCaseStudyAccessor::where(const pluss::Series& values) const {
  // Intentionally bypasses CaseSelector::where(Series), which wraps into a
  // Slate and loses strict positional validation to label alignment.
  // The Series overload's contract is positional and must match ncases()
  // exactly; DataFrame::filter(Series) is what actually enforces that.
  return frame_.filter(values);
}

DataFrame DataFrameCaseStudyAccessor::pick(const DataFrame::CasePick& pick) const {
  return frame_.pick_cases_(pick);
}

detail::CaseSelector detail::CaseSelector::at(std::int64_t row) {
  CaseSelector out;
  out.kind = Kind::PositionScalar;
  out.first = row;
  return out;
}

detail::CaseSelector detail::CaseSelector::slice(std::int64_t start, std::int64_t stop) {
  CaseSelector out;
  out.kind = Kind::PositionSlice;
  out.first = start;
  out.second = stop;
  return out;
}

detail::CaseSelector detail::CaseSelector::take(std::initializer_list<std::int64_t> rows) {
  CaseSelector out;
  out.kind = Kind::PositionList;
  out.positions.assign(rows.begin(), rows.end());
  return out;
}

detail::CaseSelector detail::CaseSelector::take(pluss::RangeView<std::int64_t> rows) {
  CaseSelector out;
  out.kind = Kind::PositionList;
  out.positions.assign(rows.begin(), rows.end());
  return out;
}

detail::CaseSelector detail::CaseSelector::label(std::string_view value) {
  CaseSelector out;
  out.kind = Kind::LabelScalar;
  out.label_values.emplace_back(value);
  return out;
}

detail::CaseSelector detail::CaseSelector::label(const char* value) {
  return label(std::string_view{value != nullptr ? value : ""});
}

detail::CaseSelector detail::CaseSelector::label(std::int64_t value) {
  return label(pluss::tils::OutputEngine::format("{}", value));
}

detail::CaseSelector detail::CaseSelector::label(double value) {
  return label(pluss::tils::OutputEngine::format("{:.12g}", value));
}

detail::CaseSelector detail::CaseSelector::labels(std::initializer_list<std::string_view> values) {
  CaseSelector out;
  out.kind = Kind::LabelList;
  out.label_values.reserve(values.size());
  for (const auto value : values) {
    out.label_values.emplace_back(value);
  }
  return out;
}

detail::CaseSelector detail::CaseSelector::labels(pluss::RangeView<std::string_view> values) {
  CaseSelector out;
  out.kind = Kind::LabelList;
  out.label_values.reserve(values.size());
  for (const auto value : values) {
    out.label_values.emplace_back(value);
  }
  return out;
}

detail::CaseSelector detail::CaseSelector::labels(std::initializer_list<std::int64_t> values) {
  CaseSelector out;
  out.kind = Kind::LabelList;
  out.label_values.reserve(values.size());
  for (const auto value : values) {
    out.label_values.push_back(pluss::tils::OutputEngine::format("{}", value));
  }
  return out;
}

detail::CaseSelector detail::CaseSelector::labels(pluss::RangeView<std::int64_t> values) {
  CaseSelector out;
  out.kind = Kind::LabelList;
  out.label_values.reserve(values.size());
  for (const auto value : values) {
    out.label_values.push_back(pluss::tils::OutputEngine::format("{}", value));
  }
  return out;
}

detail::CaseSelector detail::CaseSelector::labels(const Slate& values) {
  CaseSelector out;
  out.kind = Kind::LabelList;
  out.label_values = selector_labels_from_slate_(values, "labels(Slate)");
  return out;
}

detail::CaseSelector detail::CaseSelector::span(std::string_view start, std::string_view stop) {
  CaseSelector out;
  out.kind = Kind::LabelSlice;
  out.label_values.emplace_back(start);
  out.label_values.emplace_back(stop);
  return out;
}

detail::CaseSelector detail::CaseSelector::span(std::int64_t start, std::int64_t stop) {
  return span(pluss::tils::OutputEngine::format("{}", start), pluss::tils::OutputEngine::format("{}", stop));
}

detail::CaseSelector detail::CaseSelector::span(double start, double stop) {
  return span(pluss::tils::OutputEngine::format("{:.12g}", start), pluss::tils::OutputEngine::format("{:.12g}", stop));
}

detail::CaseSelector detail::CaseSelector::span(const Slate& start, const Slate& stop) {
  return span(selector_label_from_scalar_slate_(start, "Case::span(start)"),
              selector_label_from_scalar_slate_(stop, "Case::span(stop)"));
}

detail::CaseSelector detail::CaseSelector::where(const Slate& values) {
  CaseSelector out;
  out.kind = Kind::Mask;
  out.mask = values;
  return out;
}

detail::FeatureSelector detail::FeatureSelector::all() {
  return {};
}

detail::FeatureSelector detail::FeatureSelector::name(std::string_view value) {
  FeatureSelector out;
  out.kind = Kind::NameScalar;
  out.name_values.emplace_back(value);
  return out;
}

detail::FeatureSelector detail::FeatureSelector::name(const char* value) {
  return name(std::string_view{value != nullptr ? value : ""});
}

detail::FeatureSelector detail::FeatureSelector::at(std::int64_t position) {
  FeatureSelector out;
  out.kind = Kind::PositionScalar;
  out.first = position;
  return out;
}

detail::FeatureSelector detail::FeatureSelector::names(std::initializer_list<std::string_view> values) {
  FeatureSelector out;
  out.kind = Kind::NameList;
  out.name_values.reserve(values.size());
  for (const auto value : values) {
    out.name_values.emplace_back(value);
  }
  return out;
}

detail::FeatureSelector detail::FeatureSelector::names(pluss::RangeView<std::string_view> values) {
  FeatureSelector out;
  out.kind = Kind::NameList;
  out.name_values.reserve(values.size());
  for (const auto value : values) {
    out.name_values.emplace_back(value);
  }
  return out;
}

detail::FeatureSelector detail::FeatureSelector::take(std::initializer_list<std::int64_t> positions) {
  FeatureSelector out;
  out.kind = Kind::PositionList;
  out.positions.assign(positions.begin(), positions.end());
  return out;
}

detail::FeatureSelector detail::FeatureSelector::take(pluss::RangeView<std::int64_t> positions) {
  FeatureSelector out;
  out.kind = Kind::PositionList;
  out.positions.assign(positions.begin(), positions.end());
  return out;
}

detail::FeatureSelector detail::FeatureSelector::slice(std::int64_t start, std::int64_t stop) {
  FeatureSelector out;
  out.kind = Kind::PositionSlice;
  out.first = start;
  out.second = stop;
  return out;
}

detail::FeatureSelector detail::FeatureSelector::span(std::string_view start, std::string_view stop) {
  FeatureSelector out;
  out.kind = Kind::NameSlice;
  out.name_values.emplace_back(start);
  out.name_values.emplace_back(stop);
  return out;
}

std::optional<Slate> DataFrame::get(std::string_view feature_name) const {
  if (!has_feature(feature_name)) {
    return std::nullopt;
  }
  return features(feature_name);
}

DataFrame DataFrame::with_cases(const Slate& case_idx) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases: cases are undefined");
  PLUSS_CHECK(case_idx.size() == src.nrows_, "DataFrame::with_cases: case length mismatch");
  auto labels = detail::with_label_kind(case_idx, LabelKind::Case);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, labels));
}

DataFrame& DataFrame::with_cases_(const Slate& case_idx) {
  auto& w = mut_impl_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases_: cases are undefined");
  PLUSS_CHECK(case_idx.size() == w.nrows_, "DataFrame::with_cases_: case length mismatch");
  w.cases_ = detail::with_label_kind(case_idx, LabelKind::Case);
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

Slate DataFrame::features(std::string_view name) {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'", feature_name, pluss::exception::map_keys(src.name_to_idx_));
  return detail::with_display_labels(src.columns_[it->second], src.cases_)
      .name(std::move(feature_name));
}

Slate DataFrame::features(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'", feature_name, pluss::exception::map_keys(src.name_to_idx_));
  return detail::with_display_labels(src.columns_[it->second], src.cases_)
      .name(std::move(feature_name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), features(name));
}

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) {
  return project_names_(names);
}

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) const {
  return project_names_(names);
}

DataFrame DataFrame::features(std::initializer_list<const char*> names) {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return project_names_(pluss::msharp::RangeView::Of<std::string_view>(views));
}

DataFrame DataFrame::features(std::initializer_list<const char*> names) const {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return project_names_(pluss::msharp::RangeView::Of<std::string_view>(views));
}

DataFrame DataFrame::features(pluss::msharp::RangeView::Of<std::string_view> names) {
  return project_names_(names);
}

DataFrame DataFrame::features(pluss::msharp::RangeView::Of<std::string_view> names) const {
  return project_names_(names);
}

DataFrame DataFrame::features(StackArray<std::string_view> names) {
  return project_names_(pluss::msharp::RangeView::Of<std::string_view>(names.data().values));
}

DataFrame DataFrame::features(StackArray<std::string_view> names) const {
  return project_names_(pluss::msharp::RangeView::Of<std::string_view>(names.data().values));
}

DataFrame DataFrame::project_names_(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto picked = normalize_name_list(names);
  const auto& src = impl_ref_();

  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(picked.size());
  out_cols.reserve(picked.size());

  for (const auto& n : picked) {
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::features: unknown column '" + n + "'", n, pluss::exception::map_keys(src.name_to_idx_));
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

namespace {

bool same_flat_cases_for_select_(const Slate& lhs, const Slate& rhs) {
  if (lhs.size() != rhs.size() || lhs.dtype() != rhs.dtype()) {
    return false;
  }
  for (std::int64_t row = 0; row < lhs.size(); ++row) {
    if (detail::valid_at(lhs, row) != detail::valid_at(rhs, row)) {
      return false;
    }
    if (detail::valid_at(lhs, row) && slate_scalar_string_at(lhs, row) != slate_scalar_string_at(rhs, row)) {
      return false;
    }
  }
  return true;
}

bool same_case_axis_for_select_(const detail::CaseAxis& lhs, const detail::CaseAxis& rhs) {
  if (lhs.levels.size() != rhs.levels.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.levels.size(); ++i) {
    if (lhs.levels[i].name != rhs.levels[i].name) {
      return false;
    }
    if (!same_flat_cases_for_select_(lhs.levels[i].values, rhs.levels[i].values)) {
      return false;
    }
  }
  return true;
}

} // namespace

DataFrame DataFrame::select(pluss::msharp::RangeView::Of<Slate> conditions,
                            pluss::msharp::RangeView::Of<DataFrame> choices,
                            const DataFrame& default_value) {
  PLUSS_CHECK(!conditions.empty(), "DataFrame::select: at least one condition is required");
  PLUSS_CHECK(conditions.size() == choices.size(),
              "DataFrame::select: condition/choice count mismatch");
  PLUSS_CHECK(default_value.defined(), "DataFrame::select: default value is undefined");

  const auto& seed = default_value.impl_ref_();
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.defined(), "DataFrame::select: choice is undefined");
    PLUSS_CHECK(choice.ncases() == default_value.ncases(),
                "DataFrame::select: all choices must match default case count");
    PLUSS_CHECK(choice.nfeatures() == default_value.nfeatures(),
                "DataFrame::select: all choices must match default feature count");
    PLUSS_CHECK(choice.features() == default_value.features(),
                "DataFrame::select: all choices must match default feature order");
    PLUSS_CHECK(same_flat_cases_for_select_(choice.case_labels_(), default_value.case_labels_()),
                "DataFrame::select: all choices must match default case labels");
    PLUSS_CHECK(same_case_axis_for_select_(choice.impl_ref_().case_axis_, seed.case_axis_),
                "DataFrame::select: all choices must match default structured case metadata");
  }

  std::vector<Slate> out_cols;
  out_cols.reserve(seed.names_.size());
  std::vector<Slate> branch_cols;
  branch_cols.reserve(choices.size());
  for (const auto& name : seed.names_) {
    branch_cols.clear();
    for (const auto& choice : choices) {
      branch_cols.push_back(choice.features(name));
    }
    out_cols.push_back(Slate::select(
        conditions,
        pluss::msharp::RangeView::Of<Slate>(std::span<const Slate>(branch_cols.data(), branch_cols.size())),
        default_value.features(name)));
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      seed.names_, std::move(out_cols), seed.cases_, seed.title_, seed.case_axis_));
}

DataFrame DataFrame::select_like(std::string_view needle, bool case_sensitive) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::match_features(std::string_view pattern) const {
  const auto pat = owned_name(pattern);
  std::regex re;
  try {
    re = std::regex(pat, std::regex::ECMAScript);
  } catch (const std::regex_error& e) {
    PLUSS_FAIL("DataFrame::feataxis().match: invalid regex '" + pat + "': " + std::string(e.what()));
  }

  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::series_features() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != SlateKind::Series) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::note_features() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != SlateKind::Note) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::timeline_features() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto k = src.columns_[i].kind();
    if (k != SlateKind::Timeline) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrameFeatureAxisAccessor::DataFrameFeatureAxisAccessor(const DataFrame& frame) : frame_(frame) {}

DataFrame DataFrameFeatureAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  return frame_.select_like(needle, case_sensitive);
}

DataFrame DataFrameFeatureAxisAccessor::match(std::string_view pattern) const {
  return frame_.match_features(pattern);
}

DataFrame DataFrameFeatureAxisAccessor::series() const {
  return frame_.series_features();
}

DataFrame DataFrameFeatureAxisAccessor::note() const {
  return frame_.note_features();
}

DataFrame DataFrameFeatureAxisAccessor::timeline() const {
  return frame_.timeline_features();
}

DataFrame DataFrameFeatureAxisAccessor::duration() const {
  const auto& src = frame_.impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() == SlateKind::Duration) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrameFeatureAxisAccessor::prefix(std::string_view prefix) const {
  return frame_.add_prefix(prefix);
}

DataFrame DataFrameFeatureAxisAccessor::suffix(std::string_view suffix) const {
  return frame_.add_suffix(suffix);
}

DataFrameCastAccessor::DataFrameCastAccessor(const DataFrame& frame, SlateCastOptions options)
    : frame_(frame), options_(std::move(options)) {}

DataFrame DataFrameCastAccessor::cast_all_(SlateCastTarget target) const {
  const auto& names = frame_.features();
  if (names.empty()) {
    return frame_;
  }

  const auto& src = frame_.impl_ref_();
  std::vector<Slate> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(names.size());

  for (const auto& name : names) {
    const auto key = owned_name(name);
    PLUSS_CHECK(!key.empty(), "DataFrame::cast: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(key);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast: unknown feature '" + key + "'", key, pluss::exception::map_keys(src.name_to_idx_));
    if (!seen.insert(key).second) {
      continue;
    }
    switch (target) {
      case SlateCastTarget::Series:
        out_cols[it->second] = src.columns_[it->second].cast(options_).series();
        break;
      case SlateCastTarget::Note:
        out_cols[it->second] = src.columns_[it->second].cast(options_).note();
        break;
      case SlateCastTarget::Timeline:
        out_cols[it->second] = src.columns_[it->second].cast(options_).timeline();
        break;
      case SlateCastTarget::Duration:
        out_cols[it->second] = src.columns_[it->second].cast(options_).duration();
        break;
    }
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrameCastAccessor::series() const {
  return cast_all_(SlateCastTarget::Series);
}

DataFrame DataFrameCastAccessor::note() const {
  return cast_all_(SlateCastTarget::Note);
}

DataFrame DataFrameCastAccessor::timeline() const {
  return cast_all_(SlateCastTarget::Timeline);
}

DataFrame DataFrameCastAccessor::duration() const {
  return cast_all_(SlateCastTarget::Duration);
}

DataFrame DataFrame::astype(pluss::codem::ScalarType dtype) const {
  const auto& names = features();
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto& name : names) {
    views.push_back(name);
  }
  return astype(
      pluss::msharp::RangeView::Of<std::string_view>(
          std::span<const std::string_view>(views.data(), views.size())),
      dtype);
}

DataFrame DataFrame::astype(pluss::msharp::RangeView::Of<std::string_view> features,
                            pluss::codem::ScalarType dtype) const {
  std::vector<CastSpec> specs;
  specs.reserve(features.size());
  for (const auto name_sv : features) {
    specs.push_back(CastSpec{
        .feature = owned_name(name_sv),
        .target = SlateCastTarget::Series,
        .options = SlateCastOptions{.series_dtype = dtype},
    });
  }
  return cast(pluss::msharp::RangeView::Of<CastSpec>(
      std::span<const CastSpec>(specs.data(), specs.size())));
}

DataFrame DataFrame::insert(std::int64_t feature_index, std::string name, const Slate& feature_value) const {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::place(std::string name,
                           const Slate& feature_value,
                           std::string_view anchor_feature,
                           bool after) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!name.empty(), "DataFrame::place: feature name cannot be empty");
  PLUSS_CHECK(feature_value.defined(), "DataFrame::place: feature is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(feature_value.size() == src.nrows_, "DataFrame::place: case count mismatch");
  }

  const auto anchor = owned_name(anchor_feature);
  PLUSS_CHECK(!anchor.empty(), "DataFrame::place: anchor feature cannot be empty");
  const auto anchor_it = src.name_to_idx_.find(anchor);
  PLUSS_CHECK(anchor_it != src.name_to_idx_.end(),
              "DataFrame::place: unknown anchor feature '" + anchor + "'");

  const auto existing_it = src.name_to_idx_.find(name);
  if (existing_it == src.name_to_idx_.end()) {
    const auto pos = static_cast<std::int64_t>(anchor_it->second + (after ? 1 : 0));
    return insert(pos, std::move(name), feature_value);
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  auto existing = existing_it->second;
  out_cols[existing] = feature_value;
  if (existing == anchor_it->second) {
    return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
        std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
  }

  auto moved_name = out_names[existing];
  auto moved_col = out_cols[existing];
  out_names.erase(out_names.begin() + static_cast<std::ptrdiff_t>(existing));
  out_cols.erase(out_cols.begin() + static_cast<std::ptrdiff_t>(existing));

  auto anchor_pos = anchor_it->second;
  if (existing < anchor_pos) {
    --anchor_pos;
  }
  const auto insert_pos = anchor_pos + (after ? 1 : 0);
  out_names.insert(out_names.begin() + static_cast<std::ptrdiff_t>(insert_pos), std::move(moved_name));
  out_cols.insert(out_cols.begin() + static_cast<std::ptrdiff_t>(insert_pos), std::move(moved_col));
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::place(std::string_view name,
                           std::string_view anchor_feature,
                           bool after) const {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  PLUSS_CHECK(!feature_name.empty(), "DataFrame::place: feature name cannot be empty");
  const auto existing_it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(existing_it != src.name_to_idx_.end(),
              "DataFrame::place: unknown feature '" + feature_name + "'");
  return place(feature_name, src.columns_[existing_it->second], anchor_feature, after);
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
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (drop_set.find(src.names_[i]) == drop_set.end()) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::reorder(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto ordered = normalize_name_list(names);
  if (ordered.empty()) {
    return *this;
  }
  ReorderSpec spec;
  spec.features = ordered;
  spec.placement = ReorderPlacement::Front;
  return reorder(spec);
}

namespace {

std::vector<std::string> resolve_reorder_move_names_(
    const detail::DataFrameImpl& src,
    const std::vector<std::string>& current_order,
    const DataFrame::ReorderSpec& spec) {
  const auto pattern = owned_name(spec.pattern);
  PLUSS_CHECK(
      !spec.features.empty() || !pattern.empty(),
      "DataFrame::reorder: each ReorderSpec must provide feature names or a regex pattern");
  PLUSS_CHECK(spec.features.empty() || pattern.empty(),
              "DataFrame::reorder: provide either explicit feature names or a regex pattern, not both");

  std::vector<std::string> moved;
  if (!pattern.empty()) {
    std::regex re;
    try {
      re = std::regex(pattern, std::regex::ECMAScript);
    } catch (const std::regex_error& e) {
      PLUSS_FAIL("DataFrame::reorder: invalid regex '" + pattern + "': " + std::string(e.what()));
    }
    moved.reserve(current_order.size());
    for (const auto& name : current_order) {
      if (std::regex_search(name, re)) {
        moved.push_back(name);
      }
    }
    PLUSS_CHECK(!moved.empty(), "DataFrame::reorder: regex '" + pattern + "' did not match any features");
    return moved;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(spec.features.size());
  moved.reserve(spec.features.size());
  for (const auto& raw : spec.features) {
    const auto name = owned_name(raw);
    PLUSS_CHECK(!name.empty(), "DataFrame::reorder: feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, "DataFrame::reorder contains duplicate column name '" + name + "'");
    PLUSS_CHECK(src.name_to_idx_.find(name) != src.name_to_idx_.end(), "DataFrame::reorder unknown column '" + name + "'");
    moved.push_back(name);
  }
  return moved;
}

std::vector<std::string> apply_reorder_step_(
    const detail::DataFrameImpl& src,
    const std::vector<std::string>& current_order,
    const DataFrame::ReorderSpec& spec) {
  const auto moved = resolve_reorder_move_names_(src, current_order, spec);
  if (moved.empty()) {
    return current_order;
  }

  std::unordered_set<std::string> moved_set(moved.begin(), moved.end());
  std::vector<std::string> remainder;
  remainder.reserve(current_order.size() - std::min(current_order.size(), moved.size()));
  for (const auto& name : current_order) {
    if (!moved_set.contains(name)) {
      remainder.push_back(name);
    }
  }

  std::vector<std::string> ordered;
  ordered.reserve(current_order.size());
  switch (spec.placement) {
    case DataFrame::ReorderPlacement::Front:
      ordered.insert(ordered.end(), moved.begin(), moved.end());
      ordered.insert(ordered.end(), remainder.begin(), remainder.end());
      break;
    case DataFrame::ReorderPlacement::Back:
      ordered.insert(ordered.end(), remainder.begin(), remainder.end());
      ordered.insert(ordered.end(), moved.begin(), moved.end());
      break;
    case DataFrame::ReorderPlacement::Before:
    case DataFrame::ReorderPlacement::After: {
      const auto anchor = owned_name(spec.anchor);
      PLUSS_CHECK(!anchor.empty(), "DataFrame::reorder: anchor feature cannot be empty for before/after placement");
      PLUSS_CHECK(src.name_to_idx_.find(anchor) != src.name_to_idx_.end(),
                  "DataFrame::reorder: unknown anchor feature '" + anchor + "'");
      PLUSS_CHECK(!moved_set.contains(anchor),
                  "DataFrame::reorder: anchor feature '" + anchor + "' cannot also be one of the moved features");
      bool inserted = false;
      for (const auto& name : remainder) {
        if (spec.placement == DataFrame::ReorderPlacement::Before && name == anchor) {
          ordered.insert(ordered.end(), moved.begin(), moved.end());
          inserted = true;
        }
        ordered.push_back(name);
        if (spec.placement == DataFrame::ReorderPlacement::After && name == anchor) {
          ordered.insert(ordered.end(), moved.begin(), moved.end());
          inserted = true;
        }
      }
      PLUSS_CHECK(inserted, "DataFrame::reorder: failed to place moved features relative to anchor");
      break;
    }
  }
  return ordered;
}

} // namespace

DataFrame DataFrame::reorder(const ReorderSpec& spec) const {
  return reorder(pluss::RangeView<ReorderSpec>(std::span<const ReorderSpec>(&spec, 1)));
}

DataFrame DataFrame::reorder(pluss::RangeView<ReorderSpec> specs) const {
  if (specs.empty()) {
    return *this;
  }
  const auto& src = impl_ref_();
  auto ordered = src.names_;
  for (const auto& spec : specs) {
    ordered = apply_reorder_step_(src, ordered, spec);
  }

  std::vector<std::string_view> views;
  views.reserve(ordered.size());
  for (const auto& name : ordered) {
    views.push_back(name);
  }
  return features(pluss::msharp::RangeView::Of<std::string_view>(
      std::span<const std::string_view>(views.data(), views.size())));
}

DataFrame DataFrame::bucketize(pluss::msharp::RangeView::Double bins,
                               pluss::msharp::RangeView::Int labels,
                               bool right,
                               bool include_lowest) const {
  const auto& src = impl_ref_();
  std::vector<std::string_view> names;
  names.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == SlateKind::Series && is_numeric_series_type(col.as_series())) {
      names.push_back(src.names_[i]);
    }
  }
  PLUSS_CHECK(!names.empty(), "DataFrame::bucketize: no numeric Series features available");
  return bucketize(
      pluss::msharp::RangeView::Of<std::string_view>(std::span<const std::string_view>(names.data(), names.size())),
      bins,
      labels,
      right,
      include_lowest);
}

DataFrame DataFrame::bucketize(std::string_view feature_name,
                               pluss::msharp::RangeView::Double bins,
                               pluss::msharp::RangeView::Int labels,
                               bool right,
                               bool include_lowest) const {
  const auto one = std::array<std::string_view, 1>{feature_name};
  return bucketize(
      pluss::msharp::RangeView::Of<std::string_view>(std::span<const std::string_view>(one.data(), one.size())),
      bins,
      labels,
      right,
      include_lowest);
}

DataFrame DataFrame::bucketize(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                               pluss::msharp::RangeView::Double bins,
                               pluss::msharp::RangeView::Int labels,
                               bool right,
                               bool include_lowest) const {
  const auto selected = normalize_name_list(feature_names);
  const auto& src = impl_ref_();
  if (selected.empty()) {
    return *this;
  }

  auto out_cols = src.columns_;
  for (const auto& name : selected) {
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::bucketize: unknown feature '" + name + "'", name, pluss::exception::map_keys(src.name_to_idx_));
    out_cols[it->second] = src.columns_[it->second].bucketize(bins, labels, right, include_lowest);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::add_prefix(std::string_view prefix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(pluss::tils::OutputEngine::format("{}{}", prefix, name));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::add_suffix(std::string_view suffix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(pluss::tils::OutputEngine::format("{}{}", name, suffix));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
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

namespace {

std::vector<std::string> normalize_sort_group_names_(
    const DataFrame& frame,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  std::vector<std::string> names;
  names.reserve(features.size());
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto feature_sv : features) {
    const auto feature = owned_name(feature_sv);
    PLUSS_CHECK(!feature.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(frame.get(feature).has_value(), std::string(where) + ": unknown feature '" + feature + "'");
    if (seen.insert(feature).second) {
      names.push_back(feature);
    }
  }
  return names;
}

std::vector<DataFrame::ArrangeSpec> append_sort_specs_(
    const std::vector<DataFrame::ArrangeSpec>& base,
    const std::vector<DataFrame::ArrangeSpec>& incoming,
    std::string_view where) {
  std::vector<DataFrame::ArrangeSpec> out = base;
  out.reserve(base.size() + incoming.size());
  std::unordered_set<std::string> seen;
  seen.reserve(base.size() + incoming.size());
  for (const auto& spec : base) {
    if (!spec.feature.empty()) {
      seen.insert(spec.feature);
    }
  }
  for (const auto& spec : incoming) {
    PLUSS_CHECK(!spec.feature.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(seen.insert(spec.feature).second,
                std::string(where) + ": duplicate sort key '" + spec.feature + "'");
    out.push_back(spec);
  }
  return out;
}

std::vector<std::int64_t> sort_builder_order_(
    const DataFrame& frame,
    const std::vector<DataFrame::ArrangeSpec>& specs,
    const std::vector<std::string>& within,
    std::string_view where) {
  PLUSS_CHECK(!specs.empty(), std::string(where) + ": no sort keys configured; call .by(...) first");
  const auto normalized_specs = normalize_arrange_specs_(frame, specs, where);
  if (within.empty()) {
    return arrange_order_impl_frame_(frame, normalized_specs, where);
  }

  std::vector<Slate> group_keys;
  group_keys.reserve(within.size());
  for (const auto& name : within) {
    group_keys.push_back(frame.features(name));
  }
  const auto state = build_group_state_(
      std::span<const Slate>(group_keys.data(), group_keys.size()),
      std::span<const std::string>(within.data(), within.size()),
      frame.ncases(),
      SlateGroupOptions{.dropnull = false, .sort = false},
      where);

  std::vector<std::int64_t> ordered;
  ordered.reserve(static_cast<std::size_t>(frame.ncases()));
  for (std::size_t g = 0; g + 1 < state.group_offsets.size(); ++g) {
    const auto start = static_cast<std::size_t>(state.group_offsets[g]);
    const auto stop = static_cast<std::size_t>(state.group_offsets[g + 1]);
    if (stop <= start) {
      continue;
    }
    const auto local = arrange_order_on_indices_(
        frame,
        normalized_specs,
        std::span<const std::int64_t>(state.grouped_rows.data() + start, stop - start));
    ordered.insert(ordered.end(), local.begin(), local.end());
  }
  PLUSS_CHECK(
      ordered.size() == static_cast<std::size_t>(frame.ncases()),
      std::string(where) + ": internal sort order size mismatch");
  return ordered;
}

std::vector<std::int64_t> sort_builder_ranked_(
    const DataFrame& frame,
    const std::vector<DataFrame::ArrangeSpec>& specs,
    const std::vector<std::string>& within,
    std::int64_t keep,
    bool tail,
    std::string_view where) {
  PLUSS_CHECK(!specs.empty(), std::string(where) + ": no sort keys configured; call .by(...) first");
  PLUSS_CHECK(keep >= 0, std::string(where) + ": keep must be >= 0");
  const auto clipped = std::clamp<std::int64_t>(keep, 0, frame.ncases());
  if (clipped == 0) {
    return {};
  }

  const auto normalized_specs = normalize_arrange_specs_(frame, specs, where);
  const auto arrange_state = prepare_arrange_state_(frame, normalized_specs, where);
  if (within.empty()) {
    std::vector<std::int64_t> identity(static_cast<std::size_t>(frame.ncases()));
    std::iota(identity.begin(), identity.end(), static_cast<std::int64_t>(0));
    return arrange_partial_order_on_indices_(
        arrange_state,
        std::span<const std::int64_t>(identity.data(), identity.size()),
        clipped,
        tail);
  }

  std::vector<Slate> group_keys;
  group_keys.reserve(within.size());
  for (const auto& name : within) {
    group_keys.push_back(frame.features(name));
  }
  const auto group_state = build_group_state_(
      std::span<const Slate>(group_keys.data(), group_keys.size()),
      std::span<const std::string>(within.data(), within.size()),
      frame.ncases(),
      SlateGroupOptions{.dropnull = false, .sort = false},
      where);

  if (!tail) {
    std::vector<std::int64_t> ordered;
    ordered.reserve(static_cast<std::size_t>(clipped));
    auto remaining = clipped;
    for (std::size_t g = 0; g + 1 < group_state.group_offsets.size() && remaining > 0; ++g) {
      const auto start = static_cast<std::size_t>(group_state.group_offsets[g]);
      const auto stop = static_cast<std::size_t>(group_state.group_offsets[g + 1]);
      if (stop <= start) {
        continue;
      }
      const auto group_size = static_cast<std::int64_t>(stop - start);
      const auto take_n = std::min<std::int64_t>(remaining, group_size);
      const auto span = std::span<const std::int64_t>(group_state.grouped_rows.data() + start, stop - start);
      auto local = (take_n == group_size)
          ? arrange_order_on_indices_(frame, normalized_specs, span)
          : arrange_partial_order_on_indices_(arrange_state, span, take_n, false);
      ordered.insert(ordered.end(), local.begin(), local.end());
      remaining -= take_n;
    }
    return ordered;
  }

  std::vector<std::vector<std::int64_t>> chunks;
  auto remaining = clipped;
  for (std::size_t g = group_state.group_offsets.size(); g > 1 && remaining > 0; --g) {
    const auto idx = g - 2;
    const auto start = static_cast<std::size_t>(group_state.group_offsets[idx]);
    const auto stop = static_cast<std::size_t>(group_state.group_offsets[idx + 1]);
    if (stop <= start) {
      continue;
    }
    const auto group_size = static_cast<std::int64_t>(stop - start);
    const auto take_n = std::min<std::int64_t>(remaining, group_size);
    const auto span = std::span<const std::int64_t>(group_state.grouped_rows.data() + start, stop - start);
    auto local = (take_n == group_size)
        ? arrange_order_on_indices_(frame, normalized_specs, span)
        : arrange_partial_order_on_indices_(arrange_state, span, take_n, true);
    chunks.push_back(std::move(local));
    remaining -= take_n;
  }

  std::vector<std::int64_t> ordered;
  ordered.reserve(static_cast<std::size_t>(clipped));
  for (auto it = chunks.rbegin(); it != chunks.rend(); ++it) {
    ordered.insert(ordered.end(), it->begin(), it->end());
  }
  return ordered;
}

} // namespace

DataFrameSortBuilder DataFrame::sort(bool ascending, bool nulls_last) const {
  return DataFrameSortBuilder(*this, ascending, nulls_last);
}

DataFrameSortBuilder::DataFrameSortBuilder(DataFrame frame, bool ascending, bool nulls_last)
    : frame_(std::move(frame)), ascending_(ascending), nulls_last_(nulls_last) {}

DataFrameSortBuilder DataFrameSortBuilder::with_specs_(std::vector<DataFrame::ArrangeSpec> specs,
                                                       std::optional<std::size_t> active_spec) const {
  auto out = *this;
  out.specs_ = std::move(specs);
  out.active_spec_ = active_spec;
  out.order_cache_.reset();
  out.head_cache_n_.reset();
  out.head_cache_.clear();
  out.tail_cache_n_.reset();
  out.tail_cache_.clear();
  return out;
}

DataFrameSortBuilder DataFrameSortBuilder::mutate_active_(bool ascending, bool nulls_last) const {
  PLUSS_CHECK(active_spec_.has_value(), "DataFrame::sort: no active sort key; call .by(...) first");
  auto out = *this;
  auto& spec = out.specs_.at(*active_spec_);
  spec.ascending = ascending;
  spec.nulls_last = nulls_last;
  out.order_cache_.reset();
  out.head_cache_n_.reset();
  out.head_cache_.clear();
  out.tail_cache_n_.reset();
  out.tail_cache_.clear();
  return out;
}

std::vector<std::int64_t> DataFrameSortBuilder::ordered_indices_(std::string_view where) const {
  if (!order_cache_) {
    order_cache_ = std::make_shared<const std::vector<std::int64_t>>(
        sort_builder_order_(frame_, specs_, within_, where));
  }
  return *order_cache_;
}

std::vector<std::int64_t> DataFrameSortBuilder::ranked_indices_(std::int64_t n,
                                                                bool tail,
                                                                std::string_view where) const {
  PLUSS_CHECK(n >= 0, std::string(where) + ": n must be >= 0");
  const auto clipped = std::clamp<std::int64_t>(n, 0, frame_.ncases());
  if (clipped == 0) {
    return {};
  }

  if (order_cache_) {
    const auto& indices = *order_cache_;
    if (!tail) {
      return std::vector<std::int64_t>(indices.begin(), indices.begin() + clipped);
    }
    return std::vector<std::int64_t>(indices.end() - clipped, indices.end());
  }

  if (!tail && head_cache_n_.has_value() && *head_cache_n_ == clipped) {
    return head_cache_;
  }
  if (tail && tail_cache_n_.has_value() && *tail_cache_n_ == clipped) {
    return tail_cache_;
  }

  auto ranked = sort_builder_ranked_(frame_, specs_, within_, clipped, tail, where);
  if (!tail) {
    head_cache_n_ = clipped;
    head_cache_ = ranked;
  } else {
    tail_cache_n_ = clipped;
    tail_cache_ = ranked;
  }
  return ranked;
}

DataFrameSortBuilder DataFrameSortBuilder::by(std::string_view feature) const {
  return by(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}

DataFrameSortBuilder DataFrameSortBuilder::by(std::initializer_list<std::string_view> features) const {
  return by(pluss::msharp::RangeView::Of<std::string_view>(features));
}

DataFrameSortBuilder DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<std::string_view> features) const {
  std::vector<DataFrame::ArrangeSpec> specs;
  specs.reserve(features.size());
  for (const auto feature_sv : features) {
    specs.push_back(DataFrame::ArrangeSpec{owned_name(feature_sv), ascending_, nulls_last_});
  }
  return by(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec>(specs));
}

DataFrameSortBuilder DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const {
  std::vector<DataFrame::ArrangeSpec> owned(specs.begin(), specs.end());
  const auto active = owned.empty() ? std::nullopt : std::optional<std::size_t>(owned.size() - 1);
  return with_specs_(std::move(owned), active);
}

DataFrameSortBuilder DataFrameSortBuilder::then(std::string_view feature) const {
  return then(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}

DataFrameSortBuilder DataFrameSortBuilder::then(std::initializer_list<std::string_view> features) const {
  return then(pluss::msharp::RangeView::Of<std::string_view>(features));
}

DataFrameSortBuilder DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<std::string_view> features) const {
  std::vector<DataFrame::ArrangeSpec> specs;
  specs.reserve(features.size());
  for (const auto feature_sv : features) {
    specs.push_back(DataFrame::ArrangeSpec{owned_name(feature_sv), ascending_, nulls_last_});
  }
  return then(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec>(specs));
}

DataFrameSortBuilder DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const {
  PLUSS_CHECK(!specs_.empty(), "DataFrame::sort.then: no existing sort plan; call .by(...) first");
  const std::vector<DataFrame::ArrangeSpec> incoming(specs.begin(), specs.end());
  auto merged = append_sort_specs_(specs_, incoming, "DataFrame::sort.then");
  const auto active = merged.empty() ? std::nullopt : std::optional<std::size_t>(merged.size() - 1);
  return with_specs_(std::move(merged), active);
}

DataFrameSortBuilder DataFrameSortBuilder::within(std::string_view feature) const {
  return within(pluss::msharp::RangeView::Of<std::string_view>({feature}));
}

DataFrameSortBuilder DataFrameSortBuilder::within(std::initializer_list<std::string_view> features) const {
  return within(pluss::msharp::RangeView::Of<std::string_view>(features));
}

DataFrameSortBuilder DataFrameSortBuilder::within(pluss::msharp::RangeView::Of<std::string_view> features) const {
  auto out = *this;
  out.within_ = normalize_sort_group_names_(frame_, features, "DataFrame::sort.within");
  out.order_cache_.reset();
  out.head_cache_n_.reset();
  out.head_cache_.clear();
  out.tail_cache_n_.reset();
  out.tail_cache_.clear();
  return out;
}

DataFrameSortBuilder DataFrameSortBuilder::asc() const {
  return mutate_active_(true, active_spec_.has_value() ? specs_.at(*active_spec_).nulls_last : nulls_last_);
}

DataFrameSortBuilder DataFrameSortBuilder::desc() const {
  return mutate_active_(false, active_spec_.has_value() ? specs_.at(*active_spec_).nulls_last : nulls_last_);
}

DataFrameSortBuilder DataFrameSortBuilder::nulls_first() const {
  return mutate_active_(active_spec_.has_value() ? specs_.at(*active_spec_).ascending : ascending_, false);
}

DataFrameSortBuilder DataFrameSortBuilder::nulls_last() const {
  return mutate_active_(active_spec_.has_value() ? specs_.at(*active_spec_).ascending : ascending_, true);
}

Slate DataFrameSortBuilder::order() const {
  const auto indices = ordered_indices_("DataFrame::sort.order");
  auto out = make_i64_slate(indices);
  if (frame_.labels().has()) {
    out = out.label().set(frame_.labels().values().take(
        std::span<const std::int64_t>(indices.data(), indices.size())));
  }
  return out;
}

DataFrame DataFrameSortBuilder::yield() const {
  return frame_.take(order());
}

DataFrame DataFrameSortBuilder::head(std::int64_t n) const {
  const auto indices = ranked_indices_(n, false, "DataFrame::sort.head");
  return frame_.take(std::span<const std::int64_t>(indices.data(), indices.size()));
}

DataFrame DataFrameSortBuilder::tail(std::int64_t n) const {
  const auto indices = ranked_indices_(n, true, "DataFrame::sort.tail");
  return frame_.take(std::span<const std::int64_t>(indices.data(), indices.size()));
}

DataFrame DataFrame::sort_caselbl(bool ascending, bool nulls_last) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(ncases()));
  std::iota(idx.begin(), idx.end(), 0);
  const auto& case_axis = case_labels_();
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    const auto cmp = detail::compare_rows(case_axis, a, b, nulls_last);
    return ascending ? (cmp < 0) : (cmp > 0);
  });
  return take(idx);
}

DataFrame DataFrame::sort_caselbl(const SortLabelOptions& options) const {
  const auto& src = impl_ref_();
  if (options.names.empty()) {
    return sort_caselbl(options.ascending, options.nulls_last);
  }
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::sort_caselbl: current DataFrame has no case labels");
  std::vector<std::size_t> level_indices;
  level_indices.reserve(options.names.size());
  for (const auto& name : options.names) {
    level_indices.push_back(level_index_(src, name, "DataFrame::sort_caselbl"));
  }
  std::vector<std::int64_t> idx(static_cast<std::size_t>(ncases()));
  std::iota(idx.begin(), idx.end(), 0);
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    for (const auto level_idx : level_indices) {
      const auto& level_values = src.case_axis_.levels[level_idx].values;
      const auto cmp = detail::compare_rows(level_values, a, b, options.nulls_last);
      if (cmp != 0) {
        return options.ascending ? (cmp < 0) : (cmp > 0);
      }
    }
    return false;
  });
  return take(idx);
}

DataFrame DataFrame::top(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::top: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return arrange({ArrangeSpec{owned_name(feature_name), false, true}})
      .head(n, false);
}

DataFrame DataFrame::bottom(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::bottom: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return arrange({ArrangeSpec{owned_name(feature_name), true, true}})
      .head(n, false);
}

pluss::Series DataFrame::repeated_caselbl(pluss::msharp::RangeView::Of<std::string_view> features) const {
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
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::repeated_caselbl: unknown feature '" + owned_name(f) + "'");
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
      if (!detail::valid_at(col, r)) {
        key.append("\x1D", 1);
      } else {
        key += slate_scalar_string_at(col, r);
      }
      key.append("\x1E", 1);
    }
    const auto inserted = seen_keys.insert(std::move(key)).second;
    mask[r] = !inserted;
  }
  return out;
}

DataFrame DataFrame::unique_cases(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return filter(repeated_caselbl(features).logical_not());
}

DataFrame DataFrame::set_caselbl(std::string_view feature_name, bool drop_feature) const {
  SetLabelOptions options;
  options.features.emplace_back(feature_name);
  options.drop = drop_feature;
  return set_caselbl(options);
}

DataFrame DataFrame::set_caselbl(std::initializer_list<std::string_view> feature_names,
                               bool drop_features,
                               std::string_view separator) const {
  return set_caselbl(pluss::msharp::RangeView::Of<std::string_view>(feature_names),
                   drop_features,
                   separator);
}

DataFrame DataFrame::set_caselbl(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                               bool drop_features,
                               std::string_view separator) const {
  SetLabelOptions options;
  options.drop = drop_features;
  options.separator = std::string(separator);
  options.features.reserve(feature_names.size());
  for (const auto name : feature_names) {
    options.features.emplace_back(name);
  }
  return set_caselbl(options);
}

DataFrame DataFrame::set_caselbl(const SetLabelOptions& options) const {
  std::vector<std::string_view> feature_names;
  feature_names.reserve(options.features.size());
  for (const auto& name : options.features) {
    feature_names.push_back(name);
  }
  PLUSS_CHECK(feature_names.size() > 0, "DataFrame::set_caselbl: expected at least one feature");

  std::vector<std::string> names;
  names.reserve(feature_names.size());
  std::unordered_set<std::string> seen;
  for (const auto name_sv : feature_names) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::set_caselbl: feature name cannot be empty");
    PLUSS_CHECK(has_feature(name), "DataFrame::set_caselbl: unknown feature '" + name + "'");
    PLUSS_CHECK(seen.insert(name).second, "DataFrame::set_caselbl: duplicate feature '" + name + "'");
    names.push_back(name);
  }

  detail::CaseAxis axis;
  axis.levels.reserve(names.size());
  for (const auto& name : names) {
    axis.levels.push_back({name, features(name)});
  }
  axis.separator = std::string(options.separator);

  if (options.verify) {
    std::unordered_set<std::string> keys;
    keys.reserve(static_cast<std::size_t>(ncases()));
    for (std::int64_t row = 0; row < ncases(); ++row) {
      const auto inserted = keys.insert(case_tuple_key_(axis, row)).second;
      PLUSS_CHECK(inserted, "DataFrame::set_caselbl: duplicate structured case-label key found; set verify=false to allow duplicates");
    }
  }

  Slate case_values;
  if (names.size() == 1) {
    case_values = detail::clear_display_labels(axis.levels.front().values).unname();
  } else {
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(ncases()));
    for (std::int64_t r = 0; r < ncases(); ++r) {
      std::string label;
      for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
          label.append(options.separator.data(), options.separator.size());
        }
        label += slate_scalar_string_at(features(names[i]), r);
      }
      labels.push_back(std::move(label));
    }
    case_values = make_note_slate(labels);
  }

  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      impl_ref_().names_, impl_ref_().columns_, std::move(case_values), title(), std::move(axis)));
  if (options.drop) {
    std::vector<std::string_view> views;
    views.reserve(names.size());
    for (const auto& name : names) {
      views.push_back(name);
    }
    out = out.drop(pluss::msharp::RangeView::Of<std::string_view>(
        std::span<const std::string_view>(views.data(), views.size())));
  }
  return options.sort ? out.sort_caselbl(SortLabelOptions{.names = names}) : out;
}

DataFrame DataFrame::reset_caselbl(std::string case_feature) const {
  ResetLabelOptions options;
  if (!case_feature.empty()) {
    options.names.push_back(std::move(case_feature));
  }
  return reset_caselbl(options);
}

DataFrame DataFrame::reset_caselbl(const ResetLabelOptions& options) const {
  const auto& src = impl_ref_();
  auto out = *this;
  if (!src.case_axis_.defined()) {
    if (!options.drop) {
      const auto name = !options.names.empty() ? options.names.front() : options.prefix;
      out = out.set_features(name.empty() ? "case" : name, case_labels_());
    }
    return out.with_cases(default_cases(out.ncases()));
  }

  if (!options.drop) {
    PLUSS_CHECK(options.names.empty() || options.names.size() == src.case_axis_.levels.size(),
                "DataFrame::reset_caselbl: replacement names must match case-label count");
    for (std::size_t i = 0; i < src.case_axis_.levels.size(); ++i) {
      std::string name;
      if (!options.names.empty()) {
        name = options.names[i];
      } else {
        name = src.case_axis_.levels[i].name.empty()
                   ? pluss::tils::OutputEngine::format("{}{}", options.prefix, i)
                   : src.case_axis_.levels[i].name;
      }
      PLUSS_CHECK(!out.get(name).has_value(), "DataFrame::reset_caselbl: feature already exists '" + name + "'");
      auto level_values = src.case_axis_.levels[i].values;
      if (detail::has_display_labels(level_values)) {
        level_values = detail::clear_display_labels(level_values);
      }
      if (level_values.named()) {
        level_values = level_values.unname();
      }
      set_prepared_feature_(out.mut_impl_(), name, clear_assignment_metadata_(level_values));
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      out.impl_ref_().names_, out.impl_ref_().columns_, default_cases(out.ncases()), out.title()));
}

DataFrame DataFrame::rename_caselbls(pluss::msharp::RangeView::Of<LabelRename> renames) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::rename_caselbls: current DataFrame has no case levels");
  detail::CaseAxis axis = src.case_axis_;
  std::unordered_set<std::string> targets;
  for (const auto& rename : renames) {
    PLUSS_CHECK(!rename.from.empty(), "DataFrame::rename_caselbls: source case label cannot be empty");
    PLUSS_CHECK(!rename.to.empty(), "DataFrame::rename_caselbls: target case label cannot be empty");
    const auto idx = level_index_(src, rename.from, "DataFrame::rename_caselbls");
    PLUSS_CHECK(targets.insert(rename.to).second,
                "DataFrame::rename_caselbls: duplicate target case label '" + rename.to + "'");
    axis.levels[idx].name = rename.to;
  }
  std::unordered_set<std::string> names;
  for (const auto& level : axis.levels) {
    PLUSS_CHECK(names.insert(level.name).second,
                "DataFrame::rename_caselbls: duplicate case-label name '" + level.name + "'");
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, src.cases_, src.title_, std::move(axis)));
}

DataFrame DataFrame::order_caselbls(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::order_caselbls: current DataFrame has no case levels");
  PLUSS_CHECK(names.size() == src.case_axis_.levels.size(),
              "DataFrame::order_caselbls: expected every case level exactly once");
  detail::CaseAxis axis;
  axis.levels.reserve(names.size());
  axis.separator = src.case_axis_.separator;
  std::unordered_set<std::string> seen;
  for (const auto name_sv : names) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(seen.insert(name).second,
                "DataFrame::order_caselbls: duplicate case label '" + name + "'");
    axis.levels.push_back(src.case_axis_.levels[level_index_(src, name, "DataFrame::order_caselbls")]);
  }
  auto labels = make_note_slate(composite_case_labels_(axis, src.nrows_, axis.separator));
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, std::move(labels), src.title_, std::move(axis)));
}

DataFrame DataFrame::drop_caselbls(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::drop_caselbls: current DataFrame has no case levels");
  std::unordered_set<std::string> drop;
  drop.reserve(names.size());
  for (const auto name_sv : names) {
    const auto name = owned_name(name_sv);
    level_index_(src, name, "DataFrame::drop_caselbls");
    drop.insert(name);
  }
  detail::CaseAxis axis;
  axis.separator = src.case_axis_.separator;
  for (const auto& level : src.case_axis_.levels) {
    if (!drop.contains(level.name)) {
      axis.levels.push_back(level);
    }
  }
  Slate out_cases = axis.defined() ? make_note_slate(composite_case_labels_(axis, src.nrows_, axis.separator))
                                    : default_cases(src.nrows_);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, std::move(out_cases), src.title_, std::move(axis)));
}

namespace {

std::vector<std::string> align_target_features_(const DataFrame& frame,
                                                const DataFrame::AlignSpec& spec,
                                                std::string_view where) {
  if (spec.features.empty()) {
    return frame.features();
  }
  std::vector<std::string> out;
  out.reserve(spec.features.size());
  std::unordered_set<std::string> seen;
  seen.reserve(spec.features.size());
  for (const auto& name : spec.features) {
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature label cannot be empty");
    PLUSS_CHECK(seen.insert(name).second,
                std::string(where) + ": duplicate target feature '" + name + "'");
    out.push_back(name);
  }
  return out;
}

Slate null_align_feature_(std::int64_t size, std::optional<double> fill, std::string_view where) {
  PLUSS_CHECK(size >= 0, std::string(where) + ": output size must be non-negative");
  const double value = fill.value_or(std::numeric_limits<double>::quiet_NaN());
  auto values = pluss::Series::full({size},
                                    value,
                                    pluss::codem::ScalarType::Float64,
                                    pluss::codem::Platform::CPU);
  if (fill.has_value()) {
    return Slate(values);
  }
  return Slate(values, pluss::Series::zeros({size}, pluss::codem::ScalarType::Bool));
}

Slate remap_align_feature_(const Slate& source,
                            const Slate& target_caselbl,
                            std::optional<double> fill,
                            std::string_view where) {
  PLUSS_CHECK(source.defined(), std::string(where) + ": source feature is undefined");
  PLUSS_CHECK(target_caselbl.defined(), std::string(where) + ": target case labels are undefined");

  const auto out_size = target_caselbl.size();
  if (out_size == 0) {
    return null_slate_like_(source, 0, where);
  }
  if (source.size() == 0) {
    auto out = null_slate_like_(source, out_size, where);
    if (fill.has_value() && out.kind() == SlateKind::Series) {
      out = out.fillnull(*fill);
    }
    return out;
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> positions;
  positions.reserve(static_cast<std::size_t>(source.label().values().size()));
  const auto source_labels = source.label().values();
  for (std::int64_t row = 0; row < source_labels.size(); ++row) {
    positions[slate_scalar_string_at(source_labels, row)].push_back(row);
  }

  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(static_cast<std::size_t>(target_caselbl.size()));
  std::vector<std::int64_t> take_rows;
  std::vector<bool> present;
  take_rows.reserve(static_cast<std::size_t>(target_caselbl.size()));
  present.reserve(static_cast<std::size_t>(target_caselbl.size()));
  for (std::int64_t row = 0; row < target_caselbl.size(); ++row) {
    const auto key = slate_scalar_string_at(target_caselbl, row);
    const auto occurrence = seen[key]++;
    const auto it = positions.find(key);
    if (it != positions.end() && occurrence < it->second.size()) {
      take_rows.push_back(it->second[occurrence]);
      present.push_back(true);
    } else {
      take_rows.push_back(0);
      present.push_back(false);
    }
  }

  auto out = source.take(std::span<const std::int64_t>(take_rows.data(), take_rows.size()));
  if (fill.has_value() && out.kind() == SlateKind::Series) {
    auto missing = bool_series_from_keep(present).logical_not();
    if (missing.platform() != out.as_series().platform()) {
      missing = missing.to(out.as_series().platform());
    }
    out = Slate(out.as_series().where(missing, *fill), detail::validity_mask(out));
  }

  const bool all_present = std::all_of(present.begin(), present.end(), [](bool ok) { return ok; });
  if (!all_present || detail::has_validity_mask(out)) {
    std::vector<bool> valid;
    valid.reserve(present.size());
    for (std::int64_t row = 0; row < static_cast<std::int64_t>(present.size()); ++row) {
      const auto idx = static_cast<std::size_t>(row);
      if (!present[idx]) {
        valid.push_back(fill.has_value() && out.kind() == SlateKind::Series);
      } else {
        valid.push_back(detail::valid_at(out, row));
      }
    }
    auto validity = bool_series_from_keep(valid);
    if (out.kind() == SlateKind::Series && validity.platform() != out.as_series().platform()) {
      validity = validity.to(out.as_series().platform());
    }
    out = detail::with_validity_mask(out, validity);
  }
  return out;
}

} // namespace

DataFrame DataFrame::align(const AlignSpec& spec) const {
  const auto& src = impl_ref_();
  const Slate target_caselbl = spec.label.has_value() ? *spec.label : src.cases_;
  PLUSS_CHECK(target_caselbl.defined(), "DataFrame::align: target case labels are undefined");

  const auto target_features = align_target_features_(*this, spec, "DataFrame::align");
  std::vector<std::pair<std::string, Slate>> out;
  out.reserve(target_features.size());
  for (const auto& name : target_features) {
    const auto it = src.name_to_idx_.find(name);
    if (it == src.name_to_idx_.end()) {
      out.emplace_back(name, null_align_feature_(target_caselbl.size(), spec.fill, "DataFrame::align"));
      continue;
    }
    auto column = detail::with_display_labels(src.columns_[it->second], src.cases_);
    out.emplace_back(name, remap_align_feature_(column, target_caselbl, spec.fill, "DataFrame::align"));
  }

  return DataFrame(std::move(out), target_caselbl).title(src.title_);
}

DataFrame DataFrame::align(const Slate& new_caselbl) const {
  return align(AlignSpec{.label = new_caselbl});
}

DataFrame DataFrame::align_caselbl(const Slate& new_caselbl) const {
  return align(AlignSpec{.label = new_caselbl});
}

DataFrame DataFrame::align_caselbl(const Slate& new_caselbl, std::optional<double> fill) const {
  return align(AlignSpec{.label = new_caselbl, .fill = fill});
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
  spec.cases = FrameMatchRule::KeepLeft;
  return combine(other, spec);
}

namespace {

enum class UnifyHow : std::uint8_t {
  Common = 0,
  KeepLeft = 1,
  KeepRight = 2,
  Unite = 3,
  Cross = 4,
  LeftOnly = 5,
  RightOnly = 6,
};

enum class UnifyValidate : std::uint8_t {
  ManyToMany = 0,
  OneToOne = 1,
  OneToMany = 2,
  ManyToOne = 3,
};

std::string lowercase_ascii_copy(std::string_view text) {
  std::string out(text);
  std::transform(out.begin(), out.end(), out.begin(), [](char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  });
  return out;
}

UnifyHow parse_unify_how(std::string_view how) {
  const auto mode = lowercase_ascii_copy(how);
  if (mode.empty() || mode == "common" || mode == "inner") {
    return UnifyHow::Common;
  }
  if (mode == "keep_left" || mode == "keep-left" || mode == "left") {
    return UnifyHow::KeepLeft;
  }
  if (mode == "keep_right" || mode == "keep-right" || mode == "right") {
    return UnifyHow::KeepRight;
  }
  if (mode == "unite" || mode == "outer" || mode == "full") {
    return UnifyHow::Unite;
  }
  if (mode == "cross") {
    return UnifyHow::Cross;
  }
  if (mode == "left_only" || mode == "left-only" || mode == "left_anti") {
    return UnifyHow::LeftOnly;
  }
  if (mode == "right_only" || mode == "right-only" || mode == "right_anti") {
    return UnifyHow::RightOnly;
  }
  PLUSS_FAIL(
      "DataFrame::unify: supported modes are 'common', 'keep_left', "
      "'keep_right', 'unite', 'cross', 'left_only', and 'right_only'");
}

UnifyValidate parse_unify_validate(std::string_view validate) {
  const auto mode = lowercase_ascii_copy(validate);
  if (mode.empty() || mode == "many_to_many" || mode == "many-to-many" || mode == "m:m") {
    return UnifyValidate::ManyToMany;
  }
  if (mode == "one_to_one" || mode == "one-to-one" || mode == "1:1") {
    return UnifyValidate::OneToOne;
  }
  if (mode == "one_to_many" || mode == "one-to-many" || mode == "1:m") {
    return UnifyValidate::OneToMany;
  }
  if (mode == "many_to_one" || mode == "many-to-one" || mode == "m:1") {
    return UnifyValidate::ManyToOne;
  }
  PLUSS_FAIL(
      "DataFrame::unify: supported `validate` values are 'many_to_many', 'one_to_one', "
      "'one_to_many', and 'many_to_one'");
}

template <class T>
std::vector<std::string> numeric_unify_key_parts(
    const pluss::Series& values,
    const std::vector<bool>& valid) {
  const auto host = values.host<T>();
  std::vector<std::string> out(host.size());
  for (std::size_t i = 0; i < host.size(); ++i) {
    if (!valid[i]) {
      out[i].assign(1, '\0');
      continue;
    }
    if constexpr (std::is_floating_point_v<T>) {
      if (std::isnan(host[i])) {
        out[i].assign(1, '\0');
        continue;
      }
      const T normalized =
          host[i] == static_cast<T>(0) ? static_cast<T>(0) : host[i];
      out[i].assign(
          reinterpret_cast<const char*>(&normalized),
          sizeof(T));
    } else if constexpr (std::is_same_v<T, bool>) {
      out[i].assign(1, host[i] ? '\1' : '\0');
    } else {
      out[i].assign(
          reinterpret_cast<const char*>(&host[i]),
          sizeof(T));
    }
    out[i].insert(out[i].begin(), '\1');
  }
  return out;
}

std::vector<std::string> unify_key_parts(const Slate& values) {
  const auto count = static_cast<std::size_t>(values.size());
  auto valid = detail::has_validity_mask(values)
      ? detail::validity_mask(values)->host<bool>()
      : std::vector<bool>(count, true);

  switch (values.kind()) {
    case SlateKind::Series: {
      const auto& series = values.as_series();
      switch (series.scalar_type()) {
        case pluss::codem::ScalarType::Bool:
          return numeric_unify_key_parts<bool>(series, valid);
        case pluss::codem::ScalarType::UInt8:
          return numeric_unify_key_parts<std::uint8_t>(series, valid);
        case pluss::codem::ScalarType::UInt16:
          return numeric_unify_key_parts<std::uint16_t>(series, valid);
        case pluss::codem::ScalarType::UInt32:
          return numeric_unify_key_parts<std::uint32_t>(series, valid);
        case pluss::codem::ScalarType::UInt64:
          return numeric_unify_key_parts<std::uint64_t>(series, valid);
        case pluss::codem::ScalarType::Int8:
          return numeric_unify_key_parts<std::int8_t>(series, valid);
        case pluss::codem::ScalarType::Int16:
          return numeric_unify_key_parts<std::int16_t>(series, valid);
        case pluss::codem::ScalarType::Int32:
          return numeric_unify_key_parts<std::int32_t>(series, valid);
        case pluss::codem::ScalarType::Int64:
          return numeric_unify_key_parts<std::int64_t>(series, valid);
        case pluss::codem::ScalarType::Float16:
        case pluss::codem::ScalarType::BFloat16:
        case pluss::codem::ScalarType::Float8E4M3FN:
        case pluss::codem::ScalarType::Float8E5M2:
          return numeric_unify_key_parts<float>(
              series.to(pluss::codem::ScalarType::Float32),
              valid);
        case pluss::codem::ScalarType::Float32:
          return numeric_unify_key_parts<float>(series, valid);
        case pluss::codem::ScalarType::Float64:
          return numeric_unify_key_parts<double>(series, valid);
        case pluss::codem::ScalarType::Complex64:
          return numeric_unify_key_parts<pluss::codem::Complex64>(series, valid);
        case pluss::codem::ScalarType::Complex128:
          return numeric_unify_key_parts<pluss::codem::Complex128>(series, valid);
      }
      PLUSS_FAIL("DataFrame::unify: unsupported numeric key dtype");
    }
    case SlateKind::Note: {
      const auto notes = values.as_note().vector();
      std::vector<std::string> out(count);
      for (std::size_t i = 0; i < count; ++i) {
        out[i] = valid[i] ? std::string("\2") + std::string(notes[i])
                          : std::string(1, '\0');
      }
      return out;
    }
    case SlateKind::Timeline: {
      const auto ticks = values.as_timeline().epoch_nanos().host<std::int64_t>();
      std::vector<std::string> out(count);
      for (std::size_t i = 0; i < count; ++i) {
        if (!valid[i] || ticks[i] == pluss::Timeline::kNaT) {
          out[i].assign(1, '\0');
          continue;
        }
        out[i].assign(
            reinterpret_cast<const char*>(&ticks[i]),
            sizeof(std::int64_t));
        out[i].insert(out[i].begin(), '\3');
      }
      return out;
    }
    case SlateKind::Duration: {
      const auto nanos = values.as_duration().host();
      std::vector<std::string> out(count);
      for (std::size_t i = 0; i < count; ++i) {
        if (!valid[i] || nanos[i] == pluss::Duration::kNaT) {
          out[i].assign(1, '\0');
          continue;
        }
        out[i].assign(reinterpret_cast<const char*>(&nanos[i]), sizeof(std::int64_t));
        out[i].insert(out[i].begin(), '\4');
      }
      return out;
    }
  }
  PLUSS_FAIL("DataFrame::unify: unsupported key payload kind");
}

void ensure_unify_key_compatibility(
    const Slate& left,
    const Slate& right) {
  PLUSS_CHECK(
      left.kind() == right.kind(),
      "DataFrame::unify: paired key features must use the same Slate kind");
  if (left.kind() == SlateKind::Series) {
    PLUSS_CHECK(
        left.as_series().scalar_type() == right.as_series().scalar_type(),
        "DataFrame::unify: paired numeric key features must use the same dtype");
    PLUSS_CHECK(
        left.as_series().platform() == right.as_series().platform(),
        "DataFrame::unify: paired numeric key features must use the same platform");
  } else if (left.kind() == SlateKind::Timeline) {
    PLUSS_CHECK(
        left.as_timeline().epoch_nanos().platform() ==
            right.as_timeline().epoch_nanos().platform(),
        "DataFrame::unify: paired timeline key features must use the same platform");
  }
}

std::vector<std::string> make_unify_keys(
    const std::vector<Slate>& key_cols,
    const std::int64_t nrows) {
  std::vector<std::string> keys(static_cast<std::size_t>(nrows));
  for (const auto& column : key_cols) {
    const auto parts = unify_key_parts(column);
    for (std::size_t row = 0; row < keys.size(); ++row) {
      const auto part_size = static_cast<std::uint64_t>(parts[row].size());
      keys[row].append(
          reinterpret_cast<const char*>(&part_size),
          sizeof(part_size));
      keys[row].append(parts[row]);
    }
  }
  return keys;
}

[[noreturn]] void fail_unify_validate(std::string_view validate, std::string_view side) {
  throw ::pluss::Error(pluss::tils::OutputEngine::format(
      "DataFrame::unify: validate='{}' requires unique {} keys",
      validate.empty() ? "many_to_many" : std::string(validate),
      side));
}

void validate_unify_unique_side(const std::vector<std::string>& keys,
                                std::string_view validate,
                                std::string_view side) {
  std::unordered_set<std::string> seen;
  seen.reserve(keys.size());
  for (const auto& key : keys) {
    if (!seen.insert(key).second) {
      fail_unify_validate(validate, side);
    }
  }
}

pluss::Series bool_mask_from_values(
    const std::vector<bool>& values,
    const pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  auto out = pluss::Series::zeros(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Bool,
      pluss::codem::Platform::CPU);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    p[i] = values[i];
  }
  return platform == pluss::codem::Platform::CPU ? out : out.to(platform);
}

Slate make_unify_null_slate(const Slate& reference, std::int64_t size) {
  switch (reference.kind()) {
    case SlateKind::Series: {
      const auto platform = reference.as_series().platform();
      auto invalid = pluss::Series::zeros(
          {size},
          pluss::codem::ScalarType::Bool,
          platform);
      return Slate(
          pluss::Series::zeros(
              {size},
              reference.as_series().scalar_type(),
              platform),
          invalid);
    }
    case SlateKind::Note:
      return Slate(
          pluss::make_note(
              std::vector<std::string>(static_cast<std::size_t>(size))),
          pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Bool,
              pluss::codem::Platform::CPU));
    case SlateKind::Timeline: {
      const auto platform = reference.as_timeline().epoch_nanos().platform();
      return Slate(
          pluss::timeline::from_epoch(pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Int64,
              platform)),
          pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Bool,
              platform));
    }
    case SlateKind::Duration:
      return Slate(
          pluss::duration::nanos(
              std::vector<std::int64_t>(static_cast<std::size_t>(size), 0)),
          pluss::Series::zeros(
              {size}, pluss::codem::ScalarType::Bool, pluss::codem::Platform::CPU));
  }
  PLUSS_FAIL("DataFrame::unify: unsupported Slate kind");
}

Slate take_rows_for_unify(const Slate& source, const std::vector<std::int64_t>& rows) {
  if (rows.empty()) {
    return source.take(std::span<const std::int64_t>(rows.data(), rows.size()));
  }
  if (source.size() == 0) {
    return make_unify_null_slate(source, static_cast<std::int64_t>(rows.size()));
  }

  std::vector<std::int64_t> safe_rows;
  safe_rows.reserve(rows.size());
  std::vector<bool> out_valid(rows.size(), true);
  std::optional<std::vector<bool>> src_valid = std::nullopt;
  if (detail::has_validity_mask(source)) {
    src_valid = detail::validity_mask(source)->host<bool>();
  }

  bool all_valid = !src_valid.has_value();
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto row = rows[i];
    if (row < 0) {
      safe_rows.push_back(0);
      out_valid[i] = false;
      all_valid = false;
      continue;
    }
    PLUSS_CHECK(row < source.size(), "DataFrame::unify: matched row index out of bounds");
    safe_rows.push_back(row);
    const bool valid = src_valid.has_value() ? (*src_valid)[static_cast<std::size_t>(row)] : true;
    out_valid[i] = valid;
    all_valid = all_valid && valid;
  }

  auto out = source.take(safe_rows);
  if (all_valid) {
    return detail::has_validity_mask(out) ? detail::clear_validity_mask(out) : out;
  }
  pluss::codem::Platform platform = pluss::codem::Platform::CPU;
  if (out.kind() == SlateKind::Series) {
    platform = out.as_series().platform();
  } else if (out.kind() == SlateKind::Timeline) {
    platform = out.as_timeline().epoch_nanos().platform();
  }
  return detail::with_validity_mask(out, bool_mask_from_values(out_valid, platform));
}

Slate take_unified_key_rows(
    const Slate& left,
    const Slate& right,
    const std::vector<std::int64_t>& left_rows,
    const std::vector<std::int64_t>& right_rows) {
  auto output = detail::clear_display_labels(take_rows_for_unify(left, left_rows));
  const auto fallback = detail::clear_display_labels(take_rows_for_unify(right, right_rows));
  std::vector<bool> use_right(left_rows.size(), false);
  std::transform(
      left_rows.begin(),
      left_rows.end(),
      use_right.begin(),
      [](const std::int64_t row) { return row < 0; });

  pluss::codem::Platform platform = pluss::codem::Platform::CPU;
  if (output.kind() == SlateKind::Series) {
    platform = output.as_series().platform();
  } else if (output.kind() == SlateKind::Timeline) {
    platform = output.as_timeline().epoch_nanos().platform();
  }
  return output.put(
      Slate(bool_mask_from_values(use_right, platform)),
      fallback);
}

} // namespace

CategoryEncoding DataFrame::factor(
    const std::string_view feature,
    const bool ordered,
    const bool sort_levels) const {
  const auto source = features(feature);
  auto levels = source.dropnull().distinct(false);
  if (sort_levels) {
    levels = levels.arrange(true, true);
  }

  const auto level_parts = unify_key_parts(levels);
  std::unordered_map<std::string, std::int64_t> code_by_level;
  code_by_level.reserve(level_parts.size());
  for (std::int64_t code = 0; code < levels.size(); ++code) {
    code_by_level.emplace(
        level_parts[static_cast<std::size_t>(code)],
        code);
  }

  const auto source_parts = unify_key_parts(source);
  const auto valid = source.has_validity_mask()
      ? source.validity_mask()->host<bool>()
      : std::vector<bool>(static_cast<std::size_t>(source.size()), true);
  std::vector<std::int64_t> codes(static_cast<std::size_t>(source.size()), -1);
  for (std::size_t row = 0; row < codes.size(); ++row) {
    if (!valid[row]) {
      continue;
    }
    const auto found = code_by_level.find(source_parts[row]);
    PLUSS_CHECK(
        found != code_by_level.end(),
        "DataFrame::factor: internal level lookup failure");
    codes[row] = found->second;
  }

  auto code_values = pluss::Series::empty(
      {source.size()},
      pluss::codem::ScalarType::Int64,
      pluss::codem::Platform::CPU);
  if (!codes.empty()) {
    std::memcpy(
        code_values.raw_data(),
        codes.data(),
        codes.size() * sizeof(std::int64_t));
  }
  auto platform = pluss::codem::Platform::CPU;
  if (source.kind() == SlateKind::Series) {
    platform = source.as_series().platform();
  } else if (source.kind() == SlateKind::Timeline) {
    platform = source.as_timeline().epoch_nanos().platform();
  }
  if (platform != pluss::codem::Platform::CPU) {
    code_values = code_values.to(platform);
  }
  Slate code_slate(code_values);
  if (source.has_validity_mask()) {
    auto mask = *source.validity_mask();
    if (mask.platform() != platform) {
      mask = mask.to(platform);
    }
    code_slate = code_slate.with_validity_mask(mask);
  }
  return CategoryEncoding{
      std::move(code_slate),
      std::move(levels),
      ordered};
}

DataFrame DataFrame::encode(
    const std::string_view feature,
    const EncodeMode mode,
    const EncodeOptions& options) const {
  const auto& src = impl_ref_();
  const auto key = owned_name(feature);
  const auto it = src.name_to_idx_.find(key);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::encode: unknown feature '" + key + "'", key, pluss::exception::map_keys(src.name_to_idx_));
  const auto index = it->second;

  if (mode == EncodeMode::Ordinal) {
    const auto factored = factor(feature, options.ordered, options.sort_levels);
    auto out_cols = src.columns_;
    out_cols[index] = factored.codes;
    return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
        src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
  }

  PLUSS_CHECK(mode == EncodeMode::OneHot, "DataFrame::encode: unknown EncodeMode");

  const auto& source = src.columns_[index];
  auto levels = source.dropnull().distinct(false);
  if (options.sort_levels) {
    levels = levels.arrange(true, true);
  }

  const auto level_parts = unify_key_parts(levels);
  const auto source_parts = unify_key_parts(source);
  const auto n = source.size();
  const auto valid = source.has_validity_mask()
      ? source.validity_mask()->host<bool>()
      : std::vector<bool>(static_cast<std::size_t>(n), true);
  const bool any_invalid = std::any_of(valid.begin(), valid.end(), [](bool v) { return !v; });

  const auto level_count = static_cast<std::int64_t>(level_parts.size());
  const std::int64_t start_level = (options.drop_first && level_count > 0) ? 1 : 0;
  const auto prefix = options.prefix.empty() ? (key + "_") : options.prefix;

  std::vector<std::pair<std::string, Slate>> indicator_columns;
  indicator_columns.reserve(static_cast<std::size_t>(level_count - start_level));
  for (std::int64_t lvl = start_level; lvl < level_count; ++lvl) {
    const auto& level_key = level_parts[static_cast<std::size_t>(lvl)];
    std::vector<bool> flags(static_cast<std::size_t>(n), false);
    for (std::int64_t row = 0; row < n; ++row) {
      const auto r = static_cast<std::size_t>(row);
      if (valid[r] && source_parts[r] == level_key) {
        flags[r] = true;
      }
    }
    Slate indicator_slate(bool_series_from_keep(flags));
    if (any_invalid) {
      indicator_slate = indicator_slate.with_validity_mask(bool_series_from_keep(valid));
    }
    indicator_columns.emplace_back(prefix + level_key, std::move(indicator_slate));
  }

  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(src.names_.size() - 1 + indicator_columns.size());
  out_cols.reserve(src.names_.size() - 1 + indicator_columns.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (i == index) {
      for (auto& entry : indicator_columns) {
        out_names.push_back(std::move(entry.first));
        out_cols.push_back(std::move(entry.second));
      }
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

Slate CategoryEncoding::decode() const {
  PLUSS_CHECK(codes.defined(), "CategoryEncoding::decode: codes are undefined");
  PLUSS_CHECK(levels.defined(), "CategoryEncoding::decode: levels are undefined");
  PLUSS_CHECK(
      codes.kind() == SlateKind::Series &&
          codes.as_series().scalar_type() == pluss::codem::ScalarType::Int64,
      "CategoryEncoding::decode: codes must be an Int64 Series Slate");

  const auto raw_codes = codes.as_series().host<std::int64_t>();
  std::vector<std::int64_t> safe(raw_codes.size(), 0);
  for (std::size_t row = 0; row < raw_codes.size(); ++row) {
    if (raw_codes[row] < 0) {
      continue;
    }
    PLUSS_CHECK(
        raw_codes[row] < levels.size(),
        "CategoryEncoding::decode: code is outside the level range");
    safe[row] = raw_codes[row];
  }

  Slate output;
  if (levels.size() > 0 || safe.empty()) {
    output = levels.take(safe);
  } else {
    const auto size = static_cast<std::int64_t>(safe.size());
    switch (levels.kind()) {
      case SlateKind::Series:
        output = Slate(pluss::Series::zeros(
            {size},
            levels.as_series().scalar_type(),
            levels.as_series().platform()));
        break;
      case SlateKind::Note:
        output = Slate(pluss::make_note(
            std::vector<std::string>(safe.size())));
        break;
      case SlateKind::Timeline:
        output = Slate(pluss::timeline::from_epoch(pluss::Series::zeros(
            {size},
            pluss::codem::ScalarType::Int64,
            levels.as_timeline().epoch_nanos().platform())));
        break;
      case SlateKind::Duration:
        output = Slate(pluss::duration::nanos(
            std::vector<std::int64_t>(static_cast<std::size_t>(size), 0)));
        break;
    }
  }
  if (detail::has_validity_mask(codes)) {
    auto mask = *detail::validity_mask(codes);
    const auto platform =
        output.kind() == SlateKind::Series
            ? output.as_series().platform()
            : (output.kind() == SlateKind::Timeline
                   ? output.as_timeline().epoch_nanos().platform()
                   : pluss::codem::Platform::CPU);
    if (mask.platform() != platform) {
      mask = mask.to(platform);
    }
    output = detail::with_validity_mask(output, mask);
  }
  return output;
}

DataFrame::LetSlot DataFrame::set(std::string_view name) {
  return LetSlot(*this, owned_name(name));
}

DataFrame::LetSlot DataFrame::set(std::string_view name, const AlignOptions& options) {
  return LetSlot(*this, owned_name(name), options);
}

DataFrame::LetSlots DataFrame::set(pluss::msharp::RangeView::Of<std::string_view> names) {
  return LetSlots(*this, normalize_name_list(names));
}

DataFrame::LetSlots DataFrame::set(pluss::msharp::RangeView::Of<std::string_view> names,
                                   const AlignOptions& options) {
  return LetSlots(*this, normalize_name_list(names), options);
}

DataFrame::LetSlots DataFrame::set(std::initializer_list<const char*> names) {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return set(pluss::msharp::RangeView::Of<std::string_view>(views));
}

DataFrame::LetSlots DataFrame::set(std::initializer_list<const char*> names,
                                   const AlignOptions& options) {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return set(pluss::msharp::RangeView::Of<std::string_view>(views), options);
}

DataFrame DataFrame::set_features(std::string_view name, const Slate& value) const {
  return set_features(name, value, AlignOptions{});
}

DataFrame& DataFrame::set_(std::string_view name, const Slate& value) {
  return set_features_(name, value, AlignOptions{});
}

DataFrame& DataFrame::set_(std::string_view name, const Slate& value, const AlignOptions& options) {
  return set_features_(name, value, options);
}

DataFrame& DataFrame::set_(pluss::msharp::RangeView::Of<std::string_view> names,
                           pluss::msharp::RangeView::Of<Slate> values) {
  return set_features_(names, values);
}

DataFrame DataFrame::set_features(std::string_view name, const Slate& value, const AlignOptions& options) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(value.defined(), "DataFrame::set_features: input feature is undefined");
  const auto prepared = prepare_feature_assignment_(src, value, "DataFrame::set_features", options);

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  const auto key = owned_name(name);
  const auto it = src.name_to_idx_.find(key);
  if (it == src.name_to_idx_.end()) {
    out_names.push_back(key);
    out_cols.push_back(prepared);
  } else {
    out_cols[it->second] = prepared;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame& DataFrame::set_features_(std::string_view name, const Slate& value) {
  return set_features_(name, value, AlignOptions{});
}

DataFrame& DataFrame::set_features_(std::string_view name, const Slate& value, const AlignOptions& options) {
  auto& w = mut_impl_();
  PLUSS_CHECK(value.defined(), "DataFrame::set_features_: input feature is undefined");
  const auto prepared = prepare_feature_assignment_(w, value, "DataFrame::set_features_", options);

  const auto key = owned_name(name);
  const auto it = w.name_to_idx_.find(key);
  if (it == w.name_to_idx_.end()) {
    w.names_.push_back(key);
    w.columns_.push_back(prepared);
  } else {
    w.columns_[it->second] = prepared;
  }
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::set_features(pluss::msharp::RangeView::Of<std::string_view> names,
                                  pluss::msharp::RangeView::Of<Slate> values) const {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features: names/value size mismatch");
  DataFrame out = *this;
  for (std::size_t i = 0; i < names.size(); ++i) {
    out = out.set_features(names[i], values[i]);
  }
  return out;
}

DataFrame& DataFrame::set_features_(pluss::msharp::RangeView::Of<std::string_view> names,
                                    pluss::msharp::RangeView::Of<Slate> values) {
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
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::rename: unknown column '" + old_key + "'", old_key, pluss::exception::map_keys(src.name_to_idx_));
  if (old_key != new_name) {
    PLUSS_CHECK(src.name_to_idx_.find(new_name) == src.name_to_idx_.end(), "DataFrame::rename: target name already exists");
  }

  auto out_names = src.names_;
  out_names[it->second] = std::move(new_name);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
}

DataFrame& DataFrame::rename_(std::string_view old_name, std::string new_name) {
  auto& w = mut_impl_();
  const auto old_key = owned_name(old_name);
  const auto it = w.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::rename_: unknown column '" + old_key + "'", old_key, pluss::exception::map_keys(w.name_to_idx_));
  if (old_key != new_name) {
    PLUSS_CHECK(w.name_to_idx_.find(new_name) == w.name_to_idx_.end(), "DataFrame::rename_: target name already exists");
  }

  w.names_[it->second] = std::move(new_name);
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::rename(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> renames) const {
  DataFrame out = *this;
  for (const auto& [old_name, new_name] : renames) {
    out = out.rename(old_name, std::string(new_name));
  }
  return out;
}

DataFrame DataFrame::rename(pluss::msharp::RangeView::Of<std::string_view> feature_names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(feature_names.size() == src.names_.size(),
              "DataFrame::rename: number of names must match feature count");
  std::vector<std::string> out_names;
  out_names.reserve(feature_names.size());
  std::unordered_set<std::string> seen;
  seen.reserve(feature_names.size());
  for (const auto n : feature_names) {
    const auto key = owned_name(n);
    PLUSS_CHECK(!key.empty(), "DataFrame::rename: feature name cannot be empty");
    PLUSS_CHECK(seen.insert(key).second,
                "DataFrame::rename: duplicate feature name '" + key + "'");
    out_names.push_back(key);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::rename(std::initializer_list<const char*> feature_names) const {
  std::vector<std::string_view> names;
  names.reserve(feature_names.size());
  for (const auto* name : feature_names) {
    PLUSS_CHECK(name != nullptr, "DataFrame::rename: feature name cannot be null");
    names.emplace_back(name);
  }
  return rename(pluss::RangeView<std::string_view>(names));
}

DataFrame DataFrame::set_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const {
  return rename(feature_names);
}

DataFrame DataFrame::conform(const DataFrame& other) const {
  const auto& src = impl_ref_();
  const auto ordered = other.features();
  PLUSS_CHECK(ordered.size() == src.names_.size(),
              "DataFrame::conform: both frames must have same feature count");

  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(ordered.size());
  out_cols.reserve(ordered.size());
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::conform: duplicate feature in target '" + n + "'");
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::conform: missing feature '" + n + "'", n, pluss::exception::map_keys(src.name_to_idx_));
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
  return out.labels().align(other.labels().values());
}

DataFrame DataFrame::unify(std::string_view left_on,
                           const DataFrame& right,
                           std::string_view right_on,
                           std::string_view how,
                           std::string right_suffix,
                           std::string_view validate) const {
  std::vector<std::string_view> left_keys;
  if (!left_on.empty()) {
    left_keys.push_back(left_on);
  }
  std::vector<std::string_view> right_keys;
  if (!right_on.empty()) {
    right_keys.push_back(right_on);
  }
  return unify(pluss::msharp::RangeView::Of<std::string_view>(left_keys),
               right,
               pluss::msharp::RangeView::Of<std::string_view>(right_keys),
               how,
               std::move(right_suffix),
               validate);
}

DataFrame DataFrame::unify(pluss::msharp::RangeView::Of<std::string_view> left_on,
                           const DataFrame& right,
                           pluss::msharp::RangeView::Of<std::string_view> right_on,
                           std::string_view how,
                           std::string right_suffix,
                           std::string_view validate) const {
  const auto join_how = parse_unify_how(how);
  PLUSS_CHECK(
      join_how == UnifyHow::Cross || left_on.size() > 0,
      "DataFrame::unify: at least one left key feature is required");
  PLUSS_CHECK(
      join_how != UnifyHow::Cross || (left_on.size() == 0 && right_on.size() == 0),
      "DataFrame::unify(cross): key features must be empty");
  const auto relationship = parse_unify_validate(validate);
  std::vector<std::string> left_keys = normalize_name_list(left_on);
  std::vector<std::string> right_keys =
      (right_on.size() == 0) ? left_keys : normalize_name_list(right_on);
  PLUSS_CHECK(left_keys.size() == right_keys.size(),
              "DataFrame::unify: left/right key feature counts must match");

  const auto& left_src = impl_ref_();
  const auto& right_src = right.impl_ref_();

  std::vector<Slate> left_key_cols;
  std::vector<Slate> right_key_cols;
  left_key_cols.reserve(left_keys.size());
  right_key_cols.reserve(right_keys.size());
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    PLUSS_CHECK(has_feature(left_keys[i]),
                "DataFrame::unify: left key feature not found '" + left_keys[i] + "'");
    PLUSS_CHECK(right.get(right_keys[i]).has_value(),
                "DataFrame::unify: right key feature not found '" + right_keys[i] + "'");
    left_key_cols.push_back(features(left_keys[i]));
    right_key_cols.push_back(right.features(right_keys[i]));
    ensure_unify_key_compatibility(left_key_cols.back(), right_key_cols.back());
  }

  const auto left_hash_keys = make_unify_keys(left_key_cols, ncases());
  const auto right_hash_keys = make_unify_keys(right_key_cols, right.ncases());
  if (relationship == UnifyValidate::OneToOne || relationship == UnifyValidate::OneToMany) {
    validate_unify_unique_side(left_hash_keys, validate, "left");
  }
  if (relationship == UnifyValidate::OneToOne || relationship == UnifyValidate::ManyToOne) {
    validate_unify_unique_side(right_hash_keys, validate, "right");
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> right_lookup;
  right_lookup.reserve(static_cast<std::size_t>(right.ncases()));
  for (std::int64_t r = 0; r < right.ncases(); ++r) {
    right_lookup[right_hash_keys[static_cast<std::size_t>(r)]].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  if (join_how == UnifyHow::Cross) {
    PLUSS_CHECK(
        ncases() == 0 ||
            right.ncases() <=
                std::numeric_limits<std::int64_t>::max() / ncases(),
        "DataFrame::unify(cross): result case count overflows int64");
    const auto result_size = ncases() * right.ncases();
    left_rows.reserve(static_cast<std::size_t>(result_size));
    right_rows.reserve(static_cast<std::size_t>(result_size));
    for (std::int64_t left_row = 0; left_row < ncases(); ++left_row) {
      for (std::int64_t right_row = 0; right_row < right.ncases(); ++right_row) {
        left_rows.push_back(left_row);
        right_rows.push_back(right_row);
      }
    }
  } else if (
      join_how == UnifyHow::KeepRight ||
      join_how == UnifyHow::RightOnly) {
    std::unordered_map<std::string, std::vector<std::int64_t>> left_lookup;
    left_lookup.reserve(static_cast<std::size_t>(ncases()));
    for (std::int64_t left_row = 0; left_row < ncases(); ++left_row) {
      left_lookup[left_hash_keys[static_cast<std::size_t>(left_row)]].push_back(left_row);
    }
    for (std::int64_t right_row = 0; right_row < right.ncases(); ++right_row) {
      const auto found =
          left_lookup.find(right_hash_keys[static_cast<std::size_t>(right_row)]);
      if (found == left_lookup.end()) {
        left_rows.push_back(-1);
        right_rows.push_back(right_row);
        continue;
      }
      if (join_how == UnifyHow::RightOnly) {
        continue;
      }
      for (const auto left_row : found->second) {
        left_rows.push_back(left_row);
        right_rows.push_back(right_row);
      }
    }
  } else {
    std::vector<bool> matched_right(
        static_cast<std::size_t>(right.ncases()),
        false);
    for (std::int64_t left_row = 0; left_row < ncases(); ++left_row) {
      const auto found =
          right_lookup.find(left_hash_keys[static_cast<std::size_t>(left_row)]);
      if (found == right_lookup.end()) {
        if (join_how == UnifyHow::KeepLeft ||
            join_how == UnifyHow::Unite ||
            join_how == UnifyHow::LeftOnly) {
          left_rows.push_back(left_row);
          right_rows.push_back(-1);
        }
        continue;
      }
      if (join_how == UnifyHow::LeftOnly) {
        continue;
      }
      for (const auto right_row : found->second) {
        matched_right[static_cast<std::size_t>(right_row)] = true;
        left_rows.push_back(left_row);
        right_rows.push_back(right_row);
      }
    }
    if (join_how == UnifyHow::Unite) {
      for (std::int64_t right_row = 0; right_row < right.ncases(); ++right_row) {
        if (matched_right[static_cast<std::size_t>(right_row)]) {
          continue;
        }
        left_rows.push_back(-1);
        right_rows.push_back(right_row);
      }
    }
  }

  std::unordered_map<std::string, std::size_t> shared_key_position;
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    if (left_keys[i] == right_keys[i]) {
      shared_key_position.emplace(left_keys[i], i);
    }
  }

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(left_src.names_.size() + right_src.names_.size());
  for (std::size_t i = 0; i < left_src.names_.size(); ++i) {
    const auto shared = shared_key_position.find(left_src.names_[i]);
    if (shared != shared_key_position.end()) {
      const auto key_position = shared->second;
      out_cols.emplace_back(
          left_src.names_[i],
          take_unified_key_rows(
              left_key_cols[key_position],
              right_key_cols[key_position],
              left_rows,
              right_rows));
    } else {
      out_cols.emplace_back(
          left_src.names_[i],
          take_rows_for_unify(left_src.columns_[i], left_rows));
    }
  }

  std::unordered_set<std::string> skip_right_keys;
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    if (left_keys[i] == right_keys[i]) {
      skip_right_keys.insert(right_keys[i]);
    }
  }

  std::unordered_set<std::string> used_names;
  used_names.reserve(out_cols.size() + right_src.names_.size());
  for (const auto& [name, _] : out_cols) {
    used_names.insert(name);
  }

  for (std::size_t i = 0; i < right_src.names_.size(); ++i) {
    const auto& base = right_src.names_[i];
    if (skip_right_keys.find(base) != skip_right_keys.end()) {
      continue;
    }
    std::string out_name = base;
    if (used_names.find(out_name) != used_names.end()) {
      out_name = base + right_suffix;
      std::size_t k = 2;
      while (used_names.find(out_name) != used_names.end()) {
        out_name = pluss::tils::OutputEngine::format("{}{}_{}", base, right_suffix, k++);
      }
    }
    used_names.insert(out_name);
    out_cols.emplace_back(std::move(out_name), take_rows_for_unify(right_src.columns_[i], right_rows));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame DataFrame::unify(std::initializer_list<std::string_view> left_on,
                           const DataFrame& right,
                           std::initializer_list<std::string_view> right_on,
                           std::string_view how,
                           std::string right_suffix,
                           std::string_view validate) const {
  return unify(pluss::msharp::RangeView::Of<std::string_view>(left_on),
               right,
               pluss::msharp::RangeView::Of<std::string_view>(right_on),
               how,
               std::move(right_suffix),
               validate);
}
