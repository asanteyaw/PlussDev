Catalog Catalog::keys(std::string_view key) const {
  return project_catalog(*this, normalize_names(std::vector<std::string>{std::string(key)}), "Catalog::keys");
}

Catalog Catalog::keys(std::span<const std::string_view> field_names) const {
  return project_catalog(*this, normalize_names(field_names), "Catalog::keys");
}

Catalog Catalog::keys(std::initializer_list<std::string_view> field_names) const {
  return keys(std::span<const std::string_view>(field_names.begin(), field_names.size()));
}

CatalogKeyAxisAccessor Catalog::keyaxis() const {
  return CatalogKeyAxisAccessor(*this);
}

CatalogGroupBuilder Catalog::group() const {
  return CatalogGroupBuilder(*this);
}

CatalogCastAccessor Catalog::cast() const {
  return CatalogCastAccessor(*this, keys(), {});
}

CatalogCastAccessor Catalog::cast(const pluss::table::ThreadCastOptions& options) const {
  return CatalogCastAccessor(*this, keys(), options);
}

CatalogNoteAccessor Catalog::nt() const {
  return CatalogNoteAccessor(*this, keys());
}

CatalogTimelineAccessor Catalog::tl() const {
  return CatalogTimelineAccessor(*this, keys());
}

CatalogTimeSeriesAccessor Catalog::ts() const {
  return CatalogTimeSeriesAccessor(*this, keys());
}

CatalogKeyAccessor::CatalogKeyAccessor(Catalog base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {
  PLUSS_CHECK(base_.defined(), "Catalog::keys: undefined handle");
  if (base_.empty()) return;
  PLUSS_CHECK(!keys_.empty(), "Catalog::keys: at least one key is required");
  for (const auto& key : keys_) {
    PLUSS_CHECK(base_.has(key), "Catalog::keys: unknown key '" + key + "'");
  }
}

CatalogCastAccessor CatalogKeyAccessor::cast() const {
  return CatalogCastAccessor(base_, keys_, {});
}

CatalogCastAccessor CatalogKeyAccessor::cast(const pluss::table::ThreadCastOptions& options) const {
  return CatalogCastAccessor(base_, keys_, options);
}

CatalogNoteAccessor CatalogKeyAccessor::nt() const {
  return CatalogNoteAccessor(base_, keys_);
}

CatalogTimelineAccessor CatalogKeyAccessor::tl() const {
  return CatalogTimelineAccessor(base_, keys_);
}

CatalogTimeSeriesAccessor CatalogKeyAccessor::ts() const {
  return CatalogTimeSeriesAccessor(base_, keys_);
}

Catalog CatalogKeyAccessor::fillnull(double value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Series ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(std::string_view value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Note ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(const pluss::Timeline& value) const {
  return map_selected_([&](const Stream& stream) {
    return (stream.kind() == StreamKind::Timeline || stream.kind() == StreamKind::TimeIndex)
               ? stream.fillnull(value)
               : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(const pluss::TimeIndex& value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::TimeIndex ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::dropnull() const {
  std::optional<pluss::table::Thread> keep;
  for (const auto& key : keys_) {
    const auto valid = base_.at(key).notnull().to_thread();
    keep = keep.has_value() ? keep->land(valid) : valid;
  }
  if (!keep.has_value()) return base_;

  const auto mask = keep->as_series();
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    out.emplace_back(key, base_.at(key).filter(mask));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAccessor::ffill() const {
  return map_selected_([](const Stream& stream) { return stream.ffill(); });
}

Catalog CatalogKeyAccessor::bfill() const {
  return map_selected_([](const Stream& stream) { return stream.bfill(); });
}

Catalog CatalogKeyAccessor::isnull() const {
  return map_selected_([](const Stream& stream) { return stream.isnull(); });
}

Catalog CatalogKeyAccessor::notnull() const {
  return map_selected_([](const Stream& stream) { return stream.notnull(); });
}

Catalog CatalogKeyAccessor::invalid() const {
  return map_selected_([](const Stream& stream) { return stream.invalid(); });
}

Catalog CatalogKeyAccessor::distinct(DistinctOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.distinct(options); });
}

Catalog CatalogKeyAccessor::freq(FreqOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.freq(options); });
}

Catalog CatalogKeyAccessor::ndistinct(DistinctOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.ndistinct(options); });
}

Catalog CatalogKeyAccessor::astype(pluss::codem::ScalarType dtype) const {
  return map_selected_([&](const Stream& stream) { return stream.astype(dtype); });
}

Catalog CatalogKeyAccessor::map(const std::function<Stream(const Stream&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Catalog::keys().map: function is empty");
  return map_selected_([&](const Stream& stream) { return stream.map(fn); });
}

CatalogKeyAxisAccessor::CatalogKeyAxisAccessor(Catalog base) : base_(std::move(base)) {}

Catalog CatalogKeyAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  const auto token = case_sensitive ? std::string(needle) : lowercase_copy(needle);
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto hay = case_sensitive ? key : lowercase_copy(key);
    if (hay.find(token) != std::string::npos) selected.push_back(key);
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().like");
}

Catalog CatalogKeyAxisAccessor::match(std::string_view pattern) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  try {
    const std::regex rx{std::string(pattern)};
    std::vector<std::string> selected;
    selected.reserve(static_cast<std::size_t>(base_.nkeys()));
    for (const auto& key : base_.keys()) {
      if (std::regex_search(key, rx)) selected.push_back(key);
    }
    return project_catalog(base_, selected, "Catalog::keyaxis().match");
  } catch (const std::regex_error& e) {
    PLUSS_FAIL("Catalog::keyaxis().match: invalid regex '" + std::string(pattern) + "': " + std::string(e.what()));
  }
}

Catalog CatalogKeyAxisAccessor::series() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.at(key);
    if (stream.kind() == StreamKind::Series) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::note() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.at(key);
    if (stream.kind() == StreamKind::Note) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::timeline() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.at(key);
    if (stream.kind() == StreamKind::Timeline || stream.kind() == StreamKind::TimeIndex) {
      out.emplace_back(key, std::move(stream));
    }
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::timeindex() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.at(key);
    if (stream.kind() == StreamKind::TimeIndex) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::prefix(std::string_view prefix) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  const auto token = std::string(prefix);
  for (const auto& key : base_.keys()) {
    if (key.rfind(token, 0) == 0) selected.push_back(key);
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().prefix");
}

Catalog CatalogKeyAxisAccessor::suffix(std::string_view suffix) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  const auto token = std::string(suffix);
  for (const auto& key : base_.keys()) {
    if (key.size() >= token.size() &&
        key.compare(key.size() - token.size(), token.size(), token) == 0) {
      selected.push_back(key);
    }
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().suffix");
}

CatalogCastAccessor::CatalogCastAccessor(Catalog base,
                                         std::vector<std::string> keys,
                                         pluss::table::ThreadCastOptions options)
    : keysel_(std::move(base), std::move(keys)),
      options_(std::move(options)) {}

Catalog CatalogCastAccessor::series() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).series(); });
}

