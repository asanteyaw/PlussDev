namespace {

template <class Fn>
Axel axel_map_selected(const Axel& base, const std::vector<std::string>& keys, Fn&& fn) {
  PLUSS_CHECK(base.defined(), "Axel: undefined handle");
  PLUSS_CHECK(!keys.empty(), "Axel: at least one field is required");
  return base.map([&](const Registry& reg) { return fn(reg.fields(keys)); });
}

} // namespace

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
Axel AxelFieldAxisAccessor::duration() const { return kind(SlotKind::Duration); }
Axel AxelFieldAxisAccessor::mixed() const { return kind(SlotKind::Mixed); }
Axel AxelFieldAxisAccessor::nulls() const { return kind(SlotKind::Null); }
Axel AxelFieldAxisAccessor::empty() const { return kind(SlotKind::Empty); }

AxelCastAccessor::AxelCastAccessor(Axel base,
                                   std::vector<std::string> keys,
                                   pluss::table::ThreadCastOptions options)
    : base_(std::move(base)), keys_(std::move(keys)), options_(std::move(options)) {}

Axel AxelCastAccessor::series() const {
  return base_.map([&](const Registry& reg) {
    return keys_.empty() ? reg.cast(options_).series() : reg.fields(keys_).cast(options_).series();
  });
}

Axel AxelCastAccessor::note() const {
  return base_.map([&](const Registry& reg) {
    return keys_.empty() ? reg.cast(options_).note() : reg.fields(keys_).cast(options_).note();
  });
}

Axel AxelCastAccessor::timeline() const {
  return base_.map([&](const Registry& reg) {
    return keys_.empty() ? reg.cast(options_).timeline() : reg.fields(keys_).cast(options_).timeline();
  });
}

Axel AxelCastAccessor::duration() const {
  return base_.map([&](const Registry& reg) {
    return keys_.empty() ? reg.cast(options_).duration() : reg.fields(keys_).cast(options_).duration();
  });
}

AxelNoteAccessor::AxelNoteAccessor(Axel base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_AXEL_NOTE_UNARY(method) \
  Axel AxelNoteAccessor::method() const { \
    return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().method() : reg.fields(keys_).nt().method(); }); \
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
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().strip(chars) : reg.fields(keys_).nt().strip(chars); });
}

Axel AxelNoteAccessor::lstrip(std::string_view chars) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().lstrip(chars) : reg.fields(keys_).nt().lstrip(chars); });
}

Axel AxelNoteAccessor::rstrip(std::string_view chars) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().rstrip(chars) : reg.fields(keys_).nt().rstrip(chars); });
}

Axel AxelNoteAccessor::starts(std::string_view prefix) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().starts(prefix) : reg.fields(keys_).nt().starts(prefix); });
}

Axel AxelNoteAccessor::ends(std::string_view suffix) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().ends(suffix) : reg.fields(keys_).nt().ends(suffix); });
}

Axel AxelNoteAccessor::has(std::string_view needle,
                           const pluss::table::TextOptions& options) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().has(needle, options) : reg.fields(keys_).nt().has(needle, options); });
}

Axel AxelNoteAccessor::count(std::string_view pattern) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().count(pattern) : reg.fields(keys_).nt().count(pattern); });
}

Axel AxelNoteAccessor::count(std::string_view pattern,
                             const pluss::table::TextOptions& options) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().count(pattern, options) : reg.fields(keys_).nt().count(pattern, options); });
}

Axel AxelNoteAccessor::matches(std::string_view pattern, bool full) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().matches(pattern, full) : reg.fields(keys_).nt().matches(pattern, full); });
}

Axel AxelNoteAccessor::fullmatch(std::string_view pattern) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().fullmatch(pattern) : reg.fields(keys_).nt().fullmatch(pattern); });
}

Axel AxelNoteAccessor::pad(std::int64_t width,
                           pluss::table::TextAlign align,
                           char fill) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().pad(width, align, fill) : reg.fields(keys_).nt().pad(width, align, fill); });
}

Axel AxelNoteAccessor::chars(std::int64_t start,
                             std::optional<std::int64_t> stop,
                             std::int64_t step) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().chars(start, stop, step) : reg.fields(keys_).nt().chars(start, stop, step); });
}

Axel AxelNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().normalize(form) : reg.fields(keys_).nt().normalize(form); });
}

Axel AxelNoteAccessor::encode(std::string_view encoding) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().encode(encoding) : reg.fields(keys_).nt().encode(encoding); });
}

Axel AxelNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().replace(from, to) : reg.fields(keys_).nt().replace(from, to); });
}

Axel AxelNoteAccessor::replace(std::string_view from,
                               std::string_view to,
                               const pluss::table::TextOptions& options) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.nt().replace(from, to, options) : reg.fields(keys_).nt().replace(from, to, options); });
}

AxelTimelineAccessor::AxelTimelineAccessor(Axel base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_AXEL_TL_UNARY(method) \
  Axel AxelTimelineAccessor::method() const { \
    return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().method() : reg.fields(keys_).tl().method(); }); \
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
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().fmt(pattern) : reg.fields(keys_).tl().fmt(pattern); });
}

Axel AxelTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().epoch(unit) : reg.fields(keys_).tl().epoch(unit); });
}

Axel AxelTimelineAccessor::floor(std::string_view unit) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().floor(unit) : reg.fields(keys_).tl().floor(unit); });
}

Axel AxelTimelineAccessor::ceil(std::string_view unit) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().ceil(unit) : reg.fields(keys_).tl().ceil(unit); });
}

Axel AxelTimelineAccessor::round(std::string_view unit) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().round(unit) : reg.fields(keys_).tl().round(unit); });
}

Axel AxelTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().shift(periods, freq) : reg.fields(keys_).tl().shift(periods, freq); });
}

Axel AxelTimelineAccessor::period(std::string_view freq) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().period(freq) : reg.fields(keys_).tl().period(freq); });
}

Axel AxelTimelineAccessor::tz(std::string_view name) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().tz(name) : reg.fields(keys_).tl().tz(name); });
}

Axel AxelTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return base_.map([&](const Registry& reg) { return keys_.empty() ? reg.tl().localize(options) : reg.fields(keys_).tl().localize(options); });
}

