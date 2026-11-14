#pragma once

// The standard replacement-field grammar for scalar values, used by pluss::tils::format and by
// OutputEngine::writef/writelnf when a placeholder carries a spec:
//
//   {[arg_id][:[[fill]align][sign][#][0][width][.precision][group][L][type]]}
//
//   fill      any single code point except '{' '}'      align   '<' '>' '^'
//   sign      '+' '-' ' '                                width/precision  digits, or a nested {[arg_id]}
//   group     ',' '_' '\''  thousands separator (engine extension; applies to decimal digits)
//   L         accepted and ignored (the engine is locale-independent)
//   type      ints   b B c d o x X        floats  a A e E f F g G        strings  s ?        pointers  p
//             '%' (engine extension) renders a number as a percentage
//
// Defaults follow the usual convention: numbers right-align, strings/bools/chars left-align; `0` pads with
// zeros after the sign and base prefix; a precision on a string truncates it; a float with no type prints
// its shortest round-trip form. Malformed grammar throws pluss::Error. A type letter that does not fit
// the argument (for example `f` on an int, `d` on a double) converts the argument rather than throwing.

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <pluss/error.h>

#include "pluss/tils/print_options.h"

namespace pluss::tils::detail::fmtspec {

struct Spec {
  std::string fill{" "};
  char align{0};  // 0 = type default
  char sign{0};   // 0 '-' '+' ' '
  bool alt{false};
  bool zero{false};
  int width{0};
  int precision{-1};
  char group{0};
  char type{0};
};

[[noreturn]] inline void bad_spec(std::string_view why, std::string_view spec) {
  PLUSS_FAIL("format: invalid spec '" + std::string(spec) + "' (" + std::string(why) + ")");
}

inline std::size_t utf8_len_(unsigned char lead) noexcept {
  return lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 1;
}

// `dyn(has_id, id)` resolves a nested {} / {N} to an integer argument; it advances the caller's auto index.
template <class Dyn>
Spec parse(std::string_view s, Dyn&& dyn) {
  Spec sp;
  std::size_t i = 0;
  auto is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };
  if (!s.empty()) {
    const std::size_t cl = utf8_len_(static_cast<unsigned char>(s[0]));
    if (s.size() > cl && is_align(s[cl])) {
      if (s[0] == '{' || s[0] == '}') bad_spec("fill cannot be a brace", s);
      sp.fill.assign(s.substr(0, cl));
      sp.align = s[cl];
      i = cl + 1;
    } else if (is_align(s[0])) {
      sp.align = s[0];
      i = 1;
    }
  }
  if (i < s.size() && (s[i] == '+' || s[i] == '-' || s[i] == ' ')) sp.sign = s[i++];
  if (i < s.size() && s[i] == '#') { sp.alt = true; ++i; }
  if (i < s.size() && s[i] == '0') { sp.zero = true; ++i; }

  auto read_count = [&](int& out) {
    if (i < s.size() && s[i] == '{') {
      std::size_t j = i + 1, id = 0;
      bool has = false;
      while (j < s.size() && s[j] >= '0' && s[j] <= '9') { id = id * 10 + static_cast<std::size_t>(s[j] - '0'); has = true; ++j; }
      if (j >= s.size() || s[j] != '}') bad_spec("unterminated nested field", s);
      const long long v = dyn(has, id);
      if (v < 0 || v > (std::numeric_limits<int>::max)()) bad_spec("width/precision out of range", s);
      out = static_cast<int>(v);
      i = j + 1;
      return true;
    }
    if (i < s.size() && s[i] >= '0' && s[i] <= '9') {
      long long v = 0;
      while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        v = v * 10 + (s[i++] - '0');
        if (v > (std::numeric_limits<int>::max)()) bad_spec("width/precision out of range", s);
      }
      out = static_cast<int>(v);
      return true;
    }
    return false;
  };
  read_count(sp.width);
  if (i < s.size() && s[i] == '.') {
    ++i;
    int p = 0;
    if (!read_count(p)) bad_spec("missing precision", s);
    sp.precision = p;
  }
  if (i < s.size() && (s[i] == ',' || s[i] == '_' || s[i] == '\'')) sp.group = s[i++] == '\'' ? ' ' : s[i - 1];
  if (i < s.size() && s[i] == 'L') ++i;
  if (i < s.size()) {
    sp.type = s[i++];
    if (std::string_view("bBcdeEfFgGaAopsxX?%").find(sp.type) == std::string_view::npos) bad_spec("unknown type", s);
  }
  if (i != s.size()) bad_spec("trailing characters", s);
  return sp;
}

