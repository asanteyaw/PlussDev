#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <pluss/error.h>

#include "pluss/tils/kit/format_spec.h"
#include "pluss/tils/print_options.h"

namespace pluss::tils {

enum class OutputLevel : std::size_t {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4
};

struct OutputOptions {
  std::string separator{" "};
  bool levels_enabled{true};
  std::array<std::string, 5> level_prefixes{{"[TRACE]", "[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]"}};
  OutputLevel min_level{OutputLevel::Trace};
  bool auto_flush{false};
};

namespace detail {

template <class>
inline constexpr bool always_false_v = false;

template <class T>
concept stream_insertable = requires(std::ostream& os, const T& v) {
  { os << v } -> std::same_as<std::ostream&>;
};

template <class T>
concept adl_output_renderable = requires(std::ostream& os, const T& v) {
  output_render(os, v);
};

template <class T>
concept member_show_renderable = requires(std::ostream& os, const T& v) {
  v.show(os);
};

template <class T>
concept member_to_string_renderable = requires(const T& v) {
  v.to_string();
};

template <class T>
concept string_like = std::is_convertible_v<T, std::string_view>;

template <class T>
struct is_optional : std::false_type {};

template <class U>
struct is_optional<std::optional<U>> : std::true_type {};

template <class T>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <class T>
struct is_variant : std::false_type {};

template <class... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <class T>
inline constexpr bool is_variant_v = is_variant<std::remove_cvref_t<T>>::value;

template <class T>
struct is_pair : std::false_type {};

template <class A, class B>
struct is_pair<std::pair<A, B>> : std::true_type {};

template <class T>
inline constexpr bool is_pair_v = is_pair<std::remove_cvref_t<T>>::value;

template <class T, class = void>
struct is_tuple_like : std::false_type {};

template <class T>
struct is_tuple_like<T, std::void_t<decltype(std::tuple_size<std::remove_cvref_t<T>>::value)>> : std::true_type {};

template <class T>
inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

template <class T>
concept map_like = std::ranges::range<T> &&
    requires {
      typename std::remove_cvref_t<T>::key_type;
      typename std::remove_cvref_t<T>::mapped_type;
    };

template <class T>
concept plain_range_like = std::ranges::range<T> && !string_like<T> && !map_like<T>;

template <class T>
inline constexpr bool is_char_v =
    std::is_same_v<T, char> || std::is_same_v<T, signed char> ||
    std::is_same_v<T, unsigned char> || std::is_same_v<T, wchar_t> ||
    std::is_same_v<T, char8_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

// ── render context: a resolved-options view + current nesting depth ────────
struct RenderCtx {
  const ResolvedPrint& r;
  int depth{0};
  [[nodiscard]] RenderCtx nested() const noexcept { return {r, depth + 1}; }
};

inline const ResolvedPrint& default_resolved_() {
  static const PrintOptions po{};
  static const ResolvedPrint r = resolve(po, EngineDefaults{});
  return r;
}

template <class T>
void render_content(std::ostream& os, const T& value, const RenderCtx& ctx);
template <class T>
std::string render_to_string_(const T& value);

// ── number formatting ─────────────────────────────────────────────────────
inline void num_apply_pos_sign_(std::string& out, Sign s) {
  if (s == Sign::Always) out.push_back('+');
  else if (s == Sign::SpaceForPositive) out.push_back(' ');
}
inline void num_trim_zeros_(std::string& body, char dp) {
  const auto dot = body.find(dp);
  if (dot == std::string::npos) return;
  auto last = body.find_last_not_of('0');
  if (last == std::string::npos) return;
  if (last == dot) --last;
  body.erase(last + 1);
}
// `body` may begin with '-'. Split off the sign and the leading digit run, group
// those digits, swap '.' -> decimal_point in the remainder, prepend a positive
// sign if requested, append `unit`, write.
inline void num_finalize_(std::ostream& os, std::string body, std::string_view unit,
                          const ResolvedPrint& r) {
  const bool neg = !body.empty() && body.front() == '-';
  std::string_view d = body;
  if (neg) d.remove_prefix(1);
  std::size_t split = 0;
  while (split < d.size() && d[split] >= '0' && d[split] <= '9') ++split;
  const std::string_view int_part = d.substr(0, split);
  const std::string_view rest = d.substr(split);

  std::string out;
  out.reserve(body.size() + unit.size() + 8);
  if (neg) out.push_back('-');
  else num_apply_pos_sign_(out, r.sign);
  group_digits_into(out, int_part, r.grouping, r.group_size);
  if (!rest.empty()) {
    if (rest.front() == '.' && r.decimal_point != '.') {
      out.push_back(r.decimal_point);
      out.append(rest.substr(1));
    } else {
      out.append(rest);
    }
  }
  out.append(unit);
  os << out;
}

template <class N>
  requires(std::is_arithmetic_v<N> && !std::is_same_v<std::remove_cv_t<N>, bool> &&
           !is_char_v<std::remove_cv_t<N>>)
void render_number(std::ostream& os, N value, const ResolvedPrint& r) {
  char buf[80];

  if constexpr (std::is_floating_point_v<N>) {
    if (std::isnan(value)) { os << r.nan_text; return; }
    if (std::isinf(value)) {
      std::string out;
      if (value >= 0) num_apply_pos_sign_(out, r.sign);
      out.append(value < 0 ? r.neg_inf_text : r.pos_inf_text);
      os << out;
      return;
    }
  }

  if (r.humanize != Humanize::None) {
    const auto sc = humanize_scale(static_cast<double>(value), r.humanize);
    auto [p, ec] = std::to_chars(buf, buf + sizeof buf, sc.value,
                                 std::chars_format::fixed, (r.humanize_precision > 0 ? r.humanize_precision : 0));
    (void)ec;
    std::string body(buf, p);
    if (r.trim_trailing_zeros) num_trim_zeros_(body, '.');
    num_finalize_(os, std::move(body), sc.unit, r);
    return;
  }

  // Notation::Metric scales the number itself with an SI suffix (k, M, G, ...), the same scale
  // humanize_scale uses for Humanize::Metric. It is a notation, not a separate humanize mode, so it
  // applies even when humanize is left at None.
  if (r.notation == Notation::Metric) {
    const auto sc = humanize_scale(static_cast<double>(value), Humanize::Metric);
    auto [p, ec] = std::to_chars(buf, buf + sizeof buf, sc.value,
                                 std::chars_format::fixed, (r.humanize_precision > 0 ? r.humanize_precision : 0));
    (void)ec;
    std::string body(buf, p);
    if (r.trim_trailing_zeros) num_trim_zeros_(body, '.');
    num_finalize_(os, std::move(body), sc.unit, r);
    return;
  }

  if constexpr (std::is_integral_v<N>) {
    const int base = r.notation == Notation::Hex     ? 16
                   : r.notation == Notation::Octal   ? 8
                   : r.notation == Notation::Binary  ? 2
                                                     : 10;
    if (base != 10) {
      using U = std::make_unsigned_t<N>;
      const U mag = value < 0 ? U(0) - static_cast<U>(value) : static_cast<U>(value);
      auto [p, ec] = std::to_chars(buf, buf + sizeof buf, mag, base);
      (void)ec;
      std::string digits(buf, p);
      if (r.uppercase && base == 16)
        for (char& c : digits) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      std::string out;
      if (value < 0) out.push_back('-');
      else num_apply_pos_sign_(out, r.sign);
      if (r.alt) out += base == 16 ? "0x" : base == 8 ? "0o" : "0b";
      out += digits;
      os << out;
      return;
    }
  }

  if (r.notation == Notation::Percent) {
    const double d = static_cast<double>(value) * 100.0;
    const int prec = r.precision >= 0 ? r.precision : 2;
    auto [p, ec] = std::to_chars(buf, buf + sizeof buf, d, std::chars_format::fixed, prec);
    (void)ec;
    std::string body(buf, p);
    if (r.trim_trailing_zeros) num_trim_zeros_(body, '.');
    num_finalize_(os, std::move(body), "%", r);
    return;
  }

  std::to_chars_result res{};
  if constexpr (std::is_floating_point_v<N>) {
    if (r.notation == Notation::Fixed)
      res = std::to_chars(buf, buf + sizeof buf, value, std::chars_format::fixed,
                          r.precision >= 0 ? r.precision : 6);
    else if (r.notation == Notation::Scientific)
      res = std::to_chars(buf, buf + sizeof buf, value, std::chars_format::scientific,
                          r.precision >= 0 ? r.precision : 6);
    else if (r.notation == Notation::General && r.precision >= 0)
      res = std::to_chars(buf, buf + sizeof buf, value, std::chars_format::general, r.precision);
    else {
      // no notation chosen: the shortest round-trip form, laid out like {fmt}'s `{}`
      const bool neg = std::signbit(value);
      std::string body = (neg ? "-" : "") + fmtspec::float_body_(neg ? -value : value, fmtspec::Spec{}, 0);
      if (r.uppercase)
        for (char& c : body) if (c == 'e') c = 'E';
      num_finalize_(os, std::move(body), "", r);
      return;
    }
  } else {
    res = std::to_chars(buf, buf + sizeof buf, value);
  }
  std::string body(buf, res.ptr);
  if (r.trim_trailing_zeros && r.notation == Notation::Fixed) num_trim_zeros_(body, '.');
  if (r.uppercase)
    for (char& c : body) if (c == 'e') c = 'E';
  num_finalize_(os, std::move(body), "", r);
}

// ── structured renderers ─────────────────────────────────────────────────
template <class Tuple, std::size_t... I>
void render_tuple(std::ostream& os, const Tuple& value, const RenderCtx& ctx,
                  std::index_sequence<I...>) {
  const auto& r = ctx.r;
  os << r.tuple_open;
  bool first = true;
  auto emit = [&](const auto& item) {
    if (!first) os << r.item_sep;
    first = false;
    render_content(os, item, ctx.nested());
  };
  (emit(std::get<I>(value)), ...);
  os << r.tuple_close;
}

template <class A, class B>
void render_pairish_(std::ostream& os, const A& a, const B& b, const RenderCtx& ctx) {
  const auto& r = ctx.r;
  const auto c = ctx.nested();
  switch (r.pair_style) {
    case PairStyle::Colon:
      render_content(os, a, c); os << ": "; render_content(os, b, c); break;
    case PairStyle::Arrow:
      render_content(os, a, c); os << " => "; render_content(os, b, c); break;
    case PairStyle::Equals:
      render_content(os, a, c); os << '='; render_content(os, b, c); break;
    default:
      os << r.tuple_open; render_content(os, a, c); os << r.item_sep;
      render_content(os, b, c); os << r.tuple_close;
  }
}

template <class R>
void render_list_(std::ostream& os, const R& range, const RenderCtx& ctx) {
  const auto& r = ctx.r;
  if (r.max_depth > 0 && ctx.depth >= r.max_depth) {
    os << r.list_open << r.ellipsis << r.list_close;
    return;
  }
  os << r.list_open;
  const bool ml = r.multiline;
  std::string pad, closepad;
  if (ml) {
    for (int i = 0; i <= ctx.depth; ++i) pad += r.indent;
    for (int i = 0; i < ctx.depth; ++i) closepad += r.indent;
  }
  const std::size_t cap = r.max_items > 0 ? static_cast<std::size_t>(r.max_items) : 0;
  std::size_t idx = 0, shown = 0;
  bool any = false;
  for (const auto& item : range) {
    if (cap && shown >= cap) break;
    if (any) os << (ml ? std::string_view(",") : r.item_sep);
    any = true;
    if (ml) os << '\n' << pad;
    if (r.show_indices) os << idx << ": ";
    render_content(os, item, ctx.nested());
    ++shown;
    ++idx;
  }
  if (cap) {
    std::size_t remaining = 0;
    if constexpr (std::ranges::sized_range<R>) {
      const auto sz = static_cast<std::size_t>(std::ranges::size(range));
      remaining = sz > shown ? sz - shown : 0;
    } else {
      auto it = std::ranges::begin(range);
      for (std::size_t k = 0; k < shown && it != std::ranges::end(range); ++k) ++it;
      for (; it != std::ranges::end(range); ++it) ++remaining;
    }
    if (remaining) {
      if (any) os << (ml ? std::string_view(",") : r.item_sep);
      if (ml) os << '\n' << pad;
      os << r.ellipsis;
      if (r.more_verbose) os << " (+" << remaining << " more)";
      any = true;
    }
  }
  if (ml && any) {
    if (r.trailing_comma) os << ',';
    os << '\n' << closepad;
  }
  os << r.list_close;
}

template <class M>
void render_map_(std::ostream& os, const M& m, const RenderCtx& ctx) {
  const auto& r = ctx.r;
  if (r.max_depth > 0 && ctx.depth >= r.max_depth) {
    os << r.map_open << r.ellipsis << r.map_close;
    return;
  }
  using Key = typename std::remove_cvref_t<M>::key_type;
  using Mapped = typename std::remove_cvref_t<M>::mapped_type;
  std::vector<std::pair<const Key*, const Mapped*>> items;
  items.reserve(16);
  for (const auto& [k, v] : m) items.emplace_back(&k, &v);
  if (r.sort_keys) {
    std::sort(items.begin(), items.end(), [&](const auto& x, const auto& y) {
      return render_to_string_(*x.first) < render_to_string_(*y.first);
    });
  }
  os << r.map_open;
  const bool ml = r.multiline;
  std::string pad, closepad;
  if (ml) {
    for (int i = 0; i <= ctx.depth; ++i) pad += r.indent;
    for (int i = 0; i < ctx.depth; ++i) closepad += r.indent;
  }
  const std::size_t cap = r.max_items > 0 ? static_cast<std::size_t>(r.max_items) : 0;
  std::size_t shown = 0;
  bool any = false;
  for (const auto& [kp, vp] : items) {
    if (cap && shown >= cap) break;
    if (any) os << (ml ? std::string_view(",") : r.item_sep);
    any = true;
    if (ml) os << '\n' << pad;
    render_content(os, *kp, ctx.nested());
    os << r.kv_sep;
    render_content(os, *vp, ctx.nested());
    ++shown;
  }
  if (cap && items.size() > shown) {
    if (any) os << (ml ? std::string_view(",") : r.item_sep);
    if (ml) os << '\n' << pad;
    os << r.ellipsis;
    if (r.more_verbose) os << " (+" << (items.size() - shown) << " more)";
    any = true;
  }
  if (ml && any) {
    if (r.trailing_comma) os << ',';
    os << '\n' << closepad;
  }
  os << r.map_close;
}

template <class T>
std::string render_to_string_(const T& value) {
  std::ostringstream oss;
  render_content(oss, value, RenderCtx{default_resolved_(), 1});
  return oss.str();
}

// ── the cascade ──────────────────────────────────────────────────────────
template <class T>
void render_content(std::ostream& os, const T& value, const RenderCtx& ctx) {
  const ResolvedPrint& r = ctx.r;
  using U = std::remove_cvref_t<T>;

  if constexpr (std::is_same_v<U, bool> ||
                std::is_same_v<U, std::vector<bool>::reference> ||
                std::is_same_v<U, std::vector<bool>::const_reference>) {
    const bool b = value;  // materialise a std::vector<bool> proxy
    std::string_view t = r.bool_true, f = r.bool_false;
    switch (r.bool_style) {
      case BoolStyle::OneZero: t = "1";   f = "0";   break;
      case BoolStyle::YesNo:   t = "yes"; f = "no";  break;
      case BoolStyle::OnOff:   t = "on";  f = "off"; break;
      default: break;  // TrueFalse / Custom -> r.bool_true / r.bool_false
    }
    os << (b ? t : f);
  } else if constexpr (is_char_v<U>) {
    os << value;
  } else if constexpr (std::is_arithmetic_v<U>) {
    render_number(os, value, r);
  } else if constexpr (string_like<T>) {
    const std::string_view sv{value};
    if (r.quote_strings && ctx.depth > 0)
      os << r.quote_char << sv << r.quote_char;
    else
      os << sv;
  } else if constexpr (is_optional_v<T>) {
    if (value.has_value()) {
      if (r.optional_tag) { os << "Some("; render_content(os, *value, ctx); os << ')'; }
      else render_content(os, *value, ctx);
    } else {
      if (r.optional_tag) os << "None";
      else os << r.null_text;
    }
  } else if constexpr (is_variant_v<T>) {
    std::visit([&](const auto& v) { render_content(os, v, ctx); }, value);
  } else if constexpr (is_pair_v<T>) {
    render_pairish_(os, value.first, value.second, ctx);
  } else if constexpr (is_tuple_like_v<T> && !is_pair_v<T> && !std::ranges::range<T>) {
    constexpr auto n = std::tuple_size_v<U>;
    render_tuple(os, value, ctx, std::make_index_sequence<n>{});
  } else if constexpr (map_like<T>) {
    render_map_(os, value, ctx);
  } else if constexpr (plain_range_like<T>) {
    render_list_(os, value, ctx);
  } else if constexpr (adl_output_renderable<T>) {
    output_render(os, value);
  } else if constexpr (member_show_renderable<T>) {
    value.show(os);
  } else if constexpr (member_to_string_renderable<T>) {
    os << value.to_string();
  } else if constexpr (stream_insertable<T>) {
    os << value;
  } else {
    static_assert(always_false_v<T>,
                  "OutputEngine: value is not stream-insertable and has no supported container rendering path");
  }
}

// ── top-level field layout (width / align / truncate / case) ──────────────
inline void emit_field_(std::ostream& os, std::string s, const ResolvedPrint& r) {
  if (r.case_transform != CaseXf::None) apply_case_inplace(s, r.case_transform);
  std::size_t w = utf8_width(s);
  if (r.truncate > 0 && w > static_cast<std::size_t>(r.truncate)) {
    const std::size_t ell = utf8_width(r.ellipsis);
    const std::size_t keep = static_cast<std::size_t>(r.truncate) > ell
                                 ? static_cast<std::size_t>(r.truncate) - ell
                                 : 0;
    std::size_t cols = 0, b = 0;
    for (; b < s.size() && cols < keep; ++b)
      if ((static_cast<unsigned char>(s[b]) & 0xC0u) != 0x80u) ++cols;
    while (b < s.size() && (static_cast<unsigned char>(s[b]) & 0xC0u) == 0x80u) ++b;
    s.resize(b);
    s.append(r.ellipsis);
    w = utf8_width(s);
  }
  if (r.width > 0 && w < static_cast<std::size_t>(r.width)) {
    const std::size_t padn = static_cast<std::size_t>(r.width) - w;
    if (r.align == Align::Left) {
      os << s << std::string(padn, r.fill);
    } else if (r.align == Align::Center) {
      const std::size_t l = padn / 2;
      os << std::string(l, r.fill) << s << std::string(padn - l, r.fill);
    } else {  // Right / Numeric / default
      os << std::string(padn, r.fill) << s;
    }
  } else {
    os << s;
  }
}

// entry point: separates "content" from top-level "field layout"
template <class T>
void render_value(std::ostream& os, const T& value, const RenderCtx& ctx) {
  const auto& r = ctx.r;
  if (ctx.depth == 0 &&
      (r.width > 0 || r.truncate > 0 || r.case_transform != CaseXf::None)) {
    std::ostringstream tmp;
    render_content(tmp, value, ctx);
    emit_field_(os, std::move(tmp).str(), r);
  } else {
    render_content(os, value, ctx);
  }
}

// legacy 2-arg entry points (engine-default options)
template <class T>
void render_value(std::ostream& os, const T& value) {
  render_value(os, value, RenderCtx{default_resolved_(), 0});
}

template <class T>
std::string render_to_string(const T& value, const RenderCtx& ctx) {
  std::ostringstream oss;
  render_content(oss, value, ctx);
  return oss.str();
}
template <class T>
std::string render_to_string(const T& value) {
  return render_to_string(value, RenderCtx{default_resolved_(), 0});
}

inline std::string simple_brace_format(std::string_view fmt, std::initializer_list<std::string> args) {
  std::string out;
  out.reserve(fmt.size() + 32);

  auto it = args.begin();
  const auto end = args.end();

  for (std::size_t i = 0; i < fmt.size();) {
    const char c = fmt[i];
    if (c == '{') {
      if (i + 1 < fmt.size() && fmt[i + 1] == '{') {
        out.push_back('{');
        i += 2;
        continue;
      }
      if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
        if (it != end) {
          out += *it++;
        } else {
          out += "{}";
        }
        i += 2;
        continue;
      }
    }
    if (c == '}' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
      out.push_back('}');
      i += 2;
      continue;
    }
    out.push_back(c);
    ++i;
  }
  return out;
}

// ── engine-support: trailing PrintOptions detection + arg-pack peel ───────
// Decides whether the last argument of a call is a PrintOptions. The check lives in a function with
// `if constexpr`, so the empty-pack case never names a last element, and a call with no arguments
// compiles as an ordinary call.
template <class... Args>
constexpr bool trailing_options_fn_() {
  if constexpr (sizeof...(Args) == 0) {
    return false;
  } else {
    return is_print_options_v<
        std::tuple_element_t<sizeof...(Args) - 1, std::tuple<std::remove_cvref_t<Args>...>>>;
  }
}

template <class... Args>
inline constexpr bool trailing_options_v = trailing_options_fn_<Args...>();

template <class F, class Tuple, std::size_t... I>
decltype(auto) apply_peel_last_(F&& f, Tuple&& t, std::index_sequence<I...>) {
  constexpr std::size_t N = std::tuple_size_v<std::remove_cvref_t<Tuple>>;
  return std::forward<F>(f)(std::get<N - 1>(std::forward<Tuple>(t)),
                            std::get<I>(std::forward<Tuple>(t))...);
}

template <class Tup, std::size_t... I>
void render_nth_(std::size_t n, const Tup& t, std::ostream& os, const RenderCtx& ctx,
                 std::string_view missing, std::index_sequence<I...>) {
  bool done = false;
  ((I == n ? (render_content(os, std::get<I>(t), ctx), void(done = true)) : void()), ...);
  if (!done) os << missing;
}

// Parse a compact spec ({:[fill]align[sign][#][0][width][.prec][group][type]})
// into a per-placeholder override of an already-resolved ResolvedPrint.
inline void apply_spec_(ResolvedPrint& r, std::string_view s) {
  std::size_t i = 0;
  auto is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };
  if (s.size() >= 2 && is_align(s[1]) && !is_align(s[0])) { r.fill = s[0]; ++i; }
  if (i < s.size() && is_align(s[i])) {
    r.align = s[i] == '<' ? Align::Left : s[i] == '>' ? Align::Right : Align::Center;
    ++i;
  }
  if (i < s.size() && (s[i] == '+' || s[i] == '-' || s[i] == ' ')) {
    r.sign = s[i] == '+' ? Sign::Always : s[i] == ' ' ? Sign::SpaceForPositive : Sign::NegativeOnly;
    ++i;
  }
  if (i < s.size() && s[i] == '#') { r.alt = true; ++i; }
  if (i < s.size() && s[i] == '0') {
    r.fill = '0';
    if (r.align == Align::Left) r.align = Align::Right;
    ++i;
  }
  int w = 0;
  bool hw = false;
  while (i < s.size() && s[i] >= '0' && s[i] <= '9') { w = w * 10 + (s[i] - '0'); hw = true; ++i; }
  if (hw) r.width = w;
  if (i < s.size() && s[i] == '.') {
    ++i;
    int p = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') { p = p * 10 + (s[i] - '0'); ++i; }
    r.precision = p;
  }
  if (i < s.size() && (s[i] == ',' || s[i] == '_' || s[i] == '\'')) { r.grouping = s[i] == '\'' ? ' ' : s[i]; ++i; }
  if (i < s.size()) {
    switch (s[i]) {
      case 'd': r.notation = Notation::Auto; break;
      case 'x': r.notation = Notation::Hex; r.uppercase = false; break;
      case 'X': r.notation = Notation::Hex; r.uppercase = true; break;
      case 'o': r.notation = Notation::Octal; break;
      case 'b': r.notation = Notation::Binary; break;
      case 'f': case 'F': r.notation = Notation::Fixed; break;
      case 'e': r.notation = Notation::Scientific; break;
      case 'E': r.notation = Notation::Scientific; r.uppercase = true; break;
      case 'g': case 'G': r.notation = Notation::General; break;
      case '%': r.notation = Notation::Percent; break;
      case '?': r.quote_strings = true; break;
      case 's': default: break;
    }
  }
}

