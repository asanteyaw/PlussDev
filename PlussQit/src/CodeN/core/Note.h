#pragma once

#include "pluss/export.h"
#include "MSharp/RangeView.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteBase.h"
#include "CodeN/core/NoteNlp.h"
#include "CodeN/core/NoteRegex.h"
#include "CodeN/core/NoteTex.h"
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
  Note note_from_bool(::std::span<const ::std::uint8_t> values,
                      ::std::span<const ::std::int64_t> sizes);
  Note note_from_int64(::std::span<const ::std::int64_t> values,
                       ::std::span<const ::std::int64_t> sizes);
  Note note_from_float64(::std::span<const double> values,
                         ::std::span<const ::std::int64_t> sizes);
} // namespace ops_internal

namespace ops_internal {
struct NoteFactoryAccess;
} // namespace ops_internal

class TokenBatch;

// Note is a rank-0 through rank-3 homogeneous value with Text, Bytes, Bool,
// Int64, or Float64 lanes. TextRef is the owning single-cell text boundary;
// native lanes use the typed item and contiguous-span accessors on NoteBase.
class CODEN_API Note final : public NoteBase {
public:
  static Note undefined() noexcept { return Note(); }

  // Minimal core API remains implemented in Note.cpp
  ::std::vector<::std::int64_t> sizes() const;
  ::std::span<const ::std::int64_t> sizes_view() const;
  ::std::string_view at(::std::int64_t linear) const;

  // Stable, versioned binary representation. Unlike print()/host(), pack()
  // preserves dtype, rank, shape, scalar identity, and every payload bit.
  ::std::vector<::std::byte> pack() const;

  // Owning per-cell access. `cell(i)` and `cell_at({i,j,...})` return a TextRef
  // that co-owns the backing storage, so the result stays valid after this
  // Note is destroyed (unlike the borrowed view from `at`). `owned()` is the
  // keep-past-lifetime form of `scalar()` and requires exactly one element.
  TextRef cell(::std::int64_t linear) const;
  TextRef cell_at(::std::span<const ::std::int64_t> indices) const;
  TextRef owned() const;

  Note take(pluss::msharp::RangeView::Int indices) const;
  Note take(const Note& indices) const;

  // Ordering positions: the permutation of linear indices (0..numel()-1)
  // that would sort this Note's cells, as a 1-D Int64 Note -- closed over
  // Note itself, no CodeM/Series dependency required. 1D/scalar Notes only
  // (matches ops_internal::note_argsort_indices_1d).
  Note argsort(bool descending = false, bool stable = true) const;

  // Full surface (hundreds of methods + factories)
  #include "CodeN/generated/NoteMethodDeclsGen.h"

  // Text-aware operators.
  // Supported semantics:
  // - `+` : element-wise concatenation with scalar broadcast.
  // - `*` : repeat each element `times` (times >= 0).
  // - comparisons: dtype-aware element-wise compare with scalar broadcast.
  Note operator+(const Note& other) const;
  Note operator+(::std::string_view suffix) const;
  Note& operator+=(const Note& other);
  Note& operator+=(::std::string_view suffix);
  Note operator*(::std::int64_t times) const;
  Note& operator*=(::std::int64_t times);

  Note operator==(const Note& other) const;
  Note operator!=(const Note& other) const;
  Note operator<(const Note& other) const;
  Note operator<=(const Note& other) const;
  Note operator>(const Note& other) const;
  Note operator>=(const Note& other) const;

  Note operator==(::std::string_view other) const;
  Note operator!=(::std::string_view other) const;
  Note operator<(::std::string_view other) const;
  Note operator<=(::std::string_view other) const;
  Note operator>(::std::string_view other) const;
  Note operator>=(::std::string_view other) const;

  // Bitwise-style logical combinators over Bool Notes. Shape-preserving with
  // the same scalar-broadcast rule as the comparison operators above (either
  // operand may be a single-element Note broadcast against the other's
  // shape). Both operands must be Bool Notes.
  Note operator&(const Note& other) const;
  Note operator|(const Note& other) const;
  Note operator^(const Note& other) const;
  Note operator~() const;

  // Full reductions stay in the closed family as scalar Bool Notes.
  Note any() const;
  Note all() const;

  // Linguistic analysis and corpus-cleanup namespace. Returning this small
  // proxy by value keeps Note itself a one-pointer intrusive handle.
  NoteNlpAccessor nlp() const noexcept { return NoteNlpAccessor(*this); }

  // Pattern matching, extraction, substitution, and splitting.
  NoteRegexAccessor regex() const noexcept { return NoteRegexAccessor(*this); }

  // TeX/LaTeX-source cleanup, extraction, conversion, and rendering.
  NoteTexAccessor tex() const noexcept { return NoteTexAccessor(*this); }

private:
  Note() = default;

  explicit Note(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : NoteBase(::std::move(impl)) {}

  friend struct ops_internal::NoteFactoryAccess;
};

Note operator+(::std::string_view prefix, const Note& rhs);
Note operator*(::std::int64_t times, const Note& rhs);

Note operator==(::std::string_view lhs, const Note& rhs);
Note operator!=(::std::string_view lhs, const Note& rhs);
Note operator<(::std::string_view lhs, const Note& rhs);
Note operator<=(::std::string_view lhs, const Note& rhs);
Note operator>(::std::string_view lhs, const Note& rhs);
Note operator>=(::std::string_view lhs, const Note& rhs);


} // namespace pluss::coden

// Inline method definitions are included outside the namespace to avoid
// accidental nested-namespace pollution via transitive includes.
#include "CodeN/generated/NoteMethodDefsGen.h"
