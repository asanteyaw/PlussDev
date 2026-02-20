#include "table/Atlas.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "table/display/Display.h"

namespace pluss::table {

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
  PLUSS_CHECK(ref.nrows() == cur.nrows(), "Atlas: all frames must have identical row count (strict cube)");
  PLUSS_CHECK(ref.ncols() == cur.ncols(), "Atlas: all frames must have identical column count");
  PLUSS_CHECK(ref.column_names() == cur.column_names(), "Atlas: all frames must have identical column names/order");

  const auto ref_cols = ref.columns();
  const auto cur_cols = cur.columns();
  PLUSS_CHECK(ref_cols.size() == cur_cols.size(), "Atlas: internal schema mismatch");
  for (std::size_t i = 0; i < ref_cols.size(); ++i) {
    PLUSS_CHECK(ref_cols[i].kind() == cur_cols[i].kind(), "Atlas: column kind mismatch across frames");
    PLUSS_CHECK(ref_cols[i].dtype_name() == cur_cols[i].dtype_name(), "Atlas: column dtype mismatch across frames");
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

} // namespace

AtlasImpl::AtlasImpl(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels)
    : frames_(std::move(frames)), axis0_labels_(std::move(axis0_labels)) {
  validate_();
}

void AtlasImpl::validate_() {
  if (frames_.empty()) {
    nrows_ = 0;
    ncols_ = 0;
    names_.clear();
    validate_axis0_labels(axis0_labels_, 0);
    return;
  }

  const auto& ref = frames_.front();
  PLUSS_CHECK(ref.defined(), "Atlas: frame[0] is undefined");
  nrows_ = ref.nrows();
  ncols_ = ref.ncols();
  names_ = ref.column_names();

  for (std::size_t i = 0; i < frames_.size(); ++i) {
    PLUSS_CHECK(frames_[i].defined(), "Atlas: frame[" + std::to_string(i) + "] is undefined");
    validate_schema_compat(ref, frames_[i]);
  }
  validate_axis0_labels(axis0_labels_, static_cast<std::int64_t>(frames_.size()));
}

Atlas::Atlas(std::vector<DataFrame> frames, std::optional<Thread> axis0_labels)
    : impl_(pluss::msharp::make_intrusive<AtlasImpl>(std::move(frames), std::move(axis0_labels))) {}

Atlas::Atlas(pluss::msharp::intrusive_ptr<AtlasImpl> impl) : impl_(std::move(impl)) {}

bool Atlas::defined() const noexcept { return static_cast<bool>(impl_); }
bool Atlas::unique() const noexcept { return impl_.unique(); }

const AtlasImpl& Atlas::impl_ref_() const {
  PLUSS_CHECK(defined(), "Atlas: undefined handle");
  return *impl_;
}

std::int64_t Atlas::depth() const { return static_cast<std::int64_t>(impl_ref_().frames_.size()); }
std::int64_t Atlas::nrows() const { return impl_ref_().nrows_; }
std::int64_t Atlas::ncols() const { return impl_ref_().ncols_; }

const std::vector<std::string>& Atlas::column_names() const { return impl_ref_().names_; }
const std::optional<Thread>& Atlas::axis0_labels() const { return impl_ref_().axis0_labels_; }

DataFrame Atlas::frame(std::int64_t index) const {
  const auto& src = impl_ref_();
  const auto idx = normalize_index(index, static_cast<std::int64_t>(src.frames_.size()));
  return src.frames_[static_cast<std::size_t>(idx)];
}

DataFrame Atlas::iloc(std::int64_t index) const {
  return frame(index);
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

std::string Atlas::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

void Atlas::show(const display::DisplayOptions& options) const {
  display::print(std::cout, *this, options);
}

void Atlas::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const Atlas& atlas) {
  atlas.show(os);
  return os;
}

} // namespace pluss::table