// ── scalar placeholders with a spec use the strict grammar (format_spec.h) ─────────────────────────────────────
template <class U>
inline constexpr bool strict_scalar_v =
    std::is_arithmetic_v<U> || std::is_pointer_v<U> || std::is_same_v<U, std::nullptr_t> ||
    (std::is_convertible_v<const U&, std::string_view> && !std::is_pointer_v<U>);

template <class T>
std::string format_strict_(const T& value, const fmtspec::Spec& sp) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<U, bool>) {
    return fmtspec::format_bool(value, sp);
  } else if constexpr (is_char_v<U>) {
    return fmtspec::format_character(value, sp);
  } else if constexpr (std::is_integral_v<U>) {
    return fmtspec::format_integer(value, sp);
  } else if constexpr (std::is_floating_point_v<U>) {
    return fmtspec::format_floating(value, sp);
  } else if constexpr (std::is_same_v<U, std::nullptr_t>) {
    return fmtspec::format_pointer(nullptr, sp);
  } else if constexpr (std::is_pointer_v<U> && !std::is_convertible_v<U, std::string_view>) {
    return fmtspec::format_pointer(static_cast<const void*>(value), sp);
  } else {
    return fmtspec::format_text(std::string_view{value}, sp);
  }
}

// integer value of argument `n`, for a nested {} width / precision
template <class Tup, std::size_t... I>
long long arg_as_int_(std::size_t n, const Tup& t, std::index_sequence<I...>) {
  long long out = -1;
  bool done = false;
  auto take = [&](const auto& v) {
    using V = std::remove_cvref_t<decltype(v)>;
    if constexpr (std::is_integral_v<V> && !std::is_same_v<V, bool> && !is_char_v<V>) {
      out = static_cast<long long>(v);
      done = true;
    }
  };
  ((I == n ? (take(std::get<I>(t)), void()) : void()), ...);
  PLUSS_CHECK(done, "format: width/precision argument must be an integer");
  return out;
}