Catalog CatalogCastAccessor::note() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).note(); });
}

Catalog CatalogCastAccessor::timeline() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).timeline(); });
}

Catalog CatalogCastAccessor::timeindex() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).timeindex(); });
}

CatalogNoteAccessor::CatalogNoteAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_NOTE_UNARY(method) \
  Catalog CatalogNoteAccessor::method() const { \
    return keysel_.map_selected_([](const Stream& stream) { return stream.nt().method(); }); \
  }

PLUSS_CATALOG_NOTE_UNARY(strip)
PLUSS_CATALOG_NOTE_UNARY(lstrip)
PLUSS_CATALOG_NOTE_UNARY(rstrip)
PLUSS_CATALOG_NOTE_UNARY(lower)
PLUSS_CATALOG_NOTE_UNARY(upper)
PLUSS_CATALOG_NOTE_UNARY(casefold)
PLUSS_CATALOG_NOTE_UNARY(titlecase)
PLUSS_CATALOG_NOTE_UNARY(length)

#undef PLUSS_CATALOG_NOTE_UNARY

Catalog CatalogNoteAccessor::strip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().strip(chars); });
}

Catalog CatalogNoteAccessor::lstrip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().lstrip(chars); });
}

Catalog CatalogNoteAccessor::rstrip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().rstrip(chars); });
}

Catalog CatalogNoteAccessor::starts(std::string_view prefix) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().starts(prefix); });
}

Catalog CatalogNoteAccessor::ends(std::string_view suffix) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().ends(suffix); });
}

Catalog CatalogNoteAccessor::has(std::string_view needle,
                                 const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().has(needle, options); });
}

Catalog CatalogNoteAccessor::count(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().count(pattern); });
}

Catalog CatalogNoteAccessor::count(std::string_view pattern,
                                   const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().count(pattern, options); });
}

Catalog CatalogNoteAccessor::matches(std::string_view pattern, bool full) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().matches(pattern, full); });
}

Catalog CatalogNoteAccessor::fullmatch(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().fullmatch(pattern); });
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(std::string_view pattern) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().findall(pattern));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(
    std::string_view pattern,
    const pluss::table::TextOptions& options) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().findall(pattern, options));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().split(delimiter, limit, prefix));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::split(
    std::string_view pattern,
    const pluss::table::TextOptions& options) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().split(pattern, options));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::partition(
    std::string_view separator,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().partition(separator, prefix));
  return out;
}

Catalog CatalogNoteAccessor::pad(std::int64_t width,
                                 pluss::table::TextAlign align,
                                 char fill) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().pad(width, align, fill); });
}

Catalog CatalogNoteAccessor::chars(std::int64_t start,
                                   std::optional<std::int64_t> stop,
                                   std::int64_t step) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().chars(start, stop, step); });
}

Catalog CatalogNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().normalize(form); });
}

Catalog CatalogNoteAccessor::encode(std::string_view encoding) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().encode(encoding); });
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::indicate(
    std::string_view delimiter,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.at(key).nt().indicate(delimiter, prefix));
  return out;
}