// ── padding ──────────────────────────────────────────────────────────────────
inline std::string fill_n_(const std::string& fill, std::size_t n) {
  std::string out;
  out.reserve(fill.size() * n);
  for (std::size_t k = 0; k < n; ++k) out += fill;
  return out;
}

// default_right: numbers right-align, text left-align.
inline std::string pad(std::string body, const Spec& sp, bool default_right) {
  const std::size_t w = pluss::tils::detail::utf8_width(body);
  if (sp.width <= 0 || w >= static_cast<std::size_t>(sp.width)) return body;
  const std::size_t n = static_cast<std::size_t>(sp.width) - w;
  const char a = sp.align ? sp.align : (default_right ? '>' : '<');
  if (a == '<') return body + fill_n_(sp.fill, n);
  if (a == '>') return fill_n_(sp.fill, n) + body;
  const std::size_t l = n / 2;
  return fill_n_(sp.fill, l) + body + fill_n_(sp.fill, n - l);
}

// Numeric layout: [sign][prefix] digits, with `0` padding inserted after sign+prefix.
inline std::string pad_number(std::string_view sign_prefix, std::string_view digits, const Spec& sp, bool zero_ok) {
  std::string body;
  if (sp.zero && zero_ok && sp.align == 0 && sp.width > 0) {  // an explicit alignment overrides the `0` flag
    const std::size_t have = sign_prefix.size() + digits.size();
    body.assign(sign_prefix);
    if (have < static_cast<std::size_t>(sp.width)) body.append(static_cast<std::size_t>(sp.width) - have, '0');
    body.append(digits);
    return body;
  }
  body.assign(sign_prefix);
  body.append(digits);
  return pad(std::move(body), sp, true);
}

inline void group_into_(std::string& out, std::string_view digits, char sep) {
  pluss::tils::detail::group_digits_into(out, digits, sep, 3);
}

// ── integers ─────────────────────────────────────────────────────────────────
template <class U>
std::string int_digits_(U mag, int base, bool upper) {
  char buf[130];
  auto [p, ec] = std::to_chars(buf, buf + sizeof buf, mag, base);
  (void)ec;
  std::string out(buf, p);
  if (upper) for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

inline std::string sign_text_(bool negative, char sign) {
  if (negative) return "-";
  return sign == '+' ? "+" : sign == ' ' ? " " : "";
}

// fixed/scientific/general text for a double, shared by floats and `{:f}` on ints
template <class F>
std::string float_text_(F v, const Spec& sp, char type, bool negative_zero_sign_handled);

template <class T>
std::string format_integer(T v, const Spec& sp) {
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);
  const char t = sp.type;
  if (t == 'c') {
    if (sp.sign || sp.alt || sp.zero) bad_spec("sign/#/0 not allowed with 'c'", "c");
    PLUSS_CHECK(v >= 0 && v <= 0x10FFFF, "format: value out of range for 'c'");
    std::string ch;
    const auto cp = static_cast<std::uint32_t>(v);
    if (cp < 0x80) ch.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { ch.push_back(static_cast<char>(0xC0 | (cp >> 6))); ch.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    else if (cp < 0x10000) { ch.push_back(static_cast<char>(0xE0 | (cp >> 12))); ch.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); ch.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    else { ch.push_back(static_cast<char>(0xF0 | (cp >> 18))); ch.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F))); ch.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); ch.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    return pad(std::move(ch), sp, false);
  }
  if (t == 'e' || t == 'E' || t == 'f' || t == 'F' || t == 'g' || t == 'G' || t == 'a' || t == 'A' || t == '%') {
    return float_text_(static_cast<double>(v), sp, t, false);
  }
  using U = std::make_unsigned_t<T>;
  const bool neg = v < 0;
  const U mag = neg ? U(0) - static_cast<U>(v) : static_cast<U>(v);
  int base = 10;
  bool upper = false;
  if (t == 'b' || t == 'B') { base = 2; upper = (t == 'B'); }
  else if (t == 'o') base = 8;
  else if (t == 'x' || t == 'X') { base = 16; upper = (t == 'X'); }
  std::string digits = int_digits_(mag, base, upper && base == 16);
  std::string prefix = sign_text_(neg, sp.sign);
  if (sp.alt) {
    if (base == 2) prefix += upper ? "0B" : "0b";
    else if (base == 8) { if (mag != 0) prefix += "0"; }
    else if (base == 16) prefix += upper ? "0X" : "0x";
  }
  if (sp.group && base == 10) {
    std::string g;
    group_into_(g, digits, sp.group);
    digits = std::move(g);
  }
  return pad_number(prefix, digits, sp, true);
}

