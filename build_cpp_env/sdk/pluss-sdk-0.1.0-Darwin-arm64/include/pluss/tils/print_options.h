#pragma once

// pluss::tils::PrintOptions -- the per-call, rich formatting spec for
// pluss::tils::OutputEngine. Deliberately independent of {fmt}: every knob here
// is realised by OutputEngine's own renderer, so it behaves identically whether
// or not PLUSSQIT_HAS_FMT is set and is not capped by any external spec grammar.
//
// Design contract:
//   * Every field either has an obvious no-op default (0, '\0', empty, `None`)
//     or an explicit `Inherit` enumerator, both meaning "not set here -- fall to
//     the engine baseline, then the built-in default".
//   * `resolve(call, engine)` collapses call-spec -> engine-baseline -> literal
//     defaults into a flat, all-concrete `ResolvedPrint` ONCE per call; the hot
//     render loops then read only plain scalars / string_views (no optionals, no
//     per-element `.value_or`).
//   * `ResolvedPrint` borrows string storage from `call` and `engine.baseline`;
//     it must not outlive them.

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#if __has_include(<unistd.h>)
#include <unistd.h>
#define PLUSS_TILS_HAS_ISATTY 1
#else
#define PLUSS_TILS_HAS_ISATTY 0
#endif

namespace pluss::tils {

// ─── enums ──────────────────────────────────────────────────────────────────
enum class Align    : std::uint8_t { Inherit, Left, Right, Center, Numeric };
enum class Sign     : std::uint8_t { Inherit, NegativeOnly, Always, SpaceForPositive };
enum class Notation : std::uint8_t {
  Inherit, Auto, Fixed, Scientific, General, Hex, Percent, Metric, Binary, Octal
};
enum class Humanize : std::uint8_t { None, BytesIEC, BytesSI, Metric, DurationNs };
enum class BoolStyle: std::uint8_t { Inherit, TrueFalse, OneZero, YesNo, OnOff, Custom };
enum class CaseXf   : std::uint8_t { None, Upper, Lower, Title, Sentence };
enum class PairStyle: std::uint8_t { Inherit, Tuple, Colon, Arrow, Equals };
enum class ColorMode: std::uint8_t { Auto, Always, Never };
enum class Color    : std::uint8_t {
  Default,
  Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
  BrightBlack, BrightRed, BrightGreen, BrightYellow,
  BrightBlue, BrightMagenta, BrightCyan, BrightWhite
};

// ─── style ──────────────────────────────────────────────────────────────────
struct Style {
  Color fg{Color::Default};
  Color bg{Color::Default};
  std::optional<std::array<std::uint8_t, 3>> fg_rgb{};  // wins over `fg`
  std::optional<std::array<std::uint8_t, 3>> bg_rgb{};  // wins over `bg`
  bool bold{}, dim{}, italic{}, underline{}, blink{}, reverse{}, strike{};

  [[nodiscard]] constexpr bool empty() const noexcept {
    return fg == Color::Default && bg == Color::Default && !fg_rgb && !bg_rgb &&
           !bold && !dim && !italic && !underline && !blink && !reverse && !strike;
  }
  friend constexpr bool operator==(const Style&, const Style&) = default;
};

// per-token palette for structured values (lists / maps / tuples)
struct SyntaxTheme {
  std::optional<Style> number, string, boolean, null, key, punctuation, field;
};

// ─── the options bag ────────────────────────────────────────────────────────
struct PrintOptions {
  // joining (the write(a, b, c) path)
  std::optional<std::string> sep;          // between top-level args  (engine default " ")
  std::optional<std::string> end;          // after the last arg      (writeln adds '\n' after this)
  std::optional<std::string> prefix, suffix;
  std::optional<std::string> line_prefix;  // prepended to every physical line
  bool flush{false};                       // force-flush after this call

