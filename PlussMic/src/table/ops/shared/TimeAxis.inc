namespace {

// Keeps Slate metadata beside one lazily-built core index. Exact matching
// reads the Timeline directly; ordered operations request ordered(), which
// validates once and lets the core index retain that state for every later
// kernel call.
class TimeAxis {
public:
  explicit TimeAxis(Slate axis) : axis_(std::move(axis)) {}

  const pluss::Timeline& timeline() const noexcept { return axis_.as_timeline(); }

  const pluss::Timeline::TimelineIndexAccessor& index() const {
    if (!index_.has_value()) {
      index_.emplace(axis_.as_timeline().index(false));
    }
    return *index_;
  }

  const pluss::Timeline::TimelineIndexAccessor& ordered() const {
    if (!index_.has_value()) {
      index_.emplace(axis_.as_timeline().index(true));
    } else {
      PLUSS_CHECK(index_->is_monotonic_increasing(),
                  "time axis must be sorted (non-decreasing)");
    }
    return *index_;
  }

  std::int64_t size() const { return axis_.size(); }

private:
  Slate axis_;
  mutable std::optional<pluss::Timeline::TimelineIndexAccessor> index_;
};

} // namespace
