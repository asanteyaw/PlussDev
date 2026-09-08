Slot::Slot() = default;
Slot::Slot(std::vector<Value> values)
    : impl_(pluss::msharp::make_intrusive<detail::SlotImpl>(std::move(values))) {}
Slot::Slot(std::initializer_list<Value> values)
    : Slot(std::vector<Value>(values)) {}
Slot::Slot(Value value)
    : Slot(std::vector<Value>{std::move(value)}) {}
Slot::Slot(const Slot& other) = default;
Slot::Slot(Slot&& other) noexcept = default;
Slot& Slot::operator=(const Slot& other) = default;
Slot& Slot::operator=(Slot&& other) noexcept = default;
Slot::~Slot() = default;

Slot::Slot(pluss::msharp::intrusive_ptr<detail::SlotImpl> impl) : impl_(std::move(impl)) {}

bool Slot::defined() const noexcept { return static_cast<bool>(impl_); }
bool Slot::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Slot::size() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().values_.size());
}

bool Slot::empty() const { return size() == 0; }

std::pair<std::int64_t, std::int64_t> Slot::shape() const {
  return {size(), 1};
}

SlotKind Slot::kind() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return slot_profile(impl_ref_()).kind;
}

std::string Slot::ptype() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (values().empty()) {
    return "Empty";
  }
  std::optional<std::string> first;
  for (const auto& value : values()) {
    const auto name = value_type_name(value);
    if (!first.has_value()) {
      first = name;
    } else if (*first != name) {
      return "Mixed";
    }
  }
  return *first;
}

const std::vector<Slot::Value>& Slot::values() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return impl_ref_().values_;
}

const Slot::Value& Slot::at(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(i >= 0 && i < size(), "Slot::at: index out of bounds");
  return impl_ref_().values_[static_cast<std::size_t>(i)];
}

Slot Slot::push(const Value& value) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  auto out = *this;
  out.push_(value);
  return out;
}

Slot& Slot::push_(const Value& value) {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  auto& impl = mut_impl_();
  impl.values_.push_back(value);
  impl.invalidate_profile();
  return *this;
}

Slot Slot::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(0, clipped);
}

Slot Slot::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(size() - clipped, size());
}

Slot Slot::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<Value> out;
  out.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    out.push_back(at(i));
  }
  return Slot(std::move(out));
}

Slot Slot::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  std::vector<Value> out;
  out.reserve(indices.size());
  for (const auto idx : indices) {
    out.push_back(at(idx));
  }
  return Slot(std::move(out));
}

Slot Slot::filter(const std::function<bool(const Value&)>& predicate) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(static_cast<bool>(predicate), "Slot::filter: predicate is empty");
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    if (predicate(v)) out.push_back(v);
  }
  return Slot(std::move(out));
}

Slot Slot::transform(const std::function<Value(const Value&)>& fn) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Slot::transform: fn is empty");
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    out.push_back(fn(v));
  }
  return Slot(std::move(out));
}

Slot Slot::concat(const Slot& other) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(other.defined(), "Slot::concat: other slot is undefined");
  std::vector<Value> out;
  out.reserve(values().size() + other.values().size());
  out.insert(out.end(), values().begin(), values().end());
  out.insert(out.end(), other.values().begin(), other.values().end());
  return Slot(std::move(out));
}

std::unordered_map<std::string, std::int64_t> Slot::type_counts() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    out[value_type_name(v)] += 1;
  }
  return out;
}

Slot Slot::isnull() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (is_stream_compatible()) {
    return kindelize().isnull().scribbelize();
  }
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& value : values()) {
    out.push_back(bool_slot_value(slot_value_isnull(value)));
  }
  return Slot(std::move(out));
}

Slot Slot::notnull() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (is_stream_compatible()) {
    return kindelize().notnull().scribbelize();
  }
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& value : values()) {
    out.push_back(bool_slot_value(!slot_value_isnull(value)));
  }
  return Slot(std::move(out));
}

Slot Slot::invalid() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (is_stream_compatible()) {
    return kindelize().invalid().scribbelize();
  }
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& value : values()) {
    out.push_back(bool_slot_value(slot_value_invalid(value)));
  }
  return Slot(std::move(out));
}

Slot Slot::fillnull(double value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Series,
      "Slot::fillnull(double)",
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Series, value, {}, nullptr, nullptr));
}

