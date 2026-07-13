namespace detail {

class AtlasImpl final : public pluss::msharp::RefCounted {
public:
  AtlasImpl(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels);
  void validate_();

  std::vector<DataFrame> frames_;
  std::optional<Thread> axis0_labels_;
  std::int64_t nrows_{0};
  std::int64_t ncols_{0};
  std::vector<std::string> names_;
};

} // namespace detail

namespace {

std::int64_t normalize_index(std::int64_t idx, std::int64_t size) {
  std::int64_t out = idx;
  if (out < 0) {
    out += size;
  }
  PLUSS_CHECK(out >= 0 && out < size, "Atlas: frame index out of bounds");
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

void validate_axis0_labels(const std::optional<Thread>& labels, std::int64_t depth) {
  if (!labels.has_value()) {
    return;
  }
  PLUSS_CHECK(labels->defined(), "Atlas: axis0 labels are undefined");
  PLUSS_CHECK(labels->size() == depth, "Atlas: axis0 labels length must equal depth");
}

void validate_schema_compat(const DataFrame& ref, const DataFrame& cur) {
  PLUSS_CHECK(ref.ncases() == cur.ncases(), "Atlas: all frames must have identical row count (strict cube)");
  PLUSS_CHECK(ref.nfeats() == cur.nfeats(), "Atlas: all frames must have identical column count");
  PLUSS_CHECK(ref.features() == cur.features(), "Atlas: all frames must have identical column names/order");

  const auto& ref_names = ref.features();
  for (const auto& name : ref_names) {
    const auto ref_col = ref.features(name);
    const auto cur_col = cur.features(name);
    PLUSS_CHECK(ref_col.kind() == cur_col.kind(), "Atlas: column kind mismatch across frames");
    PLUSS_CHECK(ref_col.dtype() == cur_col.dtype(), "Atlas: column dtype mismatch across frames");
  }
}

std::int64_t find_note_label(const Thread& labels, std::string_view key) {
  PLUSS_CHECK(labels.kind() == ThreadKind::Note, "Atlas::loc(string): axis0 labels must be Note");
  const auto& note = labels.as_note();
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (note.at(i) == key) {
      return i;
    }
  }
  return -1;
}

std::int64_t find_int_label(const Thread& labels, std::int64_t key) {
  PLUSS_CHECK(labels.kind() == ThreadKind::Series, "Atlas::loc(int64): axis0 labels must be numeric Series");
  const auto& s = labels.as_series();
  const auto vals = s.to_vec<std::int64_t>();
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (vals[static_cast<std::size_t>(i)] == key) {
      return i;
    }
  }
  return -1;
}

std::vector<std::int64_t> timeline_ticks(const Thread& labels) {
  if (labels.kind() == ThreadKind::Timeline) {
    return labels.as_timeline().to_epoch_nanos_vector();
  }
  if (labels.kind() == ThreadKind::TimeIndex) {
    return labels.as_timeindex().timeline().to_epoch_nanos_vector();
  }
  PLUSS_FAIL("Atlas::loc(Timeline): axis0 labels must be Timeline or TimeIndex");
}

std::int64_t find_timeline_label(const Thread& labels, const pluss::Timeline& key) {
  PLUSS_CHECK(key.defined(), "Atlas::loc(Timeline): key is undefined");
  PLUSS_CHECK(key.numel() == 1, "Atlas::loc(Timeline): key must be scalar Timeline");
  const auto key_ticks = key.to_epoch_nanos_vector();
  const auto all_ticks = timeline_ticks(labels);
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    if (all_ticks[static_cast<std::size_t>(i)] == key_ticks[0]) {
      return i;
    }
  }
  return -1;
}

std::string label_string_at(const Thread& labels, std::int64_t i) {
  switch (labels.kind()) {
    case ThreadKind::Series: {
      const auto s = labels.as_series();
      switch (s.scalar_type()) {
        case pluss::codem::ScalarType::Int64:
          return std::to_string(s.to_vec<std::int64_t>(i));
        case pluss::codem::ScalarType::Float64:
          return std::to_string(s.to_vec<double>(i));
        default:
          return std::to_string(i);
      }
    }
    case ThreadKind::Note:
      return std::string(labels.as_note().at(i));
    case ThreadKind::Timeline:
      return std::to_string(labels.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(i)]);
    case ThreadKind::TimeIndex:
      return std::to_string(labels.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(i)]);
    default:
      return std::to_string(i);
  }
}

} // namespace

detail::AtlasImpl::AtlasImpl(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels)
    : frames_(std::move(frames)), axis0_labels_(std::move(axis0_labels)) {
  validate_();
}

void detail::AtlasImpl::validate_() {
  if (frames_.empty()) {
    nrows_ = 0;
    ncols_ = 0;
    names_.clear();
    validate_axis0_labels(axis0_labels_, 0);
    return;
  }

  const auto& ref = frames_.front();
  PLUSS_CHECK(ref.defined(), "Atlas: frame[0] is undefined");
  nrows_ = ref.ncases();
  ncols_ = ref.nfeats();
  names_ = ref.features();

  for (std::size_t i = 0; i < frames_.size(); ++i) {
    PLUSS_CHECK(frames_[i].defined(), "Atlas: frame[" + std::to_string(i) + "] is undefined");
    validate_schema_compat(ref, frames_[i]);
  }
  validate_axis0_labels(axis0_labels_, static_cast<std::int64_t>(frames_.size()));
}
