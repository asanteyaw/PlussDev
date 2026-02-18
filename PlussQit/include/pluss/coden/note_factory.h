#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/coden/coden.h"

namespace pluss {

namespace coden_detail {

inline std::vector<std::string_view> to_string_views(std::span<const std::string> src) {
  std::vector<std::string_view> out;
  out.reserve(src.size());
  for (const auto& s : src) {
    out.emplace_back(s);
  }
  return out;
}

inline bool is_ascii_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline std::int64_t checked_numel(std::span<const std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() <= 3, "Note supports up to 3D");
  std::int64_t n = 1;
  for (const auto s : sizes) {
    PLUSS_CHECK(s >= 0, "sizes must be non-negative");
    n *= s;
  }
  return n;
}

inline std::vector<std::string_view> split_lines_views(std::string_view text, bool keep_ends) {
  std::vector<std::string_view> out;
  std::size_t start = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\n') continue;
    const std::size_t len = keep_ends ? (i - start + 1) : (i - start);
    out.emplace_back(text.data() + start, len);
    start = i + 1;
  }
  if (start < text.size()) {
    out.emplace_back(text.data() + start, text.size() - start);
  }
  return out;
}

inline std::vector<std::string_view> split_words_views(std::string_view text) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  while (i < text.size()) {
    while (i < text.size() && is_ascii_space(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size()) break;
    const std::size_t start = i;
    while (i < text.size() && !is_ascii_space(static_cast<unsigned char>(text[i]))) ++i;
    out.emplace_back(text.data() + start, i - start);
  }
  return out;
}

} // namespace coden_detail

// ---- Public factories (preferred construction path) ----
//
// content(std::string&&)             -> scalar Note (0D) from rvalue text
// make_note(const vector<string>&)   -> 1D Note from lvalue text container
//
// content("hi")                     -> scalar Note (0D)
// content({"a","b"})                -> 1D Note
// make_note(flat, sizes)             -> N-D Note (up to 3D)
//
// bytes_content(...)                 -> bytes-mode Note (raw bytes, no UTF-8 guarantees)
//
// Doctrine: once a Note exists, transformations are methods on Note.
// This header should only expose Note creation factories (no free-function Note ops).

inline pluss::coden::Note full_note(std::span<const std::int64_t> sizes, std::string_view value) {
  const auto n = coden_detail::checked_numel(sizes);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note full_note(std::initializer_list<std::int64_t> sizes, std::string_view value) {
  return full_note(std::span<const std::int64_t>(sizes.begin(), sizes.size()), value);
}

inline pluss::coden::Note empty_note(std::span<const std::int64_t> sizes) {
  return full_note(sizes, std::string_view{});
}

inline pluss::coden::Note empty_note(std::initializer_list<std::int64_t> sizes) {
  return full_note(sizes, std::string_view{});
}

inline pluss::coden::Note full_note_like(const pluss::coden::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "full_note_like requires a defined Note");
  const auto sizes = like.sizes();
  return full_note(std::span<const std::int64_t>(sizes.data(), sizes.size()), value);
}

inline pluss::coden::Note empty_note_like(const pluss::coden::Note& like) {
  return full_note_like(like, std::string_view{});
}

