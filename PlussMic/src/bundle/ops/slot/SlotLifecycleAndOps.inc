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
  mut_impl_().values_.push_back(value);
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
  try {
    return Slot::from_kindle(to_kindle().isnull());
  } catch (const std::exception&) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) {
      out.push_back(bool_slot_value(slot_value_isnull(value)));
    }
    return Slot(std::move(out));
  }
}

Slot Slot::notnull() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  try {
    return Slot::from_kindle(to_kindle().notnull());
  } catch (const std::exception&) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) {
      out.push_back(bool_slot_value(!slot_value_isnull(value)));
    }
    return Slot(std::move(out));
  }
}

Slot Slot::invalid() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  try {
    return Slot::from_kindle(to_kindle().invalid());
  } catch (const std::exception&) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) {
      out.push_back(bool_slot_value(slot_value_invalid(value)));
    }
    return Slot(std::move(out));
  }
}

Slot Slot::fillnull(double value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Series,
      "Slot::fillnull(double)",
      [&](const pluss::table::Thread& t) -> Value { return t.fillnull(value).as_series(); },
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Series, value, {}, nullptr, nullptr));
}

Slot Slot::fillnull(std::string_view value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Note,
      "Slot::fillnull(string)",
      [&](const pluss::table::Thread& t) -> Value { return t.fillnull(value).as_note(); },
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Note, 0.0, value, nullptr, nullptr));
}

Slot Slot::fillnull(const pluss::Timeline& value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::Timeline,
      "Slot::fillnull(Timeline)",
      [&](const pluss::table::Thread& t) -> Value { return t.fillnull(value).as_timeline(); },
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::Timeline, 0.0, {}, &value, nullptr));
}

Slot Slot::fillnull(const pluss::TimeIndex& value) const {
  return fill_slot_nulls(
      *this,
      pluss::table::ThreadCastTarget::TimeIndex,
      "Slot::fillnull(TimeIndex)",
      [&](const pluss::table::Thread& t) -> Value { return t.fillnull(value).as_timeindex(); },
      scalar_fill_value_for_target(pluss::table::ThreadCastTarget::TimeIndex, 0.0, {}, nullptr, &value));
}

Slot Slot::dropnull() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  try {
    return Slot::from_kindle(to_kindle().dropnull());
  } catch (const std::exception&) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) {
      const bool missing = slot_value_isnull(value);
      if (!missing) out.push_back(value);
    }
    return Slot(std::move(out));
  }
}

Slot Slot::distinct(pluss::kindle::DistinctOptions options) const {
  return Slot::from_kindle(to_kindle().distinct(options));
}

Slot Slot::freq(pluss::kindle::FreqOptions options) const {
  return Slot::from_kindle(to_kindle().freq(options));
}

Slot Slot::ndistinct(pluss::kindle::DistinctOptions options) const {
  return Slot::from_kindle(to_kindle().ndistinct(options));
}

Slot Slot::cast(pluss::table::ThreadCastTarget target,
                const pluss::table::ThreadCastOptions& options) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (target == pluss::table::ThreadCastTarget::Note) {
    std::vector<Value> out;
    out.reserve(values().size());
    for (const auto& value : values()) out.push_back(slot_value_to_note(value));
    return Slot(std::move(out));
  }
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& value : values()) {
    if (std::holds_alternative<std::monostate>(value)) {
      out.push_back(std::monostate{});
      continue;
    }
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
    out.push_back(slot_value_from_thread_payload(casted));
  }
  return Slot(std::move(out));
}

Slot Slot::astype(pluss::codem::ScalarType dtype) const {
  return cast(pluss::table::ThreadCastTarget::Series,
              pluss::table::ThreadCastOptions{.series_dtype = dtype});
}

Slot Slot::sum() const {
  return Slot::from_kindle(to_kindle().sum());
}

Slot Slot::mean() const {
  return Slot::from_kindle(to_kindle().mean());
}

Slot Slot::min() const {
  return Slot::from_kindle(to_kindle().min());
}

Slot Slot::max() const {
  return Slot::from_kindle(to_kindle().max());
}

