Registry::Registry() = default;
Registry::Registry(std::vector<std::pair<std::string, Slot>> fields)
    : impl_(pluss::msharp::make_intrusive<detail::RegistryImpl>(std::move(fields))) {}
Registry::Registry(std::initializer_list<std::pair<std::string, Slot>> fields)
    : Registry(std::vector<std::pair<std::string, Slot>>(fields)) {}
Registry::Registry(const Registry& other) = default;
Registry::Registry(Registry&& other) noexcept = default;
Registry& Registry::operator=(const Registry& other) = default;
Registry& Registry::operator=(Registry&& other) noexcept = default;
Registry::~Registry() = default;

Registry::Registry(pluss::msharp::intrusive_ptr<detail::RegistryImpl> impl) : impl_(std::move(impl)) {}

bool Registry::defined() const noexcept { return static_cast<bool>(impl_); }
bool Registry::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Registry::nslots() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().slots_.size());
}

std::int64_t Registry::nfields() const {
  return nslots();
}

bool Registry::empty() const { return nslots() == 0; }

const std::vector<std::string>& Registry::keys() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().keys_;
}

const std::vector<std::string>& Registry::field_names() const {
  return keys();
}

bool Registry::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().name_to_pos_.find(std::string(key)) != impl_ref_().name_to_pos_.end();
}

bool Registry::has_slot(std::string_view key) const {
  return has(key);
}

bool Registry::has_all(std::initializer_list<std::string_view> keys_probe) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto key : keys_probe) {
    if (!has(key)) return false;
  }
  return true;
}

bool Registry::has_any(std::initializer_list<std::string_view> keys_probe) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto key : keys_probe) {
    if (has(key)) return true;
  }
  return false;
}

Slot Registry::slot(std::string_view key) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto it = impl_ref_().name_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != impl_ref_().name_to_pos_.end(), "Registry: key '" + std::string(key) + "' not found");
  return impl_ref_().slots_[static_cast<std::size_t>(it->second)];
}

Slot Registry::at(std::string_view key) const {
  return slot(key);
}

std::optional<Slot> Registry::get(std::string_view key) const {
  if (!defined()) return std::nullopt;
  auto it = impl_ref_().name_to_pos_.find(std::string(key));
  if (it == impl_ref_().name_to_pos_.end()) return std::nullopt;
  return impl_ref_().slots_[static_cast<std::size_t>(it->second)];
}

std::optional<Slot> Registry::get_slot(std::string_view key) const {
  return get(key);
}

std::vector<Slot> Registry::slots() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().slots_;
}

bool Registry::is_stream_compatible() const {
  if (!defined()) return false;
  for (std::size_t i = 0; i < impl_ref_().slots_.size(); ++i) {
    try {
      (void)impl_ref_().slots_[i].kindelize();
    } catch (const std::exception&) {
      return false;
    }
  }
  return true;
}

void Registry::ensure_stream_compatible(std::string_view where) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const std::string where_name = where.empty() ? std::string("Registry") : std::string(where);
  for (std::size_t i = 0; i < impl_ref_().slots_.size(); ++i) {
    try {
      (void)impl_ref_().slots_[i].kindelize();
    } catch (const std::exception& e) {
      PLUSS_FAIL(fmt::format("{}: slot '{}' is not stream-compatible ({})",
                             where_name,
                             impl_ref_().keys_[i],
                             e.what()));
    }
  }
}

Registry Registry::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto clipped = std::max<std::int64_t>(0, n);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i].head(clipped));
  }
  return Registry(std::move(out));
}

Registry Registry::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto clipped = std::max<std::int64_t>(0, n);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i].tail(clipped));
  }
  return Registry(std::move(out));
}

Registry Registry::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& sl = impl_ref_().slots_[i];
    const auto [s, e] = normalize_slice(start, stop, sl.size());
    out.emplace_back(impl_ref_().keys_[i], sl.slice(s, e));
  }
  return Registry(std::move(out));
}

