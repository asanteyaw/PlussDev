#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "CodeN/core/AlignedAllocator.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteDType.h"
#include "CodeN/core/TextArena.h"
#include "Flame/Flame.h"

namespace pluss::coden {

class NoteImpl final : public ::pluss::flame::RefCounted {
public:
  using OffLen = ::std::pair<::std::size_t, ::std::size_t>;
  static constexpr ::std::size_t kLaneAlignment = 64;
  using BoolStorage =
      ::std::vector<::std::uint8_t,
                    detail::AlignedAllocator<::std::uint8_t, kLaneAlignment>>;
  using Int64Storage =
      ::std::vector<::std::int64_t,
                    detail::AlignedAllocator<::std::int64_t, kLaneAlignment>>;
  using Float64Storage =
      ::std::vector<double, detail::AlignedAllocator<double, kLaneAlignment>>;

  template <class Storage>
  class LaneBuffer final : public ::pluss::flame::RefCounted {
  public:
    using storage_type = Storage;
    using value_type = typename Storage::value_type;
    explicit LaneBuffer(Storage values) : values_(::std::move(values)) {}
    const Storage &values() const noexcept { return values_; }

  private:
    Storage values_;
  };

  using BoolBuffer = LaneBuffer<BoolStorage>;
  using Int64Buffer = LaneBuffer<Int64Storage>;
  using Float64Buffer = LaneBuffer<Float64Storage>;

  template <class Buffer>
  struct LaneView {
    ::pluss::flame::intrusive_ptr<Buffer> buffer;
    ::std::size_t offset{0};
  };

  struct TextStorage {
    ::pluss::flame::intrusive_ptr<TextArena> arena;
    ::std::vector<OffLen> spans;
  };

  NoteImpl(::pluss::flame::intrusive_ptr<TextArena> arena,
           ::std::vector<OffLen> spans, ::std::array<::std::int64_t, 3> sizes,
           ::std::int64_t dim, NoteDType dtype = NoteDType::Text)
      : payload_(TextStorage{::std::move(arena), ::std::move(spans)}),
        sizes_(sizes), dim_(dim), dtype_(dtype),
        numel_(checked_numel(sizes, dim)) {
    PLUSS_CHECK(dtype == NoteDType::Text || dtype == NoteDType::Bytes,
                "Note text storage requires Text or Bytes dtype");
    validate_text_storage();
  }

  NoteImpl(BoolStorage values, ::std::array<::std::int64_t, 3> sizes,
           ::std::int64_t dim)
      : payload_(LaneView<BoolBuffer>{
            ::pluss::flame::make_intrusive<BoolBuffer>(::std::move(values)), 0}),
        sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Bool), numel_(checked_numel(sizes, dim)) {
    const auto lanes = bool_values();
    validate_lane_count(lanes.size());
    PLUSS_CHECK(::std::all_of(lanes.begin(), lanes.end(),
                              [](::std::uint8_t value) { return value <= 1; }),
                "Bool Note storage accepts only canonical 0 or 1 lanes");
  }

  NoteImpl(Int64Storage values, ::std::array<::std::int64_t, 3> sizes,
           ::std::int64_t dim)
      : payload_(LaneView<Int64Buffer>{
            ::pluss::flame::make_intrusive<Int64Buffer>(::std::move(values)), 0}),
        sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Int64), numel_(checked_numel(sizes, dim)) {
    validate_lane_count(int64_values().size());
  }

  NoteImpl(Float64Storage values, ::std::array<::std::int64_t, 3> sizes,
           ::std::int64_t dim)
      : payload_(LaneView<Float64Buffer>{
            ::pluss::flame::make_intrusive<Float64Buffer>(::std::move(values)), 0}),
        sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Float64), numel_(checked_numel(sizes, dim)) {
    validate_lane_count(float64_values().size());
  }

  NoteImpl(LaneView<BoolBuffer> view,
           ::std::array<::std::int64_t, 3> sizes, ::std::int64_t dim)
      : payload_(::std::move(view)), sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Bool), numel_(checked_numel(sizes, dim)) {
    validate_bool_view();
  }

  NoteImpl(LaneView<Int64Buffer> view,
           ::std::array<::std::int64_t, 3> sizes, ::std::int64_t dim)
      : payload_(::std::move(view)), sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Int64), numel_(checked_numel(sizes, dim)) {
    validate_view(::std::get<LaneView<Int64Buffer>>(payload_));
  }

  NoteImpl(LaneView<Float64Buffer> view,
           ::std::array<::std::int64_t, 3> sizes, ::std::int64_t dim)
      : payload_(::std::move(view)), sizes_(sizes), dim_(dim),
        dtype_(NoteDType::Float64), numel_(checked_numel(sizes, dim)) {
    validate_view(::std::get<LaneView<Float64Buffer>>(payload_));
  }

  bool defined() const noexcept { return true; }
  NoteDType dtype() const noexcept { return dtype_; }
  bool is_text() const noexcept { return dtype_ == NoteDType::Text; }
  bool is_bytes() const noexcept { return dtype_ == NoteDType::Bytes; }
  bool is_bool() const noexcept { return dtype_ == NoteDType::Bool; }
  bool is_int64() const noexcept { return dtype_ == NoteDType::Int64; }
  bool is_float64() const noexcept { return dtype_ == NoteDType::Float64; }
  bool is_textual() const noexcept { return is_text() || is_bytes(); }

  ::std::int64_t dim() const noexcept { return dim_; }
  ::std::array<::std::int64_t, 3> sizes3() const noexcept { return sizes_; }
  ::std::span<const ::std::int64_t> sizes() const noexcept {
    return {sizes_.data(), static_cast<::std::size_t>(dim_)};
  }
  ::std::int64_t numel() const noexcept { return numel_; }

  static ::std::int64_t
  shape_numel(const ::std::array<::std::int64_t, 3> &sizes,
              ::std::int64_t dim) {
    return checked_numel(sizes, dim);
  }

  ::std::string_view bytes_at(::std::int64_t linear) const {
    require_textual();
    const auto [off, len] =
        text_storage().spans.at(static_cast<::std::size_t>(linear));
    return text_storage().arena->view(off, len);
  }

  const ::std::vector<OffLen> &spans() const {
    require_textual();
    return text_storage().spans;
  }

  const ::pluss::flame::intrusive_ptr<TextArena> &arena() const {
    require_textual();
    return text_storage().arena;
  }

  ::std::span<const ::std::uint8_t> bool_values() const {
    PLUSS_CHECK(is_bool(), "Note storage does not contain Bool lanes");
    return lane_span(::std::get<LaneView<BoolBuffer>>(payload_));
  }

  ::std::span<const ::std::int64_t> int64_values() const {
    PLUSS_CHECK(is_int64(), "Note storage does not contain Int64 lanes");
    return lane_span(::std::get<LaneView<Int64Buffer>>(payload_));
  }

  ::std::span<const double> float64_values() const {
    PLUSS_CHECK(is_float64(), "Note storage does not contain Float64 lanes");
    return lane_span(::std::get<LaneView<Float64Buffer>>(payload_));
  }

  const LaneView<BoolBuffer> &bool_view() const {
    PLUSS_CHECK(is_bool(), "Note storage does not contain Bool lanes");
    return ::std::get<LaneView<BoolBuffer>>(payload_);
  }
  const LaneView<Int64Buffer> &int64_view() const {
    PLUSS_CHECK(is_int64(), "Note storage does not contain Int64 lanes");
    return ::std::get<LaneView<Int64Buffer>>(payload_);
  }
  const LaneView<Float64Buffer> &float64_view() const {
    PLUSS_CHECK(is_float64(), "Note storage does not contain Float64 lanes");
    return ::std::get<LaneView<Float64Buffer>>(payload_);
  }

