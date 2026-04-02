#pragma once

#include "pluss/export.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteBase.h"

namespace pluss::codem {
class Series;
}

namespace pluss::coden {

class Note;

namespace ops_internal {
  // Forward declarations (definitions in OpsInternal.cpp).
  Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes);
  Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes);
} // namespace ops_internal

namespace ops_internal {
struct NoteFactoryAccess;
} // namespace ops_internal

class TokenBatch;

// Note: up to 3D "tensor" of UTF-8 text.
// Patch 9: scalar text is represented as a 0D Note; the internal Chord type is
// no longer part of the public surface.
class CODEN_API Note final : public NoteBase {
public:
  static Note undefined() noexcept { return Note(); }

  // Minimal core API remains implemented in Note.cpp
  ::std::vector<::std::int64_t> sizes() const;
  pluss::msharp::RangeView::Int sizes_view() const;
  ::std::string_view at(::std::int64_t linear) const;
  Note latex_to_text(::std::int64_t mode = 0) const;
  Note text_to_latex(::std::int64_t mode = 0) const;
  Note render_latex(::std::int64_t mode = 0) const;
  Note take(pluss::msharp::RangeView::Int indices) const;

  // Full surface (hundreds of methods + factories)
  #include "CodeN/generated/NoteMethodDeclsGen.h"

  // Text-aware operators.
  // Supported semantics:
  // - `+` : element-wise concatenation with scalar broadcast.
  // - `*` : repeat each element `times` (times >= 0).
  // - comparisons: lexical element-wise compare with scalar broadcast.
  Note operator+(const Note& other) const;
  Note operator+(::std::string_view suffix) const;
  Note& operator+=(const Note& other);
  Note& operator+=(::std::string_view suffix);
  Note operator*(::std::int64_t times) const;
  Note& operator*=(::std::int64_t times);

  ::std::vector<::std::uint8_t> operator==(const Note& other) const;
  ::std::vector<::std::uint8_t> operator!=(const Note& other) const;
  ::std::vector<::std::uint8_t> operator<(const Note& other) const;
  ::std::vector<::std::uint8_t> operator<=(const Note& other) const;
  ::std::vector<::std::uint8_t> operator>(const Note& other) const;
  ::std::vector<::std::uint8_t> operator>=(const Note& other) const;

  ::std::vector<::std::uint8_t> operator==(::std::string_view other) const;
  ::std::vector<::std::uint8_t> operator!=(::std::string_view other) const;
  ::std::vector<::std::uint8_t> operator<(::std::string_view other) const;
  ::std::vector<::std::uint8_t> operator<=(::std::string_view other) const;
  ::std::vector<::std::uint8_t> operator>(::std::string_view other) const;
  ::std::vector<::std::uint8_t> operator>=(::std::string_view other) const;

#ifdef CODEN_ENABLE_SERIES_INTEROP
  // Interop helpers returning CodeM Series.
  pluss::codem::Series contains_series(::std::string_view needle) const;
  pluss::codem::Series lengths_bytes_series() const;
  pluss::codem::Series argsort(bool descending = false, bool stable = true) const;
#endif

private:
  Note() = default;

  explicit Note(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : NoteBase(::std::move(impl)) {}

  friend struct ops_internal::NoteFactoryAccess;
};

Note operator+(::std::string_view prefix, const Note& rhs);
Note operator*(::std::int64_t times, const Note& rhs);

::std::vector<::std::uint8_t> operator==(::std::string_view lhs, const Note& rhs);
::std::vector<::std::uint8_t> operator!=(::std::string_view lhs, const Note& rhs);
::std::vector<::std::uint8_t> operator<(::std::string_view lhs, const Note& rhs);
::std::vector<::std::uint8_t> operator<=(::std::string_view lhs, const Note& rhs);
::std::vector<::std::uint8_t> operator>(::std::string_view lhs, const Note& rhs);
::std::vector<::std::uint8_t> operator>=(::std::string_view lhs, const Note& rhs);


} // namespace pluss::coden

// Inline method definitions are included outside the namespace to avoid
// accidental nested-namespace pollution via transitive includes.
#include "CodeN/generated/NoteMethodDefsGen.h"
