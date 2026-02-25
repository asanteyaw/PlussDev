Aisle::Aisle() = default;
Aisle::Aisle(std::vector<Registry> registries, std::optional<Slot> labels)
    : impl_(pluss::msharp::make_intrusive<detail::AisleImpl>(std::move(registries), std::move(labels))) {}
Aisle::Aisle(const Aisle& other) = default;
Aisle::Aisle(Aisle&& other) noexcept = default;
Aisle& Aisle::operator=(const Aisle& other) = default;
Aisle& Aisle::operator=(Aisle&& other) noexcept = default;
Aisle::~Aisle() = default;

Aisle::Aisle(pluss::msharp::intrusive_ptr<detail::AisleImpl> impl) : impl_(std::move(impl)) {}

bool Aisle::defined() const noexcept { return static_cast<bool>(impl_); }
bool Aisle::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Aisle::depth() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().registries_.size());
}

bool Aisle::empty() const { return depth() == 0; }

Registry Aisle::registry(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(i >= 0 && i < depth(), "Aisle::registry: index out of bounds");
  return impl_ref_().registries_[static_cast<std::size_t>(i)];
}

const std::vector<Registry>& Aisle::registries() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return impl_ref_().registries_;
}

const std::optional<Slot>& Aisle::labels() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return impl_ref_().labels_;
}

Aisle Aisle::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(0, clipped);
}

Aisle Aisle::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(depth() - clipped, depth());
}

Aisle Aisle::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, depth());
  std::vector<Registry> out_regs;
  out_regs.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) out_regs.push_back(registry(i));

  std::optional<Slot> out_labels = std::nullopt;
  if (labels().has_value()) out_labels = labels()->slice(s, e);
  return Aisle(std::move(out_regs), std::move(out_labels));
}

Aisle Aisle::map(const std::function<Registry(const Registry&)>& fn) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Aisle::map: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = fn(registry(i));
    PLUSS_CHECK(r.defined(), "Aisle::map: function returned undefined Registry");
    out.push_back(std::move(r));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::select(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).select(keys));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Aisle::apply_slot: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = registry(i);
    if (r.has(key)) {
      r = r.apply(key, fn);
    }
    out.push_back(std::move(r));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::concat_depth(const Aisle& other) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(other.defined(), "Aisle::concat_depth: other aisle undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth() + other.depth()));
  for (std::int64_t i = 0; i < depth(); ++i) out.push_back(registry(i));
  for (std::int64_t i = 0; i < other.depth(); ++i) out.push_back(other.registry(i));

  std::optional<Slot> merged_labels = std::nullopt;
  if (labels().has_value() && other.labels().has_value()) {
    merged_labels = labels()->concat(*other.labels());
  }
  return Aisle(std::move(out), std::move(merged_labels));
}

pluss::kindle::Archive Aisle::to_kindle() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  std::vector<pluss::kindle::Catalog> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).to_kindle());
  }
  std::optional<pluss::kindle::Stream> depth_labels = std::nullopt;
  if (labels().has_value()) depth_labels = labels()->to_kindle();
  return pluss::kindle::Archive(std::move(out), std::move(depth_labels));
}

pluss::kindle::Archive Aisle::kindelize() const {
  return to_kindle();
}

pluss::kindle::Archive Aisle::as_kindle() const {
  return to_kindle();
}

Aisle Aisle::from_kindle(const pluss::kindle::Archive& archive) {
  PLUSS_CHECK(archive.defined(), "Aisle::from_kindle: archive undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(archive.depth()));
  for (std::int64_t i = 0; i < archive.depth(); ++i) {
    out.push_back(Registry::from_kindle(archive.catalog(i)));
  }
  std::optional<Slot> labels = std::nullopt;
  if (archive.depth_labels().has_value()) labels = Slot::from_kindle(*archive.depth_labels());
  return Aisle(std::move(out), std::move(labels));
}
