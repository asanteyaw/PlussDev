#include "table/Thread.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace {

using Payload = ThreadPayload;

std::int64_t normalize_index(std::int64_t idx, std::int64_t size) {
  std::int64_t out = idx;
  if (out < 0) {
    out += size;
  }
  PLUSS_CHECK(out >= 0 && out < size, "Thread: index out of bounds");
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

pluss::Series bool_series_zeros(std::int64_t n) {
  PLUSS_CHECK(n >= 0, "Thread: bool mask length must be non-negative");
  return pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
}

pluss::Series bool_series_from_u8(const std::vector<std::uint8_t>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = values[static_cast<std::size_t>(i)] != 0;
  }
  return out;
}

pluss::Series int64_index_series(const std::vector<std::int64_t>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return out;
}

std::vector<bool> bool_vec_from_series(const pluss::Series& s, std::int64_t expected_size, std::string_view where) {
  PLUSS_CHECK(s.defined(), std::string(where) + ": bool mask is undefined");
  PLUSS_CHECK(s.dim() == 1, std::string(where) + ": bool mask must be 1D");
  PLUSS_CHECK(s.numel() == expected_size, std::string(where) + ": bool mask length mismatch");
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool, std::string(where) + ": bool mask dtype must be Bool");
  return s.to_vec<bool>();
}

void validate_validity_mask(const std::optional<pluss::Series>& mask, std::int64_t size) {
  if (!mask.has_value()) {
    return;
  }
  const auto& m = *mask;
  PLUSS_CHECK(m.defined(), "Thread: validity mask is undefined");
  PLUSS_CHECK(m.dim() == 1, "Thread: validity mask must be 1D");
  PLUSS_CHECK(m.scalar_type() == pluss::codem::ScalarType::Bool, "Thread: validity mask must use Bool dtype");
  PLUSS_CHECK(m.numel() == size, "Thread: validity mask length mismatch");
}

std::int64_t infer_size(const pluss::Series& s) {
  PLUSS_CHECK(s.defined(), "Thread: Series payload is undefined");
  PLUSS_CHECK(s.dim() == 1, "Thread: Series payload must be strictly 1D");
  return s.numel();
}

std::int64_t infer_size(const pluss::Note& n) {
  PLUSS_CHECK(n.defined(), "Thread: Note payload is undefined");
  PLUSS_CHECK(n.dim() == 1, "Thread: Note payload must be strictly 1D");
  return n.numel();
}

std::int64_t infer_size(const pluss::Timeline& t) {
  PLUSS_CHECK(t.defined(), "Thread: Timeline payload is undefined");
  PLUSS_CHECK(t.dim() == 1, "Thread: Timeline payload must be strictly 1D");
  return t.numel();
}

std::int64_t infer_size(const pluss::TimeIndex& ti) {
  PLUSS_CHECK(ti.timeline().defined(), "Thread: TimeIndex payload is undefined");
  PLUSS_CHECK(ti.timeline().dim() == 1, "Thread: TimeIndex payload must be strictly 1D");
  return ti.size();
}

ThreadKind infer_kind(const Payload& payload) {
  if (std::holds_alternative<pluss::Series>(payload)) return ThreadKind::Series;
  if (std::holds_alternative<pluss::Note>(payload)) return ThreadKind::Note;
  if (std::holds_alternative<pluss::Timeline>(payload)) return ThreadKind::Timeline;
  return ThreadKind::TimeIndex;
}

std::int64_t infer_size(const Payload& payload) {
  return std::visit([](const auto& v) { return infer_size(v); }, payload);
}

std::string infer_dtype_name(const Payload& payload) {
  return std::visit([](const auto& v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
      return std::string(v.dtype().name);
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
      return static_cast<const pluss::coden::NoteBase&>(v).is_bytes() ? "bytes" : "utf8";
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
      return std::string("datetime64[ns, ") + std::string(v.tz()) + "]";
    } else {
      return "timeindex";
    }
  }, payload);
}

pluss::msharp::intrusive_ptr<ThreadImpl> build_impl(
    Payload payload,
    std::optional<pluss::Series> validity_mask);

std::vector<std::int64_t> true_indices_from_mask(const pluss::Series& mask, std::int64_t expected_size) {
  const auto keep = bool_vec_from_series(mask, expected_size, "Thread::filter");
  std::vector<std::int64_t> out;
  out.reserve(static_cast<std::size_t>(expected_size));
  for (std::int64_t i = 0; i < expected_size; ++i) {
    if (keep[static_cast<std::size_t>(i)]) {
      out.push_back(i);
    }
  }
  return out;
}