Catalog CatalogNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().replace(from, to); });
}

Catalog CatalogNoteAccessor::replace(std::string_view from,
                                     std::string_view to,
                                     const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().replace(from, to, options); });
}

CatalogTimelineAccessor::CatalogTimelineAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_TL_UNARY(method) \
  Catalog CatalogTimelineAccessor::method() const { \
    return keysel_.map_selected_([](const Stream& stream) { return stream.tl().method(); }); \
  }

PLUSS_CATALOG_TL_UNARY(isnat)
PLUSS_CATALOG_TL_UNARY(valid)
PLUSS_CATALOG_TL_UNARY(year)
PLUSS_CATALOG_TL_UNARY(month)
PLUSS_CATALOG_TL_UNARY(day)
PLUSS_CATALOG_TL_UNARY(hour)
PLUSS_CATALOG_TL_UNARY(minute)
PLUSS_CATALOG_TL_UNARY(second)
PLUSS_CATALOG_TL_UNARY(weekday)
PLUSS_CATALOG_TL_UNARY(yearday)
PLUSS_CATALOG_TL_UNARY(quarter)
PLUSS_CATALOG_TL_UNARY(week)
PLUSS_CATALOG_TL_UNARY(weekend)
PLUSS_CATALOG_TL_UNARY(month_start)
PLUSS_CATALOG_TL_UNARY(month_end)
PLUSS_CATALOG_TL_UNARY(leap)
PLUSS_CATALOG_TL_UNARY(weekday_name)
PLUSS_CATALOG_TL_UNARY(month_name)
PLUSS_CATALOG_TL_UNARY(iso)
PLUSS_CATALOG_TL_UNARY(date)
PLUSS_CATALOG_TL_UNARY(time)
PLUSS_CATALOG_TL_UNARY(utc)

#undef PLUSS_CATALOG_TL_UNARY

Catalog CatalogTimelineAccessor::fmt(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().fmt(pattern); });
}

Catalog CatalogTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().epoch(unit); });
}

Catalog CatalogTimelineAccessor::floor(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().floor(unit); });
}

Catalog CatalogTimelineAccessor::ceil(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().ceil(unit); });
}

Catalog CatalogTimelineAccessor::round(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().round(unit); });
}

Catalog CatalogTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().shift(periods, freq); });
}

Catalog CatalogTimelineAccessor::period(std::string_view freq) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().period(freq); });
}

Catalog CatalogTimelineAccessor::tz(std::string_view name) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().tz(name); });
}

Catalog CatalogTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().localize(options); });
}

Catalog CatalogTimelineAccessor::index(bool require_sorted) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().index(require_sorted); });
}

CatalogTimeSeriesAccessor::CatalogTimeSeriesAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_TS_PERIODS(method) \
  Catalog CatalogTimeSeriesAccessor::method(std::int64_t periods) const { \
    return keysel_.map_selected_([&](const Stream& stream) { return stream.ts().method(periods); }); \
  }

PLUSS_CATALOG_TS_PERIODS(returns)
PLUSS_CATALOG_TS_PERIODS(log_returns)
PLUSS_CATALOG_TS_PERIODS(diff)
PLUSS_CATALOG_TS_PERIODS(lag)
PLUSS_CATALOG_TS_PERIODS(lead)

#undef PLUSS_CATALOG_TS_PERIODS

Catalog CatalogTimeSeriesAccessor::rolling(std::int64_t window,
                                           pluss::table::WindowStatistic statistic,
                                           const pluss::table::WindowOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.ts().rolling(window, statistic, options); });
}

CatalogGroupBuilder::CatalogGroupBuilder(Catalog base) : base_(std::move(base)) {}

std::unordered_map<std::string, Catalog> CatalogGroupBuilder::by(std::string_view key_field) const {
  PLUSS_CHECK(base_.defined(), "Catalog::group: undefined handle");
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::group().by: key field cannot be empty");
  PLUSS_CHECK(base_.has(key), "Catalog::group().by: unknown key '" + key + "'");

  const auto key_stream = base_.at(key);
  const auto key_note = key_stream.to_thread().cast().note().as_note();
  const auto n = key_stream.size();

  std::unordered_map<std::string, std::vector<std::int64_t>> row_groups;
  row_groups.reserve(static_cast<std::size_t>(std::max<std::int64_t>(1, n / 8)));
  for (std::int64_t i = 0; i < n; ++i) {
    row_groups[std::string(key_note.at(i))].push_back(i);
  }

  std::unordered_map<std::string, Catalog> out;
  out.reserve(row_groups.size());
  for (const auto& [group_key, rows] : row_groups) {
    std::vector<std::pair<std::string, Stream>> grouped;
    grouped.reserve(static_cast<std::size_t>(base_.nkeys()));
    for (const auto& field : base_.keys()) {
      const auto stream = base_.at(field);
      PLUSS_CHECK(stream.size() == n, "Catalog::group().by: key field requires aligned stream lengths");
      grouped.emplace_back(field, stream.take(rows));
    }
    out.emplace(group_key, Catalog(std::move(grouped)));
  }
  return out;
}