// Call-level PrintOptions (grouping, fixed decimals, hex, ...) keep applying to a placeholder whose spec does not say
// otherwise: the spec decides what it names, the options fill in the rest. Options the strict grammar has no
// spelling for (humanized sizes, metric prefixes, a custom decimal point, trimmed zeros, custom nan/inf text,
// other bool styles) send the placeholder to the engine's own renderer.
template <class V>
constexpr bool strict_scalar_numeric_v = std::is_arithmetic_v<V> && !std::is_same_v<V, bool> && !is_char_v<V>;

template <class V>
bool call_options_need_engine_renderer_(const ResolvedPrint& r) {
  if constexpr (strict_scalar_numeric_v<V>) {
    return r.humanize != Humanize::None || r.notation == Notation::Metric || r.decimal_point != '.' ||
           r.trim_trailing_zeros || r.group_size != 3 || r.nan_text != "nan" || r.pos_inf_text != "inf" ||
           r.neg_inf_text != "-inf";
  } else if constexpr (std::is_same_v<V, bool>) {
    return r.bool_style != BoolStyle::TrueFalse;
  } else {
    return false;
  }
}

template <class V>
void inherit_call_options_(fmtspec::Spec& sp, const ResolvedPrint& r) {
  if constexpr (strict_scalar_numeric_v<V>) {
    if (sp.type == 0) {
      if constexpr (std::is_integral_v<V>) {
        if (r.notation == Notation::Hex) sp.type = r.uppercase ? 'X' : 'x';
        else if (r.notation == Notation::Octal) sp.type = 'o';
        else if (r.notation == Notation::Binary) sp.type = 'b';
        else if (r.notation == Notation::Percent) sp.type = '%';
      } else {
        if (r.notation == Notation::Fixed) sp.type = 'f';
        else if (r.notation == Notation::Scientific) sp.type = r.uppercase ? 'E' : 'e';
        else if (r.notation == Notation::General) sp.type = r.uppercase ? 'G' : 'g';
        else if (r.notation == Notation::Percent) sp.type = '%';
      }
    }
    if (sp.precision < 0 && r.precision >= 0) sp.precision = r.precision;
    if (sp.sign == 0) sp.sign = r.sign == Sign::Always ? '+' : r.sign == Sign::SpaceForPositive ? ' ' : 0;
    if (sp.group == 0 && r.grouping != '\0') sp.group = r.grouping;
    if (r.alt) sp.alt = true;
  }
}

