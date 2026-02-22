Archive::Archive() = default;
Archive::Archive(std::vector<Catalog> catalogs, std::optional<Stream> depth_labels)
    : impl_(pluss::msharp::make_intrusive<detail::ArchiveImpl>(std::move(catalogs), std::move(depth_labels))) {}
Archive::Archive(const Archive& other) = default;
Archive::Archive(Archive&& other) noexcept = default;
Archive& Archive::operator=(const Archive& other) = default;
Archive& Archive::operator=(Archive&& other) noexcept = default;
Archive::~Archive() = default;

Archive::Archive(pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl) : impl_(std::move(impl)) {}

Archive Archive::kindelize(const pluss::table::Atlas& atlas, const KindelizePolicy& policy) {
  PLUSS_CHECK(atlas.defined(), "Archive::kindelize requires a defined Atlas");
  std::vector<Catalog> catalogs;
  catalogs.reserve(static_cast<std::size_t>(atlas.depth()));
  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    catalogs.push_back(Catalog::kindelize(atlas.frame(i), policy));
  }

  std::optional<Stream> depth_labels = std::nullopt;
  if (atlas.axis0_labels().has_value()) {
    depth_labels = Stream::from_thread(*atlas.axis0_labels());
  }
  return Archive(std::move(catalogs), std::move(depth_labels));
}

bool Archive::defined() const noexcept { return static_cast<bool>(impl_); }
bool Archive::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Archive::depth() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().catalogs_.size());
}

bool Archive::empty() const { return depth() == 0; }

const std::optional<Stream>& Archive::depth_labels() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  return impl_ref_().depth_labels_;
}

Catalog Archive::catalog(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(i >= 0 && i < depth(), "Archive::catalog index out of range");
  return impl_ref_().catalogs_[static_cast<std::size_t>(i)];
}

const std::vector<Catalog>& Archive::catalogs() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  return impl_ref_().catalogs_;
}

pluss::table::Atlas Archive::tabelize(const TabelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  std::vector<pluss::table::DataFrame> frames;
  frames.reserve(impl_ref_().catalogs_.size());
  for (const auto& c : impl_ref_().catalogs_) {
    frames.push_back(c.tabelize(policy));
  }

  std::optional<pluss::table::Thread> labels = std::nullopt;
  if (impl_ref_().depth_labels_.has_value()) {
    labels = impl_ref_().depth_labels_->to_thread();
  }
  return pluss::table::Atlas(std::move(frames), std::move(labels));
}

