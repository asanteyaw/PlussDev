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

bool Registry::empty() const { return nslots() == 0; }

const std::vector<std::string>& Registry::keys() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().keys_;
}

bool Registry::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().name_to_pos_.find(std::string(key)) != impl_ref_().name_to_pos_.end();
}

Slot Registry::slot(std::string_view key) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto it = impl_ref_().name_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != impl_ref_().name_to_pos_.end(), "Registry: key '" + std::string(key) + "' not found");
  return impl_ref_().slots_[static_cast<std::size_t>(it->second)];
}

std::optional<Slot> Registry::get(std::string_view key) const {
  if (!defined()) return std::nullopt;
  auto it = impl_ref_().name_to_pos_.find(std::string(key));
  if (it == impl_ref_().name_to_pos_.end()) return std::nullopt;
  return impl_ref_().slots_[static_cast<std::size_t>(it->second)];
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

Registry Registry::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::unordered_map<std::string, std::string> mp;
  mp.reserve(mapping.size());
  for (const auto& [from, to] : mapping) {
    const auto f = std::string(from);
    const auto t = std::string(to);
    PLUSS_CHECK(!f.empty() && !t.empty(), "Registry::rename: keys cannot be empty");
    PLUSS_CHECK(has(f), "Registry::rename: unknown key '" + f + "'");
    mp.emplace(f, t);
  }

  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  std::unordered_set<std::string> seen;
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    auto it = mp.find(impl_ref_().keys_[i]);
    std::string key = (it == mp.end()) ? impl_ref_().keys_[i] : it->second;
    PLUSS_CHECK(seen.insert(key).second, "Registry::rename: duplicate target key '" + key + "'");
    out.emplace_back(std::move(key), impl_ref_().slots_[i]);
  }
  return Registry(std::move(out));
}

Registry Registry::apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.apply_(key, fn);
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

std::unordered_map<std::string, std::int64_t> Registry::slot_sizes() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(static_cast<std::size_t>(nslots()) * 2);
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace(impl_ref_().keys_[i], impl_ref_().slots_[i].size());
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
