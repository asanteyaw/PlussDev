#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"
namespace pluss::kindle {
class Archive;
struct KindelizePolicy;
struct TabelizePolicy;
}
namespace pluss::scribble {
class Axel;
}

namespace pluss::table {

namespace detail {
class AtlasImpl;
}

struct AtlasPlotOptions;
class AtlasTimelineAccessor;
class AtlasTsAccessor;
class AtlasTsResampler;
class AtlasLabelAccessor;

class Atlas {
public:
  Atlas();
  explicit Atlas(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels = std::nullopt);
  Atlas(const Atlas& other);
  Atlas(Atlas&& other) noexcept;
  Atlas& operator=(const Atlas& other);
  Atlas& operator=(Atlas&& other) noexcept;
  ~Atlas();

  bool defined() const noexcept;
  bool unique() const noexcept;
  bool empty() const;

  std::int64_t depth() const;
  std::int64_t ncases() const;
  std::int64_t nfeatures() const;

  const std::vector<std::string>& column_names() const;
  const std::optional<Thread>& axis0_labels() const;
  const std::vector<DataFrame>& frames() const;

  DataFrame frame(std::int64_t index) const;
  DataFrame dloc(std::int64_t index) const;
  Atlas features(std::string_view feature) const;
  Atlas features(std::initializer_list<std::string_view> features) const;
  Atlas features(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  Atlas features(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return this->features(names); });
  }
  AtlasLabelAccessor label() const;
  AtlasPlotAccessor plot() const;
  AtlasPlotAccessor plot(const AtlasPlotOptions& options) const;
  AtlasTsAccessor ts() const;
  AtlasTimelineAccessor tl() const;
  DataFrame squeeze() const;

  DataFrame loc(std::string_view label) const;
  DataFrame loc(std::int64_t label) const;
  DataFrame loc(const pluss::Timeline& label) const;

  Atlas head(std::int64_t n = 5) const;
  Atlas tail(std::int64_t n = 5) const;
  Atlas slice_frames(std::int64_t start, std::int64_t stop) const;
  Atlas sort_depth(bool ascending = true, bool nulls_last = true) const;
  Atlas relabel_depth(const Thread& labels) const;
  Atlas align_depth(const Thread& labels) const;

  Atlas map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const;
  Atlas map(
      const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Atlas flow(const std::function<Atlas(const Atlas&)>& fn) const;
  DataFrame aggregate_over_frames(const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) const;
  pluss::kindle::Archive kindelize() const;
  pluss::kindle::Archive kindelize(const pluss::kindle::KindelizePolicy& policy) const;
  pluss::scribble::Axel scribbelize() const;
  pluss::scribble::Axel scribbelize(const pluss::kindle::KindelizePolicy& policy) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

private:
  friend class AtlasLabelAccessor;

  explicit Atlas(pluss::msharp::intrusive_ptr<detail::AtlasImpl> impl);

  const detail::AtlasImpl& impl_ref_() const;
  pluss::msharp::intrusive_ptr<detail::AtlasImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Atlas& atlas);
void output_render(std::ostream& os, const Atlas& atlas);

class AtlasLabelAccessor {
public:
  explicit AtlasLabelAccessor(Atlas atlas);

  Thread values() const;
  bool has() const;
  Atlas set(const Thread& labels) const;
  Atlas range() const;
  Atlas reset() const;
  Atlas sort(bool ascending = true, bool nulls_last = true) const;
  Atlas align(const Thread& labels) const;
  Thread unite(const Thread& other) const;
  Thread intersect(const Thread& other) const;
  Thread diff(const Thread& other) const;
  Thread symdiff(const Thread& other) const;
  Thread find(const Thread& targets) const;
  Thread pos(double value) const;
  Thread pos(std::string_view value) const;
  Thread pos(const char* value) const {
    return pos(std::string_view{value != nullptr ? value : ""});
  }
  Thread slice(std::string_view start, std::string_view stop) const;

private:
  Thread label_index_() const;

  Atlas atlas_{};
};

class AtlasTimelineAccessor {
public:
  AtlasTimelineAccessor(Atlas atlas, std::vector<std::string> features);

