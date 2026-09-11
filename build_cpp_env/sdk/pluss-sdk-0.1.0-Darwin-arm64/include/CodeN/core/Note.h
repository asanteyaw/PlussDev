#pragma once

#include "pluss/export.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteBase.h"
#include "CodeN/core/TextRef.h"

namespace pluss::codem {
class Series;
}

namespace pluss::coden {

class Note;

namespace ops_internal {
  // Forward declarations (definitions in OpsInternal.cpp).
  Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes);
  Note note_from_utf8_packed(::std::vector<char>&& utf8_arena,
                             ::std::vector<::std::pair<::std::size_t, ::std::size_t>>&& spans,
                             ::std::span<const ::std::int64_t> sizes);
  Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes);
} // namespace ops_internal

namespace ops_internal {
struct NoteFactoryAccess;
} // namespace ops_internal

class TokenBatch;

// Note: up to 3D "tensor" of UTF-8 text. Scalar text is a 0-D Note; use
// cell(i) / owned() for an owning per-cell handle (TextRef).
class CODEN_API Note final : public NoteBase {
public:
  static Note undefined() noexcept { return Note(); }

  // Minimal core API remains implemented in Note.cpp
  ::std::vector<::std::int64_t> sizes() const;
  pluss::msharp::RangeView::Int sizes_view() const;
  ::std::string_view at(::std::int64_t linear) const;

  // Owning per-cell access. `cell(i)` and `cell_at({i,j,...})` return a TextRef
  // that co-owns the backing storage, so the result stays valid after this
  // Note is destroyed (unlike the borrowed view from `at`). `owned()` is the
  // keep-past-lifetime form of `scalar()` and requires exactly one element.
  TextRef cell(::std::int64_t linear) const;
  TextRef cell_at(::std::span<const ::std::int64_t> indices) const;
  TextRef owned() const;

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
