#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "pluss/coden/coden.h"
#include "pluss/coden/internal/note_factory_detail.h"

namespace pluss::coden::detail {

inline bool is_ascii_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline std::string_view trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && is_ascii_space(static_cast<unsigned char>(s[b]))) {
    ++b;
  }
  while (e > b && is_ascii_space(static_cast<unsigned char>(s[e - 1]))) {
    --e;
  }
  return s.substr(b, e - b);
}

inline std::string normalize_ascii_whitespace(std::string_view s, bool trim, bool collapse_whitespace) {
  auto in = trim ? trim_ascii(s) : s;
  if (!collapse_whitespace) {
    return std::string(in);
  }

  std::string out;
  out.reserve(in.size());
  bool prev_space = false;
  for (const unsigned char c : in) {
    if (is_ascii_space(c)) {
      if (!prev_space) {
        out.push_back(' ');
      }
      prev_space = true;
      continue;
    }
    out.push_back(static_cast<char>(c));
    prev_space = false;
  }
  return out;
}

} // namespace pluss::coden::detail

// pluss::note::content(...) is not a general-purpose N-D constructor the way
// pluss::note::make(...) is -- it takes exactly one piece of raw text and
// derives the resulting Note's shape/content from it under a ContentOptions
// policy: trim/collapse whitespace (Normalize, the default), split into
// lines or words (delegating to pluss::note::from_lines/from_words -- see
// note_factory.h -- so the actual splitting logic lives in exactly one
// place), or pass through untouched (Raw). There is no `content` overload
// for the RangeView/nested-literal input shapes make() already covers,
// since those forms carry no whitespace/splitting policy to apply in the
// first place -- they would just be a second spelling of make() with
// nothing added. Likewise, there's no separate content_lines()/
// content_words(): they'd just be content(text, {.mode = Lines/Words, ...})
// under a different name, and pluss::note::from_lines/from_words already
// are that name (with the two extra ContentMode-only knobs Lines/Words need
// -- keep_line_endings, drop_empty -- as their own named parameters instead
// of a ContentOptions struct).
namespace pluss::note {

enum class ContentMode : std::uint8_t {
  Normalize = 0,
  Lines = 1,
  Words = 2,
  Raw = 3,
};

struct ContentOptions {
  ContentMode mode{ContentMode::Normalize};
  bool trim{true};
  bool collapse_whitespace{true};
  bool drop_empty{true};
  bool keep_line_endings{false};
};

// Forward declarations -- defined in note_factory.h, included after this
// file there. content()'s Lines/Words branches call these directly (not the
// other way around) so note_factory.h doesn't need to include this file.
pluss::Note from_lines(std::string_view text, bool keep_ends, bool drop_empty);
pluss::Note from_words(std::string_view text);

inline pluss::Note content(std::string_view utf8, const ContentOptions& options = {}) {
  if (options.mode == ContentMode::Raw) {
    const std::array<std::string_view, 1> flat{utf8};
    return pluss::coden::ops_internal::note_from_utf8(flat, {});
  }

  if (options.mode == ContentMode::Normalize) {
    const auto normalized =
        pluss::coden::detail::normalize_ascii_whitespace(utf8, options.trim, options.collapse_whitespace);
    const std::array<std::string_view, 1> flat{normalized};
    return pluss::coden::ops_internal::note_from_utf8(flat, {});
  }

  if (options.mode == ContentMode::Lines) {
    return from_lines(utf8, options.keep_line_endings, options.drop_empty);
  }

  return from_words(utf8);
}

inline pluss::Note content(std::u8string_view utf8, const ContentOptions& options = {}) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()}, options);
}

inline pluss::Note content(const char* utf8, const ContentOptions& options = {}) {
  return content(std::string_view{utf8 ? utf8 : ""}, options);
}

inline pluss::Note content(char c, const ContentOptions& options = {}) {
  std::string s(1, c);
  return content(std::string_view{s}, options);
}

inline pluss::Note content(std::string&& utf8, const ContentOptions& options = {}) {
  return content(std::string_view{utf8}, options);
}

inline pluss::Note content(std::u8string&& utf8, const ContentOptions& options = {}) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()}, options);
}

} // namespace pluss::note