std::vector<std::string> note_take_strings(const pluss::Note& note, std::span<const std::int64_t> indices, std::int64_t size) {
  std::vector<std::string> out;
  out.reserve(indices.size());
  for (const auto raw_idx : indices) {
    const auto idx = normalize_index(raw_idx, size);
    out.emplace_back(note.at(idx));
  }
  return out;
}

pluss::Timeline timeline_take(const pluss::Timeline& t, std::span<const std::int64_t> indices, std::int64_t size) {
  const auto ticks = t.to_epoch_nanos_vector();
  std::vector<std::int64_t> out;
  out.reserve(indices.size());
  for (const auto raw_idx : indices) {
    const auto idx = normalize_index(raw_idx, size);
    out.push_back(ticks[static_cast<std::size_t>(idx)]);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(out.size())};
  auto result = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(out.data(), out.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  result.unsafe_tz_name() = std::string(t.tz());
  return result;
}

} // namespace

ThreadImpl::ThreadImpl(Payload payload, std::optional<pluss::Series> validity_mask)
    : payload_(std::move(payload)),
      kind_(infer_kind(payload_)),
      size_(infer_size(payload_)),
      dtype_name_(infer_dtype_name(payload_)),
      validity_mask_(std::move(validity_mask)) {
  validate_validity_mask(validity_mask_, size_);
}

namespace {

pluss::msharp::intrusive_ptr<ThreadImpl> build_impl(
    Payload payload,
    std::optional<pluss::Series> validity_mask) {
  return pluss::msharp::make_intrusive<ThreadImpl>(std::move(payload), std::move(validity_mask));
}

pluss::msharp::intrusive_ptr<ThreadImpl> clone_impl(const ThreadImpl& src) {
  return build_impl(src.payload_, src.validity_mask_);
}

double reduction_value(const pluss::Series& scalar_series, std::string_view where) {
  PLUSS_CHECK(scalar_series.defined(), std::string(where) + ": reduction output is undefined");
  PLUSS_CHECK(scalar_series.numel() == 1, std::string(where) + ": reduction output must be scalar");
  return scalar_series.to_vec<double>(0);
}

} // namespace

Thread::Thread(pluss::msharp::intrusive_ptr<ThreadImpl> impl) : impl_(std::move(impl)) {}

Thread::Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

bool Thread::defined() const noexcept { return static_cast<bool>(impl_); }
bool Thread::unique() const noexcept { return impl_.unique(); }

const ThreadImpl& Thread::impl_ref_() const {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  return *impl_;
}

ThreadImpl& Thread::mut_impl_() {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

std::int64_t Thread::size() const { return impl_ref_().size_; }
ThreadKind Thread::kind() const { return impl_ref_().kind_; }
std::string Thread::dtype_name() const { return impl_ref_().dtype_name_; }

bool Thread::has_validity_mask() const { return impl_ref_().validity_mask_.has_value(); }

const std::optional<pluss::Series>& Thread::validity_mask() const {
  return impl_ref_().validity_mask_;
}

Thread Thread::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, validity_mask));
}

Thread& Thread::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Thread Thread::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, std::nullopt));
}

Thread& Thread::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

pluss::Series Thread::isnull() const {
  const auto& src = impl_ref_();
  if (!src.validity_mask_.has_value()) {
    return bool_series_zeros(src.size_);
  }
  return src.validity_mask_->logical_not();
}

pluss::Series Thread::isnan() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isnan is only valid for Series payload");
  return as_series().isnan();
}

pluss::Series Thread::isfinite() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isfinite is only valid for Series payload");
  auto finite = as_series().isfinite();
  if (has_validity_mask()) {
    finite = finite.logical_and(*validity_mask());
  }
  return finite;
}

Thread Thread::fillna(double value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto out = as_series().where(*validity_mask(), value);
  return Thread(out, std::nullopt);
}

Thread Thread::abs() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::abs is only valid for Series payload");
  return Thread(as_series().abs(), validity_mask());
}

Thread Thread::clip(double min_value, double max_value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::clip is only valid for Series payload");
  return Thread(as_series().clamp(min_value, max_value), validity_mask());
}

