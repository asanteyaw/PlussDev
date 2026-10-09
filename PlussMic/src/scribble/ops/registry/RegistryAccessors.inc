namespace {

template <class Fn>
Registry registry_map_selected(const Registry& base, const std::vector<std::string>& keys, Fn&& fn) {
  PLUSS_CHECK(base.defined(), "Registry: undefined handle");
  PLUSS_CHECK(!keys.empty(), "Registry: at least one field is required");
  for (const auto& key : keys) {
    PLUSS_CHECK(base.has(key), "Registry: unknown field '" + key + "'");
  }
  return base.fields(keys).map(std::function<Slot(const Slot&)>(std::forward<Fn>(fn)));
}

} // namespace

RegistryFieldAxisAccessor::RegistryFieldAxisAccessor(Registry base) : base_(std::move(base)) {}

Registry RegistryFieldAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  PLUSS_CHECK(base_.defined(), "Registry::fieldaxis: undefined handle");
  const auto token = case_sensitive ? std::string(needle) : lower_copy(std::string(needle));
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nfields()));
  for (const auto& key : base_.fields()) {
    const auto hay = case_sensitive ? key : lower_copy(key);
    if (hay.find(token) != std::string::npos) selected.push_back(key);
  }
  return base_.fields(selected);
}

Registry RegistryFieldAxisAccessor::match(std::string_view pattern) const {
  PLUSS_CHECK(base_.defined(), "Registry::fieldaxis: undefined handle");
  try {
    const std::regex rx{std::string(pattern)};
    std::vector<std::string> selected;
    selected.reserve(static_cast<std::size_t>(base_.nfields()));
    for (const auto& key : base_.fields()) {
      if (std::regex_search(key, rx)) selected.push_back(key);
    }
    return base_.fields(selected);
  } catch (const std::regex_error& e) {
    PLUSS_FAIL("Registry::fieldaxis().match: invalid regex '" + std::string(pattern) + "': " + std::string(e.what()));
  }
}

Registry RegistryFieldAxisAccessor::prefix(std::string_view token) const {
  PLUSS_CHECK(base_.defined(), "Registry::fieldaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nfields()));
  const auto prefix_value = std::string(token);
  for (const auto& key : base_.fields()) {
    if (key.rfind(prefix_value, 0) == 0) selected.push_back(key);
  }
  return base_.fields(selected);
}

Registry RegistryFieldAxisAccessor::suffix(std::string_view token) const {
  PLUSS_CHECK(base_.defined(), "Registry::fieldaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nfields()));
  const auto suffix_value = std::string(token);
  for (const auto& key : base_.fields()) {
    if (key.size() >= suffix_value.size() &&
        key.compare(key.size() - suffix_value.size(), suffix_value.size(), suffix_value) == 0) {
      selected.push_back(key);
    }
  }
  return base_.fields(selected);
}

Registry RegistryFieldAxisAccessor::kind(SlotKind kind_value) const {
  PLUSS_CHECK(base_.defined(), "Registry::fieldaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nfields()));
  for (const auto& key : base_.fields()) {
    if (base_.fields(key).kind() == kind_value) selected.push_back(key);
  }
  return base_.fields(selected);
}

Registry RegistryFieldAxisAccessor::series() const { return kind(SlotKind::Series); }
Registry RegistryFieldAxisAccessor::note() const { return kind(SlotKind::Note); }
Registry RegistryFieldAxisAccessor::timeline() const { return kind(SlotKind::Timeline); }
Registry RegistryFieldAxisAccessor::duration() const { return kind(SlotKind::Duration); }
Registry RegistryFieldAxisAccessor::mixed() const { return kind(SlotKind::Mixed); }
Registry RegistryFieldAxisAccessor::nulls() const { return kind(SlotKind::Null); }
Registry RegistryFieldAxisAccessor::empty() const { return kind(SlotKind::Empty); }

RegistryCastAccessor::RegistryCastAccessor(Registry base,
                                           std::vector<std::string> keys,
                                           pluss::table::SlateCastOptions options)
    : base_(std::move(base)), keys_(std::move(keys)), options_(std::move(options)) {}

Registry RegistryCastAccessor::series() const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.cast(options_).series(); });
}

Registry RegistryCastAccessor::note() const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.cast(options_).note(); });
}

Registry RegistryCastAccessor::timeline() const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.cast(options_).timeline(); });
}

Registry RegistryCastAccessor::duration() const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.cast(options_).duration(); });
}

