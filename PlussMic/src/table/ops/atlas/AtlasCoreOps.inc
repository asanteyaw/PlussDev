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
std::int64_t Atlas::nrows() const { return impl_ref_().nrows_; }
std::int64_t Atlas::ncols() const { return impl_ref_().ncols_; }

const std::vector<std::string>& Atlas::column_names() const { return impl_ref_().names_; }
const std::optional<Thread>& Atlas::axis0_labels() const { return impl_ref_().axis0_labels_; }
const std::vector<DataFrame>& Atlas::frames() const { return impl_ref_().frames_; }

DataFrame Atlas::frame(std::int64_t index) const {
  const auto& src = impl_ref_();
  const auto idx = normalize_index(index, static_cast<std::int64_t>(src.frames_.size()));
  return src.frames_[static_cast<std::size_t>(idx)];
}

DataFrame Atlas::iloc(std::int64_t index) const {
  return frame(index);
}

DataFrame Atlas::squeeze() const {
  const auto d = depth();
  PLUSS_CHECK(d == 1, "Atlas::squeeze requires depth == 1");
  return frame(0);
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
  return pluss::kindle::Archive::kindelize(*this, policy);
}
