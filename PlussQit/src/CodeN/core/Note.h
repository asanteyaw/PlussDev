#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteBase.h"

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

class Chord;
struct TokenBatch;

// Note: up to 3D "tensor" of UTF-8 text.
// Patch 9: scalar text is represented as a 0D Note; the internal Chord type is
// no longer part of the public surface.
class CODEN_API Note final : public NoteBase {
public:
  Note() = default;

  // Minimal core API remains implemented in Note.cpp
  ::std::vector<::std::int64_t> sizes() const;
  ::std::string_view at(::std::int64_t linear) const;

  // ---- Factories (preferred construction path) ----
  // (Also available as free functions in include/pluss/coden.h: pluss::content / pluss::bytes_content)
  static Note from_utf8(::std::span<const ::std::string_view> utf8_flat,
                        ::std::span<const ::std::int64_t> sizes) {
    return ::pluss::coden::ops_internal::note_from_utf8(utf8_flat, sizes);
  }

  static Note from_bytes(::std::span<const ::std::string_view> bytes_flat,
                         ::std::span<const ::std::int64_t> sizes) {
    return ::pluss::coden::ops_internal::note_from_bytes(bytes_flat, sizes);
  }

  static Note from_text(::std::string_view utf8) {
    const ::std::array<::std::string_view, 1> flat{utf8};
    return from_utf8(flat, {});
  }
  static Note from_text(::std::u8string_view utf8) {
    return from_text(::std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
  }
  static Note from_text(const char* utf8) { return from_text(::std::string_view{utf8 ? utf8 : ""}); }
  static Note from_text(char c) { ::std::string s(1, c); return from_text(::std::string_view{s}); }

  static Note from_text_1d(::std::span<const ::std::string_view> utf8_1d) {
    const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(utf8_1d.size())};
    return from_utf8(utf8_1d, sizes);
  }

  static Note from_bytes_scalar(::std::string_view bytes) {
    const ::std::array<::std::string_view, 1> flat{bytes};
    return from_bytes(flat, {});
  }

  // Full surface (hundreds of methods + factories)
  #include "CodeN/generated/NoteMethodDeclsGen.h"

#ifdef CODEN_ENABLE_SERIES_INTEROP
  // Interop helpers returning CodeM Series.
  pluss::codem::Series contains_series(::std::string_view needle) const;
  pluss::codem::Series lengths_bytes_series() const;
#endif

private:
  explicit Note(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : NoteBase(::std::move(impl)) {}

  friend struct ops_internal::NoteFactoryAccess;
};


} // namespace pluss::coden

// Inline method definitions are included outside the namespace to avoid
// accidental nested-namespace pollution via transitive includes.
#include "CodeN/generated/NoteMethodDefsGen.h"
