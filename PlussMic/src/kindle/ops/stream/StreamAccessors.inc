StreamCastAccessor Stream::cast() const {
  return StreamCastAccessor(*this, {});
}

StreamCastAccessor Stream::cast(const pluss::table::ThreadCastOptions& options) const {
  return StreamCastAccessor(*this, options);
}

Stream Stream::cast(pluss::table::ThreadCastTarget target,
                    const pluss::table::ThreadCastOptions& options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return thread_accessor_stream_result(*this, [&](const pluss::table::Thread& t) {
    switch (target) {
      case pluss::table::ThreadCastTarget::Series:
        return t.cast(options).series();
      case pluss::table::ThreadCastTarget::Note:
        return t.cast(options).note();
      case pluss::table::ThreadCastTarget::Timeline:
        return t.cast(options).timeline();
      case pluss::table::ThreadCastTarget::Duration:
        return t.cast(options).duration();
    }
    return t;
  });
}

Stream Stream::cast(pluss::table::ThreadCastTarget target,
                    std::optional<pluss::codem::ScalarType> series_dtype) const {
  return cast(target, pluss::table::ThreadCastOptions{.series_dtype = series_dtype});
}

StreamNoteAccessor Stream::nt() const {
  return StreamNoteAccessor(*this);
}

StreamTimelineAccessor Stream::tl() const {
  return StreamTimelineAccessor(*this);
}

StreamTimeSeriesAccessor Stream::ts() const {
  return StreamTimeSeriesAccessor(*this);
}

StreamLabelAccessor Stream::lbl() const {
  return StreamLabelAccessor(*this);
}

StreamPlotAccessor Stream::plot() const {
  return StreamPlotAccessor(*this, {});
}

StreamPlotAccessor Stream::plot(const pluss::table::DataFramePlotOptions& options) const {
  return StreamPlotAccessor(*this, options);
}

StreamCastAccessor::StreamCastAccessor(Stream base, pluss::table::ThreadCastOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

Stream StreamCastAccessor::series() const {
  return base_.cast(pluss::table::ThreadCastTarget::Series, options_);
}

Stream StreamCastAccessor::note() const {
  return base_.cast(pluss::table::ThreadCastTarget::Note, options_);
}

Stream StreamCastAccessor::timeline() const {
  return base_.cast(pluss::table::ThreadCastTarget::Timeline, options_);
}

Stream StreamCastAccessor::duration() const {
  return base_.cast(pluss::table::ThreadCastTarget::Duration, options_);
}

StreamNoteAccessor::StreamNoteAccessor(Stream base) : base_(std::move(base)) {}

Stream StreamNoteAccessor::strip() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().strip(); });
}

Stream StreamNoteAccessor::strip(std::string_view chars) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().strip(chars); });
}

Stream StreamNoteAccessor::lstrip() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().lstrip(); });
}

Stream StreamNoteAccessor::lstrip(std::string_view chars) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().lstrip(chars); });
}

Stream StreamNoteAccessor::rstrip() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().rstrip(); });
}

Stream StreamNoteAccessor::rstrip(std::string_view chars) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().rstrip(chars); });
}

Stream StreamNoteAccessor::lower() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().lower(); });
}

Stream StreamNoteAccessor::upper() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().upper(); });
}

Stream StreamNoteAccessor::casefold() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().casefold(); });
}

Stream StreamNoteAccessor::titlecase() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().titlecase(); });
}

Stream StreamNoteAccessor::length() const {
  return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.nt().length(); });
}

Stream StreamNoteAccessor::starts(std::string_view prefix) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().starts(prefix); });
}

Stream StreamNoteAccessor::ends(std::string_view suffix) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().ends(suffix); });
}

Stream StreamNoteAccessor::has(std::string_view needle,
                               const pluss::table::TextOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().has(needle, options); });
}

Stream StreamNoteAccessor::count(std::string_view pattern) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().count(pattern); });
}

Stream StreamNoteAccessor::count(std::string_view pattern,
                                 const pluss::table::TextOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.nt().count(pattern, options);
  });
}

Stream StreamNoteAccessor::matches(std::string_view pattern, bool full) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.nt().matches(pattern, full);
  });
}

Stream StreamNoteAccessor::fullmatch(std::string_view pattern) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().fullmatch(pattern); });
}

pluss::table::DataFrame StreamNoteAccessor::findall(std::string_view pattern) const {
  return base_.to_thread().nt().findall(pattern);
}

