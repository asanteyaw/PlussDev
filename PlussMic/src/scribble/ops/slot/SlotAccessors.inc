SlotCastAccessor::SlotCastAccessor(Slot base, pluss::table::ThreadCastOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

Slot SlotCastAccessor::series() const {
  return base_.cast(pluss::table::ThreadCastTarget::Series, options_);
}

Slot SlotCastAccessor::note() const {
  return base_.cast(pluss::table::ThreadCastTarget::Note, options_);
}

Slot SlotCastAccessor::timeline() const {
  return base_.cast(pluss::table::ThreadCastTarget::Timeline, options_);
}

Slot SlotCastAccessor::duration() const {
  return base_.cast(pluss::table::ThreadCastTarget::Duration, options_);
}

SlotNoteAccessor::SlotNoteAccessor(Slot base) : base_(std::move(base)) {}

#define PLUSS_SLOT_NT_STREAM() slot_family_stream(base_, SlotKind::Note, "Slot::nt")
#define PLUSS_SLOT_NT_UNARY(method) \
  Slot SlotNoteAccessor::method() const { \
    return PLUSS_SLOT_NT_STREAM().nt().method().scribbelize(); \
  }

PLUSS_SLOT_NT_UNARY(strip)
PLUSS_SLOT_NT_UNARY(lstrip)
PLUSS_SLOT_NT_UNARY(rstrip)
PLUSS_SLOT_NT_UNARY(lower)
PLUSS_SLOT_NT_UNARY(upper)
PLUSS_SLOT_NT_UNARY(casefold)
PLUSS_SLOT_NT_UNARY(titlecase)
PLUSS_SLOT_NT_UNARY(length)

#undef PLUSS_SLOT_NT_UNARY

Slot SlotNoteAccessor::strip(std::string_view chars) const {
  return PLUSS_SLOT_NT_STREAM().nt().strip(chars).scribbelize();
}

Slot SlotNoteAccessor::lstrip(std::string_view chars) const {
  return PLUSS_SLOT_NT_STREAM().nt().lstrip(chars).scribbelize();
}

Slot SlotNoteAccessor::rstrip(std::string_view chars) const {
  return PLUSS_SLOT_NT_STREAM().nt().rstrip(chars).scribbelize();
}

Slot SlotNoteAccessor::starts(std::string_view prefix) const {
  return PLUSS_SLOT_NT_STREAM().nt().starts(prefix).scribbelize();
}

Slot SlotNoteAccessor::ends(std::string_view suffix) const {
  return PLUSS_SLOT_NT_STREAM().nt().ends(suffix).scribbelize();
}

Slot SlotNoteAccessor::has(std::string_view needle,
                           const pluss::table::TextOptions& options) const {
  return PLUSS_SLOT_NT_STREAM().nt().has(needle, options).scribbelize();
}

Slot SlotNoteAccessor::count(std::string_view pattern) const {
  return PLUSS_SLOT_NT_STREAM().nt().count(pattern).scribbelize();
}

Slot SlotNoteAccessor::count(std::string_view pattern,
                             const pluss::table::TextOptions& options) const {
  return PLUSS_SLOT_NT_STREAM().nt().count(pattern, options).scribbelize();
}

Slot SlotNoteAccessor::matches(std::string_view pattern, bool full) const {
  return PLUSS_SLOT_NT_STREAM().nt().matches(pattern, full).scribbelize();
}

Slot SlotNoteAccessor::fullmatch(std::string_view pattern) const {
  return PLUSS_SLOT_NT_STREAM().nt().fullmatch(pattern).scribbelize();
}

pluss::table::DataFrame SlotNoteAccessor::findall(std::string_view pattern) const {
  return PLUSS_SLOT_NT_STREAM().nt().findall(pattern);
}

pluss::table::DataFrame SlotNoteAccessor::findall(std::string_view pattern,
                                                  const pluss::table::TextOptions& options) const {
  return PLUSS_SLOT_NT_STREAM().nt().findall(pattern, options);
}

pluss::table::DataFrame SlotNoteAccessor::split(std::string_view delimiter,
                                                std::int64_t limit,
                                                std::string prefix) const {
  return PLUSS_SLOT_NT_STREAM().nt().split(delimiter, limit, std::move(prefix));
}

pluss::table::DataFrame SlotNoteAccessor::split(std::string_view pattern,
                                                const pluss::table::TextOptions& options) const {
  return PLUSS_SLOT_NT_STREAM().nt().split(pattern, options);
}

pluss::table::DataFrame SlotNoteAccessor::partition(std::string_view separator,
                                                    std::string prefix) const {
  return PLUSS_SLOT_NT_STREAM().nt().partition(separator, std::move(prefix));
}

Slot SlotNoteAccessor::pad(std::int64_t width,
                           pluss::table::TextAlign align,
                           char fill) const {
  return PLUSS_SLOT_NT_STREAM().nt().pad(width, align, fill).scribbelize();
}

Slot SlotNoteAccessor::chars(std::int64_t start,
                             std::optional<std::int64_t> stop,
                             std::int64_t step) const {
  return PLUSS_SLOT_NT_STREAM().nt().chars(start, stop, step).scribbelize();
}

Slot SlotNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return PLUSS_SLOT_NT_STREAM().nt().normalize(form).scribbelize();
}

