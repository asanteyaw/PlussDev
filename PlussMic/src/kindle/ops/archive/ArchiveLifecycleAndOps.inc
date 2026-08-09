namespace {

std::int64_t normalize_index(std::int64_t idx, std::int64_t size) {
  std::int64_t out = idx;
  if (out < 0) {
    out += size;
  }
  PLUSS_CHECK(out >= 0 && out < size, "Archive: index out of bounds");
  return out;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) {
    e = s;
  }
  return {s, e};
}

std::int64_t find_note_label(const Stream& labels, std::string_view key) {
  const auto note = labels.to_thread().cast().note().as_note();
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (note.at(i) == key) {
      return i;
    }
  }
  return -1;
}

std::int64_t find_int_label(const Stream& labels, std::int64_t key) {
  const auto thread = labels.to_thread();
  PLUSS_CHECK(thread.kind() == pluss::table::ThreadKind::Series,
              "Archive::loc(int64): depth labels must be numeric Series");
  const auto values = thread.as_series().to_vec<std::int64_t>();
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (values[static_cast<std::size_t>(i)] == key) {
      return i;
    }
  }
  return -1;
}

std::int64_t find_timeline_label(const Stream& labels, const pluss::Timeline& key) {
  PLUSS_CHECK(key.defined(), "Archive::loc(Timeline): key is undefined");
  PLUSS_CHECK(key.numel() == 1, "Archive::loc(Timeline): key must be scalar Timeline");
  const auto keys = labels.to_thread().cast().timeline().as_timeline().to_epoch_nanos_vector();
  const auto want = key.to_epoch_nanos_vector();
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (keys[static_cast<std::size_t>(i)] == want[0]) {
      return i;
    }
  }
  return -1;
}

std::string label_string_at(const Stream& labels, std::int64_t i) {
  const auto note = labels.to_thread().cast().note().as_note();
  return std::string(note.at(i));
}

} // namespace

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

std::int64_t Archive::size() const {
  return depth();
}

std::int64_t Archive::nkeys() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  const auto n = impl_ref_().catalogs_.front().nkeys();
  for (const auto& catalog : impl_ref_().catalogs_) {
    PLUSS_CHECK(catalog.nkeys() == n, "Archive::nkeys: catalogs have different key counts");
  }
  return n;
}

std::int64_t Archive::minkeys() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  auto out = impl_ref_().catalogs_.front().nkeys();
  for (const auto& catalog : impl_ref_().catalogs_) {
    out = std::min(out, catalog.nkeys());
  }
  return out;
}

std::int64_t Archive::maxkeys() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  auto out = impl_ref_().catalogs_.front().nkeys();
  for (const auto& catalog : impl_ref_().catalogs_) {
    out = std::max(out, catalog.nkeys());
  }
  return out;
}

std::int64_t Archive::nrecords() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  const auto n = impl_ref_().catalogs_.front().nrows();
  for (const auto& catalog : impl_ref_().catalogs_) {
    PLUSS_CHECK(catalog.nrows() == n, "Archive::nrecords: catalogs have different record counts");
  }
  return n;
}

std::int64_t Archive::minrecords() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  auto out = impl_ref_().catalogs_.front().minrows();
  for (const auto& catalog : impl_ref_().catalogs_) {
    out = std::min(out, catalog.minrows());
  }
  return out;
}

std::int64_t Archive::maxrecords() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  if (impl_ref_().catalogs_.empty()) return 0;
  auto out = impl_ref_().catalogs_.front().maxrows();
  for (const auto& catalog : impl_ref_().catalogs_) {
    out = std::max(out, catalog.maxrows());
  }
  return out;
}

std::tuple<std::int64_t, std::int64_t, std::int64_t> Archive::shape() const {
  return {depth(), maxrecords(), maxkeys()};
}

bool Archive::empty() const { return depth() == 0; }

Catalog Archive::catalog(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  const auto idx = normalize_index(i, depth());
  return impl_ref_().catalogs_[static_cast<std::size_t>(idx)];
}

Catalog Archive::dloc(std::int64_t i) const {
  return catalog(i);
}

Catalog Archive::loc(std::string_view label) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(impl_ref_().depth_labels_.has_value(), "Archive::loc(string): depth labels are not set");
  const auto pos = find_note_label(*impl_ref_().depth_labels_, label);
  PLUSS_CHECK(pos >= 0, "Archive::loc(string): label not found");
  return impl_ref_().catalogs_[static_cast<std::size_t>(pos)];
}

