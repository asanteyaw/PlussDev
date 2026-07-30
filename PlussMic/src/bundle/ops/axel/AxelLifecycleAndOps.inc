Axel::Axel() = default;
Axel::Axel(std::vector<Registry> registries, std::optional<Slot> labels)
    : impl_(pluss::msharp::make_intrusive<detail::AxelImpl>(std::move(registries), std::move(labels))) {}
Axel::Axel(const Axel& other) = default;
Axel::Axel(Axel&& other) noexcept = default;
Axel& Axel::operator=(const Axel& other) = default;
Axel& Axel::operator=(Axel&& other) noexcept = default;
Axel::~Axel() = default;

Axel::Axel(pluss::msharp::intrusive_ptr<detail::AxelImpl> impl) : impl_(std::move(impl)) {}

bool Axel::defined() const noexcept { return static_cast<bool>(impl_); }
bool Axel::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Axel::depth() const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().registries_.size());
}

std::int64_t Axel::size() const {
  return depth();
}

bool Axel::empty() const { return depth() == 0; }

AxelFieldAccessor Axel::fields(std::span<const std::string_view> keys_sel) const {
  return AxelFieldAccessor(*this, normalize_names(keys_sel));
}

AxelFieldAccessor Axel::fields(std::initializer_list<std::string_view> keys_sel) const {
  return fields(std::span<const std::string_view>(keys_sel.begin(), keys_sel.size()));
}

AxelFieldAxisAccessor Axel::fieldaxis() const {
  return AxelFieldAxisAccessor(*this);
}

Registry Axel::registry(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  PLUSS_CHECK(i >= 0 && i < depth(), "Axel::registry: index out of bounds");
  return impl_ref_().registries_[static_cast<std::size_t>(i)];
}

const std::vector<Registry>& Axel::registries() const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  return impl_ref_().registries_;
}

const std::optional<Slot>& Axel::labels() const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  return impl_ref_().labels_;
}

Axel Axel::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(0, clipped);
}

Axel Axel::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(depth() - clipped, depth());
}

Axel Axel::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, depth());
  std::vector<Registry> out_regs;
  out_regs.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) out_regs.push_back(registry(i));

  std::optional<Slot> out_labels = std::nullopt;
  if (labels().has_value()) out_labels = labels()->slice(s, e);
  return Axel(std::move(out_regs), std::move(out_labels));
}

Axel Axel::map(const std::function<Registry(const Registry&)>& fn) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Axel::map: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = fn(registry(i));
    PLUSS_CHECK(r.defined(), "Axel::map: function returned undefined Registry");
    out.push_back(std::move(r));
  }
  return Axel(std::move(out), labels());
}

Axel Axel::select(std::span<const std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).select(keys));
  }
  return Axel(std::move(out), labels());
}

Axel Axel::select(std::initializer_list<std::string_view> keys) const {
  return select(std::span<const std::string_view>(keys.begin(), keys.size()));
}

Axel Axel::drop(std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return reg.drop(fields); });
}

Axel Axel::drop(std::initializer_list<std::string_view> fields) const {
  return drop(std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const {
  return map([&](const Registry& reg) { return reg.rename(mapping); });
}

Axel Axel::rename(std::string_view from, std::string_view to) const {
  return map([&](const Registry& reg) { return reg.rename(from, to); });
}

Axel Axel::apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Axel::apply_slot: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = registry(i);
    if (r.has(key)) {
      r = r.apply(key, fn);
    }
    out.push_back(std::move(r));
  }
  return Axel(std::move(out), labels());
}

bool Axel::has(std::string_view field) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  for (std::int64_t i = 0; i < depth(); ++i) {
    if (registry(i).has(field)) return true;
  }
  return false;
}

Axel Axel::isnull() const {
  return map([](const Registry& reg) { return reg.isnull(); });
}

Axel Axel::isnull(std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).isnull(); });
}

Axel Axel::isnull(std::initializer_list<std::string_view> fields) const {
  return isnull(std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::notnull() const {
  return map([](const Registry& reg) { return reg.notnull(); });
}

Axel Axel::notnull(std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).notnull(); });
}

Axel Axel::notnull(std::initializer_list<std::string_view> fields) const {
  return notnull(std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::invalid() const {
  return map([](const Registry& reg) { return reg.invalid(); });
}

Axel Axel::invalid(std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).invalid(); });
}

Axel Axel::invalid(std::initializer_list<std::string_view> fields) const {
  return invalid(std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::fillnull(double value) const {
  return map([&](const Registry& reg) { return reg.fillnull(value); });
}

Axel Axel::fillnull(double value, std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).fillnull(value); });
}

Axel Axel::fillnull(double value, std::initializer_list<std::string_view> fields) const {
  return fillnull(value, std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::fillnull(std::string_view value) const {
  return map([&](const Registry& reg) { return reg.fillnull(value); });
}

Axel Axel::fillnull(std::string_view value, std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).fillnull(value); });
}

Axel Axel::fillnull(std::string_view value, std::initializer_list<std::string_view> fields) const {
  return fillnull(value, std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::fillnull(const pluss::Timeline& value) const {
  return map([&](const Registry& reg) { return reg.fillnull(value); });
}

Axel Axel::fillnull(const pluss::Timeline& value, std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).fillnull(value); });
}

