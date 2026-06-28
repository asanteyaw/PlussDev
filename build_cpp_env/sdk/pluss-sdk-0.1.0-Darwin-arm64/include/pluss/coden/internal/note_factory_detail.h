#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/coden/coden.h"
#include "pluss/detail/factory_containers.h"

namespace pluss::coden::factory_detail {

inline bool is_ascii_space(unsigned char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline std::int64_t checked_numel(pluss::detail::ShapeSpec sizes) {
  PLUSS_CHECK(sizes.rank() <= 3, "Note supports up to 3D");
  for (const auto dim : sizes.span()) {
    PLUSS_CHECK(dim >= 0, "sizes must be non-negative");
  }
  return sizes.numel();
}

inline std::vector<std::string_view> split_lines_views(std::string_view text, bool keep_ends) {
  std::vector<std::string_view> out;
  const auto newline_count = static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
  out.reserve(newline_count + ((text.empty() || text.back() == '\n') ? 0ULL : 1ULL));

  std::size_t start = 0;
  while (start < text.size()) {
    const auto nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      out.emplace_back(text.data() + start, text.size() - start);
      break;
    }
    const std::size_t len = keep_ends ? (nl - start + 1) : (nl - start);
    out.emplace_back(text.data() + start, len);
    start = nl + 1;
  }
  return out;
}

inline std::vector<std::string_view> split_words_views(std::string_view text) {
  std::vector<std::string_view> out;
  out.reserve(text.empty() ? 0ULL : 8ULL);

  const char* ptr = text.data();
  const char* end = ptr + text.size();
  while (ptr < end) {
    while (ptr < end && is_ascii_space(static_cast<unsigned char>(*ptr))) {
      ++ptr;
    }
    if (ptr >= end) {
      break;
    }
    const char* start = ptr;
    while (ptr < end && !is_ascii_space(static_cast<unsigned char>(*ptr))) {
      ++ptr;
    }
    out.emplace_back(start, static_cast<std::size_t>(ptr - start));
  }
  return out;
}

inline pluss::coden::Note make_note_from_textspan(pluss::detail::TextSpan utf8_flat,
                                                   pluss::detail::ShapeSpec sizes) {
  return pluss::coden::ops_internal::note_from_utf8(utf8_flat.span(), sizes.span());
}

inline pluss::coden::Note make_note_from_textspan_1d(pluss::detail::TextSpan utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return make_note_from_textspan(
      std::move(utf8_1d),
      pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::coden::Note make_bytes_note_from_textspan(pluss::detail::TextSpan bytes_flat,
                                                         pluss::detail::ShapeSpec sizes) {
  return pluss::coden::ops_internal::note_from_bytes(bytes_flat.span(), sizes.span());
}

} // namespace pluss::coden::factory_detail
