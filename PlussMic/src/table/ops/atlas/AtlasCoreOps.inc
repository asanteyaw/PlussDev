Atlas::Atlas(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels)
    : impl_(pluss::msharp::make_intrusive<detail::AtlasImpl>(std::move(frames), std::move(axis0_labels))) {}

Atlas::Atlas() = default;

Atlas::Atlas(pluss::msharp::intrusive_ptr<detail::AtlasImpl> impl) : impl_(std::move(impl)) {}

Atlas::Atlas(const Atlas& other) = default;
Atlas::Atlas(Atlas&& other) noexcept = default;
Atlas& Atlas::operator=(const Atlas& other) = default;
Atlas& Atlas::operator=(Atlas&& other) noexcept = default;
Atlas::~Atlas() = default;

bool Atlas::defined() const noexcept { return static_cast<bool>(impl_); }
bool Atlas::unique() const noexcept { return impl_.unique(); }
bool Atlas::empty() const {
  if (!defined()) {
    return true;
  }
  return depth() == 0;
}

const detail::AtlasImpl& Atlas::impl_ref_() const {
  PLUSS_CHECK(defined(), "Atlas: undefined handle");
  return *impl_;
}

std::int64_t Atlas::depth() const { return static_cast<std::int64_t>(impl_ref_().frames_.size()); }
std::int64_t Atlas::ncases() const { return impl_ref_().nrows_; }
std::int64_t Atlas::nfeatures() const { return impl_ref_().ncols_; }

const std::vector<std::string>& Atlas::column_names() const { return impl_ref_().names_; }
const std::optional<Thread>& Atlas::axis0_labels() const { return impl_ref_().axis0_labels_; }
const std::vector<DataFrame>& Atlas::frames() const { return impl_ref_().frames_; }

DataFrame Atlas::frame(std::int64_t index) const {
  const auto& src = impl_ref_();
  const auto idx = normalize_index(index, static_cast<std::int64_t>(src.frames_.size()));
  return src.frames_[static_cast<std::size_t>(idx)];
}

DataFrame Atlas::dloc(std::int64_t index) const {
  return frame(index);
}

Atlas Atlas::features(std::string_view feature) const {
  const auto single = std::array<std::string_view, 1>{feature};
  return features(pluss::msharp::RangeView::Of<std::string_view>(
      std::span<const std::string_view>(single.data(), single.size())));
}

Atlas Atlas::features(std::initializer_list<std::string_view> features) const {
  return this->features(pluss::msharp::RangeView::Of<std::string_view>(features));
}

Atlas Atlas::features(pluss::msharp::RangeView::Of<std::string_view> features) const {
  PLUSS_CHECK(features.size() > 0, "Atlas::features: expected at least one feature");
  std::vector<DataFrame> frames;
  frames.reserve(static_cast<std::size_t>(depth()));
  for (const auto& frame : impl_ref_().frames_) {
    frames.push_back(frame.features(features));
  }
  return Atlas(std::move(frames), axis0_labels());
}

AtlasLabelAccessor Atlas::label() const {
  return AtlasLabelAccessor(*this);
}

AtlasLabelAccessor::AtlasLabelAccessor(Atlas atlas)
    : atlas_(std::move(atlas)) {
  PLUSS_CHECK(atlas_.defined(), "Atlas::label: atlas is undefined");
}

Thread AtlasLabelAccessor::values() const {
  const auto& src = atlas_.impl_ref_();
  if (src.axis0_labels_.has_value()) {
    return *src.axis0_labels_;
  }
  return default_depth_labels_(atlas_.depth());
}

bool AtlasLabelAccessor::has() const {
  return atlas_.impl_ref_().axis0_labels_.has_value();
}

Atlas AtlasLabelAccessor::set(const Thread& labels) const {
  return atlas_.relabel_depth(labels);
}

Atlas AtlasLabelAccessor::range() const {
  return set(default_depth_labels_(atlas_.depth()));
}

Atlas AtlasLabelAccessor::reset() const {
  return Atlas(atlas_.impl_ref_().frames_, std::nullopt);
}

Atlas AtlasLabelAccessor::sort(bool ascending, bool nulls_last) const {
  return atlas_.sort_depth(ascending, nulls_last);
}

Atlas AtlasLabelAccessor::align(const Thread& labels) const {
  return atlas_.align_depth(labels);
}

Thread AtlasLabelAccessor::label_index_() const {
  const auto labels = values();
  return labels.label().set(labels);
}

Thread AtlasLabelAccessor::unite(const Thread& other) const {
  return label_index_().label().unite(other);
}

