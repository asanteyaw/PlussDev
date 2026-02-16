#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace pluss::coden::native::cpu {

// ASCII-only lowercasing.
::std::string lower_ascii(::std::string_view s);

// ASCII whitespace strip.
::std::string strip_ascii(::std::string_view s);

// Byte substring contains.
bool contains_bytes(::std::string_view hay, ::std::string_view needle);

// Byte replace (non-overlapping, left-to-right).
::std::string replace_bytes(::std::string_view s, ::std::string_view from, ::std::string_view to);

// ASCII whitespace split, returning (offset,len) spans into the input view.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_whitespace_spans(::std::string_view s);

// Split by an explicit delimiter (non-empty), returning (offset,len) spans into the input view.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_delim_spans(::std::string_view s, ::std::string_view delim);


} // namespace pluss::coden::native::cpu
