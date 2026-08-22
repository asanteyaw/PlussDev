#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <random>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/coden/coden.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/factory_core.h"

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

// The one canonical line-splitter -- pluss::note::from_lines and
// pluss::note::content's Lines mode both call this directly (rather than one
// calling the other) to share the splitting logic without an include-order
// dependency between note_factory.h and note_content.h.
inline std::vector<std::string_view> split_lines_views(std::string_view text, bool keep_ends,
                                                        bool drop_empty = false) {
  std::vector<std::string_view> out;
  const auto newline_count = static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
  out.reserve(newline_count + ((text.empty() || text.back() == '\n') ? 0ULL : 1ULL));

  std::size_t start = 0;
  while (start < text.size()) {
    const auto nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      std::string_view tail(text.data() + start, text.size() - start);
      if (!drop_empty || !tail.empty()) {
        out.push_back(tail);
      }
      break;
    }
    const std::size_t len = keep_ends ? (nl - start + 1) : (nl - start);
    std::string_view item(text.data() + start, len);
    if (!drop_empty || !item.empty()) {
      out.push_back(item);
    }
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

inline char require_ascii_char(std::string_view token, std::string_view where) {
  PLUSS_CHECK(token.size() == 1, std::string(where) + ": expected a single ASCII character");
  const auto ch = static_cast<unsigned char>(token.front());
  PLUSS_CHECK(ch <= 0x7F, std::string(where) + ": expected an ASCII character");
  return static_cast<char>(ch);
}

inline pluss::coden::Note make_note_from_owned_strings_1d(const std::vector<std::string>& flat) {
  return make_note_from_textspan_1d(pluss::detail::TextSpan::from_strings(flat));
}

inline std::vector<std::string> random_chars_from_charset(std::int64_t n,
                                                          std::string_view charset,
                                                          std::uint64_t seed) {
  PLUSS_CHECK(n >= 0, "note::rand: numel must be non-negative");
  PLUSS_CHECK(!charset.empty(), "note::rand: charset must not be empty");
  auto rng = pluss::detail::FactoryCore::make_rng(seed);
  std::uniform_int_distribution<std::size_t> dist(0, charset.size() - 1);
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    out.emplace_back(1, charset[dist(rng)]);
  }
  return out;
}

inline std::vector<std::string> random_chars_between(std::int64_t n,
                                                     std::string_view lo,
                                                     std::string_view hi,
                                                     std::uint64_t seed,
                                                     std::string_view where) {
  const char lo_ch = require_ascii_char(lo, where);
  const char hi_ch = require_ascii_char(hi, where);
  PLUSS_CHECK(lo_ch <= hi_ch, std::string(where) + ": expected lo <= hi");
  auto rng = pluss::detail::FactoryCore::make_rng(seed);
  std::uniform_int_distribution<int> dist(static_cast<unsigned char>(lo_ch),
                                          static_cast<unsigned char>(hi_ch));
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    out.emplace_back(1, static_cast<char>(dist(rng)));
  }
  return out;
}

inline std::vector<std::string> char_arange(std::string_view start,
                                            std::string_view end,
                                            std::int64_t step,
                                            bool inclusive,
                                            std::string_view where) {
  const char lo = require_ascii_char(start, where);
  const char hi = require_ascii_char(end, where);
  PLUSS_CHECK(step != 0, std::string(where) + ": step cannot be 0");
  std::vector<std::string> out;
  if (step > 0) {
    for (int ch = static_cast<unsigned char>(lo);
         inclusive ? (ch <= static_cast<unsigned char>(hi))
                   : (ch < static_cast<unsigned char>(hi));
         ch += static_cast<int>(step)) {
      out.emplace_back(1, static_cast<char>(ch));
    }
  } else {
    for (int ch = static_cast<unsigned char>(lo);
         inclusive ? (ch >= static_cast<unsigned char>(hi))
                   : (ch > static_cast<unsigned char>(hi));
         ch += static_cast<int>(step)) {
      out.emplace_back(1, static_cast<char>(ch));
    }
  }
  return out;
}

enum class WordSelectMode : std::uint8_t {
  Combination = 0,
  Permutation = 1,
};

inline std::vector<std::string> word_select(std::string_view word,
                                            std::int64_t pick,
                                            WordSelectMode mode,
                                            bool with_replacement,
                                            bool unique_results) {
  PLUSS_CHECK(pick >= 0, "note::perm: pick must be non-negative");
  if (!with_replacement && pick > static_cast<std::int64_t>(word.size())) {
    return {};
  }

  std::vector<std::string> out;
  std::set<std::string> seen;
  std::string current;
  current.reserve(static_cast<std::size_t>(std::max<std::int64_t>(pick, 0)));

  auto push_result = [&](const std::string& value) {
    if (!unique_results) {
      out.push_back(value);
      return;
    }
    if (seen.insert(value).second) {
      out.push_back(value);
    }
  };

  if (pick == 0) {
    push_result(std::string{});
    return out;
  }

  if (mode == WordSelectMode::Combination) {
    std::function<void(std::size_t)> dfs = [&](std::size_t start_index) {
      if (static_cast<std::int64_t>(current.size()) == pick) {
        push_result(current);
        return;
      }
      for (std::size_t i = start_index; i < word.size(); ++i) {
        current.push_back(word[i]);
        dfs(with_replacement ? i : (i + 1));
        current.pop_back();
      }
    };
    dfs(0);
    return out;
  }

  std::vector<bool> used(word.size(), false);
  std::function<void()> dfs = [&]() {
    if (static_cast<std::int64_t>(current.size()) == pick) {
      push_result(current);
      return;
    }
    for (std::size_t i = 0; i < word.size(); ++i) {
      if (!with_replacement && used[i]) {
        continue;
      }
      const bool prior = used[i];
      if (!with_replacement) {
        used[i] = true;
      }
      current.push_back(word[i]);
      dfs();
      current.pop_back();
      used[i] = prior;
    }
  };
  dfs();
  return out;
}

} // namespace pluss::coden::factory_detail
