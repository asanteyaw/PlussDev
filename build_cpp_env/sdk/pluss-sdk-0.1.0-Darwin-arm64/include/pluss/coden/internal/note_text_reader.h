#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/coden/internal/note_factory_detail.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

namespace pluss::coden::detail {

// Normalizes every Note::make/bytes input shape (scalar, 1D list, nested-
// braced N-D, or explicit flat+shape) into one flat pluss::detail::TextSpan +
// pluss::detail::ShapeSpec pair -- the CodeN counterpart to CodeH's
// TimelineTextReader, reusing the same shared TextSpan/ShapeSpec/
// flatten_note_literal machinery (pluss::detail:: is common to both modules).
class NoteTextReader {
 public:
  // Scalar (0D). string_view/u8string_view are non-owning views; safe here
  // because the reader is always consumed synchronously within the same full
  // expression as the pluss::note::make(...)/bytes(...) call that builds it.
  NoteTextReader(std::string_view s) : flat_(std::vector<std::string_view>{s}) {}
  NoteTextReader(const char* s) : NoteTextReader(std::string_view{s ? s : ""}) {}
  // std::string -> std::string_view -> NoteTextReader would chain two
  // user-defined conversions, which C++ won't do implicitly -- needs its own
  // direct constructor (this one also binds std::string&& via the const&).
  NoteTextReader(const std::string& s) : NoteTextReader(std::string_view{s}) {}
#if defined(__cpp_char8_t)
  // Covers std::u8string too (implicit u8string -> u8string_view conversion),
  // both lvalue and rvalue, the same way the std::string& overload above
  // covers std::string&&.
  NoteTextReader(std::u8string_view s)
      : NoteTextReader(std::string_view{reinterpret_cast<const char*>(s.data()), s.size()}) {}
#endif
  // A bare char has no storage of its own to view -- owned_strings_ holds
  // the one-character std::string this constructor builds, matching the
  // nested-literal path's "owned storage first, then flat_ built from it"
  // ordering below.
  NoteTextReader(char c) {
    owned_strings_.assign(1, std::string(1, c));
    flat_ = pluss::detail::TextSpan::from_strings(owned_strings_);
  }

  // 1D. std::span/std::vector each get direct constructors rather than
  // relying on the RangeView ones below -- routing a caller's raw
  // span/vector through RangeView first would chain two user-defined
  // conversions, which C++ won't do implicitly.
  NoteTextReader(std::initializer_list<const char*> s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  NoteTextReader(std::span<const std::string_view> s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  NoteTextReader(const std::vector<std::string_view>& s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  NoteTextReader(const std::vector<std::string>& s1d) {
    assign_1d_(pluss::detail::TextSpan::from_strings(s1d));
  }
  NoteTextReader(pluss::msharp::RangeView::Of<std::string_view> s1d) {
    assign_1d_(pluss::detail::TextSpan(std::span<const std::string_view>(s1d.data(), s1d.size())));
  }
  NoteTextReader(pluss::msharp::RangeView::Of<std::string> s1d) {
    assign_1d_(pluss::detail::TextSpan::from_strings(std::span<const std::string>(s1d.data(), s1d.size())));
  }

  // Nested braces -- shape inferred from nesting depth (0D..3D). Builds
  // *owned* string storage (flatten_note_literal materializes fresh
  // std::strings), which must outlive flat_'s string_views into it --
  // owned_strings_ is populated first, then flat_ is built from it, in that
  // order, in this constructor body.
  NoteTextReader(std::initializer_list<pluss::detail::NestedNoteLiteral> nested) {
    auto lit = pluss::detail::flatten_note_literal(nested, 3, "pluss::note::make");
    owned_strings_ = std::move(lit.flat);
    flat_ = pluss::detail::TextSpan::from_strings(owned_strings_);
    shape_ = pluss::detail::ShapeSpec(std::move(lit.sizes));
  }

  // Explicit flat + shape, two source shapes. Rank-checked via the same
  // checked_numel() the pre-reader factories already used.
  NoteTextReader(pluss::msharp::RangeView::Of<std::string_view> flat,
                pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    flat_ = pluss::detail::TextSpan(std::span<const std::string_view>(flat.data(), flat.size()));
    assign_shape_(sizes);
  }
  NoteTextReader(pluss::msharp::RangeView::Of<std::string> flat,
                pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    flat_ = pluss::detail::TextSpan::from_strings(std::span<const std::string>(flat.data(), flat.size()));
    assign_shape_(sizes);
  }

  const pluss::detail::TextSpan& flat() const noexcept { return flat_; }
  const pluss::detail::ShapeSpec& shape() const noexcept { return shape_; }

 private:
  void assign_1d_(pluss::detail::TextSpan flat) {
    const std::array<std::int64_t, 1> sz{static_cast<std::int64_t>(flat.size())};
    flat_ = std::move(flat);
    shape_ = pluss::detail::ShapeSpec(std::span<const std::int64_t>(sz.data(), sz.size()));
  }
  void assign_shape_(pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    shape_ = pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size()));
    (void)pluss::coden::factory_detail::checked_numel(shape_);
  }

  std::vector<std::string> owned_strings_;  // backing storage for the char/nested-literal paths only
  pluss::detail::TextSpan flat_;
  pluss::detail::ShapeSpec shape_;  // empty = scalar (0D)
};

} // namespace pluss::coden::detail