Registry Registry::iloc(std::int64_t row) const {
  return slice(row, row + 1);
}

Registry Registry::iloc(std::int64_t start, std::int64_t stop) const {
  return slice(start, stop);
}

Registry Registry::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i].take(indices));
  }
  return Registry(std::move(out));
}

Registry Registry::take(const std::vector<std::int64_t>& indices) const {
  return take(std::span<const std::int64_t>(indices.data(), indices.size()));
}

Registry Registry::sample(std::int64_t n, std::optional<std::uint64_t> random_state) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  PLUSS_CHECK(n >= 0, "Registry::sample: n must be >= 0");
  if (empty()) {
    return *this;
  }

  const auto nrows = impl_ref_().slots_.front().size();
  for (std::size_t i = 1; i < impl_ref_().slots_.size(); ++i) {
    PLUSS_CHECK(impl_ref_().slots_[i].size() == nrows,
                "Registry::sample: requires aligned slot lengths");
  }

  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows);
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows));
  std::iota(idx.begin(), idx.end(), static_cast<std::int64_t>(0));
  std::mt19937_64 rng(random_state.value_or(static_cast<std::uint64_t>(std::random_device{}())));
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(clipped));
  return take(idx);
}

Registry Registry::select(std::initializer_list<std::string_view> keys_sel) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_sel);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Registry::select: unknown key '" + n + "'");
    out.emplace_back(n, slot(n));
  }
  return Registry(std::move(out));
}

Registry Registry::select(const std::vector<std::string>& keys_sel) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_sel);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Registry::select: unknown key '" + n + "'");
    out.emplace_back(n, slot(n));
  }
  return Registry(std::move(out));
}

Registry Registry::select(std::string_view key) const {
  return select(std::vector<std::string>{std::string(key)});
}

Registry Registry::drop(std::initializer_list<std::string_view> keys_drop) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_drop);
  std::unordered_set<std::string> drop_set(names.begin(), names.end());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Registry::drop: unknown key '" + n + "'");
  }

  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (drop_set.find(impl_ref_().keys_[i]) == drop_set.end()) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i]);
    }
  }
  return Registry(std::move(out));
}

Registry Registry::drop(const std::vector<std::string>& keys_drop) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_drop);
  std::unordered_set<std::string> drop_set(names.begin(), names.end());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Registry::drop: unknown key '" + n + "'");
  }

  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (drop_set.find(impl_ref_().keys_[i]) == drop_set.end()) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i]);
    }
  }
  return Registry(std::move(out));
}

Registry Registry::drop(std::string_view key) const {
  return drop(std::vector<std::string>{std::string(key)});
}

Registry Registry::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::unordered_map<std::string, std::string> mp;
  mp.reserve(mapping.size());
  for (const auto& [from, to] : mapping) {
    const auto f = std::string(from);
    const auto t = trim_copy(to);
    PLUSS_CHECK(!f.empty() && !t.empty(), "Registry::rename: keys cannot be empty");
    PLUSS_CHECK(has(f), "Registry::rename: unknown key '" + f + "'");
    const auto inserted = mp.emplace(f, t).second;
    PLUSS_CHECK(inserted, "Registry::rename: duplicate source key in mapping");
  }
  return rename(mp);
}

Registry Registry::rename(const std::unordered_map<std::string, std::string>& mapping) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  if (mapping.empty()) return *this;

  for (const auto& [from, to] : mapping) {
    PLUSS_CHECK(has(from), "Registry::rename: unknown key '" + from + "'");
    PLUSS_CHECK(!trim_copy(to).empty(), "Registry::rename: target key cannot be empty");
  }

  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(nslots()) * 2);
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& old_key = impl_ref_().keys_[i];
    const auto it = mapping.find(old_key);
    const std::string key = it == mapping.end() ? old_key : trim_copy(it->second);
    PLUSS_CHECK(seen.insert(key).second, "Registry::rename: duplicate target key '" + key + "'");
    out.emplace_back(key, impl_ref_().slots_[i]);
  }
  return Registry(std::move(out));
}

