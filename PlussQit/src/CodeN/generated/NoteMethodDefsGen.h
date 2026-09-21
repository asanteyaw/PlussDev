#pragma once

// AUTO-GENERATED: inline method definitions via _compat::invoke
// Included outside namespace pluss::coden (see core headers).

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden {

inline Note Note::lower() const {
  return ::pluss::coden::_compat::invoke<Note>("lower", *this);
}

inline Note Note::upper() const {
  return ::pluss::coden::_compat::invoke<Note>("upper", *this);
}

inline Note Note::title() const {
  return ::pluss::coden::_compat::invoke<Note>("title", *this);
}

inline Note Note::capitalize() const {
  return ::pluss::coden::_compat::invoke<Note>("capitalize", *this);
}

inline Note Note::casefold() const {
  return ::pluss::coden::_compat::invoke<Note>("casefold", *this);
}

inline Note Note::swapcase() const {
  return ::pluss::coden::_compat::invoke<Note>("swapcase", *this);
}

inline Note Note::strip() const {
  return ::pluss::coden::_compat::invoke<Note>("strip", *this);
}

inline Note Note::lstrip() const {
  return ::pluss::coden::_compat::invoke<Note>("lstrip", *this);
}

inline Note Note::rstrip() const {
  return ::pluss::coden::_compat::invoke<Note>("rstrip", *this);
}

inline Note Note::collapse_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_whitespace", *this);
}

inline Note Note::dedent() const {
  return ::pluss::coden::_compat::invoke<Note>("dedent", *this);
}

inline Note Note::indent() const {
  return ::pluss::coden::_compat::invoke<Note>("indent", *this);
}

inline Note Note::remove_control_chars() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_control_chars", *this);
}

inline Note Note::keep_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_ascii", *this);
}

inline Note Note::to_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("to_ascii", *this);
}

inline Note Note::to_nfkc() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfkc", *this);
}

inline Note Note::to_nfc() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfc", *this);
}

inline Note Note::to_nfd() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfd", *this);
}

inline Note Note::to_nfkd() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfkd", *this);
}

inline Note Note::remove_punctuation() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_punctuation", *this);
}

inline Note Note::remove_digits() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_digits", *this);
}

inline Note Note::remove_letters() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_letters", *this);
}

inline Note Note::remove_symbols() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_symbols", *this);
}

inline Note Note::remove_emojis() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_emojis", *this);
}

inline Note Note::keep_alnum() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_alnum", *this);
}

inline Note Note::keep_alpha() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_alpha", *this);
}

inline Note Note::keep_digits() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_digits", *this);
}

inline Note Note::keep_printable() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_printable", *this);
}

inline Note Note::normalize_quotes() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_quotes", *this);
}

inline Note Note::normalize_dashes() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_dashes", *this);
}

inline Note Note::normalize_ellipsis() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_ellipsis", *this);
}

inline Note Note::strip_accents() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_accents", *this);
}

inline Note Note::strip_zero_width() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_zero_width", *this);
}

inline Note Note::strip_bom() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_bom", *this);
}

inline Note Note::collapse_repeats() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_repeats", *this);
}

inline Note Note::collapse_punct() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_punct", *this);
}

inline Note Note::collapse_spaces() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_spaces", *this);
}

inline Note Note::collapse_newlines() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_newlines", *this);
}

inline Note Note::strip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("strip", *this, {chars});
}

inline Note Note::lstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("lstrip", *this, {chars});
}

inline Note Note::rstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("rstrip", *this, {chars});
}

inline Note Note::replace(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace", *this, {from, to});
}

inline Note Note::replace_first(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_first", *this, {from, to});
}

inline Note Note::replace_last(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_last", *this, {from, to});
}

inline Note Note::replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_n", *this, {from, to, n});
}

inline Note Note::remove(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("remove", *this, {needle});
}

inline Note Note::remove_all(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_all", *this, {needle});
}

inline Note Note::remove_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_prefix", *this, {prefix});
}

inline Note Note::remove_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_suffix", *this, {suffix});
}

inline Note Note::ensure_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Note>("ensure_prefix", *this, {prefix});
}

inline Note Note::ensure_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Note>("ensure_suffix", *this, {suffix});
}

inline Note Note::surround(::std::string_view left, ::std::string_view right) const {
  return ::pluss::coden::_compat::invoke<Note>("surround", *this, {left, right});
}

inline Note Note::quote() const {
  return ::pluss::coden::_compat::invoke<Note>("quote", *this);
}

inline Note Note::unquote() const {
  return ::pluss::coden::_compat::invoke<Note>("unquote", *this);
}

inline Note Note::casefold(::std::string_view locale) const {
  return ::pluss::coden::_compat::invoke<Note>("casefold", *this, {locale});
}

// Predicate kernels construct their Bool Note directly, preserving shape and
// avoiding an intermediate host vector plus a second output allocation.
inline Note Note::contains(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("contains", *this, {needle});
}

inline Note Note::starts_with(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Note>("starts_with", *this, {prefix});
}

inline Note Note::ends_with(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Note>("ends_with", *this, {suffix});
}

inline Note Note::is_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("is_ascii", *this);
}

inline Note Note::is_utf8_valid() const {
  return ::pluss::coden::_compat::invoke<Note>("is_utf8_valid", *this);
}

inline Note Note::is_blank() const {
  return ::pluss::coden::_compat::invoke<Note>("is_blank", *this);
}

inline Note Note::lengths_bytes() const {
  return ::pluss::coden::_compat::invoke<Note>("lengths_bytes", *this);
}

inline Note Note::lengths_chars() const {
  return ::pluss::coden::_compat::invoke<Note>("lengths_chars", *this);
}

