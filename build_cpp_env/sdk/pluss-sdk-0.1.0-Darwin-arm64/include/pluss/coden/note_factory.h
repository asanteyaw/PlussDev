#pragma once

#include <array>
#include <concepts>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
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

class FormatValue {
public:
  using storage_type = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;

  FormatValue(bool value) : storage_(value) {}
  FormatValue(char value) : storage_(std::string(1, value)) {}
  FormatValue(const char* value) : storage_(std::string(value != nullptr ? value : "")) {}
  FormatValue(std::string_view value) : storage_(std::string(value)) {}
  FormatValue(std::string value) : storage_(std::move(value)) {}

  template <std::signed_integral Int>
    requires (!std::same_as<std::remove_cvref_t<Int>, bool> &&
              !std::same_as<std::remove_cvref_t<Int>, char>)
  FormatValue(Int value) : storage_(static_cast<std::int64_t>(value)) {}

  template <std::unsigned_integral UInt>
    requires (!std::same_as<std::remove_cvref_t<UInt>, bool> &&
              !std::same_as<std::remove_cvref_t<UInt>, char>)
  FormatValue(UInt value) : storage_(static_cast<std::uint64_t>(value)) {}

  template <std::floating_point Float>
  FormatValue(Float value) : storage_(static_cast<double>(value)) {}

  const storage_type& storage() const noexcept { return storage_; }

private:
  storage_type storage_;
};

struct Binding {
  std::string name;
  FormatValue value;

  template <class T>
  Binding(std::string_view key, T&& bound_value)
      : name(key), value(std::forward<T>(bound_value)) {}
};

namespace detail {

inline bool note_format_is_ident_start(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalpha(u) || ch == '_';
}

inline bool note_format_is_ident_continue(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_';
}

inline std::string_view note_format_trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return text.substr(begin, end - begin);
}

inline std::string note_format_double_to_string(double value) {
  std::ostringstream oss;
  oss << std::setprecision(15) << value;
  return oss.str();
}

inline std::string note_format_stringify(const FormatValue& value) {
  return std::visit(
      [](const auto& cell) -> std::string {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, bool>) {
          return cell ? "true" : "false";
        } else if constexpr (std::same_as<T, std::string>) {
          return cell;
        } else if constexpr (std::same_as<T, std::int64_t>) {
          return std::to_string(cell);
        } else if constexpr (std::same_as<T, std::uint64_t>) {
          return std::to_string(cell);
        } else {
          return note_format_double_to_string(cell);
        }
      },
      value.storage());
}

inline double note_format_as_numeric(const Binding& binding, std::string_view expr) {
  return std::visit(
      [&](const auto& cell) -> double {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t> ||
                      std::same_as<T, double>) {
          return static_cast<double>(cell);
        } else {
          PLUSS_FAIL("pluss::note::format: binding '" + binding.name +
                     "' is not numeric and cannot be used in expression {" +
                     std::string(expr) + "}");
        }
      },
      binding.value.storage());
}

inline const Binding& note_format_lookup_binding(
    const std::unordered_map<std::string_view, const Binding*>& bindings,
    std::string_view name) {
  const auto it = bindings.find(name);
  PLUSS_CHECK(it != bindings.end(),
              "pluss::note::format: missing binding '" + std::string(name) + "'");
  return *it->second;
}

class NoteFormatExprParser {
public:
  NoteFormatExprParser(std::string_view expr,
                       const std::unordered_map<std::string_view, const Binding*>& bindings)
      : expr_(expr), bindings_(bindings) {}

  double parse() {
    const double value = parse_expr();
    skip_ws();
    PLUSS_CHECK(pos_ == expr_.size(),
                "pluss::note::format: unexpected trailing input in expression {" +
                    std::string(expr_) + "}");
    return value;
  }

private:
  double parse_expr() {
    double lhs = parse_term();
    while (true) {
      skip_ws();
      if (match('+')) {
        lhs += parse_term();
      } else if (match('-')) {
        lhs -= parse_term();
      } else {
        return lhs;
      }
    }
  }

  double parse_term() {
    double lhs = parse_factor();
    while (true) {
      skip_ws();
      if (match('*')) {
        lhs *= parse_factor();
      } else if (match('/')) {
        const double rhs = parse_factor();
        PLUSS_CHECK(rhs != 0.0,
                    "pluss::note::format: division by zero in expression {" +
                        std::string(expr_) + "}");
        lhs /= rhs;
      } else {
        return lhs;
      }
    }
  }

  double parse_factor() {
    skip_ws();
    if (match('+')) return parse_factor();
    if (match('-')) return -parse_factor();

    if (match('(')) {
      const double value = parse_expr();
      skip_ws();
      PLUSS_CHECK(match(')'),
                  "pluss::note::format: expected ')' in expression {" +
                      std::string(expr_) + "}");
      return value;
    }

    if (peek_is_number_start()) return parse_number();
    if (peek_is_identifier_start()) return parse_identifier();

    PLUSS_FAIL("pluss::note::format: expected number, identifier, or '(' in expression {" +
               std::string(expr_) + "}");
  }