Slot SlotNoteAccessor::encode(std::string_view encoding) const {
  return PLUSS_SLOT_NT_STREAM().nt().encode(encoding).scribbelize();
}

pluss::table::DataFrame SlotNoteAccessor::indicate(std::string_view delimiter,
                                                   std::string prefix) const {
  return PLUSS_SLOT_NT_STREAM().nt().indicate(delimiter, std::move(prefix));
}

Slot SlotNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return PLUSS_SLOT_NT_STREAM().nt().replace(from, to).scribbelize();
}

Slot SlotNoteAccessor::replace(std::string_view from,
                               std::string_view to,
                               const pluss::table::TextOptions& options) const {
  return PLUSS_SLOT_NT_STREAM().nt().replace(from, to, options).scribbelize();
}

#undef PLUSS_SLOT_NT_STREAM

SlotTimelineAccessor::SlotTimelineAccessor(Slot base) : base_(std::move(base)) {}

#define PLUSS_SLOT_TL_STREAM() slot_family_stream(base_, SlotKind::Timeline, "Slot::tl")
#define PLUSS_SLOT_TL_UNARY(method) \
  Slot SlotTimelineAccessor::method() const { \
    return PLUSS_SLOT_TL_STREAM().tl().method().scribbelize(); \
  }

PLUSS_SLOT_TL_UNARY(isnat)
PLUSS_SLOT_TL_UNARY(valid)
PLUSS_SLOT_TL_UNARY(year)
PLUSS_SLOT_TL_UNARY(month)
PLUSS_SLOT_TL_UNARY(day)
PLUSS_SLOT_TL_UNARY(hour)
PLUSS_SLOT_TL_UNARY(minute)
PLUSS_SLOT_TL_UNARY(second)
PLUSS_SLOT_TL_UNARY(weekday)
PLUSS_SLOT_TL_UNARY(yearday)
PLUSS_SLOT_TL_UNARY(quarter)
PLUSS_SLOT_TL_UNARY(week)
PLUSS_SLOT_TL_UNARY(weekend)
PLUSS_SLOT_TL_UNARY(month_start)
PLUSS_SLOT_TL_UNARY(month_end)
PLUSS_SLOT_TL_UNARY(leap)
PLUSS_SLOT_TL_UNARY(weekday_name)
PLUSS_SLOT_TL_UNARY(month_name)
PLUSS_SLOT_TL_UNARY(iso)
PLUSS_SLOT_TL_UNARY(date)
PLUSS_SLOT_TL_UNARY(time)
PLUSS_SLOT_TL_UNARY(utc)

#undef PLUSS_SLOT_TL_UNARY

Slot SlotTimelineAccessor::fmt(std::string_view pattern) const {
  return PLUSS_SLOT_TL_STREAM().tl().fmt(pattern).scribbelize();
}

Slot SlotTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return PLUSS_SLOT_TL_STREAM().tl().epoch(unit).scribbelize();
}

Slot SlotTimelineAccessor::floor(std::string_view unit) const {
  return PLUSS_SLOT_TL_STREAM().tl().floor(unit).scribbelize();
}

Slot SlotTimelineAccessor::ceil(std::string_view unit) const {
  return PLUSS_SLOT_TL_STREAM().tl().ceil(unit).scribbelize();
}

Slot SlotTimelineAccessor::round(std::string_view unit) const {
  return PLUSS_SLOT_TL_STREAM().tl().round(unit).scribbelize();
}

Slot SlotTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return PLUSS_SLOT_TL_STREAM().tl().shift(periods, freq).scribbelize();
}

Slot SlotTimelineAccessor::period(std::string_view freq) const {
  return PLUSS_SLOT_TL_STREAM().tl().period(freq).scribbelize();
}

Slot SlotTimelineAccessor::tz(std::string_view name) const {
  return PLUSS_SLOT_TL_STREAM().tl().tz(name).scribbelize();
}

Slot SlotTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return PLUSS_SLOT_TL_STREAM().tl().localize(options).scribbelize();
}

#undef PLUSS_SLOT_TL_STREAM