  // field layout (applied to each rendered top-level value)
  Align align{Align::Inherit};
  int   width{0};                          // min field width (0 = none)
  char  fill{' '};
  int   truncate{0};                       // max display width (0 = unlimited)
  std::string ellipsis{"\xE2\x80\xA6"};    // "…"

  // numbers
  Notation notation{Notation::Inherit};
  std::optional<int> precision;            // fraction digits (also sig-digits for General)
  Sign sign{Sign::Inherit};
  bool alt{false};                         // '#': base prefix 0x/0o/0b; keep trailing '.'
  bool uppercase{false};                   // hex digits, exponent 'E', inf/nan casing
  char grouping{'\0'};                     // thousands sep: '\0' none, ',' '_' ' '
  int  group_size{3};
  char decimal_point{'.'};
  bool trim_trailing_zeros{false};         // 1.2000 -> 1.2
  Humanize humanize{Humanize::None};
  int  humanize_precision{1};
  std::string nan_text{"nan"}, pos_inf_text{"inf"}, neg_inf_text{"-inf"};

  // booleans / null / optional
  BoolStyle bool_style{BoolStyle::Inherit};
  std::string bool_true{"true"}, bool_false{"false"};   // used when bool_style == Custom
  std::optional<std::string> null_text;                 // nullopt / null (engine default "nullopt")
  bool optional_tag{false};                             // render optional<T> as Some(x)/None

  // strings
  bool quote_strings{false};
  char quote_char{'"'};
  bool escape_non_printable{false};
  CaseXf case_transform{CaseXf::None};

  // containers / maps / tuples
  std::optional<std::string> item_sep;                  // between elements (default ", ")
  std::optional<std::string> kv_sep;                    // map key <-> value (default ": ")
  std::optional<std::array<std::string, 2>> list_brackets, map_brackets, tuple_brackets;
  PairStyle pair_style{PairStyle::Inherit};
  int  max_items{0};                                    // cap elements (0 = all)
  bool more_verbose{false};                             // "… (+7 more)" vs "…"
  int  max_depth{0};                                    // 0 = unlimited; deeper -> "[…]"/"{…}"
  bool sort_keys{false};
  bool show_indices{false};                             // list items as "0: a  1: b"
  bool multiline{false};
  std::string indent{"  "};
  bool trailing_comma{false};

  // ANSI
  ColorMode color_mode{ColorMode::Auto};
  Style style;                                          // wraps the whole call's output
  std::optional<SyntaxTheme> syntax;                    // per-token styling of structures

  // format-string path (writef / writelnf)
  std::optional<std::string> missing_arg;               // for a '{}' with no arg (default "{}")
  bool strict_args{false};                              // throw on arg-count mismatch

  // ── fluent builders ──────────────────────────────────────────────────────
  PrintOptions& with_sep(std::string s) { sep = std::move(s); return *this; }
  PrintOptions& with_end(std::string s) { end = std::move(s); return *this; }
  PrintOptions& fixed(int p) { notation = Notation::Fixed; precision = p; return *this; }
  PrintOptions& sci(int p)   { notation = Notation::Scientific; precision = p; return *this; }
  PrintOptions& hex(bool prefix = true) { notation = Notation::Hex; alt = prefix; return *this; }
  PrintOptions& percent(int p = 1) { notation = Notation::Percent; precision = p; return *this; }
  PrintOptions& pad(int w, Align a = Align::Right, char f = ' ') { width = w; align = a; fill = f; return *this; }
  PrintOptions& grouped(char g = ',', int n = 3) { grouping = g; group_size = n; return *this; }
  PrintOptions& colored(Style s) { style = std::move(s); return *this; }
  PrintOptions& quoted(bool on = true) { quote_strings = on; return *this; }
  PrintOptions& upcase(bool on = true) { case_transform = on ? CaseXf::Upper : CaseXf::None; return *this; }
  PrintOptions& pretty(std::string ind = "  ") { multiline = true; indent = std::move(ind); return *this; }
  PrintOptions& capped(int n) { max_items = n; return *this; }