pluss::kindle::Stream Slot::to_kindle() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (empty()) return pluss::kindle::Stream(pluss::make_note(std::vector<std::string>{}));

  if (all_type<pluss::Series>(values())) {
    if (values().size() == 1) return pluss::kindle::Stream(std::get<pluss::Series>(values().front()));

    const auto dtype = std::get<pluss::Series>(values().front()).scalar_type();
    if (dtype == pluss::codem::ScalarType::Bool) {
      std::vector<bool> out;
      out.reserve(values().size());
      for (const auto& v : values()) {
        const auto& s = std::get<pluss::Series>(v);
        PLUSS_CHECK(s.scalar_type() == dtype, "Slot::to_kindle: mixed Series dtypes are not stream-compatible");
        PLUSS_CHECK(s.numel() == 1, "Slot::to_kindle: Series values must be scalar-like (numel==1)");
        out.push_back(s.to_vec<bool>()[0]);
      }
      const auto n = static_cast<std::int64_t>(out.size());
      auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
      auto* p = reinterpret_cast<bool*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) p[i] = out[static_cast<std::size_t>(i)];
      return pluss::kindle::Stream(s);
    }
    if (dtype == pluss::codem::ScalarType::Int64) {
      std::vector<std::int64_t> out;
      out.reserve(values().size());
      for (const auto& v : values()) {
        const auto& s = std::get<pluss::Series>(v);
        PLUSS_CHECK(s.scalar_type() == dtype, "Slot::to_kindle: mixed Series dtypes are not stream-compatible");
        PLUSS_CHECK(s.numel() == 1, "Slot::to_kindle: Series values must be scalar-like (numel==1)");
        out.push_back(s.to_vec<std::int64_t>()[0]);
      }
      return pluss::kindle::Stream(pluss::make_series(out, {static_cast<std::int64_t>(out.size())}));
    }

    std::vector<double> out;
    out.reserve(values().size());
    for (const auto& v : values()) {
      const auto& s = std::get<pluss::Series>(v);
      PLUSS_CHECK(s.scalar_type() == dtype, "Slot::to_kindle: mixed Series dtypes are not stream-compatible");
      PLUSS_CHECK(s.numel() == 1, "Slot::to_kindle: Series values must be scalar-like (numel==1)");
      out.push_back(s.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0]);
    }
    auto merged = pluss::make_series(out, {static_cast<std::int64_t>(out.size())});
    if (dtype != pluss::codem::ScalarType::Float64) merged = merged.to(dtype);
    return pluss::kindle::Stream(merged);
  }

  if (all_type<pluss::Note>(values())) {
    if (values().size() == 1) return pluss::kindle::Stream(std::get<pluss::Note>(values().front()));
    std::vector<std::string> out;
    out.reserve(values().size());
    for (const auto& v : values()) {
      const auto& n = std::get<pluss::Note>(v);
      PLUSS_CHECK(n.numel() == 1, "Slot::to_kindle: Note values must be scalar-like (numel==1)");
      out.emplace_back(n.at(0));
    }
    return pluss::kindle::Stream(pluss::make_note(out));
  }

  if (all_type<pluss::Timeline>(values())) {
    if (values().size() == 1) return pluss::kindle::Stream(std::get<pluss::Timeline>(values().front()));
    std::vector<std::int64_t> ticks;
    ticks.reserve(values().size());
    for (const auto& v : values()) {
      const auto& t = std::get<pluss::Timeline>(v);
      PLUSS_CHECK(t.numel() == 1, "Slot::to_kindle: Timeline values must be scalar-like (numel==1)");
      ticks.push_back(t.to_epoch_nanos_vector()[0]);
    }
    return pluss::kindle::Stream(pluss::from_epoch_nanos(
        ticks,
        std::vector<std::int64_t>{static_cast<std::int64_t>(ticks.size())}));
  }

  if (all_type<pluss::TimeIndex>(values())) {
    if (values().size() == 1) return pluss::kindle::Stream(std::get<pluss::TimeIndex>(values().front()));
    std::vector<std::int64_t> ticks;
    ticks.reserve(values().size());
    for (const auto& v : values()) {
      const auto& t = std::get<pluss::TimeIndex>(v);
      PLUSS_CHECK(t.size() == 1, "Slot::to_kindle: TimeIndex values must be scalar-like (size==1)");
      ticks.push_back(t.timeline().to_epoch_nanos_vector()[0]);
    }
    return pluss::kindle::Stream(pluss::make_timeindex(ticks, false));
  }

  throw pluss::Error("Slot::to_kindle: heterogeneous slot is not stream-compatible");
}

pluss::kindle::Stream Slot::kindelize() const {
  return to_kindle();
}

pluss::kindle::Stream Slot::as_kindle() const {
  return to_kindle();
}