// Render argument `n` under `spec` into `out`. Scalars follow the strict grammar; containers and other
// values keep the engine's element-wise override (`{:.2f}` on a list formats every element).
template <class Tup, class Dyn, std::size_t... I>
void render_nth_spec_(std::size_t n, const Tup& t, std::string_view spec, Dyn&& dyn, const RenderCtx& base,
                      std::string& out, std::string_view missing, std::index_sequence<I...>) {
  bool done = false;
  auto legacy = [&](const auto& v) {
    ResolvedPrint pr = base.r;
    apply_spec_(pr, spec);
    RenderCtx c2{pr, 0};
    std::ostringstream tmp;
    render_content(tmp, v, c2);
    std::string piece = std::move(tmp).str();
    if (pr.width > 0 || pr.truncate > 0 || pr.case_transform != CaseXf::None) {
      std::ostringstream f;
      emit_field_(f, std::move(piece), pr);
      piece = std::move(f).str();
    }
    out += piece;
  };
  auto one = [&](const auto& v) {
    using V = std::remove_cvref_t<decltype(v)>;
    if constexpr (strict_scalar_v<V>) {
      if (call_options_need_engine_renderer_<V>(base.r)) {
        legacy(v);
      } else {
        // The spec owns the field layout. A call-level width / truncate / case still applies to the piece: the
        // whole layout when the spec gave no width, and only truncate and case when it did.
        fmtspec::Spec sp = fmtspec::parse(spec, dyn);
        inherit_call_options_<V>(sp, base.r);
        std::string piece = format_strict_(v, sp);
        ResolvedPrint pr = base.r;
        if (sp.width > 0) pr.width = 0;
        if (pr.width > 0 || pr.truncate > 0 || pr.case_transform != CaseXf::None) {
          std::ostringstream f;
          emit_field_(f, std::move(piece), pr);
          piece = std::move(f).str();
        }
        out += piece;
      }
    } else {
      legacy(v);
    }
    done = true;
  };
  ((I == n ? (one(std::get<I>(t)), void()) : void()), ...);
  if (!done) out.append(missing);
}

