#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

#include "common/StringViewNameRange.h"
#include "MSharp/IntrusivePtr.h"
#include "kindle/catalog.h"
#include "table/plot/AtlasPlot.h"

namespace pluss::table {
class Atlas;
}
namespace pluss::scribble {
class Axel;
}

namespace pluss::kindle {
namespace detail {
class ArchiveImpl;
}

class ArchiveLabelAccessor;
class ArchiveKeyAccessor;
class ArchiveKeyAxisAccessor;
class ArchiveCastAccessor;
class ArchivePlotAccessor;   
class ArchiveTimelineAccessor;
class ArchiveTimeSeriesAccessor;

// Archive plot options speak of keys, as every Archive does: an Archive is a stack of catalogs, so what a
// depth plot draws are keys, never features. Each struct mirrors the plot it configures, with the selection
// as `keys` (all_keys, include_keys, exclude_keys) and the cap as `max_keys`.
struct ArchiveDepthComparePlotOptions {
  pluss::table::AtlasDepthStatistic statistic{pluss::table::AtlasDepthStatistic::Mean};
  bool show_grid{true};
  float line_width{1.5F};
  std::int64_t max_keys{0};
  KeySelection keys{};
};

struct ArchiveDepthFacetPlotOptions {
  pluss::table::AtlasDepthStatistic statistic{pluss::table::AtlasDepthStatistic::Mean};
  bool show_grid{true};
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  std::int64_t max_keys{0};
  KeySelection keys{};
};

// The options of Archive::plot(): the figure style, the settings each plot starts from, and `keys`, the
// selection every key-selecting plot (depth_compare, depth_facet, triplot) falls back to when it is not
// given its own.
struct ArchivePlotOptions {
  pluss::table::DataFramePlotOptions figure{};
  pluss::table::AtlasFramesPlotOptions frames{};
  pluss::table::AtlasDepthAxisOptions depth_axis{};
  pluss::table::Surface3DPlotOptions surface{};
  pluss::table::Wireframe3DPlotOptions wireframe{};
  pluss::table::Scatter3DPlotOptions cloud3{};
  pluss::table::AtlasDepthPlotOptions depth{};
  ArchiveDepthComparePlotOptions depth_compare{};
  ArchiveDepthFacetPlotOptions depth_facet{};
  pluss::table::AtlasDepthRollingPlotOptions depth_rolling{};
  pluss::pl::flash::TriplotOptions triplot{};
  KeySelection keys{};
};

class Archive {
public:
  Archive();
  explicit Archive(std::vector<Catalog> catalogs,
                   std::optional<Stream> depth_labels = std::nullopt);
  Archive(const Archive& other);
  Archive(Archive&& other) noexcept;
  Archive& operator=(const Archive& other);
  Archive& operator=(Archive&& other) noexcept;
  ~Archive();

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t depth() const;
  std::int64_t size() const;
  std::int64_t nkeys() const;
  std::int64_t minkeys() const;
  std::int64_t maxkeys() const;
  std::int64_t nrecords() const;
  std::int64_t minrecords() const;
  std::int64_t maxrecords() const;
  std::tuple<std::int64_t, std::int64_t, std::int64_t> shape() const;
  bool empty() const;

  ArchiveLabelAccessor label() const;
  Archive keys(std::span<const std::string_view> keys) const;
  Archive keys(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Archive keys(const NameRange& keys) const;
  ArchiveKeyAxisAccessor keyaxis() const;
  Catalog catalog(std::int64_t i) const;
  Catalog dloc(std::int64_t i) const;
  Catalog loc(std::string_view label) const;
  Catalog loc(std::int64_t label) const;
  Catalog loc(const pluss::Timeline& label) const;
  const std::vector<Catalog>& catalogs() const;

  Archive head(std::int64_t n = 5) const;
  Archive tail(std::int64_t n = 5) const;
  Archive slice_depth(std::int64_t start, std::int64_t stop) const;
  Archive map_catalogs(const std::function<Catalog(const Catalog&)>& fn) const;
  Catalog aggregate_depth(const std::function<Catalog(const std::vector<Catalog>&)>& fn) const;

  bool has(std::string_view key) const;
  Archive drop(std::span<const std::string_view> keys) const;
  Archive drop(std::initializer_list<std::string_view> keys) const;
  Archive drop(const std::vector<std::string>& keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Archive drop(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return drop(names); });
  }
  Archive rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Archive rename(std::string_view from, std::string_view to) const;
  Archive fillnull(double value) const;
  Archive fillnull(std::string_view value) const;
  Archive fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Archive fillnull(const pluss::Timeline& value) const;
  Archive fillnull(const pluss::Duration& value) const;
  Archive dropnull() const;
  Archive ffill() const;
  Archive bfill() const;
  Archive isnull() const;
  Archive notnull() const;
  Archive invalid() const;
  // NaN and infinity are values, not missing cells (see table/NonFinite.h): isnull / dropnull / fillnull never touch them and
  // these never touch a null.
  Archive isnan() const;
  Archive notnan() const;
  Archive isinf() const;
  Archive isposinf() const;
  Archive isneginf() const;
  Archive notinf() const;
  Archive dropnan() const;
  Archive dropinvalid() const;
  Archive dropinf() const;
  Archive dropposinf() const;
  Archive dropneginf() const;
  Archive fillnan(double value) const;
  Archive fillinvalid(double value) const;
  Archive fillinf(double value) const;
  Archive fillposinf(double value) const;
  Archive fillneginf(double value) const;
  ArchiveCastAccessor cast() const;
  ArchiveCastAccessor cast(const pluss::table::ShelfCastOptions& options) const;
  Archive cast(pluss::table::ShelfCastTarget target,
               const pluss::table::ShelfCastOptions& options = {}) const;
  ArchiveTimelineAccessor tl() const;
  ArchiveTimeSeriesAccessor ts() const;
  Archive astype(pluss::codem::ScalarType dtype) const;
  Archive distinct(DistinctOptions options = {}) const;
  Archive freq(FreqOptions options = {}) const;
  Archive ndistinct(DistinctOptions options = {}) const;

