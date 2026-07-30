namespace {

std::vector<std::string> normalize_archive_keys(std::span<const std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (const auto key_sv : keys) {
    std::string key(key_sv);
    PLUSS_CHECK(!key.empty(), "Archive::keys: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Archive::keys: duplicate key '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

} // namespace

ArchiveLabelAccessor Archive::label() const {
  return ArchiveLabelAccessor(*this);
}

ArchiveKeyAccessor Archive::keys(std::span<const std::string_view> keys) const {
  return ArchiveKeyAccessor(*this, normalize_archive_keys(keys));
}

ArchiveKeyAccessor Archive::keys(std::initializer_list<std::string_view> keys) const {
  return this->keys(std::span<const std::string_view>(keys.begin(), keys.size()));
}

ArchiveKeyAxisAccessor Archive::keyaxis() const {
  return ArchiveKeyAxisAccessor(*this);
}

ArchiveLabelAccessor::ArchiveLabelAccessor(Archive archive) : archive_(std::move(archive)) {}

bool ArchiveLabelAccessor::has() const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  return archive_.impl_ref_().depth_labels_.has_value();
}

Stream ArchiveLabelAccessor::values() const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  PLUSS_CHECK(archive_.impl_ref_().depth_labels_.has_value(), "Archive::label().values: depth labels are not set");
  return *archive_.impl_ref_().depth_labels_;
}

Archive ArchiveLabelAccessor::set(const Stream& labels) const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  return Archive(archive_.impl_ref_().catalogs_, labels);
}

Archive ArchiveLabelAccessor::reset() const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  return Archive(archive_.impl_ref_().catalogs_, std::nullopt);
}

Archive ArchiveLabelAccessor::sort(bool ascending, bool nulls_last) const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  std::vector<std::int64_t> idx(static_cast<std::size_t>(archive_.depth()));
  std::iota(idx.begin(), idx.end(), 0);

  if (archive_.impl_ref_().depth_labels_.has_value()) {
    const auto labels = archive_.impl_ref_().depth_labels_->to_thread();
    std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
      const auto cmp = pluss::table::detail::compare_rows(labels, a, b, nulls_last);
      return ascending ? (cmp < 0) : (cmp > 0);
    });
  } else if (!ascending) {
    std::reverse(idx.begin(), idx.end());
  }

  std::vector<Catalog> out;
  out.reserve(idx.size());
  for (const auto i : idx) out.push_back(archive_.impl_ref_().catalogs_[static_cast<std::size_t>(i)]);

  std::optional<Stream> out_labels = std::nullopt;
  if (archive_.impl_ref_().depth_labels_.has_value()) {
    out_labels = archive_.impl_ref_().depth_labels_->take(idx);
  }
  return Archive(std::move(out), std::move(out_labels));
}

Archive ArchiveLabelAccessor::align(const Stream& labels) const {
  PLUSS_CHECK(archive_.defined(), "Archive::label: undefined handle");
  PLUSS_CHECK(archive_.impl_ref_().depth_labels_.has_value(),
              "Archive::label().align: current archive has no depth labels");
  PLUSS_CHECK(labels.defined(), "Archive::label().align: labels are undefined");

  std::unordered_map<std::string, std::int64_t> pos;
  pos.reserve(static_cast<std::size_t>(archive_.depth()));
  for (std::int64_t i = 0; i < archive_.depth(); ++i) {
    const auto key = label_string_at(*archive_.impl_ref_().depth_labels_, i);
    const auto inserted = pos.emplace(key, i).second;
    PLUSS_CHECK(inserted, "Archive::label().align: duplicate current depth label '" + key + "'");
  }

  std::vector<Catalog> out;
  out.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    const auto key = label_string_at(labels, i);
    const auto it = pos.find(key);
    PLUSS_CHECK(it != pos.end(), "Archive::label().align: label not found '" + key + "'");
    out.push_back(archive_.impl_ref_().catalogs_[static_cast<std::size_t>(it->second)]);
  }
  return Archive(std::move(out), labels);
}

ArchiveKeyAxisAccessor::ArchiveKeyAxisAccessor(Archive archive) : archive_(std::move(archive)) {}