  double parse_number() {
    const std::size_t begin = pos_;
    bool saw_digit = false;

    while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
      ++pos_;
      saw_digit = true;
    }
    if (pos_ < expr_.size() && expr_[pos_] == '.') {
      ++pos_;
      while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
        ++pos_;
        saw_digit = true;
      }
    }
    if (pos_ < expr_.size() && (expr_[pos_] == 'e' || expr_[pos_] == 'E')) {
      const std::size_t exp_pos = pos_;
      ++pos_;
      if (pos_ < expr_.size() && (expr_[pos_] == '+' || expr_[pos_] == '-')) ++pos_;
      const std::size_t exp_digits = pos_;
      while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
        ++pos_;
      }
      PLUSS_CHECK(pos_ > exp_digits,
                  "pluss::note::format: malformed exponent in expression {" +
                      std::string(expr_) + "}");
      saw_digit = true;
      (void)exp_pos;
    }

    PLUSS_CHECK(saw_digit,
                "pluss::note::format: malformed numeric literal in expression {" +
                    std::string(expr_) + "}");
    return std::stod(std::string(expr_.substr(begin, pos_ - begin)));
  }

  double parse_identifier() {
    const std::size_t begin = pos_;
    ++pos_;
    while (pos_ < expr_.size() && note_format_is_ident_continue(expr_[pos_])) ++pos_;
    const std::string_view name = expr_.substr(begin, pos_ - begin);
    const auto& binding = note_format_lookup_binding(bindings_, name);
    return note_format_as_numeric(binding, expr_);
  }

  bool peek_is_number_start() const {
    if (pos_ >= expr_.size()) return false;
    const char ch = expr_[pos_];
    if (std::isdigit(static_cast<unsigned char>(ch))) return true;
    return ch == '.' && pos_ + 1 < expr_.size() &&
           std::isdigit(static_cast<unsigned char>(expr_[pos_ + 1]));
  }

  bool peek_is_identifier_start() const {
    return pos_ < expr_.size() && note_format_is_ident_start(expr_[pos_]);
  }

  void skip_ws() {
    while (pos_ < expr_.size() && std::isspace(static_cast<unsigned char>(expr_[pos_]))) ++pos_;
  }

  bool match(char ch) {
    if (pos_ < expr_.size() && expr_[pos_] == ch) {
      ++pos_;
      return true;
    }
    return false;
  }

  std::string_view expr_;
  const std::unordered_map<std::string_view, const Binding*>& bindings_;
  std::size_t pos_ = 0;
};

inline bool note_format_is_plain_identifier(std::string_view expr) {
  const auto trimmed = note_format_trim(expr);
  if (trimmed.empty() || !note_format_is_ident_start(trimmed.front())) return false;
  for (std::size_t i = 1; i < trimmed.size(); ++i) {
    if (!note_format_is_ident_continue(trimmed[i])) return false;
  }
  return true;
}

inline std::string note_format_eval_placeholder(
    std::string_view placeholder,
    const std::unordered_map<std::string_view, const Binding*>& bindings) {
  const auto trimmed = note_format_trim(placeholder);
  PLUSS_CHECK(!trimmed.empty(), "pluss::note::format: empty placeholder is not allowed");

  if (note_format_is_plain_identifier(trimmed)) {
    return note_format_stringify(note_format_lookup_binding(bindings, trimmed).value);
  }

  NoteFormatExprParser parser(trimmed, bindings);
  return note_format_double_to_string(parser.parse());
}

inline std::string note_format_render(
    std::string_view tpl,
    const std::unordered_map<std::string_view, const Binding*>& bindings) {
  std::string out;
  out.reserve(tpl.size());

  for (std::size_t i = 0; i < tpl.size(); ++i) {
    const char ch = tpl[i];
    if (ch == '{') {
      if (i + 1 < tpl.size() && tpl[i + 1] == '{') {
        out.push_back('{');
        ++i;
        continue;
      }

      const std::size_t open = i + 1;
      std::size_t close = open;
      while (close < tpl.size() && tpl[close] != '}') {
        PLUSS_CHECK(tpl[close] != '{',
                    "pluss::note::format: nested '{' is not allowed inside placeholders");
        ++close;
      }
      PLUSS_CHECK(close < tpl.size(),
                  "pluss::note::format: unmatched '{' in format template");
      out += note_format_eval_placeholder(tpl.substr(open, close - open), bindings);
      i = close;
      continue;
    }

    if (ch == '}') {
      if (i + 1 < tpl.size() && tpl[i + 1] == '}') {
        out.push_back('}');
        ++i;
        continue;
      }
      PLUSS_FAIL("pluss::note::format: unmatched '}' in format template");
    }

    out.push_back(ch);
  }

  return out;
}

} // namespace detail

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

inline pluss::Note format(std::string_view tpl,
                          std::initializer_list<Binding> bindings) {
  std::unordered_map<std::string_view, const Binding*> binding_map;
  binding_map.reserve(bindings.size());
  for (const auto& binding : bindings) {
    PLUSS_CHECK(!binding.name.empty(),
                "pluss::note::format: binding names must not be empty");
    const bool inserted = binding_map.emplace(std::string_view(binding.name), &binding).second;
    PLUSS_CHECK(inserted,
                "pluss::note::format: duplicate binding '" + binding.name + "'");
  }
  return make(detail::note_format_render(tpl, binding_map));
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