inline Note Note::lengths_graphemes() const {
  return ::pluss::coden::_compat::invoke<Note>("lengths_graphemes", *this);
}

inline TokenBatch Note::split(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, {delim});
}

inline TokenBatch Note::split_whitespace() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this);
}

inline TokenBatch Note::split_lines() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this);
}

inline TokenBatch Note::split_chars() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_chars", *this);
}

inline Note Note::join() const {
  return ::pluss::coden::_compat::invoke<Note>("join", *this);
}

inline Note Note::join(::std::string_view sep) const {
  return ::pluss::coden::_compat::invoke<Note>("join", *this, {sep});
}

inline Note Note::reshape(::pluss::msharp::RangeView::Int sizes) const {
  return ::pluss::coden::_compat::invoke<Note>("reshape", *this, {sizes});
}

inline Note Note::transpose(::std::int64_t dim0, ::std::int64_t dim1) const {
  return ::pluss::coden::_compat::invoke<Note>("transpose", *this, {dim0, dim1});
}

inline Note Note::permute(::pluss::msharp::RangeView::Int dims) const {
  return ::pluss::coden::_compat::invoke<Note>("permute", *this, {dims});
}

inline Note Note::select(::std::int64_t dim, ::std::int64_t index) const {
  return ::pluss::coden::_compat::invoke<Note>("select", *this, {dim, index});
}

inline Note Note::narrow(::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) const {
  return ::pluss::coden::_compat::invoke<Note>("narrow", *this, {dim, start, length});
}

inline Note Note::flatten() const {
  return ::pluss::coden::_compat::invoke<Note>("flatten", *this);
}

inline Note Note::contiguous() const {
  return ::pluss::coden::_compat::invoke<Note>("contiguous", *this);
}

inline Note Note::base64_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("base64_encode", *this);
}

inline Note Note::base64_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("base64_decode", *this);
}

inline Note Note::hex_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("hex_encode", *this);
}

inline Note Note::hex_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("hex_decode", *this);
}

inline Note Note::url_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("url_encode", *this);
}

inline Note Note::url_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("url_decode", *this);
}

inline Note Note::html_escape() const {
  return ::pluss::coden::_compat::invoke<Note>("html_escape", *this);
}

inline Note Note::html_unescape() const {
  return ::pluss::coden::_compat::invoke<Note>("html_unescape", *this);
}

inline Note Note::json_escape() const {
  return ::pluss::coden::_compat::invoke<Note>("json_escape", *this);
}

inline Note Note::is_alpha() const {
  return ::pluss::coden::_compat::invoke<Note>("is_alpha", *this);
}

inline Note Note::is_alnum() const {
  return ::pluss::coden::_compat::invoke<Note>("is_alnum", *this);
}

inline Note Note::is_digit() const {
  return ::pluss::coden::_compat::invoke<Note>("is_digit", *this);
}

inline Note Note::is_lower() const {
  return ::pluss::coden::_compat::invoke<Note>("is_lower", *this);
}

inline Note Note::is_upper() const {
  return ::pluss::coden::_compat::invoke<Note>("is_upper", *this);
}

inline Note Note::is_title() const {
  return ::pluss::coden::_compat::invoke<Note>("is_title", *this);
}

inline Note Note::is_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("is_whitespace", *this);
}

inline Note Note::contains_any_digit() const {
  return ::pluss::coden::_compat::invoke<Note>("contains_any_digit", *this);
}

inline Note Note::contains_any_alpha() const {
  return ::pluss::coden::_compat::invoke<Note>("contains_any_alpha", *this);
}

inline Note Note::count_substring(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("count_substring", *this, {needle});
}

inline TokenBatch Note::split(::std::string_view delim, ::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, {delim, limit});
}

inline TokenBatch Note::split_lines(bool keep_ends) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this, {keep_ends});
}

inline TokenBatch Note::split_whitespace(::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this, {limit});
}

inline Note Note::to_utf8_strict() const {
  return ::pluss::coden::_compat::invoke<Note>("to_utf8_strict", *this);
}

inline Note Note::to_utf8_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_utf8_lossy", *this);
}

inline Note Note::from_latin1() const {
  return ::pluss::coden::_compat::invoke<Note>("from_latin1", *this);
}

inline Note Note::to_latin1_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_latin1_lossy", *this);
}

inline Note Note::from_windows1252() const {
  return ::pluss::coden::_compat::invoke<Note>("from_windows1252", *this);
}

inline Note Note::to_windows1252_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_windows1252_lossy", *this);
}

inline TokenBatch Note::diff_lines(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("diff_lines", *this, {other});
}

inline TokenBatch Note::diff_chars(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("diff_chars", *this, {other});
}

inline Note Note::deduplicate() const {
  return ::pluss::coden::_compat::invoke<Note>("deduplicate", *this);
}

inline Note Note::deduplicate(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("deduplicate", *this, {dim});
}

inline Note Note::sort() const {
  return ::pluss::coden::_compat::invoke<Note>("sort", *this);
}

inline Note Note::sort(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("sort", *this, {dim});
}

inline Note Note::unique() const {
  return ::pluss::coden::_compat::invoke<Note>("unique", *this);
}

inline Note Note::unique(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("unique", *this, {dim});
}

inline Note Note::squeeze(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("squeeze", *this, {dim});
}

inline Note Note::unsqueeze(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("unsqueeze", *this, {dim});
}

inline Note Note::repeat(::pluss::msharp::RangeView::Int repeats) const {
  return ::pluss::coden::_compat::invoke<Note>("repeat", *this, {repeats});
}

inline Note Note::repeat_interleave(::std::int64_t repeats, ::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("repeat_interleave", *this, {repeats, dim});
}

} // namespace pluss::coden
