#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/coden/coden.h"
#include "pluss/coden/internal/note_factory_detail.h"
#include "pluss/coden/note_content.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

namespace pluss {

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

inline pluss::coden::Note full_note(pluss::msharp::RangeView::Of<std::int64_t> sizes, std::string_view value) {
  const detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
}

inline pluss::coden::Note empty_note(pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return full_note(sizes, std::string_view{});
}

inline pluss::coden::Note full_note_like(const pluss::coden::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "full_note_like requires a defined Note");
  const auto sizes = like.sizes();
  return full_note(pluss::msharp::RangeView::Of<std::int64_t>(sizes), value);
}

inline pluss::coden::Note empty_note_like(const pluss::coden::Note& like) {
  return full_note_like(like, std::string_view{});
}

inline pluss::coden::Note note_from_lines(std::string_view text, bool keep_ends = false) {
  auto flat = coden::factory_detail::split_lines_views(text, keep_ends);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note note_from_words(std::string_view text) {
  auto flat = coden::factory_detail::split_words_views(text);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::coden::Note make_note(std::string_view utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return pluss::coden::ops_internal::note_from_utf8(flat, {});
}
inline pluss::coden::Note make_note(std::u8string_view utf8) {
  return make_note(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}
inline pluss::coden::Note make_note(const char* utf8) {
  return make_note(std::string_view{utf8 ? utf8 : ""});
}
inline pluss::coden::Note make_note(std::initializer_list<const char*> utf8_1d) {
  std::vector<std::string_view> views;
  views.reserve(utf8_1d.size());
  for (const char* s : utf8_1d) {
    views.emplace_back(s ? s : "");
  }
  return coden::factory_detail::make_note_from_textspan_1d(detail::TextSpan(views));
}
inline pluss::coden::Note make_note(char c) {
  std::string s(1, c);
  return make_note(std::string_view{s});
}

inline pluss::coden::Note make_note(pluss::msharp::RangeView::Of<std::string_view> utf8_flat,
                                    pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return coden::factory_detail::make_note_from_textspan(
      detail::TextSpan(std::span<const std::string_view>(utf8_flat.data(), utf8_flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::coden::Note make_note(pluss::msharp::RangeView::Of<std::string> utf8_flat,
                                    pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return coden::factory_detail::make_note_from_textspan(
      detail::TextSpan::from_strings(std::span<const std::string>(utf8_flat.data(), utf8_flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::coden::Note make_note(pluss::msharp::RangeView::Of<std::string_view> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return make_note(utf8_1d, pluss::msharp::RangeView::Of<std::int64_t>(sizes));
}

inline pluss::coden::Note make_note(pluss::msharp::RangeView::Of<std::string> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return make_note(utf8_1d, pluss::msharp::RangeView::Of<std::int64_t>(sizes));
}

inline pluss::coden::Note make_note(std::initializer_list<pluss::detail::NestedNoteLiteral> utf8_nd) {
  auto flattened = pluss::detail::flatten_note_literal(utf8_nd, 3, "pluss::make_note");
  return coden::factory_detail::make_note_from_textspan(
      detail::TextSpan::from_strings(flattened.flat),
      detail::ShapeSpec(std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size())));
}

inline pluss::coden::Note make_note(std::string&& utf8) { return make_note(std::string_view{utf8}); }
inline pluss::coden::Note make_note(std::u8string&& utf8) {
  return make_note(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
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

inline pluss::coden::Note bytes_content(pluss::msharp::RangeView::Of<std::byte> bytes) {
  return bytes_content(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

inline pluss::coden::Note bytes_content(pluss::msharp::RangeView::Of<std::string_view> bytes_1d) {
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan(std::span<const std::string_view>(bytes_1d.data(), bytes_1d.size())).span(),
      std::array<std::int64_t, 1>{static_cast<std::int64_t>(bytes_1d.size())});
}

inline pluss::coden::Note bytes_content(pluss::msharp::RangeView::Of<std::string> bytes_1d) {
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan::from_strings(std::span<const std::string>(bytes_1d.data(), bytes_1d.size())).span(),
      std::array<std::int64_t, 1>{static_cast<std::int64_t>(bytes_1d.size())});
}

inline pluss::coden::Note bytes_content(std::initializer_list<pluss::detail::NestedNoteLiteral> bytes_nd) {
  auto flattened = pluss::detail::flatten_note_literal(bytes_nd, 3, "pluss::bytes_content");
  return pluss::coden::ops_internal::note_from_bytes(
      detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline pluss::coden::Note make_bytes_note(pluss::msharp::RangeView::Of<std::string_view> bytes_flat,
                                          pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return coden::factory_detail::make_bytes_note_from_textspan(
      detail::TextSpan(std::span<const std::string_view>(bytes_flat.data(), bytes_flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::coden::Note make_bytes_note(pluss::msharp::RangeView::Of<std::string> bytes_flat,
                                          pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return coden::factory_detail::make_bytes_note_from_textspan(
      detail::TextSpan::from_strings(std::span<const std::string>(bytes_flat.data(), bytes_flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::coden::Note make_bytes_note(std::initializer_list<pluss::detail::NestedNoteLiteral> bytes_nd) {
  auto flattened = pluss::detail::flatten_note_literal(bytes_nd, 3, "pluss::make_bytes_note");
  return coden::factory_detail::make_bytes_note_from_textspan(
      detail::TextSpan::from_strings(flattened.flat),
      detail::ShapeSpec(std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size())));
}

inline pluss::coden::Note full_bytes_note_like(const pluss::coden::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "full_bytes_note_like requires a defined Note");
  const auto sizes = like.sizes();
  const detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_bytes(
      std::span<const std::string_view>(flat.data(), flat.size()),
      shape.span());
}

inline pluss::coden::Note empty_bytes_note_like(const pluss::coden::Note& like) {
  return full_bytes_note_like(like, std::string_view{});
}

} // namespace pluss
  
