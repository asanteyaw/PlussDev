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
  {"lower", "Note lower()"},
  {"strip", "Note strip()"},

  {"contains", "Note Note::contains(string)"},


  {"reshape", "Note reshape(IntList sizes)"},
  {"transpose", "Note transpose(i64 dim0, i64 dim1)"},
  {"permute", "Note permute(IntList dims)"},
  {"select", "Note select(i64 dim, i64 idx)"},
  {"narrow", "Note narrow(i64 dim, i64 start, i64 len)"},

  // Patch8: Unicode-ish
  {"casefold", "Note casefold()"},
  {"casefold", "Note casefold(string locale)"},

  // Patch8: Regex
  {"regex_match", "Note Note regex_match(string pattern)"},
  {"regex_search", "Note Note regex_search(string pattern)"},
  {"regex_replace", "Note regex_replace(string pattern, string repl)"},

  // Patch8: split/join
  {"split_whitespace", "TokenBatch Note split_whitespace()"},
  {"split", "TokenBatch Note split(string delim)"},
  {"join", "Note join()"},
  {"join", "Note join(string sep)"},

  {"lengths_bytes", "i64list Note lengths_bytes()"},
};

} // namespace pluss::coden::_compat