// The brace engine behind writef / writelnf / format. `{}` / `{N}` render through the call ctx; `{:spec}`
// applies the spec; `{{` / `}}` are literal braces.
template <class Tup>
std::string format_expand_(std::string_view pat, const RenderCtx& base, const Tup& args) {
  constexpr std::size_t N = std::tuple_size_v<Tup>;
  const std::string_view missing = base.r.missing_arg;
  std::string out;
  out.reserve(pat.size() + 64);
  std::size_t auto_idx = 0, used = 0;

  for (std::size_t i = 0; i < pat.size();) {
    const char c = pat[i];
    if (c == '{') {
      if (i + 1 < pat.size() && pat[i + 1] == '{') { out.push_back('{'); i += 2; continue; }
      std::size_t j = i + 1, idx = 0;
      bool has_idx = false;
      while (j < pat.size() && pat[j] >= '0' && pat[j] <= '9') {
        idx = idx * 10 + static_cast<std::size_t>(pat[j] - '0');
        has_idx = true;
        ++j;
      }
      std::string_view spec;
      if (j < pat.size() && pat[j] == ':') {
        std::size_t k = j + 1;
        int depth = 0;  // nested {} (dynamic width / precision) belong to the spec
        while (k < pat.size() && (pat[k] != '}' || depth > 0)) {
          if (pat[k] == '{') ++depth;
          else if (pat[k] == '}') --depth;
          ++k;
        }
        spec = pat.substr(j + 1, k - (j + 1));
        j = k;
      }
      if (j >= pat.size() || pat[j] != '}') { out.push_back('{'); ++i; continue; }

      const std::size_t use = has_idx ? idx : auto_idx++;
      if (spec.empty()) {
        std::ostringstream tmp;
        if (use < N) render_nth_(use, args, tmp, base, missing, std::make_index_sequence<N>{});
        else tmp << missing;
        std::string piece = std::move(tmp).str();
        const ResolvedPrint& pr = base.r;
        if (pr.width > 0 || pr.truncate > 0 || pr.case_transform != CaseXf::None) {  // call-level field layout
          std::ostringstream f;
          emit_field_(f, std::move(piece), pr);
          piece = std::move(f).str();
        }
        out.append(piece);
      } else if (use < N) {
        auto dyn = [&](bool has, std::size_t id) {
          const std::size_t at = has ? id : auto_idx++;
          PLUSS_CHECK(at < N, "format: width/precision argument index out of range");
          used = (std::max)(used, at + 1);
          return arg_as_int_(at, args, std::make_index_sequence<N>{});
        };
        render_nth_spec_(use, args, spec, dyn, base, out, missing, std::make_index_sequence<N>{});
      } else {
        out.append(missing);
      }
      if (use < N) used = (std::max)(used, use + 1);
      i = j + 1;
      continue;
    }
    if (c == '}' && i + 1 < pat.size() && pat[i + 1] == '}') { out.push_back('}'); i += 2; continue; }
    out.push_back(c);
    ++i;
  }
  PLUSS_CHECK(!(base.r.strict_args && used != N), "OutputEngine: format placeholder / argument count mismatch");
  return out;
}

// ── compile-time checking of literal patterns ────────────────────────────────────────────────────────────────
// Each failure throws a distinct tag type, so the diagnostic of a bad literal names the problem:
//   format_string_error_unmatched_brace, _bad_spec, _missing_argument, _mixed_indexing
struct format_string_error_unmatched_brace {};
struct format_string_error_bad_spec {};
struct format_string_error_missing_argument {};
struct format_string_error_mixed_indexing {};

inline constexpr std::size_t kUnknownArgCount = static_cast<std::size_t>(-1);

