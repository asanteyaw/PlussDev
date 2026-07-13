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
std::pair<std::int64_t, std::int64_t> DataFrame::shape() const {
  return {ncases(), nfeats()};
}

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
  PLUSS_CHECK(it != entries_.end(), "DataFrame::DTypes::at: unknown feature '" + std::string(feature) + "'");
  return it->second;
}

const std::string& DataFrame::DTypes::operator[](std::string_view feature) const {
  return at(feature);
}

const std::vector<DataFrame::DTypes::Entry>& DataFrame::DTypes::entries() const noexcept {
  return entries_;
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
const std::vector<std::string>& DataFrame::features() const { return impl_ref_().names_; }

DataFrame::DTypes DataFrame::dtypes() const {
  const auto& src = impl_ref_();
  std::vector<DTypes::Entry> entries;
  entries.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto dtype = [&]() -> std::string {
      switch (src.columns_[i].kind()) {
        case ThreadKind::Series:
          return "Series";
        case ThreadKind::Note:
          return "Note";
        case ThreadKind::Timeline:
          return "Timeline";
        case ThreadKind::TimeIndex:
          return "TimeIndex";
      }
      return "Unknown";
    }();
    entries.emplace_back(src.names_[i], dtype);
  }
  return DTypes(std::move(entries));
}

const Thread& DataFrame::cases() const { return impl_ref_().cases_; }

const Thread& DataFrame::caselbl() const { return cases(); }

DataFrame DataFrame::caselbl(const Thread& labels) const {
  return with_cases(labels);
}

DataFrame& DataFrame::caselbl_(const Thread& labels) {
  return with_cases_(labels);
}

namespace {

std::string selector_label_from_scalar_thread_(const Thread& value, std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": selector thread is undefined");
  PLUSS_CHECK(value.size() == 1, std::string(where) + ": selector thread must contain exactly one label");
  return thread_scalar_string_at(value, 0);
}

std::vector<std::string> selector_labels_from_thread_(const Thread& values, std::string_view where) {
  PLUSS_CHECK(values.defined(), std::string(where) + ": selector thread is undefined");
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(values.size()));
  for (std::int64_t i = 0; i < values.size(); ++i) {
    out.push_back(thread_scalar_string_at(values, i));
  }
  return out;
}

std::unordered_map<std::string, std::vector<std::int64_t>> build_case_lookup_(const DataFrame& frame) {
  std::unordered_map<std::string, std::vector<std::int64_t>> lookup;
  lookup.reserve(static_cast<std::size_t>(std::max<std::int64_t>(8, frame.ncases() * 2)));
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    lookup[thread_scalar_string_at(frame.caselbl(), i)].push_back(i);
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
    key += level.values.is_valid(row) ? thread_scalar_string_at(level.values, row) : std::string("NA");
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
      label += values.is_valid(row) ? thread_scalar_string_at(values, row) : std::string("NA");
    }
    labels.push_back(std::move(label));
  }
  return labels;
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