Registry Registry::rename(std::string_view from, std::string_view to) const {
  return rename({{from, to}});
}

Registry& Registry::rename_(std::string_view from, std::string_view to) {
  *this = rename(from, to);
  return *this;
}

Registry Registry::apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.apply_(key, fn);
  return out;
}

Registry Registry::apply(std::initializer_list<std::string_view> keys_apply,
                         const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.apply_(keys_apply, fn);
  return out;
}

Registry& Registry::apply_(std::string_view key, const std::function<Slot(const Slot&)>& fn) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Registry::apply_: fn is empty");
  const auto name = std::string(key);
  PLUSS_CHECK(!name.empty(), "Registry::apply_: key cannot be empty");
  PLUSS_CHECK(has(name), "Registry::apply_: unknown key '" + name + "'");
  const auto transformed = fn(slot(name));
  PLUSS_CHECK(transformed.defined(), "Registry::apply_: function returned undefined Slot");
  return set_slot_(name, transformed);
}

Registry& Registry::apply_(std::initializer_list<std::string_view> keys_apply,
                           const std::function<Slot(const Slot&)>& fn) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_apply);
  for (const auto& name : names) {
    apply_(name, fn);
  }
  return *this;
}

Registry Registry::apply_all(const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Registry::apply_all: fn is empty");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    auto transformed = fn(impl_ref_().slots_[i]);
    PLUSS_CHECK(transformed.defined(), "Registry::apply_all: function returned undefined Slot");
    out.emplace_back(impl_ref_().keys_[i], std::move(transformed));
  }
  return Registry(std::move(out));
}

Registry Registry::map(const std::function<Slot(const Slot&)>& fn) const {
  return apply_all(fn);
}

Registry& Registry::map_(const std::function<Slot(const Slot&)>& fn) {
  *this = apply_all(fn);
  return *this;
}

std::unordered_map<std::string, std::int64_t> Registry::slot_sizes() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(static_cast<std::size_t>(nslots()) * 2);
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace(impl_ref_().keys_[i], impl_ref_().slots_[i].size());
  }
  return out;
}

std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> Registry::type_profiles() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> out;
  out.reserve(static_cast<std::size_t>(nslots()) * 2);
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace(impl_ref_().keys_[i], impl_ref_().slots_[i].type_counts());
  }
  return out;
}

Registry Registry::combine(const Registry& other, std::string suffix) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  PLUSS_CHECK(other.defined(), "Registry::combine: other registry undefined");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots() + other.nslots()));
  std::unordered_set<std::string> used;
  used.reserve(static_cast<std::size_t>(nslots() + other.nslots()) * 2);

  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    used.insert(impl_ref_().keys_[i]);
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i]);
  }

  for (std::size_t i = 0; i < other.impl_ref_().keys_.size(); ++i) {
    const auto& base = other.impl_ref_().keys_[i];
    std::string name = base;
    if (used.find(name) != used.end()) {
      name = base + suffix;
      std::size_t k = 2;
      while (used.find(name) != used.end()) {
        name = base + suffix + "_" + std::to_string(k++);
      }
    }
    used.insert(name);
    out.emplace_back(std::move(name), other.impl_ref_().slots_[i]);
  }
  return Registry(std::move(out));
}

Registry Registry::clear() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return Registry(std::vector<std::pair<std::string, Slot>>{});
}

Registry& Registry::clear_() {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto& w = mut_impl_();
  w.keys_.clear();
  w.slots_.clear();
  w.name_to_pos_.clear();
  return *this;
}

Registry Registry::drop_empty_slots() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (impl_ref_().slots_[i].size() > 0) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i]);
    }
  }
  return Registry(std::move(out));
}

Registry& Registry::drop_empty_slots_() {
  *this = drop_empty_slots();
  return *this;
}

Registry Registry::set_slot(std::string_view key, const Slot& value) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.set_slot_(key, value);
  return out;
}

