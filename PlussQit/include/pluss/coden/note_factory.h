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
#include "pluss/coden/internal/note_text_reader.h"
#include "pluss/coden/note_content.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

// `Note` (capital N) names the type; `pluss::note::` (lowercase) is its
// dedicated factory namespace -- the construction surface, kept separate
// from the type itself the same way `pluss::timeline::make`/
// `pluss::duration::*` are kept separate from `Timeline`/`Duration`.
//
// pluss::note::make("hi")                   -> scalar Note (0D)
// pluss::note::make({"a","b"})               -> 1D Note
// pluss::note::make(flat, sizes)             -> N-D Note (up to 3D)
// pluss::note::bytes(...)                    -> bytes-mode Note (raw bytes, no UTF-8 guarantees)
//
// Doctrine: once a Note exists, transformations are methods on Note. This
// header should only expose Note creation factories (no free-function Note
// ops).
//
// make/bytes each take a single NoteTextReader parameter covering
// scalar/1D/nested-N-D construction via implicit conversion (see
// internal/note_text_reader.h for why this isn't built on Series' DataReader
// -- text vs numeric domains, same reasoning as CodeH's TimelineTextReader),
// plus one explicit-shape (flat, sizes) overload per source container type
// (two call-site arguments can't fold into one converted reader parameter).
// This collapses what was previously 12 make_note overloads (and 7
// bytes_content + 3 make_bytes_note overloads) down to 3 each.
namespace pluss::note {

inline pluss::Note make(pluss::coden::detail::NoteTextReader data) {
  return pluss::coden::factory_detail::make_note_from_textspan(data.flat(), data.shape());
}
inline pluss::Note make(pluss::msharp::RangeView::Of<std::string_view> flat,
                        pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::coden::detail::NoteTextReader(flat, sizes));
}
inline pluss::Note make(pluss::msharp::RangeView::Of<std::string> flat,
                        pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::coden::detail::NoteTextReader(flat, sizes));
}

// ---- Bytes-mode factories ----

inline pluss::Note bytes(pluss::coden::detail::NoteTextReader data) {
  return pluss::coden::factory_detail::make_bytes_note_from_textspan(data.flat(), data.shape());
}
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::string_view> flat,
                         pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return bytes(pluss::coden::detail::NoteTextReader(flat, sizes));
}
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::string> flat,
                         pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return bytes(pluss::coden::detail::NoteTextReader(flat, sizes));
}
// A raw byte buffer read as one opaque scalar "string" -- not text-domain
// input, so it doesn't go through NoteTextReader like the overloads above.
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::byte> raw) {
  const std::string_view sv{reinterpret_cast<const char*>(raw.data()), raw.size()};
  const std::array<std::string_view, 1> flat{sv};
  return pluss::coden::ops_internal::note_from_bytes(flat, {});
}

// ---- Shape factories ----

inline pluss::Note full(pluss::msharp::RangeView::Of<std::int64_t> sizes, std::string_view value) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
}

inline pluss::Note empty(pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return full(sizes, std::string_view{});
}

inline pluss::Note full_like(const pluss::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "pluss::note::full_like requires a defined Note");
  const auto sizes = like.sizes();
  return full(pluss::msharp::RangeView::Of<std::int64_t>(sizes), value);
}

inline pluss::Note empty_like(const pluss::Note& like) {
  return full_like(like, std::string_view{});
}

inline pluss::Note rep(std::string_view value, pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return full(sizes, value);
}
inline pluss::Note rep(std::string_view value, std::initializer_list<std::int64_t> sizes) {
  return rep(value, pluss::msharp::RangeView::Of<std::int64_t>(sizes));
}

inline pluss::Note rand(pluss::msharp::RangeView::Of<std::int64_t> sizes,
                        bool capitalize = true,
                        std::uint64_t seed = 0) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  const auto flat = pluss::coden::factory_detail::random_chars_from_charset(
      n,
      capitalize ? std::string_view{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"}
                 : std::string_view{"abcdefghijklmnopqrstuvwxyz"},
      seed);
  return pluss::coden::factory_detail::make_note_from_textspan(
      pluss::detail::TextSpan::from_strings(flat),
      shape);
}
inline pluss::Note rand(std::initializer_list<std::int64_t> sizes,
                        bool capitalize = true,
                        std::uint64_t seed = 0) {
  return rand(pluss::msharp::RangeView::Of<std::int64_t>(sizes), capitalize, seed);
}

inline pluss::Note rand_between(std::string_view lo,
                                std::string_view hi,
                                pluss::msharp::RangeView::Of<std::int64_t> sizes,
                                std::uint64_t seed = 0) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  const auto flat = pluss::coden::factory_detail::random_chars_between(
      n, lo, hi, seed, "pluss::note::rand_between");
  return pluss::coden::factory_detail::make_note_from_textspan(
      pluss::detail::TextSpan::from_strings(flat),
      shape);
}
inline pluss::Note rand_between(std::string_view lo,
                                std::string_view hi,
                                std::initializer_list<std::int64_t> sizes,
                                std::uint64_t seed = 0) {
  return rand_between(lo, hi, pluss::msharp::RangeView::Of<std::int64_t>(sizes), seed);
}

inline pluss::Note arange(std::string_view start,
                          std::string_view end,
                          std::int64_t step = 1,
                          bool inclusive = true) {
  const auto flat = pluss::coden::factory_detail::char_arange(
      start, end, step, inclusive, "pluss::note::arange");
  return pluss::coden::factory_detail::make_note_from_owned_strings_1d(flat);
}

using WordSelectMode = pluss::coden::factory_detail::WordSelectMode;

inline pluss::Note perm(std::string_view word,
                        std::int64_t pick,
                        WordSelectMode mode = WordSelectMode::Combination,
                        bool with_replacement = false,
                        bool unique_results = true) {
  const auto flat = pluss::coden::factory_detail::word_select(
      word, pick, mode, with_replacement, unique_results);
  return pluss::coden::factory_detail::make_note_from_owned_strings_1d(flat);
}

inline pluss::Note full_bytes_like(const pluss::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "pluss::note::full_bytes_like requires a defined Note");
  const auto sizes = like.sizes();
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_bytes(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
}

inline pluss::Note empty_bytes_like(const pluss::Note& like) {
  return full_bytes_like(like, std::string_view{});
}

// ---- Text-splitting factories ----

inline pluss::Note from_lines(std::string_view text, bool keep_ends = false, bool drop_empty = false) {
  auto flat = pluss::coden::factory_detail::split_lines_views(text, keep_ends, drop_empty);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::Note from_words(std::string_view text) {
  auto flat = pluss::coden::factory_detail::split_words_views(text);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

// pluss::note::print(n) -- human-readable rendering, mirroring
// pluss::duration::print/pluss::timeline::print. `std::cout << n` also works
// directly via pluss::coden's Note operator<<.
inline std::string print(const pluss::Note& n) {
  return pluss::coden::print_note(n);
}

} // namespace pluss::note

// Keep the backward-compatible flat pluss::* spellings transitively
// available from note_factory.h. Kept in a separate file since the
// namespaced surface above plus the full compat mirror (12 make_note
// overloads, 7 bytes_content, 3 make_bytes_note, plus the shape/splitting
// factories) would make one file unwieldy.
#include "pluss/coden/note_factory_compat.h"