std::vector<std::int64_t> resolve_case_positions_(const DataFrame& frame, const DataFrame::CaseSelector& selector) {
  std::vector<std::int64_t> indices;
  switch (selector.kind) {
    case DataFrame::CaseSelector::Kind::PositionScalar:
      indices.push_back(normalize_row_index(selector.first, frame.ncases()));
      return indices;
    case DataFrame::CaseSelector::Kind::PositionSlice: {
      const auto [start, stop] = normalize_slice(selector.first, selector.second, frame.ncases());
      indices.reserve(static_cast<std::size_t>(stop - start));
      for (std::int64_t i = start; i < stop; ++i) {
        indices.push_back(i);
      }
      return indices;
    }
    case DataFrame::CaseSelector::Kind::PositionList:
      indices.reserve(selector.positions.size());
      for (const auto row : selector.positions) {
        indices.push_back(normalize_row_index(row, frame.ncases()));
      }
      return indices;
    case DataFrame::CaseSelector::Kind::Mask:
      PLUSS_FAIL("resolve_case_positions_: mask selectors must be handled by DataFrame::case_study directly");
    case DataFrame::CaseSelector::Kind::LabelScalar:
    case DataFrame::CaseSelector::Kind::LabelList:
    case DataFrame::CaseSelector::Kind::LabelSlice:
      break;
  }

  const auto lookup = build_case_lookup_(frame);
  if (selector.kind == DataFrame::CaseSelector::Kind::LabelScalar) {
    const auto& key = selector.labels.front();
    const auto it = lookup.find(key);
    PLUSS_CHECK(it != lookup.end(), "DataFrame::case_study: case label not found '" + key + "'");
    return it->second;
  }

  if (selector.kind == DataFrame::CaseSelector::Kind::LabelList) {
    for (const auto& key : selector.labels) {
      const auto it = lookup.find(key);
      PLUSS_CHECK(it != lookup.end(), "DataFrame::case_study: case label not found '" + key + "'");
      indices.insert(indices.end(), it->second.begin(), it->second.end());
    }
    return indices;
  }

  const auto& start_key = selector.labels.at(0);
  const auto& stop_key = selector.labels.at(1);
  const auto start_it = lookup.find(start_key);
  const auto stop_it = lookup.find(stop_key);
  PLUSS_CHECK(start_it != lookup.end(), "DataFrame::case_study: case label not found '" + start_key + "'");
  PLUSS_CHECK(stop_it != lookup.end(), "DataFrame::case_study: case label not found '" + stop_key + "'");
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

std::vector<std::size_t> resolve_feature_positions_(const DataFrame& frame, const DataFrame::FeatureSelector& selector) {
  std::vector<std::size_t> indices;
  switch (selector.kind) {
    case DataFrame::FeatureSelector::Kind::All:
      indices.resize(static_cast<std::size_t>(frame.nfeats()));
      std::iota(indices.begin(), indices.end(), static_cast<std::size_t>(0));
      return indices;
    case DataFrame::FeatureSelector::Kind::PositionScalar:
      indices.push_back(static_cast<std::size_t>(normalize_row_index(selector.first, frame.nfeats())));
      return indices;
    case DataFrame::FeatureSelector::Kind::PositionSlice: {
      const auto [start, stop] = normalize_slice(selector.first, selector.second, frame.nfeats());
      indices.reserve(static_cast<std::size_t>(stop - start));
      for (std::int64_t i = start; i < stop; ++i) {
        indices.push_back(static_cast<std::size_t>(i));
      }
      return indices;
    }
    case DataFrame::FeatureSelector::Kind::PositionList:
      indices.reserve(selector.positions.size());
      for (const auto pos : selector.positions) {
        indices.push_back(static_cast<std::size_t>(normalize_row_index(pos, frame.nfeats())));
      }
      return indices;
    case DataFrame::FeatureSelector::Kind::NameScalar:
    case DataFrame::FeatureSelector::Kind::NameList:
    case DataFrame::FeatureSelector::Kind::NameSlice:
      break;
  }

  const auto& names = frame.features();
  std::unordered_map<std::string, std::size_t> by_name;
  by_name.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    by_name.emplace(names[i], i);
  }

  if (selector.kind == DataFrame::FeatureSelector::Kind::NameScalar) {
    const auto& key = selector.names.front();
    const auto it = by_name.find(key);
    PLUSS_CHECK(it != by_name.end(), "DataFrame::case_study: unknown feature '" + key + "'");
    indices.push_back(it->second);
    return indices;
  }

  if (selector.kind == DataFrame::FeatureSelector::Kind::NameList) {
    indices.reserve(selector.names.size());
    for (const auto& key : selector.names) {
      const auto it = by_name.find(key);
      PLUSS_CHECK(it != by_name.end(), "DataFrame::case_study: unknown feature '" + key + "'");
      indices.push_back(it->second);
    }
    return indices;
  }

  const auto& start_key = selector.names.at(0);
  const auto& stop_key = selector.names.at(1);
  const auto start_it = by_name.find(start_key);
  const auto stop_it = by_name.find(stop_key);
  PLUSS_CHECK(start_it != by_name.end(), "DataFrame::case_study: unknown feature '" + start_key + "'");
  PLUSS_CHECK(stop_it != by_name.end(), "DataFrame::case_study: unknown feature '" + stop_key + "'");
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

bool DataFrame::has_levels() const {
  return impl_ref_().case_axis_.defined();
}

std::vector<std::string> DataFrame::levels() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out;
  out.reserve(src.case_axis_.levels.size());
  for (const auto& level : src.case_axis_.levels) {
    out.push_back(level.name);
  }
  return out;
}

Thread DataFrame::level(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.case_axis_.levels[level_index_(src, name, "DataFrame::level")].values;
}

Thread DataFrame::level(std::int64_t position) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::level: current DataFrame has no case levels");
  auto pos = position < 0 ? position + static_cast<std::int64_t>(src.case_axis_.levels.size()) : position;
  PLUSS_CHECK(pos >= 0 && pos < static_cast<std::int64_t>(src.case_axis_.levels.size()),
              "DataFrame::level: level position out of bounds");
  return src.case_axis_.levels[static_cast<std::size_t>(pos)].values;
}