constexpr bool is_digit_(char c) noexcept { return c >= '0' && c <= '9'; }

// Walks one pattern exactly like format_expand_ / fmtspec::parse do, without any argument values.
constexpr void check_pattern(std::string_view pat, std::size_t nargs) {
  std::size_t auto_idx = 0;
  int mode = 0;  // 0 none yet, 1 automatic indexing, 2 manual indexing
  auto take = [&](bool manual, std::size_t id) {
    const int want = manual ? 2 : 1;
    if (mode != 0 && mode != want) throw format_string_error_mixed_indexing{};
    mode = want;
    const std::size_t at = manual ? id : auto_idx++;
    if (nargs != kUnknownArgCount && at >= nargs) throw format_string_error_missing_argument{};
  };
  auto utf8_len = [](unsigned char lead) -> std::size_t {
    return lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 1;
  };
  auto is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };

  for (std::size_t i = 0; i < pat.size();) {
    const char c = pat[i];
    if (c == '}') {
      if (i + 1 < pat.size() && pat[i + 1] == '}') { i += 2; continue; }
      throw format_string_error_unmatched_brace{};
    }
    if (c != '{') { ++i; continue; }
    if (i + 1 < pat.size() && pat[i + 1] == '{') { i += 2; continue; }
    std::size_t j = i + 1, id = 0;
    bool has_id = false;
    while (j < pat.size() && is_digit_(pat[j])) { id = id * 10 + static_cast<std::size_t>(pat[j] - '0'); has_id = true; ++j; }
    take(has_id, id);
    if (j < pat.size() && pat[j] == ':') {
      std::size_t k = j + 1, depth = 0;
      while (k < pat.size() && (pat[k] != '}' || depth > 0)) {
        if (pat[k] == '{') ++depth;
        else if (pat[k] == '}') --depth;
        ++k;
      }
      if (k >= pat.size()) throw format_string_error_unmatched_brace{};
      const std::string_view s = pat.substr(j + 1, k - (j + 1));
      std::size_t p = 0;
      if (!s.empty()) {
        const std::size_t cl = utf8_len(static_cast<unsigned char>(s[0]));
        if (s.size() > cl && is_align(s[cl])) p = cl + 1;
        else if (is_align(s[0])) p = 1;
      }
      if (p < s.size() && (s[p] == '+' || s[p] == '-' || s[p] == ' ')) ++p;
      if (p < s.size() && s[p] == '#') ++p;
      if (p < s.size() && s[p] == '0') ++p;
      auto count = [&]() {  // width / precision: digits or a nested {[id]}
        if (p < s.size() && s[p] == '{') {
          std::size_t q = p + 1, nid = 0;
          bool nhas = false;
          while (q < s.size() && is_digit_(s[q])) { nid = nid * 10 + static_cast<std::size_t>(s[q] - '0'); nhas = true; ++q; }
          if (q >= s.size() || s[q] != '}') throw format_string_error_bad_spec{};
          take(nhas, nid);
          p = q + 1;
          return true;
        }
        if (p < s.size() && is_digit_(s[p])) { while (p < s.size() && is_digit_(s[p])) ++p; return true; }
        return false;
      };
      count();
      if (p < s.size() && s[p] == '.') {
        ++p;
        if (!count()) throw format_string_error_bad_spec{};
      }
      if (p < s.size() && (s[p] == ',' || s[p] == '_' || s[p] == '\'')) ++p;
      if (p < s.size() && s[p] == 'L') ++p;
      if (p < s.size()) {
        const char t = s[p++];
        bool ok = false;
        for (const char allowed : std::string_view("bBcdeEfFgGaAopsxX?%")) ok = ok || allowed == t;
        if (!ok) throw format_string_error_bad_spec{};
      }
      if (p != s.size()) throw format_string_error_bad_spec{};
      j = k;
    }
    if (j >= pat.size() || pat[j] != '}') throw format_string_error_unmatched_brace{};
    i = j + 1;
  }
}

// How many values a call's pattern can refer to: all arguments except a trailing PrintOptions.
template <class... Args>
constexpr std::size_t pattern_arg_count() {
  if constexpr (sizeof...(Args) == 1 && (is_arg_pack_v<std::remove_cvref_t<Args>> && ...)) {
    return kUnknownArgCount;  // a packed ArgPack carries its values at run time
  } else if constexpr (trailing_options_v<Args...>) {
    return sizeof...(Args) - 1;
  } else {
    return sizeof...(Args);
  }
}

}  // namespace detail

// A pattern that was checked while compiling. Literals convert implicitly and are validated against the number of
// arguments (`format("{} {}", 1)` does not compile); a pattern built at run time goes through runtime_format().
struct runtime_format {
  std::string_view text;
  explicit runtime_format(std::string_view t) noexcept : text(t) {}
};

namespace detail {
template <class... Args>
struct format_string {
  std::string_view text;
  template <std::size_t N>
  consteval format_string(const char (&literal)[N]) : text(literal, N - 1) {
    check_pattern(text, pattern_arg_count<Args...>());
  }
  format_string(runtime_format r) noexcept : text(r.text) {}
};
template <class... Args>
using format_string_t = std::type_identity_t<format_string<std::remove_cvref_t<Args>...>>;

template <class... Rest>
std::string format_string_(std::string_view pattern, const PrintOptions& po, const Rest&... rest) {
  const ResolvedPrint r = resolve(po, EngineDefaults{});
  const RenderCtx ctx{r, 0};
  return format_expand_(pattern, ctx, std::tie(rest...));
}
} // namespace detail

class OutputEngine {
public:
  explicit OutputEngine(std::ostream& os = std::cout) noexcept
      : sink_(&os) {}

  explicit OutputEngine(OutputOptions opts)
      : sink_(&std::cout) {
    apply_options_unlocked(std::move(opts));
  }

  OutputEngine(std::ostream& os, OutputOptions opts)
      : sink_(&os) {
    apply_options_unlocked(std::move(opts));
  }

  OutputEngine(const OutputEngine&) = delete;
  OutputEngine& operator=(const OutputEngine&) = delete;

  OutputEngine& to(std::ostream& os) noexcept {
    std::scoped_lock lock(mu_);
    sink_ = &os;
    return *this;
  }

  OutputEngine& sep(std::string s) {
    std::scoped_lock lock(mu_);
    separator_ = std::move(s);
    return *this;
  }

  OutputEngine& prefix(OutputLevel level, std::string p) {
    std::scoped_lock lock(mu_);
    prefixes_[to_index(level)] = std::move(p);
    return *this;
  }

  OutputEngine& enable_levels(bool enabled = true) noexcept {
    std::scoped_lock lock(mu_);
    levels_enabled_ = enabled;
    return *this;
  }

  OutputEngine& set_options(OutputOptions opts) {
    std::scoped_lock lock(mu_);
    apply_options_unlocked(std::move(opts));
    return *this;
  }

