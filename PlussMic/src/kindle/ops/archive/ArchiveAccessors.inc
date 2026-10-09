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

Archive Archive::keys(std::span<const std::string_view> keys) const {
  const auto selected = normalize_archive_keys(keys);
  return map_catalogs([&](const Catalog& catalog) { return catalog.keys(selected); });
}

Archive Archive::keys(std::initializer_list<std::string_view> keys) const {
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
    const auto labels = archive_.impl_ref_().depth_labels_->to_slate();
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

Archive ArchiveKeyAxisAccessor::duration() const {
  return archive_.map_catalogs([](const Catalog& catalog) { return catalog.keyaxis().duration(); });
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
  if (archive_.empty()) return;
  PLUSS_CHECK(!keys_.empty(), "Archive::keys: at least one key is required");
  for (const auto& key : keys_) {
    PLUSS_CHECK(archive_.has(key), "Archive::keys: unknown key '" + key + "'");
  }
}

ArchiveCastAccessor ArchiveKeyAccessor::cast() const {
  return ArchiveCastAccessor(archive_, keys_, {});
}

ArchiveCastAccessor ArchiveKeyAccessor::cast(const pluss::table::SlateCastOptions& options) const {
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

Archive ArchiveKeyAccessor::fillnull(const pluss::Duration& value) const {
  return map_selected_([&](const Catalog& catalog) {
    return catalog.keys(keys_).fillnull(value);
  });
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
                                         pluss::table::SlateCastOptions options)
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

Archive ArchiveCastAccessor::duration() const {
  return keysel_.map_selected_([&](const Catalog& catalog) {
    return catalog.keys(keysel_.keys_).cast(options_).duration();
  });
}

ArchiveTimelineAccessor::ArchiveTimelineAccessor(Archive archive, std::vector<std::string> keys)
    : keysel_(std::move(archive), std::move(keys)) {}

#define PLUSS_ARCHIVE_TL_UNARY(method) \
  Archive ArchiveTimelineAccessor::method() const { \
    return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().method(); }); \
  }

PLUSS_ARCHIVE_TL_UNARY(isnat)
PLUSS_ARCHIVE_TL_UNARY(valid)
PLUSS_ARCHIVE_TL_UNARY(year)
PLUSS_ARCHIVE_TL_UNARY(month)
PLUSS_ARCHIVE_TL_UNARY(day)
PLUSS_ARCHIVE_TL_UNARY(hour)
PLUSS_ARCHIVE_TL_UNARY(minute)
PLUSS_ARCHIVE_TL_UNARY(second)
PLUSS_ARCHIVE_TL_UNARY(weekday)
PLUSS_ARCHIVE_TL_UNARY(yearday)
PLUSS_ARCHIVE_TL_UNARY(quarter)
PLUSS_ARCHIVE_TL_UNARY(week)
PLUSS_ARCHIVE_TL_UNARY(weekend)
PLUSS_ARCHIVE_TL_UNARY(month_start)
PLUSS_ARCHIVE_TL_UNARY(month_end)
PLUSS_ARCHIVE_TL_UNARY(leap)
PLUSS_ARCHIVE_TL_UNARY(weekday_name)
PLUSS_ARCHIVE_TL_UNARY(month_name)
PLUSS_ARCHIVE_TL_UNARY(iso)
PLUSS_ARCHIVE_TL_UNARY(date)
PLUSS_ARCHIVE_TL_UNARY(time)
PLUSS_ARCHIVE_TL_UNARY(utc)

#undef PLUSS_ARCHIVE_TL_UNARY

Archive ArchiveTimelineAccessor::fmt(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().fmt(pattern); });
}

Archive ArchiveTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().epoch(unit); });
}

Archive ArchiveTimelineAccessor::floor(std::string_view unit) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().floor(unit); });
}

Archive ArchiveTimelineAccessor::ceil(std::string_view unit) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().ceil(unit); });
}

Archive ArchiveTimelineAccessor::round(std::string_view unit) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().round(unit); });
}

Archive ArchiveTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return keysel_.map_selected_(
      [&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().shift(periods, freq); });
}

Archive ArchiveTimelineAccessor::period(std::string_view freq) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().period(freq); });
}

Archive ArchiveTimelineAccessor::tz(std::string_view name) const {
  return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().tz(name); });
}