Registry& Registry::set_slot_(std::string_view key, const Slot& value) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  PLUSS_CHECK(value.defined(), "Registry::set_slot_: slot undefined");
  const auto name = std::string(key);
  PLUSS_CHECK(!name.empty(), "Registry::set_slot_: key cannot be empty");

  auto& w = mut_impl_();
  auto it = w.name_to_pos_.find(name);
  if (it == w.name_to_pos_.end()) {
    const auto pos = static_cast<std::int64_t>(w.keys_.size());
    w.name_to_pos_.emplace(name, pos);
    w.keys_.push_back(name);
    w.slots_.push_back(value);
  } else {
    w.slots_[static_cast<std::size_t>(it->second)] = value;
  }
  return *this;
}

Registry Registry::set_slots(std::initializer_list<std::pair<std::string_view, Slot>> items) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.set_slots_(items);
  return out;
}

Registry& Registry::set_slots_(std::initializer_list<std::pair<std::string_view, Slot>> items) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto& [key, slot_value] : items) {
    set_slot_(key, slot_value);
  }
  return *this;
}

Registry Registry::set_slots(std::initializer_list<std::pair<std::string, Slot>> items) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.set_slots_(items);
  return out;
}

Registry& Registry::set_slots_(std::initializer_list<std::pair<std::string, Slot>> items) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto& [key, slot_value] : items) {
    set_slot_(key, slot_value);
  }
  return *this;
}

Registry Registry::push(std::string_view key, const Slot::Value& value) const {
  return apply(key, [&](const Slot& s) { return s.push(value); });
}

Registry& Registry::push_(std::string_view key, const Slot::Value& value) {
  return apply_(key, [&](const Slot& s) { return s.push(value); });
}

Registry Registry::push_all(const Slot::Value& value) const {
  return apply_all([&](const Slot& s) { return s.push(value); });
}

Registry& Registry::push_all_(const Slot::Value& value) {
  *this = push_all(value);
  return *this;
}

Registry Registry::filter_values(std::string_view key,
                                 const std::function<bool(const Slot::Value&)>& predicate) const {
  PLUSS_CHECK(static_cast<bool>(predicate), "Registry::filter_values: predicate is empty");
  return apply(key, [&](const Slot& s) { return s.filter(predicate); });
}

Registry& Registry::filter_values_(std::string_view key,
                                   const std::function<bool(const Slot::Value&)>& predicate) {
  PLUSS_CHECK(static_cast<bool>(predicate), "Registry::filter_values_: predicate is empty");
  return apply_(key, [&](const Slot& s) { return s.filter(predicate); });
}