pluss::table::DataFrame StreamNoteAccessor::findall(std::string_view pattern,
                                                    const pluss::table::TextOptions& options) const {
  return base_.to_thread().nt().findall(pattern, options);
}

pluss::table::DataFrame StreamNoteAccessor::split(std::string_view delimiter,
                                                  std::int64_t limit,
                                                  std::string prefix) const {
  return base_.to_thread().nt().split(delimiter, limit, std::move(prefix));
}

pluss::table::DataFrame StreamNoteAccessor::split(std::string_view pattern,
                                                  const pluss::table::TextOptions& options) const {
  return base_.to_thread().nt().split(pattern, options);
}

pluss::table::DataFrame StreamNoteAccessor::partition(std::string_view separator,
                                                      std::string prefix) const {
  return base_.to_thread().nt().partition(separator, std::move(prefix));
}

Stream StreamNoteAccessor::pad(std::int64_t width,
                               pluss::table::TextAlign align,
                               char fill) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().pad(width, align, fill); });
}

Stream StreamNoteAccessor::chars(std::int64_t start,
                                 std::optional<std::int64_t> stop,
                                 std::int64_t step) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().chars(start, stop, step); });
}

Stream StreamNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().normalize(form); });
}

Stream StreamNoteAccessor::encode(std::string_view encoding) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().encode(encoding); });
}

pluss::table::DataFrame StreamNoteAccessor::indicate(std::string_view delimiter,
                                                     std::string prefix) const {
  return base_.to_thread().nt().indicate(delimiter, std::move(prefix));
}

Stream StreamNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.nt().replace(from, to); });
}

Stream StreamNoteAccessor::replace(std::string_view from,
                                   std::string_view to,
                                   const pluss::table::TextOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.nt().replace(from, to, options);
  });
}

StreamTimelineAccessor::StreamTimelineAccessor(Stream base) : base_(std::move(base)) {}

#define PLUSS_STREAM_TL_UNARY(method) \
  Stream StreamTimelineAccessor::method() const { \
    return thread_accessor_stream_result(base_, [](const pluss::table::Thread& t) { return t.tl().method(); }); \
  }

PLUSS_STREAM_TL_UNARY(isnat)
PLUSS_STREAM_TL_UNARY(valid)
PLUSS_STREAM_TL_UNARY(year)
PLUSS_STREAM_TL_UNARY(month)
PLUSS_STREAM_TL_UNARY(day)
PLUSS_STREAM_TL_UNARY(hour)
PLUSS_STREAM_TL_UNARY(minute)
PLUSS_STREAM_TL_UNARY(second)
PLUSS_STREAM_TL_UNARY(weekday)
PLUSS_STREAM_TL_UNARY(yearday)
PLUSS_STREAM_TL_UNARY(quarter)
PLUSS_STREAM_TL_UNARY(week)
PLUSS_STREAM_TL_UNARY(weekend)
PLUSS_STREAM_TL_UNARY(month_start)
PLUSS_STREAM_TL_UNARY(month_end)
PLUSS_STREAM_TL_UNARY(leap)
PLUSS_STREAM_TL_UNARY(weekday_name)
PLUSS_STREAM_TL_UNARY(month_name)
PLUSS_STREAM_TL_UNARY(iso)
PLUSS_STREAM_TL_UNARY(date)
PLUSS_STREAM_TL_UNARY(time)
PLUSS_STREAM_TL_UNARY(utc)

#undef PLUSS_STREAM_TL_UNARY

Stream StreamTimelineAccessor::fmt(std::string_view pattern) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().fmt(pattern); });
}

Stream StreamTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().epoch(unit); });
}

Stream StreamTimelineAccessor::floor(std::string_view unit) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().floor(unit); });
}

Stream StreamTimelineAccessor::ceil(std::string_view unit) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().ceil(unit); });
}

Stream StreamTimelineAccessor::round(std::string_view unit) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().round(unit); });
}

Stream StreamTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().shift(periods, freq); });
}

Stream StreamTimelineAccessor::period(std::string_view freq) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().period(freq); });
}

Stream StreamTimelineAccessor::tz(std::string_view name) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().tz(name); });
}

Stream StreamTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.tl().localize(options); });
}

StreamTimeSeriesAccessor::StreamTimeSeriesAccessor(Stream base) : base_(std::move(base)) {}

Stream StreamTimeSeriesAccessor::returns(std::int64_t periods) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.ts().returns(periods); });
}

Stream StreamTimeSeriesAccessor::log_returns(std::int64_t periods) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.ts().log_returns(periods); });
}