DataFrame DataFrame::case_study(const CaseSelector& rows) const {
  return case_study(rows, all_features());
}

DataFrame DataFrame::case_study(const CasePick& pick) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::case_study: current DataFrame has no case levels");

  std::vector<std::string> level_names = pick.levels;
  std::vector<std::string> values = pick.values;
  if (!pick.level.empty()) {
    PLUSS_CHECK(level_names.empty(), "DataFrame::case_study: use either `level` or `levels`, not both");
    level_names.push_back(pick.level);
    values.push_back(pick.value);
  }
  if (!pick.key.empty()) {
    PLUSS_CHECK(level_names.empty(), "DataFrame::case_study: use either `key` or level/value selectors, not both");
    PLUSS_CHECK(pick.key.size() == src.case_axis_.levels.size(),
                "DataFrame::case_study: full key length must match case level count");
    for (const auto& level : src.case_axis_.levels) {
      level_names.push_back(level.name);
    }
    values = pick.key;
  }
  PLUSS_CHECK(!level_names.empty(), "DataFrame::case_study: expected a level selector or full key");
  PLUSS_CHECK(level_names.size() == values.size(), "DataFrame::case_study: levels/values length mismatch");

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
      const auto observed = column.is_valid(row) ? thread_scalar_string_at(column, row) : std::string("NA");
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
    out = out.drop_levels(pluss::msharp::RangeView::Of<std::string_view>(
        std::span<const std::string_view>(drop_views.data(), drop_views.size())));
  }
  return out;
}

DataFrame DataFrame::case_study(const CaseSelector& rows, const FeatureSelector& selected_features) const {
  if (rows.kind == CaseSelector::Kind::Mask) {
    auto filtered = filter(rows.mask);
    return project_feature_positions_(filtered, resolve_feature_positions_(filtered, selected_features));
  }
  const auto case_positions = resolve_case_positions_(*this, rows);
  auto out = take(case_positions);
  return project_feature_positions_(out, resolve_feature_positions_(out, selected_features));
}

DataFrame::CaseSelector pos(std::int64_t row) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::PositionScalar;
  out.first = row;
  return out;
}

DataFrame::CaseSelector pos(std::int64_t start, std::int64_t stop) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::PositionSlice;
  out.first = start;
  out.second = stop;
  return out;
}

DataFrame::CaseSelector pos(std::initializer_list<std::int64_t> rows) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::PositionList;
  out.positions.assign(rows.begin(), rows.end());
  return out;
}

DataFrame::CaseSelector pos(pluss::RangeView<std::int64_t> rows) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::PositionList;
  out.positions.assign(rows.begin(), rows.end());
  return out;
}

DataFrame::CaseSelector label(std::string_view value) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelScalar;
  out.labels.emplace_back(value);
  return out;
}

DataFrame::CaseSelector label(const char* value) {
  return label(std::string_view{value != nullptr ? value : ""});
}

DataFrame::CaseSelector label(std::int64_t value) {
  return label(fmt::format("{}", value));
}

DataFrame::CaseSelector label(double value) {
  return label(fmt::format("{:.12g}", value));
}

DataFrame::CaseSelector labels(std::initializer_list<std::string_view> values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelList;
  out.labels.reserve(values.size());
  for (const auto value : values) {
    out.labels.emplace_back(value);
  }
  return out;
}

DataFrame::CaseSelector labels(pluss::RangeView<std::string_view> values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelList;
  out.labels.reserve(values.size());
  for (const auto value : values) {
    out.labels.emplace_back(value);
  }
  return out;
}

DataFrame::CaseSelector labels(std::initializer_list<std::int64_t> values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelList;
  out.labels.reserve(values.size());
  for (const auto value : values) {
    out.labels.push_back(fmt::format("{}", value));
  }
  return out;
}

DataFrame::CaseSelector labels(pluss::RangeView<std::int64_t> values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelList;
  out.labels.reserve(values.size());
  for (const auto value : values) {
    out.labels.push_back(fmt::format("{}", value));
  }
  return out;
}

DataFrame::CaseSelector labels(const Thread& values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelList;
  out.labels = selector_labels_from_thread_(values, "labels(Thread)");
  return out;
}

DataFrame::CaseSelector label_range(std::string_view start, std::string_view stop) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::LabelSlice;
  out.labels.emplace_back(start);
  out.labels.emplace_back(stop);
  return out;
}