Slot Slot::fillnull(std::string_view value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Note,
      "Slot::fillnull(string)",
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Note, 0.0, value, nullptr, nullptr));
}

Slot Slot::fillnull(const pluss::Timeline& value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Timeline,
      "Slot::fillnull(Timeline)",
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Timeline, 0.0, {}, &value, nullptr));
}

Slot Slot::fillnull(const pluss::TimeIndex& value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::TimeIndex,
      "Slot::fillnull(TimeIndex)",
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::TimeIndex, 0.0, {}, nullptr, &value));
}

Slot Slot::dropnull() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (is_stream_compatible()) {
    return kindelize().dropnull().scribbelize();
  }
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& value : values()) {
    const bool missing = slot_value_isnull(value);
    if (!missing) out.push_back(value);
  }
  return Slot(std::move(out));
}

Slot Slot::distinct(pluss::kindle::DistinctOptions options) const {
  return kindelize().distinct(options).scribbelize();
}

Slot Slot::freq(pluss::kindle::FreqOptions options) const {
  return kindelize().freq(options).scribbelize();
}

Slot Slot::ndistinct(pluss::kindle::DistinctOptions options) const {
  return kindelize().ndistinct(options).scribbelize();
}

SlotCastAccessor Slot::cast() const {
  return SlotCastAccessor(*this, {});
}

SlotCastAccessor Slot::cast(const pluss::table::ThreadCastOptions& options) const {
  return SlotCastAccessor(*this, options);
}

SlotNoteAccessor Slot::nt() const {
  return SlotNoteAccessor(*this);
}

SlotTimelineAccessor Slot::tl() const {
  return SlotTimelineAccessor(*this);
}

Slot Slot::cast(pluss::table::ThreadCastTarget target,
                const pluss::table::ThreadCastOptions& options) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (is_stream_compatible()) {
    return kindelize().cast(target, options).scribbelize();
  }
  if (target == pluss::table::ThreadCastTarget::Note) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) out.push_back(slot_value_to_note(value));
    return Slot(std::move(out));
  }

  struct SeriesBatch {
    pluss::codem::ScalarType dtype{};
    std::vector<std::int64_t> positions{};
    std::vector<Value> values{};
  };

  std::vector<SeriesBatch> series_batches;
  std::vector<std::int64_t> note_positions;
  std::vector<Value> note_values;
  std::vector<std::int64_t> timeline_positions;
  std::vector<Value> timeline_values;
  std::vector<std::int64_t> timeindex_positions;
  std::vector<Value> timeindex_values;
  std::vector<std::int64_t> slow_positions;

  std::vector<Value> out(static_cast<std::size_t>(size()), std::monostate{});

  for (std::int64_t i = 0; i < size(); ++i) {
    const auto& value = at(i);
    if (std::holds_alternative<std::monostate>(value)) {
      continue;
    }
    if (!slot_value_scalar_like(value)) {
      slow_positions.push_back(i);
      continue;
    }

    switch (slot_kind_of_value(value)) {
      case SlotKind::Series: {
        const auto dtype = std::get<pluss::Series>(value).scalar_type();
        auto it = std::find_if(series_batches.begin(), series_batches.end(),
                               [&](const SeriesBatch& batch) { return batch.dtype == dtype; });
        if (it == series_batches.end()) {
          series_batches.push_back(SeriesBatch{.dtype = dtype});
          it = std::prev(series_batches.end());
        }
        it->positions.push_back(i);
        it->values.push_back(value);
        break;
      }
      case SlotKind::Note:
        note_positions.push_back(i);
        note_values.push_back(value);
        break;
      case SlotKind::Timeline:
        timeline_positions.push_back(i);
        timeline_values.push_back(value);
        break;
      case SlotKind::TimeIndex:
        timeindex_positions.push_back(i);
        timeindex_values.push_back(value);
        break;
      case SlotKind::Empty:
      case SlotKind::Null:
      case SlotKind::Mixed:
        slow_positions.push_back(i);
        break;
    }
  }

  auto scatter_casted = [&](const std::vector<std::int64_t>& positions, std::vector<Value> values_batch) {
    if (positions.empty()) return;
    auto casted = Slot(std::move(values_batch)).kindelize().cast(target, options).scribbelize();
    const auto& casted_values = casted.values();
    for (std::size_t j = 0; j < positions.size(); ++j) {
      out[static_cast<std::size_t>(positions[j])] = casted_values[j];
    }
  };

  for (auto& batch : series_batches) {
    scatter_casted(batch.positions, std::move(batch.values));
  }
  scatter_casted(note_positions, std::move(note_values));
  scatter_casted(timeline_positions, std::move(timeline_values));
  scatter_casted(timeindex_positions, std::move(timeindex_values));

  for (const auto pos : slow_positions) {
    const auto& value = at(pos);
    const auto casted = [&]() {
      const auto thread = thread_from_slot_value(value);
      switch (target) {
        case pluss::table::ThreadCastTarget::Series:
          return thread.cast(options).series();
        case pluss::table::ThreadCastTarget::Note:
          return thread.cast(options).note();
        case pluss::table::ThreadCastTarget::Timeline:
          return thread.cast(options).timeline();
        case pluss::table::ThreadCastTarget::TimeIndex:
          return thread.cast(options).timeindex();
      }
      return thread;
    }();
    out[static_cast<std::size_t>(pos)] = slot_value_from_thread_payload(casted);
  }
  return Slot(std::move(out));
}