Thread AtlasLabelAccessor::intersect(const Thread& other) const {
  return label_index_().label().intersect(other);
}

Thread AtlasLabelAccessor::diff(const Thread& other) const {
  return label_index_().label().diff(other);
}

Thread AtlasLabelAccessor::symdiff(const Thread& other) const {
  return label_index_().label().symdiff(other);
}

Thread AtlasLabelAccessor::find(const Thread& targets) const {
  return label_index_().label().find(targets);
}

Thread AtlasLabelAccessor::pos(double value) const {
  return label_index_().label().pos(value);
}

Thread AtlasLabelAccessor::pos(std::string_view value) const {
  return label_index_().label().pos(value);
}

Thread AtlasLabelAccessor::slice(std::string_view start, std::string_view stop) const {
  return label_index_().label().slice(start, stop);
}

DataFrame Atlas::squeeze() const {
  const auto d = depth();
  PLUSS_CHECK(d == 1, "Atlas::squeeze requires depth == 1");
  return frame(0);
}

AtlasTsAccessor Atlas::ts() const {
  return AtlasTsAccessor(*this, {});
}

AtlasTimelineAccessor Atlas::tl() const {
  return AtlasTimelineAccessor(*this, {});
}

AtlasTimelineAccessor::AtlasTimelineAccessor(Atlas atlas, std::vector<std::string> features)
    : atlas_(std::move(atlas)),
      features_(std::move(features)) {
  PLUSS_CHECK(atlas_.defined(), "Atlas::tl: atlas is undefined");
}

AtlasTsAccessor::AtlasTsAccessor(Atlas atlas, std::vector<std::string> features)
    : atlas_(std::move(atlas)),
      features_(std::move(features)) {
  PLUSS_CHECK(atlas_.defined(), "Atlas::ts: atlas is undefined");
}

AtlasTsResampler::AtlasTsResampler(Atlas atlas,
                                   std::vector<std::string> features,
                                   std::string freq,
                                   std::int64_t step)
    : atlas_(std::move(atlas)),
      features_(std::move(features)),
      freq_(std::move(freq)),
      step_(step) {
  PLUSS_CHECK(atlas_.defined(), "Atlas::ts().resample: atlas is undefined");
  PLUSS_CHECK(!freq_.empty(), "Atlas::ts().resample: freq cannot be empty");
  PLUSS_CHECK(step_ > 0, "Atlas::ts().resample: step must be positive");
}

Atlas AtlasTimelineAccessor::map_(
    const std::function<DataFrame(const DataFrame&)>& fn,
    std::string_view op) const {
  PLUSS_CHECK(static_cast<bool>(fn), std::string(op) + ": callback is empty");
  return atlas_.map_frames(fn);
}

namespace {

std::vector<std::string_view> atlas_tl_feature_views_(const std::vector<std::string>& features) {
  std::vector<std::string_view> views;
  views.reserve(features.size());
  for (const auto& feature : features) {
    views.push_back(feature);
  }
  return views;
}

std::vector<std::string_view> atlas_ts_feature_views_(const std::vector<std::string>& features) {
  std::vector<std::string_view> views;
  views.reserve(features.size());
  for (const auto& feature : features) {
    views.push_back(feature);
  }
  return views;
}

DataFrameTimelineAccessor atlas_frame_tl_(const DataFrame& frame, const std::vector<std::string>& features) {
  const auto views = atlas_tl_feature_views_(features);
  if (views.empty()) {
    return frame.tl();
  }
  return frame.features(pluss::msharp::RangeView::Of<std::string_view>(views)).tl();
}

DataFrameTsAccessor atlas_frame_ts_(const DataFrame& frame, const std::vector<std::string>& features) {
  const auto views = atlas_ts_feature_views_(features);
  if (views.empty()) {
    return frame.ts();
  }
  return frame.features(pluss::msharp::RangeView::Of<std::string_view>(views)).ts();
}

} // namespace

Atlas AtlasTsAccessor::map_(
    const std::function<DataFrame(const DataFrame&)>& fn,
    std::string_view op) const {
  PLUSS_CHECK(static_cast<bool>(fn), std::string(op) + ": callback is empty");
  return atlas_.map_frames(fn);
}