DataFrame::CaseSelector label_range(std::int64_t start, std::int64_t stop) {
  return label_range(fmt::format("{}", start), fmt::format("{}", stop));
}

DataFrame::CaseSelector label_range(double start, double stop) {
  return label_range(fmt::format("{:.12g}", start), fmt::format("{:.12g}", stop));
}

DataFrame::CaseSelector label_range(const Thread& start, const Thread& stop) {
  return label_range(selector_label_from_scalar_thread_(start, "label_range(start)"),
                     selector_label_from_scalar_thread_(stop, "label_range(stop)"));
}

DataFrame::CaseSelector mask(const Thread& values) {
  DataFrame::CaseSelector out;
  out.kind = DataFrame::CaseSelector::Kind::Mask;
  out.mask = values;
  return out;
}

DataFrame::CaseSelector mask(const pluss::Series& values) {
  return mask(Thread(values));
}

DataFrame::FeatureSelector all_features() {
  return {};
}

DataFrame::FeatureSelector col(std::string_view name) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::NameScalar;
  out.names.emplace_back(name);
  return out;
}

DataFrame::FeatureSelector col(const char* name) {
  return col(std::string_view{name != nullptr ? name : ""});
}

DataFrame::FeatureSelector col(std::int64_t position) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::PositionScalar;
  out.first = position;
  return out;
}

DataFrame::FeatureSelector cols(std::initializer_list<std::string_view> names) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::NameList;
  out.names.reserve(names.size());
  for (const auto name : names) {
    out.names.emplace_back(name);
  }
  return out;
}

DataFrame::FeatureSelector cols(pluss::RangeView<std::string_view> names) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::NameList;
  out.names.reserve(names.size());
  for (const auto name : names) {
    out.names.emplace_back(name);
  }
  return out;
}

DataFrame::FeatureSelector cols(std::initializer_list<std::int64_t> positions) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::PositionList;
  out.positions.assign(positions.begin(), positions.end());
  return out;
}

DataFrame::FeatureSelector cols(pluss::RangeView<std::int64_t> positions) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::PositionList;
  out.positions.assign(positions.begin(), positions.end());
  return out;
}

DataFrame::FeatureSelector cols(std::int64_t start, std::int64_t stop) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::PositionSlice;
  out.first = start;
  out.second = stop;
  return out;
}

DataFrame::FeatureSelector col_range(std::string_view start, std::string_view stop) {
  DataFrame::FeatureSelector out;
  out.kind = DataFrame::FeatureSelector::Kind::NameSlice;
  out.names.emplace_back(start);
  out.names.emplace_back(stop);
  return out;
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
  auto labels = detail::with_label_kind(case_idx, LabelKind::Case);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, labels));
}

