#include "CodeN/core/Errors.h"
#include "CodeN/native/cpu/TextKernels.h"

#include <cctype>
#include <string>
#include <utility>

namespace pluss::coden::native::cpu {

::std::string lower_ascii(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (c >= 'A' && c <= 'Z') out.push_back(static_cast<char>(c - 'A' + 'a'));
    else out.push_back(static_cast<char>(c));
  }
  return out;
}

static inline bool is_space_ascii(unsigned char c) {
  switch (c) {
    case ' ': case '\t': case '\n': case '\r': case '\f': case '\v':
      return true;
    default:
      return false;
  }
}

::std::string strip_ascii(::std::string_view s) {
  ::std::size_t b = 0;
  ::std::size_t e = s.size();
  while (b < e && is_space_ascii(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && is_space_ascii(static_cast<unsigned char>(s[e - 1]))) --e;
  return ::std::string(s.substr(b, e - b));
}

bool contains_bytes(::std::string_view hay, ::std::string_view needle) {
  if (needle.empty()) return true;
  return hay.find(needle) != ::std::string_view::npos;
}

::std::string replace_bytes(::std::string_view s, ::std::string_view from, ::std::string_view to) {
  if (from.empty()) return ::std::string(s);
  ::std::string out;
  ::std::size_t pos = 0;
  while (true) {
    ::std::size_t p = s.find(from, pos);
    if (p == ::std::string_view::npos) {
      out.append(s.substr(pos));
      break;
    }
    out.append(s.substr(pos, p - pos));
    out.append(to);
    pos = p + from.size();
  }
  return out;
}

::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_whitespace_spans(::std::string_view s) {
  ::std::vector<::std::pair<::std::size_t, ::std::size_t>> out;
  const ::std::size_t n = s.size();
  ::std::size_t i = 0;

  auto is_ws = [](unsigned char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  };

  while (i < n) {
    while (i < n && is_ws(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= n) break;
    const ::std::size_t start = i;
    while (i < n && !is_ws(static_cast<unsigned char>(s[i]))) ++i;
    out.emplace_back(start, i - start);
  }
  return out;
}

} // namespace pluss::coden::native::cpu

::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_delim_spans(::std::string_view s, ::std::string_view delim) {
  ::std::vector<::std::pair<::std::size_t, ::std::size_t>> out;
  PLUSS_CHECK(!delim.empty(), "split_delim_spans: delimiter must be non-empty");

  ::std::size_t pos = 0;
  while (pos <= s.size()) {
    const ::std::size_t p = s.find(delim, pos);
    if (p == ::std::string_view::npos) {
      out.emplace_back(pos, s.size() - pos);
      break;
    }
    out.emplace_back(pos, p - pos);
    pos = p + delim.size();
  }
  return out;
}

