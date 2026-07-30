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

Registry Registry::load(std::string_view source_type, std::string_view filepath) {
  return load(source_type, filepath, pluss::loader::Config{});
}

Registry Registry::load(std::string_view source_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config) {
  const auto source = lower_copy(trim_copy(source_type));
  if (source == "csv") {
    return load_csv(filepath, config);
  }
  if (source == "txt" || source == "text") {
    return load_txt(filepath, config);
  }
  if (source == "excel" || source == "xlsx") {
    return load_excel(filepath, config);
  }
  if (source == "parquet") {
    return load_parquet(filepath, config);
  }
  if (source == "json") {
    return load_json(filepath, config);
  }
  PLUSS_FAIL("Registry::load: unsupported source type '" + std::string(source_type) + "'");
}

Registry Registry::load_csv(std::string_view filepath) {
  return load_csv(filepath, pluss::loader::ReadCsvOptions{});
}

Registry Registry::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return registry_from_raw_batch(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

Registry Registry::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  const auto options = pluss::loader::csv_options_from_config(config);
  return registry_from_raw_batch(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

Registry Registry::load_txt(std::string_view filepath) {
  return load_txt(filepath, pluss::loader::Config{});
}

Registry Registry::load_txt(std::string_view filepath, const pluss::loader::Config& config) {
  const auto options = pluss::loader::csv_options_from_config(config);
  return registry_from_raw_batch(pluss::loader::read_txt_raw_batch(filepath, config), options);
}

Registry Registry::load_excel(std::string_view filepath) {
  return load_excel(filepath, pluss::loader::ReadExcelOptions{});
}

Registry Registry::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = options.header;
  return registry_from_raw_batch(pluss::loader::read_excel_raw_batch(filepath, options), infer_options);
}

Registry Registry::load_excel(std::string_view filepath, const pluss::loader::Config& config) {
  const auto options = pluss::loader::excel_options_from_config(config);
  pluss::loader::ReadCsvOptions infer_options = pluss::loader::csv_options_from_config(config);
  infer_options.header = options.header;
  return registry_from_raw_batch(pluss::loader::read_excel_raw_batch(filepath, options), infer_options);
}

Registry Registry::load_parquet(std::string_view filepath) {
  return load_parquet(filepath, pluss::loader::Config{});
}

Registry Registry::load_parquet(std::string_view filepath, const pluss::loader::Config& config) {
  std::ifstream probe(std::string(filepath), std::ios::in | std::ios::binary);
  if (!probe.is_open()) {
    throw pluss::Error("Registry::load_parquet: file not found '" + std::string(filepath) + "'");
  }
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return registry_from_raw_batch(pluss::loader::read_parquet_raw_batch(filepath, config), infer_options);
}

Registry Registry::load_json(std::string_view filepath) {
  return load_json(filepath, pluss::loader::Config{});
}

Registry Registry::load_json(std::string_view filepath, const pluss::loader::Config& config) {
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return registry_from_raw_batch(pluss::loader::read_json_raw_batch(filepath, config), infer_options);
}

bool Registry::defined() const noexcept { return static_cast<bool>(impl_); }
bool Registry::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Registry::nslots() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().slots_.size());
}

std::int64_t Registry::nfields() const {
  return nslots();
}

std::int64_t Registry::size() const {
  return nslots();
}

std::pair<std::int64_t, std::int64_t> Registry::shape() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::int64_t max_len = 0;
  for (const auto& slot : impl_ref_().slots_) {
    max_len = std::max(max_len, slot.size());
  }
  return {max_len, nslots()};
}

bool Registry::empty() const { return nslots() == 0; }

const std::vector<std::string>& Registry::keys() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().keys_;
}

const std::vector<std::string>& Registry::field_names() const {
  return keys();
}

const std::vector<std::string>& Registry::fields() const {
  return keys();
}

RegistryFieldAccessor Registry::fields(std::string_view key) const {
  return RegistryFieldAccessor(*this, normalize_names(std::vector<std::string>{std::string(key)}));
}

RegistryFieldAccessor Registry::fields(std::span<const std::string_view> keys_sel) const {
  return RegistryFieldAccessor(*this, normalize_names(keys_sel));
}

RegistryFieldAccessor Registry::fields(std::initializer_list<std::string_view> keys_sel) const {
  return fields(std::span<const std::string_view>(keys_sel.begin(), keys_sel.size()));
}

RegistryFieldAxisAccessor Registry::fieldaxis() const {
  return RegistryFieldAxisAccessor(*this);
}

bool Registry::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return impl_ref_().name_to_pos_.find(std::string(key)) != impl_ref_().name_to_pos_.end();
}

bool Registry::has_slot(std::string_view key) const {
  return has(key);
}

bool Registry::has_all(std::span<const std::string_view> keys_probe) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto key : keys_probe) {
    if (!has(key)) return false;
  }
  return true;
}

bool Registry::has_all(std::initializer_list<std::string_view> keys_probe) const {
  return has_all(std::span<const std::string_view>(keys_probe.begin(), keys_probe.size()));
}

