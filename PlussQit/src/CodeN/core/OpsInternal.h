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
Chord upper(const Chord& x);
Chord title(const Chord& x);
Chord capitalize(const Chord& x);
Chord swapcase(const Chord& x);
Chord strip(const Chord& x);
Chord lstrip(const Chord& x);
Chord rstrip(const Chord& x);
Chord trim(const Chord& x);
Chord trim_left(const Chord& x);
Chord trim_right(const Chord& x);
Chord collapse_whitespace(const Chord& x);
Chord dedent(const Chord& x);
Chord indent(const Chord& x);
Chord normalize_spaces(const Chord& x);
Chord remove_control_chars(const Chord& x);
Chord remove_non_ascii(const Chord& x);
Chord to_ascii(const Chord& x);
Chord remove_punctuation(const Chord& x);
Chord remove_digits(const Chord& x);
Chord remove_letters(const Chord& x);
Chord remove_symbols(const Chord& x);
Chord remove_emojis(const Chord& x);
Chord keep_alnum(const Chord& x);
Chord keep_alpha(const Chord& x);
Chord keep_digits(const Chord& x);
Chord keep_ascii(const Chord& x);
Chord keep_printable(const Chord& x);
Chord normalize_quotes(const Chord& x);
Chord normalize_dashes(const Chord& x);
Chord normalize_ellipsis(const Chord& x);
Chord strip_accents(const Chord& x);
Chord strip_diacritics(const Chord& x);
Chord strip_zero_width(const Chord& x);
Chord strip_bom(const Chord& x);
Chord collapse_repeats(const Chord& x);
Chord collapse_punct(const Chord& x);
Chord collapse_spaces(const Chord& x);
Chord collapse_newlines(const Chord& x);
Chord nlp_clean(const Chord& x);
Chord nlp_normalize_punct(const Chord& x, bool ascii_only = false);
Chord nlp_remove_urls(const Chord& x);
Chord nlp_remove_emails(const Chord& x);
Chord nlp_remove_mentions(const Chord& x);
Chord nlp_remove_hashtags(const Chord& x);
Chord nlp_remove_html(const Chord& x);
Chord nlp_remove_markdown(const Chord& x);
Chord nlp_remove_code(const Chord& x);
Chord nlp_remove_math(const Chord& x);
Chord nlp_remove_latex(const Chord& x);
Chord nlp_remove_brackets(const Chord& x);
Chord nlp_remove_parentheticals(const Chord& x);
Chord nlp_remove_quotes(const Chord& x);
Chord nlp_remove_bullets(const Chord& x);
Chord nlp_remove_headers(const Chord& x);
Chord nlp_remove_footers(const Chord& x);
Chord nlp_remove_boilerplate(const Chord& x);
Chord nlp_remove_tables(const Chord& x);

Chord strip(const Chord& x, ::std::string_view chars);
Chord lstrip(const Chord& x, ::std::string_view chars);
Chord rstrip(const Chord& x, ::std::string_view chars);
Chord trim(const Chord& x, ::std::string_view chars);
Chord trim_left(const Chord& x, ::std::string_view chars);
Chord trim_right(const Chord& x, ::std::string_view chars);

bool contains(const Chord& x, ::std::string_view needle);
Chord replace(const Chord& x, ::std::string_view from, ::std::string_view to);
Chord replace_first(const Chord& x, ::std::string_view from, ::std::string_view to);
Chord replace_last(const Chord& x, ::std::string_view from, ::std::string_view to);
Chord replace_n(const Chord& x, ::std::string_view from, ::std::string_view to, ::std::int64_t n);
Chord remove(const Chord& x, ::std::string_view needle);
Chord remove_all(const Chord& x, ::std::string_view needle);
Chord remove_prefix(const Chord& x, ::std::string_view prefix);
Chord remove_suffix(const Chord& x, ::std::string_view suffix);
Chord ensure_prefix(const Chord& x, ::std::string_view prefix);
Chord ensure_suffix(const Chord& x, ::std::string_view suffix);
Chord surround(const Chord& x, ::std::string_view left, ::std::string_view right);
Chord quote(const Chord& x);
Chord unquote(const Chord& x);

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
TokenBatch split_lines(const Chord& x, bool keep_ends = false);
TokenBatch split_chars(const Chord& x);
TokenBatch split_graphemes(const Chord& x);
TokenBatch tokenize_words(const Chord& x, ::std::int64_t flags = 0);
TokenBatch tokenize_sentences(const Chord& x, ::std::int64_t flags = 0);
TokenBatch tokenize_punct(const Chord& x);
TokenBatch tokenize_regex(const Chord& x, ::std::string_view pattern);
TokenBatch regex_findall(const Chord& x, ::std::string_view pattern, ::std::int64_t flags = 0);
TokenBatch regex_split(const Chord& x, ::std::string_view pattern, ::std::int64_t limit = -1);