#define PLUSS_ATLAS_TS_PERIOD(method_name)                                      \
  Atlas AtlasTsAccessor::method_name(std::int64_t periods) const {              \
    return map_([this, periods](const DataFrame& frame) {                       \
      return atlas_frame_ts_(frame, features_).method_name(periods);            \
    }, "Atlas::ts()." #method_name);                                            \
  }

PLUSS_ATLAS_TS_PERIOD(returns)
PLUSS_ATLAS_TS_PERIOD(log_returns)
PLUSS_ATLAS_TS_PERIOD(diff)
PLUSS_ATLAS_TS_PERIOD(lag)
PLUSS_ATLAS_TS_PERIOD(lead)

#undef PLUSS_ATLAS_TS_PERIOD

Atlas AtlasTsAccessor::rolling(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  return map_([this, window, statistic, options](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).rolling(window, statistic, options);
  }, "Atlas::ts().rolling");
}

Atlas AtlasTsAccessor::rolling(
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  const auto owned_duration = std::string(duration);
  return map_([this, owned_duration, statistic, options, closure](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).rolling(owned_duration, statistic, options, closure);
  }, "Atlas::ts().rolling");
}

Atlas AtlasTsAccessor::expanding(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  return map_([this, statistic, options](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).expanding(statistic, options);
  }, "Atlas::ts().expanding");
}

Atlas AtlasTsAccessor::ewm(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  return map_([this, alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_)
        .ewm(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls);
  }, "Atlas::ts().ewm");
}

Atlas AtlasTsAccessor::fill_gaps(
    pluss::subsp::stats::ts::GapFillMode mode,
    std::optional<double> constant) const {
  return map_([this, mode, constant](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).fill_gaps(mode, constant);
  }, "Atlas::ts().fill_gaps");
}

Atlas AtlasTsAccessor::sort(bool ascending, bool nulls_last) const {
  return map_([this, ascending, nulls_last](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).sort(ascending, nulls_last);
  }, "Atlas::ts().sort");
}

AtlasTsResampler AtlasTsAccessor::resample(std::string_view freq, std::int64_t step) const {
  return AtlasTsResampler(atlas_, features_, std::string(freq), step);
}

Atlas AtlasTsAccessor::asfreq(std::string_view freq,
                              std::int64_t step,
                              pluss::subsp::stats::ts::GapFillMode fill_mode,
                              std::optional<double> constant,
                              pluss::subsp::stats::ts::ResampleOp op) const {
  return map_([this, freq, step, fill_mode, constant, op](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).asfreq(freq, step, fill_mode, constant, op);
  }, "Atlas::ts().asfreq");
}

Atlas AtlasTsResampler::aggregate_(pluss::subsp::stats::ts::ResampleOp op) const {
  return atlas_.map_frames([this, op](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).resample(freq_, op, step_);
  });
}

Atlas AtlasTsResampler::mean() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Mean); }
Atlas AtlasTsResampler::sum() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Sum); }
Atlas AtlasTsResampler::min() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Min); }
Atlas AtlasTsResampler::max() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Max); }
Atlas AtlasTsResampler::first() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::First); }
Atlas AtlasTsResampler::last() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Last); }
Atlas AtlasTsResampler::count() const { return aggregate_(pluss::subsp::stats::ts::ResampleOp::Count); }

Atlas AtlasTsResampler::asfreq(std::optional<double> fill_value) const {
  return atlas_.map_frames([this, fill_value](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).resample(freq_, step_).asfreq(fill_value);
  });
}

Atlas AtlasTsResampler::ffill() const {
  return atlas_.map_frames([this](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).resample(freq_, step_).ffill();
  });
}

Atlas AtlasTsResampler::bfill() const {
  return atlas_.map_frames([this](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).resample(freq_, step_).bfill();
  });
}

Atlas AtlasTsResampler::nearest() const {
  return atlas_.map_frames([this](const DataFrame& frame) {
    return atlas_frame_ts_(frame, features_).resample(freq_, step_).nearest();
  });
}

