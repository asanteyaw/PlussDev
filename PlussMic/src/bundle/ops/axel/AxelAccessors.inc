AxelFieldAccessor::AxelFieldAccessor(Axel base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {
  PLUSS_CHECK(base_.defined(), "Axel::fields: undefined handle");
  PLUSS_CHECK(!keys_.empty(), "Axel::fields: at least one field is required");
  for (std::int64_t i = 0; i < base_.depth(); ++i) {
    for (const auto& key : keys_) {
      PLUSS_CHECK(base_.registry(i).has(key),
                  "Axel::fields: unknown field '" + key + "' in registry " + std::to_string(i));
    }
  }
}

template <class Fn>
Axel AxelFieldAccessor::map_selected_(Fn&& fn) const {
  return base_.map([&](const Registry& reg) { return fn(reg); });
}

AxelCastAccessor AxelFieldAccessor::cast() const {
  return AxelCastAccessor(base_, keys_, {});
}

AxelCastAccessor AxelFieldAccessor::cast(const pluss::table::ThreadCastOptions& options) const {
  return AxelCastAccessor(base_, keys_, options);
}

AxelNoteAccessor AxelFieldAccessor::nt() const {
  return AxelNoteAccessor(base_, keys_);
}

AxelTimelineAccessor AxelFieldAccessor::tl() const {
  return AxelTimelineAccessor(base_, keys_);
}

Axel AxelFieldAccessor::fillnull(double value) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).fillnull(value); });
}

Axel AxelFieldAccessor::fillnull(std::string_view value) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).fillnull(value); });
}

Axel AxelFieldAccessor::fillnull(const pluss::Timeline& value) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).fillnull(value); });
}

Axel AxelFieldAccessor::fillnull(const pluss::TimeIndex& value) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).fillnull(value); });
}

Axel AxelFieldAccessor::dropnull() const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).dropnull(); });
}

Axel AxelFieldAccessor::isnull() const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).isnull(); });
}

Axel AxelFieldAccessor::notnull() const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).notnull(); });
}

Axel AxelFieldAccessor::invalid() const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).invalid(); });
}

Axel AxelFieldAccessor::distinct(pluss::kindle::DistinctOptions options) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).distinct(options); });
}

Axel AxelFieldAccessor::freq(pluss::kindle::FreqOptions options) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).freq(options); });
}

Axel AxelFieldAccessor::ndistinct(pluss::kindle::DistinctOptions options) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).ndistinct(options); });
}

Axel AxelFieldAccessor::astype(pluss::codem::ScalarType dtype) const {
  return map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).astype(dtype); });
}

AxelFieldAxisAccessor::AxelFieldAxisAccessor(Axel base) : base_(std::move(base)) {}

Axel AxelFieldAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  return base_.map([&](const Registry& reg) { return reg.fieldaxis().like(needle, case_sensitive); });
}

Axel AxelFieldAxisAccessor::match(std::string_view pattern) const {
  return base_.map([&](const Registry& reg) { return reg.fieldaxis().match(pattern); });
}

Axel AxelFieldAxisAccessor::prefix(std::string_view token) const {
  return base_.map([&](const Registry& reg) { return reg.fieldaxis().prefix(token); });
}

Axel AxelFieldAxisAccessor::suffix(std::string_view token) const {
  return base_.map([&](const Registry& reg) { return reg.fieldaxis().suffix(token); });
}

Axel AxelFieldAxisAccessor::kind(SlotKind kind_value) const {
  return base_.map([&](const Registry& reg) { return reg.fieldaxis().kind(kind_value); });
}

Axel AxelFieldAxisAccessor::series() const { return kind(SlotKind::Series); }
Axel AxelFieldAxisAccessor::note() const { return kind(SlotKind::Note); }
Axel AxelFieldAxisAccessor::timeline() const { return kind(SlotKind::Timeline); }
Axel AxelFieldAxisAccessor::timeindex() const { return kind(SlotKind::TimeIndex); }
Axel AxelFieldAxisAccessor::mixed() const { return kind(SlotKind::Mixed); }
Axel AxelFieldAxisAccessor::nulls() const { return kind(SlotKind::Null); }
Axel AxelFieldAxisAccessor::empty() const { return kind(SlotKind::Empty); }

AxelCastAccessor::AxelCastAccessor(Axel base,
                                   std::vector<std::string> keys,
                                   pluss::table::ThreadCastOptions options)
    : base_(std::move(base)), keys_(std::move(keys)), options_(std::move(options)) {}

Axel AxelCastAccessor::series() const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).cast(options_).series(); });
}

Axel AxelCastAccessor::note() const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).cast(options_).note(); });
}

Axel AxelCastAccessor::timeline() const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).cast(options_).timeline(); });
}

Axel AxelCastAccessor::timeindex() const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).cast(options_).timeindex(); });
}

AxelNoteAccessor::AxelNoteAccessor(Axel base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_AXEL_NOTE_UNARY(method) \
  Axel AxelNoteAccessor::method() const { \
    return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().method(); }); \
  }