Archive ArchiveKeyAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return catalog.keyaxis().like(needle, case_sensitive); });
}

Archive ArchiveKeyAxisAccessor::match(std::string_view pattern) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return catalog.keyaxis().match(pattern); });
}

Archive ArchiveKeyAxisAccessor::series() const {
  return archive_.map_catalogs([](const Catalog& catalog) { return catalog.keyaxis().series(); });
}

Archive ArchiveKeyAxisAccessor::note() const {
  return archive_.map_catalogs([](const Catalog& catalog) { return catalog.keyaxis().note(); });
}

Archive ArchiveKeyAxisAccessor::timeline() const {
  return archive_.map_catalogs([](const Catalog& catalog) { return catalog.keyaxis().timeline(); });
}

Archive ArchiveKeyAxisAccessor::timeindex() const {
  return archive_.map_catalogs([](const Catalog& catalog) { return catalog.keyaxis().timeindex(); });
}

Archive ArchiveKeyAxisAccessor::prefix(std::string_view prefix) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return catalog.keyaxis().prefix(prefix); });
}

Archive ArchiveKeyAxisAccessor::suffix(std::string_view suffix) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return catalog.keyaxis().suffix(suffix); });
}

ArchiveKeyAccessor::ArchiveKeyAccessor(Archive archive, std::vector<std::string> keys)
    : archive_(std::move(archive)), keys_(std::move(keys)) {
  PLUSS_CHECK(archive_.defined(), "Archive::keys: undefined handle");
  PLUSS_CHECK(!keys_.empty(), "Archive::keys: at least one key is required");
  for (const auto& key : keys_) {
    PLUSS_CHECK(archive_.has(key), "Archive::keys: unknown key '" + key + "'");
  }
}

ArchiveCastAccessor ArchiveKeyAccessor::cast() const {
  return ArchiveCastAccessor(archive_, keys_, {});
}

ArchiveCastAccessor ArchiveKeyAccessor::cast(const pluss::table::ThreadCastOptions& options) const {
  return ArchiveCastAccessor(archive_, keys_, options);
}

Archive ArchiveKeyAccessor::fillnull(double value) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).fillnull(value); });
}

Archive ArchiveKeyAccessor::fillnull(std::string_view value) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).fillnull(value); });
}

Archive ArchiveKeyAccessor::fillnull(const pluss::Timeline& value) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).fillnull(value); });
}

Archive ArchiveKeyAccessor::fillnull(const pluss::TimeIndex& value) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).fillnull(value); });
}

Archive ArchiveKeyAccessor::dropnull() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).dropnull(); });
}

Archive ArchiveKeyAccessor::ffill() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).ffill(); });
}

Archive ArchiveKeyAccessor::bfill() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).bfill(); });
}

Archive ArchiveKeyAccessor::isnull() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).isnull(); });
}

Archive ArchiveKeyAccessor::notnull() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).notnull(); });
}

Archive ArchiveKeyAccessor::invalid() const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).invalid(); });
}

Archive ArchiveKeyAccessor::astype(pluss::codem::ScalarType dtype) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).astype(dtype); });
}

Archive ArchiveKeyAccessor::distinct(DistinctOptions options) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).distinct(options); });
}

Archive ArchiveKeyAccessor::freq(FreqOptions options) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).freq(options); });
}

Archive ArchiveKeyAccessor::ndistinct(DistinctOptions options) const {
  return map_selected_([&](const Catalog& catalog) { return catalog.keys(keys_).ndistinct(options); });
}

ArchiveCastAccessor::ArchiveCastAccessor(Archive archive,
                                         std::vector<std::string> keys,
                                         pluss::table::ThreadCastOptions options)
    : keysel_(std::move(archive), std::move(keys)), options_(std::move(options)) {}

Archive ArchiveCastAccessor::series() const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).cast(options_).series(); });
}

Archive ArchiveCastAccessor::note() const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).cast(options_).note(); });
}

Archive ArchiveCastAccessor::timeline() const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).cast(options_).timeline(); });
}

Archive ArchiveCastAccessor::timeindex() const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).cast(options_).timeindex(); });
}