bool Registry::has_any(std::span<const std::string_view> keys_probe) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  for (const auto key : keys_probe) {
    if (has(key)) return true;
  }
  return false;
}

bool Registry::has_any(std::initializer_list<std::string_view> keys_probe) const {
  return has_any(std::span<const std::string_view>(keys_probe.begin(), keys_probe.size()));
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
    if (!impl_ref_().slots_[i].is_stream_compatible()) {
      return false;
    }
  }
  return true;
}

void Registry::ensure_stream_compatible(std::string_view where) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const std::string where_name = where.empty() ? std::string("Registry") : std::string(where);
  for (std::size_t i = 0; i < impl_ref_().slots_.size(); ++i) {
    impl_ref_().slots_[i].ensure_stream_compatible(
        fmt::format("{}: slot '{}'", where_name, impl_ref_().keys_[i]));
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

Registry Registry::cloc(std::int64_t row) const {
  return slice(row, row + 1);
}

Registry Registry::cloc(std::int64_t start, std::int64_t stop) const {
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

Registry Registry::select(std::span<const std::string_view> keys_sel) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_sel);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    if (!has(n)) {
      throw pluss::Error("Registry::select: unknown key '" + n + "'");
    }
    out.emplace_back(n, slot(n));
  }
  return Registry(std::move(out));
}

Registry Registry::select(std::initializer_list<std::string_view> keys_sel) const {
  return select(std::span<const std::string_view>(keys_sel.begin(), keys_sel.size()));
}

Registry Registry::select(const std::vector<std::string>& keys_sel) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_sel);
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    if (!has(n)) {
      throw pluss::Error("Registry::select: unknown key '" + n + "'");
    }
    out.emplace_back(n, slot(n));
  }
  return Registry(std::move(out));
}

Slot Registry::select(std::string_view key) const {
  return slot(key);
}