PLUSS_AXEL_NOTE_UNARY(strip)
PLUSS_AXEL_NOTE_UNARY(lstrip)
PLUSS_AXEL_NOTE_UNARY(rstrip)
PLUSS_AXEL_NOTE_UNARY(lower)
PLUSS_AXEL_NOTE_UNARY(upper)
PLUSS_AXEL_NOTE_UNARY(casefold)
PLUSS_AXEL_NOTE_UNARY(titlecase)
PLUSS_AXEL_NOTE_UNARY(length)

#undef PLUSS_AXEL_NOTE_UNARY

Axel AxelNoteAccessor::strip(std::string_view chars) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().strip(chars); });
}

Axel AxelNoteAccessor::lstrip(std::string_view chars) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().lstrip(chars); });
}

Axel AxelNoteAccessor::rstrip(std::string_view chars) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().rstrip(chars); });
}

Axel AxelNoteAccessor::starts(std::string_view prefix) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().starts(prefix); });
}

Axel AxelNoteAccessor::ends(std::string_view suffix) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().ends(suffix); });
}

Axel AxelNoteAccessor::has(std::string_view needle,
                           const pluss::table::TextOptions& options) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().has(needle, options); });
}

Axel AxelNoteAccessor::count(std::string_view pattern) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().count(pattern); });
}

Axel AxelNoteAccessor::count(std::string_view pattern,
                             const pluss::table::TextOptions& options) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().count(pattern, options); });
}

Axel AxelNoteAccessor::matches(std::string_view pattern, bool full) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().matches(pattern, full); });
}

Axel AxelNoteAccessor::fullmatch(std::string_view pattern) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().fullmatch(pattern); });
}

Axel AxelNoteAccessor::pad(std::int64_t width,
                           pluss::table::TextAlign align,
                           char fill) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().pad(width, align, fill); });
}

Axel AxelNoteAccessor::chars(std::int64_t start,
                             std::optional<std::int64_t> stop,
                             std::int64_t step) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().chars(start, stop, step); });
}

Axel AxelNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().normalize(form); });
}

Axel AxelNoteAccessor::encode(std::string_view encoding) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().encode(encoding); });
}

Axel AxelNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().replace(from, to); });
}

Axel AxelNoteAccessor::replace(std::string_view from,
                               std::string_view to,
                               const pluss::table::TextOptions& options) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).nt().replace(from, to, options); });
}

AxelTimelineAccessor::AxelTimelineAccessor(Axel base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_AXEL_TL_UNARY(method) \
  Axel AxelTimelineAccessor::method() const { \
    return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().method(); }); \
  }

PLUSS_AXEL_TL_UNARY(isnat)
PLUSS_AXEL_TL_UNARY(valid)
PLUSS_AXEL_TL_UNARY(year)
PLUSS_AXEL_TL_UNARY(month)
PLUSS_AXEL_TL_UNARY(day)
PLUSS_AXEL_TL_UNARY(hour)
PLUSS_AXEL_TL_UNARY(minute)
PLUSS_AXEL_TL_UNARY(second)
PLUSS_AXEL_TL_UNARY(weekday)
PLUSS_AXEL_TL_UNARY(yearday)
PLUSS_AXEL_TL_UNARY(quarter)
PLUSS_AXEL_TL_UNARY(week)
PLUSS_AXEL_TL_UNARY(weekend)
PLUSS_AXEL_TL_UNARY(month_start)
PLUSS_AXEL_TL_UNARY(month_end)
PLUSS_AXEL_TL_UNARY(leap)
PLUSS_AXEL_TL_UNARY(weekday_name)
PLUSS_AXEL_TL_UNARY(month_name)
PLUSS_AXEL_TL_UNARY(iso)
PLUSS_AXEL_TL_UNARY(date)
PLUSS_AXEL_TL_UNARY(time)
PLUSS_AXEL_TL_UNARY(utc)

#undef PLUSS_AXEL_TL_UNARY

Axel AxelTimelineAccessor::fmt(std::string_view pattern) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().fmt(pattern); });
}

Axel AxelTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().epoch(unit); });
}

Axel AxelTimelineAccessor::floor(std::string_view unit) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().floor(unit); });
}

Axel AxelTimelineAccessor::ceil(std::string_view unit) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().ceil(unit); });
}

Axel AxelTimelineAccessor::round(std::string_view unit) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().round(unit); });
}

Axel AxelTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().shift(periods, freq); });
}

Axel AxelTimelineAccessor::period(std::string_view freq) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().period(freq); });
}

Axel AxelTimelineAccessor::tz(std::string_view name) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().tz(name); });
}

Axel AxelTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().localize(options); });
}

Axel AxelTimelineAccessor::index(bool require_sorted) const {
  return AxelFieldAccessor(base_, keys_).map_selected_([&](const Registry& reg) { return RegistryFieldAccessor(reg, keys_).tl().index(require_sorted); });
}