  // ── presets ──────────────────────────────────────────────────────────────
  static PrintOptions plain() {
    PrintOptions p;
    p.list_brackets = std::array<std::string, 2>{"", ""};
    p.map_brackets = std::array<std::string, 2>{"", ""};
    p.tuple_brackets = std::array<std::string, 2>{"", ""};
    p.color_mode = ColorMode::Never;
    return p;
  }
  static PrintOptions pretty_json() {
    PrintOptions p;
    p.multiline = true;
    p.quote_strings = true;
    p.sort_keys = true;
    p.trailing_comma = false;
    p.kv_sep = std::string(": ");
    p.pair_style = PairStyle::Colon;
    p.null_text = std::string("null");
    p.bool_style = BoolStyle::TrueFalse;
    return p;
  }
  static PrintOptions repr() {
    PrintOptions p;
    p.quote_strings = true;
    p.optional_tag = true;
    p.alt = true;
    p.escape_non_printable = true;
    return p;
  }
  static PrintOptions table_cell(int width) {
    PrintOptions p;
    p.width = width;
    p.align = Align::Right;
    p.truncate = width;
    return p;
  }
};

// ─── heterogeneous arg bundle for the format path ───────────────────────────
// `out.writelnf("{} {}", pack(id, name), opts)` -- one object, so the trailing
// `opts` is never ambiguous with a substitution value; also reusable across
// patterns.
template <class... Ts>
struct ArgPack {
  std::tuple<Ts...> vals;
  static constexpr std::size_t size() noexcept { return sizeof...(Ts); }
};
template <class... Ts>
[[nodiscard]] ArgPack<std::decay_t<Ts>...> pack(Ts&&... xs) {
  return ArgPack<std::decay_t<Ts>...>{std::make_tuple(std::forward<Ts>(xs)...)};
}
template <class T>
struct is_arg_pack : std::false_type {};
template <class... Ts>
struct is_arg_pack<ArgPack<Ts...>> : std::true_type {};
template <class T>
inline constexpr bool is_arg_pack_v = is_arg_pack<std::remove_cvref_t<T>>::value;

template <class T>
inline constexpr bool is_print_options_v =
    std::is_same_v<std::remove_cvref_t<T>, PrintOptions>;

// ─── engine-side inputs to resolution ──────────────────────────────────────
struct EngineDefaults {
  std::string_view separator{" "};
  std::string_view null_text{"nullopt"};
  bool color_enabled{false};                 // engine's already-resolved tty state
  const PrintOptions* baseline{nullptr};     // engine-level PrintOptions, if any
};

// ─── the flat, all-concrete view the renderer consumes ─────────────────────
struct ResolvedPrint {
  // joins / affixes
  std::string_view sep, end, prefix, suffix, line_prefix;
  std::string_view item_sep, kv_sep;
  std::string_view list_open, list_close;
  std::string_view map_open, map_close;
  std::string_view tuple_open, tuple_close;
  std::string_view indent, ellipsis, missing_arg;
  std::string_view null_text, bool_true, bool_false;
  std::string_view nan_text, pos_inf_text, neg_inf_text;

  // enums
  Align align{Align::Left};
  Sign sign{Sign::NegativeOnly};
  Notation notation{Notation::Auto};
  Humanize humanize{Humanize::None};
  BoolStyle bool_style{BoolStyle::TrueFalse};
  CaseXf case_transform{CaseXf::None};
  PairStyle pair_style{PairStyle::Tuple};

  // ints (0 / -1 sentinels folded away where possible)
  int width{0}, truncate{0}, group_size{3};
  int max_items{0}, max_depth{0}, humanize_precision{1};
  int precision{-1};                          // -1 == not set (Auto)

  // chars / bools
  char fill{' '}, grouping{'\0'}, decimal_point{'.'}, quote_char{'"'};
  bool alt{false}, uppercase{false}, trim_trailing_zeros{false};
  bool quote_strings{false}, escape_non_printable{false};
  bool sort_keys{false}, show_indices{false}, multiline{false}, trailing_comma{false};
  bool optional_tag{false}, more_verbose{false};
  bool flush{false}, strict_args{false};
  bool color{false};                          // ANSI on for this call

