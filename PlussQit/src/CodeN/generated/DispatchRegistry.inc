// Generated registry of supported CodeN ops.
// Used only for better error messages.

#pragma once

#include <string_view>

namespace pluss::coden::_compat {

struct OpSig {
  ::std::string_view name;
  ::std::string_view pretty;
};

inline constexpr OpSig kOpSigs[] = {
  {"lower", "Chord lower()"},
  {"lower", "Note lower()"},
  {"strip", "Chord strip()"},
  {"strip", "Note strip()"},

  {"contains", "bool Chord::contains(string)"},
  {"contains", "u8list Note::contains(string)"},

  {"replace", "Chord replace(string from, string to)"},

  {"reshape", "Note reshape(IntList sizes)"},
  {"transpose", "Note transpose(i64 dim0, i64 dim1)"},
  {"permute", "Note permute(IntList dims)"},
  {"select", "Note select(i64 dim, i64 idx)"},
  {"narrow", "Note narrow(i64 dim, i64 start, i64 len)"},

  // Patch8: Unicode-ish
  {"normalize", "Chord normalize(i64 form)"},
  {"normalize", "Note normalize(i64 form)"},
  {"casefold", "Chord casefold()"},
  {"casefold", "Chord casefold(string locale)"},
  {"casefold", "Note casefold()"},
  {"casefold", "Note casefold(string locale)"},

  // Patch8: Regex
  {"regex_match", "bool Chord regex_match(string pattern)"},
  {"regex_search", "bool Chord regex_search(string pattern)"},
  {"regex_replace", "Chord regex_replace(string pattern, string repl)"},
  {"regex_match", "u8list Note regex_match(string pattern)"},
  {"regex_search", "u8list Note regex_search(string pattern)"},
  {"regex_replace", "Note regex_replace(string pattern, string repl)"},

  // Patch8: split/join
  {"split_whitespace", "TokenBatch Chord split_whitespace()"},
  {"split_whitespace", "TokenBatch Note split_whitespace()"},
  {"split", "TokenBatch Chord split(string delim)"},
  {"split", "TokenBatch Note split(string delim)"},
  {"join", "Note join()"},
  {"join", "Note join(string sep)"},

  {"lengths_bytes", "i64list Note lengths_bytes()"},
};

} // namespace pluss::coden::_compat