#define PLUSS_ATLAS_TL_NOARG(method_name)                                      \
  Atlas AtlasTimelineAccessor::method_name() const {                           \
    return map_([this](const DataFrame& frame) {                               \
      return atlas_frame_tl_(frame, features_)                                 \
          .method_name();                                                      \
    }, "Atlas::tl()." #method_name);                                           \
  }

PLUSS_ATLAS_TL_NOARG(isnat)
PLUSS_ATLAS_TL_NOARG(valid)
PLUSS_ATLAS_TL_NOARG(year)
PLUSS_ATLAS_TL_NOARG(month)
PLUSS_ATLAS_TL_NOARG(day)
PLUSS_ATLAS_TL_NOARG(hour)
PLUSS_ATLAS_TL_NOARG(minute)
PLUSS_ATLAS_TL_NOARG(second)
PLUSS_ATLAS_TL_NOARG(weekday)
PLUSS_ATLAS_TL_NOARG(yearday)
PLUSS_ATLAS_TL_NOARG(quarter)
PLUSS_ATLAS_TL_NOARG(week)
PLUSS_ATLAS_TL_NOARG(weekend)
PLUSS_ATLAS_TL_NOARG(month_start)
PLUSS_ATLAS_TL_NOARG(month_end)
PLUSS_ATLAS_TL_NOARG(leap)
PLUSS_ATLAS_TL_NOARG(weekday_name)
PLUSS_ATLAS_TL_NOARG(month_name)
PLUSS_ATLAS_TL_NOARG(iso)
PLUSS_ATLAS_TL_NOARG(date)
PLUSS_ATLAS_TL_NOARG(time)
PLUSS_ATLAS_TL_NOARG(utc)

#undef PLUSS_ATLAS_TL_NOARG

Atlas AtlasTimelineAccessor::fmt(std::string_view pattern) const {
  return map_([this, pattern](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).fmt(pattern);
  }, "Atlas::tl().fmt");
}

Atlas AtlasTimelineAccessor::epoch(TimeUnit unit) const {
  return map_([this, unit](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).epoch(unit);
  }, "Atlas::tl().epoch");
}

Atlas AtlasTimelineAccessor::span(
    const Thread& other,
    const TimelineSpanOptions& options) const {
  return map_([this, other, options](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).span(other, options);
  }, "Atlas::tl().span");
}

Atlas AtlasTimelineAccessor::span(const Thread& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Atlas AtlasTimelineAccessor::span(
    const pluss::Timeline& other,
    const TimelineSpanOptions& options) const {
  return span(Thread(other), options);
}

Atlas AtlasTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Atlas AtlasTimelineAccessor::floor(std::string_view unit) const {
  return map_([this, unit](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).floor(unit);
  }, "Atlas::tl().floor");
}

Atlas AtlasTimelineAccessor::ceil(std::string_view unit) const {
  return map_([this, unit](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).ceil(unit);
  }, "Atlas::tl().ceil");
}

Atlas AtlasTimelineAccessor::round(std::string_view unit) const {
  return map_([this, unit](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).round(unit);
  }, "Atlas::tl().round");
}

Atlas AtlasTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return map_([this, periods, freq](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).shift(periods, freq);
  }, "Atlas::tl().shift");
}

Atlas AtlasTimelineAccessor::tz(std::string_view name) const {
  return map_([this, name](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).tz(name);
  }, "Atlas::tl().tz");
}

Atlas AtlasTimelineAccessor::localize(const TimelineLocalizeOptions& options) const {
  return map_([this, options](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).localize(options);
  }, "Atlas::tl().localize");
}

Atlas AtlasTimelineAccessor::index(bool require_sorted) const {
  return map_([this, require_sorted](const DataFrame& frame) {
    return atlas_frame_tl_(frame, features_).index(require_sorted);
  }, "Atlas::tl().index");
}

DataFrame Atlas::loc(std::string_view label) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.axis0_labels_.has_value(), "Atlas::loc(string): axis0 labels are not set");
  const auto pos = find_note_label(*src.axis0_labels_, label);
  PLUSS_CHECK(pos >= 0, "Atlas::loc(string): label not found");
  return src.frames_[static_cast<std::size_t>(pos)];
}

DataFrame Atlas::loc(std::int64_t label) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.axis0_labels_.has_value(), "Atlas::loc(int64): axis0 labels are not set");
  const auto pos = find_int_label(*src.axis0_labels_, label);
  PLUSS_CHECK(pos >= 0, "Atlas::loc(int64): label not found");
  return src.frames_[static_cast<std::size_t>(pos)];
}

DataFrame Atlas::loc(const pluss::Timeline& label) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.axis0_labels_.has_value(), "Atlas::loc(Timeline): axis0 labels are not set");
  const auto pos = find_timeline_label(*src.axis0_labels_, label);
  PLUSS_CHECK(pos >= 0, "Atlas::loc(Timeline): label not found");
  return src.frames_[static_cast<std::size_t>(pos)];
}

Atlas Atlas::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice_frames(0, clipped);
}

Atlas Atlas::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice_frames(depth() - clipped, depth());
}