Registry Registry::transform_values(std::string_view key,
                                    const std::function<Slot::Value(const Slot::Value&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Registry::transform_values: fn is empty");
  return apply(key, [&](const Slot& s) { return s.transform(fn); });
}

Registry& Registry::transform_values_(std::string_view key,
                                      const std::function<Slot::Value(const Slot::Value&)>& fn) {
  PLUSS_CHECK(static_cast<bool>(fn), "Registry::transform_values_: fn is empty");
  return apply_(key, [&](const Slot& s) { return s.transform(fn); });
}

Registry Registry::take_values(std::string_view key, std::span<const std::int64_t> indices) const {
  return apply(key, [&](const Slot& s) { return s.take(indices); });
}

Registry& Registry::take_values_(std::string_view key, std::span<const std::int64_t> indices) {
  return apply_(key, [&](const Slot& s) { return s.take(indices); });
}

Registry Registry::concat_values(std::string_view key, const Slot& other) const {
  PLUSS_CHECK(other.defined(), "Registry::concat_values: other slot undefined");
  return apply(key, [&](const Slot& s) { return s.concat(other); });
}

Registry& Registry::concat_values_(std::string_view key, const Slot& other) {
  PLUSS_CHECK(other.defined(), "Registry::concat_values_: other slot undefined");
  return apply_(key, [&](const Slot& s) { return s.concat(other); });
}

Registry Registry::series() const {
  return bridge_registry_catalog(*this, "Registry::series", [](const pluss::kindle::Catalog& c) { return c.series(); });
}

Registry Registry::note() const {
  return bridge_registry_catalog(*this, "Registry::note", [](const pluss::kindle::Catalog& c) { return c.note(); });
}

Registry Registry::timeline() const {
  return bridge_registry_catalog(*this, "Registry::timeline", [](const pluss::kindle::Catalog& c) { return c.timeline(); });
}

Registry Registry::timeindex() const {
  return bridge_registry_catalog(*this, "Registry::timeindex", [](const pluss::kindle::Catalog& c) { return c.timeindex(); });
}

Registry Registry::sort(std::string_view by_field, bool ascending) const {
  const auto key = trim_copy(by_field);
  PLUSS_CHECK(!key.empty(), "Registry::sort: by_field cannot be empty");
  return bridge_registry_catalog(
      *this,
      "Registry::sort",
      [&](const pluss::kindle::Catalog& c) { return c.sort(key, ascending); });
}

Registry Registry::sort_by(const std::vector<std::string>& fields,
                           const std::vector<bool>& ascending) const {
  return bridge_registry_catalog(
      *this,
      "Registry::sort_by",
      [&](const pluss::kindle::Catalog& c) { return c.sort_by(fields, ascending); });
}

std::unordered_map<std::string, Registry> Registry::group_by(std::string_view key_field) const {
  ensure_stream_compatible("Registry::group_by");
  const auto groups = kindelize().group_by(key_field);
  std::unordered_map<std::string, Registry> out;
  out.reserve(groups.size());
  for (const auto& [group, cat] : groups) {
    out.emplace(group, Registry::from_kindle(cat));
  }
  return out;
}

Registry Registry::fillna(double value, std::initializer_list<std::string_view> keys_fill) const {
  return bridge_registry_catalog(
      *this,
      "Registry::fillna",
      [&](const pluss::kindle::Catalog& c) { return c.fillna(value, keys_fill); });
}

Registry Registry::dropna(std::initializer_list<std::string_view> keys_drop) const {
  return bridge_registry_catalog(
      *this,
      "Registry::dropna",
      [&](const pluss::kindle::Catalog& c) { return c.dropna(keys_drop); });
}

Registry Registry::ffill(std::initializer_list<std::string_view> keys_fill) const {
  return bridge_registry_catalog(
      *this,
      "Registry::ffill",
      [&](const pluss::kindle::Catalog& c) { return c.ffill(keys_fill); });
}

Registry Registry::bfill(std::initializer_list<std::string_view> keys_fill) const {
  return bridge_registry_catalog(
      *this,
      "Registry::bfill",
      [&](const pluss::kindle::Catalog& c) { return c.bfill(keys_fill); });
}

Registry Registry::isnull(std::initializer_list<std::string_view> keys_probe) const {
  return bridge_registry_catalog(
      *this,
      "Registry::isnull",
      [&](const pluss::kindle::Catalog& c) { return c.isnull(keys_probe); });
}

Registry Registry::notnull(std::initializer_list<std::string_view> keys_probe) const {
  return bridge_registry_catalog(
      *this,
      "Registry::notnull",
      [&](const pluss::kindle::Catalog& c) { return c.notnull(keys_probe); });
}

Registry Registry::cast_series(std::initializer_list<std::string_view> keys_cast,
                               std::optional<pluss::codem::ScalarType> dtype) const {
  return bridge_registry_catalog(
      *this,
      "Registry::cast_series",
      [&](const pluss::kindle::Catalog& c) { return c.cast_series(keys_cast, dtype); });
}

Registry Registry::cast_note(std::initializer_list<std::string_view> keys_cast) const {
  return bridge_registry_catalog(
      *this,
      "Registry::cast_note",
      [&](const pluss::kindle::Catalog& c) { return c.cast_note(keys_cast); });
}

Registry Registry::cast_timeline(std::initializer_list<std::string_view> keys_cast) const {
  return bridge_registry_catalog(
      *this,
      "Registry::cast_timeline",
      [&](const pluss::kindle::Catalog& c) { return c.cast_timeline(keys_cast); });
}

Registry Registry::cast_timeindex(std::initializer_list<std::string_view> keys_cast) const {
  return bridge_registry_catalog(
      *this,
      "Registry::cast_timeindex",
      [&](const pluss::kindle::Catalog& c) { return c.cast_timeindex(keys_cast); });
}

Registry Registry::astype(std::initializer_list<std::string_view> keys_cast,
                          pluss::codem::ScalarType dtype) const {
  return bridge_registry_catalog(
      *this,
      "Registry::astype",
      [&](const pluss::kindle::Catalog& c) { return c.astype(keys_cast, dtype); });
}

Slot Registry::sum(std::string_view field) const {
  ensure_stream_compatible("Registry::sum");
  return Slot::from_kindle(kindelize().sum(field));
}

Slot Registry::mean(std::string_view field) const {
  ensure_stream_compatible("Registry::mean");
  return Slot::from_kindle(kindelize().mean(field));
}

Slot Registry::min(std::string_view field) const {
  ensure_stream_compatible("Registry::min");
  return Slot::from_kindle(kindelize().min(field));
}

Slot Registry::max(std::string_view field) const {
  ensure_stream_compatible("Registry::max");
  return Slot::from_kindle(kindelize().max(field));
}

std::int64_t Registry::count() const {
  ensure_stream_compatible("Registry::count");
  return kindelize().count();
}

Registry Registry::merge(const Registry& other, std::string_view on, std::string_view how) const {
  PLUSS_CHECK(other.defined(), "Registry::merge: other registry undefined");
  ensure_stream_compatible("Registry::merge");
  other.ensure_stream_compatible("Registry::merge");
  return Registry::from_kindle(kindelize().merge(other.kindelize(), on, how));
}

Registry Registry::text_preprocess(std::string_view field) const {
  return bridge_registry_catalog(
      *this,
      "Registry::text_preprocess",
      [&](const pluss::kindle::Catalog& c) { return c.text_preprocess(field); });
}

Registry Registry::extract_keywords(std::string_view field, int top_n) const {
  return bridge_registry_catalog(
      *this,
      "Registry::extract_keywords",
      [&](const pluss::kindle::Catalog& c) { return c.extract_keywords(field, top_n); });
}

Registry Registry::compute_similarity(std::string_view field1,
                                      std::string_view field2,
                                      std::string_view method) const {
  const auto method_norm = lowercase_copy(trim_copy(method));
  PLUSS_CHECK(method_norm == "cosine", "Registry::compute_similarity currently supports only method='cosine'");
  return bridge_registry_catalog(
      *this,
      "Registry::compute_similarity",
      [&](const pluss::kindle::Catalog& c) { return c.compute_similarity(field1, field2, method_norm); });
}

Registry Registry::filter_by_text(std::string_view field, std::string_view pattern) const {
  return bridge_registry_catalog(
      *this,
      "Registry::filter_by_text",
      [&](const pluss::kindle::Catalog& c) { return c.filter_by_text(field, pattern); });
}

Registry Registry::add_text_features(std::string_view field) const {
  return bridge_registry_catalog(
      *this,
      "Registry::add_text_features",
      [&](const pluss::kindle::Catalog& c) { return c.add_text_features(field); });
}

Registry Registry::aggregate_text_stats(std::string_view field, std::string_view stat_type) const {
  return bridge_registry_catalog(
      *this,
      "Registry::aggregate_text_stats",
      [&](const pluss::kindle::Catalog& c) { return c.aggregate_text_stats(field, stat_type); });
}

Registry Registry::tfidf(std::string_view field) const {
  return bridge_registry_catalog(
      *this,
      "Registry::tfidf",
      [&](const pluss::kindle::Catalog& c) { return c.tfidf(field); });
}