Axel Axel::fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> fields) const {
  return fillnull(value, std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::fillnull(const pluss::TimeIndex& value) const {
  return map([&](const Registry& reg) { return reg.fillnull(value); });
}

Axel Axel::fillnull(const pluss::TimeIndex& value, std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).fillnull(value); });
}

Axel Axel::fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> fields) const {
  return fillnull(value, std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::dropnull() const {
  return map([](const Registry& reg) { return reg.dropnull(); });
}

Axel Axel::dropnull(std::span<const std::string_view> fields) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).dropnull(); });
}

Axel Axel::dropnull(std::initializer_list<std::string_view> fields) const {
  return dropnull(std::span<const std::string_view>(fields.begin(), fields.size()));
}

Axel Axel::distinct(pluss::kindle::DistinctOptions options) const {
  return map([&](const Registry& reg) { return reg.distinct(options); });
}

Axel Axel::distinct(std::span<const std::string_view> fields,
                    pluss::kindle::DistinctOptions options) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).distinct(options); });
}

Axel Axel::distinct(std::initializer_list<std::string_view> fields,
                    pluss::kindle::DistinctOptions options) const {
  return distinct(std::span<const std::string_view>(fields.begin(), fields.size()), options);
}

Axel Axel::freq(pluss::kindle::FreqOptions options) const {
  return map([&](const Registry& reg) { return reg.freq(options); });
}

Axel Axel::freq(std::span<const std::string_view> fields,
                pluss::kindle::FreqOptions options) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).freq(options); });
}

Axel Axel::freq(std::initializer_list<std::string_view> fields,
                pluss::kindle::FreqOptions options) const {
  return freq(std::span<const std::string_view>(fields.begin(), fields.size()), options);
}

Axel Axel::ndistinct(pluss::kindle::DistinctOptions options) const {
  return map([&](const Registry& reg) { return reg.ndistinct(options); });
}

Axel Axel::ndistinct(std::span<const std::string_view> fields,
                     pluss::kindle::DistinctOptions options) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).ndistinct(options); });
}

Axel Axel::ndistinct(std::initializer_list<std::string_view> fields,
                     pluss::kindle::DistinctOptions options) const {
  return ndistinct(std::span<const std::string_view>(fields.begin(), fields.size()), options);
}

Axel Axel::cast(std::span<const std::string_view> fields,
                pluss::table::ThreadCastTarget target,
                const pluss::table::ThreadCastOptions& options) const {
  const auto names = normalize_names(fields);
  return map([&](const Registry& reg) {
    auto accessor = RegistryFieldAccessor(reg, names).cast(options);
    switch (target) {
      case pluss::table::ThreadCastTarget::Series: return accessor.series();
      case pluss::table::ThreadCastTarget::Note: return accessor.note();
      case pluss::table::ThreadCastTarget::Timeline: return accessor.timeline();
      case pluss::table::ThreadCastTarget::TimeIndex: return accessor.timeindex();
    }
    return reg;
  });
}

Axel Axel::cast(std::initializer_list<std::string_view> fields,
                pluss::table::ThreadCastTarget target,
                const pluss::table::ThreadCastOptions& options) const {
  return cast(std::span<const std::string_view>(fields.begin(), fields.size()), target, options);
}

Axel Axel::astype(std::span<const std::string_view> fields,
                  pluss::codem::ScalarType dtype) const {
  return map([&](const Registry& reg) { return RegistryFieldAccessor(reg, normalize_names(fields)).astype(dtype); });
}

Axel Axel::astype(std::initializer_list<std::string_view> fields,
                  pluss::codem::ScalarType dtype) const {
  return astype(std::span<const std::string_view>(fields.begin(), fields.size()), dtype);
}

Axel Axel::concat_depth(const Axel& other) const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  PLUSS_CHECK(other.defined(), "Axel::concat_depth: other axel undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth() + other.depth()));
  for (std::int64_t i = 0; i < depth(); ++i) out.push_back(registry(i));
  for (std::int64_t i = 0; i < other.depth(); ++i) out.push_back(other.registry(i));

  std::optional<Slot> merged_labels = std::nullopt;
  if (labels().has_value() && other.labels().has_value()) {
    merged_labels = labels()->concat(*other.labels());
  }
  return Axel(std::move(out), std::move(merged_labels));
}

pluss::kindle::Archive Axel::to_kindle() const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  std::vector<pluss::kindle::Catalog> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).to_kindle());
  }
  std::optional<pluss::kindle::Stream> depth_labels = std::nullopt;
  if (labels().has_value()) depth_labels = labels()->to_kindle();
  return pluss::kindle::Archive(std::move(out), std::move(depth_labels));
}

pluss::kindle::Archive Axel::kindelize() const {
  return to_kindle();
}

pluss::kindle::Archive Axel::as_kindle() const {
  return to_kindle();
}

Axel Axel::from_kindle(const pluss::kindle::Archive& archive) {
  PLUSS_CHECK(archive.defined(), "Axel::from_kindle: archive undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(archive.depth()));
  for (std::int64_t i = 0; i < archive.depth(); ++i) {
    out.push_back(Registry::from_kindle(archive.catalog(i)));
  }
  std::optional<Slot> labels = std::nullopt;
  if (archive.label().has()) labels = Slot::from_kindle(archive.label().values());
  return Axel(std::move(out), std::move(labels));
}