inline pluss::coden::Note note_from_lines(std::string_view text, bool keep_ends = false) {
  auto flat = coden_detail::split_lines_views(text, keep_ends);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note note_from_words(std::string_view text) {
  auto flat = coden_detail::split_words_views(text);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note content(std::string_view utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return pluss::coden::ops_internal::note_from_utf8(flat, {});
}

inline pluss::coden::Note content(std::u8string_view utf8) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

inline pluss::coden::Note content(const char* utf8) {
  return content(std::string_view{utf8 ? utf8 : ""});
}

inline pluss::coden::Note content(char c) {
  std::string s(1, c);
  return content(std::string_view{s});
}

inline pluss::coden::Note content(std::span<const std::string_view> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return pluss::coden::ops_internal::note_from_utf8(utf8_1d, sizes);
}

inline pluss::coden::Note content(const std::vector<std::string_view>& utf8_1d) {
  return content(std::span<const std::string_view>(utf8_1d.data(), utf8_1d.size()));
}

inline pluss::coden::Note content(std::initializer_list<const char*> utf8_1d) {
  std::vector<std::string_view> flat;
  flat.reserve(utf8_1d.size());
  for (auto s : utf8_1d) flat.emplace_back(s ? s : "");
  return content(flat);
}

inline pluss::coden::Note make_note(std::span<const std::string_view> utf8_flat,
                                    std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_utf8(utf8_flat, sizes);
}

inline pluss::coden::Note make_note(std::span<const std::string> utf8_flat,
                                    std::span<const std::int64_t> sizes) {
  const auto flat_views = coden_detail::to_string_views(utf8_flat);
  return make_note(std::span<const std::string_view>(flat_views.data(), flat_views.size()), sizes);
}

inline pluss::coden::Note make_note(const std::vector<std::string_view>& flat,
                                    std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_utf8(flat, sizes);
}

inline pluss::coden::Note make_note(const std::vector<std::string>& flat,
                                    std::span<const std::int64_t> sizes) {
  return make_note(std::span<const std::string>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note make_note(std::span<const std::string_view> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return make_note(utf8_1d, sizes);
}

inline pluss::coden::Note make_note(std::span<const std::string> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return make_note(utf8_1d, sizes);
}

inline pluss::coden::Note make_note(const std::vector<std::string_view>& utf8_1d) {
  return make_note(std::span<const std::string_view>(utf8_1d.data(), utf8_1d.size()));
}

inline pluss::coden::Note make_note(const std::vector<std::string>& utf8_1d) {
  return make_note(std::span<const std::string>(utf8_1d.data(), utf8_1d.size()));
}

// rvalue-oriented scalar/1D factories.
inline pluss::coden::Note content(std::string&& utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return make_note(std::span<const std::string_view>(flat.data(), flat.size()),
                   std::span<const std::int64_t>{});
}

inline pluss::coden::Note content(std::u8string&& utf8) {
  return content(std::string{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

inline pluss::coden::Note content(std::vector<std::string>&& utf8_1d) {
  return make_note(utf8_1d);
}

inline pluss::coden::Note content(std::vector<std::string_view>&& utf8_1d) {
  return make_note(utf8_1d);
}

inline pluss::coden::Note content(std::vector<std::string>&& utf8_flat,
                                  std::span<const std::int64_t> sizes) {
  return make_note(std::span<const std::string>(utf8_flat.data(), utf8_flat.size()), sizes);
}

inline pluss::coden::Note content(std::vector<std::string_view>&& utf8_flat,
                                  std::span<const std::int64_t> sizes) {
  return make_note(std::span<const std::string_view>(utf8_flat.data(), utf8_flat.size()), sizes);
}

// ---- Bytes-mode factories ----

inline pluss::coden::Note bytes_content(std::string_view bytes) {
  const std::array<std::string_view, 1> flat{bytes};
  return pluss::coden::ops_internal::note_from_bytes(flat, {});
}

inline pluss::coden::Note bytes_content(const char* bytes) {
  return bytes_content(std::string_view{bytes ? bytes : ""});
}

inline pluss::coden::Note bytes_content(std::u8string_view bytes) {
  return bytes_content(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

inline pluss::coden::Note bytes_content(std::span<const std::byte> bytes) {
  return bytes_content(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

inline pluss::coden::Note bytes_content(std::span<const std::string_view> bytes_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(bytes_1d.size())};
  return pluss::coden::ops_internal::note_from_bytes(bytes_1d, sizes);
}

inline pluss::coden::Note bytes_content(const std::vector<std::string_view>& bytes_1d) {
  return bytes_content(std::span<const std::string_view>(bytes_1d.data(), bytes_1d.size()));
}

inline pluss::coden::Note bytes_content(std::initializer_list<const char*> bytes_1d) {
  std::vector<std::string_view> flat;
  flat.reserve(bytes_1d.size());
  for (auto s : bytes_1d) flat.emplace_back(s ? s : "");
  return bytes_content(flat);
}

inline pluss::coden::Note make_bytes_note(std::span<const std::string_view> bytes_flat,
                                          std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_bytes(bytes_flat, sizes);
}

inline pluss::coden::Note make_bytes_note(const std::vector<std::string_view>& flat,
                                          std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_bytes(flat, sizes);
}

inline pluss::coden::Note full_bytes_note_like(const pluss::coden::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "full_bytes_note_like requires a defined Note");
  const auto sizes = like.sizes();
  const auto n = coden_detail::checked_numel(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_bytes(
      std::span<const std::string_view>(flat.data(), flat.size()),
      std::span<const std::int64_t>(sizes.data(), sizes.size()));
}

inline pluss::coden::Note empty_bytes_note_like(const pluss::coden::Note& like) {
  return full_bytes_note_like(like, std::string_view{});
}

} // namespace pluss
  