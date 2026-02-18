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

class TokenBatch;

// Note: up to 3D "tensor" of UTF-8 text.
// Patch 9: scalar text is represented as a 0D Note; the internal Chord type is
// no longer part of the public surface.
class CODEN_API Note final : public NoteBase {
public:
  static Note undefined() noexcept { return Note(); }

  // Minimal core API remains implemented in Note.cpp
  ::std::vector<::std::int64_t> sizes() const;
  ::std::string_view at(::std::int64_t linear) const;

  // Full surface (hundreds of methods + factories)
  #include "CodeN/generated/NoteMethodDeclsGen.h"

#ifdef CODEN_ENABLE_SERIES_INTEROP
  // Interop helpers returning CodeM Series.
  pluss::codem::Series contains_series(::std::string_view needle) const;
  pluss::codem::Series lengths_bytes_series() const;
#endif

private:
  Note() = default;

  explicit Note(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : NoteBase(::std::move(impl)) {}

  friend struct ops_internal::NoteFactoryAccess;
};


} // namespace pluss::coden

// Inline method definitions are included outside the namespace to avoid
// accidental nested-namespace pollution via transitive includes.
#include "CodeN/generated/NoteMethodDefsGen.h"