DataFrame& DataFrame::with_cases_(const Thread& case_idx) {
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

Thread DataFrame::features(std::string_view name) {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'");
  return detail::with_display_labels(src.columns_[it->second], src.cases_)
      .with_display_name(std::move(feature_name));
}

Thread DataFrame::features(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto feature_name = owned_name(name);
  const auto it = src.name_to_idx_.find(feature_name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + feature_name + "'");
  return detail::with_display_labels(src.columns_[it->second], src.cases_)
      .with_display_name(std::move(feature_name));
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

DataFrame DataFrame::features(std::initializer_list<const char*> names) {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return select(pluss::msharp::RangeView::Of<std::string_view>(views));
}

DataFrame DataFrame::features(std::initializer_list<const char*> names) const {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto* name : names) {
    views.emplace_back(name != nullptr ? name : "");
  }
  return select(pluss::msharp::RangeView::Of<std::string_view>(views));
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
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
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::astype(pluss::msharp::RangeView::Of<std::string_view> features,
                            pluss::codem::ScalarType dtype) const {
  std::vector<CastSpec> specs;
  specs.reserve(features.size());
  for (const auto name_sv : features) {
    specs.push_back(CastSpec{
        .feature = owned_name(name_sv),
        .target = ThreadCastTarget::Series,
        .options = ThreadCastOptions{.series_dtype = dtype},
    });
  }
  return cast(pluss::msharp::RangeView::Of<CastSpec>(
      std::span<const CastSpec>(specs.data(), specs.size())));
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::place(std::string name,
                           const Thread& feature_value,
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
  return select(pluss::msharp::RangeView::Of<std::string_view>(
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
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
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
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::bucketize: unknown feature '" + name + "'");
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
    out_names.push_back(fmt::format("{}{}", prefix, name));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::add_suffix(std::string_view suffix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(fmt::format("{}{}", name, suffix));
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

DataFrame DataFrame::sort_cases(const SortCasesOptions& options) const {
  const auto& src = impl_ref_();
  if (options.levels.empty()) {
    return sort_cases(options.ascending, options.nulls_last);
  }
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::sort_cases: current DataFrame has no case levels");
  std::vector<std::size_t> level_indices;
  level_indices.reserve(options.levels.size());
  for (const auto& name : options.levels) {
    level_indices.push_back(level_index_(src, name, "DataFrame::sort_cases"));
  }
  std::vector<std::int64_t> idx(static_cast<std::size_t>(ncases()));
  std::iota(idx.begin(), idx.end(), 0);
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    for (const auto level_idx : level_indices) {
      const auto& level_values = src.case_axis_.levels[level_idx].values;
      const auto cmp = level_values.compare_rows(a, b, options.nulls_last);
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
  SetCasesOptions options;
  options.features.emplace_back(feature_name);
  options.drop = drop_feature;
  return set_cases(options);
}

DataFrame DataFrame::set_cases(std::initializer_list<std::string_view> feature_names,
                               bool drop_features,
                               std::string_view separator) const {
  return set_cases(pluss::msharp::RangeView::Of<std::string_view>(feature_names),
                   drop_features,
                   separator);
}

DataFrame DataFrame::set_cases(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                               bool drop_features,
                               std::string_view separator) const {
  SetCasesOptions options;
  options.drop = drop_features;
  options.separator = std::string(separator);
  options.features.reserve(feature_names.size());
  for (const auto name : feature_names) {
    options.features.emplace_back(name);
  }
  return set_cases(options);
}

DataFrame DataFrame::set_cases(const SetCasesOptions& options) const {
  std::vector<std::string_view> feature_names;
  feature_names.reserve(options.features.size());
  for (const auto& name : options.features) {
    feature_names.push_back(name);
  }
  PLUSS_CHECK(feature_names.size() > 0, "DataFrame::set_cases: expected at least one feature");

  std::vector<std::string> names;
  names.reserve(feature_names.size());
  std::unordered_set<std::string> seen;
  for (const auto name_sv : feature_names) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::set_cases: feature name cannot be empty");
    PLUSS_CHECK(has_feature(name), "DataFrame::set_cases: unknown feature '" + name + "'");
    PLUSS_CHECK(seen.insert(name).second, "DataFrame::set_cases: duplicate feature '" + name + "'");
    names.push_back(name);
  }

  detail::CaseAxis axis;
  axis.levels.reserve(names.size());
  for (const auto& name : names) {
    axis.levels.push_back({name, features(name)});
  }

  if (options.verify) {
    std::unordered_set<std::string> keys;
    keys.reserve(static_cast<std::size_t>(ncases()));
    for (std::int64_t row = 0; row < ncases(); ++row) {
      const auto inserted = keys.insert(case_tuple_key_(axis, row)).second;
      PLUSS_CHECK(inserted, "DataFrame::set_cases: duplicate structured case key found; set verify=false to allow duplicates");
    }
  }

  Thread case_values;
  if (names.size() == 1) {
    case_values = detail::clear_display_labels(axis.levels.front().values).clear_display_name();
  } else {
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(ncases()));
    for (std::int64_t r = 0; r < ncases(); ++r) {
      std::string label;
      for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
          label.append(options.separator.data(), options.separator.size());
        }
        label += thread_scalar_string_at(features(names[i]), r);
      }
      labels.push_back(std::move(label));
    }
    case_values = make_note_thread(labels);
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
  return options.sort ? out.sort_cases(SortCasesOptions{.levels = names}) : out;
}

DataFrame DataFrame::reset_cases(std::string case_feature) const {
  ResetCasesOptions options;
  options.names.push_back(std::move(case_feature));
  return reset_cases(options);
}

DataFrame DataFrame::reset_cases(const ResetCasesOptions& options) const {
  const auto& src = impl_ref_();
  auto out = *this;
  if (!src.case_axis_.defined()) {
    if (!options.drop) {
      const auto name = !options.names.empty() ? options.names.front() : options.prefix;
      out = out.set_features(name.empty() ? "case" : name, cases());
    }
    return out.with_cases(default_cases(out.ncases()));
  }

  if (!options.drop) {
    PLUSS_CHECK(options.names.empty() || options.names.size() == src.case_axis_.levels.size(),
                "DataFrame::reset_cases: replacement names must match case level count");
    for (std::size_t i = 0; i < src.case_axis_.levels.size(); ++i) {
      std::string name;
      if (!options.names.empty()) {
        name = options.names[i];
      } else {
        name = src.case_axis_.levels[i].name.empty()
                   ? fmt::format("{}{}", options.prefix, i)
                   : src.case_axis_.levels[i].name;
      }
      PLUSS_CHECK(!out.has_feature(name), "DataFrame::reset_cases: feature already exists '" + name + "'");
      auto level_values = src.case_axis_.levels[i].values;
      if (detail::has_display_labels(level_values)) {
        level_values = detail::clear_display_labels(level_values);
      }
      if (level_values.has_display_name()) {
        level_values = level_values.clear_display_name();
      }
      set_prepared_feature_(out.mut_impl_(), name, clear_assignment_metadata_(level_values));
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      out.impl_ref_().names_, out.impl_ref_().columns_, default_cases(out.ncases()), out.title()));
}

DataFrame DataFrame::rename_levels(pluss::msharp::RangeView::Of<LevelRename> renames) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::rename_levels: current DataFrame has no case levels");
  detail::CaseAxis axis = src.case_axis_;
  std::unordered_set<std::string> targets;
  for (const auto& rename : renames) {
    PLUSS_CHECK(!rename.from.empty(), "DataFrame::rename_levels: source level cannot be empty");
    PLUSS_CHECK(!rename.to.empty(), "DataFrame::rename_levels: target level cannot be empty");
    const auto idx = level_index_(src, rename.from, "DataFrame::rename_levels");
    PLUSS_CHECK(targets.insert(rename.to).second,
                "DataFrame::rename_levels: duplicate target level '" + rename.to + "'");
    axis.levels[idx].name = rename.to;
  }
  std::unordered_set<std::string> names;
  for (const auto& level : axis.levels) {
    PLUSS_CHECK(names.insert(level.name).second,
                "DataFrame::rename_levels: duplicate case level name '" + level.name + "'");
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, src.cases_, src.title_, std::move(axis)));
}

DataFrame DataFrame::reorder_levels(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::reorder_levels: current DataFrame has no case levels");
  PLUSS_CHECK(names.size() == src.case_axis_.levels.size(),
              "DataFrame::reorder_levels: expected every case level exactly once");
  detail::CaseAxis axis;
  axis.levels.reserve(names.size());
  std::unordered_set<std::string> seen;
  for (const auto name_sv : names) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(seen.insert(name).second,
                "DataFrame::reorder_levels: duplicate case level '" + name + "'");
    axis.levels.push_back(src.case_axis_.levels[level_index_(src, name, "DataFrame::reorder_levels")]);
  }
  auto labels = make_note_thread(composite_case_labels_(axis, src.nrows_));
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, std::move(labels), src.title_, std::move(axis)));
}