Archive ArchiveTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return keysel_.map_selected_(
      [&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).tl().localize(options); });
}


ArchiveTimeSeriesAccessor::ArchiveTimeSeriesAccessor(Archive archive, std::vector<std::string> keys)
    : keysel_(std::move(archive), std::move(keys)) {}

#define PLUSS_ARCHIVE_TS_PERIODS(method) \
  Archive ArchiveTimeSeriesAccessor::method(std::int64_t periods) const { \
    return keysel_.map_selected_([&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).ts().method(periods); }); \
  }

PLUSS_ARCHIVE_TS_PERIODS(returns)
PLUSS_ARCHIVE_TS_PERIODS(log_returns)
PLUSS_ARCHIVE_TS_PERIODS(diff)
PLUSS_ARCHIVE_TS_PERIODS(lag)
PLUSS_ARCHIVE_TS_PERIODS(lead)

#undef PLUSS_ARCHIVE_TS_PERIODS

Archive ArchiveTimeSeriesAccessor::rolling(std::int64_t window,
                                           pluss::table::WindowStatistic statistic,
                                           const pluss::table::WindowOptions& options) const {
  return keysel_.map_selected_(
      [&](const Catalog& catalog) { return catalog.keys(keysel_.keys_).ts().rolling(window, statistic, options); });
}

ArchivePlotAccessor Archive::plot() const {
  return ArchivePlotAccessor(*this);
}

ArchivePlotAccessor Archive::plot(const pluss::table::AtlasPlotOptions& options) const {
  return ArchivePlotAccessor(*this, options);
}

ArchivePlotAccessor::ArchivePlotAccessor(Archive archive, pluss::table::AtlasPlotOptions options)
    : archive_(std::move(archive)), options_(std::move(options)) {}

namespace {

std::optional<pluss::table::Slate> archive_depth_labels_as_slate(const Archive& archive) {
  if (!archive.label().has()) return std::nullopt;
  return archive.label().values().to_slate();
}

std::vector<pluss::table::DataFrame> archive_tabelized_frames(const Archive& archive) {
  const auto& catalogs = archive.catalogs();
  std::vector<pluss::table::DataFrame> frames(catalogs.size());
  std::transform(
      catalogs.begin(),
      catalogs.end(),
      frames.begin(),
      [](const Catalog& catalog) { return catalog.tabelize(); });
  return frames;
}

// Archive is the ragged-capable depth stack. Atlas is the strict cube. Plotting
// should therefore not force Archive through Atlas construction just to reuse
// the shared plot kernels, because strict Atlas validation rejects exactly the
// unequal-depth row counts Archive exists to carry.
//
// Instead, build the raw tabelized frames per catalog here and hand them
// directly to AtlasPlotAccessor, which already owns the actual rendering logic
// and now supports ragged frame lengths for surface/wireframe/cloud3/depth*
// style operations.
pluss::table::AtlasPlotAccessor archive_plot_accessor(const Archive& archive,
                                                      const pluss::table::AtlasPlotOptions& options) {
  auto frames = archive_tabelized_frames(archive);
  return pluss::table::AtlasPlotAccessor(std::move(frames), archive_depth_labels_as_slate(archive), options);
}

pluss::table::AtlasPlotAccessor archive_plot_accessor_scoped(const Archive& archive,
                                                             std::span<const std::string_view> keys,
                                                             const pluss::table::AtlasPlotOptions& options) {
  const auto scoped = archive.keys(keys);
  auto frames = archive_tabelized_frames(scoped);
  return pluss::table::AtlasPlotAccessor(std::move(frames), archive_depth_labels_as_slate(scoped), options);
}

pluss::table::AtlasPlotAccessor archive_plot_accessor_scoped(const Archive& archive,
                                                             std::initializer_list<std::string_view> keys,
                                                             const pluss::table::AtlasPlotOptions& options) {
  return archive_plot_accessor_scoped(archive, std::span<const std::string_view>(keys.begin(), keys.size()), options);
}

} // namespace

CatalogPlotAccessor ArchivePlotAccessor::frames(std::int64_t frame_index) const {
  return archive_.catalog(frame_index).plot(options_.figure);
}