Thread& Thread::fillna_(double value) {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna_(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto& w = mut_impl_();
  auto* s = std::get_if<pluss::Series>(&w.payload_);
  PLUSS_CHECK(s != nullptr, "Thread::fillna_: internal payload mismatch");
  *s = s->where(*w.validity_mask_, value);
  w.validity_mask_.reset();
  return *this;
}

Thread Thread::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(0, clipped);
}

Thread Thread::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(size() - clipped, size());
}

Thread Thread::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    indices.push_back(i);
  }
  return take(indices);
}

Thread Thread::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  Payload out_payload = std::visit([&](const auto& payload) -> Payload {
    using T = std::decay_t<decltype(payload)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
      std::vector<std::int64_t> idx_norm;
      idx_norm.reserve(indices.size());
      for (const auto raw_idx : indices) {
        idx_norm.push_back(normalize_index(raw_idx, src.size_));
      }
      auto idx_series = int64_index_series(idx_norm);
      return payload.take(idx_series);
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
      const auto values = note_take_strings(payload, indices, src.size_);
      if (static_cast<const pluss::coden::NoteBase&>(payload).is_bytes()) {
        std::vector<std::string_view> views;
        views.reserve(values.size());
        for (const auto& v : values) views.emplace_back(v);
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
        return pluss::coden::ops_internal::note_from_bytes(
            std::span<const std::string_view>(views.data(), views.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
      }
      std::vector<std::string_view> views;
      views.reserve(values.size());
      for (const auto& v : values) views.emplace_back(v);
      const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
      return pluss::coden::ops_internal::note_from_utf8(
          std::span<const std::string_view>(views.data(), views.size()),
          std::span<const std::int64_t>(shape.data(), shape.size()));
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
      return timeline_take(payload, indices, src.size_);
    } else {
      auto t = timeline_take(payload.timeline(), indices, src.size_);
      return t.to_timeindex(false);
    }
  }, src.payload_);

  std::optional<pluss::Series> out_mask = std::nullopt;
  if (src.validity_mask_.has_value()) {
    std::vector<std::int64_t> idx_norm;
    idx_norm.reserve(indices.size());
    for (const auto raw_idx : indices) {
      idx_norm.push_back(normalize_index(raw_idx, src.size_));
    }
    auto idx_series = int64_index_series(idx_norm);
    out_mask = src.validity_mask_->take(idx_series);
  }

  return Thread(build_impl(std::move(out_payload), std::move(out_mask)));
}

Thread Thread::filter(const pluss::Series& bool_mask) const {
  const auto indices = true_indices_from_mask(bool_mask, size());
  return take(indices);
}

double Thread::sum() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::sum on an empty (all-null) selection");
  return reduction_value(s.sum(), "Thread::sum");
}

double Thread::mean() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::mean is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::mean on an empty (all-null) selection");
  return reduction_value(s.mean(), "Thread::mean");
}

double Thread::min() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::min is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::min on an empty (all-null) selection");
  return reduction_value(s.min(), "Thread::min");
}

double Thread::max() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::max is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::max on an empty (all-null) selection");
  return reduction_value(s.max(), "Thread::max");
}

double Thread::var(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::var is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::var on an empty (all-null) selection");
  return reduction_value(s.var(unbiased), "Thread::var");
}

double Thread::std(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::std is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::std on an empty (all-null) selection");
  return reduction_value(s.std(unbiased), "Thread::std");
}

Thread Thread::lower() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::lower is only valid for Note payload");
  return Thread(as_note().lower(), validity_mask());
}

pluss::Series Thread::contains(std::string_view needle) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::contains is only valid for Note payload");
  return bool_series_from_u8(as_note().contains(needle));
}

const pluss::Series& Thread::as_series() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread: payload is not Series");
  return std::get<pluss::Series>(impl_ref_().payload_);
}

const pluss::Note& Thread::as_note() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread: payload is not Note");
  return std::get<pluss::Note>(impl_ref_().payload_);
}

const pluss::Timeline& Thread::as_timeline() const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline, "Thread: payload is not Timeline");
  return std::get<pluss::Timeline>(impl_ref_().payload_);
}

const pluss::TimeIndex& Thread::as_timeindex() const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex, "Thread: payload is not TimeIndex");
  return std::get<pluss::TimeIndex>(impl_ref_().payload_);
}

std::string Thread::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

void Thread::show(const display::DisplayOptions& options) const {
  display::print(std::cout, *this, options);
}

void Thread::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const Thread& thread) {
  thread.show(os);
  return os;
}

} // namespace pluss::table