RegistryNoteAccessor::RegistryNoteAccessor(Registry base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_REGISTRY_NOTE_UNARY(method) \
  Registry RegistryNoteAccessor::method() const { \
    return registry_map_selected(base_, keys_, [](const Slot& slot) { return slot.nt().method(); }); \
  }

PLUSS_REGISTRY_NOTE_UNARY(strip)
PLUSS_REGISTRY_NOTE_UNARY(lstrip)
PLUSS_REGISTRY_NOTE_UNARY(rstrip)
PLUSS_REGISTRY_NOTE_UNARY(lower)
PLUSS_REGISTRY_NOTE_UNARY(upper)
PLUSS_REGISTRY_NOTE_UNARY(casefold)
PLUSS_REGISTRY_NOTE_UNARY(titlecase)
PLUSS_REGISTRY_NOTE_UNARY(length)

#undef PLUSS_REGISTRY_NOTE_UNARY

Registry RegistryNoteAccessor::strip(std::string_view chars) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().strip(chars); });
}

Registry RegistryNoteAccessor::lstrip(std::string_view chars) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().lstrip(chars); });
}

Registry RegistryNoteAccessor::rstrip(std::string_view chars) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().rstrip(chars); });
}

Registry RegistryNoteAccessor::starts(std::string_view prefix) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().starts(prefix); });
}

Registry RegistryNoteAccessor::ends(std::string_view suffix) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().ends(suffix); });
}

Registry RegistryNoteAccessor::has(std::string_view needle,
                                   const pluss::table::TextOptions& options) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().has(needle, options); });
}

Registry RegistryNoteAccessor::count(std::string_view pattern) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().count(pattern); });
}

Registry RegistryNoteAccessor::count(std::string_view pattern,
                                     const pluss::table::TextOptions& options) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().count(pattern, options); });
}

Registry RegistryNoteAccessor::matches(std::string_view pattern, bool full) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().matches(pattern, full); });
}

Registry RegistryNoteAccessor::fullmatch(std::string_view pattern) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().fullmatch(pattern); });
}

Registry RegistryNoteAccessor::pad(std::int64_t width,
                                   pluss::table::TextAlign align,
                                   char fill) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().pad(width, align, fill); });
}

Registry RegistryNoteAccessor::chars(std::int64_t start,
                                     std::optional<std::int64_t> stop,
                                     std::int64_t step) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().chars(start, stop, step); });
}

Registry RegistryNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().normalize(form); });
}

Registry RegistryNoteAccessor::encode(std::string_view encoding) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().encode(encoding); });
}

Registry RegistryNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().replace(from, to); });
}

Registry RegistryNoteAccessor::replace(std::string_view from,
                                       std::string_view to,
                                       const pluss::table::TextOptions& options) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.nt().replace(from, to, options); });
}

RegistryTimelineAccessor::RegistryTimelineAccessor(Registry base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {}

#define PLUSS_REGISTRY_TL_UNARY(method) \
  Registry RegistryTimelineAccessor::method() const { \
    return registry_map_selected(base_, keys_, [](const Slot& slot) { return slot.tl().method(); }); \
  }

PLUSS_REGISTRY_TL_UNARY(isnat)
PLUSS_REGISTRY_TL_UNARY(valid)
PLUSS_REGISTRY_TL_UNARY(year)
PLUSS_REGISTRY_TL_UNARY(month)
PLUSS_REGISTRY_TL_UNARY(day)
PLUSS_REGISTRY_TL_UNARY(hour)
PLUSS_REGISTRY_TL_UNARY(minute)
PLUSS_REGISTRY_TL_UNARY(second)
PLUSS_REGISTRY_TL_UNARY(weekday)
PLUSS_REGISTRY_TL_UNARY(yearday)
PLUSS_REGISTRY_TL_UNARY(quarter)
PLUSS_REGISTRY_TL_UNARY(week)
PLUSS_REGISTRY_TL_UNARY(weekend)
PLUSS_REGISTRY_TL_UNARY(month_start)
PLUSS_REGISTRY_TL_UNARY(month_end)
PLUSS_REGISTRY_TL_UNARY(leap)
PLUSS_REGISTRY_TL_UNARY(weekday_name)
PLUSS_REGISTRY_TL_UNARY(month_name)
PLUSS_REGISTRY_TL_UNARY(iso)
PLUSS_REGISTRY_TL_UNARY(date)
PLUSS_REGISTRY_TL_UNARY(time)
PLUSS_REGISTRY_TL_UNARY(utc)

#undef PLUSS_REGISTRY_TL_UNARY

Registry RegistryTimelineAccessor::fmt(std::string_view pattern) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().fmt(pattern); });
}

Registry RegistryTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().epoch(unit); });
}

Registry RegistryTimelineAccessor::floor(std::string_view unit) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().floor(unit); });
}

Registry RegistryTimelineAccessor::ceil(std::string_view unit) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().ceil(unit); });
}

Registry RegistryTimelineAccessor::round(std::string_view unit) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().round(unit); });
}

Registry RegistryTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().shift(periods, freq); });
}

Registry RegistryTimelineAccessor::period(std::string_view freq) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().period(freq); });
}

Registry RegistryTimelineAccessor::tz(std::string_view name) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().tz(name); });
}

Registry RegistryTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return registry_map_selected(base_, keys_, [&](const Slot& slot) { return slot.tl().localize(options); });
}