private:
  using Payload = ::std::variant<TextStorage, LaneView<BoolBuffer>,
                                 LaneView<Int64Buffer>, LaneView<Float64Buffer>>;

  template <class Buffer>
  ::std::span<const typename Buffer::value_type>
  lane_span(const LaneView<Buffer> &view) const {
    const auto &values = view.buffer->values();
    return ::std::span<const typename Buffer::value_type>(values.data(), values.size())
        .subspan(view.offset, static_cast<::std::size_t>(numel_));
  }

  static ::std::int64_t
  checked_numel(const ::std::array<::std::int64_t, 3> &sizes,
                ::std::int64_t dim) {
    PLUSS_CHECK(dim >= 0 && dim <= 3, "Note supports ranks from 0 through 3");
    ::std::int64_t count = 1;
    for (::std::int64_t axis = 0; axis < dim; ++axis) {
      const auto extent = sizes[static_cast<::std::size_t>(axis)];
      PLUSS_CHECK(extent >= 0, "Note extents must be non-negative");
      if (count != 0) {
        PLUSS_CHECK(extent <=
                        ::std::numeric_limits<::std::int64_t>::max() / count,
                    "Note shape product overflows int64");
      }
      count *= extent;
    }
    return count;
  }

  void validate_lane_count(::std::size_t count) const {
    PLUSS_CHECK(count == static_cast<::std::size_t>(numel_),
                "Note storage lane count must equal shape cardinality");
  }

  template <class Buffer>
  void validate_view(const LaneView<Buffer> &view) const {
    PLUSS_CHECK(view.buffer, "Note lane view requires a backing buffer");
    const auto count = static_cast<::std::size_t>(numel_);
    PLUSS_CHECK(view.offset <= view.buffer->values().size(),
                "Note lane view offset is out of bounds");
    PLUSS_CHECK(count <= view.buffer->values().size() - view.offset,
                "Note lane view extends beyond its backing buffer");
  }

  void validate_bool_view() const {
    const auto &view = ::std::get<LaneView<BoolBuffer>>(payload_);
    validate_view(view);
    const auto lanes = bool_values();
    PLUSS_CHECK(::std::all_of(lanes.begin(), lanes.end(),
                              [](::std::uint8_t value) { return value <= 1; }),
                "Bool Note storage accepts only canonical 0 or 1 lanes");
  }

  void validate_text_storage() const {
    const auto &text = text_storage();
    PLUSS_CHECK(text.arena, "Text and Bytes Note storage requires an arena");
    validate_lane_count(text.spans.size());
    for (const auto [offset, length] : text.spans) {
      PLUSS_CHECK(offset <= text.arena->size(),
                  "Note text span offset is out of bounds");
      PLUSS_CHECK(length <= text.arena->size() - offset,
                  "Note text span length is out of bounds");
    }
  }

  void require_textual() const {
    PLUSS_CHECK(is_textual(), "Note operation requires Text or Bytes storage");
  }

  const TextStorage &text_storage() const {
    PLUSS_CHECK(is_textual(), "Note storage does not contain text cells");
    return ::std::get<TextStorage>(payload_);
  }

  Payload payload_;
  ::std::array<::std::int64_t, 3> sizes_{0, 0, 0};
  ::std::int64_t dim_ = 0;
  NoteDType dtype_ = NoteDType::Text;
  ::std::int64_t numel_ = 0;
};

} // namespace pluss::coden