DataFrame DataFrame::drop_levels(pluss::msharp::RangeView::Of<std::string_view> names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.case_axis_.defined(), "DataFrame::drop_levels: current DataFrame has no case levels");
  std::unordered_set<std::string> drop;
  drop.reserve(names.size());
  for (const auto name_sv : names) {
    const auto name = owned_name(name_sv);
    level_index_(src, name, "DataFrame::drop_levels");
    drop.insert(name);
  }
  detail::CaseAxis axis;
  for (const auto& level : src.case_axis_.levels) {
    if (!drop.contains(level.name)) {
      axis.levels.push_back(level);
    }
  }
  Thread out_cases = axis.defined() ? make_note_thread(composite_case_labels_(axis, src.nrows_))
                                    : default_cases(src.nrows_);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, std::move(out_cases), src.title_, std::move(axis)));
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
  const auto host = values.to_vec<T>();
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

std::vector<std::string> unify_key_parts(const Thread& values) {
  const auto count = static_cast<std::size_t>(values.size());
  auto valid = detail::has_validity_mask(values)
      ? detail::validity_mask(values)->to_vec<bool>()
      : std::vector<bool>(count, true);

  switch (values.kind()) {
    case ThreadKind::Series: {
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
    case ThreadKind::Note: {
      const auto notes = values.as_note().vector();
      std::vector<std::string> out(count);
      for (std::size_t i = 0; i < count; ++i) {
        out[i] = valid[i] ? std::string("\2") + std::string(notes[i])
                          : std::string(1, '\0');
      }
      return out;
    }
    case ThreadKind::Timeline:
    case ThreadKind::TimeIndex: {
      const auto ticks =
          values.kind() == ThreadKind::Timeline
              ? values.as_timeline().epoch_nanos().to_vec<std::int64_t>()
              : values.as_timeindex().timeline().epoch_nanos().to_vec<std::int64_t>();
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
  }
  PLUSS_FAIL("DataFrame::unify: unsupported key payload kind");
}

void ensure_unify_key_compatibility(
    const Thread& left,
    const Thread& right) {
  PLUSS_CHECK(
      left.kind() == right.kind(),
      "DataFrame::unify: paired key features must use the same Thread kind");
  if (left.kind() == ThreadKind::Series) {
    PLUSS_CHECK(
        left.as_series().scalar_type() == right.as_series().scalar_type(),
        "DataFrame::unify: paired numeric key features must use the same dtype");
    PLUSS_CHECK(
        left.as_series().platform() == right.as_series().platform(),
        "DataFrame::unify: paired numeric key features must use the same platform");
  } else if (left.kind() == ThreadKind::Timeline) {
    PLUSS_CHECK(
        left.as_timeline().epoch_nanos().platform() ==
            right.as_timeline().epoch_nanos().platform(),
        "DataFrame::unify: paired timeline key features must use the same platform");
  } else if (left.kind() == ThreadKind::TimeIndex) {
    PLUSS_CHECK(
        left.as_timeindex().timeline().epoch_nanos().platform() ==
            right.as_timeindex().timeline().epoch_nanos().platform(),
        "DataFrame::unify: paired time-index key features must use the same platform");
  }
}

std::vector<std::string> make_unify_keys(
    const std::vector<Thread>& key_cols,
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
  throw ::pluss::Error(fmt::format(
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

Thread make_unify_null_thread(const Thread& reference, std::int64_t size) {
  switch (reference.kind()) {
    case ThreadKind::Series: {
      const auto platform = reference.as_series().platform();
      auto invalid = pluss::Series::zeros(
          {size},
          pluss::codem::ScalarType::Bool,
          platform);
      return Thread(
          pluss::Series::zeros(
              {size},
              reference.as_series().scalar_type(),
              platform),
          invalid);
    }
    case ThreadKind::Note:
      return Thread(
          pluss::make_note(
              std::vector<std::string>(static_cast<std::size_t>(size))),
          pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Bool,
              pluss::codem::Platform::CPU));
    case ThreadKind::Timeline: {
      const auto platform = reference.as_timeline().epoch_nanos().platform();
      return Thread(
          pluss::Timeline::fromSeries(pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Int64,
              platform)),
          pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Bool,
              platform));
    }
    case ThreadKind::TimeIndex: {
      const auto platform =
          reference.as_timeindex().timeline().epoch_nanos().platform();
      return Thread(
          pluss::Timeline::fromSeries(pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Int64,
              platform)).to_timeindex(false),
          pluss::Series::zeros(
              {size},
              pluss::codem::ScalarType::Bool,
              platform));
    }
  }
  PLUSS_FAIL("DataFrame::unify: unsupported Thread kind");
}

Thread take_rows_for_unify(const Thread& source, const std::vector<std::int64_t>& rows) {
  if (rows.empty()) {
    return source.take(std::span<const std::int64_t>(rows.data(), rows.size()));
  }
  if (source.size() == 0) {
    return make_unify_null_thread(source, static_cast<std::int64_t>(rows.size()));
  }

  std::vector<std::int64_t> safe_rows;
  safe_rows.reserve(rows.size());
  std::vector<bool> out_valid(rows.size(), true);
  std::optional<std::vector<bool>> src_valid = std::nullopt;
  if (detail::has_validity_mask(source)) {
    src_valid = detail::validity_mask(source)->to_vec<bool>();
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
  if (out.kind() == ThreadKind::Series) {
    platform = out.as_series().platform();
  } else if (out.kind() == ThreadKind::Timeline) {
    platform = out.as_timeline().epoch_nanos().platform();
  } else if (out.kind() == ThreadKind::TimeIndex) {
    platform = out.as_timeindex().timeline().epoch_nanos().platform();
  }
  return detail::with_validity_mask(out, bool_mask_from_values(out_valid, platform));
}

Thread take_unified_key_rows(
    const Thread& left,
    const Thread& right,
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
  if (output.kind() == ThreadKind::Series) {
    platform = output.as_series().platform();
  } else if (output.kind() == ThreadKind::Timeline) {
    platform = output.as_timeline().epoch_nanos().platform();
  } else if (output.kind() == ThreadKind::TimeIndex) {
    platform = output.as_timeindex().timeline().epoch_nanos().platform();
  }
  return output.put(
      Thread(bool_mask_from_values(use_right, platform)),
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
    levels = levels.sort_values(true, true);
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
      ? source.validity_mask()->to_vec<bool>()
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
  if (source.kind() == ThreadKind::Series) {
    platform = source.as_series().platform();
  } else if (source.kind() == ThreadKind::Timeline) {
    platform = source.as_timeline().epoch_nanos().platform();
  } else if (source.kind() == ThreadKind::TimeIndex) {
    platform = source.as_timeindex().timeline().epoch_nanos().platform();
  }
  if (platform != pluss::codem::Platform::CPU) {
    code_values = code_values.to(platform);
  }
  Thread code_thread(code_values);
  if (source.has_validity_mask()) {
    auto mask = *source.validity_mask();
    if (mask.platform() != platform) {
      mask = mask.to(platform);
    }
    code_thread = code_thread.with_validity_mask(mask);
  }
  return CategoryEncoding{
      std::move(code_thread),
      std::move(levels),
      ordered};
}

Thread CategoryEncoding::decode() const {
  PLUSS_CHECK(codes.defined(), "CategoryEncoding::decode: codes are undefined");
  PLUSS_CHECK(levels.defined(), "CategoryEncoding::decode: levels are undefined");
  PLUSS_CHECK(
      codes.kind() == ThreadKind::Series &&
          codes.as_series().scalar_type() == pluss::codem::ScalarType::Int64,
      "CategoryEncoding::decode: codes must be an Int64 Series Thread");

  const auto raw_codes = codes.as_series().to_vec<std::int64_t>();
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

  Thread output;
  if (levels.size() > 0 || safe.empty()) {
    output = levels.take(safe);
  } else {
    const auto size = static_cast<std::int64_t>(safe.size());
    switch (levels.kind()) {
      case ThreadKind::Series:
        output = Thread(pluss::Series::zeros(
            {size},
            levels.as_series().scalar_type(),
            levels.as_series().platform()));
        break;
      case ThreadKind::Note:
        output = Thread(pluss::make_note(
            std::vector<std::string>(safe.size())));
        break;
      case ThreadKind::Timeline:
        output = Thread(pluss::Timeline::fromSeries(pluss::Series::zeros(
            {size},
            pluss::codem::ScalarType::Int64,
            levels.as_timeline().epoch_nanos().platform())));
        break;
      case ThreadKind::TimeIndex:
        output = Thread(
            pluss::Timeline::fromSeries(pluss::Series::zeros(
                {size},
                pluss::codem::ScalarType::Int64,
                levels.as_timeindex().timeline().epoch_nanos().platform()))
                .to_timeindex(false));
        break;
    }
  }
  if (detail::has_validity_mask(codes)) {
    auto mask = *detail::validity_mask(codes);
    const auto platform =
        output.kind() == ThreadKind::Series
            ? output.as_series().platform()
            : (output.kind() == ThreadKind::Timeline
                   ? output.as_timeline().epoch_nanos().platform()
                   : (output.kind() == ThreadKind::TimeIndex
                          ? output.as_timeindex().timeline().epoch_nanos().platform()
                          : pluss::codem::Platform::CPU));
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

DataFrame DataFrame::set_features(std::string_view name, const Thread& value) const {
  return set_features(name, value, AlignOptions{});
}

DataFrame DataFrame::set_features(std::string_view name, const Thread& value, const AlignOptions& options) const {
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

DataFrame& DataFrame::set_features_(std::string_view name, const Thread& value) {
  return set_features_(name, value, AlignOptions{});
}

DataFrame& DataFrame::set_features_(std::string_view name, const Thread& value, const AlignOptions& options) {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_, src.title_, src.case_axis_));
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
  std::vector<Thread> out_cols;
  out_names.reserve(ordered.size());
  out_cols.reserve(ordered.size());
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::conform: duplicate feature in target '" + n + "'");
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::conform: missing feature '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_, src.case_axis_));
  return out.align_cases(other.caselbl());
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

  std::vector<Thread> left_key_cols;
  std::vector<Thread> right_key_cols;
  left_key_cols.reserve(left_keys.size());
  right_key_cols.reserve(right_keys.size());
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    PLUSS_CHECK(has_feature(left_keys[i]),
                "DataFrame::unify: left key feature not found '" + left_keys[i] + "'");
    PLUSS_CHECK(right.has_feature(right_keys[i]),
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

  std::vector<std::pair<std::string, Thread>> out_cols;
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
        out_name = fmt::format("{}{}_{}", base, right_suffix, k++);
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