// ── floating point ───────────────────────────────────────────────────────────
// Split the shortest round-trip scientific form into its digit string and decimal exponent.
template <class F>
void shortest_digits_(F v, std::string& digits, int& exp10) {
  char buf[64];
  auto [p, ec] = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::scientific);
  (void)ec;
  std::string_view s(buf, static_cast<std::size_t>(p - buf));
  const std::size_t e = s.find('e');
  digits.clear();
  for (char c : s.substr(0, e)) if (c != '.') digits.push_back(c);
  exp10 = std::atoi(std::string(s.substr(e + 1)).c_str());
}

inline std::string exp_suffix_(int exp10, bool upper) {
  std::string out(1, upper ? 'E' : 'e');
  out += exp10 < 0 ? '-' : '+';
  const int a = exp10 < 0 ? -exp10 : exp10;
  if (a < 10) out += '0';
  out += std::to_string(a);
  return out;
}

template <class F>
std::string float_body_(F av, const Spec& sp, char type) {
  // av is finite and non-negative; returns the text without sign
  const bool upper = (type == 'E' || type == 'F' || type == 'G' || type == 'A');
  char buf[1100];
  auto finish = [&](std::to_chars_result r) { return std::string(buf, r.ptr); };
  switch (type) {
    case 'f': case 'F': {
      auto body = finish(std::to_chars(buf, buf + sizeof buf, av, std::chars_format::fixed, sp.precision >= 0 ? sp.precision : 6));
      if (sp.alt && body.find('.') == std::string::npos) body += '.';
      return body;
    }
    case 'e': case 'E': {
      auto body = finish(std::to_chars(buf, buf + sizeof buf, av, std::chars_format::scientific, sp.precision >= 0 ? sp.precision : 6));
      if (sp.alt && body.find('.') == std::string::npos) body.insert(body.find('e'), ".");
      if (upper) for (char& c : body) if (c == 'e') c = 'E';
      return body;
    }
    case 'g': case 'G': {
      int p = sp.precision >= 0 ? sp.precision : 6;
      if (p == 0) p = 1;
      if (sp.alt) {
        std::snprintf(buf, sizeof buf, "%#.*Lg", p, static_cast<long double>(av));
        std::string body(buf);
        if (upper) for (char& c : body) if (c == 'e') c = 'E';
        return body;
      }
      auto body = finish(std::to_chars(buf, buf + sizeof buf, av, std::chars_format::general, p));
      if (upper) for (char& c : body) if (c == 'e') c = 'E';
      return body;
    }
    case 'a': case 'A': {
      std::string body;
      if (av == 0) {
        body = "0x0";
        if (sp.alt || sp.precision > 0) body += '.';
        if (sp.precision > 0) body.append(static_cast<std::size_t>(sp.precision), '0');
        body += "p-1022";
      } else {
        std::to_chars_result r = sp.precision >= 0 ? std::to_chars(buf, buf + sizeof buf, av, std::chars_format::hex, sp.precision)
                                                    : std::to_chars(buf, buf + sizeof buf, av, std::chars_format::hex);
        body = "0x" + finish(r);
        if (sp.alt && body.find('.') == std::string::npos) body.insert(body.find('p'), ".");
      }
      if (upper) for (char& c : body) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      return body;
    }
    default: break;
  }
  // no type: precision -> general with that many significant digits, else shortest round-trip
  if (sp.precision >= 0) {
    const int p = sp.precision == 0 ? 1 : sp.precision;
    if (sp.alt) {
      std::snprintf(buf, sizeof buf, "%#.*Lg", p, static_cast<long double>(av));
      return buf;
    }
    return finish(std::to_chars(buf, buf + sizeof buf, av, std::chars_format::general, p));
  }
  if (av == 0) return sp.alt ? "0." : "0";
  std::string digits;
  int e10 = 0;
  shortest_digits_(av, digits, e10);
  std::string body;
  if (e10 < -4 || e10 >= std::numeric_limits<F>::digits10 + 1) {
    body.push_back(digits[0]);
    if (digits.size() > 1 || sp.alt) { body.push_back('.'); body.append(digits, 1, std::string::npos); }
    body += exp_suffix_(e10, false);
  } else if (e10 < 0) {
    body = "0." + std::string(static_cast<std::size_t>(-e10 - 1), '0') + digits;
  } else if (static_cast<int>(digits.size()) <= e10 + 1) {
    body = digits + std::string(static_cast<std::size_t>(e10 + 1) - digits.size(), '0');
    if (sp.alt) body += '.';
  } else {
    body = digits.substr(0, static_cast<std::size_t>(e10) + 1) + "." + digits.substr(static_cast<std::size_t>(e10) + 1);
  }
  return body;
}