// TeX cleanup family (scalar semantics).
Chord tex_strip_comments(const Chord& x);
Chord tex_strip_preamble(const Chord& x);
Chord tex_remove_commands(const Chord& x);
Chord tex_to_text(const Chord& x, ::std::int64_t mode = 0);
Chord tex_sanitize(const Chord& x, ::std::int64_t level = 0);

// ---- Note ops ----
Note lower(const Note& x);
Note upper(const Note& x);
Note title(const Note& x);
Note capitalize(const Note& x);
Note swapcase(const Note& x);
Note strip(const Note& x);
Note lstrip(const Note& x);
Note rstrip(const Note& x);
Note trim(const Note& x);
Note trim_left(const Note& x);
Note trim_right(const Note& x);
Note collapse_whitespace(const Note& x);
Note dedent(const Note& x);
Note indent(const Note& x);
Note normalize_spaces(const Note& x);
Note remove_control_chars(const Note& x);
Note remove_non_ascii(const Note& x);
Note to_ascii(const Note& x);
Note remove_punctuation(const Note& x);
Note remove_digits(const Note& x);
Note remove_letters(const Note& x);
Note remove_symbols(const Note& x);
Note remove_emojis(const Note& x);
Note keep_alnum(const Note& x);
Note keep_alpha(const Note& x);
Note keep_digits(const Note& x);
Note keep_ascii(const Note& x);
Note keep_printable(const Note& x);
Note normalize_quotes(const Note& x);
Note normalize_dashes(const Note& x);
Note normalize_ellipsis(const Note& x);
Note strip_accents(const Note& x);
Note strip_diacritics(const Note& x);
Note strip_zero_width(const Note& x);
Note strip_bom(const Note& x);
Note collapse_repeats(const Note& x);
Note collapse_punct(const Note& x);
Note collapse_spaces(const Note& x);
Note collapse_newlines(const Note& x);
Note nlp_clean(const Note& x);
Note nlp_normalize_punct(const Note& x, bool ascii_only = false);
Note nlp_remove_urls(const Note& x);
Note nlp_remove_emails(const Note& x);
Note nlp_remove_mentions(const Note& x);
Note nlp_remove_hashtags(const Note& x);
Note nlp_remove_html(const Note& x);
Note nlp_remove_markdown(const Note& x);
Note nlp_remove_code(const Note& x);
Note nlp_remove_math(const Note& x);
Note nlp_remove_latex(const Note& x);
Note nlp_remove_brackets(const Note& x);
Note nlp_remove_parentheticals(const Note& x);
Note nlp_remove_quotes(const Note& x);
Note nlp_remove_bullets(const Note& x);
Note nlp_remove_headers(const Note& x);
Note nlp_remove_footers(const Note& x);
Note nlp_remove_boilerplate(const Note& x);
Note nlp_remove_tables(const Note& x);

Note strip(const Note& x, ::std::string_view chars);
Note lstrip(const Note& x, ::std::string_view chars);
Note rstrip(const Note& x, ::std::string_view chars);
Note trim(const Note& x, ::std::string_view chars);
Note trim_left(const Note& x, ::std::string_view chars);
Note trim_right(const Note& x, ::std::string_view chars);

Note replace(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_first(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_last(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_n(const Note& x, ::std::string_view from, ::std::string_view to, ::std::int64_t n);
Note remove(const Note& x, ::std::string_view needle);
Note remove_all(const Note& x, ::std::string_view needle);
Note remove_prefix(const Note& x, ::std::string_view prefix);
Note remove_suffix(const Note& x, ::std::string_view suffix);
Note ensure_prefix(const Note& x, ::std::string_view prefix);
Note ensure_suffix(const Note& x, ::std::string_view suffix);
Note surround(const Note& x, ::std::string_view left, ::std::string_view right);
Note quote(const Note& x);
Note unquote(const Note& x);
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
TokenBatch split_lines(const Note& x, bool keep_ends = false);
TokenBatch split_chars(const Note& x);
TokenBatch split_graphemes(const Note& x);
TokenBatch tokenize_words(const Note& x, ::std::int64_t flags = 0);
TokenBatch tokenize_sentences(const Note& x, ::std::int64_t flags = 0);
TokenBatch tokenize_punct(const Note& x);
TokenBatch tokenize_regex(const Note& x, ::std::string_view pattern);
TokenBatch regex_findall(const Note& x, ::std::string_view pattern, ::std::int64_t flags = 0);
TokenBatch regex_split(const Note& x, ::std::string_view pattern, ::std::int64_t limit = -1);

Note tex_strip_comments(const Note& x);
Note tex_strip_preamble(const Note& x);
Note tex_remove_commands(const Note& x);
Note tex_to_text(const Note& x, ::std::int64_t mode = 0);
Note tex_sanitize(const Note& x, ::std::int64_t level = 0);

// Join flattened tokens into a scalar (0D) Note.
Note join(const Note& x, ::std::string_view sep = {});

} // namespace pluss::coden::ops_internal