Stream StreamTimeSeriesAccessor::diff(std::int64_t periods) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.ts().diff(periods); });
}

Stream StreamTimeSeriesAccessor::lag(std::int64_t periods) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.ts().lag(periods); });
}

Stream StreamTimeSeriesAccessor::lead(std::int64_t periods) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) { return t.ts().lead(periods); });
}

Stream StreamTimeSeriesAccessor::rolling(std::int64_t window,
                                         pluss::table::WindowStatistic statistic,
                                         const pluss::table::WindowOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().rolling(window, statistic, options);
  });
}

Stream StreamTimeSeriesAccessor::rolling(const Stream& axis,
                                         std::string_view duration,
                                         pluss::table::WindowStatistic statistic,
                                         const pluss::table::StatisticOptions& options,
                                         pluss::table::WindowClosure closure) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().rolling(axis.to_thread(), duration, statistic, options, closure);
  });
}

Stream StreamTimeSeriesAccessor::rolling(const Stream& axis,
                                         const Stream& groups,
                                         std::string_view duration,
                                         pluss::table::WindowStatistic statistic,
                                         const pluss::table::StatisticOptions& options,
                                         pluss::table::WindowClosure closure) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().rolling(axis.to_thread(), groups.to_thread(), duration, statistic, options, closure);
  });
}

Stream StreamTimeSeriesAccessor::timed(const Stream& axis,
                                       std::int64_t duration_nanos,
                                       pluss::table::TimeWindowStatistic statistic,
                                       const pluss::table::StatisticOptions& options,
                                       pluss::table::WindowClosure closure) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().timed(axis.to_thread(), duration_nanos, statistic, options, closure);
  });
}

Stream StreamTimeSeriesAccessor::grouped(const Stream& groups,
                                         std::int64_t window,
                                         pluss::table::WindowStatistic statistic,
                                         const pluss::table::WindowOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().grouped(groups.to_thread(), window, statistic, options);
  });
}

Stream StreamTimeSeriesAccessor::expanding(pluss::table::WindowStatistic statistic,
                                           const pluss::table::StatisticOptions& options) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().expanding(statistic, options);
  });
}

Stream StreamTimeSeriesAccessor::ewm(double alpha,
                                     pluss::table::WindowStatistic statistic,
                                     bool adjusted,
                                     bool bias_corrected,
                                     std::int64_t min_count,
                                     bool ignore_nulls) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().ewm(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls);
  });
}

Stream StreamTimeSeriesAccessor::zscore_rolling(std::int64_t window,
                                                std::int64_t min_periods,
                                                double epsilon) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().zscore_rolling(window, min_periods, epsilon);
  });
}

Stream StreamTimeSeriesAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode,
                                           std::optional<double> constant) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().fill_gaps(mode, constant);
  });
}

Stream StreamTimeSeriesAccessor::align_to(const Stream& source_axis,
                                          const Stream& target_axis,
                                          pluss::table::TsAlignMode mode) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().align_to(source_axis.to_thread(), target_axis.to_thread(), mode);
  });
}

Stream StreamTimeSeriesAccessor::align_to(const Stream& source_axis,
                                          const Stream& target_axis,
                                          pluss::table::TsAlignMode mode,
                                          pluss::subsp::stats::ts::GapFillMode fill_mode,
                                          std::optional<double> constant) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().align_to(source_axis.to_thread(), target_axis.to_thread(), mode, fill_mode, constant);
  });
}

pluss::table::DataFrame StreamTimeSeriesAccessor::lag_block(pluss::msharp::RangeView::Int lags) const {
  return base_.to_thread().ts().lag_block(lags);
}

pluss::table::DataFrame StreamTimeSeriesAccessor::rolling_block(
    pluss::msharp::RangeView::Int windows,
    const pluss::table::TsRollingBlockOptions& options) const {
  return base_.to_thread().ts().rolling_block(windows, options);
}

pluss::table::DataFrame StreamTimeSeriesAccessor::calendar(const Stream& axis) const {
  return base_.to_thread().ts().calendar(axis.to_thread());
}

Stream StreamTimeSeriesAccessor::sort(const Stream& axis,
                                      bool ascending,
                                      bool nulls_last) const {
  return thread_accessor_stream_result(base_, [&](const pluss::table::Thread& t) {
    return t.ts().sort(axis.to_thread(), ascending, nulls_last);
  });
}

StreamLabelAccessor::StreamLabelAccessor(Stream base) : base_(std::move(base)) {}

Stream StreamLabelAccessor::values() const {
  const auto out = base_.to_thread().label().values();
  return Stream::from_thread(out, thread_output_labels(out));
}