Atlas Atlas::slice_frames(std::int64_t start, std::int64_t stop) const {
  const auto& src = impl_ref_();
  const auto [s, e] = normalize_slice(start, stop, static_cast<std::int64_t>(src.frames_.size()));

  std::vector<DataFrame> out_frames;
  out_frames.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    out_frames.push_back(src.frames_[static_cast<std::size_t>(i)]);
  }

  std::optional<Thread> out_labels = std::nullopt;
  if (src.axis0_labels_.has_value()) {
    out_labels = src.axis0_labels_->slice(s, e);
  }
  return Atlas(std::move(out_frames), std::move(out_labels));
}

Atlas Atlas::sort_depth(bool ascending, bool nulls_last) const {
  const auto& src = impl_ref_();
  std::vector<std::int64_t> idx(static_cast<std::size_t>(depth()));
  std::iota(idx.begin(), idx.end(), 0);

  if (src.axis0_labels_.has_value()) {
    const auto labels = *src.axis0_labels_;
    std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
      const auto cmp = labels.compare_rows(a, b, nulls_last);
      return ascending ? (cmp < 0) : (cmp > 0);
    });
  } else if (!ascending) {
    std::reverse(idx.begin(), idx.end());
  }

  std::vector<DataFrame> out_frames;
  out_frames.reserve(idx.size());
  for (const auto i : idx) {
    out_frames.push_back(src.frames_[static_cast<std::size_t>(i)]);
  }

  std::optional<Thread> out_labels = std::nullopt;
  if (src.axis0_labels_.has_value()) {
    out_labels = src.axis0_labels_->take(idx);
  }
  return Atlas(std::move(out_frames), std::move(out_labels));
}

Atlas Atlas::relabel_depth(const Thread& labels) const {
  return Atlas(impl_ref_().frames_, labels);
}

Atlas Atlas::align_depth(const Thread& labels) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.axis0_labels_.has_value(),
              "Atlas::align_depth: current atlas has no depth labels");
  PLUSS_CHECK(labels.defined(), "Atlas::align_depth: new labels are undefined");

  std::unordered_map<std::string, std::int64_t> pos;
  pos.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    pos.emplace(label_string_at(*src.axis0_labels_, i), i);
  }

  std::vector<DataFrame> out_frames;
  out_frames.reserve(static_cast<std::size_t>(labels.size()));
  std::vector<std::int64_t> take_idx;
  take_idx.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    const auto key = label_string_at(labels, i);
    const auto it = pos.find(key);
    PLUSS_CHECK(it != pos.end(), "Atlas::align_depth: label not found '" + key + "'");
    out_frames.push_back(src.frames_[static_cast<std::size_t>(it->second)]);
    take_idx.push_back(i);
  }
  auto out_labels = labels.take(take_idx);
  return Atlas(std::move(out_frames), out_labels);
}

Atlas Atlas::map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Atlas::map_frames: mapper function is empty");
  const auto& src = impl_ref_();

  std::vector<DataFrame> out_frames;
  out_frames.reserve(src.frames_.size());
  for (const auto& f : src.frames_) {
    out_frames.push_back(fn(f));
  }
  return Atlas(std::move(out_frames), src.axis0_labels_);
}

Atlas Atlas::map(
    const std::function<ThreadValue(const ThreadValue&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Atlas::map: callback is empty");
  return map_frames(
      [&fn](const DataFrame& frame) {
        return frame.map(fn);
      });
}

Atlas Atlas::flow(
    const std::function<Atlas(const Atlas&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Atlas::flow: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Atlas::flow: callback returned an undefined Atlas");
  return result;
}

DataFrame Atlas::aggregate_over_frames(
    const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Atlas::aggregate_over_frames: reducer function is empty");
  const auto& src = impl_ref_();
  return fn(src.frames_);
}

pluss::kindle::Archive Atlas::kindelize() const {
  return kindelize(pluss::kindle::KindelizePolicy{});
}

pluss::kindle::Archive Atlas::kindelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Atlas::kindelize requires a defined Atlas");
  std::vector<pluss::kindle::Catalog> catalogs;
  catalogs.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    catalogs.push_back(frame(i).kindelize(policy));
  }

  std::optional<pluss::kindle::Stream> depth_labels = std::nullopt;
  if (axis0_labels().has_value()) {
    depth_labels = pluss::kindle::Stream::from_thread(*axis0_labels());
  }
  return pluss::kindle::Archive(std::move(catalogs), std::move(depth_labels));
}

pluss::scribble::Axel Atlas::scribbelize() const {
  return scribbelize(pluss::kindle::KindelizePolicy{});
}

pluss::scribble::Axel Atlas::scribbelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Atlas::scribbelize requires a defined Atlas");
  return kindelize(policy).scribbelize();
}