  pluss::table::Atlas tabelize(const TabelizePolicy& policy = {}) const;
  pluss::scribble::Axel scribbelize() const;

  ArchivePlotAccessor plot() const;
  ArchivePlotAccessor plot(const KeySelection& keys) const;
  ArchivePlotAccessor plot(const ArchivePlotOptions& options) const;

  std::string to_string() const;
  void show(std::ostream& os) const;

private:
  explicit Archive(pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl);
  const detail::ArchiveImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl_{nullptr};

  friend class ArchiveLabelAccessor;
  friend class ArchiveKeyAccessor;
  friend class ArchiveKeyAxisAccessor;
};

class ArchiveLabelAccessor {
public:
  explicit ArchiveLabelAccessor(Archive archive);

  bool has() const;
  Stream values() const;
  Archive set(const Stream& labels) const;
  Archive reset() const;
  Archive sort(bool ascending = true, bool nulls_last = true) const;
  Archive align(const Stream& labels) const;

private:
  Archive archive_{};
};

class ArchiveKeyAxisAccessor {
public:
  explicit ArchiveKeyAxisAccessor(Archive archive);

  Archive like(std::string_view needle, bool case_sensitive = true) const;
  Archive match(std::string_view pattern) const;
  Archive series() const;
  Archive note() const;
  Archive timeline() const;
  Archive duration() const;
  Archive prefix(std::string_view prefix) const;
  Archive suffix(std::string_view suffix) const;

private:
  Archive archive_{};
};

class ArchiveKeyAccessor {
public:
  ArchiveKeyAccessor(Archive archive, std::vector<std::string> keys);

  ArchiveCastAccessor cast() const;
  ArchiveCastAccessor cast(const pluss::table::ShelfCastOptions& options) const;
  Archive fillnull(double value) const;
  Archive fillnull(std::string_view value) const;
  Archive fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Archive fillnull(const pluss::Timeline& value) const;
  Archive fillnull(const pluss::Duration& value) const;
  Archive dropnull() const;
  Archive ffill() const;
  Archive bfill() const;
  Archive isnull() const;
  Archive notnull() const;
  Archive invalid() const;
  Archive astype(pluss::codem::ScalarType dtype) const;
  Archive distinct(DistinctOptions options = {}) const;
  Archive freq(FreqOptions options = {}) const;
  Archive ndistinct(DistinctOptions options = {}) const;

private:
  template <class Fn>
  Archive map_selected_(Fn&& fn) const;

  Archive archive_{};
  std::vector<std::string> keys_{};

  friend class ArchiveCastAccessor;
  friend class ArchiveTimelineAccessor;
  friend class ArchiveTimeSeriesAccessor;
};

class ArchiveCastAccessor {
public:
  ArchiveCastAccessor(Archive archive,
                      std::vector<std::string> keys,
                      pluss::table::ShelfCastOptions options = {});

  Archive series() const;
  Archive note() const;
  Archive timeline() const;
  Archive duration() const;

private:
  ArchiveKeyAccessor keysel_;
  pluss::table::ShelfCastOptions options_{};
};

class ArchiveTimelineAccessor {
public:
  ArchiveTimelineAccessor(Archive archive, std::vector<std::string> keys);

  Archive isnat() const;
  Archive valid() const;
  Archive year() const;
  Archive month() const;
  Archive day() const;
  Archive hour() const;
  Archive minute() const;
  Archive second() const;
  Archive weekday() const;
  Archive yearday() const;
  Archive quarter() const;
  Archive week() const;
  Archive weekend() const;
  Archive month_start() const;
  Archive month_end() const;
  Archive leap() const;
  Archive weekday_name() const;
  Archive month_name() const;
  Archive fmt(std::string_view pattern) const;
  Archive iso() const;
  Archive epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Archive floor(std::string_view unit) const;
  Archive ceil(std::string_view unit) const;
  Archive round(std::string_view unit) const;
  Archive date() const;
  Archive time() const;
  Archive shift(std::int64_t periods, std::string_view freq) const;
  Archive period(std::string_view freq) const;
  Archive tz(std::string_view name) const;
  Archive localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Archive utc() const;

private:
  ArchiveKeyAccessor keysel_;
};

class ArchiveTimeSeriesAccessor {
public:
  ArchiveTimeSeriesAccessor(Archive archive, std::vector<std::string> keys);