bool StreamLabelAccessor::has() const {
  return base_.has_label();
}

Stream StreamLabelAccessor::set(const Stream& labels) const {
  return set(labels.to_thread());
}

Stream StreamLabelAccessor::set(const pluss::table::Thread& labels) const {
  const auto out = base_.to_thread().label().set(labels);
  return Stream::from_thread(out, thread_output_labels(out));
}

Stream StreamLabelAccessor::range() const {
  const auto out = base_.to_thread().label().range();
  return Stream::from_thread(out, thread_output_labels(out));
}

Stream StreamLabelAccessor::reset() const {
  const auto out = base_.to_thread().label().reset();
  return Stream::from_thread(out, thread_output_labels(out));
}

Stream StreamLabelAccessor::align(const Stream& target_labels,
                                  const pluss::table::AlignOptions& options) const {
  return align(target_labels.to_thread(), options);
}

Stream StreamLabelAccessor::align(const pluss::table::Thread& target_labels,
                                  const pluss::table::AlignOptions& options) const {
  const auto out = base_.to_thread().label().align(target_labels, options);
  return Stream::from_thread(out, thread_output_labels(out));
}

StreamPlotAccessor::StreamPlotAccessor(Stream base, pluss::table::DataFramePlotOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

pluss::pl::Studio StreamPlotAccessor::line() const {
  return base_.to_thread().plot(options_).line();
}

pluss::pl::Studio StreamPlotAccessor::line(const Stream& x) const {
  return base_.to_thread().plot(options_).line(x.to_thread());
}

pluss::pl::Studio StreamPlotAccessor::line(const pluss::table::LinePlotOptions& options) const {
  return base_.to_thread().plot(options_).line(options);
}

pluss::pl::Studio StreamPlotAccessor::line(const Stream& x,
                                           const pluss::table::LinePlotOptions& options) const {
  return base_.to_thread().plot(options_).line(x.to_thread(), options);
}

pluss::pl::Studio StreamPlotAccessor::hist(std::int64_t bins) const {
  return base_.to_thread().plot(options_).hist(bins);
}

pluss::pl::Studio StreamPlotAccessor::hist(const pluss::table::HistPlotOptions& options) const {
  return base_.to_thread().plot(options_).hist(options);
}

pluss::pl::Studio StreamPlotAccessor::bar() const {
  return base_.to_thread().plot(options_).bar();
}

pluss::pl::Studio StreamPlotAccessor::bar(const Stream& x) const {
  return base_.to_thread().plot(options_).bar(x.to_thread());
}

pluss::pl::Studio StreamPlotAccessor::bar(const pluss::table::BarPlotOptions& options) const {
  return base_.to_thread().plot(options_).bar(options);
}

pluss::pl::Studio StreamPlotAccessor::bar(const Stream& x,
                                          const pluss::table::BarPlotOptions& options) const {
  return base_.to_thread().plot(options_).bar(x.to_thread(), options);
}

pluss::pl::Studio StreamPlotAccessor::scatter(const Stream& x) const {
  return base_.to_thread().plot(options_).scatter(x.to_thread());
}

pluss::pl::Studio StreamPlotAccessor::scatter(const Stream& x,
                                              const pluss::table::ScatterPlotOptions& options) const {
  return base_.to_thread().plot(options_).scatter(x.to_thread(), options);
}

pluss::pl::Studio StreamPlotAccessor::scatter(const Stream& x, const Stream& hue) const {
  return base_.to_thread().plot(options_).scatter(x.to_thread(), hue.to_thread());
}

pluss::pl::Studio StreamPlotAccessor::scatter(const Stream& x,
                                              const Stream& hue,
                                              const pluss::table::ScatterPlotOptions& options) const {
  return base_.to_thread().plot(options_).scatter(x.to_thread(), hue.to_thread(), options);
}

pluss::pl::Studio StreamPlotAccessor::density(std::int64_t points) const {
  return base_.to_thread().plot(options_).density(points);
}

pluss::pl::Studio StreamPlotAccessor::density(const pluss::table::DensityPlotOptions& options) const {
  return base_.to_thread().plot(options_).density(options);
}

pluss::pl::Studio StreamPlotAccessor::auto_plot() const {
  return base_.to_thread().plot(options_).auto_plot();
}

pluss::pl::Studio StreamPlotAccessor::auto_plot(const pluss::table::AutoPlotOptions& options) const {
  return base_.to_thread().plot(options_).auto_plot(options);
}
