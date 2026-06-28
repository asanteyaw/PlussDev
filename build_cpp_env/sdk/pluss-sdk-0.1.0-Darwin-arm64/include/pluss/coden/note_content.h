#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/coden/coden.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

namespace pluss::coden {

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

namespace detail {

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

inline std::vector<std::string_view> split_lines_views(std::string_view text,
                                                       bool keep_ends,
                                                       bool drop_empty) {
  std::vector<std::string_view> out;
  std::size_t start = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\n') {
      continue;
    }
    const std::size_t len = keep_ends ? (i - start + 1) : (i - start);
    std::string_view item(text.data() + start, len);
    if (!drop_empty || !item.empty()) {
      out.push_back(item);
    }
    start = i + 1;
  }
  if (start < text.size()) {
    std::string_view tail(text.data() + start, text.size() - start);
    if (!drop_empty || !tail.empty()) {
      out.push_back(tail);
    }
  }
  return out;
}

inline std::vector<std::string_view> split_words_views(std::string_view text, bool drop_empty) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  while (i < text.size()) {
    while (i < text.size() && is_ascii_space(static_cast<unsigned char>(text[i]))) {
      ++i;
    }
    if (i >= text.size()) {
      break;
    }
    const std::size_t start = i;
    while (i < text.size() && !is_ascii_space(static_cast<unsigned char>(text[i]))) {
      ++i;
    }
    const std::string_view word(text.data() + start, i - start);
    if (!drop_empty || !word.empty()) {
      out.push_back(word);
    }
  }
  return out;
}

inline Note content_from_textspan(::pluss::detail::TextSpan utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return ops_internal::note_from_utf8(utf8_1d.span(), sizes);
}

} // namespace detail

inline Note content(std::string_view utf8, const ContentOptions& options = {}) {
  if (options.mode == ContentMode::Raw) {
    const std::array<std::string_view, 1> flat{utf8};
    return ops_internal::note_from_utf8(flat, {});
  }

  if (options.mode == ContentMode::Normalize) {
    const auto normalized =
        detail::normalize_ascii_whitespace(utf8, options.trim, options.collapse_whitespace);
    const std::array<std::string_view, 1> flat{normalized};
    return ops_internal::note_from_utf8(flat, {});
  }

  if (options.mode == ContentMode::Lines) {
    auto lines = detail::split_lines_views(utf8, options.keep_line_endings, options.drop_empty);
    const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(lines.size())};
    return ops_internal::note_from_utf8(std::span<const std::string_view>(lines.data(), lines.size()), sizes);
  }

  auto words = detail::split_words_views(utf8, options.drop_empty);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(words.size())};
  return ops_internal::note_from_utf8(std::span<const std::string_view>(words.data(), words.size()), sizes);
}

inline Note content(std::u8string_view utf8, const ContentOptions& options = {}) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()}, options);
}

inline Note content(const char* utf8, const ContentOptions& options = {}) {
  return content(std::string_view{utf8 ? utf8 : ""}, options);
}

inline Note content(char c, const ContentOptions& options = {}) {
  std::string s(1, c);
  return content(std::string_view{s}, options);
}

inline Note content(pluss::msharp::RangeView::Of<std::string_view> utf8_1d) {
  return detail::content_from_textspan(::pluss::detail::TextSpan(
      std::span<const std::string_view>(utf8_1d.data(), utf8_1d.size())));
}

inline Note content(pluss::msharp::RangeView::Of<std::string> utf8_1d) {
  return detail::content_from_textspan(::pluss::detail::TextSpan::from_strings(
      std::span<const std::string>(utf8_1d.data(), utf8_1d.size())));
}

inline Note content(std::initializer_list<::pluss::detail::NestedNoteLiteral> utf8_nd) {
  auto flattened = ::pluss::detail::flatten_note_literal(utf8_nd, 3, "pluss::coden::content");
  return ops_internal::note_from_utf8(
      ::pluss::detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline Note content(std::string&& utf8, const ContentOptions& options = {}) {
  return content(std::string_view{utf8}, options);
}

inline Note content(std::u8string&& utf8, const ContentOptions& options = {}) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()}, options);
}

inline Note content_lines(std::string_view utf8,
                          bool keep_line_endings = false,
                          bool drop_empty = true) {
  ContentOptions options;
  options.mode = ContentMode::Lines;
  options.keep_line_endings = keep_line_endings;
  options.drop_empty = drop_empty;
  return content(utf8, options);
}

inline Note content_words(std::string_view utf8, bool drop_empty = true) {
  ContentOptions options;
  options.mode = ContentMode::Words;
  options.drop_empty = drop_empty;
  return content(utf8, options);
}

} // namespace pluss::coden

namespace pluss {
using coden::ContentMode;
using coden::ContentOptions;
using coden::content;
using coden::content_lines;
using coden::content_words;
} // namespace pluss