Slot Slot::astype(pluss::codem::ScalarType dtype) const {
  return cast(pluss::table::ThreadCastTarget::Series,
              pluss::table::ThreadCastOptions{.series_dtype = dtype});
}

Slot Slot::sum() const {
  return kindelize().sum().scribbelize();
}

Slot Slot::mean() const {
  return kindelize().mean().scribbelize();
}

Slot Slot::min() const {
  return kindelize().min().scribbelize();
}

Slot Slot::max() const {
  return kindelize().max().scribbelize();
}

bool Slot::is_stream_compatible() const {
  if (!defined()) return false;
  return slot_profile(impl_ref_()).stream_compatible;
}

void Slot::ensure_stream_compatible(std::string_view where) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto& profile = slot_profile(impl_ref_());
  PLUSS_CHECK(profile.stream_compatible,
              std::string(where.empty() ? "Slot" : where) +
                  ": slot is not stream-compatible (kind=" + slot_kind_name(profile.kind) + ")");
}

pluss::kindle::Stream Slot::kindelize() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto& profile = slot_profile(impl_ref_());
  if (profile.kind == SlotKind::Empty) {
    return pluss::kindle::Stream(pluss::make_note(std::vector<std::string>{}));
  }
  PLUSS_CHECK(profile.stream_compatible,
              "Slot::kindelize: slot is not stream-compatible (kind=" + slot_kind_name(profile.kind) + ")");

  if (profile.kind == SlotKind::Series) {
    // A single scalar value is stored as a 0D Series (numel() == 1, dim() ==
    // 0) -- every kindle::Stream payload must be strictly 1D (see Thread's
    // constructor-time checks in ThreadInternals.inc), the same invariant the
    // N>1 path below already produces via make_series(out, {n}). reshape()
    // is a pure metadata change here (numel is already 1), so this is exact:
    // no dtype round-trip, no precision loss, unlike the N>1 non-Bool/Int64
    // path's Float64 staging below.
    if (values().size() == 1) {
      return pluss::kindle::Stream(std::get<pluss::Series>(values().front()).reshape({1}));
    }

    const auto dtype = std::get<pluss::Series>(values().front()).scalar_type();
    if (dtype == pluss::codem::ScalarType::Bool) {
      std::vector<bool> out;
      out.reserve(values().size());
      for (const auto& v : values()) out.push_back(std::get<pluss::Series>(v).to_vec<bool>()[0]);
      const auto n = static_cast<std::int64_t>(out.size());
      auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
      auto* p = reinterpret_cast<bool*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) p[i] = out[static_cast<std::size_t>(i)];
      return pluss::kindle::Stream(s);
    }
    if (dtype == pluss::codem::ScalarType::Int64) {
      std::vector<std::int64_t> out;
      out.reserve(values().size());
      for (const auto& v : values()) out.push_back(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0]);
      return pluss::kindle::Stream(pluss::make_series(out, {static_cast<std::int64_t>(out.size())}));
    }

    std::vector<double> out;
    out.reserve(values().size());
    for (const auto& v : values()) {
      out.push_back(std::get<pluss::Series>(v).to(pluss::codem::ScalarType::Float64).to_vec<double>()[0]);
    }
    auto merged = pluss::make_series(out, {static_cast<std::int64_t>(out.size())});
    if (dtype != pluss::codem::ScalarType::Float64) merged = merged.to(dtype);
    return pluss::kindle::Stream(merged);
  }

  if (profile.kind == SlotKind::Note) {
    // Same 0D-scalar-must-become-1D reshape as the Series branch above.
    if (values().size() == 1) {
      return pluss::kindle::Stream(std::get<pluss::Note>(values().front()).reshape({1}));
    }
    std::vector<std::string> out;
    out.reserve(values().size());
    for (const auto& v : values()) out.emplace_back(std::get<pluss::Note>(v).at(0));
    return pluss::kindle::Stream(pluss::make_note(out));
  }

  if (profile.kind == SlotKind::Timeline) {
    // Timeline has no reshape() of its own (unlike Series/Note above); its
    // ticks_ Series does, and unsafe_ticks()/timeline_from_series() are the
    // same public "reshape while preserving tz" pair already used to keep
    // display-timezone labels intact across this same Slot<->Stream boundary
    // (see slot_family_stream() above). tz() is preserved explicitly since
    // timeline_from_series would otherwise default it back to "UTC".
    if (values().size() == 1) {
      const auto& t = std::get<pluss::Timeline>(values().front());
      return pluss::kindle::Stream(pluss::codeh::ops_internal::timeline_from_series(
          t.unsafe_ticks().reshape({1}), std::string(t.tz())));
    }
    std::vector<std::int64_t> ticks;
    ticks.reserve(values().size());
    std::optional<std::string> tz_name;
    for (const auto& v : values()) {
      const auto& t = std::get<pluss::Timeline>(v);
      ticks.push_back(t.ticks()[0]);
      std::string value_tz(t.tz());
      if (!tz_name.has_value()) {
        tz_name = std::move(value_tz);
      } else {
        PLUSS_CHECK(*tz_name == value_tz,
                    std::string("Slot::kindelize: Timeline slot values carry mismatched timezones ('") +
                        *tz_name + "' vs '" + value_tz +
                        "'); a Stream's Timeline column has one shared tz, so relabel with tz() "
                        "(or localize()) to a common zone before combining");
      }
    }
    return pluss::kindle::Stream(pluss::timeline::from_epoch(
        ticks,
        std::vector<std::int64_t>{static_cast<std::int64_t>(ticks.size())},
        tz_name.value_or("UTC")));
  }

  if (profile.kind == SlotKind::TimeIndex) {
    // Same 0D-scalar reshape as the Timeline branch above, via TimeIndex's
    // own underlying Timeline.
    if (values().size() == 1) {
      const auto& tl = std::get<pluss::TimeIndex>(values().front()).timeline();
      auto reshaped = pluss::codeh::ops_internal::timeline_from_series(
          tl.unsafe_ticks().reshape({1}), std::string(tl.tz()));
      return pluss::kindle::Stream(pluss::make_timeindex(reshaped, false));
    }
    std::vector<std::int64_t> ticks;
    ticks.reserve(values().size());
    std::optional<std::string> tz_name;
    for (const auto& v : values()) {
      const auto& tl = std::get<pluss::TimeIndex>(v).timeline();
      ticks.push_back(tl.ticks()[0]);
      std::string value_tz(tl.tz());
      if (!tz_name.has_value()) {
        tz_name = std::move(value_tz);
      } else {
        PLUSS_CHECK(*tz_name == value_tz,
                    std::string("Slot::kindelize: TimeIndex slot values carry mismatched timezones ('") +
                        *tz_name + "' vs '" + value_tz +
                        "'); a Stream's TimeIndex column has one shared tz, so relabel with tz() "
                        "(or localize()) to a common zone before combining");
      }
    }
    auto merged = pluss::timeline::from_epoch(
        ticks,
        std::vector<std::int64_t>{static_cast<std::int64_t>(ticks.size())},
        tz_name.value_or("UTC"));
    return pluss::kindle::Stream(pluss::make_timeindex(merged, false));
  }

  throw pluss::Error("Slot::kindelize: unsupported slot kind");
}

pluss::table::Thread Slot::tabelize() const {
  return kindelize().tabelize();
}