Registry Registry::drop(std::span<const std::string_view> keys_drop) const {
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

Registry Registry::drop(std::initializer_list<std::string_view> keys_drop) const {
  return drop(std::span<const std::string_view>(keys_drop.begin(), keys_drop.size()));
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

Registry Registry::apply(std::span<const std::string_view> keys_apply,
                         const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  auto out = *this;
  out.apply_(keys_apply, fn);
  return out;
}

Registry Registry::apply(std::initializer_list<std::string_view> keys_apply,
                         const std::function<Slot(const Slot&)>& fn) const {
  return apply(std::span<const std::string_view>(keys_apply.begin(), keys_apply.size()), fn);
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

Registry& Registry::apply_(std::span<const std::string_view> keys_apply,
                           const std::function<Slot(const Slot&)>& fn) {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  const auto names = normalize_names(keys_apply);
  for (const auto& name : names) {
    apply_(name, fn);
  }
  return *this;
}

Registry& Registry::apply_(std::initializer_list<std::string_view> keys_apply,
                           const std::function<Slot(const Slot&)>& fn) {
  return apply_(std::span<const std::string_view>(keys_apply.begin(), keys_apply.size()), fn);
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

std::unordered_map<std::string, std::int64_t> Registry::sizes() const {
  return slot_sizes();
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

Registry Registry::drop_empty() const {
  return drop_empty_slots();
}

Registry& Registry::drop_empty_() {
  return drop_empty_slots_();
}

Registry Registry::isnull() const {
  return apply_all([](const Slot& s) { return s.isnull(); });
}

Registry Registry::isnull(std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::isnull(fields): fields cannot be empty; use isnull() for all fields");
  return apply(fields_sel, [](const Slot& s) { return s.isnull(); });
}

Registry Registry::isnull(std::initializer_list<std::string_view> fields_sel) const {
  return isnull(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::notnull() const {
  return apply_all([](const Slot& s) { return s.notnull(); });
}

Registry Registry::notnull(std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::notnull(fields): fields cannot be empty; use notnull() for all fields");
  return apply(fields_sel, [](const Slot& s) { return s.notnull(); });
}

Registry Registry::notnull(std::initializer_list<std::string_view> fields_sel) const {
  return notnull(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::invalid() const {
  return apply_all([](const Slot& s) { return s.invalid(); });
}

Registry Registry::invalid(std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::invalid(fields): fields cannot be empty; use invalid() for all fields");
  return apply(fields_sel, [](const Slot& s) { return s.invalid(); });
}

Registry Registry::invalid(std::initializer_list<std::string_view> fields_sel) const {
  return invalid(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::fillnull(double value) const {
  return apply_all([&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(double value, std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0,
              "Registry::fillnull(double, fields): fields cannot be empty; use fillnull(value) for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(double value, std::initializer_list<std::string_view> fields_sel) const {
  return fillnull(value, std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::fillnull(std::string_view value) const {
  return apply_all([&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(std::string_view value, std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0,
              "Registry::fillnull(string, fields): fields cannot be empty; use fillnull(value) for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(std::string_view value, std::initializer_list<std::string_view> fields_sel) const {
  return fillnull(value, std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::fillnull(const pluss::Timeline& value) const {
  return apply_all([&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(const pluss::Timeline& value, std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0,
              "Registry::fillnull(Timeline, fields): fields cannot be empty; use fillnull(value) for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> fields_sel) const {
  return fillnull(value, std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::fillnull(const pluss::TimeIndex& value) const {
  return apply_all([&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(const pluss::TimeIndex& value, std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0,
              "Registry::fillnull(TimeIndex, fields): fields cannot be empty; use fillnull(value) for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.fillnull(value); });
}

Registry Registry::fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> fields_sel) const {
  return fillnull(value, std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::dropnull() const {
  return apply_all([](const Slot& s) { return s.dropnull(); });
}

Registry Registry::dropnull(std::span<const std::string_view> fields_sel) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::dropnull(fields): fields cannot be empty; use dropnull() for all fields");
  return apply(fields_sel, [](const Slot& s) { return s.dropnull(); });
}

Registry Registry::dropnull(std::initializer_list<std::string_view> fields_sel) const {
  return dropnull(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()));
}

Registry Registry::distinct(pluss::kindle::DistinctOptions options) const {
  return apply_all([&](const Slot& s) { return s.distinct(options); });
}

Registry Registry::distinct(std::span<const std::string_view> fields_sel,
                            pluss::kindle::DistinctOptions options) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::distinct(fields): fields cannot be empty; use distinct() for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.distinct(options); });
}

Registry Registry::distinct(std::initializer_list<std::string_view> fields_sel,
                            pluss::kindle::DistinctOptions options) const {
  return distinct(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()), options);
}

Registry Registry::freq(pluss::kindle::FreqOptions options) const {
  return apply_all([&](const Slot& s) { return s.freq(options); });
}

Registry Registry::freq(std::span<const std::string_view> fields_sel,
                        pluss::kindle::FreqOptions options) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::freq(fields): fields cannot be empty; use freq() for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.freq(options); });
}

Registry Registry::freq(std::initializer_list<std::string_view> fields_sel,
                        pluss::kindle::FreqOptions options) const {
  return freq(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()), options);
}

Registry Registry::ndistinct(pluss::kindle::DistinctOptions options) const {
  return apply_all([&](const Slot& s) { return s.ndistinct(options); });
}

Registry Registry::ndistinct(std::span<const std::string_view> fields_sel,
                             pluss::kindle::DistinctOptions options) const {
  PLUSS_CHECK(fields_sel.size() > 0,
              "Registry::ndistinct(fields): fields cannot be empty; use ndistinct() for all fields");
  return apply(fields_sel, [&](const Slot& s) { return s.ndistinct(options); });
}

Registry Registry::ndistinct(std::initializer_list<std::string_view> fields_sel,
                             pluss::kindle::DistinctOptions options) const {
  return ndistinct(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()), options);
}

Registry Registry::cast(std::span<const std::string_view> fields_sel,
                        pluss::table::ThreadCastTarget target,
                        const pluss::table::ThreadCastOptions& options) const {
  PLUSS_CHECK(fields_sel.size() > 0, "Registry::cast(fields): fields cannot be empty");
  return apply(fields_sel, [&](const Slot& s) {
    switch (target) {
      case pluss::table::ThreadCastTarget::Series:
        return s.cast(options).series();
      case pluss::table::ThreadCastTarget::Note:
        return s.cast(options).note();
      case pluss::table::ThreadCastTarget::Timeline:
        return s.cast(options).timeline();
      case pluss::table::ThreadCastTarget::TimeIndex:
        return s.cast(options).timeindex();
    }
    return s;
  });
}

Registry Registry::cast(std::initializer_list<std::string_view> fields_sel,
                        pluss::table::ThreadCastTarget target,
                        const pluss::table::ThreadCastOptions& options) const {
  return cast(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()), target, options);
}

Registry Registry::astype(std::span<const std::string_view> fields_sel,
                          pluss::codem::ScalarType dtype) const {
  return cast(fields_sel,
              pluss::table::ThreadCastTarget::Series,
              pluss::table::ThreadCastOptions{.series_dtype = dtype});
}

Registry Registry::astype(std::initializer_list<std::string_view> fields_sel,
                          pluss::codem::ScalarType dtype) const {
  return astype(std::span<const std::string_view>(fields_sel.begin(), fields_sel.size()), dtype);
}

Slot Registry::sum(std::string_view field) const {
  return select(field).sum();
}

Slot Registry::mean(std::string_view field) const {
  return select(field).mean();
}

Slot Registry::min(std::string_view field) const {
  return select(field).min();
}

Slot Registry::max(std::string_view field) const {
  return select(field).max();
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

Registry Registry::set(std::string_view key, const Slot& value) const {
  return set_slot(key, value);
}

Registry& Registry::set_(std::string_view key, const Slot& value) {
  return set_slot_(key, value);
}

Registry Registry::set(std::initializer_list<std::pair<std::string_view, Slot>> items) const {
  return set_slots(items);
}

Registry& Registry::set_(std::initializer_list<std::pair<std::string_view, Slot>> items) {
  return set_slots_(items);
}

Registry Registry::set(std::initializer_list<std::pair<std::string, Slot>> items) const {
  return set_slots(items);
}

Registry& Registry::set_(std::initializer_list<std::pair<std::string, Slot>> items) {
  return set_slots_(items);
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
