#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "CodeN/core/Chord.h"
#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatch.h"

#ifdef CODEN_ENABLE_SERIES_INTEROP
#include "CodeM/core/SeriesSlim.h"
#endif

namespace pluss::coden::ops_internal {

// Internal accessors for Base implementations (keeps impl() protected).
struct ChordAccess {
  static const ::pluss::flame::intrusive_ptr<ChordImpl>& impl(const Chord& x) noexcept { return x.impl(); }
  static ::pluss::flame::intrusive_ptr<ChordImpl>& impl(Chord& x) noexcept { return x.impl(); }
};
struct NoteAccess {
  static const ::pluss::flame::intrusive_ptr<NoteImpl>& impl(const Note& x) noexcept { return x.impl(); }
  static ::pluss::flame::intrusive_ptr<NoteImpl>& impl(Note& x) noexcept { return x.impl(); }
};

// ---- Factories ----
Chord chord_from_utf8(::std::string_view utf8);
Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes);
Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes);

// ---- Chord ops ----
Chord lower(const Chord& x);
Chord strip(const Chord& x);
bool contains(const Chord& x, ::std::string_view needle);
Chord replace(const Chord& x, ::std::string_view from, ::std::string_view to);

// ICU-optional Unicode-ish surface (no-op / ASCII fallback when ICU is absent).
// form: 0=NFC, 1=NFD, 2=NFKC, 3=NFKD
Chord normalize(const Chord& x, ::std::int64_t form);
Chord casefold(const Chord& x, ::std::string_view locale = {});

// Regex hooks (std::regex fallback; ICU optional later).
bool regex_match(const Chord& x, ::std::string_view pattern);
bool regex_search(const Chord& x, ::std::string_view pattern);
Chord regex_replace(const Chord& x, ::std::string_view pattern, ::std::string_view repl);

// Tokenization / splitting.
TokenBatch split_whitespace(const Chord& x);
TokenBatch split(const Chord& x, ::std::string_view delim);

// ---- Note ops ----
Note lower(const Note& x);
Note strip(const Note& x);
Note replace(const Note& x, ::std::string_view from, ::std::string_view to);
::std::vector<::std::uint8_t> contains(const Note& x, ::std::string_view needle);
::std::vector<::std::int64_t> lengths_bytes(const Note& x);
Note reshape(const Note& x, ::std::span<const ::std::int64_t> sizes);
Note transpose(const Note& x, ::std::int64_t dim0, ::std::int64_t dim1);
Note permute(const Note& x, ::std::span<const ::std::int64_t> dims);
Note select(const Note& x, ::std::int64_t dim, ::std::int64_t index);
Note narrow(const Note& x, ::std::int64_t dim, ::std::int64_t start, ::std::int64_t length);

Note normalize(const Note& x, ::std::int64_t form);
Note casefold(const Note& x, ::std::string_view locale = {});

::std::vector<::std::uint8_t> regex_match(const Note& x, ::std::string_view pattern);
::std::vector<::std::uint8_t> regex_search(const Note& x, ::std::string_view pattern);
Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl);

TokenBatch split_whitespace(const Note& x);
TokenBatch split(const Note& x, ::std::string_view delim);

// Join flattened tokens into a single Chord.
Chord join(const Note& x, ::std::string_view sep = {});

} // namespace pluss::coden::ops_internal