  Atlas isnat() const;
  Atlas valid() const;
  Atlas year() const;
  Atlas month() const;
  Atlas day() const;
  Atlas hour() const;
  Atlas minute() const;
  Atlas second() const;
  Atlas weekday() const;
  Atlas yearday() const;
  Atlas quarter() const;
  Atlas week() const;
  Atlas weekend() const;
  Atlas month_start() const;
  Atlas month_end() const;
  Atlas leap() const;
  Atlas weekday_name() const;
  Atlas month_name() const;
  Atlas fmt(std::string_view pattern) const;
  Atlas iso() const;
  Atlas epoch(TimeUnit unit = TimeUnit::Nanos) const;
  Atlas span(const Thread& other, const TimelineSpanOptions& options = {}) const;
  Atlas span(const Thread& other, TimeUnit unit) const;
  Atlas span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const;
  Atlas span(const pluss::Timeline& other, TimeUnit unit) const;
  Atlas floor(std::string_view unit) const;
  Atlas ceil(std::string_view unit) const;
  Atlas round(std::string_view unit) const;
  Atlas date() const;
  Atlas time() const;
  Atlas shift(std::int64_t periods, std::string_view freq) const;
  Atlas tz(std::string_view name) const;
  Atlas localize(const TimelineLocalizeOptions& options = {}) const;
  Atlas utc() const;

private:
  Atlas map_(const std::function<DataFrame(const DataFrame&)>& fn, std::string_view op) const;

  Atlas atlas_{};
  std::vector<std::string> features_{};
};

class AtlasTsAccessor {
public:
  AtlasTsAccessor(Atlas atlas, std::vector<std::string> features);

  Atlas returns(std::int64_t periods = 1) const;
  Atlas log_returns(std::int64_t periods = 1) const;
  Atlas diff(std::int64_t periods = 1) const;
  Atlas lag(std::int64_t periods = 1) const;
  Atlas lead(std::int64_t periods = 1) const;
  Atlas rolling(std::int64_t window,
                WindowStatistic statistic = WindowStatistic::Mean,
                const WindowOptions& options = {}) const;
  Atlas rolling(std::string_view duration,
                WindowStatistic statistic = WindowStatistic::Mean,
                const StatisticOptions& options = {},
                WindowClosure closure = WindowClosure::Right) const;
  Atlas expanding(WindowStatistic statistic = WindowStatistic::Mean,
                  const StatisticOptions& options = {}) const;
  Atlas ewm(double alpha,
            WindowStatistic statistic = WindowStatistic::Mean,
            bool adjusted = true,
            bool bias_corrected = true,
            std::int64_t min_count = 1,
            bool ignore_nulls = false) const;
  Atlas fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                      pluss::subsp::stats::ts::GapFillMode::Forward,
                  std::optional<double> constant = std::nullopt) const;
  Atlas sort(bool ascending = true, bool nulls_last = true) const;
  AtlasTsResampler resample(std::string_view freq, std::int64_t step = 1) const;
  Atlas asfreq(std::string_view freq,
               std::int64_t step = 1,
               pluss::subsp::stats::ts::GapFillMode fill_mode =
                   pluss::subsp::stats::ts::GapFillMode::Forward,
               std::optional<double> constant = std::nullopt,
               pluss::subsp::stats::ts::ResampleOp op =
                   pluss::subsp::stats::ts::ResampleOp::Last) const;

private:
  Atlas map_(const std::function<DataFrame(const DataFrame&)>& fn, std::string_view op) const;

  Atlas atlas_{};
  std::vector<std::string> features_{};
};

class AtlasTsResampler {
public:
  AtlasTsResampler(Atlas atlas, std::vector<std::string> features, std::string freq, std::int64_t step);

  Atlas mean() const;
  Atlas sum() const;
  Atlas min() const;
  Atlas max() const;
  Atlas first() const;
  Atlas last() const;
  Atlas count() const;
  Atlas asfreq(std::optional<double> fill_value = std::nullopt) const;
  Atlas ffill() const;
  Atlas bfill() const;
  Atlas nearest() const;

private:
  Atlas aggregate_(pluss::subsp::stats::ts::ResampleOp op) const;

  Atlas atlas_{};
  std::vector<std::string> features_{};
  std::string freq_{};
  std::int64_t step_{1};
};

} // namespace pluss::table

#include "table/plot/AtlasPlot.h"