template <class F>
std::string float_text_(F v, const Spec& sp, char type, bool) {
  const bool upper = (type == 'E' || type == 'F' || type == 'G' || type == 'A');
  const bool neg = std::signbit(v);
  std::string prefix = sign_text_(neg, sp.sign);
  if (!std::isfinite(v)) {
    std::string t = std::isnan(v) ? "nan" : "inf";
    if (upper) for (char& c : t) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return pad(prefix + t, sp, /*default_right=*/sp.zero);  // inf / nan pad with the fill, never zeros: left-aligned unless `0` is given
  }
  const F av = neg ? -v : v;
  if (type == '%') {
    Spec q = sp;
    if (q.precision < 0) q.precision = 2;
    std::string body = float_body_(static_cast<F>(av * 100), q, 'f') + "%";
    return pad_number(prefix, body, sp, true);
  }
  std::string body = float_body_(av, sp, type);
  if (sp.group && type != 'a' && type != 'A') {
    std::size_t k = 0;
    while (k < body.size() && body[k] >= '0' && body[k] <= '9') ++k;
    std::string g;
    group_into_(g, std::string_view(body).substr(0, k), sp.group);
    g.append(body, k, std::string::npos);
    body = std::move(g);
  }
  return pad_number(prefix, body, sp, true);
}

template <class F>
std::string format_floating(F v, const Spec& sp) {
  static_assert(std::is_floating_point_v<F>);
  const char t = sp.type;
  if (t == 'd' || t == 'b' || t == 'B' || t == 'o' || t == 'x' || t == 'X' || t == 'c' || t == 's') return float_text_(v, sp, 0, false);
  return float_text_(v, sp, t, false);
}

// ── text ─────────────────────────────────────────────────────────────────────
inline void escape_into_(std::string& out, std::string_view s, char quote) {
  out.push_back(quote);
  for (unsigned char c : s) {
    switch (c) {
      case '\t': out += "\\t"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\\': out += "\\\\"; break;
      default:
        if (c == static_cast<unsigned char>(quote)) { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c < 0x20 || c == 0x7F) {
          char b[8];
          std::snprintf(b, sizeof b, "\\x%x", c);
          out += b;
        } else out.push_back(static_cast<char>(c));
    }
  }
  out.push_back(quote);
}

inline std::string format_text(std::string_view s, const Spec& sp) {
  if (sp.sign || sp.alt || sp.zero) bad_spec("sign/#/0 not allowed for text", "");
  std::string escaped;
  if (sp.type == '?') {
    escape_into_(escaped, s, '"');
    s = escaped;
  }
  if (sp.precision >= 0) {
    std::size_t cols = 0, b = 0;
    while (b < s.size()) {
      const std::size_t l = utf8_len_(static_cast<unsigned char>(s[b]));
      if (cols == static_cast<std::size_t>(sp.precision)) break;
      b += (std::min)(l, s.size() - b);
      ++cols;
    }
    s = s.substr(0, b);
  }
  return pad(std::string(s), sp, false);
}

inline std::string format_bool(bool b, const Spec& sp) {
  if (sp.type == 0 || sp.type == 's') return format_text(b ? "true" : "false", sp);
  return format_integer<int>(b ? 1 : 0, sp);
}

template <class C>
std::string format_character(C c, const Spec& sp) {
  if (sp.type == 0 || sp.type == 'c') {
    if (sp.sign || sp.alt || sp.zero) bad_spec("sign/#/0 not allowed with a char", "");
    return pad(std::string(1, static_cast<char>(c)), sp, false);
  }
  if (sp.type == '?') {
    std::string out;
    escape_into_(out, std::string_view(&reinterpret_cast<const char&>(c), 1), '\'');
    return pad(std::move(out), sp, false);
  }
  return format_integer<int>(static_cast<int>(c), sp);
}

inline std::string format_pointer(const void* p, const Spec& sp) {
  if (sp.type != 0 && sp.type != 'p') bad_spec("pointers only take 'p'", "");
  std::string digits = int_digits_(reinterpret_cast<std::uintptr_t>(p), 16, false);
  return pad_number("0x", digits, sp, true);
}

}  // namespace pluss::tils::detail::fmtspec