Catalog Archive::loc(std::int64_t label) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(impl_ref_().depth_labels_.has_value(), "Archive::loc(int64): depth labels are not set");
  const auto pos = find_int_label(*impl_ref_().depth_labels_, label);
  PLUSS_CHECK(pos >= 0, "Archive::loc(int64): label not found");
  return impl_ref_().catalogs_[static_cast<std::size_t>(pos)];
}

Catalog Archive::loc(const pluss::Timeline& label) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(impl_ref_().depth_labels_.has_value(), "Archive::loc(Timeline): depth labels are not set");
  const auto pos = find_timeline_label(*impl_ref_().depth_labels_, label);
  PLUSS_CHECK(pos >= 0, "Archive::loc(Timeline): label not found");
  return impl_ref_().catalogs_[static_cast<std::size_t>(pos)];
}

const std::vector<Catalog>& Archive::catalogs() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  return impl_ref_().catalogs_;
}

Archive Archive::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice_depth(0, clipped);
}

Archive Archive::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice_depth(depth() - clipped, depth());
}

Archive Archive::slice_depth(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, depth());
  std::vector<Catalog> out;
  out.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    out.push_back(impl_ref_().catalogs_[static_cast<std::size_t>(i)]);
  }

  std::optional<Stream> out_labels = std::nullopt;
  if (impl_ref_().depth_labels_.has_value()) {
    out_labels = impl_ref_().depth_labels_->slice(s, e);
  }
  return Archive(std::move(out), std::move(out_labels));
}

Archive Archive::map_catalogs(const std::function<Catalog(const Catalog&)>& fn) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Archive::map_catalogs: mapper function is empty");
  std::vector<Catalog> out;
  out.reserve(impl_ref_().catalogs_.size());
  for (const auto& c : impl_ref_().catalogs_) {
    out.push_back(fn(c));
  }
  return Archive(std::move(out), impl_ref_().depth_labels_);
}

Catalog Archive::aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Archive::aggregate_over_catalogs: reducer function is empty");
  return fn(impl_ref_().catalogs_);
}

bool Archive::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  for (const auto& catalog : impl_ref_().catalogs_) {
    if (!catalog.has(key)) return false;
  }
  return true;
}

Archive Archive::drop(std::span<const std::string_view> keys) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.drop(keys); });
}

Archive Archive::drop(std::initializer_list<std::string_view> keys) const {
  return drop(std::span<const std::string_view>(keys.begin(), keys.size()));
}

Archive Archive::drop(const std::vector<std::string>& keys) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.drop(keys); });
}

Archive Archive::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.rename(mapping); });
}

Archive Archive::rename(std::string_view from, std::string_view to) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.rename(from, to); });
}

Archive Archive::fillnull(double value) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.fillnull(value); });
}

Archive Archive::fillnull(std::string_view value) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.fillnull(value); });
}

Archive Archive::fillnull(const pluss::Timeline& value) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.fillnull(value); });
}

Archive Archive::fillnull(const pluss::TimeIndex& value) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.fillnull(value); });
}

Archive Archive::dropnull() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.dropnull(); });
}

Archive Archive::ffill() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.ffill(); });
}

Archive Archive::bfill() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.bfill(); });
}

Archive Archive::isnull() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.isnull(); });
}

Archive Archive::notnull() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.notnull(); });
}

Archive Archive::invalid() const {
  return map_catalogs([](const Catalog& catalog) { return catalog.invalid(); });
}

ArchiveCastAccessor Archive::cast() const {
  if (empty()) return ArchiveCastAccessor(*this, {}, {});
  return ArchiveCastAccessor(*this, catalog(0).keys(), {});
}

ArchiveCastAccessor Archive::cast(const pluss::table::ThreadCastOptions& options) const {
  if (empty()) return ArchiveCastAccessor(*this, {}, options);
  return ArchiveCastAccessor(*this, catalog(0).keys(), options);
}

Archive Archive::cast(pluss::table::ThreadCastTarget target,
                      const pluss::table::ThreadCastOptions& options) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.cast(target, options); });
}

Archive Archive::astype(pluss::codem::ScalarType dtype) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.astype(dtype); });
}

Archive Archive::distinct(DistinctOptions options) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.distinct(options); });
}

Archive Archive::freq(FreqOptions options) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.freq(options); });
}

Archive Archive::ndistinct(DistinctOptions options) const {
  return map_catalogs([&](const Catalog& catalog) { return catalog.ndistinct(options); });
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

pluss::bundle::Axel Archive::bundelize() const {
  PLUSS_CHECK(defined(), "Archive::bundelize: archive undefined");
  return pluss::bundle::Axel::from_kindle(*this);
}
