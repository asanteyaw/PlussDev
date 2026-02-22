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

pluss::kindle::Stream Slot::kindelize() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (empty()) return pluss::kindle::Stream(pluss::make_note(std::vector<std::string>{}));

  if (all_type<bool>(values())) {
    std::vector<bool> b;
    b.reserve(values().size());
    for (const auto& v : values()) b.push_back(std::get<bool>(v));
    const auto n = static_cast<std::int64_t>(b.size());
    auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(s.raw_data());
    for (std::int64_t i = 0; i < n; ++i) p[i] = b[static_cast<std::size_t>(i)];
    return pluss::kindle::Stream(s);
  }
  if (all_type<std::int64_t>(values())) {
    std::vector<std::int64_t> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<std::int64_t>(v));
    return pluss::kindle::Stream(pluss::make_series(x, {static_cast<std::int64_t>(x.size())}));
  }
  if (all_type<double>(values())) {
    std::vector<double> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<double>(v));
    return pluss::kindle::Stream(pluss::make_series(x, {static_cast<std::int64_t>(x.size())}));
  }
  if (all_type<std::string>(values())) {
    std::vector<std::string> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<std::string>(v));
    return pluss::kindle::Stream(pluss::make_note(x));
  }
  if (values().size() == 1) {
    const auto& v = values().front();
    if (std::holds_alternative<pluss::Series>(v)) return pluss::kindle::Stream(std::get<pluss::Series>(v));
    if (std::holds_alternative<pluss::Note>(v)) return pluss::kindle::Stream(std::get<pluss::Note>(v));
    if (std::holds_alternative<pluss::Timeline>(v)) return pluss::kindle::Stream(std::get<pluss::Timeline>(v));
    if (std::holds_alternative<pluss::TimeIndex>(v)) return pluss::kindle::Stream(std::get<pluss::TimeIndex>(v));
  }

  PLUSS_FAIL("Slot::kindelize: heterogeneous slot is not stream-compatible");
}

