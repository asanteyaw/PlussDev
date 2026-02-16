#pragma once

// Public CodeN entry point (structured includes).
// End users see Note (+ TokenBatch). Chord is internal.

#include <cstdint>
#include <span>
#include <string_view>

#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden::ops_internal {
  // Internal factories (implemented in CodeN). Public factories forward to these.
  Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                      ::std::span<const ::std::int64_t> sizes);

  // Bytes-mode factory (no UTF-8 guarantees).
  Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                       ::std::span<const ::std::int64_t> sizes);
} // namespace pluss::coden::ops_internal