  Archive returns(std::int64_t periods = 1) const;
  Archive log_returns(std::int64_t periods = 1) const;
  Archive diff(std::int64_t periods = 1) const;
  Archive lag(std::int64_t periods = 1) const;
  Archive lead(std::int64_t periods = 1) const;
  Archive rolling(std::int64_t window,
                  pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                  const pluss::table::WindowOptions& options = {}) const;

private:
  ArchiveKeyAccessor keysel_;
};

// Structural 3D plotting for a *stack* of catalogs -- the kindle-layer
// counterpart to AtlasPlotAccessor. Compare to Catalog::plot3d(), which
// draws chart-level 3D plots (scatter/line/hist/surface/wireframe) from a
// single catalog's own key columns rather than visualizing the stack itself.
// Delegates through Archive::tabelize()/Atlas::plot() to reuse
// AtlasPlotAccessor's depth/frames/cloud3/triplot logic exactly rather than
// duplicating it at the kindle layer; frames(single index) goes straight to
// Archive::catalog(i) instead, since that doesn't need the whole stack
// tabelized just to look at one frame.
class ArchivePlotAccessor {
public:
  explicit ArchivePlotAccessor(Archive archive, ArchivePlotOptions options = {});

  CatalogPlotAccessor frames(std::int64_t frame_index = 0) const;
  // x first, then y: frames(indices, x_key, y_key). A single key is y against the record labels.
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices, std::string_view y_key) const;
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices,
                           std::string_view y_key,
                           const pluss::table::AtlasFramesPlotOptions& options) const;
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices,
                           std::string_view x_key,
                           std::string_view y_key) const;
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices,
                           std::string_view x_key,
                           std::string_view y_key,
                           const pluss::table::AtlasFramesPlotOptions& options) const;

  pluss::pl::Studio surface(std::string_view key) const;
  pluss::pl::Studio surface(std::string_view key, const pluss::table::Surface3DPlotOptions& options) const;
  pluss::pl::Studio wireframe(std::string_view key) const;
  pluss::pl::Studio wireframe(std::string_view key, const pluss::table::Wireframe3DPlotOptions& options) const;
  pluss::pl::Studio cloud3(std::string_view x_key, std::string_view y_key, std::string_view z_key) const;
  pluss::pl::Studio cloud3(std::string_view x_key,
                           std::string_view y_key,
                           std::string_view z_key,
                           const pluss::table::Scatter3DPlotOptions& options) const;

  pluss::pl::Studio depth(std::string_view key) const;
  pluss::pl::Studio depth(std::string_view key, const pluss::table::AtlasDepthPlotOptions& options) const;
  pluss::pl::Studio depth_compare(pluss::msharp::RangeView::Of<std::string_view> keys = {}) const;
  pluss::pl::Studio depth_compare(pluss::msharp::RangeView::Of<std::string_view> keys,
                                  const ArchiveDepthComparePlotOptions& options) const;
  pluss::pl::Studio depth_compare(const ArchiveDepthComparePlotOptions& options) const;
  pluss::pl::Studio depth_facet(pluss::msharp::RangeView::Of<std::string_view> keys = {}) const;
  pluss::pl::Studio depth_facet(pluss::msharp::RangeView::Of<std::string_view> keys,
                                const ArchiveDepthFacetPlotOptions& options) const;
  pluss::pl::Studio depth_facet(const ArchiveDepthFacetPlotOptions& options) const;
  pluss::pl::Studio depth_rolling(std::string_view key) const;
  pluss::pl::Studio depth_rolling(std::string_view key,
                                  const pluss::table::AtlasDepthRollingPlotOptions& options) const;

  pluss::pl::Studio triplot() const;
  pluss::pl::Studio triplot(const pluss::pl::flash::TriplotOptions& options) const;
  pluss::pl::Studio triplot(const KeySelection& keys) const;
  pluss::pl::Studio triplot(const KeySelection& keys, const pluss::pl::flash::TriplotOptions& options) const;
  pluss::pl::Studio triplot(pluss::msharp::RangeView::Of<std::string_view> keys) const;
  pluss::pl::Studio triplot(pluss::msharp::RangeView::Of<std::string_view> keys,
                            const pluss::pl::flash::TriplotOptions& options) const;

private:
  Archive archive_{};
  ArchivePlotOptions options_{};
};

template <class Fn>
Archive ArchiveKeyAccessor::map_selected_(Fn&& fn) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return std::forward<Fn>(fn)(catalog); });
}

template <pluss::detail::StringViewNameRange NameRange>
Archive Archive::keys(const NameRange& keys) const {
  return pluss::detail::with_name_span(keys, [&](auto names) { return this->keys(names); });
}

std::ostream& operator<<(std::ostream& os, const Archive& archive);

} // namespace pluss::kindle