  [[nodiscard]] OutputOptions options() const {
    std::scoped_lock lock(mu_);
    OutputOptions opts;
    opts.separator = separator_;
    opts.levels_enabled = levels_enabled_;
    opts.level_prefixes = prefixes_;
    opts.min_level = min_level_;
    opts.auto_flush = auto_flush_;
    return opts;
  }

  // Engine-level baseline PrintOptions: applied under every per-call spec so a
  // house style / `pretty()` need not be repeated. Cleared with `{}`.
  OutputEngine& print_defaults(PrintOptions po) {
    std::scoped_lock lock(mu_);
    print_defaults_ = std::move(po);
    print_defaults_set_ = true;
    return *this;
  }
  [[nodiscard]] PrintOptions print_defaults() const {
    std::scoped_lock lock(mu_);
    return print_defaults_;
  }

  template <class... Args>
  OutputEngine& write(const Args&... args) {
    if constexpr (detail::trailing_options_v<Args...>) {
      return detail::apply_peel_last_(
          [this](const PrintOptions& o, const auto&... vs) -> OutputEngine& {
            return this->emit_values_(&o, /*newline=*/false, /*lvl=*/nullptr, vs...);
          },
          std::tie(args...), std::make_index_sequence<sizeof...(Args) - 1>{});
    } else {
      return emit_values_(nullptr, false, nullptr, args...);
    }
  }

  template <class... Args>
  OutputEngine& writeln(const Args&... args) {
    if constexpr (detail::trailing_options_v<Args...>) {
      return detail::apply_peel_last_(
          [this](const PrintOptions& o, const auto&... vs) -> OutputEngine& {
            return this->emit_values_(&o, /*newline=*/true, /*lvl=*/nullptr, vs...);
          },
          std::tie(args...), std::make_index_sequence<sizeof...(Args) - 1>{});
    } else {
      return emit_values_(nullptr, true, nullptr, args...);
    }
  }

