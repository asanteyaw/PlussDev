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
#include "pluss/coden/note_content.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

namespace pluss {

namespace coden_detail {

inline bool is_ascii_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline std::int64_t checked_numel(detail::ShapeSpec sizes) {
  PLUSS_CHECK(sizes.rank() <= 3, "Note supports up to 3D");
  for (const auto s : sizes.span()) {
    PLUSS_CHECK(s >= 0, "sizes must be non-negative");
  }
  return sizes.numel();
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
// make_note("hi")                   -> scalar Note (0D)
// make_note({"a","b"})              -> 1D Note
// make_note(flat, sizes)             -> N-D Note (up to 3D)
//
// bytes_content(...)                 -> bytes-mode Note (raw bytes, no UTF-8 guarantees)
//
// Doctrine: once a Note exists, transformations are methods on Note.
// This header should only expose Note creation factories (no free-function Note ops).

inline pluss::coden::Note full_note(std::span<const std::int64_t> sizes, std::string_view value) {
  const detail::ShapeSpec shape(sizes);
  const auto n = coden_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
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

inline pluss::coden::Note make_note(std::string_view utf8) { return pluss::coden::content(utf8); }
inline pluss::coden::Note make_note(std::u8string_view utf8) { return pluss::coden::content(utf8); }
inline pluss::coden::Note make_note(const char* utf8) { return pluss::coden::content(utf8); }
inline pluss::coden::Note make_note(char c) { return pluss::coden::content(c); }
inline pluss::coden::Note make_note(detail::TextSpan utf8_1d) { return pluss::coden::content(utf8_1d); }
inline pluss::coden::Note make_note(std::initializer_list<const char*> utf8_1d) { return pluss::coden::content(utf8_1d); }

inline pluss::coden::Note make_note(detail::TextSpan utf8_flat,
                                    detail::ShapeSpec sizes) {
  return pluss::coden::ops_internal::note_from_utf8(utf8_flat.span(), sizes.span());
}

inline pluss::coden::Note make_note(std::span<const std::string_view> utf8_flat,
                                    std::span<const std::int64_t> sizes) {
  return make_note(detail::TextSpan(utf8_flat), detail::ShapeSpec(sizes));
}

inline pluss::coden::Note make_note(std::span<const std::string> utf8_flat,
                                    std::span<const std::int64_t> sizes) {
  return make_note(detail::TextSpan::from_strings(utf8_flat), detail::ShapeSpec(sizes));
}

inline pluss::coden::Note make_note(const std::vector<std::string_view>& flat,
                                    std::span<const std::int64_t> sizes) {
  return make_note(detail::TextSpan(flat), detail::ShapeSpec(sizes));
}

inline pluss::coden::Note make_note(const std::vector<std::string>& flat,
                                    std::span<const std::int64_t> sizes) {
  return make_note(detail::TextSpan::from_strings(flat), detail::ShapeSpec(sizes));
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

inline pluss::coden::Note make_note(std::initializer_list<pluss::detail::NestedNoteLiteral> utf8_nd) {
  auto flattened = pluss::detail::flatten_note_literal(utf8_nd, 3, "pluss::make_note");
  return pluss::coden::ops_internal::note_from_utf8(
      detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline pluss::coden::Note make_note(std::string&& utf8) { return pluss::coden::content(std::move(utf8)); }
inline pluss::coden::Note make_note(std::u8string&& utf8) { return pluss::coden::content(std::move(utf8)); }
inline pluss::coden::Note make_note(std::vector<std::string>&& utf8_1d) { return pluss::coden::content(std::move(utf8_1d)); }
inline pluss::coden::Note make_note(std::vector<std::string_view>&& utf8_1d) { return pluss::coden::content(std::move(utf8_1d)); }

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
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan(bytes_1d).span(),
      std::array<std::int64_t, 1>{static_cast<std::int64_t>(bytes_1d.size())});
}

inline pluss::coden::Note bytes_content(const std::vector<std::string_view>& bytes_1d) {
  return bytes_content(detail::TextSpan(bytes_1d).span());
}

inline pluss::coden::Note bytes_content(std::initializer_list<const char*> bytes_1d) {
  return bytes_content(detail::TextSpan(bytes_1d).span());
}

inline pluss::coden::Note bytes_content(std::initializer_list<pluss::detail::NestedNoteLiteral> bytes_nd) {
  auto flattened = pluss::detail::flatten_note_literal(bytes_nd, 3, "pluss::bytes_content");
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline pluss::coden::Note make_bytes_note(detail::TextSpan bytes_flat,
                                          detail::ShapeSpec sizes) {
  return pluss::coden::ops_internal::note_from_bytes(bytes_flat.span(), sizes.span());
}

inline pluss::coden::Note make_bytes_note(std::span<const std::string_view> bytes_flat,
                                          std::span<const std::int64_t> sizes) {
  return make_bytes_note(detail::TextSpan(bytes_flat), detail::ShapeSpec(sizes));
}

inline pluss::coden::Note make_bytes_note(const std::vector<std::string_view>& flat,
                                          std::span<const std::int64_t> sizes) {
  return make_bytes_note(detail::TextSpan(flat), detail::ShapeSpec(sizes));
}

inline pluss::coden::Note make_bytes_note(std::initializer_list<pluss::detail::NestedNoteLiteral> bytes_nd) {
  auto flattened = pluss::detail::flatten_note_literal(bytes_nd, 3, "pluss::make_bytes_note");
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline pluss::coden::Note full_bytes_note_like(const pluss::coden::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "full_bytes_note_like requires a defined Note");
  const auto sizes = like.sizes();
  const detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = coden_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_bytes(
      std::span<const std::string_view>(flat.data(), flat.size()),
      shape.span());
}

inline pluss::coden::Note empty_bytes_note_like(const pluss::coden::Note& like) {
  return full_bytes_note_like(like, std::string_view{});
}

} // namespace pluss
  