pluss::pl::Studio ArchivePlotAccessor::frames(pluss::msharp::RangeView::Int frame_indices,
                                              std::string_view y_key,
                                              std::optional<std::string_view> x_key) const {
  std::vector<std::string_view> keys{y_key};
  if (x_key.has_value()) keys.push_back(*x_key);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .frames(frame_indices, y_key, x_key);
}

pluss::pl::Studio ArchivePlotAccessor::frames(pluss::msharp::RangeView::Int frame_indices,
                                              std::string_view y_key,
                                              std::optional<std::string_view> x_key,
                                              const pluss::table::AtlasFramesPlotOptions& options) const {
  std::vector<std::string_view> keys{y_key};
  if (x_key.has_value()) keys.push_back(*x_key);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .frames(frame_indices, y_key, x_key, options);
}

pluss::pl::Studio ArchivePlotAccessor::surface(std::string_view key) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).surface(key);
}

pluss::pl::Studio ArchivePlotAccessor::surface(std::string_view key,
                                               const pluss::table::Surface3DPlotOptions& options) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).surface(key, options);
}

pluss::pl::Studio ArchivePlotAccessor::wireframe(std::string_view key) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).wireframe(key);
}

pluss::pl::Studio ArchivePlotAccessor::wireframe(std::string_view key,
                                                 const pluss::table::Wireframe3DPlotOptions& options) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).wireframe(key, options);
}

pluss::pl::Studio ArchivePlotAccessor::cloud3(std::string_view x_key,
                                              std::string_view y_key,
                                              std::string_view z_key) const {
  return archive_plot_accessor_scoped(archive_, {x_key, y_key, z_key}, options_).cloud3(x_key, y_key, z_key);
}

pluss::pl::Studio ArchivePlotAccessor::cloud3(std::string_view x_key,
                                              std::string_view y_key,
                                              std::string_view z_key,
                                              const pluss::table::Scatter3DPlotOptions& options) const {
  return archive_plot_accessor_scoped(archive_, {x_key, y_key, z_key}, options_).cloud3(x_key, y_key, z_key, options);
}

pluss::pl::Studio ArchivePlotAccessor::depth(std::string_view key) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).depth(key);
}

pluss::pl::Studio ArchivePlotAccessor::depth(std::string_view key,
                                             const pluss::table::AtlasDepthPlotOptions& options) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).depth(key, options);
}

pluss::pl::Studio ArchivePlotAccessor::depth_compare(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).depth_compare(keys);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .depth_compare(keys);
}

pluss::pl::Studio ArchivePlotAccessor::depth_compare(
    pluss::msharp::RangeView::Of<std::string_view> keys,
    const pluss::table::AtlasDepthComparePlotOptions& options) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).depth_compare(keys, options);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .depth_compare(keys, options);
}

pluss::pl::Studio ArchivePlotAccessor::depth_facet(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).depth_facet(keys);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .depth_facet(keys);
}

pluss::pl::Studio ArchivePlotAccessor::depth_facet(
    pluss::msharp::RangeView::Of<std::string_view> keys,
    const pluss::table::AtlasDepthFacetPlotOptions& options) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).depth_facet(keys, options);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .depth_facet(keys, options);
}

pluss::pl::Studio ArchivePlotAccessor::depth_rolling(std::string_view key) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).depth_rolling(key);
}

pluss::pl::Studio ArchivePlotAccessor::depth_rolling(
    std::string_view key,
    const pluss::table::AtlasDepthRollingPlotOptions& options) const {
  return archive_plot_accessor_scoped(archive_, {key}, options_).depth_rolling(key, options);
}

pluss::pl::Studio ArchivePlotAccessor::triplot() const {
  return archive_plot_accessor(archive_, options_).triplot();
}

pluss::pl::Studio ArchivePlotAccessor::triplot(const pluss::pl::flash::TriplotOptions& options) const {
  return archive_plot_accessor(archive_, options_).triplot(options);
}

pluss::pl::Studio ArchivePlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).triplot(keys);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .triplot(keys);
}

pluss::pl::Studio ArchivePlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> keys,
                                               const pluss::pl::flash::TriplotOptions& options) const {
  if (keys.size() == 0) return archive_plot_accessor(archive_, options_).triplot(keys, options);
  return archive_plot_accessor_scoped(archive_, std::span<const std::string_view>(keys.data(), keys.size()), options_)
      .triplot(keys, options);
}