  template <class... Args>
  OutputEngine& writef(detail::format_string_t<Args...> pattern, Args&&... args) {
    return format_dispatch_(pattern.text, /*newline=*/false, /*lvl=*/nullptr,
                            std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& writelnf(detail::format_string_t<Args...> pattern, Args&&... args) {
    return format_dispatch_(pattern.text, /*newline=*/true, /*lvl=*/nullptr,
                            std::forward<Args>(args)...);
  }

  // ── formatters (return a string instead of writing to a sink) ───────────────────────────────────────────
  // OutputEngine::format("{:>8.3f} {}", x, name) renders with the same brace grammar and value rendering as
  // writef/writelnf, but with pristine defaults, so it is safe for library messages whatever any engine
  // instance has been configured to do. A trailing PrintOptions overrides the defaults for this call. A literal
  // pattern is checked against its arguments while compiling; a pattern built at run time goes through
  // runtime_format(). Static, so it is usable as OutputEngine::format(...) or engine.format(...).
  template <class... Args>
  [[nodiscard]] static std::string format(detail::format_string_t<Args...> pattern, const Args&... args) {
    if constexpr (detail::trailing_options_v<Args...>) {
      return detail::apply_peel_last_(
          [&](const PrintOptions& o, const auto&... rest) { return detail::format_string_(pattern.text, o, rest...); },
          std::forward_as_tuple(args...), std::make_index_sequence<sizeof...(Args) - 1>{});
    } else {
      return detail::format_string_(pattern.text, PrintOptions{}, args...);
    }
  }

  // OutputEngine::join(range, ", ") -> the elements rendered one by one, separated by `sep`.
  template <std::ranges::input_range R>
  [[nodiscard]] static std::string join(const R& range, std::string_view sep) {
    std::string out;
    bool first = true;
    for (const auto& item : range) {
      if (!first) out.append(sep);
      first = false;
      out += detail::render_to_string(item);
    }
    return out;
  }

  OutputEngine& endln() {
    std::scoped_lock lock(mu_);
    (*sink_) << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  OutputEngine& flush() {
    std::scoped_lock lock(mu_);
    sink_->flush();
    return *this;
  }

  template <class... Args>
  OutputEngine& log(OutputLevel level, const Args&... args) {
    if constexpr (detail::trailing_options_v<Args...>) {
      return detail::apply_peel_last_(
          [this, level](const PrintOptions& o, const auto&... vs) -> OutputEngine& {
            return this->emit_values_(&o, /*newline=*/true, &level, vs...);
          },
          std::tie(args...), std::make_index_sequence<sizeof...(Args) - 1>{});
    } else {
      return emit_values_(nullptr, true, &level, args...);
    }
  }

  template <class... Args>
  OutputEngine& logf(OutputLevel level, detail::format_string_t<Args...> pattern, Args&&... args) {
    return format_dispatch_(pattern.text, /*newline=*/true, &level, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& trace(const Args&... args) {
    return log(OutputLevel::Trace, args...);
  }

  template <class... Args>
  OutputEngine& debug(const Args&... args) {
    return log(OutputLevel::Debug, args...);
  }

  template <class... Args>
  OutputEngine& info(const Args&... args) {
    return log(OutputLevel::Info, args...);
  }

  template <class... Args>
  OutputEngine& warn(const Args&... args) {
    return log(OutputLevel::Warn, args...);
  }

  template <class... Args>
  OutputEngine& error(const Args&... args) {
    return log(OutputLevel::Error, args...);
  }

  template <class... Args>
  OutputEngine& tracef(detail::format_string_t<Args...> pattern, Args&&... args) {
    return logf(OutputLevel::Trace, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& debugf(detail::format_string_t<Args...> pattern, Args&&... args) {
    return logf(OutputLevel::Debug, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& infof(detail::format_string_t<Args...> pattern, Args&&... args) {
    return logf(OutputLevel::Info, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& warnf(detail::format_string_t<Args...> pattern, Args&&... args) {
    return logf(OutputLevel::Warn, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& errorf(detail::format_string_t<Args...> pattern, Args&&... args) {
    return logf(OutputLevel::Error, pattern, std::forward<Args>(args)...);
  }

private:
  // ── plumbing ────────────────────────────────────────────────────────────
  void write_level_prefix_locked(OutputLevel level) {
    if (!levels_enabled_) return;
    (*sink_) << prefixes_[to_index(level)] << " ";
  }
  bool should_log_locked(OutputLevel level) const noexcept {
    return to_index(level) >= to_index(min_level_);
  }
  void apply_options_unlocked(OutputOptions opts) {
    separator_ = std::move(opts.separator);
    levels_enabled_ = opts.levels_enabled;
    prefixes_ = std::move(opts.level_prefixes);
    min_level_ = opts.min_level;
    auto_flush_ = opts.auto_flush;
  }
  static constexpr std::size_t to_index(OutputLevel level) {
    return static_cast<std::size_t>(level);
  }
  bool sink_is_tty_() const noexcept {
    return sink_ == &std::cout && detail::stdout_is_tty();
  }
  EngineDefaults engine_defaults_(const PrintOptions* call) const {
    EngineDefaults ed;
    ed.separator = separator_;
    ed.null_text = "nullopt";
    ed.color_enabled = sink_is_tty_();
    ed.baseline = (call && print_defaults_set_) ? &print_defaults_ : nullptr;
    return ed;
  }
  static std::string apply_line_prefix_(std::string_view s, std::string_view pfx) {
    std::string o;
    o.reserve(s.size() + pfx.size() * 4);
    o.append(pfx);
    for (std::size_t i = 0; i < s.size(); ++i) {
      o.push_back(s[i]);
      if (s[i] == '\n' && i + 1 < s.size()) o.append(pfx);
    }
    return o;
  }

  // ── the fast path: no per-call spec, no engine baseline ─────────────────
  template <class... Vs>
  void join_raw_locked_(const Vs&... vs) {
    bool first = true;
    auto e = [&](const auto& v) {
      if (!first) (*sink_) << separator_;
      first = false;
      detail::render_value(*sink_, v);
    };
    (e(vs), ...);
  }

  // ── envelope + write for an already-rendered body ──────────────────────
  void write_enveloped_locked_(const ResolvedPrint& r, std::string body, bool newline,
                               const OutputLevel* lvl) {
    std::string outp;
    outp.reserve(r.prefix.size() + body.size() + r.suffix.size() + r.end.size() + 8);
    outp.append(r.prefix);
    outp.append(body);
    outp.append(r.suffix);
    if (!r.line_prefix.empty()) outp = apply_line_prefix_(outp, r.line_prefix);
    if (r.color && !r.style.empty()) {
      const auto sgr = detail::sgr_open(r.style);
      std::string wrapped;
      wrapped.reserve(outp.size() + sgr.len + detail::sgr_reset().size());
      wrapped.append(sgr.view());
      wrapped.append(outp);
      wrapped.append(detail::sgr_reset());
      outp.swap(wrapped);
    }
    outp.append(r.end);
    if (lvl) write_level_prefix_locked(*lvl);
    (*sink_) << outp;
    if (newline) (*sink_) << '\n';
    if (r.flush || auto_flush_) sink_->flush();
  }

  // ── write(a, b, c [, PrintOptions]) core ───────────────────────────────
  template <class... Vs>
  OutputEngine& emit_values_(const PrintOptions* call, bool newline, const OutputLevel* lvl,
                             const Vs&... vs) {
    std::scoped_lock lock(mu_);
    if (lvl && !should_log_locked(*lvl)) return *this;

    if (!call && !print_defaults_set_) {  // fast path
      if (lvl) write_level_prefix_locked(*lvl);
      join_raw_locked_(vs...);
      if (newline) (*sink_) << '\n';
      if (auto_flush_) sink_->flush();
      return *this;
    }

    const PrintOptions& po = call ? *call : print_defaults_;
    const ResolvedPrint r = resolve(po, engine_defaults_(call));
    const detail::RenderCtx ctx{r, 0};

    std::ostringstream oss;
    bool first = true;
    auto one = [&](const auto& v) {
      if (!first) oss << r.sep;
      first = false;
      detail::render_value(oss, v, ctx);
    };
    (one(vs), ...);
    write_enveloped_locked_(r, std::move(oss).str(), newline, lvl);
    return *this;
  }

  // ── writef / writelnf / logf core: detect trailing PrintOptions ────────
  template <class... Args>
  OutputEngine& format_dispatch_(std::string_view pattern, bool newline, const OutputLevel* lvl,
                                 Args&&... args) {
    if constexpr (detail::trailing_options_v<Args...>) {
      return detail::apply_peel_last_(
          [&](const PrintOptions& o, auto&&... rest) -> OutputEngine& {
            return format_run_(pattern, newline, lvl, &o, std::forward<decltype(rest)>(rest)...);
          },
          std::forward_as_tuple(std::forward<Args>(args)...),
          std::make_index_sequence<sizeof...(Args) - 1>{});
    } else {
      return format_run_(pattern, newline, lvl, nullptr, std::forward<Args>(args)...);
    }
  }

  template <class... Rest>
  OutputEngine& format_run_(std::string_view pattern, bool newline, const OutputLevel* lvl,
                            const PrintOptions* call, Rest&&... rest) {
    const PrintOptions& po = call ? *call : print_defaults_;
    const ResolvedPrint r = resolve(po, engine_defaults_(call));
    const detail::RenderCtx ctx{r, 0};

    std::string body;
    if constexpr (sizeof...(Rest) == 1) {
      using First = std::tuple_element_t<0, std::tuple<std::remove_cvref_t<Rest>...>>;
      if constexpr (is_arg_pack_v<First>) {
        body = std::apply(
            [&](const auto&... xs) { return detail::format_expand_(pattern, ctx, std::tie(xs...)); },
            std::get<0>(std::forward_as_tuple(std::forward<Rest>(rest)...)).vals);
      } else {
        body = detail::format_expand_(pattern, ctx, std::tie(rest...));
      }
    } else {
      body = detail::format_expand_(pattern, ctx, std::tie(rest...));
    }

    std::scoped_lock lock(mu_);
    if (lvl && !should_log_locked(*lvl)) return *this;
    write_enveloped_locked_(r, std::move(body), newline, lvl);
    return *this;
  }

  std::ostream* sink_{nullptr};
  mutable std::mutex mu_;
  std::string separator_{" "};
  std::array<std::string, 5> prefixes_{{"[TRACE]", "[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]"}};
  bool levels_enabled_{true};
  OutputLevel min_level_{OutputLevel::Trace};
  bool auto_flush_{false};
  PrintOptions print_defaults_{};
  bool print_defaults_set_{false};
};

// ── free-function forms of the engine's formatters ─────────────────────────────────────────────────────────
template <class... Args>
[[nodiscard]] std::string format(detail::format_string_t<Args...> pattern, const Args&... args) {
  return OutputEngine::format<Args...>(pattern, args...);
}

// join(range, ", ") -> the elements rendered one by one, separated by `sep`.
template <std::ranges::input_range R>
[[nodiscard]] std::string join(const R& range, std::string_view sep) {
  return OutputEngine::join(range, sep);
}

// print / println: format and write to std::cout, a stream, or a C FILE*.
template <class... Args>
void print(std::ostream& os, detail::format_string_t<Args...> pattern, const Args&... args) {
  os << format<Args...>(pattern, args...);
}
template <class... Args>
void print(std::FILE* file, detail::format_string_t<Args...> pattern, const Args&... args) {
  const std::string text = format<Args...>(pattern, args...);
  std::fwrite(text.data(), 1, text.size(), file);
}
template <class... Args>
void print(detail::format_string_t<Args...> pattern, const Args&... args) {
  print<Args...>(std::cout, pattern, args...);
}
template <class... Args>
void println(std::ostream& os, detail::format_string_t<Args...> pattern, const Args&... args) {
  os << format<Args...>(pattern, args...) << '\n';
}
template <class... Args>
void println(detail::format_string_t<Args...> pattern, const Args&... args) {
  println<Args...>(std::cout, pattern, args...);
}

} // namespace pluss::tils