  Style style{};
  const SyntaxTheme* syntax{nullptr};
};

namespace detail {

// pick the first engaged optional<string> across [call, baseline], else literal.
inline std::string_view pick(const std::optional<std::string>& a,
                             const std::optional<std::string>* b,
                             std::string_view lit) noexcept {
  if (a) return *a;
  if (b && *b) return **b;
  return lit;
}
inline std::string_view pick_bracket(
    const std::optional<std::array<std::string, 2>>& a,
    const std::optional<std::array<std::string, 2>>* b,
    std::size_t i, std::string_view lit) noexcept {
  if (a) return (*a)[i];
  if (b && *b) return (**b)[i];
  return lit;
}
template <class T>
inline T pick_enum(T a, const T* b, T lit) noexcept {
  if (a != T::Inherit) return a;
  if (b && *b != T::Inherit) return *b;
  return lit;
}

inline bool stdout_is_tty() noexcept {
#if PLUSS_TILS_HAS_ISATTY
  static const bool v = ::isatty(STDOUT_FILENO) != 0;
  return v;
#else
  return false;
#endif
}

}  // namespace detail

// Collapse call-spec -> engine baseline -> literal defaults, once.
[[nodiscard]] inline ResolvedPrint resolve(const PrintOptions& c,
                                           const EngineDefaults& e) noexcept {
  const PrintOptions* b = e.baseline;
  ResolvedPrint r;

  auto opt = [&](const std::optional<std::string>& ca,
                 const std::optional<std::string>* ba,
                 std::string_view lit) { return detail::pick(ca, ba, lit); };

  r.sep         = opt(c.sep,         b ? &b->sep : nullptr,         e.separator);
  r.end         = opt(c.end,         b ? &b->end : nullptr,         "");
  r.prefix      = opt(c.prefix,      b ? &b->prefix : nullptr,      "");
  r.suffix      = opt(c.suffix,      b ? &b->suffix : nullptr,      "");
  r.line_prefix = opt(c.line_prefix, b ? &b->line_prefix : nullptr, "");
  r.item_sep    = opt(c.item_sep,    b ? &b->item_sep : nullptr,    ", ");
  r.kv_sep      = opt(c.kv_sep,      b ? &b->kv_sep : nullptr,      ": ");
  r.null_text   = opt(c.null_text,   b ? &b->null_text : nullptr,   e.null_text);
  r.missing_arg = opt(c.missing_arg, b ? &b->missing_arg : nullptr, "{}");

  r.list_open   = detail::pick_bracket(c.list_brackets,  b ? &b->list_brackets : nullptr,  0, "[");
  r.list_close  = detail::pick_bracket(c.list_brackets,  b ? &b->list_brackets : nullptr,  1, "]");
  r.map_open    = detail::pick_bracket(c.map_brackets,   b ? &b->map_brackets : nullptr,   0, "{");
  r.map_close   = detail::pick_bracket(c.map_brackets,   b ? &b->map_brackets : nullptr,   1, "}");
  r.tuple_open  = detail::pick_bracket(c.tuple_brackets, b ? &b->tuple_brackets : nullptr, 0, "(");
  r.tuple_close = detail::pick_bracket(c.tuple_brackets, b ? &b->tuple_brackets : nullptr, 1, ")");

  // plain-valued strings: call wins if non-default-ish, else baseline, else literal.
  auto plain_or = [](std::string_view call_v, std::string_view call_default,
                     const std::string* base_v, std::string_view lit) -> std::string_view {
    if (call_v != call_default) return call_v;
    if (base_v && *base_v != std::string(lit)) return *base_v;
    return lit;
  };
  r.indent      = plain_or(c.indent,   "  ",    b ? &b->indent : nullptr,   "  ");
  r.ellipsis    = plain_or(c.ellipsis, "\xE2\x80\xA6", b ? &b->ellipsis : nullptr, "\xE2\x80\xA6");
  r.bool_true   = plain_or(c.bool_true,  "true",  b ? &b->bool_true : nullptr,  "true");
  r.bool_false  = plain_or(c.bool_false, "false", b ? &b->bool_false : nullptr, "false");
  r.nan_text    = plain_or(c.nan_text,     "nan",  b ? &b->nan_text : nullptr,     "nan");
  r.pos_inf_text= plain_or(c.pos_inf_text, "inf",  b ? &b->pos_inf_text : nullptr, "inf");
  r.neg_inf_text= plain_or(c.neg_inf_text, "-inf", b ? &b->neg_inf_text : nullptr, "-inf");

  r.align     = detail::pick_enum(c.align, b ? &b->align : nullptr, Align::Left);
  r.sign      = detail::pick_enum(c.sign,  b ? &b->sign : nullptr,  Sign::NegativeOnly);
  r.notation  = detail::pick_enum(c.notation, b ? &b->notation : nullptr, Notation::Auto);
  r.bool_style= detail::pick_enum(c.bool_style, b ? &b->bool_style : nullptr, BoolStyle::TrueFalse);
  r.pair_style= detail::pick_enum(c.pair_style, b ? &b->pair_style : nullptr, PairStyle::Tuple);
  r.humanize  = c.humanize != Humanize::None ? c.humanize
              : (b && b->humanize != Humanize::None ? b->humanize : Humanize::None);
  r.case_transform = c.case_transform != CaseXf::None ? c.case_transform
              : (b && b->case_transform != CaseXf::None ? b->case_transform : CaseXf::None);

  auto pick_int = [&](int cv, int bv, int def) { return cv != 0 ? cv : (bv != 0 ? bv : def); };
  r.width      = pick_int(c.width, b ? b->width : 0, 0);
  r.truncate   = pick_int(c.truncate, b ? b->truncate : 0, 0);
  r.group_size = c.group_size != 3 ? c.group_size : (b && b->group_size != 3 ? b->group_size : 3);
  r.max_items  = pick_int(c.max_items, b ? b->max_items : 0, 0);
  r.max_depth  = pick_int(c.max_depth, b ? b->max_depth : 0, 0);
  r.humanize_precision =
      c.humanize_precision != 1 ? c.humanize_precision
      : (b && b->humanize_precision != 1 ? b->humanize_precision : 1);
  r.precision  = c.precision ? *c.precision : (b && b->precision ? *b->precision : -1);

  auto pick_char = [](char cv, char def, char bv) { return cv != def ? cv : (bv != def ? bv : def); };
  r.fill          = pick_char(c.fill, ' ', b ? b->fill : ' ');
  r.grouping      = c.grouping ? c.grouping : (b && b->grouping ? b->grouping : '\0');
  r.decimal_point = pick_char(c.decimal_point, '.', b ? b->decimal_point : '.');
  r.quote_char    = pick_char(c.quote_char, '"', b ? b->quote_char : '"');

  auto any = [](bool cv, bool bv) { return cv || bv; };
  r.alt                 = any(c.alt, b && b->alt);
  r.uppercase           = any(c.uppercase, b && b->uppercase);
  r.trim_trailing_zeros = any(c.trim_trailing_zeros, b && b->trim_trailing_zeros);
  r.quote_strings       = any(c.quote_strings, b && b->quote_strings);
  r.escape_non_printable= any(c.escape_non_printable, b && b->escape_non_printable);
  r.sort_keys           = any(c.sort_keys, b && b->sort_keys);
  r.show_indices        = any(c.show_indices, b && b->show_indices);
  r.multiline           = any(c.multiline, b && b->multiline);
  r.trailing_comma      = any(c.trailing_comma, b && b->trailing_comma);
  r.optional_tag        = any(c.optional_tag, b && b->optional_tag);
  r.more_verbose        = any(c.more_verbose, b && b->more_verbose);
  r.flush               = any(c.flush, b && b->flush);
  r.strict_args         = any(c.strict_args, b && b->strict_args);

  r.style  = !c.style.empty() ? c.style : (b && !b->style.empty() ? b->style : Style{});
  r.syntax = c.syntax ? &*c.syntax : (b && b->syntax ? &*b->syntax : nullptr);

  const ColorMode cm = c.color_mode != ColorMode::Auto ? c.color_mode
                     : (b ? b->color_mode : ColorMode::Auto);
  r.color = cm == ColorMode::Always ? true
          : cm == ColorMode::Never  ? false
          : e.color_enabled;

  return r;
}

// ─── perf-critical formatting primitives (allocation-lean, branch-light) ────
namespace detail {

// Pre-built ANSI SGR sequence for a Style -- writing color is then a memcpy.
struct SgrCode {
  std::array<char, 56> buf{};
  std::uint8_t len{0};
  [[nodiscard]] std::string_view view() const noexcept { return {buf.data(), len}; }
};

inline void sgr_push_uint_(SgrCode& s, unsigned v) noexcept {
  char tmp[3];
  int n = 0;
  if (v == 0) tmp[n++] = '0';
  while (v) { tmp[n++] = char('0' + v % 10); v /= 10; }
  while (n) s.buf[s.len++] = tmp[--n];
}
inline void sgr_push_code_(SgrCode& s, unsigned code, bool& first) noexcept {
  if (!first) s.buf[s.len++] = ';';
  first = false;
  sgr_push_uint_(s, code);
}
inline void sgr_push_rgb_(SgrCode& s, unsigned lead, const std::array<std::uint8_t, 3>& c,
                          bool& first) noexcept {
  if (!first) s.buf[s.len++] = ';';
  first = false;
  sgr_push_uint_(s, lead);           // 38 (fg) / 48 (bg)
  s.buf[s.len++] = ';';
  s.buf[s.len++] = '2';
  for (std::uint8_t ch : c) { s.buf[s.len++] = ';'; sgr_push_uint_(s, ch); }
}
inline unsigned fg_code_(Color c) noexcept {
  unsigned i = static_cast<unsigned>(c);
  if (i == 0) return 39;                               // Default
  return i <= 8 ? 29 + i : 82 + i;                     // 30..37  /  90..97
}
inline unsigned bg_code_(Color c) noexcept {
  unsigned i = static_cast<unsigned>(c);
  if (i == 0) return 49;
  return i <= 8 ? 39 + i : 92 + i;                     // 40..47  /  100..107
}
[[nodiscard]] inline SgrCode sgr_open(const Style& st) noexcept {
  SgrCode s;
  if (st.empty()) return s;
  s.buf[s.len++] = '\x1b';
  s.buf[s.len++] = '[';
  bool first = true;
  if (st.bold)      sgr_push_code_(s, 1, first);
  if (st.dim)       sgr_push_code_(s, 2, first);
  if (st.italic)    sgr_push_code_(s, 3, first);
  if (st.underline) sgr_push_code_(s, 4, first);
  if (st.blink)     sgr_push_code_(s, 5, first);
  if (st.reverse)   sgr_push_code_(s, 7, first);
  if (st.strike)    sgr_push_code_(s, 9, first);
  if (st.fg_rgb)                    sgr_push_rgb_(s, 38, *st.fg_rgb, first);
  else if (st.fg != Color::Default) sgr_push_code_(s, fg_code_(st.fg), first);
  if (st.bg_rgb)                    sgr_push_rgb_(s, 48, *st.bg_rgb, first);
  else if (st.bg != Color::Default) sgr_push_code_(s, bg_code_(st.bg), first);
  s.buf[s.len++] = 'm';
  return s;
}
constexpr std::string_view sgr_reset() noexcept { return "\x1b[0m"; }

// Insert `sep` every `group` digits from the right of an ASCII digit run.
// `out` receives the grouped digits appended; caller pre-reserves.
inline void group_digits_into(std::string& out, std::string_view digits,
                              char sep, int group) {
  const int n = static_cast<int>(digits.size());
  if (group <= 0 || sep == '\0' || n <= group) { out.append(digits); return; }
  int first = n % group;
  if (first == 0) first = group;
  out.append(digits.data(), static_cast<std::size_t>(first));
  for (int i = first; i < n; i += group) {
    out.push_back(sep);
    out.append(digits.data() + i, static_cast<std::size_t>(group));
  }
}

// SI/IEC/metric magnitude scaling. Returns scaled value + unit suffix (with any
// leading space baked in).
struct Scaled { double value; std::string_view unit; };
[[nodiscard]] inline Scaled humanize_scale(double v, Humanize mode) noexcept {
  static constexpr std::string_view iec[]  = {"", " KiB", " MiB", " GiB", " TiB", " PiB", " EiB"};
  static constexpr std::string_view sib[]  = {" B", " kB", " MB", " GB", " TB", " PB", " EB"};
  static constexpr std::string_view metric[]= {"", "k", "M", "G", "T", "P", "E"};
  static constexpr std::string_view dur[]   = {" ns", " \xC2\xB5s", " ms", " s"};   // ns, µs, ms, s
  const double base = (mode == Humanize::BytesIEC) ? 1024.0 : 1000.0;
  const double av = v < 0 ? -v : v;
  const double sgn = v < 0 ? -1.0 : 1.0;

  if (mode == Humanize::DurationNs) {
    double x = av;
    std::size_t i = 0;
    while (i + 1 < std::size(dur) && x >= 1000.0) { x /= 1000.0; ++i; }
    return {sgn * x, dur[i]};
  }
  const std::string_view* tab =
      mode == Humanize::BytesIEC ? iec : mode == Humanize::BytesSI ? sib : metric;
  const std::size_t cap = mode == Humanize::BytesIEC ? std::size(iec)
                        : mode == Humanize::BytesSI  ? std::size(sib)
                                                     : std::size(metric);
  double x = av;
  std::size_t i = 0;
  while (i + 1 < cap && x >= base) { x /= base; ++i; }
  return {sgn * x, tab[i]};
}

// ASCII case transform in place (UTF-8 bytes >= 0x80 pass through untouched).
inline void apply_case_inplace(std::string& s, CaseXf mode) noexcept {
  if (mode == CaseXf::None) return;
  bool at_word_start = true;
  bool at_sentence_start = true;  // true at the start of the text and after . ! or ?
  for (char& ch : s) {
    const unsigned char u = static_cast<unsigned char>(ch);
    const bool alpha = (u | 0x20u) - 'a' < 26u;
    switch (mode) {
      case CaseXf::Upper: if (alpha) ch = char(u & ~0x20u); break;
      case CaseXf::Lower: if (alpha) ch = char(u | 0x20u); break;
      case CaseXf::Title:
        if (alpha) ch = at_word_start ? char(u & ~0x20u) : char(u | 0x20u);
        break;
      case CaseXf::Sentence:
        // Only the first letter of each sentence is capitalized; every other letter is lowercased.
        if (alpha) {
          ch = at_sentence_start ? char(u & ~0x20u) : char(u | 0x20u);
          at_sentence_start = false;
        } else if (u == '.' || u == '!' || u == '?') {
          at_sentence_start = true;
        }
        break;
      default: break;
    }
    at_word_start = !alpha && u != '\'';
  }
}

// Display columns for a UTF-8 string: count non-continuation bytes. Good enough
// for aligning ordinary text; does not account for wide/combining glyphs.
[[nodiscard]] inline std::size_t utf8_width(std::string_view s) noexcept {
  std::size_t w = 0;
  for (unsigned char c : s) w += ((c & 0xC0u) != 0x80u);
  return w;
}

}  // namespace detail

}  // namespace pluss::tils
