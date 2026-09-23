#include "CodeN/core/OpsInternal.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <limits>
#include <random>
#include <numeric>
#include <optional>
#include <span>
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <regex>
#include <thread>
#include <type_traits>

#if defined(PLUSSQIT_HAS_LIBUNISTRING)
#include <unicase.h>
#include <uninorm.h>
// Some libunistring packages expose fixed-width C types as macros on
// non-glibc platforms.  Do not allow those compatibility macros to rewrite
// std::uint*_t throughout this C++ translation unit.
#ifdef int8_t
#undef int8_t
#endif
#ifdef uint8_t
#undef uint8_t
#endif
#ifdef int16_t
#undef int16_t
#endif
#ifdef uint16_t
#undef uint16_t
#endif
#ifdef int32_t
#undef int32_t
#endif
#ifdef uint32_t
#undef uint32_t
#endif
#endif

#include "CodeN/core/TextArena.h"
#include "CodeN/core/TokenBatchImpl.h"
#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::coden::ops_internal {

namespace {

static bool starts_with_sv(::std::string_view s, ::std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

static bool ends_with_sv(::std::string_view s, ::std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

[[maybe_unused]] static bool ascii_only_(::std::string_view value) {
  return ::std::all_of(
      value.begin(), value.end(),
      [](unsigned char byte) { return byte < 0x80; });
}

static ::std::string ascii_lower_copy(::std::string_view s) {
  ::std::string out(s);
  ::std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(::std::tolower(c));
  });
  return out;
}

static bool env_bool_flag(const char* key, bool default_value) {
  const char* raw = ::std::getenv(key);
  if (raw == nullptr || *raw == '\0') return default_value;
  const ::std::string v = ascii_lower_copy(raw);
  if (v == "0" || v == "off" || v == "false" || v == "no") return false;
  if (v == "1" || v == "on" || v == "true" || v == "yes") return true;
  return default_value;
}

static bool coden_reference_mode_enabled() {
  const char* mode_raw = ::std::getenv("PLUSS_CODEN_EXECUTION_MODE");
  if (mode_raw != nullptr && *mode_raw != '\0') {
    const ::std::string mode = ascii_lower_copy(mode_raw);
    if (mode == "reference" || mode == "ref" || mode == "legacy" || mode == "safe") return true;
    if (mode == "optimized" || mode == "opt" || mode == "fast") return false;
  }
  return env_bool_flag("PLUSS_CODEN_REFERENCE_MODE", false);
}

static bool coden_count_fastpath_enabled() {
  if (coden_reference_mode_enabled()) return false;
  return env_bool_flag("PLUSS_CODEN_COUNT_FASTPATH", true);
}

static ::std::string strip_math_delims(::std::string_view m);
static ::std::string normalize_tex_removed(::std::string_view s);
static ::std::string tex_remove_labels_text(::std::string_view s);
static ::std::string tex_remove_refs_text(::std::string_view s);
static ::std::string tex_remove_citations_text(::std::string_view s);
static ::std::string tex_remove_packages_text(::std::string_view s);
static ::std::string tex_remove_bibliography_text(::std::string_view s);
static ::std::string tex_remove_hyperref_text(::std::string_view s);
static ::std::string tex_remove_urls_text(::std::string_view s);
static ::std::string tex_remove_color_text(::std::string_view s);
static ::std::string tex_remove_left_right_text(::std::string_view s);
static ::std::string tex_remove_big_delims_text(::std::string_view s);
static ::std::string tex_remove_spacing_cmds_text(::std::string_view s);
static ::std::string tex_expand_macros_text(::std::string_view s, ::std::int64_t max_passes);
static ::std::string tex_remove_math_delimiters_text(::std::string_view s);
static bool tex_parse_braced_group(::std::string_view s,
                                   ::std::size_t open_pos,
                                   ::std::size_t* next_pos,
                                   ::std::string* body);

static ::std::string replace_first_text(::std::string_view s, ::std::string_view from, ::std::string_view to) {
  if (from.empty()) return ::std::string(s);
  const auto pos = s.find(from);
  if (pos == ::std::string_view::npos) return ::std::string(s);
  ::std::string out;
  out.reserve(s.size() + (to.size() > from.size() ? to.size() - from.size() : 0));
  out.append(s.substr(0, pos));
  out.append(to);
  out.append(s.substr(pos + from.size()));
  return out;
}

static ::std::string replace_last_text(::std::string_view s, ::std::string_view from, ::std::string_view to) {
  if (from.empty()) return ::std::string(s);
  const auto pos = s.rfind(from);
  if (pos == ::std::string_view::npos) return ::std::string(s);
  ::std::string out;
  out.reserve(s.size() + (to.size() > from.size() ? to.size() - from.size() : 0));
  out.append(s.substr(0, pos));
  out.append(to);
  out.append(s.substr(pos + from.size()));
  return out;
}

static ::std::string replace_n_text(::std::string_view s, ::std::string_view from, ::std::string_view to, ::std::int64_t n) {
  if (from.empty() || n <= 0) return ::std::string(s);
  ::std::string out;
  ::std::size_t pos = 0;
  ::std::int64_t replaced = 0;
  while (pos < s.size()) {
    if (replaced >= n) {
      out.append(s.substr(pos));
      break;
    }
    const auto p = s.find(from, pos);
    if (p == ::std::string_view::npos) {
      out.append(s.substr(pos));
      break;
    }
    out.append(s.substr(pos, p - pos));
    out.append(to);
    pos = p + from.size();
    ++replaced;
  }
  return out;
}

static ::std::string remove_prefix_text(::std::string_view s, ::std::string_view prefix) {
  if (prefix.empty() || !starts_with_sv(s, prefix)) return ::std::string(s);
  return ::std::string(s.substr(prefix.size()));
}

static ::std::string remove_suffix_text(::std::string_view s, ::std::string_view suffix) {
  if (suffix.empty() || !ends_with_sv(s, suffix)) return ::std::string(s);
  return ::std::string(s.substr(0, s.size() - suffix.size()));
}

static ::std::string ensure_prefix_text(::std::string_view s, ::std::string_view prefix) {
  if (prefix.empty() || starts_with_sv(s, prefix)) return ::std::string(s);
  ::std::string out;
  out.reserve(prefix.size() + s.size());
  out.append(prefix);
  out.append(s);
  return out;
}

static ::std::string ensure_suffix_text(::std::string_view s, ::std::string_view suffix) {
  if (suffix.empty() || ends_with_sv(s, suffix)) return ::std::string(s);
  ::std::string out;
  out.reserve(s.size() + suffix.size());
  out.append(s);
  out.append(suffix);
  return out;
}

static ::std::string surround_text(::std::string_view s, ::std::string_view left, ::std::string_view right) {
  ::std::string out;
  out.reserve(left.size() + s.size() + right.size());
  out.append(left);
  out.append(s);
  out.append(right);
  return out;
}

static ::std::string quote_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() + 2);
  out.push_back('"');
  out.append(s);
  out.push_back('"');
  return out;
}

static ::std::string unquote_text(::std::string_view s) {
  if (s.size() < 2) return ::std::string(s);
  const char first = s.front();
  const char last = s.back();
  if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
    return ::std::string(s.substr(1, s.size() - 2));
  }
  return ::std::string(s);
}

static int hex_nibble(unsigned char c) {
  if (c >= '0' && c <= '9') return static_cast<int>(c - '0');
  if (c >= 'a' && c <= 'f') return static_cast<int>(10 + c - 'a');
  if (c >= 'A' && c <= 'F') return static_cast<int>(10 + c - 'A');
  return -1;
}

static int b64_index(unsigned char c) {
  if (c >= 'A' && c <= 'Z') return static_cast<int>(c - 'A');
  if (c >= 'a' && c <= 'z') return static_cast<int>(26 + c - 'a');
  if (c >= '0' && c <= '9') return static_cast<int>(52 + c - '0');
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

static bool is_url_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

static void append_utf8_cp(::std::uint32_t cp, ::std::string& out) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
    return;
  }
  if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    return;
  }
  if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    return;
  }
  if (cp <= 0x10FFFF) {
    out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  }
}

static ::std::string hex_encode_text(::std::string_view s) {
  static constexpr char kHex[] = "0123456789abcdef";
  ::std::string out;
  out.resize(s.size() * 2);
  for (::std::size_t i = 0; i < s.size(); ++i) {
    const unsigned char b = static_cast<unsigned char>(s[i]);
    out[2 * i] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

static ::std::string hex_decode_text(::std::string_view s) {
  if ((s.size() & 1u) != 0u) return ::std::string(s);
  ::std::string out;
  out.reserve(s.size() / 2);
  for (::std::size_t i = 0; i < s.size(); i += 2) {
    const int hi = hex_nibble(static_cast<unsigned char>(s[i]));
    const int lo = hex_nibble(static_cast<unsigned char>(s[i + 1]));
    if (hi < 0 || lo < 0) return ::std::string(s);
    out.push_back(static_cast<char>((hi << 4) | lo));
  }
  return out;
}

static ::std::string base64_encode_text(::std::string_view s) {
  static constexpr char kB64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  ::std::string out;
  out.reserve(((s.size() + 2) / 3) * 4);
  ::std::size_t i = 0;
  while (i + 2 < s.size()) {
    const unsigned char b0 = static_cast<unsigned char>(s[i++]);
    const unsigned char b1 = static_cast<unsigned char>(s[i++]);
    const unsigned char b2 = static_cast<unsigned char>(s[i++]);
    out.push_back(kB64[b0 >> 2]);
    out.push_back(kB64[((b0 & 0x03) << 4) | (b1 >> 4)]);
    out.push_back(kB64[((b1 & 0x0F) << 2) | (b2 >> 6)]);
    out.push_back(kB64[b2 & 0x3F]);
  }
  const ::std::size_t rem = s.size() - i;
  if (rem == 1) {
    const unsigned char b0 = static_cast<unsigned char>(s[i]);
    out.push_back(kB64[b0 >> 2]);
    out.push_back(kB64[(b0 & 0x03) << 4]);
    out.push_back('=');
    out.push_back('=');
  } else if (rem == 2) {
    const unsigned char b0 = static_cast<unsigned char>(s[i]);
    const unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
    out.push_back(kB64[b0 >> 2]);
    out.push_back(kB64[((b0 & 0x03) << 4) | (b1 >> 4)]);
    out.push_back(kB64[(b1 & 0x0F) << 2]);
    out.push_back('=');
  }
  return out;
}

static ::std::string base64_decode_text(::std::string_view s) {
  ::std::string out;
  out.reserve((s.size() / 4) * 3);
  ::std::uint32_t acc = 0;
  int bits = 0;
  for (char c : s) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (::std::isspace(uc) != 0) continue;
    if (c == '=') break;
    const int v = b64_index(uc);
    if (v < 0) return ::std::string(s);
    acc = (acc << 6) | static_cast<::std::uint32_t>(v);
    bits += 6;
    while (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<char>((acc >> bits) & 0xFFu));
      if (bits == 0) acc = 0;
      else acc &= (1u << bits) - 1u;
    }
  }
  return out;
}

static ::std::string url_encode_text(::std::string_view s) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  ::std::string out;
  out.reserve(s.size() * 3 / 2 + 8);
  for (unsigned char c : s) {
    if (is_url_unreserved(c)) {
      out.push_back(static_cast<char>(c));
      continue;
    }
    out.push_back('%');
    out.push_back(kHex[(c >> 4) & 0x0F]);
    out.push_back(kHex[c & 0x0F]);
  }
  return out;
}

static ::std::string url_decode_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '+' ) {
      out.push_back(' ');
      continue;
    }
    if (c == '%' && i + 2 < s.size()) {
      const int hi = hex_nibble(static_cast<unsigned char>(s[i + 1]));
      const int lo = hex_nibble(static_cast<unsigned char>(s[i + 2]));
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(c);
  }
  return out;
}

static ::std::string html_escape_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() + s.size() / 8);
  for (char c : s) {
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else if (c == '\'') out += "&#39;";
    else out.push_back(c);
  }
  return out;
}

static ::std::string html_unescape_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] != '&') {
      out.push_back(s[i]);
      continue;
    }
    const auto semi = s.find(';', i + 1);
    if (semi == ::std::string_view::npos) {
      out.push_back('&');
      continue;
    }
    const ::std::string_view ent = s.substr(i + 1, semi - (i + 1));
    bool replaced = true;
    if (ent == "amp") out.push_back('&');
    else if (ent == "lt") out.push_back('<');
    else if (ent == "gt") out.push_back('>');
    else if (ent == "quot") out.push_back('"');
    else if (ent == "apos" || ent == "#39") out.push_back('\'');
    else if (!ent.empty() && ent.front() == '#') {
      ::std::uint32_t cp = 0;
      bool ok = true;
      ::std::size_t j = 1;
      int base = 10;
      if (j < ent.size() && (ent[j] == 'x' || ent[j] == 'X')) {
        base = 16;
        ++j;
      }
      if (j >= ent.size()) ok = false;
      for (; ok && j < ent.size(); ++j) {
        const unsigned char d = static_cast<unsigned char>(ent[j]);
        int v = -1;
        if (base == 10) {
          if (d >= '0' && d <= '9') v = static_cast<int>(d - '0');
        } else {
          v = hex_nibble(d);
        }
        if (v < 0) {
          ok = false;
          break;
        }
        cp = cp * static_cast<::std::uint32_t>(base) + static_cast<::std::uint32_t>(v);
        if (cp > 0x10FFFFu) {
          ok = false;
          break;
        }
      }
      if (ok) append_utf8_cp(cp, out);
      else replaced = false;
    } else {
      replaced = false;
    }
    if (!replaced) {
      out.push_back('&');
      out.append(ent.data(), ent.size());
      out.push_back(';');
    }
    i = semi;
  }
  return out;
}

static ::std::string json_escape_text(::std::string_view s) {
  static constexpr char kHex[] = "0123456789abcdef";
  ::std::string out;
  out.reserve(s.size() + s.size() / 8);
  for (unsigned char c : s) {
    switch (c) {
      case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          out.push_back(kHex[(c >> 4) & 0x0F]);
          out.push_back(kHex[c & 0x0F]);
        } else {
          out.push_back(static_cast<char>(c));
        }
        break;
    }
  }
  return out;
}

static bool decode_utf8_len(::std::string_view s, ::std::size_t i, ::std::size_t& cp_len) {
  if (i >= s.size()) return false;
  const unsigned char b0 = static_cast<unsigned char>(s[i]);
  if (b0 < 0x80) {
    cp_len = 1;
    return true;
  }
  if ((b0 & 0xE0) == 0xC0 && i + 1 < s.size() &&
      (static_cast<unsigned char>(s[i + 1]) & 0xC0) == 0x80) {
    cp_len = 2;
    return true;
  }
  if ((b0 & 0xF0) == 0xE0 && i + 2 < s.size() &&
      (static_cast<unsigned char>(s[i + 1]) & 0xC0) == 0x80 &&
      (static_cast<unsigned char>(s[i + 2]) & 0xC0) == 0x80) {
    cp_len = 3;
    return true;
  }
  if ((b0 & 0xF8) == 0xF0 && i + 3 < s.size() &&
      (static_cast<unsigned char>(s[i + 1]) & 0xC0) == 0x80 &&
      (static_cast<unsigned char>(s[i + 2]) & 0xC0) == 0x80 &&
      (static_cast<unsigned char>(s[i + 3]) & 0xC0) == 0x80) {
    cp_len = 4;
    return true;
  }
  cp_len = 1;
  return true;
}

static void split_lines_to(::std::string_view s, bool keep_ends, ::std::vector<::std::string>& out) {
  ::std::size_t start = 0;
  while (start <= s.size()) {
    ::std::size_t end = start;
    while (end < s.size() && s[end] != '\n' && s[end] != '\r') ++end;
    if (end == s.size()) {
      if (start < s.size()) out.emplace_back(s.substr(start));
      break;
    }
    ::std::size_t delim_len = 1;
    if (s[end] == '\r' && end + 1 < s.size() && s[end + 1] == '\n') delim_len = 2;
    if (keep_ends) out.emplace_back(s.substr(start, end - start + delim_len));
    else out.emplace_back(s.substr(start, end - start));
    start = end + delim_len;
    if (start == s.size()) break;
  }
}

static void split_chars_to(::std::string_view s, ::std::vector<::std::string>& out) {
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t cp_len = 1;
    decode_utf8_len(s, i, cp_len);
    out.emplace_back(s.substr(i, cp_len));
    i += cp_len;
  }
}

static void tokenize_words_to(::std::string_view s, ::std::vector<::std::string>& out) {
  const auto spans = ::pluss::coden::native::cpu::split_word_spans(s);
  out.reserve(out.size() + spans.size());
  for (const auto& [off, len] : spans) out.emplace_back(s.substr(off, len));
}

// tokenize_words(flags): bit 0 (value 1) case-folds every emitted token; the
// token boundaries are identical to the no-flags form. flags <= 0 is unchanged.
static void tokenize_words_flagged_to(::std::string_view s, ::std::int64_t flags,
                                      ::std::vector<::std::string>& out) {
  const ::std::size_t start = out.size();
  tokenize_words_to(s, out);
  if (flags > 0 && (flags & 1)) {
    for (::std::size_t i = start; i < out.size(); ++i) {
      out[i] = ::pluss::coden::native::cpu::lower_ascii(out[i]);
    }
  }
}

static void tokenize_sentences_to(::std::string_view s, ::std::vector<::std::string>& out) {
  bool ascii_only = true;
  for (unsigned char c : s) {
    if (c >= 0x80) {
      ascii_only = false;
      break;
    }
  }
  if (ascii_only) {
    const auto spans = ::pluss::coden::native::cpu::split_sentences_spans_ascii(s);
    out.reserve(out.size() + spans.size());
    for (const auto& [off, len] : spans) out.emplace_back(s.substr(off, len));
    return;
  }

  ::std::size_t start = 0;
  for (::std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c != '.' && c != '!' && c != '?') continue;
    ::std::size_t j = i + 1;
    while (j < s.size() && (s[j] == '.' || s[j] == '!' || s[j] == '?')) ++j;
    const bool boundary = (j == s.size()) || ::std::isspace(static_cast<unsigned char>(s[j]));
    if (!boundary) continue;
    while (start < i && ::std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    ::std::size_t end = i + 1;
    while (end > start && ::std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    if (end > start) out.emplace_back(s.substr(start, end - start));
    start = j;
  }
  while (start < s.size() && ::std::isspace(static_cast<unsigned char>(s[start]))) ++start;
  if (start < s.size()) out.emplace_back(s.substr(start));
}

static void tokenize_punct_to(::std::string_view s, ::std::vector<::std::string>& out) {
  for (unsigned char c : s) {
    if ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126)) {
      out.emplace_back(1, static_cast<char>(c));
    }
  }
}

static void tokenize_regex_to(::std::string_view s, const ::std::regex& re, ::std::vector<::std::string>& out) {
  ::std::string owned(s);
  for (::std::sregex_iterator it(owned.begin(), owned.end(), re), end; it != end; ++it) {
    if ((*it).size() > 1 && (*it)[1].matched) out.push_back((*it)[1].str());
    else out.push_back((*it)[0].str());
  }
}

// `regex_findall` accepts an optional flags bitmask: bit 0 (value 1) makes the
// match case-insensitive, bit 1 (value 2) makes `^`/`$` match at line breaks.
// A flags value <= 0 keeps the default ECMAScript, case-sensitive behavior.
static ::std::regex make_findall_regex(::std::string_view pattern, ::std::int64_t flags) {
  auto syntax = ::std::regex::ECMAScript;
  if (flags > 0) {
    if (flags & 1) syntax |= ::std::regex::icase;
    if (flags & 2) syntax |= ::std::regex::multiline;
  }
  return ::std::regex(::std::string(pattern), syntax);
}

static void regex_split_to(::std::string_view s, const ::std::regex& re, ::std::int64_t limit, ::std::vector<::std::string>& out) {
  ::std::string owned(s);
  ::std::size_t pos = 0;
  ::std::int64_t splits = 0;
  for (::std::sregex_iterator it(owned.begin(), owned.end(), re), end; it != end; ++it) {
    if (limit >= 0 && splits >= limit) break;
    const auto m = *it;
    const ::std::size_t mpos = static_cast<::std::size_t>(m.position());
    out.emplace_back(owned.substr(pos, mpos - pos));
    pos = mpos + static_cast<::std::size_t>(m.length());
    ++splits;
  }
  out.emplace_back(owned.substr(pos));
}

static void split_delim_with_limit_to(::std::string_view s,
                                      ::std::string_view delim,
                                      ::std::int64_t limit,
                                      ::std::vector<::std::string>& out) {
  PLUSS_CHECK(!delim.empty(), "split: delimiter must be non-empty");
  ::std::size_t pos = 0;
  ::std::int64_t splits = 0;
  while (pos <= s.size()) {
    if (limit >= 0 && splits >= limit) {
      out.emplace_back(s.substr(pos));
      return;
    }
    const ::std::size_t p = s.find(delim, pos);
    if (p == ::std::string_view::npos) {
      out.emplace_back(s.substr(pos));
      return;
    }
    out.emplace_back(s.substr(pos, p - pos));
    pos = p + delim.size();
    ++splits;
  }
}

static void split_whitespace_with_limit_to(::std::string_view s,
                                           ::std::int64_t limit,
                                           ::std::vector<::std::string>& out) {
  auto is_ws = [](unsigned char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  };

  ::std::size_t i = 0;
  ::std::int64_t splits = 0;
  while (i < s.size()) {
    while (i < s.size() && is_ws(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= s.size()) break;
    if (limit >= 0 && splits >= limit) {
      out.emplace_back(::pluss::coden::native::cpu::strip_ascii(s.substr(i)));
      return;
    }
    const ::std::size_t start = i;
    while (i < s.size() && !is_ws(static_cast<unsigned char>(s[i]))) ++i;
    out.emplace_back(s.substr(start, i - start));
    ++splits;
  }
}

static void regex_collect_to(::std::string_view s,
                             const ::std::regex& re,
                             ::std::vector<::std::string>& out,
                             int group = 0,
                             ::std::int64_t limit = -1) {
  ::std::string owned(s);
  ::std::int64_t produced = 0;
  for (::std::sregex_iterator it(owned.begin(), owned.end(), re), end; it != end; ++it) {
    if (limit >= 0 && produced >= limit) break;
    const auto& m = *it;
    if (group > 0 && m.size() > static_cast<::std::size_t>(group) && m[group].matched) out.push_back(m[group].str());
    else out.push_back(m.str());
    ++produced;
  }
}

static bool tex_control_word_at(::std::string_view s,
                                ::std::size_t pos,
                                ::std::size_t* name_begin,
                                ::std::size_t* name_end,
                                ::std::size_t* token_end) {
  if (pos >= s.size() || s[pos] != '\\' || pos + 1 >= s.size() ||
      s[pos + 1] == '\\') {
    return false;
  }
  const unsigned char first = static_cast<unsigned char>(s[pos + 1]);
  if (::std::isalpha(first) == 0 && s[pos + 1] != '@') return false;

  *name_begin = pos + 1;
  *name_end = pos + 2;
  while (*name_end < s.size()) {
    const unsigned char c = static_cast<unsigned char>(s[*name_end]);
    if (::std::isalpha(c) == 0 && s[*name_end] != '@') break;
    ++*name_end;
  }
  *token_end = *name_end;
  if (*token_end < s.size() && s[*token_end] == '*') ++*token_end;
  return true;
}

static bool tex_group_end(::std::string_view s,
                          ::std::size_t open_pos,
                          char open,
                          char close,
                          ::std::size_t* next_pos);

static ::std::size_t tex_skip_space(::std::string_view s, ::std::size_t pos) {
  while (pos < s.size() &&
         ::std::isspace(static_cast<unsigned char>(s[pos])) != 0) {
    ++pos;
  }
  return pos;
}

static bool tex_skip_optional_group(::std::string_view s,
                                    ::std::size_t* pos) {
  *pos = tex_skip_space(s, *pos);
  if (*pos >= s.size() || s[*pos] != '[') return true;
  ::std::size_t next = 0;
  if (!tex_group_end(s, *pos, '[', ']', &next)) return false;
  *pos = tex_skip_space(s, next);
  return true;
}

template <class Accept>
static void extract_tex_command_bodies_to(::std::string_view s,
                                          Accept&& accept,
                                          bool allow_optional,
                                          ::std::vector<::std::string>& out) {
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      i += (s[i] == '\\' && i + 1 < s.size()) ? 2 : 1;
      continue;
    }

    const auto name = s.substr(name_begin, name_end - name_begin);
    if (!accept(name)) {
      i = token_end;
      continue;
    }

    ::std::size_t body_begin = tex_skip_space(s, token_end);
    if (allow_optional && !tex_skip_optional_group(s, &body_begin)) {
      i = token_end;
      continue;
    }
    ::std::size_t next = body_begin;
    ::std::string body;
    if (tex_parse_braced_group(s, body_begin, &next, &body)) {
      out.push_back(::std::move(body));
      i = next;
    } else {
      i = token_end;
    }
  }
}

static void extract_tex_packages_to(::std::string_view s,
                                    ::std::vector<::std::string>& out) {
  ::std::vector<::std::string> groups;
  extract_tex_command_bodies_to(
      s,
      [](::std::string_view name) {
        return name == "usepackage" || name == "RequirePackage" ||
               name == "RequirePackageWithOptions";
      },
      true, groups);

  for (const auto& group : groups) {
    for (::std::size_t begin = 0; begin <= group.size();) {
      const auto comma = group.find(',', begin);
      const auto part = ::pluss::coden::native::cpu::strip_ascii(
          ::std::string_view(group).substr(
              begin, comma == ::std::string::npos ? ::std::string::npos
                                                   : comma - begin));
      if (!part.empty()) out.push_back(part);
      if (comma == ::std::string::npos) break;
      begin = comma + 1;
    }
  }
}

static bool tex_macro_name_at(::std::string_view s,
                              ::std::size_t pos,
                              ::std::size_t* next,
                              ::std::string* name) {
  pos = tex_skip_space(s, pos);
  if (pos < s.size() && s[pos] == '{') {
    ::std::string body;
    ::std::size_t after = 0;
    if (!tex_parse_braced_group(s, pos, &after, &body)) return false;
    const auto trimmed = ::pluss::coden::native::cpu::strip_ascii(body);
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(trimmed, 0, &name_begin, &name_end, &token_end) ||
        token_end != trimmed.size()) {
      return false;
    }
    name->assign(trimmed);
    *next = after;
    return true;
  }

  ::std::size_t name_begin = 0;
  ::std::size_t name_end = 0;
  ::std::size_t token_end = 0;
  if (!tex_control_word_at(s, pos, &name_begin, &name_end, &token_end)) {
    return false;
  }
  name->assign(s.substr(pos, token_end - pos));
  *next = token_end;
  return true;
}

static void extract_tex_macro_defs_to(::std::string_view s,
                                      ::std::vector<::std::string>& out) {
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t command_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &command_end)) {
      i += (s[i] == '\\' && i + 1 < s.size()) ? 2 : 1;
      continue;
    }

    const auto command = s.substr(name_begin, name_end - name_begin);
    const bool new_family = command == "newcommand" ||
        command == "renewcommand" || command == "providecommand";
    if (!new_family && command != "def") {
      i = command_end;
      continue;
    }

    ::std::size_t cursor = command_end;
    ::std::string macro;
    if (!tex_macro_name_at(s, cursor, &cursor, &macro)) {
      i = command_end;
      continue;
    }

    if (new_family) {
      cursor = tex_skip_space(s, cursor);
      while (cursor < s.size() && s[cursor] == '[') {
        if (!tex_skip_optional_group(s, &cursor)) {
          cursor = ::std::string_view::npos;
          break;
        }
      }
    } else {
      while (cursor < s.size() && s[cursor] != '{') {
        if (s[cursor] == '\\' && cursor + 1 < s.size()) ++cursor;
        ++cursor;
      }
    }

    ::std::size_t declaration_end = 0;
    ::std::string body;
    if (cursor != ::std::string_view::npos &&
        tex_parse_braced_group(s, cursor, &declaration_end, &body)) {
      out.push_back(::std::move(macro));
      i = declaration_end;
    } else {
      i = command_end;
    }
  }
}

static bool tex_group_end(::std::string_view s,
                          ::std::size_t open_pos,
                          char open,
                          char close,
                          ::std::size_t* next_pos) {
  if (open_pos >= s.size() || s[open_pos] != open) return false;
  ::std::int64_t depth = 0;
  for (::std::size_t i = open_pos; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      ++i;
      continue;
    }
    if (s[i] == open) {
      ++depth;
    } else if (s[i] == close) {
      if (--depth == 0) {
        *next_pos = i + 1;
        return true;
      }
    }
  }
  return false;
}

static void extract_tex_commands_to(::std::string_view s,
                                    bool with_args,
                                    ::std::vector<::std::string>& out) {
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      i += (s[i] == '\\' && i + 1 < s.size()) ? 2 : 1;
      continue;
    }

    if (with_args) {
      ::std::size_t cursor = token_end;
      while (true) {
        ::std::size_t group_begin = cursor;
        while (group_begin < s.size() &&
               (s[group_begin] == ' ' || s[group_begin] == '\t' ||
                s[group_begin] == '\n' || s[group_begin] == '\r')) {
          ++group_begin;
        }
        if (group_begin >= s.size() ||
            (s[group_begin] != '[' && s[group_begin] != '{')) {
          break;
        }
        ::std::size_t group_end = 0;
        const char open = s[group_begin];
        if (!tex_group_end(s, group_begin, open, open == '[' ? ']' : '}',
                           &group_end)) {
          break;
        }
        cursor = group_end;
      }
      token_end = cursor;
    }

    out.emplace_back(s.substr(i, token_end - i));
    i = token_end;
  }
}

static void keep_unique_in_place(::std::vector<::std::string>& v) {
  ::std::unordered_set<::std::string> seen;
  ::std::vector<::std::string> out;
  out.reserve(v.size());
  for (auto& x : v) {
    if (seen.insert(x).second) out.push_back(::std::move(x));
  }
  v = ::std::move(out);
}

static bool is_stopword(::std::string_view w) {
  static const ::std::unordered_set<::std::string_view> kStop = {
      "a", "an", "and", "are", "as", "at", "be", "by", "for", "from", "in", "into",
      "is", "it", "its", "of", "on", "or", "that", "the", "their", "this", "to", "with"};
  return kStop.find(w) != kStop.end();
}

static ::std::string stem_simple(::std::string w) {
  if (w.size() > 5 && ends_with_sv(w, "ing")) w.erase(w.size() - 3);
  else if (w.size() > 4 && ends_with_sv(w, "ed")) w.erase(w.size() - 2);
  else if (w.size() > 4 && ends_with_sv(w, "es")) w.erase(w.size() - 2);
  else if (w.size() > 3 && ends_with_sv(w, "s")) w.erase(w.size() - 1);
  return w;
}

static void word_tokens_lower_filtered(::std::string_view s,
                                       bool alpha_only,
                                       bool alnum_only,
                                       ::std::int64_t min_len,
                                       ::std::int64_t max_len,
                                       ::std::vector<::std::string>& words) {
  ::pluss::coden::native::cpu::tokenize_words_ascii_lower_filtered(
      s, words, alpha_only, alnum_only, min_len, max_len);
}

static void word_tokens_lower(::std::string_view s, ::std::vector<::std::string>& words) {
  word_tokens_lower_filtered(s, false, false, 0, -1, words);
}

static bool is_ascii_punct_byte(unsigned char c) {
  return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126);
}

static bool has_ascii_upper(::std::string_view s) {
  for (unsigned char c : s) if (c >= 'A' && c <= 'Z') return true;
  return false;
}

static bool has_ascii_digit(::std::string_view s) {
  for (unsigned char c : s) if (c >= '0' && c <= '9') return true;
  return false;
}

static bool has_any_char_from(::std::string_view s, ::std::string_view chars) {
  return s.find_first_of(chars) != ::std::string_view::npos;
}

static bool contains_ascii_nocase(::std::string_view s, ::std::string_view needle) {
  if (needle.empty() || needle.size() > s.size()) return false;
  for (::std::size_t i = 0; i + needle.size() <= s.size(); ++i) {
    bool ok = true;
    for (::std::size_t j = 0; j < needle.size(); ++j) {
      unsigned char a = static_cast<unsigned char>(s[i + j]);
      unsigned char b = static_cast<unsigned char>(needle[j]);
      if (a >= 'A' && a <= 'Z') a = static_cast<unsigned char>(a - 'A' + 'a');
      if (b >= 'A' && b <= 'Z') b = static_cast<unsigned char>(b - 'A' + 'a');
      if (a != b) {
        ok = false;
        break;
      }
    }
    if (ok) return true;
  }
  return false;
}

static void split_on_casechange_to(::std::string_view s, ::std::vector<::std::string>& out) {
  ::std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && !::std::isalnum(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= s.size()) break;
    ::std::size_t b = i++;
    while (i < s.size()) {
      const unsigned char prev = static_cast<unsigned char>(s[i - 1]);
      const unsigned char cur = static_cast<unsigned char>(s[i]);
      if (!::std::isalnum(cur)) break;
      const bool boundary =
          (::std::islower(prev) && ::std::isupper(cur)) ||
          (::std::isalpha(prev) && ::std::isdigit(cur)) ||
          (::std::isdigit(prev) && ::std::isalpha(cur));
      if (boundary) break;
      ++i;
    }
    out.emplace_back(s.substr(b, i - b));
  }
}

static void sort_tokens_in_place(::std::vector<::std::string>& v) {
  ::std::sort(v.begin(), v.end());
}

static void shuffle_tokens_in_place(::std::vector<::std::string>& v) {
  ::std::uint64_t seed = 1469598103934665603ull;
  for (const auto& t : v) {
    for (unsigned char c : t) {
      seed ^= c;
      seed *= 1099511628211ull;
    }
  }
  ::std::mt19937_64 rng(seed);
  ::std::shuffle(v.begin(), v.end(), rng);
}

static void emit_word_ngrams(::std::span<const ::std::string> words, ::std::int64_t n, ::std::vector<::std::string>& out) {
  if (n <= 0 || words.size() < static_cast<::std::size_t>(n)) return;
  for (::std::size_t i = 0; i + static_cast<::std::size_t>(n) <= words.size(); ++i) {
    ::std::string g;
    for (::std::int64_t j = 0; j < n; ++j) {
      if (j > 0) g.push_back(' ');
      g.append(words[i + static_cast<::std::size_t>(j)]);
    }
    out.push_back(::std::move(g));
  }
}

static void emit_char_ngrams(::std::string_view s, ::std::int64_t n, ::std::vector<::std::string>& out) {
  if (n <= 0) return;
  ::std::vector<::std::string> chars;
  split_chars_to(s, chars);
  if (chars.size() < static_cast<::std::size_t>(n)) return;
  for (::std::size_t i = 0; i + static_cast<::std::size_t>(n) <= chars.size(); ++i) {
    ::std::string gram;
    for (::std::int64_t j = 0; j < n; ++j)
      gram.append(chars[i + static_cast<::std::size_t>(j)]);
    out.push_back(::std::move(gram));
  }
}

static void top_k_frequent(::std::span<const ::std::string> words, ::std::int64_t k, ::std::vector<::std::string>& out) {
  if (k <= 0 || words.empty()) return;
  if (!coden_count_fastpath_enabled()) {
    ::std::unordered_map<::std::string, ::std::int64_t> freq;
    for (const auto& w : words) {
      if (is_stopword(w)) continue;
      ++freq[w];
    }
    ::std::vector<::std::pair<::std::string, ::std::int64_t>> items;
    items.reserve(freq.size());
    for (auto& kv : freq) items.push_back(kv);
    ::std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
      if (a.second != b.second) return a.second > b.second;
      return a.first < b.first;
    });
    const ::std::size_t take = ::std::min<::std::size_t>(static_cast<::std::size_t>(k), items.size());
    out.reserve(out.size() + take);
    for (::std::size_t i = 0; i < take; ++i) out.push_back(items[i].first);
    return;
  }

  ::std::vector<::std::string_view> filtered;
  filtered.reserve(words.size());
  for (const auto& w : words) {
    if (!is_stopword(w)) filtered.emplace_back(w);
  }
  if (filtered.empty()) return;

  ::std::vector<::std::uint64_t> hashes(filtered.size(), 0);
  ::pluss::coden::native::cpu::hash_bytes_fnv1a_batch(
      ::std::span<const ::std::string_view>(filtered.data(), filtered.size()),
      ::std::span<::std::uint64_t>(hashes.data(), hashes.size()));

  ::std::unordered_map<::std::uint64_t, ::std::vector<::std::uint32_t>> buckets;
  buckets.reserve(filtered.size() * 2);
  ::std::vector<::std::string_view> dict;
  dict.reserve(filtered.size());
  ::std::vector<::std::int64_t> counts;
  counts.reserve(filtered.size());

  for (::std::size_t i = 0; i < filtered.size(); ++i) {
    const auto h = hashes[i];
    const auto tok = filtered[i];
    auto& bucket = buckets[h];

    ::std::uint32_t code = ::std::numeric_limits<::std::uint32_t>::max();
    for (const auto idx : bucket) {
      if (dict[static_cast<::std::size_t>(idx)] == tok) {
        code = idx;
        break;
      }
    }
    if (code == ::std::numeric_limits<::std::uint32_t>::max()) {
      code = static_cast<::std::uint32_t>(dict.size());
      dict.push_back(tok);
      counts.push_back(1);
      bucket.push_back(code);
    } else {
      ++counts[static_cast<::std::size_t>(code)];
    }
  }

  ::std::vector<::std::uint32_t> order(dict.size());
  ::std::iota(order.begin(), order.end(), 0u);
  ::std::stable_sort(order.begin(), order.end(), [&](::std::uint32_t a, ::std::uint32_t b) {
    const auto ca = counts[static_cast<::std::size_t>(a)];
    const auto cb = counts[static_cast<::std::size_t>(b)];
    if (ca != cb) return ca > cb;
    return ::pluss::coden::native::cpu::compare_lex_bytes(
        dict[static_cast<::std::size_t>(a)],
        dict[static_cast<::std::size_t>(b)],
        ::pluss::coden::native::cpu::LexCompareOp::Lt);
  });

  const ::std::size_t take = ::std::min<::std::size_t>(static_cast<::std::size_t>(k), order.size());
  out.reserve(out.size() + take);
  for (::std::size_t i = 0; i < take; ++i) {
    out.emplace_back(dict[static_cast<::std::size_t>(order[i])]);
  }
}

static void extract_tex_environment_blocks_to(::std::string_view s,
                                              ::std::string_view env,
                                              ::std::vector<::std::string>& out,
                                              ::std::int64_t limit = -1) {
  const ::std::string begin = "\\begin{" + ::std::string(env) + "}";
  const ::std::string end = "\\end{" + ::std::string(env) + "}";
  auto escaped = [s](::std::size_t pos) {
    ::std::size_t slashes = 0;
    while (pos > slashes && s[pos - slashes - 1] == '\\') ++slashes;
    return (slashes & 1U) != 0U;
  };
  for (::std::size_t search = 0; search < s.size();) {
    const ::std::size_t first = s.find(begin, search);
    if (first == ::std::string_view::npos) return;
    if (escaped(first)) {
      search = first + begin.size();
      continue;
    }
    ::std::size_t cursor = first + begin.size();
    ::std::int64_t depth = 1;
    while (cursor < s.size()) {
      const ::std::size_t next_begin = s.find(begin, cursor);
      const ::std::size_t next_end = s.find(end, cursor);
      if (next_end == ::std::string_view::npos) return;
      if (next_begin != ::std::string_view::npos && next_begin < next_end) {
        cursor = next_begin + begin.size();
        if (!escaped(next_begin)) ++depth;
        continue;
      }
      cursor = next_end + end.size();
      if (escaped(next_end)) continue;
      if (--depth == 0) {
        out.emplace_back(s.substr(first, cursor - first));
        if (limit >= 0 && static_cast<::std::int64_t>(out.size()) >= limit) return;
        search = cursor;
        break;
      }
    }
    if (depth != 0) return;
  }
}

static void extract_tex_environment_variants_to(
    ::std::string_view s,
    ::std::span<const ::std::string_view> envs,
    ::std::vector<::std::string>& out) {
  auto escaped = [s](::std::size_t pos) {
    ::std::size_t slashes = 0;
    while (pos > slashes && s[pos - slashes - 1] == '\\') ++slashes;
    return (slashes & 1U) != 0U;
  };

  for (::std::size_t search = 0; search < s.size();) {
    ::std::size_t first = ::std::string_view::npos;
    ::std::string_view selected;
    for (const auto env : envs) {
      const ::std::string opener = "\\begin{" + ::std::string(env) + "}";
      ::std::size_t candidate = s.find(opener, search);
      while (candidate != ::std::string_view::npos && escaped(candidate)) {
        candidate = s.find(opener, candidate + opener.size());
      }
      if (candidate < first) {
        first = candidate;
        selected = env;
      }
    }
    if (first == ::std::string_view::npos) return;

    const ::std::string begin = "\\begin{" + ::std::string(selected) + "}";
    const ::std::string end = "\\end{" + ::std::string(selected) + "}";
    ::std::size_t cursor = first + begin.size();
    ::std::int64_t depth = 1;
    while (cursor < s.size()) {
      ::std::size_t next_begin = s.find(begin, cursor);
      while (next_begin != ::std::string_view::npos && escaped(next_begin)) {
        next_begin = s.find(begin, next_begin + begin.size());
      }
      ::std::size_t next_end = s.find(end, cursor);
      while (next_end != ::std::string_view::npos && escaped(next_end)) {
        next_end = s.find(end, next_end + end.size());
      }
      if (next_end == ::std::string_view::npos) break;
      if (next_begin != ::std::string_view::npos && next_begin < next_end) {
        ++depth;
        cursor = next_begin + begin.size();
        continue;
      }
      cursor = next_end + end.size();
      if (--depth == 0) {
        out.emplace_back(s.substr(first, cursor - first));
        search = cursor;
        break;
      }
    }
    if (depth != 0) search = first + begin.size();
  }
}

enum class TexMathScope { All, Inline, Display };

static void extract_tex_math_blocks_to(::std::string_view s,
                                       ::std::vector<::std::string>& out,
                                       ::std::int64_t limit = -1,
                                       TexMathScope scope = TexMathScope::All) {
  if (limit == 0) return;
  auto escaped = [s](::std::size_t pos) {
    ::std::size_t slashes = 0;
    while (pos > slashes && s[pos - slashes - 1] == '\\') ++slashes;
    return (slashes & 1U) != 0U;
  };
  for (::std::size_t i = 0; i < s.size();) {
    ::std::string_view close;
    ::std::size_t open_size = 0;
    bool single_dollar = false;
    bool is_inline = false;
    if (s.substr(i, 2) == "$$" && !escaped(i)) {
      close = "$$";
      open_size = 2;
    } else if ((s.substr(i, 2) == "\\[" || s.substr(i, 2) == "\\(") &&
               !escaped(i)) {
      close = s[i + 1] == '[' ? "\\]" : "\\)";
      open_size = 2;
      is_inline = s[i + 1] == '(';
    } else if (s[i] == '$' && !escaped(i)) {
      close = "$";
      open_size = 1;
      single_dollar = true;
      is_inline = true;
    } else {
      ++i;
      continue;
    }
    ::std::size_t cursor = i + open_size;
    bool found = false;
    while (cursor < s.size()) {
      const ::std::size_t candidate = s.find(close, cursor);
      if (candidate == ::std::string_view::npos ||
          (single_dollar && s.substr(cursor, candidate - cursor).find('\n') !=
                                ::std::string_view::npos)) {
        break;
      }
      if (!escaped(candidate)) {
        const ::std::size_t after = candidate + close.size();
        const bool accepted = scope == TexMathScope::All ||
            (scope == TexMathScope::Inline && is_inline) ||
            (scope == TexMathScope::Display && !is_inline);
        if (accepted) {
          out.emplace_back(s.substr(i, after - i));
          if (limit >= 0 && static_cast<::std::int64_t>(out.size()) >= limit) return;
        }
        i = after;
        found = true;
        break;
      }
      cursor = candidate + close.size();
    }
    if (!found) i += open_size;
  }
}

static void token_feature_extract_to(::std::string_view s,
                                     ::std::string_view op,
                                     ::std::int64_t iarg,
                                     ::std::string_view sarg,
                                     ::std::vector<::std::string>& out) {
  if (op == "split_sentences" || op == "nlp_sentences") {
    tokenize_sentences_to(s, out);
    return;
  }
  if (op == "split_csv") {
    split_delim_with_limit_to(s, ",", -1, out);
    return;
  }
  if (op == "split_tsv") {
    split_delim_with_limit_to(s, "\t", -1, out);
    return;
  }
  if (op == "split_on_punct") {
    const auto spans = ::pluss::coden::native::cpu::split_on_punct_spans(s);
    out.reserve(out.size() + spans.size());
    for (const auto& [off, len] : spans) out.emplace_back(s.substr(off, len));
    return;
  }
  if (op == "split_on_snakecase") {
    split_delim_with_limit_to(s, "_", -1, out);
    ::std::vector<::std::string> filtered;
    filtered.reserve(out.size());
    for (auto& t : out) if (!t.empty()) filtered.push_back(::std::move(t));
    out = ::std::move(filtered);
    return;
  }
  if (op == "split_on_casechange" || op == "split_on_camelcase") {
    split_on_casechange_to(s, out);
    return;
  }
  if (op == "split_unicode_words") {
    tokenize_words_to(s, out);
    return;
  }
  if (op == "find_all_regex") {
    if (sarg.empty()) return;
    const ::std::regex re{::std::string(sarg)};
    regex_collect_to(s, re, out, 0, iarg);
    return;
  }
  if (op == "filter_tokens") {
    tokenize_words_to(s, out);
    if (!sarg.empty()) {
      const ::std::regex re{::std::string(sarg)};
      ::std::vector<::std::string> keep;
      keep.reserve(out.size());
      for (const auto& t : out) {
        if (::std::regex_search(t, re)) keep.push_back(t);
      }
      out = ::std::move(keep);
    }
    return;
  }
  if (op == "map_tokens") {
    tokenize_words_to(s, out);
    if (sarg.empty()) return;
    for (auto& t : out) {
      if (sarg == "lower") t = ::pluss::coden::native::cpu::lower_ascii(t);
      else if (sarg == "upper") t = ::pluss::coden::native::cpu::upper_ascii(t);
      else if (sarg == "stem") t = stem_simple(::pluss::coden::native::cpu::lower_ascii(t));
      else if (sarg == "strip_punct") {
        ::std::string u;
        u.reserve(t.size());
        for (unsigned char c : t) if (!is_ascii_punct_byte(c)) u.push_back(static_cast<char>(c));
        t = ::std::move(u);
      }
    }
    return;
  }
  if (op == "sort_tokens") {
    tokenize_words_to(s, out);
    sort_tokens_in_place(out);
    return;
  }
  if (op == "unique_tokens") {
    tokenize_words_to(s, out);
    keep_unique_in_place(out);
    return;
  }
  if (op == "nlp_tokens" || op == "nlp_words") {
    tokenize_words_to(s, out);
    return;
  }
  if (op == "nlp_lowercase_tokens") {
    word_tokens_lower(s, out);
    return;
  }
  if (op == "nlp_unique_tokens") {
    word_tokens_lower(s, out);
    keep_unique_in_place(out);
    return;
  }
  if (op == "nlp_sort_tokens") {
    word_tokens_lower(s, out);
    sort_tokens_in_place(out);
    return;
  }
  if (op == "nlp_shuffle_tokens") {
    tokenize_words_to(s, out);
    shuffle_tokens_in_place(out);
    return;
  }
  if (op == "nlp_filter_tokens_alpha") {
    tokenize_words_to(s, out);
    ::std::vector<::std::string> keep;
    keep.reserve(out.size());
    for (const auto& t : out) {
      bool ok = !t.empty();
      for (unsigned char c : t) if (!::std::isalpha(c)) { ok = false; break; }
      if (ok) keep.push_back(t);
    }
    out = ::std::move(keep);
    return;
  }
  if (op == "nlp_filter_tokens_alnum") {
    tokenize_words_to(s, out);
    ::std::vector<::std::string> keep;
    keep.reserve(out.size());
    for (const auto& t : out) {
      bool ok = !t.empty();
      for (unsigned char c : t) if (!::std::isalnum(c)) { ok = false; break; }
      if (ok) keep.push_back(t);
    }
    out = ::std::move(keep);
    return;
  }
  if (op == "nlp_filter_tokens_len_ge") {
    tokenize_words_to(s, out);
    const ::std::size_t lim = static_cast<::std::size_t>(::std::max<::std::int64_t>(0, iarg));
    ::std::vector<::std::string> keep;
    keep.reserve(out.size());
    for (const auto& t : out) if (t.size() >= lim) keep.push_back(t);
    out = ::std::move(keep);
    return;
  }
  if (op == "nlp_filter_tokens_len_le") {
    tokenize_words_to(s, out);
    const ::std::size_t lim = static_cast<::std::size_t>(::std::max<::std::int64_t>(0, iarg));
    ::std::vector<::std::string> keep;
    keep.reserve(out.size());
    for (const auto& t : out) if (t.size() <= lim) keep.push_back(t);
    out = ::std::move(keep);
    return;
  }
  if (op == "nlp_filter_tokens_regex") {
    tokenize_words_to(s, out);
    if (!sarg.empty()) {
      const ::std::regex re{::std::string(sarg)};
      ::std::vector<::std::string> keep;
      keep.reserve(out.size());
      for (const auto& t : out) if (::std::regex_search(t, re)) keep.push_back(t);
      out = ::std::move(keep);
    }
    return;
  }
  if (op == "nlp_remove_stopword_tokens" || op == "nlp_stopwords_removed_tokens") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    for (auto& w : words) if (!is_stopword(w)) out.push_back(::std::move(w));
    return;
  }
  if (op == "nlp_remove_number_tokens") {
    tokenize_words_to(s, out);
    ::std::vector<::std::string> keep;
    keep.reserve(out.size());
    for (const auto& t : out) {
      bool all_digit = !t.empty();
      for (unsigned char c : t) if (!::std::isdigit(c)) { all_digit = false; break; }
      if (!all_digit) keep.push_back(t);
    }
    out = ::std::move(keep);
    return;
  }
  if (op == "nlp_stopwords") {
    static const ::std::array<::std::string_view, 24> kStop{
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from", "in", "into",
        "is", "it", "its", "of", "on", "or", "that", "the", "their", "this", "to", "with"};
    out.reserve(kStop.size());
    for (auto w : kStop) out.emplace_back(w);
    return;
  }
  if (op == "nlp_chunks" || op == "nlp_verb_phrases") {
    static const ::std::regex re(R"(\b([A-Z][a-z]+(?:\s+[A-Z][a-z]+)*)\b)");
    regex_collect_to(s, re, out, 1);
    keep_unique_in_place(out);
    return;
  }
  if (op == "nlp_stems") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    for (auto& w : words) out.push_back(stem_simple(::std::move(w)));
    return;
  }
  if (op == "nlp_lemmas") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    for (const auto& w : words) {
      out.push_back(::pluss::coden::native::cpu::simple_lemma_ascii(w));
    }
    return;
  }
  if (op == "nlp_bigrams") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    emit_word_ngrams(words, 2, out);
    return;
  }
  if (op == "nlp_trigrams") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    emit_word_ngrams(words, 3, out);
    return;
  }
  if (op == "ngrams_word" || op == "nlp_word_ngrams") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    emit_word_ngrams(words, iarg, out);
    return;
  }
  if (op == "ngrams_char" || op == "nlp_char_ngrams") {
    emit_char_ngrams(s, iarg, out);
    return;
  }
  if (op == "nlp_keywords") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    top_k_frequent(words, iarg, out);
    return;
  }
  if (op == "nlp_keyphrases") {
    ::std::vector<::std::string> words;
    word_tokens_lower(s, words);
    ::std::vector<::std::string> phrases;
    emit_word_ngrams(words, 2, phrases);
    top_k_frequent(phrases, iarg, out);
    return;
  }
  if (op == "nlp_urls") {
    if (!contains_ascii_nocase(s, "http://") && !contains_ascii_nocase(s, "https://") &&
        !contains_ascii_nocase(s, "www.")) {
      return;
    }
    static const ::std::regex re(R"((https?:\/\/[^\s<>"')\]]+|www\.[^\s<>"')\]]+))", ::std::regex::icase);
    regex_collect_to(s, re, out);
    for (auto& url : out) {
      while (!url.empty() && has_any_char_from(
                 ::std::string_view(url).substr(url.size() - 1), ".,;:!?")) {
        url.pop_back();
      }
    }
    return;
  }
  if (op == "nlp_emails") {
    if (s.find('@') == ::std::string_view::npos) return;
    static const ::std::regex re(R"(([A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}))");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "nlp_mentions") {
    if (s.find('@') == ::std::string_view::npos) return;
    static const ::std::regex re(R"((^|[\s(])(@[A-Za-z0-9_]+))", ::std::regex::icase);
    regex_collect_to(s, re, out, 2);
    return;
  }
  if (op == "nlp_hashtags") {
    if (s.find('#') == ::std::string_view::npos) return;
    static const ::std::regex re(R"((^|[\s(])(#[A-Za-z0-9_]+))", ::std::regex::icase);
    regex_collect_to(s, re, out, 2);
    return;
  }
  if (op == "nlp_numbers") {
    if (!has_ascii_digit(s)) return;
    static const ::std::regex re(R"(\b\d+(?:[.,:/-]\d+)*\b)");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "nlp_extract_acronyms") {
    if (!has_ascii_upper(s)) return;
    static const ::std::regex re(R"(\b[A-Z]{2,}\b)");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "nlp_extract_definitions") {
    static const ::std::regex re(R"(\b([A-Za-z][A-Za-z0-9_-]{1,})\s+(?:is|are|means|denotes)\s+([^.;\n]+))", ::std::regex::icase);
    regex_collect_to(s, re, out, 1);
    return;
  }
  if (op == "nlp_extract_terms") {
    ::std::vector<::std::string> words;
    word_tokens_lower_filtered(s, false, false, 4, -1, words);
    for (auto& w : words) {
      if (!is_stopword(w)) out.push_back(::std::move(w));
    }
    keep_unique_in_place(out);
    return;
  }
  if (op == "nlp_extract_citations") {
    if (!has_any_char_from(s, "[(") || !has_ascii_digit(s)) return;
    static const ::std::regex re(R"((\[[0-9]{1,3}\]|\([A-Z][A-Za-z]+,\s*[12][0-9]{3}[a-z]?\)))");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "nlp_extract_math_symbols") {
    if (!has_any_char_from(s, "\\+-*/=<>^")) return;
    static const ::std::regex re(R"((\\[A-Za-z]+|[+\-*/=<>^]))");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "nlp_named_entities") {
    static const ::std::regex re(R"(\b([A-Z][a-z]+(?:\s+[A-Z][a-z]+)*)\b)");
    regex_collect_to(s, re, out, 1);
    keep_unique_in_place(out);
    return;
  }
  if (op == "nlp_pos_tags") {
    ::std::vector<::std::string> words;
    tokenize_words_to(s, words);
    for (auto& w : words) {
      ::std::string tag = "NN";
      if (!w.empty() && ::std::isupper(static_cast<unsigned char>(w.front()))) tag = "NNP";
      else if (w.size() > 3 && ends_with_sv(w, "ing")) tag = "VBG";
      else if (w.size() > 2 && ends_with_sv(w, "ed")) tag = "VBD";
      out.push_back(w + "/" + tag);
    }
    return;
  }
  if (op == "tex_command_names") {
    for (::std::size_t i = 0; i < s.size();) {
      if (s[i] != '\\' || i + 1 >= s.size()) {
        ++i;
        continue;
      }
      if (s[i + 1] == '\\') {
        i += 2;
        continue;
      }
      const unsigned char first = static_cast<unsigned char>(s[i + 1]);
      if (::std::isalpha(first) == 0 && s[i + 1] != '@') {
        i += 2;
        continue;
      }
      ::std::size_t end = i + 2;
      while (end < s.size()) {
        const unsigned char c = static_cast<unsigned char>(s[end]);
        if (::std::isalpha(c) == 0 && s[end] != '@') break;
        ++end;
      }
      if (end < s.size() && s[end] == '*') ++end;
      out.emplace_back(s.substr(i + 1, end - i - 1));
      i = end;
    }
    return;
  }
  if (op == "tex_commands") {
    extract_tex_commands_to(s, iarg > 0, out);
    return;
  }
  if (op == "tex_macros_used") {
    for (::std::size_t i = 0; i < s.size();) {
      ::std::size_t name_begin = 0;
      ::std::size_t name_end = 0;
      ::std::size_t token_end = 0;
      if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
        i += (s[i] == '\\' && i + 1 < s.size()) ? 2 : 1;
        continue;
      }
      out.emplace_back(s.substr(i, token_end - i));
      i = token_end;
    }
    keep_unique_in_place(out);
    return;
  }
  if (op == "tex_extract_command_args") {
    while (!sarg.empty() && sarg.front() == '\\') sarg.remove_prefix(1);
    const ::std::string needle = "\\" + ::std::string(sarg);
    for (::std::size_t search = 0; search < s.size();) {
      const ::std::size_t found = s.find(needle, search);
      if (found == ::std::string_view::npos) break;
      ::std::size_t p = found + needle.size();
      const bool word_name = !sarg.empty() &&
          (::std::isalpha(static_cast<unsigned char>(sarg.back())) != 0 || sarg.back() == '@');
      if ((found > 0 && s[found - 1] == '\\') ||
          (word_name && p < s.size() &&
           (::std::isalpha(static_cast<unsigned char>(s[p])) != 0 || s[p] == '@'))) {
        search = p;
        continue;
      }
      while (p < s.size() && ::std::isspace(static_cast<unsigned char>(s[p])) != 0) ++p;
      if (p < s.size() && s[p] == '[') {
        ::std::int64_t depth = 1;
        for (++p; p < s.size() && depth > 0; ++p) {
          if (s[p] == '\\' && p + 1 < s.size()) { ++p; continue; }
          if (s[p] == '[') ++depth;
          if (s[p] == ']') --depth;
        }
        if (depth != 0) { search = found + needle.size(); continue; }
        while (p < s.size() && ::std::isspace(static_cast<unsigned char>(s[p])) != 0) ++p;
      }
      ::std::size_t next = p;
      ::std::string body;
      if (tex_parse_braced_group(s, p, &next, &body)) {
        out.push_back(::std::move(body));
        search = next;
      } else {
        search = found + needle.size();
      }
    }
    return;
  }
  if (op == "tex_environments" || op == "tex_environment_names") {
    if (op == "tex_environment_names") {
      static const ::std::regex re(R"(\\begin\{([A-Za-z*@]+)\})");
      regex_collect_to(s, re, out, 1, iarg >= 0 ? iarg : -1);
    } else {
      static const ::std::regex re(R"(\\begin\{([A-Za-z*@]+)\}[\s\S]*?\\end\{\1\})");
      regex_collect_to(s, re, out, 0, iarg >= 0 ? iarg : -1);
    }
    return;
  }
  if (starts_with_sv(op, "tex_extract_env_")) {
    ::std::string env(op.substr(16));
    if (env == "align_") env = "align*";
    extract_tex_environment_blocks_to(s, env, out, iarg >= 0 ? iarg : -1);
    return;
  }
  if (op == "tex_math_blocks") {
    extract_tex_math_blocks_to(s, out, iarg);
    return;
  }
  if (op == "tex_inline_math") {
    extract_tex_math_blocks_to(s, out, -1, TexMathScope::Inline);
    return;
  }
  if (op == "tex_display_math") {
    extract_tex_math_blocks_to(s, out, -1, TexMathScope::Display);
    return;
  }
  if (op == "tex_labels") {
    extract_tex_command_bodies_to(
        s, [](::std::string_view name) { return name == "label"; },
        false, out);
    return;
  }
  if (op == "tex_refs") {
    extract_tex_command_bodies_to(
        s,
        [](::std::string_view name) {
          return name == "ref" || name == "eqref" || name == "autoref" ||
                 name == "pageref" || name == "nameref" || name == "cref" ||
                 name == "Cref" || name == "vref" || name == "Vref";
        },
        false, out);
    return;
  }
  if (op == "tex_citations") {
    static const ::std::regex re(
        R"(\\cite[a-zA-Z*]*\s*(?:\[[^\]]*\]\s*)*\{([^{}]*)\})");
    regex_collect_to(s, re, out, 1);
    return;
  }
  if (op == "tex_packages") {
    extract_tex_packages_to(s, out);
    return;
  }
  if (op == "tex_includes") {
    extract_tex_command_bodies_to(
        s,
        [](::std::string_view name) {
          return name == "input" || name == "include" ||
                 name == "includegraphics";
        },
        true, out);
    return;
  }
  if (op == "tex_macros_defined") {
    extract_tex_macro_defs_to(s, out);
    return;
  }
  if (op == "tex_bibliography_keys") {
    static const ::std::regex re(
        R"(\\(?:cite[a-zA-Z*]*\s*(?:\[[^\]]*\]\s*)*|bibitem\s*(?:\[[^\]]*\]\s*)?)\{([^{}]*)\})");
    const ::std::string owned(s);
    for (::std::sregex_iterator it(owned.begin(), owned.end(), re), end;
         it != end; ++it) {
      const ::std::string keys = (*it)[1].str();
      for (::std::size_t start = 0; start <= keys.size();) {
        const auto comma = keys.find(',', start);
        const auto key = ::pluss::coden::native::cpu::strip_ascii(
            ::std::string_view(keys).substr(
                start, comma == ::std::string::npos ? ::std::string::npos
                                                     : comma - start));
        if (!key.empty()) out.push_back(key);
        if (comma == ::std::string::npos) break;
        start = comma + 1;
      }
    }
    return;
  }
  if (op == "tex_sections") {
    extract_tex_command_bodies_to(
        s, [](::std::string_view name) { return name == "section"; },
        true, out);
    return;
  }
  if (op == "tex_subsections") {
    extract_tex_command_bodies_to(
        s, [](::std::string_view name) { return name == "subsection"; },
        true, out);
    return;
  }
  if (op == "tex_subsubsections") {
    extract_tex_command_bodies_to(
        s, [](::std::string_view name) { return name == "subsubsection"; },
        true, out);
    return;
  }
  if (op == "tex_headings") {
    extract_tex_command_bodies_to(
        s,
        [iarg](::std::string_view name) {
          ::std::int64_t depth = 0;
          if (name == "part" || name == "chapter" || name == "section") depth = 1;
          else if (name == "subsection") depth = 2;
          else if (name == "subsubsection") depth = 3;
          else if (name == "paragraph") depth = 4;
          return depth > 0 && depth <= iarg;
        },
        true, out);
    return;
  }
  if (op == "tex_align_blocks") {
    static const ::std::regex re(
        R"(\\begin\{(align\*?)\}[\s\S]*?\\end\{\1\})");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "tex_figures") {
    static constexpr ::std::array<::std::string_view, 2> kFigures{
        "figure", "figure*"};
    extract_tex_environment_variants_to(s, kFigures, out);
    return;
  }
  if (op == "tex_tables") {
    static constexpr ::std::array<::std::string_view, 2> kTables{
        "table", "table*"};
    extract_tex_environment_variants_to(s, kTables, out);
    return;
  }
  if (op == "tex_theorems") {
    extract_tex_environment_blocks_to(s, "theorem", out);
    return;
  }
  if (op == "tex_lemmas") {
    extract_tex_environment_blocks_to(s, "lemma", out);
    return;
  }
  if (op == "tex_corollaries") {
    extract_tex_environment_blocks_to(s, "corollary", out);
    return;
  }
  if (op == "tex_definitions") {
    extract_tex_environment_blocks_to(s, "definition", out);
    return;
  }
  if (op == "tex_examples") {
    extract_tex_environment_blocks_to(s, "example", out);
    return;
  }
  if (op == "tex_proofs") {
    extract_tex_environment_blocks_to(s, "proof", out);
    return;
  }
  if (op == "tex_captions") {
    static const ::std::regex re(
        R"(\\caption\*?\s*(?:\[[^\]]*\]\s*)?\{([^{}]*)\})");
    regex_collect_to(s, re, out, 1);
    return;
  }
  if (op == "tex_footnotes") {
    extract_tex_command_bodies_to(
        s, [](::std::string_view name) { return name == "footnote"; },
        true, out);
    return;
  }
  if (op == "tex_todos") {
    static const ::std::regex re(R"(\\todo\s*\{([^{}]*)\})");
    regex_collect_to(s, re, out, 1);
    return;
  }
  if (op == "tex_warnings" || op == "tex_detect_errors") {
    static const ::std::regex re(R"((\\PackageWarning\{[^{}]*\}\{[^{}]*\}|\\PackageError\{[^{}]*\}\{[^{}]*\}\{[^{}]*\}|Undefined control sequence|LaTeX Error:[^\n]*))");
    regex_collect_to(s, re, out);
    return;
  }
  if (op == "tex_arrays") {
    extract_tex_environment_blocks_to(s, "array", out);
    return;
  }
  if (op == "tex_tabular_blocks") {
    static constexpr ::std::array<::std::string_view, 2> kTabular{
        "tabular", "tabular*"};
    extract_tex_environment_variants_to(s, kTabular, out);
    return;
  }
  if (op == "tex_extract_symbols" || op == "tex_extract_operators" ||
      op == "tex_extract_functions" || op == "tex_extract_numbers" ||
      op == "tex_extract_identifiers" || op == "tex_extract_greek" ||
      op == "tex_extract_plain_math") {
    ::std::vector<::std::string> math_blocks;
    extract_tex_math_blocks_to(s, math_blocks);
    if (op == "tex_extract_plain_math") {
      out.reserve(math_blocks.size());
      for (auto& m : math_blocks) out.push_back(strip_math_delims(m));
      return;
    }
    ::std::string joined;
    for (const auto& m : math_blocks) {
      if (!joined.empty()) joined.push_back(' ');
      joined.append(m);
    }
    if (op == "tex_extract_numbers") {
      static const ::std::regex re(R"(\b\d+(?:\.\d+)?\b)");
      regex_collect_to(joined, re, out);
      return;
    }
    if (op == "tex_extract_operators") {
      static const ::std::regex re(R"(([+\-*/=<>^]|\\(?:times|cdot|pm|mp|leq|geq|neq|approx)))");
      regex_collect_to(joined, re, out);
      return;
    }
    if (op == "tex_extract_identifiers") {
      static const ::std::regex re(R"(\b[A-Za-z][A-Za-z0-9_]*\b)");
      regex_collect_to(joined, re, out);
      return;
    }
    if (op == "tex_extract_functions") {
      static const ::std::regex re(R"(\\(sin|cos|tan|log|ln|exp|max|min|argmax|argmin|det|Pr|sqrt)\b)");
      regex_collect_to(joined, re, out, 1);
      return;
    }
    if (op == "tex_extract_greek") {
      static const ::std::regex re(R"(\\(alpha|beta|gamma|delta|epsilon|varepsilon|zeta|eta|theta|vartheta|iota|kappa|lambda|mu|nu|xi|pi|rho|sigma|tau|upsilon|phi|varphi|chi|psi|omega)\b)");
      regex_collect_to(joined, re, out, 1);
      return;
    }
    static const ::std::regex re(R"((\\[A-Za-z]+|[+\-*/=<>^]))");
    regex_collect_to(joined, re, out);
    return;
  }
}

enum class SeqGranularity {
  Words,
  Lines,
  Chars,
  Sentences
};

static void tokenize_granularity(::std::string_view s, SeqGranularity g, ::std::vector<::std::string>& out) {
  switch (g) {
    case SeqGranularity::Words:
      tokenize_words_to(s, out);
      return;
    case SeqGranularity::Lines:
      split_lines_to(s, false, out);
      return;
    case SeqGranularity::Chars:
      split_chars_to(s, out);
      return;
    case SeqGranularity::Sentences:
      tokenize_sentences_to(s, out);
      return;
  }
}

static ::std::vector<::std::int32_t> lcs_table(::std::span<const ::std::string> a, ::std::span<const ::std::string> b) {
  const ::std::size_t m = a.size();
  const ::std::size_t n = b.size();
  ::std::vector<::std::int32_t> dp((m + 1) * (n + 1), 0);
  auto at = [&](::std::size_t i, ::std::size_t j) -> ::std::int32_t& { return dp[i * (n + 1) + j]; };
  for (::std::size_t i = m; i-- > 0;) {
    for (::std::size_t j = n; j-- > 0;) {
      if (a[i] == b[j]) at(i, j) = at(i + 1, j + 1) + 1;
      else at(i, j) = ::std::max(at(i + 1, j), at(i, j + 1));
    }
  }
  return dp;
}

static void emit_alignment_or_diff(::std::span<const ::std::string> a,
                                   ::std::span<const ::std::string> b,
                                   bool include_equal,
                                   ::std::vector<::std::string>& out) {
  const ::std::size_t m = a.size();
  const ::std::size_t n = b.size();
  const auto dp = lcs_table(a, b);
  auto at = [&](::std::size_t i, ::std::size_t j) -> ::std::int32_t { return dp[i * (n + 1) + j]; };
  auto tagged = [](::std::string_view tok, char tag) {
    ::std::string z;
    z.reserve(tok.size() + 1);
    z.push_back(tag);
    z.append(tok);
    return z;
  };

  ::std::size_t i = 0;
  ::std::size_t j = 0;
  while (i < m || j < n) {
    if (i < m && j < n && a[i] == b[j]) {
      if (include_equal) out.push_back(tagged(a[i], '='));
      ++i;
      ++j;
      continue;
    }
    if (j == n || (i < m && at(i + 1, j) >= at(i, j + 1))) {
      out.push_back(tagged(a[i], '-'));
      ++i;
    } else {
      out.push_back(tagged(b[j], '+'));
      ++j;
    }
  }
}

static void diff_or_align_to(::std::string_view a_text,
                             ::std::string_view b_text,
                             SeqGranularity granularity,
                             bool include_equal,
                             ::std::vector<::std::string>& out) {
  ::std::vector<::std::string> a;
  ::std::vector<::std::string> b;
  tokenize_granularity(a_text, granularity, a);
  tokenize_granularity(b_text, granularity, b);
  emit_alignment_or_diff(a, b, include_equal, out);
}

static bool coden_parallel_enabled() {
  if (coden_reference_mode_enabled()) return false;
  return env_bool_flag("PLUSS_CODEN_PARALLEL", true);
}

static ::std::int64_t coden_parallel_workers(::std::int64_t tasks, ::std::int64_t min_tasks_per_worker) {
  if (!coden_parallel_enabled()) return 1;
  if (tasks <= 0 || min_tasks_per_worker <= 0) return 1;
  const auto hw_u = ::std::thread::hardware_concurrency();
  if (hw_u < 2u) return 1;
  const auto hw = static_cast<::std::int64_t>(hw_u);
  if (tasks < min_tasks_per_worker * 2) return 1;
  return ::std::max<::std::int64_t>(1, ::std::min<::std::int64_t>(hw, tasks));
}

template <class TokenizeFn>
static TokenBatch tokenbatch_from_note(const Note& x, const char* op, TokenizeFn&& fn) {
  if (!x.defined()) {
    NoteImpl::Int64Storage offsets{0};
    return TokenBatchFactoryAccess::make(
        Note::undefined(), note_from_int64_storage(::std::move(offsets),
                                                   ::std::array<::std::int64_t, 1>{1}));
  }
  const auto& impl = NoteAccess::impl(x);
  PLUSS_CHECK(impl->is_text(), ::std::string(op) + " requires a UTF-8 Text Note");
  const ::std::int64_t n = impl->numel();
  NoteImpl::Int64Storage offsets(static_cast<::std::size_t>(n) + 1, 0);
  ::std::vector<::std::string> flat_tokens;

  const auto workers = coden_parallel_workers(n, /*min_tasks_per_worker=*/256);
  if (workers <= 1) {
    for (::std::int64_t i = 0; i < n; ++i) {
      ::std::vector<::std::string> row;
      fn(impl->bytes_at(i), row);
      flat_tokens.insert(flat_tokens.end(),
                         ::std::make_move_iterator(row.begin()),
                         ::std::make_move_iterator(row.end()));
      offsets[static_cast<::std::size_t>(i) + 1] = static_cast<::std::int64_t>(flat_tokens.size());
    }
  } else {
    ::std::vector<::std::vector<::std::string>> per_row(static_cast<::std::size_t>(n));
    const ::std::int64_t chunk = (n + workers - 1) / workers;
    ::std::vector<::std::thread> pool;
    pool.reserve(static_cast<::std::size_t>(workers - 1));

    for (::std::int64_t w = 1; w < workers; ++w) {
      const ::std::int64_t b = w * chunk;
      const ::std::int64_t e = ::std::min<::std::int64_t>(n, b + chunk);
      if (b >= e) break;
      pool.emplace_back([&, b, e]() {
        for (::std::int64_t i = b; i < e; ++i) {
          fn(impl->bytes_at(i), per_row[static_cast<::std::size_t>(i)]);
        }
      });
    }

    const ::std::int64_t b0 = 0;
    const ::std::int64_t e0 = ::std::min<::std::int64_t>(n, chunk);
    for (::std::int64_t i = b0; i < e0; ++i) {
      fn(impl->bytes_at(i), per_row[static_cast<::std::size_t>(i)]);
    }
    for (auto& t : pool) t.join();

    ::std::int64_t total = 0;
    for (::std::int64_t i = 0; i < n; ++i) {
      total += static_cast<::std::int64_t>(per_row[static_cast<::std::size_t>(i)].size());
      offsets[static_cast<::std::size_t>(i) + 1] = total;
    }
    flat_tokens.reserve(static_cast<::std::size_t>(total));
    for (::std::int64_t i = 0; i < n; ++i) {
      auto& row = per_row[static_cast<::std::size_t>(i)];
      flat_tokens.insert(flat_tokens.end(),
                         ::std::make_move_iterator(row.begin()),
                         ::std::make_move_iterator(row.end()));
    }
  }

  ::std::vector<::std::string_view> views;
  views.reserve(flat_tokens.size());
  for (const auto& tok : flat_tokens) views.push_back(tok);
  const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(flat_tokens.size())};
  Note token_note = note_from_utf8(views, ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  const ::std::array<::std::int64_t, 1> offset_shape{
      static_cast<::std::int64_t>(offsets.size())};
  return TokenBatchFactoryAccess::make(
      ::std::move(token_note),
      note_from_int64_storage(::std::move(offsets), offset_shape));
}

template <class TokenizePairFn>
static TokenBatch tokenbatch_from_note_pair(const Note& x,
                                            const Note& other,
                                            const char* op,
                                            TokenizePairFn&& fn) {
  if (!x.defined() || !other.defined()) {
    NoteImpl::Int64Storage offsets{0};
    return TokenBatchFactoryAccess::make(
        Note::undefined(), note_from_int64_storage(::std::move(offsets),
                                                   ::std::array<::std::int64_t, 1>{1}));
  }
  const auto& x_impl = NoteAccess::impl(x);
  const auto& y_impl = NoteAccess::impl(other);
  PLUSS_CHECK(x_impl->is_text(), ::std::string(op) + " requires a UTF-8 Text Note (lhs)");
  PLUSS_CHECK(y_impl->is_text(), ::std::string(op) + " requires a UTF-8 Text Note (rhs)");

  const ::std::int64_t nx = x_impl->numel();
  const ::std::int64_t ny = y_impl->numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(nx == ny || bx || by, ::std::string(op) + " expects equal numel or scalar broadcast");
  const ::std::int64_t n = (nx == ny) ? nx : (bx ? ny : nx);

  NoteImpl::Int64Storage offsets(static_cast<::std::size_t>(n) + 1, 0);
  ::std::vector<::std::string> flat_tokens;

  const auto workers = coden_parallel_workers(n, /*min_tasks_per_worker=*/256);
  if (workers <= 1) {
    for (::std::int64_t i = 0; i < n; ++i) {
      const ::std::string_view sx = x_impl->bytes_at(bx ? 0 : i);
      const ::std::string_view sy = y_impl->bytes_at(by ? 0 : i);
      fn(sx, sy, flat_tokens);
      offsets[static_cast<::std::size_t>(i) + 1] = static_cast<::std::int64_t>(flat_tokens.size());
    }
  } else {
    ::std::vector<::std::vector<::std::string>> per_row(static_cast<::std::size_t>(n));
    const ::std::int64_t chunk = (n + workers - 1) / workers;
    ::std::vector<::std::thread> pool;
    pool.reserve(static_cast<::std::size_t>(workers - 1));

    for (::std::int64_t w = 1; w < workers; ++w) {
      const ::std::int64_t b = w * chunk;
      const ::std::int64_t e = ::std::min<::std::int64_t>(n, b + chunk);
      if (b >= e) break;
      pool.emplace_back([&, b, e]() {
        for (::std::int64_t i = b; i < e; ++i) {
          const ::std::string_view sx = x_impl->bytes_at(bx ? 0 : i);
          const ::std::string_view sy = y_impl->bytes_at(by ? 0 : i);
          fn(sx, sy, per_row[static_cast<::std::size_t>(i)]);
        }
      });
    }

    const ::std::int64_t b0 = 0;
    const ::std::int64_t e0 = ::std::min<::std::int64_t>(n, chunk);
    for (::std::int64_t i = b0; i < e0; ++i) {
      const ::std::string_view sx = x_impl->bytes_at(bx ? 0 : i);
      const ::std::string_view sy = y_impl->bytes_at(by ? 0 : i);
      fn(sx, sy, per_row[static_cast<::std::size_t>(i)]);
    }
    for (auto& t : pool) t.join();

    ::std::int64_t total = 0;
    for (::std::int64_t i = 0; i < n; ++i) {
      total += static_cast<::std::int64_t>(per_row[static_cast<::std::size_t>(i)].size());
      offsets[static_cast<::std::size_t>(i) + 1] = total;
    }
    flat_tokens.reserve(static_cast<::std::size_t>(total));
    for (::std::int64_t i = 0; i < n; ++i) {
      auto& row = per_row[static_cast<::std::size_t>(i)];
      flat_tokens.insert(flat_tokens.end(),
                         ::std::make_move_iterator(row.begin()),
                         ::std::make_move_iterator(row.end()));
    }
  }

  ::std::vector<::std::string_view> views;
  views.reserve(flat_tokens.size());
  for (const auto& tok : flat_tokens) views.push_back(tok);
  const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(flat_tokens.size())};
  Note token_note = note_from_utf8(views, ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  const ::std::array<::std::int64_t, 1> offset_shape{
      static_cast<::std::int64_t>(offsets.size())};
  return TokenBatchFactoryAccess::make(
      ::std::move(token_note),
      note_from_int64_storage(::std::move(offsets), offset_shape));
}

template <class TransformPairFn>
static Note transform_note_pair(const Note& x,
                                const Note& other,
                                const char* op,
                                TransformPairFn&& fn) {
  if (!x.defined() || !other.defined()) return Note::undefined();
  const auto& x_impl = NoteAccess::impl(x);
  const auto& y_impl = NoteAccess::impl(other);
  PLUSS_CHECK(!x_impl->is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (lhs is bytes)");
  PLUSS_CHECK(!y_impl->is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (rhs is bytes)");

  const ::std::int64_t nx = x_impl->numel();
  const ::std::int64_t ny = y_impl->numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(nx == ny || bx || by, ::std::string(op) + " expects equal numel or scalar broadcast");
  const ::std::int64_t n = (nx == ny) ? nx : (bx ? ny : nx);

  const auto& out_impl = bx ? y_impl : x_impl;
  ::std::vector<::std::string> flat;
  flat.reserve(static_cast<::std::size_t>(n));
  for (::std::int64_t i = 0; i < n; ++i) {
    const ::std::string_view sx = x_impl->bytes_at(bx ? 0 : i);
    const ::std::string_view sy = y_impl->bytes_at(by ? 0 : i);
    flat.push_back(fn(sx, sy));
  }

  ::std::vector<::std::string_view> views;
  views.reserve(flat.size());
  for (const auto& z : flat) views.push_back(z);

  ::std::array<::std::int64_t, 3> sz = out_impl->sizes3();
  const ::std::int64_t dim = out_impl->dim();
  return note_from_utf8(
      views, ::std::span<const ::std::int64_t>(sz.data(), static_cast<::std::size_t>(dim)));
}

// Float64 counterpart of transform_note_pair: maps each cell pair to a raw
// double (e.g. a similarity score) and stores it as a native Float64 Note,
// with the same scalar-broadcast rule as the text version.
template <class TransformPairFn>
static Note transform_note_pair_to_float64(const Note& x,
                                           const Note& other,
                                           const char* op,
                                           TransformPairFn&& fn) {
  if (!x.defined() || !other.defined()) return Note::undefined();
  const auto& x_impl = NoteAccess::impl(x);
  const auto& y_impl = NoteAccess::impl(other);
  PLUSS_CHECK(!x_impl->is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (lhs is bytes)");
  PLUSS_CHECK(!y_impl->is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (rhs is bytes)");

  const ::std::int64_t nx = x_impl->numel();
  const ::std::int64_t ny = y_impl->numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(nx == ny || bx || by, ::std::string(op) + " expects equal numel or scalar broadcast");
  const ::std::int64_t n = (nx == ny) ? nx : (bx ? ny : nx);

  const auto& out_impl = bx ? y_impl : x_impl;
  NoteImpl::Float64Storage values(static_cast<::std::size_t>(n));
  for (::std::int64_t i = 0; i < n; ++i) {
    const ::std::string_view sx = x_impl->bytes_at(bx ? 0 : i);
    const ::std::string_view sy = y_impl->bytes_at(by ? 0 : i);
    values[static_cast<::std::size_t>(i)] = fn(sx, sy);
  }
  return note_from_float64_storage(::std::move(values), out_impl->sizes());
}

enum class TokenSetKind {
  Tokens,
  Terms,
  Acronyms,
  Citations,
  MathSymbols
};

static void extract_family_tokens(::std::string_view s, TokenSetKind kind, ::std::vector<::std::string>& toks) {
  if (kind == TokenSetKind::Acronyms) {
    if (!has_ascii_upper(s)) return;
    static const ::std::regex re(R"(\b[A-Z]{2,}\b)");
    regex_collect_to(s, re, toks);
    return;
  }
  if (kind == TokenSetKind::Citations) {
    if (!has_any_char_from(s, "[(") || !has_ascii_digit(s)) return;
    static const ::std::regex re(R"((\[[0-9]{1,3}\]|\([A-Z][A-Za-z]+,\s*[12][0-9]{3}[a-z]?\)))");
    regex_collect_to(s, re, toks);
    return;
  }
  if (kind == TokenSetKind::MathSymbols) {
    if (!has_any_char_from(s, "\\+-*/=<>^")) return;
    static const ::std::regex re(R"((\\[A-Za-z]+|[+\-*/=<>^]))");
    regex_collect_to(s, re, toks);
    return;
  }
}

static void collect_token_set(::std::string_view s,
                              TokenSetKind kind,
                              ::std::vector<::std::string>& order,
                              ::std::unordered_set<::std::string>& set) {
  ::std::vector<::std::string> toks;
  if (kind == TokenSetKind::Tokens) {
    word_tokens_lower(s, toks);
  } else if (kind == TokenSetKind::Terms) {
    word_tokens_lower_filtered(s, false, false, 4, -1, toks);
  } else {
    extract_family_tokens(s, kind, toks);
    if (kind == TokenSetKind::Acronyms || kind == TokenSetKind::Citations) {
      for (auto& t : toks) t = ::pluss::coden::native::cpu::lower_ascii(t);
    }
  }
  order.reserve(order.size() + toks.size());
  for (auto& t : toks) {
    if (kind == TokenSetKind::Terms && is_stopword(t)) continue;
    if (set.insert(t).second) order.push_back(::std::move(t));
  }
}

static void set_intersection_to(::std::string_view a_text,
                                ::std::string_view b_text,
                                TokenSetKind kind,
                                ::std::vector<::std::string>& out) {
  ::std::vector<::std::string> a_order;
  ::std::unordered_set<::std::string> a_set;
  collect_token_set(a_text, kind, a_order, a_set);

  ::std::vector<::std::string> b_order;
  ::std::unordered_set<::std::string> b_set;
  collect_token_set(b_text, kind, b_order, b_set);

  for (const auto& t : a_order) {
    if (b_set.find(t) != b_set.end()) out.push_back(t);
  }
}

static double set_jaccard_score(::std::string_view a_text, ::std::string_view b_text, TokenSetKind kind) {
  ::std::vector<::std::string> a_order;
  ::std::unordered_set<::std::string> a_set;
  collect_token_set(a_text, kind, a_order, a_set);

  ::std::vector<::std::string> b_order;
  ::std::unordered_set<::std::string> b_set;
  collect_token_set(b_text, kind, b_order, b_set);

  if (a_set.empty() && b_set.empty()) return 1.0;
  ::std::size_t inter = 0;
  for (const auto& t : a_set) if (b_set.find(t) != b_set.end()) ++inter;
  const ::std::size_t uni = a_set.size() + b_set.size() - inter;
  return (uni == 0) ? 0.0 : (static_cast<double>(inter) / static_cast<double>(uni));
}

static double set_overlap_score(::std::string_view a_text, ::std::string_view b_text, TokenSetKind kind) {
  ::std::vector<::std::string> a_order;
  ::std::unordered_set<::std::string> a_set;
  collect_token_set(a_text, kind, a_order, a_set);

  ::std::vector<::std::string> b_order;
  ::std::unordered_set<::std::string> b_set;
  collect_token_set(b_text, kind, b_order, b_set);

  if (a_set.empty() && b_set.empty()) return 1.0;
  const ::std::size_t denom = ::std::min(a_set.size(), b_set.size());
  if (denom == 0) return 0.0;
  ::std::size_t inter = 0;
  for (const auto& t : a_set) if (b_set.find(t) != b_set.end()) ++inter;
  return static_cast<double>(inter) / static_cast<double>(denom);
}

// Similarity/sentiment-style scores are contractually in [0, 1]; a heuristic
// computation can occasionally overshoot by floating-point error, so this
// clamp is a correctness guard, not a formatting nicety (unlike the decimal
// rounding these numeric-score methods used to apply when they stored their
// result as text -- now that they store a native Float64 Note, the value
// itself no longer needs to be quantized for display).
static double clamp01(double x) {
  if (x < 0.0) return 0.0;
  if (x > 1.0) return 1.0;
  return x;
}

static bool decode_utf8_cp(::std::string_view s,
                           ::std::size_t i,
                           ::std::uint32_t& cp,
                           ::std::size_t& cp_len) {
  if (i >= s.size()) return false;
  const unsigned char b0 = static_cast<unsigned char>(s[i]);
  if (b0 < 0x80) {
    cp = b0;
    cp_len = 1;
    return true;
  }
  if ((b0 & 0xE0u) == 0xC0u && i + 1 < s.size()) {
    const unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
    if ((b1 & 0xC0u) == 0x80u) {
      cp = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
      cp_len = 2;
      return true;
    }
  }
  if ((b0 & 0xF0u) == 0xE0u && i + 2 < s.size()) {
    const unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
    const unsigned char b2 = static_cast<unsigned char>(s[i + 2]);
    if ((b1 & 0xC0u) == 0x80u && (b2 & 0xC0u) == 0x80u) {
      cp = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
      cp_len = 3;
      return true;
    }
  }
  if ((b0 & 0xF8u) == 0xF0u && i + 3 < s.size()) {
    const unsigned char b1 = static_cast<unsigned char>(s[i + 1]);
    const unsigned char b2 = static_cast<unsigned char>(s[i + 2]);
    const unsigned char b3 = static_cast<unsigned char>(s[i + 3]);
    if ((b1 & 0xC0u) == 0x80u && (b2 & 0xC0u) == 0x80u && (b3 & 0xC0u) == 0x80u) {
      cp = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
      cp_len = 4;
      return true;
    }
  }
  cp = b0;
  cp_len = 1;
  return false;
}

static bool is_valid_utf8_text(::std::string_view s) {
  ::std::size_t i = 0;
  while (i < s.size()) {
    ::std::uint32_t cp = 0;
    ::std::size_t cp_len = 0;
    if (!decode_utf8_cp(s, i, cp, cp_len)) return false;
    i += cp_len;
  }
  return true;
}

static ::std::string maybe_lang(::std::string_view lang) {
  if (lang.empty()) return "en";
  return ::pluss::coden::native::cpu::lower_ascii(lang);
}

static ::std::string nlp_normalize_numbers_text(::std::string_view s) {
  return ::pluss::coden::native::cpu::nlp_normalize_numbers_fast(s);
}

static ::std::string nlp_expand_contractions_text(::std::string_view s) {
  static const ::std::array<::std::pair<::std::string_view, ::std::string_view>, 18> kMap{{
      {"can't", "can not"}, {"won't", "will not"}, {"n't", " not"},
      {"i'm", "i am"}, {"you're", "you are"}, {"we're", "we are"}, {"they're", "they are"},
      {"it's", "it is"}, {"that's", "that is"}, {"there's", "there is"},
      {"what's", "what is"}, {"who's", "who is"}, {"let's", "let us"},
      {"i've", "i have"}, {"we've", "we have"}, {"they've", "they have"},
      {"i'll", "i will"}, {"you'll", "you will"},
  }};
  const auto ascii_lower = [](unsigned char c) {
    return static_cast<char>(::std::tolower(c));
  };
  const auto expand_word = [&](::std::string_view word) -> ::std::string {
    ::std::string lower;
    lower.reserve(word.size());
    for (const unsigned char c : word) lower.push_back(ascii_lower(c));

    ::std::string replacement;
    for (const auto& [from, to] : kMap) {
      if (lower == from) {
        replacement.assign(to);
        break;
      }
    }
    if (replacement.empty() && lower.size() > 3 && lower.ends_with("n't")) {
      replacement.assign(lower, 0, lower.size() - 3);
      replacement.append(" not");
    }
    if (replacement.empty()) return ::std::string(word);

    const bool all_upper = ::std::any_of(word.begin(), word.end(), [](unsigned char c) {
      return ::std::isalpha(c) != 0;
    }) && ::std::none_of(word.begin(), word.end(), [](unsigned char c) {
      return ::std::islower(c) != 0;
    });
    if (all_upper) {
      for (char& c : replacement) c = static_cast<char>(::std::toupper(static_cast<unsigned char>(c)));
    } else if (::std::isupper(static_cast<unsigned char>(word.front()))) {
      replacement.front() = static_cast<char>(::std::toupper(static_cast<unsigned char>(replacement.front())));
    }
    return replacement;
  };

  ::std::string out;
  out.reserve(s.size());
  ::std::size_t i = 0;
  while (i < s.size()) {
    if (!::std::isalpha(static_cast<unsigned char>(s[i]))) {
      out.push_back(s[i++]);
      continue;
    }
    const ::std::size_t begin = i++;
    while (i < s.size() &&
           (::std::isalpha(static_cast<unsigned char>(s[i])) || s[i] == '\'')) {
      ++i;
    }
    out.append(expand_word(s.substr(begin, i - begin)));
  }
  return out;
}

static ::std::string nlp_remove_stopwords_text(::std::string_view s, ::std::string_view lang) {
  if (maybe_lang(lang) != "en") return ::std::string(s);
  ::std::vector<::std::string> words;
  tokenize_words_to(s, words);
  ::std::string out;
  bool first = true;
  for (const auto& w : words) {
    ::std::string lw = ::pluss::coden::native::cpu::lower_ascii(w);
    if (is_stopword(lw)) continue;
    if (!first) out.push_back(' ');
    first = false;
    out.append(w);
  }
  return out;
}

struct ScriptGuess {
  ::std::string lang = "unknown";
  ::std::string script = "Unknown";
  double confidence = 0.0;
};

static ScriptGuess detect_script_and_language(::std::string_view s) {
  ::std::int64_t latin = 0;
  ::std::int64_t cyrillic = 0;
  ::std::int64_t greek = 0;
  ::std::int64_t letters = 0;

  for (::std::size_t i = 0; i < s.size();) {
    ::std::uint32_t cp = 0;
    ::std::size_t cp_len = 0;
    decode_utf8_cp(s, i, cp, cp_len);
    i += cp_len;

    if (cp <= 0x7Fu) {
      if (::std::isalpha(static_cast<unsigned char>(cp)) != 0) {
        ++latin;
        ++letters;
      }
      continue;
    }
    if ((cp >= 0x00C0u && cp <= 0x02AFu) || (cp >= 0x1E00u && cp <= 0x1EFFu)) {
      ++latin;
      ++letters;
      continue;
    }
    if (cp >= 0x0370u && cp <= 0x03FFu) {
      ++greek;
      ++letters;
      continue;
    }
    if (cp >= 0x0400u && cp <= 0x052Fu) {
      ++cyrillic;
      ++letters;
      continue;
    }
  }

  ScriptGuess g;
  if (letters == 0) return g;

  if (cyrillic >= latin && cyrillic >= greek) {
    g.script = "Cyrillic";
    g.lang = "ru";
    g.confidence = static_cast<double>(cyrillic) / static_cast<double>(letters);
  } else if (greek >= latin && greek >= cyrillic) {
    g.script = "Greek";
    g.lang = "el";
    g.confidence = static_cast<double>(greek) / static_cast<double>(letters);
  } else {
    g.script = "Latin";
    g.lang = "en";
    g.confidence = static_cast<double>(latin) / static_cast<double>(letters);
  }
  return g;
}

static ::std::int64_t count_syllables(::std::string_view w) {
  if (w.empty()) return 0;
  ::std::string x = ::pluss::coden::native::cpu::lower_ascii(w);
  auto is_vowel = [](char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
  };
  ::std::int64_t n = 0;
  bool prev_v = false;
  for (char c : x) {
    const bool v = is_vowel(c);
    if (v && !prev_v) ++n;
    prev_v = v;
  }
  if (x.size() > 2 && x.back() == 'e' && n > 1) --n;
  return ::std::max<::std::int64_t>(1, n);
}

static double ratio_score(::std::int64_t a, ::std::int64_t b) {
  const ::std::int64_t d = ::std::max<::std::int64_t>(1, b);
  return static_cast<double>(a) / static_cast<double>(d);
}

static double lexical_score(::std::string_view s,
                            const ::std::unordered_set<::std::string_view>& pos,
                            const ::std::unordered_set<::std::string_view>& neg) {
  ::std::vector<::std::string> words;
  word_tokens_lower(s, words);
  if (words.empty()) return 0.5;
  ::std::int64_t p = 0;
  ::std::int64_t n = 0;
  for (const auto& w : words) {
    if (pos.find(w) != pos.end()) ++p;
    if (neg.find(w) != neg.end()) ++n;
  }
  const double centered = static_cast<double>(p - n) / static_cast<double>(::std::max<::std::int64_t>(1, p + n));
  return ::std::clamp(0.5 + 0.5 * centered, 0.0, 1.0);
}

static double sentiment_score_text(::std::string_view s) {
  static const ::std::unordered_set<::std::string_view> kPos{
      "good","great","excellent","happy","love","like","awesome","fast","clear","robust","improve","success"};
  static const ::std::unordered_set<::std::string_view> kNeg{
      "bad","poor","terrible","hate","slow","bug","broken","awful","failure","wrong","worse","error"};
  return lexical_score(s, kPos, kNeg);
}

static double toxicity_score_text(::std::string_view s) {
  static const ::std::unordered_set<::std::string_view> kToxic{
      "idiot","stupid","dumb","hate","trash","shut","kill","moron","loser"};
  ::std::vector<::std::string> words;
  word_tokens_lower(s, words);
  if (words.empty()) return 0.0;
  ::std::int64_t tox = 0;
  for (const auto& w : words) if (kToxic.find(w) != kToxic.end()) ++tox;
  return ::std::clamp(ratio_score(tox, static_cast<::std::int64_t>(words.size())) * 1.8, 0.0, 1.0);
}

static double spam_score_text(::std::string_view s) {
  static const ::std::unordered_set<::std::string_view> kSpam{
      "free","winner","win","buy","click","urgent","offer","limited","bonus","guarantee","cash","prize"};
  ::std::vector<::std::string> words;
  word_tokens_lower(s, words);
  ::std::int64_t hit = 0;
  for (const auto& w : words) if (kSpam.find(w) != kSpam.end()) ++hit;
  if (contains_ascii_nocase(s, "http")) hit += 2;
  if (contains_ascii_nocase(s, "unsubscribe")) hit += 1;
  if (has_ascii_digit(s)) hit += 1;
  return ::std::clamp(ratio_score(hit, static_cast<::std::int64_t>(::std::max<::std::size_t>(1, words.size() / 3 + 1))), 0.0, 1.0);
}

static double subjectivity_score_text(::std::string_view s) {
  static const ::std::unordered_set<::std::string_view> kSubj{
      "i","me","my","mine","feel","think","believe","opinion","probably","maybe","seems"};
  ::std::vector<::std::string> words;
  word_tokens_lower(s, words);
  if (words.empty()) return 0.5;
  ::std::int64_t hit = 0;
  for (const auto& w : words) if (kSubj.find(w) != kSubj.end()) ++hit;
  return ::std::clamp(0.2 + ratio_score(hit, static_cast<::std::int64_t>(words.size())) * 2.0, 0.0, 1.0);
}

static double formality_score_text(::std::string_view s) {
  static const ::std::unordered_set<::std::string_view> kInformal{
      "lol","omg","btw","idk","u","ur","wanna","gonna","ain't"};
  ::std::vector<::std::string> words;
  word_tokens_lower(s, words);
  if (words.empty()) return 0.5;
  ::std::int64_t informal = 0;
  ::std::int64_t long_words = 0;
  for (const auto& w : words) {
    if (kInformal.find(w) != kInformal.end()) ++informal;
    if (w.size() >= 7) ++long_words;
  }
  const double formal = ratio_score(long_words, static_cast<::std::int64_t>(words.size()));
  const double inf = ratio_score(informal, static_cast<::std::int64_t>(words.size()));
  return ::std::clamp(0.5 + (formal - inf), 0.0, 1.0);
}

static double readability_grade_raw(::std::string_view s) {
  ::std::vector<::std::string> words;
  word_tokens_lower_filtered(s, true, false, 1, -1, words);
  const ::std::int64_t w = static_cast<::std::int64_t>(words.size());
  if (w == 0) return 0.0;
  ::std::int64_t sentences = 0;
  for (char c : s) {
    if (c == '.' || c == '!' || c == '?') ++sentences;
  }
  if (sentences <= 0) sentences = 1;
  ::std::int64_t syll = 0;
  for (const auto& wd : words) syll += count_syllables(wd);
  const double wps = static_cast<double>(w) / static_cast<double>(sentences);
  const double spw = static_cast<double>(syll) / static_cast<double>(w);
  return 0.39 * wps + 11.8 * spw - 15.59;
}

static ::std::string readability_level_text(::std::string_view s) {
  const double g = readability_grade_raw(s);
  if (g <= 5.0) return "elementary";
  if (g <= 8.0) return "middle_school";
  if (g <= 12.0) return "high_school";
  return "college";
}

static ::std::string transliterate_ascii_text(::std::string_view s) {
  ::std::string out = ::pluss::coden::native::cpu::strip_accents_utf8(s);
  for (char& c : out) {
    if (static_cast<unsigned char>(c) >= 0x80u) c = '?';
  }
  return out;
}

static ::std::string transliterate_latin_text(::std::string_view s) {
  return transliterate_ascii_text(s);
}

// Script maps return: nullopt -> code point is not handled, keep the original
// bytes; "" -> drop the code point entirely; non-empty -> ASCII replacement.
// Combining diacritics (U+0300..U+036F) are dropped by every script map, so
// `to_nfd().transliterate_*()` widens coverage to any accented / polytonic form.
using ScriptMapResult = ::std::optional<::std::string>;

static bool is_combining_mark_cp(::std::uint32_t cp) {
  return cp >= 0x0300u && cp <= 0x036Fu;
}

static ScriptMapResult greek_map_cp(::std::uint32_t cp) {
  if (is_combining_mark_cp(cp)) return ::std::string{};
  switch (cp) {
    case 0x0391: case 0x03B1: case 0x0386: case 0x03AC: return "a";
    case 0x0392: case 0x03B2: return "b";
    case 0x0393: case 0x03B3: return "g";
    case 0x0394: case 0x03B4: return "d";
    case 0x0395: case 0x03B5: case 0x0388: case 0x03AD: return "e";
    case 0x0396: case 0x03B6: return "z";
    case 0x0397: case 0x03B7: case 0x0389: case 0x03AE: return "i";
    case 0x0398: case 0x03B8: return "th";
    case 0x0399: case 0x03B9: case 0x038A: case 0x03AF:
    case 0x03AA: case 0x03CA: case 0x0390: return "i";
    case 0x039A: case 0x03BA: return "k";
    case 0x039B: case 0x03BB: return "l";
    case 0x039C: case 0x03BC: return "m";
    case 0x039D: case 0x03BD: return "n";
    case 0x039E: case 0x03BE: return "x";
    case 0x039F: case 0x03BF: case 0x038C: case 0x03CC: return "o";
    case 0x03A0: case 0x03C0: return "p";
    case 0x03A1: case 0x03C1: return "r";
    case 0x03A3: case 0x03C3: case 0x03C2: return "s";
    case 0x03A4: case 0x03C4: return "t";
    case 0x03A5: case 0x03C5: case 0x038E: case 0x03CD:
    case 0x03AB: case 0x03CB: case 0x03B0: return "y";
    case 0x03A6: case 0x03C6: return "f";
    case 0x03A7: case 0x03C7: return "ch";
    case 0x03A8: case 0x03C8: return "ps";
    case 0x03A9: case 0x03C9: case 0x038F: case 0x03CE: return "o";
    default: return ::std::nullopt;
  }
}

static ScriptMapResult cyrillic_map_cp(::std::uint32_t cp) {
  if (is_combining_mark_cp(cp)) return ::std::string{};
  switch (cp) {
    case 0x0410: case 0x0430: return "a";
    case 0x0411: case 0x0431: return "b";
    case 0x0412: case 0x0432: return "v";
    case 0x0413: case 0x0433: return "g";
    case 0x0414: case 0x0434: return "d";
    case 0x0415: case 0x0435: return "e";
    case 0x0401: case 0x0451: return "yo";
    case 0x0416: case 0x0436: return "zh";
    case 0x0417: case 0x0437: return "z";
    case 0x0418: case 0x0438: return "i";
    case 0x0419: case 0x0439: return "y";
    case 0x041A: case 0x043A: return "k";
    case 0x041B: case 0x043B: return "l";
    case 0x041C: case 0x043C: return "m";
    case 0x041D: case 0x043D: return "n";
    case 0x041E: case 0x043E: return "o";
    case 0x041F: case 0x043F: return "p";
    case 0x0420: case 0x0440: return "r";
    case 0x0421: case 0x0441: return "s";
    case 0x0422: case 0x0442: return "t";
    case 0x0423: case 0x0443: return "u";
    case 0x0424: case 0x0444: return "f";
    case 0x0425: case 0x0445: return "kh";
    case 0x0426: case 0x0446: return "ts";
    case 0x0427: case 0x0447: return "ch";
    case 0x0428: case 0x0448: return "sh";
    case 0x0429: case 0x0449: return "shch";
    case 0x042A: case 0x044A: return "";   // hard sign -> dropped
    case 0x042C: case 0x044C: return "";   // soft sign -> dropped
    case 0x042B: case 0x044B: return "y";
    case 0x042D: case 0x044D: return "e";
    case 0x042E: case 0x044E: return "yu";
    case 0x042F: case 0x044F: return "ya";
    default: return ::std::nullopt;
  }
}

template <class MapFn>
static ::std::string transliterate_with_map(::std::string_view s, MapFn map_fn) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::uint32_t cp = 0;
    ::std::size_t cp_len = 0;
    decode_utf8_cp(s, i, cp, cp_len);
    const ScriptMapResult repl = map_fn(cp);
    if (repl.has_value()) out.append(*repl);          // may be "" -> drop
    else out.append(s.substr(i, cp_len));             // unmapped -> keep original
    i += cp_len;
  }
  return out;
}

static ::std::string soundex_word(::std::string_view w) {
  if (w.empty()) return {};
  auto up = [](unsigned char c) { return static_cast<char>(::std::toupper(c)); };
  auto code = [](char c) {
    switch (c) {
      case 'B': case 'F': case 'P': case 'V': return '1';
      case 'C': case 'G': case 'J': case 'K': case 'Q': case 'S': case 'X': case 'Z': return '2';
      case 'D': case 'T': return '3';
      case 'L': return '4';
      case 'M': case 'N': return '5';
      case 'R': return '6';
      default: return '0';
    }
  };
  ::std::string out(1, up(static_cast<unsigned char>(w.front())));
  char prev = code(out.front());
  for (::std::size_t i = 1; i < w.size() && out.size() < 4; ++i) {
    const char c = up(static_cast<unsigned char>(w[i]));
    if (c < 'A' || c > 'Z') continue;
    // 'H' and 'W' are transparent: they neither emit a digit nor break a run,
    // so two identically-coded consonants separated only by H/W collapse to
    // one digit (NARA / U.S. Census Soundex rule). Vowels and 'Y' are
    // separators and do reset the run.
    if (c == 'H' || c == 'W') continue;
    const char cur = code(c);
    if (cur != '0' && cur != prev) out.push_back(cur);
    prev = cur;
  }
  while (out.size() < 4) out.push_back('0');
  return out;
}

static ::std::string metaphone_simple_word(::std::string_view w) {
  if (w.empty()) return {};
  ::std::string x = ::pluss::coden::native::cpu::upper_ascii(w);
  ::std::string out;
  out.reserve(x.size());
  auto is_vowel = [](char c) { return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U'; };
  out.push_back(x.front());
  for (::std::size_t i = 1; i < x.size(); ++i) {
    const char c = x[i];
    if (c < 'A' || c > 'Z') continue;
    if (is_vowel(c)) continue;
    if (c == 'H' || c == 'W' || c == 'Y') continue;
    if (!out.empty() && out.back() == c) continue;
    out.push_back(c);
  }
  if (out.size() > 8) out.resize(8);
  return out;
}

static ::std::string phonetic_transform_text(::std::string_view s,
                                             const ::std::function<::std::string(::std::string_view)>& word_fn) {
  ::std::vector<::std::string> words;
  tokenize_words_to(s, words);
  ::std::string out;
  bool first = true;
  for (const auto& w : words) {
    const ::std::string code = word_fn(w);
    if (code.empty()) continue;
    if (!first) out.push_back(' ');
    first = false;
    out.append(code);
  }
  return out;
}

static ::std::string latin1_decode_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() * 2);
  for (unsigned char b : s) append_utf8_cp(static_cast<::std::uint32_t>(b), out);
  return out;
}

static ::std::string windows1252_decode_text(::std::string_view s) {
  static const ::std::array<::std::uint16_t, 32> kMap{{
      0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
      0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
      0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
      0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178,
  }};
  ::std::string out;
  out.reserve(s.size() * 2);
  for (unsigned char b : s) {
    if (b >= 0x80u && b <= 0x9Fu) {
      const auto cp = kMap[static_cast<::std::size_t>(b - 0x80u)];
      if (cp == 0) out.push_back('?');
      else append_utf8_cp(cp, out);
    } else {
      append_utf8_cp(static_cast<::std::uint32_t>(b), out);
    }
  }
  return out;
}

static ::std::string latin1_encode_lossy_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::uint32_t cp = 0;
    ::std::size_t cp_len = 0;
    decode_utf8_cp(s, i, cp, cp_len);
    i += cp_len;
    if (cp <= 0xFFu) out.push_back(static_cast<char>(cp));
    else out.push_back('?');
  }
  return out;
}

static ::std::string windows1252_encode_lossy_text(::std::string_view s) {
  static const ::std::unordered_map<::std::uint32_t, unsigned char> kReverse{
      {0x20AC, 0x80}, {0x201A, 0x82}, {0x0192, 0x83}, {0x201E, 0x84}, {0x2026, 0x85},
      {0x2020, 0x86}, {0x2021, 0x87}, {0x02C6, 0x88}, {0x2030, 0x89}, {0x0160, 0x8A},
      {0x2039, 0x8B}, {0x0152, 0x8C}, {0x017D, 0x8E}, {0x2018, 0x91}, {0x2019, 0x92},
      {0x201C, 0x93}, {0x201D, 0x94}, {0x2022, 0x95}, {0x2013, 0x96}, {0x2014, 0x97},
      {0x02DC, 0x98}, {0x2122, 0x99}, {0x0161, 0x9A}, {0x203A, 0x9B}, {0x0153, 0x9C},
      {0x017E, 0x9E}, {0x0178, 0x9F},
  };
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::uint32_t cp = 0;
    ::std::size_t cp_len = 0;
    decode_utf8_cp(s, i, cp, cp_len);
    i += cp_len;
    if (cp <= 0x7Fu || (cp >= 0xA0u && cp <= 0xFFu)) {
      out.push_back(static_cast<char>(cp));
      continue;
    }
    const auto it = kReverse.find(cp);
    out.push_back(it == kReverse.end() ? '?' : static_cast<char>(it->second));
  }
  return out;
}

static ::std::string tex_strip_comments_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] == '%') {
      ::std::size_t bs = 0;
      for (::std::size_t j = i; j > 0 && s[j - 1] == '\\'; --j) ++bs;
      if ((bs % 2) == 0) {
        while (i < s.size() && s[i] != '\n' && s[i] != '\r') ++i;
        continue;
      }
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

static ::std::string tex_strip_preamble_text(::std::string_view s) {
  auto document_tag_end = [s](::std::size_t pos,
                              ::std::string_view command,
                              ::std::size_t* end) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, pos, &name_begin, &name_end, &token_end) ||
        token_end != name_end ||
        s.substr(name_begin, name_end - name_begin) != command)
      return false;
    const auto group = tex_skip_space(s, token_end);
    ::std::size_t group_end = 0;
    if (group >= s.size() || s[group] != '{' ||
        !tex_group_end(s, group, '{', '}', &group_end) ||
        ::pluss::coden::native::cpu::strip_ascii(
            s.substr(group + 1, group_end - group - 2)) != "document")
      return false;
    *end = group_end;
    return true;
  };

  ::std::size_t body_begin = ::std::string_view::npos;
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t end = 0;
    if (document_tag_end(i, "begin", &end)) {
      body_begin = end;
      break;
    }
    i += s[i] == '\\' && i + 1 < s.size() ? 2 : 1;
  }
  if (body_begin == ::std::string_view::npos) return ::std::string(s);

  for (::std::size_t i = body_begin; i < s.size();) {
    ::std::size_t end = 0;
    if (document_tag_end(i, "end", &end)) {
      return ::pluss::coden::native::cpu::strip_ascii(
          s.substr(body_begin, i - body_begin));
    }
    i += s[i] == '\\' && i + 1 < s.size() ? 2 : 1;
  }
  return ::std::string(s);
}

static ::std::string tex_remove_commands_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    ::std::size_t cursor = token_end;
    for (;;) {
      const auto group = tex_skip_space(s, cursor);
      if (group >= s.size() || (s[group] != '[' && s[group] != '{')) break;
      ::std::size_t next = 0;
      if (!tex_group_end(s, group, s[group], s[group] == '[' ? ']' : '}', &next))
        break;
      cursor = next;
    }
    out.push_back(' ');
    i = cursor;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_to_text_text(::std::string_view s, ::std::int64_t mode) {
  ::std::string out = tex_strip_comments_text(s);
  out = tex_strip_preamble_text(out);
  out = ::pluss::coden::native::cpu::nlp_remove_math_fast(out);
  static const ::std::regex kTextCmd(
      R"(\\(?:text|textbf|textit|emph|underline|texttt|mathrm|mathbf|mathit|mathsf|operatorname)\s*\{([^{}]*)\})");
  out = ::std::regex_replace(out, kTextCmd, "$1");
  out = tex_remove_commands_text(out);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "{", " ");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "}", " ");
  out = ::pluss::coden::native::cpu::collapse_whitespace_ascii(out);
  out = ::pluss::coden::native::cpu::strip_ascii(out);
  // mode >= 1 also case-folds the recovered text for normalized keys.
  if (mode >= 1) out = ::pluss::coden::native::cpu::lower_ascii(out);
  return out;
}

static ::std::string tex_normalize_newlines_text(::std::string_view s) {
  ::std::string out = ::pluss::coden::native::cpu::replace_bytes(s, "\r\n", "\n");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\r", "\n");
  return out;
}

static ::std::string tex_dedup_blank_lines_text(::std::string_view s) {
  auto newline_size = [s](::std::size_t pos) -> ::std::size_t {
    if (pos >= s.size()) return 0;
    if (s[pos] == '\r') return pos + 1 < s.size() && s[pos + 1] == '\n' ? 2 : 1;
    return s[pos] == '\n' ? 1 : 0;
  };

  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    const auto first_size = newline_size(i);
    if (first_size == 0) {
      out.push_back(s[i++]);
      continue;
    }

    ::std::size_t cursor = i;
    ::std::size_t count = 0;
    ::std::size_t second_begin = i;
    ::std::size_t second_size = 0;
    while (const auto size = newline_size(cursor)) {
      const auto newline_begin = cursor;
      cursor += size;
      ++count;
      if (count == 2) {
        second_begin = newline_begin;
        second_size = size;
      }
      ::std::size_t next = cursor;
      while (next < s.size() && (s[next] == ' ' || s[next] == '\t')) ++next;
      if (newline_size(next) == 0) break;
      cursor = next;
    }
    if (count >= 3) {
      out.append(s.substr(i, first_size));
      out.append(s.substr(second_begin, second_size));
    } else {
      out.append(s.substr(i, cursor - i));
    }
    i = cursor;
  }
  return out;
}

static ::std::string tex_fix_linebreaks_text(::std::string_view s) {
  static const ::std::regex kTrail(R"([ \t]+\n)");
  ::std::string out = tex_normalize_newlines_text(s);
  out = ::std::regex_replace(out, kTrail, "\n");
  out = tex_dedup_blank_lines_text(out);
  return out;
}

static ::std::string tex_escape_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() * 2);
  for (const char c : s) {
    switch (c) {
      case '\\': out += "\\textbackslash{}"; break;
      case '{': out += "\\{"; break;
      case '}': out += "\\}"; break;
      case '#': out += "\\#"; break;
      case '$': out += "\\$"; break;
      case '%': out += "\\%"; break;
      case '&': out += "\\&"; break;
      case '_': out += "\\_"; break;
      case '^': out += "\\^{}"; break;
      case '~': out += "\\~{}"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

static ::std::string tex_unescape_text(::std::string_view s) {
  ::std::string out(s);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\textbackslash{}", "\\");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\{", "{");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\}", "}");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\#", "#");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\$", "$");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\%", "%");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\&", "&");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\_", "_");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\^{}", "^");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\~{}", "~");
  return out;
}

static ::std::string lower_ascii_copy(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (const unsigned char c : s) out.push_back(static_cast<char>(::std::tolower(c)));
  return out;
}

static ::std::string text_to_tex_word_commands(::std::string_view s) {
  static const ::std::unordered_map<::std::string, ::std::string> kWordMap{
      {"alpha", "\\alpha"},   {"beta", "\\beta"},     {"gamma", "\\gamma"},
      {"delta", "\\delta"},   {"theta", "\\theta"},   {"lambda", "\\lambda"},
      {"mu", "\\mu"},         {"pi", "\\pi"},         {"sigma", "\\sigma"},
      {"phi", "\\phi"},       {"omega", "\\omega"},   {"infty", "\\infty"},
      {"inf", "\\infty"},     {"sum", "\\sum"},       {"prod", "\\prod"},
      {"int", "\\int"},       {"sqrt", "\\sqrt"},
  };

  ::std::string out;
  out.reserve(s.size() + 8);
  for (::std::size_t i = 0; i < s.size();) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (::std::isalpha(c) == 0) {
      out.push_back(s[i]);
      ++i;
      continue;
    }
    ::std::size_t j = i;
    while (j < s.size() && ::std::isalpha(static_cast<unsigned char>(s[j])) != 0) ++j;
    const auto token = s.substr(i, j - i);
    const auto it = kWordMap.find(lower_ascii_copy(token));
    if (it != kWordMap.end()) out.append(it->second);
    else out.append(token);
    i = j;
  }
  return out;
}

static bool tex_should_wrap_math(::std::string_view s) {
  if (s.find('\\') != ::std::string_view::npos) return true;
  return s.find_first_of("=+-*/^_<>") != ::std::string_view::npos;
}

static ::std::string text_to_tex_text(::std::string_view s, ::std::int64_t mode) {
  ::std::string out = tex_escape_text(s);
  if (mode > 0) {
    out = text_to_tex_word_commands(out);
    out = ::pluss::coden::native::cpu::replace_bytes(out, "<=", "\\leq ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, ">=", "\\geq ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "!=", "\\neq ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "+/-", "\\pm ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "-/+", "\\mp ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "<->", "\\leftrightarrow ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "->", "\\to ");
    out = ::pluss::coden::native::cpu::replace_bytes(out, "<-", "\\leftarrow ");

    static const ::std::regex kSqrt(R"(\\sqrt\s*\(\s*([^)]+)\s*\))");
    out = ::std::regex_replace(out, kSqrt, "\\sqrt{$1}");

    out = ::pluss::coden::native::cpu::collapse_whitespace_ascii(out);
    out = ::pluss::coden::native::cpu::strip_ascii(out);
  }

  if (mode > 1 && !out.empty() && tex_should_wrap_math(out)) {
    out = "$" + out + "$";
  }
  return out;
}

static const char* tex_superscript_char(char c) {
  switch (c) {
    case '0': return "\xE2\x81\xB0";
    case '1': return "\xC2\xB9";
    case '2': return "\xC2\xB2";
    case '3': return "\xC2\xB3";
    case '4': return "\xE2\x81\xB4";
    case '5': return "\xE2\x81\xB5";
    case '6': return "\xE2\x81\xB6";
    case '7': return "\xE2\x81\xB7";
    case '8': return "\xE2\x81\xB8";
    case '9': return "\xE2\x81\xB9";
    case '+': return "\xE2\x81\xBA";
    case '-': return "\xE2\x81\xBB";
    case '=': return "\xE2\x81\xBC";
    case '(': return "\xE2\x81\xBD";
    case ')': return "\xE2\x81\xBE";
    case 'n': return "\xE2\x81\xBF";
    case 'i': return "\xE2\x81\xB1";
    default: return nullptr;
  }
}

static const char* tex_subscript_char(char c) {
  switch (c) {
    case '0': return "\xE2\x82\x80";
    case '1': return "\xE2\x82\x81";
    case '2': return "\xE2\x82\x82";
    case '3': return "\xE2\x82\x83";
    case '4': return "\xE2\x82\x84";
    case '5': return "\xE2\x82\x85";
    case '6': return "\xE2\x82\x86";
    case '7': return "\xE2\x82\x87";
    case '8': return "\xE2\x82\x88";
    case '9': return "\xE2\x82\x89";
    case '+': return "\xE2\x82\x8A";
    case '-': return "\xE2\x82\x8B";
    case '=': return "\xE2\x82\x8C";
    case '(': return "\xE2\x82\x8D";
    case ')': return "\xE2\x82\x8E";
    case 'a': return "\xE2\x82\x90";
    case 'e': return "\xE2\x82\x91";
    case 'h': return "\xE2\x82\x95";
    case 'i': return "\xE1\xB5\xA2";
    case 'j': return "\xE2\xB1\xBC";
    case 'k': return "\xE2\x82\x96";
    case 'l': return "\xE2\x82\x97";
    case 'm': return "\xE2\x82\x98";
    case 'n': return "\xE2\x82\x99";
    case 'o': return "\xE2\x82\x92";
    case 'p': return "\xE2\x82\x9A";
    case 'r': return "\xE1\xB5\xA3";
    case 's': return "\xE2\x82\x9B";
    case 't': return "\xE2\x82\x9C";
    case 'u': return "\xE1\xB5\xA4";
    case 'v': return "\xE1\xB5\xA5";
    case 'x': return "\xE2\x82\x93";
    default: return nullptr;
  }
}

static bool tex_parse_braced_group(::std::string_view s,
                                   ::std::size_t open_pos,
                                   ::std::size_t* next_pos,
                                   ::std::string* body) {
  if (open_pos >= s.size() || s[open_pos] != '{') return false;
  ::std::string out;
  ::std::int64_t depth = 0;
  for (::std::size_t i = open_pos; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '\\' && i + 1 < s.size()) {
      if (depth > 0) out.push_back(c);
      if (depth > 0) out.push_back(s[++i]);
      continue;
    }
    if (c == '{') {
      if (depth > 0) out.push_back(c);
      ++depth;
      continue;
    }
    if (c == '}') {
      --depth;
      if (depth < 0) return false;
      if (depth == 0) {
        *next_pos = i + 1;
        *body = ::std::move(out);
        return true;
      }
      out.push_back(c);
      continue;
    }
    if (depth > 0) out.push_back(c);
  }
  return false;
}

static ::std::string tex_render_script_token(::std::string_view token, bool superscript) {
  ::std::string out;
  out.reserve(token.size() * 2);
  for (const char c : token) {
    const char* mapped = superscript ? tex_superscript_char(c) : tex_subscript_char(c);
    if (mapped == nullptr) return {};
    out.append(mapped);
  }
  return out;
}

static ::std::string tex_render_apply_scripts(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() + 8);
  for (::std::size_t i = 0; i < s.size();) {
    const char c = s[i];
    if ((c == '^' || c == '_') && i + 1 < s.size()) {
      const bool superscript = (c == '^');
      const ::std::size_t marker_pos = i;
      ::std::size_t j = i + 1;
      bool had_braces = false;
      ::std::string token;
      if (s[j] == '{') {
        had_braces = true;
        if (!tex_parse_braced_group(s, j, &j, &token)) {
          out.push_back(c);
          ++i;
          continue;
        }
      } else {
        token.assign(1, s[j]);
        ++j;
      }

      const ::std::string rendered = tex_render_script_token(token, superscript);
      if (!rendered.empty()) {
        out.append(rendered);
      } else {
        out.push_back(s[marker_pos]);
        if (had_braces) out.push_back('{');
        out.append(token);
        if (had_braces) out.push_back('}');
      }
      i = j;
      continue;
    }
    out.push_back(c);
    ++i;
  }
  return out;
}

template <::std::size_t N>
static ::std::string tex_replace_named_commands(
    ::std::string_view text,
    const ::std::array<::std::pair<::std::string_view, ::std::string_view>, N>& map) {
  ::std::string out;
  out.reserve(text.size());
  for (::std::size_t i = 0; i < text.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(text, i, &name_begin, &name_end, &token_end) ||
        token_end != name_end) {
      const auto take = text[i] == '\\' && i + 1 < text.size() ? 2U : 1U;
      out.append(text.substr(i, take));
      i += take;
      continue;
    }
    const auto name = text.substr(name_begin, name_end - name_begin);
    const auto found = ::std::find_if(
        map.begin(), map.end(),
        [name](const auto& item) { return item.first == name; });
    if (found == map.end()) out.append(text.substr(i, token_end - i));
    else out.append(found->second);
    i = token_end;
  }
  return out;
}

static ::std::string tex_drop_control_words(::std::string_view text) {
  ::std::string out;
  out.reserve(text.size());
  for (::std::size_t i = 0; i < text.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (tex_control_word_at(text, i, &name_begin, &name_end, &token_end)) {
      out.push_back(' ');
      i = token_end;
      continue;
    }
    const auto take = text[i] == '\\' && i + 1 < text.size() ? 2U : 1U;
    out.append(text.substr(i, take));
    i += take;
  }
  return out;
}

static ::std::string tex_render_text(::std::string_view s, ::std::int64_t mode) {
  ::std::string out = tex_strip_comments_text(s);
  out = tex_strip_preamble_text(out);
  out = tex_expand_macros_text(out, mode > 0 ? mode : 1);
  out = tex_remove_left_right_text(out);
  out = tex_remove_spacing_cmds_text(out);

  static const ::std::regex kTextCmd(
      R"(\\(?:text|mathrm|mathbf|mathit|mathsf|operatorname|textbf|textit|emph)\s*\{([^{}]*)\})");
  static const ::std::regex kFrac(R"(\\(?:d|t)?frac\s*\{\s*([^{}]+?)\s*\}\s*\{\s*([^{}]+?)\s*\})");
  static const ::std::regex kSqrtN(R"(\\sqrt\s*\[\s*([^\]]+?)\s*\]\s*\{\s*([^{}]+?)\s*\})");
  static const ::std::regex kSqrt(R"(\\sqrt\s*\{\s*([^{}]+?)\s*\})");
  out = ::std::regex_replace(out, kTextCmd, "$1");
  out = ::std::regex_replace(out, kFrac, "($1)/($2)");
  out = ::std::regex_replace(out, kSqrtN, "root($2,$1)");
  out = ::std::regex_replace(out, kSqrt, "sqrt($1)");

  static constexpr ::std::array<::std::pair<::std::string_view, ::std::string_view>, 37> kMap{{
      {"alpha", "\xCE\xB1"}, {"beta", "\xCE\xB2"}, {"gamma", "\xCE\xB3"}, {"delta", "\xCE\xB4"},
      {"theta", "\xCE\xB8"}, {"lambda", "\xCE\xBB"}, {"mu", "\xCE\xBC"}, {"pi", "\xCF\x80"},
      {"sigma", "\xCF\x83"}, {"phi", "\xCF\x86"}, {"omega", "\xCF\x89"}, {"Gamma", "\xCE\x93"},
      {"Delta", "\xCE\x94"}, {"Theta", "\xCE\x98"}, {"Lambda", "\xCE\x9B"}, {"Pi", "\xCE\xA0"},
      {"Sigma", "\xCE\xA3"}, {"Phi", "\xCE\xA6"}, {"Omega", "\xCE\xA9"}, {"pm", "\xC2\xB1"},
      {"mp", "\xE2\x88\x93"}, {"times", "\xC3\x97"}, {"div", "\xC3\xB7"}, {"cdot", "\xC2\xB7"},
      {"leq", "\xE2\x89\xA4"}, {"geq", "\xE2\x89\xA5"}, {"neq", "\xE2\x89\xA0"}, {"approx", "\xE2\x89\x88"},
      {"to", "\xE2\x86\x92"}, {"leftarrow", "\xE2\x86\x90"}, {"leftrightarrow", "\xE2\x86\x94"},
      {"infty", "\xE2\x88\x9E"}, {"sum", "\xE2\x88\x91"}, {"prod", "\xE2\x88\x8F"},
      {"int", "\xE2\x88\xAB"}, {"in", "\xE2\x88\x88"}, {"notin", "\xE2\x88\x89"},
  }};
  out = tex_replace_named_commands(::std::move(out), kMap);
  out = tex_render_apply_scripts(out);
  out = tex_remove_math_delimiters_text(out);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\\\", "\n");
  out = tex_drop_control_words(out);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "{", "");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "}", "");
  out = tex_unescape_text(out);
  out = tex_fix_linebreaks_text(out);
  out = ::pluss::coden::native::cpu::collapse_whitespace_ascii(out);
  return ::pluss::coden::native::cpu::strip_ascii(out);
}

static ::std::string tex_remove_math_delimiters_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      if (s[i + 1] == '\\' || s[i + 1] == '$') {
        out.append(s.substr(i, 2));
        i += 2;
        continue;
      }
      if (s[i + 1] == '(' || s[i + 1] == ')' ||
          s[i + 1] == '[' || s[i + 1] == ']') {
        i += 2;
        continue;
      }
      out.push_back(s[i++]);
      continue;
    }
    if (s[i] == '$') {
      i += i + 1 < s.size() && s[i + 1] == '$' ? 2 : 1;
      continue;
    }
    out.push_back(s[i++]);
  }
  return out;
}

static ::std::string tex_add_math_delimiters_text(::std::string_view s) {
  const ::std::string t = ::pluss::coden::native::cpu::strip_ascii(s);
  if (t.empty()) return t;
  if (starts_with_sv(t, "$$") || ends_with_sv(t, "$$"))
    return (starts_with_sv(t, "$$") ? "" : "$$") + t +
           (ends_with_sv(t, "$$") ? "" : "$$");
  if (starts_with_sv(t, "\\(") || ends_with_sv(t, "\\)"))
    return (starts_with_sv(t, "\\(") ? "" : "\\(") + t +
           (ends_with_sv(t, "\\)") ? "" : "\\)");
  if (starts_with_sv(t, "\\[") || ends_with_sv(t, "\\]"))
    return (starts_with_sv(t, "\\[") ? "" : "\\[") + t +
           (ends_with_sv(t, "\\]") ? "" : "\\]");
  if (starts_with_sv(t, "$") || ends_with_sv(t, "$"))
    return (starts_with_sv(t, "$") ? "" : "$") + t +
           (ends_with_sv(t, "$") ? "" : "$");
  return "$" + t + "$";
}

static bool tex_name_in(::std::string_view name,
                        ::std::initializer_list<::std::string_view> names) {
  return ::std::find(names.begin(), names.end(), name) != names.end();
}

static ::std::string tex_remove_formatting_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (!tex_name_in(name, {"textbf", "textit", "emph", "underline",
                            "texttt", "mathrm", "mathbf", "mathit",
                            "mathsf"})) {
      out.push_back(s[i++]);
      continue;
    }
    const auto body = tex_skip_space(s, token_end);
    ::std::size_t next = 0;
    if (body >= s.size() || s[body] != '{' ||
        !tex_group_end(s, body, '{', '}', &next)) {
      out.push_back(s[i++]);
      continue;
    }
    out += tex_remove_formatting_text(
        s.substr(body + 1, next - body - 2));
    i = next;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_font_commands_text(::std::string_view s) {
  const auto unwrapped = tex_remove_formatting_text(s);
  ::std::string out;
  out.reserve(unwrapped.size());
  for (::std::size_t i = 0; i < unwrapped.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (tex_control_word_at(unwrapped, i, &name_begin, &name_end, &token_end) &&
        tex_name_in(unwrapped.substr(name_begin, name_end - name_begin),
                    {"rmfamily", "sffamily", "ttfamily", "bfseries",
                     "itshape", "scshape"})) {
      out.push_back(' ');
      i = token_end;
      continue;
    }
    const auto take = unwrapped[i] == '\\' && i + 1 < unwrapped.size() ? 2U : 1U;
    out.append(unwrapped.substr(i, take));
    i += take;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_canon_macro_name(::std::string_view name) {
  ::std::string n = ::pluss::coden::native::cpu::strip_ascii(name);
  while (!n.empty() && n.front() == '\\') n.erase(n.begin());
  return n;
}

static ::std::string tex_strip_macro_decls_text(::std::string_view s, ::std::string_view macro_name = {}) {
  const ::std::string wanted = tex_canon_macro_name(macro_name);
  const auto skip_space = [s](::std::size_t p) {
    while (p < s.size() &&
           ::std::isspace(static_cast<unsigned char>(s[p])) != 0)
      ++p;
    return p;
  };
  const auto skip_brackets = [s, &skip_space](::std::size_t p) {
    p = skip_space(p);
    while (p < s.size() && s[p] == '[') {
      ::std::int64_t depth = 1;
      ++p;
      while (p < s.size() && depth > 0) {
        if (s[p] == '\\' && p + 1 < s.size()) {
          p += 2;
          continue;
        }
        if (s[p] == '[') ++depth;
        if (s[p] == ']') --depth;
        ++p;
      }
      if (depth != 0) return ::std::string_view::npos;
      p = skip_space(p);
    }
    return p;
  };
  const auto control_word_end = [s](::std::size_t p) {
    while (p < s.size()) {
      const unsigned char c = static_cast<unsigned char>(s[p]);
      if (::std::isalpha(c) == 0 && s[p] != '@') break;
      ++p;
    }
    return p;
  };

  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] != '\\' || i + 1 >= s.size()) {
      out.push_back(s[i++]);
      continue;
    }
    if (s[i + 1] == '\\') {
      out.append(s.substr(i, 2));
      i += 2;
      continue;
    }

    const ::std::size_t command_end = control_word_end(i + 1);
    const auto command = s.substr(i + 1, command_end - i - 1);
    ::std::size_t declaration_end = ::std::string_view::npos;
    ::std::string declared_name;

    if (command == "newcommand" || command == "renewcommand" ||
        command == "providecommand") {
      ::std::size_t p = command_end;
      if (p < s.size() && s[p] == '*') ++p;
      p = skip_space(p);
      ::std::size_t after_name = 0;
      ::std::string name_group;
      if (tex_parse_braced_group(s, p, &after_name, &name_group)) {
        declared_name = tex_canon_macro_name(name_group);
        p = skip_brackets(after_name);
        ::std::string ignored_body;
        if (p != ::std::string_view::npos &&
            tex_parse_braced_group(s, p, &declaration_end, &ignored_body)) {
          // declaration_end is now the first byte after the nested body.
        }
      }
    } else if (command == "def") {
      ::std::size_t p = skip_space(command_end);
      if (p < s.size() && s[p] == '\\' && p + 1 < s.size()) {
        const ::std::size_t name_begin = p + 1;
        ::std::size_t name_end = control_word_end(name_begin);
        if (name_end == name_begin) ++name_end;
        declared_name.assign(s.substr(name_begin, name_end - name_begin));
        p = name_end;
        while (p < s.size() && s[p] != '{') {
          if (s[p] == '\\' && p + 1 < s.size()) ++p;
          ++p;
        }
        ::std::string ignored_body;
        (void)tex_parse_braced_group(
            s, p, &declaration_end, &ignored_body);
      }
    }

    const bool remove = declaration_end != ::std::string_view::npos &&
                        (wanted.empty() || declared_name == wanted);
    if (remove) {
      out.push_back(' ');
      i = declaration_end;
      continue;
    }
    out.push_back(s[i++]);
  }
  return out;
}

static ::std::unordered_map<::std::string, ::std::string> tex_collect_simple_macros(::std::string_view s) {
  ::std::unordered_map<::std::string, ::std::string> map;
  const ::std::string text(s);
  static const ::std::regex kNew(
      R"(\\(?:re)?newcommand\s*\{\s*\\([A-Za-z@]+)\s*\}(?:\s*\[[0-9]+\])?\s*\{([^{}]*)\})");
  static const ::std::regex kDef(R"(\\def\s*\\([A-Za-z@]+)\s*\{([^{}]*)\})");

  for (::std::sregex_iterator it(text.begin(), text.end(), kNew), end; it != end; ++it) {
    map[(*it)[1].str()] = (*it)[2].str();
  }
  for (::std::sregex_iterator it(text.begin(), text.end(), kDef), end; it != end; ++it) {
    map[(*it)[1].str()] = (*it)[2].str();
  }
  return map;
}

static ::std::string tex_apply_macro_map(::std::string text,
                                         const ::std::unordered_map<::std::string, ::std::string>& map,
                                         ::std::int64_t max_passes) {
  if (map.empty() || max_passes <= 0) return text;
  for (::std::int64_t pass = 0; pass < max_passes; ++pass) {
    bool changed = false;
    ::std::string next;
    next.reserve(text.size());
    for (::std::size_t i = 0; i < text.size();) {
      if (text[i] != '\\' || i + 1 >= text.size()) {
        next.push_back(text[i++]);
        continue;
      }
      if (text[i + 1] == '\\') {
        next.append(text, i, 2);
        i += 2;
        continue;
      }
      const unsigned char first = static_cast<unsigned char>(text[i + 1]);
      if (::std::isalpha(first) == 0 && text[i + 1] != '@') {
        next.push_back(text[i++]);
        continue;
      }
      ::std::size_t end = i + 2;
      while (end < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[end]);
        if (::std::isalpha(c) == 0 && text[end] != '@') break;
        ++end;
      }
      const auto found = map.find(text.substr(i + 1, end - i - 1));
      if (found == map.end()) next.append(text, i, end - i);
      else {
        next.append(found->second);
        changed = true;
      }
      i = end;
    }
    text = ::std::move(next);
    if (!changed) break;
  }
  return text;
}

static ::std::string tex_balance_braces_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() + 8);
  ::std::int64_t depth = 0;
  for (::std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '\\' && i + 1 < s.size()) {
      out.push_back(c);
      out.push_back(s[++i]);
      continue;
    }
    if (c == '{') {
      ++depth;
      out.push_back(c);
      continue;
    }
    if (c == '}') {
      if (depth > 0) {
        --depth;
        out.push_back(c);
      }
      continue;
    }
    out.push_back(c);
  }
  for (::std::int64_t i = 0; i < depth; ++i) out.push_back('}');
  return out;
}

static ::std::size_t count_substr(::std::string_view s, ::std::string_view needle) {
  if (needle.empty()) return 0;
  ::std::size_t n = 0;
  for (::std::size_t p = 0;;) {
    const auto f = s.find(needle, p);
    if (f == ::std::string_view::npos) break;
    ++n;
    p = f + needle.size();
  }
  return n;
}

static ::std::string tex_balance_math_delims_text(::std::string_view s) {
  ::std::string out(s);
  ::std::size_t dollars = 0;
  for (::std::size_t i = 0; i < out.size(); ++i) {
    if (out[i] != '$') continue;
    ::std::size_t bs = 0;
    for (::std::size_t j = i; j > 0 && out[j - 1] == '\\'; --j) ++bs;
    if ((bs % 2) == 0) ++dollars;
  }
  if ((dollars % 2u) != 0u) out.push_back('$');

  const auto par_open = count_substr(out, "\\(");
  const auto par_close = count_substr(out, "\\)");
  if (par_open > par_close) {
    for (::std::size_t i = 0; i < par_open - par_close; ++i) out += "\\)";
  }
  const auto br_open = count_substr(out, "\\[");
  const auto br_close = count_substr(out, "\\]");
  if (br_open > br_close) {
    for (::std::size_t i = 0; i < br_open - br_close; ++i) out += "\\]";
  }
  if (par_close > par_open || br_close > br_open) {
    ::std::string prefix;
    prefix.reserve(2 * ((par_close > par_open ? par_close - par_open : 0) +
                        (br_close > br_open ? br_close - br_open : 0)));
    // Preserve the former ordering while moving the existing bytes only once.
    for (::std::size_t i = br_open; i < br_close; ++i) prefix += "\\[";
    for (::std::size_t i = par_open; i < par_close; ++i) prefix += "\\(";
    out.insert(0, prefix);
  }
  return out;
}

static ::std::string tex_canonicalize_macros_text(::std::string_view s) {
  static constexpr ::std::array<::std::pair<::std::string_view, ::std::string_view>, 12> kMap{{
      {"dfrac", "\\frac"}, {"tfrac", "\\frac"}, {"cfrac", "\\frac"},
      {"operatorname", "\\mathrm"}, {"rm", "\\mathrm"}, {"bf", "\\mathbf"},
      {"it", "\\mathit"}, {"cal", "\\mathcal"}, {"Bbb", "\\mathbb"},
      {"lt", "<"}, {"gt", ">"}, {"leqslant", "\\leq"},
  }};

  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '\\') {
      out.append(s.substr(i, 2));
      i += 2;
      continue;
    }
    if (s[i] != '\\' || i + 1 >= s.size() ||
        (::std::isalpha(static_cast<unsigned char>(s[i + 1])) == 0 &&
         s[i + 1] != '@')) {
      out.push_back(s[i++]);
      continue;
    }
    ::std::size_t end = i + 2;
    while (end < s.size()) {
      const unsigned char c = static_cast<unsigned char>(s[end]);
      if (::std::isalpha(c) == 0 && s[end] != '@') break;
      ++end;
    }
    const auto name = s.substr(i + 1, end - i - 1);
    const auto replacement = ::std::find_if(
        kMap.begin(), kMap.end(), [name](const auto& entry) {
          return entry.first == name;
        });
    if (replacement == kMap.end()) out.append(s.substr(i, end - i));
    else out.append(replacement->second);
    i = end;
  }
  return out;
}

static ::std::string tex_expand_macros_text(::std::string_view s, ::std::int64_t max_passes) {
  ::std::string out(s);
  const auto macros = tex_collect_simple_macros(out);
  out = tex_strip_macro_decls_text(out);
  out = tex_apply_macro_map(::std::move(out), macros, max_passes);
  return normalize_tex_removed(out);
}

static ::std::string tex_inline_macros_text(::std::string_view s) {
  return tex_expand_macros_text(s, 1);
}

static ::std::string tex_collapse_macros_text(::std::string_view s) {
  ::std::string out = tex_strip_macro_decls_text(s);
  out = tex_canonicalize_macros_text(out);
  out = ::pluss::coden::native::cpu::collapse_whitespace_ascii(out);
  return ::pluss::coden::native::cpu::strip_ascii(out);
}

static ::std::string tex_set_macro_text(::std::string_view s, ::std::string_view name, ::std::string_view body) {
  const ::std::string n = tex_canon_macro_name(name);
  if (n.empty()) return ::std::string(s);
  ::std::string out = tex_strip_macro_decls_text(s, n);
  out = ::pluss::coden::native::cpu::lstrip_ascii(out);
  return "\\newcommand{\\" + n + "}{" + ::std::string(body) + "}\n" + out;
}

static ::std::string tex_remove_macro_text(::std::string_view s, ::std::string_view name) {
  const ::std::string n = tex_canon_macro_name(name);
  if (n.empty()) return ::std::string(s);
  return normalize_tex_removed(tex_strip_macro_decls_text(s, n));
}

static ::std::string tex_wrap_inline_math_text(::std::string_view s, ::std::string_view delim) {
  const ::std::string t = ::pluss::coden::native::cpu::strip_ascii(s);
  if (t.empty()) return t;
  if (t.find('$') != ::std::string::npos || t.find("\\(") != ::std::string::npos ||
      t.find("\\[") != ::std::string::npos) {
    return t;
  }
  const ::std::string d = delim.empty() ? "$" : ::std::string(delim);
  return d + t + d;
}

static ::std::string tex_wrap_display_math_text(::std::string_view s, ::std::string_view delim) {
  const ::std::string t = ::pluss::coden::native::cpu::strip_ascii(s);
  if (t.empty()) return t;
  ::std::string open = "\\[";
  ::std::string close = "\\]";
  if (!delim.empty()) {
    if (delim == "$$") {
      open = "$$";
      close = "$$";
    } else {
      open = ::std::string(delim);
      close = ::std::string(delim);
    }
  }
  return open + "\n" + t + "\n" + close;
}

static ::std::string tex_simplify_fractions_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        token_end != name_end ||
        !tex_name_in(s.substr(name_begin, name_end - name_begin),
                     {"frac", "dfrac", "tfrac", "cfrac"})) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    const auto num_open = tex_skip_space(s, token_end);
    ::std::size_t num_end = 0;
    if (num_open >= s.size() || s[num_open] != '{' ||
        !tex_group_end(s, num_open, '{', '}', &num_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    const auto den_open = tex_skip_space(s, num_end);
    ::std::size_t den_end = 0;
    if (den_open >= s.size() || s[den_open] != '{' ||
        !tex_group_end(s, den_open, '{', '}', &den_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }

    const auto numerator = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(num_open + 1, num_end - num_open - 2));
    const auto denominator = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(den_open + 1, den_end - den_open - 2));
    out += "\\frac{";
    out += tex_simplify_fractions_text(numerator);
    out += "}{";
    out += tex_simplify_fractions_text(denominator);
    out.push_back('}');
    i = den_end;
  }
  return out;
}

static ::std::string tex_simplify_roots_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        token_end != name_end ||
        s.substr(name_begin, name_end - name_begin) != "sqrt") {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    auto cursor = tex_skip_space(s, token_end);
    ::std::string degree;
    if (cursor < s.size() && s[cursor] == '[') {
      ::std::size_t degree_end = 0;
      if (!tex_group_end(s, cursor, '[', ']', &degree_end)) {
        out.append(s.substr(i, token_end - i));
        i = token_end;
        continue;
      }
      degree = ::pluss::coden::native::cpu::strip_ascii(
          s.substr(cursor + 1, degree_end - cursor - 2));
      cursor = tex_skip_space(s, degree_end);
    }

    ::std::size_t radicand_end = 0;
    if (cursor >= s.size() || s[cursor] != '{' ||
        !tex_group_end(s, cursor, '{', '}', &radicand_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    const auto radicand = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(cursor + 1, radicand_end - cursor - 2));
    out += "\\sqrt";
    if (!degree.empty()) {
      out.push_back('[');
      out += tex_simplify_roots_text(degree);
      out.push_back(']');
    }
    out.push_back('{');
    out += tex_simplify_roots_text(radicand);
    out.push_back('}');
    i = radicand_end;
  }
  return out;
}

static ::std::string tex_convert_frac_to_slash_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '\\') {
        out.append("\\\\");
        i += 2;
        continue;
      }
      out.push_back(s[i++]);
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (token_end != name_end ||
        (name != "frac" && name != "dfrac" && name != "tfrac" &&
         name != "cfrac")) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }

    ::std::size_t num_open = token_end;
    while (num_open < s.size() &&
           ::std::isspace(static_cast<unsigned char>(s[num_open]))) {
      ++num_open;
    }
    ::std::size_t num_end = 0;
    if (!tex_group_end(s, num_open, '{', '}', &num_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    ::std::size_t den_open = num_end;
    while (den_open < s.size() &&
           ::std::isspace(static_cast<unsigned char>(s[den_open]))) {
      ++den_open;
    }
    ::std::size_t den_end = 0;
    if (!tex_group_end(s, den_open, '{', '}', &den_end)) {
      out.append(s.substr(i, num_end - i));
      i = num_end;
      continue;
    }
    const auto numerator = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(num_open + 1, num_end - num_open - 2));
    const auto denominator = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(den_open + 1, den_end - den_open - 2));
    out += '(';
    out += tex_convert_frac_to_slash_text(numerator);
    out += ")/(";
    out += tex_convert_frac_to_slash_text(denominator);
    out += ')';
    i = den_end;
  }
  return out;
}

static ::std::string tex_convert_sqrt_to_pow_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        token_end != name_end ||
        s.substr(name_begin, name_end - name_begin) != "sqrt") {
      if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '\\') {
        out.append("\\\\");
        i += 2;
        continue;
      }
      out.push_back(s[i++]);
      continue;
    }

    ::std::size_t cursor = token_end;
    while (cursor < s.size() &&
           ::std::isspace(static_cast<unsigned char>(s[cursor]))) {
      ++cursor;
    }
    ::std::string degree = "2";
    if (cursor < s.size() && s[cursor] == '[') {
      ::std::size_t degree_end = 0;
      if (!tex_group_end(s, cursor, '[', ']', &degree_end)) {
        out.append(s.substr(i, token_end - i));
        i = token_end;
        continue;
      }
      degree = ::pluss::coden::native::cpu::strip_ascii(
          s.substr(cursor + 1, degree_end - cursor - 2));
      cursor = degree_end;
      while (cursor < s.size() &&
             ::std::isspace(static_cast<unsigned char>(s[cursor]))) {
        ++cursor;
      }
    }
    ::std::size_t radicand_end = 0;
    if (!tex_group_end(s, cursor, '{', '}', &radicand_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    const auto radicand = ::pluss::coden::native::cpu::strip_ascii(
        s.substr(cursor + 1, radicand_end - cursor - 2));
    out += '(';
    out += tex_convert_sqrt_to_pow_text(radicand);
    out += ")^(1/";
    out += tex_convert_sqrt_to_pow_text(degree);
    out += ')';
    i = radicand_end;
  }
  return out;
}

static ::std::string tex_convert_pm_to_plusminus_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0;
    ::std::size_t name_end = 0;
    ::std::size_t token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '\\') {
        out.append("\\\\");
        i += 2;
        continue;
      }
      out.push_back(s[i++]);
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (token_end == name_end && name == "pm") out += "+/-";
    else if (token_end == name_end && name == "mp") out += "-/+";
    else out.append(s.substr(i, token_end - i));
    i = token_end;
  }
  return out;
}

static ::std::string tex_standardize_vectors_text(::std::string_view s) {
  static const ::std::regex kV1(R"(\\(?:mathbf|boldsymbol|overrightarrow)\s*\{([^{}]+)\})");
  static const ::std::regex kV2(R"(\\vec\s+([A-Za-z]))");
  ::std::string out = ::std::regex_replace(::std::string(s), kV1, "\\vec{$1}");
  out = ::std::regex_replace(out, kV2, "\\vec{$1}");
  return out;
}

static ::std::string tex_standardize_matrices_text(::std::string_view s) {
  static const ::std::regex kBegin(R"(\\begin\{(?:bmatrix|pmatrix|Bmatrix|vmatrix|Vmatrix|smallmatrix)\})");
  static const ::std::regex kEnd(R"(\\end\{(?:bmatrix|pmatrix|Bmatrix|vmatrix|Vmatrix|smallmatrix)\})");
  ::std::string out = ::std::regex_replace(::std::string(s), kBegin, "\\begin{matrix}");
  out = ::std::regex_replace(out, kEnd, "\\end{matrix}");
  return out;
}

static ::std::string tex_standardize_integrals_text(::std::string_view s) {
  static const ::std::regex kInt(R"(\\(?:iint|iiint|iiiint|oint|oiint|oiiint)\b)");
  return ::std::regex_replace(::std::string(s), kInt, "\\int");
}

static ::std::string tex_standardize_summations_text(::std::string_view s) {
  ::std::string out(s);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\sumlimits", "\\sum");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\sumnolimits", "\\sum");
  return out;
}

static ::std::string tex_standardize_limits_text(::std::string_view s) {
  ::std::string out(s);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\limits", "");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\nolimits", "");
  return normalize_tex_removed(out);
}

static ::std::string tex_standardize_subscripts_text(::std::string_view s) {
  static const ::std::regex kSub(R"(_(?!\{)(\\?[A-Za-z0-9]+))");
  return ::std::regex_replace(::std::string(s), kSub, "_{$1}");
}

static ::std::string tex_standardize_superscripts_text(::std::string_view s) {
  static const ::std::regex kSup(R"(\^(?!\{)(\\?[A-Za-z0-9]+))");
  return ::std::regex_replace(::std::string(s), kSup, "^{$1}");
}

static ::std::string tex_standardize_primes_text(::std::string_view s) {
  ::std::string out(s);
  static const ::std::regex k3(R"(([A-Za-z0-9])''')");
  static const ::std::regex k2(R"(([A-Za-z0-9])'')");
  static const ::std::regex k1(R"(([A-Za-z0-9])')");
  out = ::std::regex_replace(out, k3, "$1^{\\prime\\prime\\prime}");
  out = ::std::regex_replace(out, k2, "$1^{\\prime\\prime}");
  out = ::std::regex_replace(out, k1, "$1^{\\prime}");
  return out;
}

static ::std::string tex_standardize_dots_text(::std::string_view s) {
  ::std::string out(s);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\ldots", "\\dots");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\cdots", "\\dots");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\dotsc", "\\dots");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "\\dotsb", "\\dots");
  return out;
}

static ::std::string tex_expand_inputs_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] != '\\' || i + 1 >= s.size()) {
      out.push_back(s[i++]);
      continue;
    }
    if (s[i + 1] == '\\') {
      out.append(s.substr(i, 2));
      i += 2;
      continue;
    }
    ::std::size_t end = i + 1;
    while (end < s.size() &&
           ::std::isalpha(static_cast<unsigned char>(s[end])) != 0)
      ++end;
    const auto command = s.substr(i + 1, end - i - 1);
    if (command != "input" && command != "include") {
      out.push_back(s[i++]);
      continue;
    }
    ::std::size_t group_begin = end;
    while (group_begin < s.size() &&
           ::std::isspace(static_cast<unsigned char>(s[group_begin])) != 0)
      ++group_begin;
    ::std::size_t after_group = 0;
    ::std::string path;
    if (!tex_parse_braced_group(s, group_begin, &after_group, &path)) {
      out.append(s.substr(i, end - i));
      i = end;
      continue;
    }
    out += "[include:";
    out += path;
    out.push_back(']');
    i = after_group;
  }
  return out;
}

static ::std::string tex_resolve_graphics_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        s.substr(name_begin, name_end - name_begin) != "includegraphics") {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    auto cursor = tex_skip_space(s, token_end);
    if (cursor < s.size() && s[cursor] == '[') {
      ::std::size_t next = 0;
      if (!tex_group_end(s, cursor, '[', ']', &next)) {
        out.append(s.substr(i, token_end - i));
        i = token_end;
        continue;
      }
      cursor = tex_skip_space(s, next);
    }
    ::std::size_t next = 0;
    ::std::string path;
    if (!tex_parse_braced_group(s, cursor, &next, &path)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    out += "[graphic:";
    out += path;
    out.push_back(']');
    i = next;
  }
  return out;
}

static ::std::string tex_remove_accents_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] != '\\' || i + 1 >= s.size()) {
      out.push_back(s[i++]);
      continue;
    }
    if (s[i + 1] == '\\') {
      out.append(s.substr(i, 2));
      i += 2;
      continue;
    }

    ::std::size_t command_end = i + 2;
    bool accent = ::std::string_view("'\"`^~=.").find(s[i + 1]) !=
                  ::std::string_view::npos;
    if (!accent) {
      ::std::size_t name_begin = 0;
      ::std::size_t name_end = 0;
      ::std::size_t token_end = 0;
      if (tex_control_word_at(s, i, &name_begin, &name_end, &token_end) &&
          token_end == name_end) {
        const auto name = s.substr(name_begin, name_end - name_begin);
        accent = name == "v" || name == "u" || name == "H" ||
                 name == "c" || name == "k" || name == "b" ||
                 name == "d" || name == "r" || name == "t";
        command_end = token_end;
      }
    }

    if (!accent) {
      out.push_back(s[i++]);
      continue;
    }

    const auto operand = tex_skip_space(s, command_end);
    ::std::size_t next = operand;
    ::std::string body;
    if (tex_parse_braced_group(s, operand, &next, &body)) {
      out.append(body);
      i = next;
      continue;
    }
    if (operand < s.size()) {
      ::std::uint32_t cp = 0;
      ::std::size_t cp_len = 0;
      decode_utf8_cp(s, operand, cp, cp_len);
      out.append(s.substr(operand, cp_len));
      i = operand + cp_len;
      continue;
    }

    out.append(s.substr(i, command_end - i));
    i = command_end;
  }
  return ::pluss::coden::native::cpu::strip_accents_utf8(out);
}

static ::std::string tex_to_markdown_text(::std::string_view s, ::std::int64_t mode) {
  ::std::string out(s);
  static const ::std::regex kSec(R"(\\section\*?\s*\{([^{}]*)\})");
  static const ::std::regex kSub(R"(\\subsection\*?\s*\{([^{}]*)\})");
  static const ::std::regex kSubSub(R"(\\subsubsection\*?\s*\{([^{}]*)\})");
  static const ::std::regex kBf(R"(\\textbf\s*\{([^{}]*)\})");
  static const ::std::regex kEm(R"(\\(emph|textit)\s*\{([^{}]*)\})");
  static const ::std::regex kItem(R"(\\item\b)");

  out = ::std::regex_replace(out, kSec, "\n# $1\n");
  out = ::std::regex_replace(out, kSub, "\n## $1\n");
  out = ::std::regex_replace(out, kSubSub, "\n### $1\n");
  out = ::std::regex_replace(out, kBf, "**$1**");
  out = ::std::regex_replace(out, kEm, "*$2*");
  out = ::std::regex_replace(out, kItem, "- ");

  out = tex_remove_color_text(out);
  out = tex_remove_hyperref_text(out);
  out = tex_remove_labels_text(out);
  out = tex_remove_refs_text(out);
  out = tex_remove_citations_text(out);
  out = tex_remove_packages_text(out);
  out = tex_remove_bibliography_text(out);
  out = tex_remove_spacing_cmds_text(out);
  out = tex_remove_left_right_text(out);
  out = tex_remove_big_delims_text(out);
  out = tex_remove_font_commands_text(out);
  out = tex_normalize_newlines_text(out);
  out = tex_dedup_blank_lines_text(out);
  out = ::pluss::coden::native::cpu::strip_ascii(out);
  // mode >= 1 also case-folds the recovered text, the same contract as
  // tex_to_text_text's own mode.
  if (mode >= 1) out = ::pluss::coden::native::cpu::lower_ascii(out);
  return out;
}

static ::std::string tex_to_html_text(::std::string_view s) {
  ::std::string out(s);
  static const ::std::regex kSec(R"(\\section\*?\s*\{([^{}]*)\})");
  static const ::std::regex kSub(R"(\\subsection\*?\s*\{([^{}]*)\})");
  static const ::std::regex kSubSub(R"(\\subsubsection\*?\s*\{([^{}]*)\})");
  static const ::std::regex kBf(R"(\\textbf\s*\{([^{}]*)\})");
  static const ::std::regex kEm(R"(\\(emph|textit)\s*\{([^{}]*)\})");
  static const ::std::regex kBr(R"(\\\\)");
  static const ::std::regex kItem(R"(\\item\b)");

  out = ::std::regex_replace(out, kSec, "<h1>$1</h1>");
  out = ::std::regex_replace(out, kSub, "<h2>$1</h2>");
  out = ::std::regex_replace(out, kSubSub, "<h3>$1</h3>");
  out = ::std::regex_replace(out, kBf, "<strong>$1</strong>");
  out = ::std::regex_replace(out, kEm, "<em>$2</em>");
  out = ::std::regex_replace(out, kBr, "<br/>");
  out = ::std::regex_replace(out, kItem, "<li>");

  out = tex_remove_color_text(out);
  out = tex_remove_hyperref_text(out);
  out = tex_remove_labels_text(out);
  out = tex_remove_refs_text(out);
  out = tex_remove_citations_text(out);
  out = tex_remove_packages_text(out);
  out = tex_remove_bibliography_text(out);
  out = tex_remove_spacing_cmds_text(out);
  out = tex_remove_left_right_text(out);
  out = tex_remove_big_delims_text(out);
  out = tex_remove_font_commands_text(out);
  out = tex_normalize_newlines_text(out);
  out = tex_dedup_blank_lines_text(out);
  return ::pluss::coden::native::cpu::strip_ascii(out);
}

static ::std::string normalize_tex_removed(::std::string_view s) {
  return ::pluss::coden::native::cpu::strip_ascii(
      ::pluss::coden::native::cpu::collapse_whitespace_ascii(s));
}

static ::std::string tex_remove_environment_text(::std::string_view s, ::std::string_view name) {
  const ::std::string begin = "\\begin{" + ::std::string(name) + "}";
  const ::std::string end = "\\end{" + ::std::string(name) + "}";
  auto escaped = [s](::std::size_t pos) {
    ::std::size_t slashes = 0;
    while (pos > slashes && s[pos - slashes - 1] == '\\') ++slashes;
    return (slashes & 1U) != 0U;
  };

  ::std::string out;
  out.reserve(s.size());
  ::std::size_t copied = 0;
  for (::std::size_t search = 0; search < s.size();) {
    auto first = s.find(begin, search);
    while (first != ::std::string_view::npos && escaped(first))
      first = s.find(begin, first + begin.size());
    if (first == ::std::string_view::npos) break;

    ::std::size_t cursor = first + begin.size();
    ::std::int64_t depth = 1;
    while (cursor < s.size() && depth > 0) {
      auto next_begin = s.find(begin, cursor);
      while (next_begin != ::std::string_view::npos && escaped(next_begin))
        next_begin = s.find(begin, next_begin + begin.size());
      auto next_end = s.find(end, cursor);
      while (next_end != ::std::string_view::npos && escaped(next_end))
        next_end = s.find(end, next_end + end.size());
      if (next_end == ::std::string_view::npos) break;
      if (next_begin != ::std::string_view::npos && next_begin < next_end) {
        ++depth;
        cursor = next_begin + begin.size();
      } else {
        --depth;
        cursor = next_end + end.size();
      }
    }
    if (depth != 0) break;
    out.append(s.substr(copied, first - copied));
    out.push_back(' ');
    copied = cursor;
    search = cursor;
  }
  out.append(s.substr(copied));
  return normalize_tex_removed(out);
}

struct TexEnvironmentToken {
  bool begin = false;
  ::std::string_view name;
  ::std::size_t end = 0;
};

static bool tex_environment_token_at(::std::string_view s, ::std::size_t pos,
                                     TexEnvironmentToken* token) {
  ::std::size_t name_begin = 0, name_end = 0, command_end = 0;
  if (!tex_control_word_at(s, pos, &name_begin, &name_end, &command_end))
    return false;
  const auto command = s.substr(name_begin, name_end - name_begin);
  if (command != "begin" && command != "end") return false;

  const auto group = tex_skip_space(s, command_end);
  ::std::size_t group_end = 0;
  if (group >= s.size() || s[group] != '{' ||
      !tex_group_end(s, group, '{', '}', &group_end))
    return false;
  const auto name = s.substr(group + 1, group_end - group - 2);
  if (name.empty()) return false;
  for (::std::size_t i = 0; i < name.size(); ++i) {
    const auto c = static_cast<unsigned char>(name[i]);
    if (::std::isalpha(c) == 0 && name[i] != '@' &&
        !(name[i] == '*' && i + 1 == name.size()))
      return false;
  }
  *token = TexEnvironmentToken{command == "begin", name, group_end};
  return true;
}

static ::std::string tex_remove_environments_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    TexEnvironmentToken first;
    if (!tex_environment_token_at(s, i, &first) || !first.begin) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    ::std::vector<::std::string_view> stack{first.name};
    ::std::size_t cursor = first.end;
    while (cursor < s.size() && !stack.empty()) {
      const auto slash = s.find('\\', cursor);
      if (slash == ::std::string_view::npos) break;
      TexEnvironmentToken next;
      if (!tex_environment_token_at(s, slash, &next)) {
        cursor = slash + 1;
        continue;
      }
      if (next.begin) {
        stack.push_back(next.name);
      } else if (next.name == stack.back()) {
        stack.pop_back();
      }
      cursor = next.end;
    }
    if (!stack.empty()) {
      out.push_back(s[i++]);
      continue;
    }
    out.push_back(' ');
    i = cursor;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_strip_metadata_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        token_end != name_end) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (name == "maketitle") {
      out.push_back(' ');
      i = token_end;
      continue;
    }
    if (!tex_name_in(name, {"title", "author", "date", "thanks"})) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    const auto body = tex_skip_space(s, token_end);
    ::std::size_t body_end = 0;
    if (body >= s.size() || s[body] != '{' ||
        !tex_group_end(s, body, '{', '}', &body_end)) {
      out.append(s.substr(i, token_end - i));
      i = token_end;
      continue;
    }
    out.push_back(' ');
    i = body_end;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_complete_calls(
    ::std::string_view s,
    ::std::initializer_list<::std::string_view> names,
    bool allow_optional) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        !tex_name_in(s.substr(name_begin, name_end - name_begin), names)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    auto cursor = tex_skip_space(s, token_end);
    if (allow_optional) {
      while (cursor < s.size() && s[cursor] == '[') {
        ::std::size_t next = 0;
        if (!tex_group_end(s, cursor, '[', ']', &next)) break;
        cursor = tex_skip_space(s, next);
      }
    }
    ::std::size_t next = 0;
    if (cursor >= s.size() || s[cursor] != '{' ||
        !tex_group_end(s, cursor, '{', '}', &next)) {
      out.push_back(s[i++]);
      continue;
    }
    out.push_back(' ');
    i = next;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_labels_text(::std::string_view s) {
  return tex_remove_complete_calls(s, {"label"}, false);
}

static ::std::string tex_remove_refs_text(::std::string_view s) {
  return tex_remove_complete_calls(
      s, {"ref", "eqref", "autoref", "pageref", "nameref", "cref",
          "Cref", "vref", "Vref"}, false);
}

static ::std::string tex_remove_citations_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        !starts_with_sv(s.substr(name_begin, name_end - name_begin), "cite")) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    ::std::size_t cursor = token_end;
    while (true) {
      const auto group = tex_skip_space(s, cursor);
      if (group >= s.size() || s[group] != '[') break;
      ::std::size_t next = 0;
      if (!tex_group_end(s, group, '[', ']', &next)) break;
      cursor = next;
    }
    const auto body = tex_skip_space(s, cursor);
    ::std::size_t next = 0;
    if (body < s.size() && s[body] == '{' &&
        tex_group_end(s, body, '{', '}', &next)) {
      out.push_back(' ');
      i = next;
    } else {
      out.push_back(s[i++]);
    }
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_graphics_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end) ||
        s.substr(name_begin, name_end - name_begin) != "includegraphics") {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    auto cursor = tex_skip_space(s, token_end);
    if (cursor < s.size() && s[cursor] == '[') {
      ::std::size_t next = 0;
      if (!tex_group_end(s, cursor, '[', ']', &next)) {
        out.push_back(s[i++]);
        continue;
      }
      cursor = tex_skip_space(s, next);
    }
    ::std::size_t next = 0;
    if (cursor >= s.size() || s[cursor] != '{' ||
        !tex_group_end(s, cursor, '{', '}', &next)) {
      out.push_back(s[i++]);
      continue;
    }
    out.push_back(' ');
    i = next;
  }
  out = tex_remove_environment_text(out, "figure");
  out = tex_remove_environment_text(out, "figure*");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_tables_text(::std::string_view s) {
  static constexpr ::std::array<::std::string_view, 6> kTables{
      "array", "longtable", "table", "table*", "tabular", "tabular*"};
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    TexEnvironmentToken first;
    if (!tex_environment_token_at(s, i, &first) || !first.begin ||
        ::std::find(kTables.begin(), kTables.end(), first.name) == kTables.end()) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }

    ::std::vector<::std::string_view> stack{first.name};
    ::std::size_t cursor = first.end;
    while (cursor < s.size() && !stack.empty()) {
      const auto slash = s.find('\\', cursor);
      if (slash == ::std::string_view::npos) break;
      TexEnvironmentToken next;
      if (!tex_environment_token_at(s, slash, &next)) {
        cursor = slash + 1;
        continue;
      }
      if (next.begin) stack.push_back(next.name);
      else if (next.name == stack.back()) stack.pop_back();
      cursor = next.end;
    }
    if (!stack.empty()) {
      out.push_back(s[i++]);
      continue;
    }
    out.push_back(' ');
    i = cursor;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_figures_text(::std::string_view s) {
  ::std::string out(s);
  out = tex_remove_environment_text(out, "figure");
  out = tex_remove_environment_text(out, "figure*");
  out = tex_remove_environment_text(out, "wrapfigure");
  out = tex_remove_environment_text(out, "subfigure");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_packages_text(::std::string_view s) {
  return tex_remove_complete_calls(
      s, {"usepackage", "RequirePackage", "documentclass"}, true);
}

static ::std::string tex_remove_bibliography_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    const bool requires_body = name == "bibliography" || name == "addbibresource";
    if (!requires_body && name != "printbibliography") {
      out.push_back(s[i++]);
      continue;
    }
    ::std::size_t cursor = token_end;
    while (true) {
      const auto group = tex_skip_space(s, cursor);
      if (group >= s.size() || s[group] != '[') break;
      ::std::size_t next = 0;
      if (!tex_group_end(s, group, '[', ']', &next)) break;
      cursor = next;
    }
    const auto body = tex_skip_space(s, cursor);
    ::std::size_t next = 0;
    if (body < s.size() && s[body] == '{' &&
        tex_group_end(s, body, '{', '}', &next)) {
      cursor = next;
    } else if (requires_body) {
      out.push_back(s[i++]);
      continue;
    }
    out.push_back(' ');
    i = cursor;
  }
  out = tex_remove_environment_text(out, "thebibliography");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_hyperref_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (name != "href" && name != "hyperref" && name != "url") {
      out.push_back(s[i++]);
      continue;
    }

    auto cursor = tex_skip_space(s, token_end);
    const char first_open = name == "hyperref" ? '[' : '{';
    const char first_close = name == "hyperref" ? ']' : '}';
    ::std::size_t first_end = 0;
    if (cursor >= s.size() || s[cursor] != first_open ||
        !tex_group_end(s, cursor, first_open, first_close, &first_end)) {
      out.push_back(s[i++]);
      continue;
    }
    if (name == "url") {
      out.push_back(' ');
      i = first_end;
      continue;
    }

    const auto body = tex_skip_space(s, first_end);
    ::std::size_t body_end = 0;
    if (body >= s.size() || s[body] != '{' ||
        !tex_group_end(s, body, '{', '}', &body_end)) {
      out.push_back(s[i++]);
      continue;
    }
    out += tex_remove_hyperref_text(
        s.substr(body + 1, body_end - body - 2));
    i = body_end;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_urls_text(::std::string_view s) {
  return tex_remove_hyperref_text(s);
}

static ::std::string tex_remove_color_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    int required = 0;
    int keep = -1;
    if (name == "textcolor" || name == "colorbox") { required = 2; keep = 1; }
    else if (name == "fcolorbox") { required = 3; keep = 2; }
    else if (name == "color" || name == "pagecolor") { required = 1; }
    else { out.push_back(s[i++]); continue; }

    ::std::size_t cursor = token_end;
    const auto optional = tex_skip_space(s, cursor);
    if (optional < s.size() && s[optional] == '[') {
      ::std::size_t next = 0;
      if (!tex_group_end(s, optional, '[', ']', &next)) {
        out.push_back(s[i++]);
        continue;
      }
      cursor = next;
    }
    ::std::vector<::std::string> bodies;
    bool complete = true;
    for (int group_index = 0; group_index < required; ++group_index) {
      const auto group = tex_skip_space(s, cursor);
      ::std::size_t next = 0;
      ::std::string body;
      if (!tex_parse_braced_group(s, group, &next, &body)) {
        complete = false;
        break;
      }
      bodies.push_back(::std::move(body));
      cursor = next;
    }
    if (!complete) { out.push_back(s[i++]); continue; }
    if (keep >= 0) out.append(bodies[static_cast<::std::size_t>(keep)]);
    else out.push_back(' ');
    i = cursor;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_strip_math_text(::std::string_view s) {
  return normalize_tex_removed(::pluss::coden::native::cpu::nlp_remove_math_fast(s));
}

static ::std::string strip_math_delims(::std::string_view m) {
  if (starts_with_sv(m, "$$") && ends_with_sv(m, "$$") && m.size() >= 4) return ::std::string(m.substr(2, m.size() - 4));
  if (starts_with_sv(m, "$") && ends_with_sv(m, "$") && m.size() >= 2) return ::std::string(m.substr(1, m.size() - 2));
  if (starts_with_sv(m, "\\(") && ends_with_sv(m, "\\)") && m.size() >= 4) return ::std::string(m.substr(2, m.size() - 4));
  if (starts_with_sv(m, "\\[") && ends_with_sv(m, "\\]") && m.size() >= 4) return ::std::string(m.substr(2, m.size() - 4));
  return ::std::string(m);
}

static ::std::string tex_strip_text_text(::std::string_view s) {
  ::std::vector<::std::string> blocks;
  extract_tex_math_blocks_to(s, blocks);
  ::std::string out;
  for (const auto& block : blocks) {
    ::std::string m = strip_math_delims(block);
    m = ::pluss::coden::native::cpu::strip_ascii(m);
    if (m.empty()) continue;
    if (!out.empty()) out.push_back(' ');
    out.append(m);
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_left_right_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto name = s.substr(name_begin, name_end - name_begin);
      if (name == "left" || name == "right") {
        i = token_end;
        continue;
      }
    }
    const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
    out.append(s.substr(i, take));
    i += take;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_big_delims_text(::std::string_view s) {
  static constexpr ::std::array<::std::string_view, 12> kBigCmds{
      "bigl", "bigr", "Bigl", "Bigr", "biggl", "biggr",
      "Biggl", "Biggr", "big", "Big", "bigg", "Bigg"};
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto name = s.substr(name_begin, name_end - name_begin);
      if (::std::find(kBigCmds.begin(), kBigCmds.end(), name) != kBigCmds.end()) {
        i = token_end;
        continue;
      }
    }
    const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
    out.append(s.substr(i, take));
    i += take;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_spacing_cmds_text(::std::string_view s) {
  static constexpr ::std::array<::std::string_view, 6> kWords{
      "enspace", "medspace", "quad", "qquad", "thickspace", "thinspace"};
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] != '\\' &&
        ::std::string_view(",;:! /").find(s[i + 1]) != ::std::string_view::npos) {
      out.push_back(' ');
      i += 2;
      continue;
    }

    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const auto name = s.substr(name_begin, name_end - name_begin);
    if (token_end == name_end &&
        ::std::find(kWords.begin(), kWords.end(), name) != kWords.end()) {
      out.push_back(' ');
      i = token_end;
      continue;
    }
    if (name == "hspace" || name == "vspace") {
      auto cursor = tex_skip_space(s, token_end);
      ::std::size_t next = 0;
      if (cursor < s.size() && s[cursor] == '{' &&
          tex_group_end(s, cursor, '{', '}', &next)) {
        out.push_back(' ');
        i = next;
        continue;
      }
    }
    if (name == "vskip" && token_end == name_end) {
      auto cursor = tex_skip_space(s, token_end);
      const auto value_begin = cursor;
      while (cursor < s.size() &&
             ::std::isspace(static_cast<unsigned char>(s[cursor])) == 0 &&
             s[cursor] != '\\') {
        ++cursor;
      }
      if (cursor > value_begin) {
        out.push_back(' ');
        i = cursor;
        continue;
      }
    }
    out.append(s.substr(i, token_end - i));
    i = token_end;
  }
  return normalize_tex_removed(out);
}

static ::std::string tex_replace_command_text(::std::string_view s,
                                              ::std::string_view cmd,
                                              ::std::string_view repl) {
  while (!cmd.empty() && cmd.front() == '\\') cmd.remove_prefix(1);
  if (cmd.empty()) return ::std::string(s);
  const bool want_star = cmd.back() == '*';
  if (want_star) cmd.remove_suffix(1);

  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    ::std::size_t name_begin = 0, name_end = 0, token_end = 0;
    if (!tex_control_word_at(s, i, &name_begin, &name_end, &token_end)) {
      const auto take = s[i] == '\\' && i + 1 < s.size() ? 2U : 1U;
      out.append(s.substr(i, take));
      i += take;
      continue;
    }
    const bool has_star = token_end != name_end;
    if (s.substr(name_begin, name_end - name_begin) == cmd &&
        has_star == want_star) {
      out.append(repl);
    } else {
      out.append(s.substr(i, token_end - i));
    }
    i = token_end;
  }
  return out;
}

static ::std::array<::std::int64_t,3> strides3(const ::std::array<::std::int64_t,3>& sizes, ::std::int64_t dim) {
  ::std::array<::std::int64_t,3> st{1,1,1};
  ::std::int64_t acc = 1;
  for (::std::int64_t i = dim - 1; i >= 0; --i) {
    st[static_cast<::std::size_t>(i)] = acc;
    acc *= sizes[static_cast<::std::size_t>(i)];
  }
  return st;
}

static void unravel(::std::int64_t linear,
                    const ::std::array<::std::int64_t,3>& sizes,
                    ::std::int64_t dim,
                    ::std::array<::std::int64_t,3>& out_coords) {
  out_coords = {0,0,0};
  for (::std::int64_t i = dim - 1; i >= 0; --i) {
    const auto s = sizes[static_cast<::std::size_t>(i)];
    out_coords[static_cast<::std::size_t>(i)] = (s == 0) ? 0 : (linear % s);
    linear = (s == 0) ? 0 : (linear / s);
  }
}

static ::std::int64_t ravel(const ::std::array<::std::int64_t,3>& coords,
                          const ::std::array<::std::int64_t,3>& strides,
                          ::std::int64_t dim) {
  ::std::int64_t linear = 0;
  for (::std::int64_t i = 0; i < dim; ++i) {
    linear += coords[static_cast<::std::size_t>(i)] * strides[static_cast<::std::size_t>(i)];
  }
  return linear;
}

static ::std::int64_t wrap_dim(::std::int64_t d, ::std::int64_t nd, bool allow_end) {
  const ::std::int64_t upper = allow_end ? nd : (nd - 1);
  if (d < 0) d += nd + (allow_end ? 1 : 0);
  PLUSS_CHECK(d >= 0 && d <= upper, "dim out of range");
  return d;
}

// Validate an axis argument and normalize a negative axis (-1 == last).
static ::std::int64_t check_dim(const Note& x, ::std::int64_t d) {
  PLUSS_CHECK(x.defined(), "Note op on undefined Note");
  const auto dim = x.dim();
  if (d < 0) d += dim;
  PLUSS_CHECK(d >= 0 && d < dim, "dim out of range");
  return d;
}

} // namespace

// Internal construction hooks.

struct NoteFactoryAccess {
  static Note wrap(::pluss::flame::intrusive_ptr<NoteImpl> impl) {
    return Note(::std::move(impl));
  }
};

static ::std::pair<::std::array<::std::int64_t, 3>, ::std::int64_t>
note_shape3(::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() <= 3, "Note supports up to 3D");
  ::std::array<::std::int64_t, 3> shape{1, 1, 1};
  for (::std::size_t i = 0; i < sizes.size(); ++i) shape[i] = sizes[i];
  return {shape, static_cast<::std::int64_t>(sizes.size())};
}

static void require_note_payload_count(
    ::std::size_t count,
    const ::std::array<::std::int64_t, 3>& shape,
    ::std::int64_t dim) {
  const auto expected = NoteImpl::shape_numel(shape, dim);
  PLUSS_CHECK(count == static_cast<::std::size_t>(expected),
              "Note payload count must equal shape cardinality");
}

Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes) {
  const auto [sz, dim] = note_shape3(sizes);
  require_note_payload_count(utf8_flat.size(), sz, dim);

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(utf8_flat.size());
  for (auto s : utf8_flat) {
    spans.push_back(arena->append(s));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteDType::Text);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_utf8_packed(::std::vector<char>&& utf8_arena,
                           ::std::vector<::std::pair<::std::size_t, ::std::size_t>>&& spans,
                           ::std::span<const ::std::int64_t> sizes) {
  const auto [sz, dim] = note_shape3(sizes);
  require_note_payload_count(spans.size(), sz, dim);

  auto arena = ::pluss::flame::make_intrusive<TextArena>(::std::move(utf8_arena));
  ::std::vector<NoteImpl::OffLen> note_spans = ::std::move(spans);
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena,
      ::std::move(note_spans),
      sz,
      dim,
      NoteDType::Text);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes) {
  const auto [sz, dim] = note_shape3(sizes);
  require_note_payload_count(bytes_flat.size(), sz, dim);

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(bytes_flat.size());
  for (auto b : bytes_flat) {
    spans.push_back(arena->append(b));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteDType::Bytes);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_bool(::std::span<const ::std::uint8_t> values,
                    ::std::span<const ::std::int64_t> sizes) {
  return note_from_bool_storage(NoteImpl::BoolStorage(values.begin(), values.end()), sizes);
}

Note note_from_bool_storage(NoteImpl::BoolStorage values,
                            ::std::span<const ::std::int64_t> sizes) {
  const auto [shape, dim] = note_shape3(sizes);
  require_note_payload_count(values.size(), shape, dim);
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(values), shape, dim);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_int64(::std::span<const ::std::int64_t> values,
                     ::std::span<const ::std::int64_t> sizes) {
  return note_from_int64_storage(NoteImpl::Int64Storage(values.begin(), values.end()), sizes);
}

Note note_from_int64_storage(NoteImpl::Int64Storage values,
                             ::std::span<const ::std::int64_t> sizes) {
  const auto [shape, dim] = note_shape3(sizes);
  require_note_payload_count(values.size(), shape, dim);
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(values), shape, dim);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_float64(::std::span<const double> values,
                       ::std::span<const ::std::int64_t> sizes) {
  return note_from_float64_storage(NoteImpl::Float64Storage(values.begin(), values.end()), sizes);
}

Note note_from_float64_storage(NoteImpl::Float64Storage values,
                               ::std::span<const ::std::int64_t> sizes) {
  const auto [shape, dim] = note_shape3(sizes);
  require_note_payload_count(values.size(), shape, dim);
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(values), shape, dim);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_contiguous_view(const Note &source, ::std::int64_t offset,
                          ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(source.defined(), "Note view source must be defined");
  const auto [shape, dim] = note_shape3(sizes);
  const auto count = NoteImpl::shape_numel(shape, dim);
  PLUSS_CHECK(offset >= 0 && offset <= source.numel(),
              "Note view offset is out of range");
  PLUSS_CHECK(count <= source.numel() - offset,
              "Note view extends beyond its source");

  const auto &source_impl = NoteAccess::impl(source);
  ::pluss::flame::intrusive_ptr<NoteImpl> view_impl;
  if (source_impl->is_textual()) {
    const auto spans = source_impl->spans();
    const auto first = spans.begin() + offset;
    view_impl = ::pluss::flame::make_intrusive<NoteImpl>(
        source_impl->arena(),
        ::std::vector<NoteImpl::OffLen>(first, first + count), shape, dim,
        source_impl->dtype());
  } else if (source_impl->is_bool()) {
    auto view = source_impl->bool_view();
    view.offset += static_cast<::std::size_t>(offset);
    view_impl = ::pluss::flame::make_intrusive<NoteImpl>(::std::move(view), shape, dim);
  } else if (source_impl->is_int64()) {
    auto view = source_impl->int64_view();
    view.offset += static_cast<::std::size_t>(offset);
    view_impl = ::pluss::flame::make_intrusive<NoteImpl>(::std::move(view), shape, dim);
  } else {
    auto view = source_impl->float64_view();
    view.offset += static_cast<::std::size_t>(offset);
    view_impl = ::pluss::flame::make_intrusive<NoteImpl>(::std::move(view), shape, dim);
  }
  return NoteFactoryAccess::wrap(::std::move(view_impl));
}

static void require_text_note(const NoteImpl& impl, const char* op) {
  PLUSS_CHECK(impl.is_text(),
              ::std::string(op) + " requires a Text Note");
}

template <class Transform>
static Note transform_note_text(const Note& x, const char* op, Transform&& fn) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, op);

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    spans.push_back(arena->append(fn(impl->bytes_at(i))));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

// Float64 counterpart of transform_note_text: maps each text cell to a raw
// double and stores it as a native Float64 Note, preserving the receiver's
// shape. Used by the numeric-score family (nlp_sentiment, jaccard_*, ...)
// so these produce a real Float64 Note instead of a formatted Text Note.
template <class Transform>
static Note transform_note_text_to_float64(const Note& x, const char* op, Transform&& fn) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, op);

  NoteImpl::Float64Storage values(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    values[static_cast<::std::size_t>(i)] = fn(impl->bytes_at(i));
  }
  return note_from_float64_storage(::std::move(values), impl->sizes());
}

template <class Transform>
static Note transform_note_any(const Note& x, Transform&& fn) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    spans.push_back(arena->append(fn(impl->bytes_at(i))));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

template <class Estimate, class Transform>
static Note transform_note_any_reserve(const Note& x,
                                       Estimate&& estimate,
                                       Transform&& fn) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::size_t total_bytes = 0;
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    total_bytes += estimate(impl->bytes_at(i));
  }
  arena->reserve(total_bytes);
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    spans.push_back(arena->append(fn(impl->bytes_at(i))));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

template <class Transform>
static Note transform_note_with_encoding(const Note& x,
                                         NoteDType out_encoding,
                                         Transform&& fn) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    spans.push_back(arena->append(fn(impl->bytes_at(i))));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), out_encoding);
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

#if defined(PLUSSQIT_HAS_LIBUNISTRING)
using UnicodeCaseMap_ = uint8_t* (*)(
    const uint8_t*, size_t, const char*, uninorm_t, uint8_t*, size_t*);

static ::std::string unicode_case_map_(
    ::std::string_view input,
    UnicodeCaseMap_ operation,
    const char* where) {
  size_t output_size = 0;
  auto* output = operation(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(),
      nullptr, nullptr, nullptr, &output_size);
  PLUSS_CHECK(output != nullptr,
              ::std::string(where) + ": Unicode case mapping failed");
  ::std::string mapped(
      reinterpret_cast<const char*>(output), output_size);
  ::std::free(output);
  return mapped;
}
#endif

Note lower(const Note& x) {
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  return transform_note_text(
      x, "Note::lower",
      [](auto value) {
        return unicode_case_map_(
            value, u8_tolower, "Note::lower");
      });
#else
  return transform_note_text(x, "Note::lower", [](auto value) {
    PLUSS_CHECK(
        ascii_only_(value),
        "Note::lower: non-ASCII text requires a Unicode backend");
    return ::pluss::coden::native::cpu::lower_ascii(value);
  });
#endif
}
Note upper(const Note& x) {
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  return transform_note_text(
      x, "Note::upper",
      [](auto value) {
        return unicode_case_map_(
            value, u8_toupper, "Note::upper");
      });
#else
  return transform_note_text(x, "Note::upper", [](auto value) {
    PLUSS_CHECK(
        ascii_only_(value),
        "Note::upper: non-ASCII text requires a Unicode backend");
    return ::pluss::coden::native::cpu::upper_ascii(value);
  });
#endif
}
Note title(const Note& x) {
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  return transform_note_text(
      x, "Note::title",
      [](auto value) {
        return unicode_case_map_(
            value, u8_totitle, "Note::title");
      });
#else
  return transform_note_text(x, "Note::title", [](auto value) {
    PLUSS_CHECK(
        ascii_only_(value),
        "Note::title: non-ASCII text requires a Unicode backend");
    return ::pluss::coden::native::cpu::title_ascii(value);
  });
#endif
}
Note capitalize(const Note& x) { return transform_note_text(x, "Note::capitalize", ::pluss::coden::native::cpu::capitalize_ascii); }
Note swapcase(const Note& x) { return transform_note_text(x, "Note::swapcase", ::pluss::coden::native::cpu::swapcase_ascii); }
Note strip(const Note& x) { return transform_note_text(x, "Note::strip", [](auto s) { return ::pluss::coden::native::cpu::strip_ascii(s); }); }
Note lstrip(const Note& x) { return transform_note_text(x, "Note::lstrip", [](auto s) { return ::pluss::coden::native::cpu::lstrip_ascii(s); }); }
Note rstrip(const Note& x) { return transform_note_text(x, "Note::rstrip", [](auto s) { return ::pluss::coden::native::cpu::rstrip_ascii(s); }); }
Note collapse_whitespace(const Note& x) {
  return transform_note_text(x, "Note::collapse_whitespace", ::pluss::coden::native::cpu::collapse_whitespace_ascii);
}
Note dedent(const Note& x) { return transform_note_text(x, "Note::dedent", ::pluss::coden::native::cpu::dedent_ascii); }
Note indent(const Note& x) {
  return transform_note_text(x, "Note::indent", [](auto s) { return ::pluss::coden::native::cpu::indent_ascii(s, 2); });
}
Note remove_control_chars(const Note& x) {
  return transform_note_text(x, "Note::remove_control_chars", ::pluss::coden::native::cpu::remove_control_ascii);
}
Note keep_ascii(const Note& x) {
  return transform_note_text(x, "Note::keep_ascii", ::pluss::coden::native::cpu::remove_non_ascii);
}
Note to_ascii(const Note& x) {
  return transform_note_text(x, "Note::to_ascii", ::pluss::coden::native::cpu::to_ascii_lossy);
}
Note remove_punctuation(const Note& x) {
  return transform_note_text(x, "Note::remove_punctuation", ::pluss::coden::native::cpu::remove_punctuation_ascii);
}
Note remove_digits(const Note& x) {
  return transform_note_text(x, "Note::remove_digits", ::pluss::coden::native::cpu::remove_digits_ascii);
}
Note remove_letters(const Note& x) {
  return transform_note_text(x, "Note::remove_letters", ::pluss::coden::native::cpu::remove_letters_ascii);
}
Note remove_symbols(const Note& x) {
  return transform_note_text(x, "Note::remove_symbols", ::pluss::coden::native::cpu::remove_symbols_ascii);
}
Note remove_emojis(const Note& x) {
  return transform_note_text(x, "Note::remove_emojis", ::pluss::coden::native::cpu::remove_emojis_utf8);
}
Note keep_alnum(const Note& x) {
  return transform_note_text(x, "Note::keep_alnum", ::pluss::coden::native::cpu::keep_alnum_ascii);
}
Note keep_alpha(const Note& x) {
  return transform_note_text(x, "Note::keep_alpha", ::pluss::coden::native::cpu::keep_alpha_ascii);
}
Note keep_digits(const Note& x) {
  return transform_note_text(x, "Note::keep_digits", ::pluss::coden::native::cpu::keep_digits_ascii);
}
Note keep_printable(const Note& x) {
  return transform_note_text(x, "Note::keep_printable", ::pluss::coden::native::cpu::keep_printable_ascii);
}
Note normalize_quotes(const Note& x) {
  return transform_note_text(x, "Note::normalize_quotes", ::pluss::coden::native::cpu::normalize_quotes_utf8);
}
Note normalize_dashes(const Note& x) {
  return transform_note_text(x, "Note::normalize_dashes", ::pluss::coden::native::cpu::normalize_dashes_utf8);
}
Note normalize_ellipsis(const Note& x) {
  return transform_note_text(x, "Note::normalize_ellipsis", ::pluss::coden::native::cpu::normalize_ellipsis_utf8);
}
Note strip_accents(const Note& x) {
  return transform_note_text(x, "Note::strip_accents", ::pluss::coden::native::cpu::strip_accents_utf8);
}
Note strip_zero_width(const Note& x) {
  return transform_note_text(x, "Note::strip_zero_width", ::pluss::coden::native::cpu::strip_zero_width_utf8);
}
Note strip_bom(const Note& x) {
  return transform_note_text(x, "Note::strip_bom", ::pluss::coden::native::cpu::strip_bom_utf8);
}
Note collapse_repeats(const Note& x) {
  return transform_note_text(x, "Note::collapse_repeats", ::pluss::coden::native::cpu::collapse_repeats_ascii);
}
Note collapse_punct(const Note& x) {
  return transform_note_text(x, "Note::collapse_punct", ::pluss::coden::native::cpu::collapse_punct_ascii);
}
Note collapse_spaces(const Note& x) {
  return transform_note_text(x, "Note::collapse_spaces", ::pluss::coden::native::cpu::collapse_spaces_ascii);
}
Note collapse_newlines(const Note& x) {
  return transform_note_text(x, "Note::collapse_newlines", ::pluss::coden::native::cpu::collapse_newlines_ascii);
}
Note nlp_normalize_punct(const Note& x, bool ascii_only) {
  auto out = normalize_quotes(x);
  out = normalize_dashes(out);
  out = normalize_ellipsis(out);
  if (ascii_only) out = to_ascii(out);
  return out;
}
Note nlp_clean(const Note& x) {
  auto out = nlp_normalize_punct(x, false);
  out = remove_control_chars(out);
  out = strip_zero_width(out);
  out = collapse_whitespace(out);
  out = strip(out);
  return out;
}
Note nlp_normalize_numbers(const Note& x) {
  return transform_note_text(x, "Note::nlp_normalize_numbers", [](auto s) { return nlp_normalize_numbers_text(s); });
}
Note nlp_expand_contractions(const Note& x) {
  return transform_note_text(x, "Note::nlp_expand_contractions", [](auto s) { return nlp_expand_contractions_text(s); });
}
Note nlp_remove_stopwords(const Note& x) {
  return nlp_remove_stopwords(x, "en");
}
Note nlp_remove_stopwords(const Note& x, ::std::string_view lang) {
  return transform_note_text(
      x, "Note::nlp_remove_stopwords",
      [lang](auto s) { return nlp_remove_stopwords_text(s, lang); });
}
Note nlp_language(const Note& x) {
  return transform_note_text(x, "Note::nlp_language", [](auto s) { return detect_script_and_language(s).lang; });
}
Note nlp_script(const Note& x) {
  return transform_note_text(x, "Note::nlp_script", [](auto s) { return detect_script_and_language(s).script; });
}
Note nlp_sentiment(const Note& x) {
  return transform_note_text_to_float64(x, "Note::nlp_sentiment", [](auto s) { return clamp01(sentiment_score_text(s)); });
}
Note nlp_toxicity(const Note& x) {
  return transform_note_text_to_float64(x, "Note::nlp_toxicity", [](auto s) { return clamp01(toxicity_score_text(s)); });
}
Note nlp_readability_grade(const Note& x) {
  return transform_note_text_to_float64(
      x, "Note::nlp_readability_grade",
      [](auto s) { return ::std::max(0.0, readability_grade_raw(s)); });
}
Note nlp_readability_level(const Note& x) {
  return transform_note_text(
      x, "Note::nlp_readability_level",
      [](auto s) { return readability_level_text(s); });
}
Note nlp_spam_score(const Note& x) {
  return transform_note_text_to_float64(x, "Note::nlp_spam_score", [](auto s) { return clamp01(spam_score_text(s)); });
}
Note nlp_formality(const Note& x) {
  return transform_note_text_to_float64(x, "Note::nlp_formality", [](auto s) { return clamp01(formality_score_text(s)); });
}
Note nlp_subjectivity(const Note& x) {
  return transform_note_text_to_float64(
      x, "Note::nlp_subjectivity",
      [](auto s) { return clamp01(subjectivity_score_text(s)); });
}
Note to_utf8_strict(const Note& x) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    PLUSS_CHECK(is_valid_utf8_text(impl->bytes_at(i)), "Note::to_utf8_strict found invalid UTF-8 payload");
  }
  return transform_note_with_encoding(
      x, NoteDType::Text,
      [](auto s) { return ::std::string(s); });
}
Note to_utf8_lossy(const Note& x) {
  return transform_note_with_encoding(
      x, NoteDType::Text,
      [](auto s) {
        ::std::string out;
        out.reserve(s.size());
        for (::std::size_t i = 0; i < s.size();) {
          ::std::uint32_t cp = 0;
          ::std::size_t cp_len = 0;
          const bool ok = decode_utf8_cp(s, i, cp, cp_len);
          if (ok) out.append(s.substr(i, cp_len));
          else out.append("\xEF\xBF\xBD");
          i += cp_len;
        }
        return out;
      });
}
Note from_latin1(const Note& x) {
  return transform_note_with_encoding(
      x, NoteDType::Text,
      [](auto s) { return latin1_decode_text(s); });
}
Note to_latin1_lossy(const Note& x) {
  return transform_note_with_encoding(
      to_utf8_lossy(x), NoteDType::Bytes,
      [](auto s) { return latin1_encode_lossy_text(s); });
}
Note from_windows1252(const Note& x) {
  return transform_note_with_encoding(
      x, NoteDType::Text,
      [](auto s) { return windows1252_decode_text(s); });
}
Note to_windows1252_lossy(const Note& x) {
  return transform_note_with_encoding(
      to_utf8_lossy(x), NoteDType::Bytes,
      [](auto s) { return windows1252_encode_lossy_text(s); });
}
Note transliterate_ascii(const Note& x) {
  return transform_note_text(
      x, "Note::transliterate_ascii",
      [](auto s) { return transliterate_ascii_text(s); });
}
Note transliterate_latin(const Note& x) {
  return transform_note_text(
      x, "Note::transliterate_latin",
      [](auto s) { return transliterate_latin_text(s); });
}
Note transliterate_greek(const Note& x) {
  return transform_note_text(
      x, "Note::transliterate_greek",
      [](auto s) { return transliterate_with_map(s, greek_map_cp); });
}
Note transliterate_cyrillic(const Note& x) {
  return transform_note_text(
      x, "Note::transliterate_cyrillic",
      [](auto s) { return transliterate_with_map(s, cyrillic_map_cp); });
}
Note phonetic_soundex(const Note& x) {
  return transform_note_text(
      x, "Note::phonetic_soundex",
      [](auto s) {
        return phonetic_transform_text(
            s, [](::std::string_view w) { return soundex_word(w); });
      });
}
Note phonetic_metaphone(const Note& x) {
  return transform_note_text(
      x, "Note::phonetic_metaphone",
      [](auto s) {
        return phonetic_transform_text(
            s, [](::std::string_view w) { return metaphone_simple_word(w); });
      });
}
Note nlp_remove_urls(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_urls", ::pluss::coden::native::cpu::nlp_remove_urls_fast);
}
Note nlp_remove_emails(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_emails", ::pluss::coden::native::cpu::nlp_remove_emails_fast);
}
Note nlp_remove_mentions(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_mentions", ::pluss::coden::native::cpu::nlp_remove_mentions_fast);
}
Note nlp_remove_hashtags(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_hashtags", ::pluss::coden::native::cpu::nlp_remove_hashtags_fast);
}
Note nlp_remove_html(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_html", ::pluss::coden::native::cpu::nlp_remove_html_fast);
}
Note nlp_remove_markdown(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_markdown", ::pluss::coden::native::cpu::nlp_remove_markdown_fast);
}
Note nlp_remove_code(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_code", ::pluss::coden::native::cpu::nlp_remove_code_fast);
}
Note nlp_remove_math(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_math", ::pluss::coden::native::cpu::nlp_remove_math_fast);
}
Note nlp_remove_latex(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_latex", ::pluss::coden::native::cpu::nlp_remove_latex_fast);
}
Note nlp_remove_brackets(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_brackets", ::pluss::coden::native::cpu::nlp_remove_brackets_fast);
}
Note nlp_remove_parentheticals(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_parentheticals", ::pluss::coden::native::cpu::nlp_remove_parentheticals_fast);
}
Note nlp_remove_quotes(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_quotes", ::pluss::coden::native::cpu::nlp_remove_quotes_fast);
}
Note nlp_remove_bullets(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_bullets", ::pluss::coden::native::cpu::nlp_remove_bullets_fast);
}
Note nlp_remove_headers(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_headers", ::pluss::coden::native::cpu::nlp_remove_headers_fast);
}
Note nlp_remove_footers(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_footers", ::pluss::coden::native::cpu::nlp_remove_footers_fast);
}
Note nlp_remove_boilerplate(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_boilerplate", ::pluss::coden::native::cpu::nlp_remove_boilerplate_fast);
}
Note nlp_remove_tables(const Note& x) {
  return transform_note_text(x, "Note::nlp_remove_tables", ::pluss::coden::native::cpu::nlp_remove_tables_fast);
}
Note nlp_mask_numbers(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_numbers", ::pluss::coden::native::cpu::nlp_mask_numbers_fast);
}
Note nlp_mask_emails(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_emails", ::pluss::coden::native::cpu::nlp_mask_emails_fast);
}
Note nlp_mask_urls(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_urls", ::pluss::coden::native::cpu::nlp_mask_urls_fast);
}
Note nlp_mask_mentions(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_mentions", ::pluss::coden::native::cpu::nlp_mask_mentions_fast);
}
Note nlp_mask_hashtags(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_hashtags", ::pluss::coden::native::cpu::nlp_mask_hashtags_fast);
}
Note nlp_mask_dates(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_dates", ::pluss::coden::native::cpu::nlp_mask_dates_fast);
}
Note nlp_mask_times(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_times", ::pluss::coden::native::cpu::nlp_mask_times_fast);
}
Note nlp_mask_currency(const Note& x) {
  return transform_note_text(x, "Note::nlp_mask_currency", ::pluss::coden::native::cpu::nlp_mask_currency_fast);
}
Note nlp_mask_pii(const Note& x, ::std::int64_t flags) {
  return transform_note_text(
      x, "Note::nlp_mask_pii", [flags](auto s) { return ::pluss::coden::native::cpu::nlp_mask_pii_fast(s, flags); });
}
Note tex_strip_comments(const Note& x) {
  return transform_note_text(x, "Note::tex_strip_comments", [](auto s) { return tex_strip_comments_text(s); });
}
Note tex_strip_preamble(const Note& x) {
  return transform_note_text(x, "Note::tex_strip_preamble", [](auto s) { return tex_strip_preamble_text(s); });
}
Note tex_strip_metadata(const Note& x) {
  return transform_note_text(x, "Note::tex_strip_metadata", [](auto s) { return tex_strip_metadata_text(s); });
}
Note tex_remove_commands(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_commands", [](auto s) { return tex_remove_commands_text(s); });
}
Note tex_remove_environments(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_environments", [](auto s) { return tex_remove_environments_text(s); });
}
Note tex_remove_environment(const Note& x, ::std::string_view name) {
  return transform_note_text(x, "Note::tex_remove_environment(name)", [name](auto s) { return tex_remove_environment_text(s, name); });
}
Note tex_remove_labels(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_labels", [](auto s) { return tex_remove_labels_text(s); });
}
Note tex_remove_refs(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_refs", [](auto s) { return tex_remove_refs_text(s); });
}
Note tex_remove_citations(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_citations", [](auto s) { return tex_remove_citations_text(s); });
}
Note tex_remove_graphics(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_graphics", [](auto s) { return tex_remove_graphics_text(s); });
}
Note tex_remove_tables(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_tables", [](auto s) { return tex_remove_tables_text(s); });
}
Note tex_remove_figures(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_figures", [](auto s) { return tex_remove_figures_text(s); });
}
Note tex_remove_packages(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_packages", [](auto s) { return tex_remove_packages_text(s); });
}
Note tex_remove_bibliography(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_bibliography", [](auto s) { return tex_remove_bibliography_text(s); });
}
Note tex_remove_color(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_color", [](auto s) { return tex_remove_color_text(s); });
}
Note tex_remove_hyperref(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_hyperref", [](auto s) { return tex_remove_hyperref_text(s); });
}
Note tex_remove_urls(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_urls", [](auto s) { return tex_remove_urls_text(s); });
}
Note tex_remove_left_right(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_left_right", [](auto s) { return tex_remove_left_right_text(s); });
}
Note tex_remove_big_delims(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_big_delims", [](auto s) { return tex_remove_big_delims_text(s); });
}
Note tex_remove_spacing_cmds(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_spacing_cmds", [](auto s) { return tex_remove_spacing_cmds_text(s); });
}
Note tex_balance_braces(const Note& x) {
  return transform_note_text(x, "Note::tex_balance_braces", [](auto s) { return tex_balance_braces_text(s); });
}
Note tex_balance_math_delims(const Note& x) {
  return transform_note_text(
      x, "Note::tex_balance_math_delims", [](auto s) { return tex_balance_math_delims_text(s); });
}
Note tex_canonicalize_macros(const Note& x) {
  return transform_note_text(
      x, "Note::tex_canonicalize_macros", [](auto s) { return tex_canonicalize_macros_text(s); });
}
Note tex_expand_macros(const Note& x, ::std::int64_t max_passes) {
  PLUSS_CHECK(max_passes > 0, "Note.tex().macro_subs: max_passes must be positive");
  return transform_note_text(
      x, "Note::tex().macro_subs", [max_passes](auto s) { return tex_expand_macros_text(s, max_passes); });
}
Note tex_inline_macros(const Note& x) {
  return transform_note_text(x, "Note::tex_inline_macros", [](auto s) { return tex_inline_macros_text(s); });
}
Note tex_collapse_macros(const Note& x) {
  return transform_note_text(x, "Note::tex_collapse_macros", [](auto s) { return tex_collapse_macros_text(s); });
}
Note tex_expand_inputs(const Note& x) {
  return transform_note_text(x, "Note::tex().input_tags", [](auto s) { return tex_expand_inputs_text(s); });
}
Note tex_resolve_graphics(const Note& x) {
  return transform_note_text(x, "Note::tex_resolve_graphics", [](auto s) { return tex_resolve_graphics_text(s); });
}
Note tex_set_macro(const Note& x, ::std::string_view name, ::std::string_view body) {
  return transform_note_text(
      x, "Note::tex_set_macro", [name, body](auto s) { return tex_set_macro_text(s, name, body); });
}
Note tex_remove_macro(const Note& x, ::std::string_view name) {
  return transform_note_text(x, "Note::tex_remove_macro", [name](auto s) { return tex_remove_macro_text(s, name); });
}
Note tex_wrap_inline_math(const Note& x, ::std::string_view delim) {
  return transform_note_text(
      x, "Note::tex_wrap_inline_math", [delim](auto s) { return tex_wrap_inline_math_text(s, delim); });
}
Note tex_wrap_display_math(const Note& x, ::std::string_view delim) {
  return transform_note_text(
      x, "Note::tex_wrap_display_math", [delim](auto s) { return tex_wrap_display_math_text(s, delim); });
}
Note tex_simplify_fractions(const Note& x) {
  return transform_note_text(x, "Note::tex_simplify_fractions", [](auto s) { return tex_simplify_fractions_text(s); });
}
Note tex_simplify_roots(const Note& x) {
  return transform_note_text(x, "Note::tex_simplify_roots", [](auto s) { return tex_simplify_roots_text(s); });
}
Note tex_convert_frac_to_slash(const Note& x) {
  return transform_note_text(
      x, "Note::tex().frac_slash", [](auto s) { return tex_convert_frac_to_slash_text(s); });
}
Note tex_convert_sqrt_to_pow(const Note& x) {
  return transform_note_text(
      x, "Note::tex().sqrt_pow", [](auto s) { return tex_convert_sqrt_to_pow_text(s); });
}
Note tex_convert_pm_to_plusminus(const Note& x) {
  return transform_note_text(
      x, "Note::tex().pm_signs", [](auto s) { return tex_convert_pm_to_plusminus_text(s); });
}
Note tex_standardize_vectors(const Note& x) {
  return transform_note_text(x, "Note::tex().vector_norm", [](auto s) { return tex_standardize_vectors_text(s); });
}
Note tex_standardize_matrices(const Note& x) {
  return transform_note_text(x, "Note::tex().matrix_norm", [](auto s) { return tex_standardize_matrices_text(s); });
}
Note tex_standardize_integrals(const Note& x) {
  return transform_note_text(
      x, "Note::tex().int_norm", [](auto s) { return tex_standardize_integrals_text(s); });
}
Note tex_standardize_summations(const Note& x) {
  return transform_note_text(
      x, "Note::tex().sum_norm", [](auto s) { return tex_standardize_summations_text(s); });
}
Note tex_standardize_limits(const Note& x) {
  return transform_note_text(x, "Note::tex().limit_norm", [](auto s) { return tex_standardize_limits_text(s); });
}
Note tex_standardize_subscripts(const Note& x) {
  return transform_note_text(
      x, "Note::tex().sub_norm", [](auto s) { return tex_standardize_subscripts_text(s); });
}
Note tex_standardize_superscripts(const Note& x) {
  return transform_note_text(
      x, "Note::tex().sup_norm", [](auto s) { return tex_standardize_superscripts_text(s); });
}
Note tex_standardize_primes(const Note& x) {
  return transform_note_text(x, "Note::tex().prime_norm", [](auto s) { return tex_standardize_primes_text(s); });
}
Note tex_standardize_dots(const Note& x) {
  return transform_note_text(x, "Note::tex().dots_norm", [](auto s) { return tex_standardize_dots_text(s); });
}
Note tex_remove_accents(const Note& x) {
  return transform_note_text(x, "Note::tex_remove_accents", [](auto s) { return tex_remove_accents_text(s); });
}
Note tex_replace_command(const Note& x, ::std::string_view cmd, ::std::string_view repl) {
  return transform_note_text(x, "Note::tex_replace_command", [cmd, repl](auto s) { return tex_replace_command_text(s, cmd, repl); });
}
Note tex_to_text(const Note& x, ::std::int64_t mode) {
  return transform_note_text(x, "Note::tex_to_text", [mode](auto s) { return tex_to_text_text(s, mode); });
}
Note text_to_tex(const Note& x, ::std::int64_t mode) {
  return transform_note_text(x, "Note::text_to_tex", [mode](auto s) { return text_to_tex_text(s, mode); });
}
Note tex_render(const Note& x, ::std::int64_t mode) {
  return transform_note_text(x, "Note::tex_render", [mode](auto s) { return tex_render_text(s, mode); });
}
Note tex_normalize_newlines(const Note& x) {
  return transform_note_text(x, "Note::tex_normalize_newlines", [](auto s) { return tex_normalize_newlines_text(s); });
}
Note tex_dedup_blank_lines(const Note& x) {
  return transform_note_text(x, "Note::tex_dedup_blank_lines", [](auto s) { return tex_dedup_blank_lines_text(s); });
}
Note tex_fix_linebreaks(const Note& x) {
  return transform_note_text(
      x, "Note::tex().fix_linebreaks",
      [](auto s) { return tex_fix_linebreaks_text(s); });
}
Note tex_escape(const Note& x) {
  return transform_note_text(x, "Note::tex_escape", [](auto s) { return tex_escape_text(s); });
}
Note tex_unescape(const Note& x) {
  return transform_note_text(x, "Note::tex_unescape", [](auto s) { return tex_unescape_text(s); });
}
Note tex_remove_math_delimiters(const Note& x) {
  return transform_note_text(
      x, "Note::tex_remove_math_delimiters",
      [](auto s) { return tex_remove_math_delimiters_text(s); });
}
Note tex_add_math_delimiters(const Note& x) {
  return transform_note_text(
      x, "Note::tex_add_math_delimiters",
      [](auto s) { return tex_add_math_delimiters_text(s); });
}
Note tex_math_only(const Note& x) {
  return transform_note_text(x, "Note::tex_math_only", [](auto s) { return tex_strip_text_text(s); });
}
Note tex_text_only(const Note& x) {
  return transform_note_text(x, "Note::tex_text_only", [](auto s) { return tex_strip_math_text(s); });
}
Note tex_remove_formatting(const Note& x) {
  return transform_note_text(
      x, "Note::tex_remove_formatting",
      [](auto s) { return tex_remove_formatting_text(s); });
}
Note tex_remove_font_commands(const Note& x) {
  return transform_note_text(
      x, "Note::tex_remove_font_commands",
      [](auto s) { return tex_remove_font_commands_text(s); });
}
Note tex_to_markdown(const Note& x, ::std::int64_t mode) {
  return transform_note_text(
      x, "Note::tex_to_markdown",
      [mode](auto s) { return tex_to_markdown_text(s, mode); });
}
Note tex_to_html(const Note& x, ::std::int64_t mode) {
  (void)mode;
  return transform_note_text(
      x, "Note::tex_to_html",
      [](auto s) { return tex_to_html_text(s); });
}

Note strip(const Note& x, ::std::string_view chars) {
  return transform_note_text(x, "Note::strip(chars)", [chars](auto s) { return ::pluss::coden::native::cpu::strip_ascii(s, chars); });
}
Note lstrip(const Note& x, ::std::string_view chars) {
  return transform_note_text(x, "Note::lstrip(chars)", [chars](auto s) { return ::pluss::coden::native::cpu::lstrip_ascii(s, chars); });
}
Note rstrip(const Note& x, ::std::string_view chars) {
  return transform_note_text(x, "Note::rstrip(chars)", [chars](auto s) { return ::pluss::coden::native::cpu::rstrip_ascii(s, chars); });
}

Note replace(const Note& x, ::std::string_view from, ::std::string_view to) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    ::std::string out = ::pluss::coden::native::cpu::replace_bytes(impl->bytes_at(i), from, to);
    spans.push_back(arena->append(out));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note replace_first(const Note& x, ::std::string_view from, ::std::string_view to) {
  return transform_note_any(x, [from, to](auto s) { return replace_first_text(s, from, to); });
}

Note replace_last(const Note& x, ::std::string_view from, ::std::string_view to) {
  return transform_note_any(x, [from, to](auto s) { return replace_last_text(s, from, to); });
}

Note replace_n(const Note& x, ::std::string_view from, ::std::string_view to, ::std::int64_t n) {
  return transform_note_any(x, [from, to, n](auto s) { return replace_n_text(s, from, to, n); });
}

Note remove(const Note& x, ::std::string_view needle) {
  return replace_first(x, needle, {});
}

Note remove_all(const Note& x, ::std::string_view needle) {
  return replace(x, needle, {});
}

Note remove_prefix(const Note& x, ::std::string_view prefix) {
  return transform_note_any(x, [prefix](auto s) { return remove_prefix_text(s, prefix); });
}

Note remove_suffix(const Note& x, ::std::string_view suffix) {
  return transform_note_any(x, [suffix](auto s) { return remove_suffix_text(s, suffix); });
}

Note ensure_prefix(const Note& x, ::std::string_view prefix) {
  return transform_note_any(x, [prefix](auto s) { return ensure_prefix_text(s, prefix); });
}

Note ensure_suffix(const Note& x, ::std::string_view suffix) {
  return transform_note_any(x, [suffix](auto s) { return ensure_suffix_text(s, suffix); });
}

Note surround(const Note& x, ::std::string_view left, ::std::string_view right) {
  return transform_note_any(x, [left, right](auto s) { return surround_text(s, left, right); });
}

Note quote(const Note& x) {
  return transform_note_any(x, [](auto s) { return quote_text(s); });
}

Note unquote(const Note& x) {
  return transform_note_any(x, [](auto s) { return unquote_text(s); });
}

Note base64_encode(const Note& x) {
  return transform_note_any_reserve(
      x,
      [](auto s) { return ((s.size() + 2u) / 3u) * 4u; },
      [](auto s) { return base64_encode_text(s); });
}

Note base64_decode(const Note& x) {
  return transform_note_any(x, [](auto s) { return base64_decode_text(s); });
}

Note hex_encode(const Note& x) {
  return transform_note_any_reserve(
      x,
      [](auto s) { return s.size() * 2u; },
      [](auto s) { return hex_encode_text(s); });
}

Note hex_decode(const Note& x) {
  return transform_note_any(x, [](auto s) { return hex_decode_text(s); });
}

Note url_encode(const Note& x) {
  return transform_note_any_reserve(
      x,
      [](auto s) { return s.size() * 3u; },
      [](auto s) { return url_encode_text(s); });
}

Note url_decode(const Note& x) {
  return transform_note_any(x, [](auto s) { return url_decode_text(s); });
}

Note html_escape(const Note& x) {
  return transform_note_any(x, [](auto s) { return html_escape_text(s); });
}

Note html_unescape(const Note& x) {
  return transform_note_any(x, [](auto s) { return html_unescape_text(s); });
}

Note json_escape(const Note& x) {
  return transform_note_any(x, [](auto s) { return json_escape_text(s); });
}

Note contains(const Note& x, ::std::string_view needle) {
  PLUSS_CHECK(x.defined(), "Note::contains: receiver must be defined");
  const auto& impl = NoteAccess::impl(x);
  PLUSS_CHECK(impl->is_textual(),
              "Note::contains: operation requires Text or Bytes storage");
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    out[static_cast<::std::size_t>(i)] = ::pluss::coden::native::cpu::contains_bytes(impl->bytes_at(i), needle) ? 1 : 0;
  }
  return note_from_bool_storage(::std::move(out), x.sizes_view());
}

namespace {

// Rebuilds a NoteImpl by mapping each output lane directly to one source lane.
// Text/Bytes copy only span metadata; native dtypes copy into packed storage.
// The callback form lets repetition avoid an O(output-numel) scratch index
// vector while selection operations can still pass a precomputed map.
template <class OldIndexAt>
::pluss::flame::intrusive_ptr<NoteImpl> map_note_impl(
    const NoteImpl& impl,
    ::std::size_t output_numel,
    OldIndexAt&& old_index_at,
    ::std::array<::std::int64_t, 3> new_sizes,
    ::std::int64_t new_dim) {
  switch (impl.dtype()) {
    case NoteDType::Text:
    case NoteDType::Bytes: {
      const auto& spans = impl.spans();
      ::std::vector<NoteImpl::OffLen> out(output_numel);
      for (::std::size_t i = 0; i < output_numel; ++i) {
        out[i] = spans.at(static_cast<::std::size_t>(old_index_at(i)));
      }
      return ::pluss::flame::make_intrusive<NoteImpl>(
          impl.arena(), ::std::move(out), new_sizes, new_dim, impl.dtype());
    }
    case NoteDType::Bool: {
      const auto values = impl.bool_values();
      NoteImpl::BoolStorage out(output_numel);
      for (::std::size_t i = 0; i < output_numel; ++i) {
        out[i] = values[static_cast<::std::size_t>(old_index_at(i))];
      }
      return ::pluss::flame::make_intrusive<NoteImpl>(::std::move(out), new_sizes, new_dim);
    }
    case NoteDType::Int64: {
      const auto values = impl.int64_values();
      NoteImpl::Int64Storage out(output_numel);
      for (::std::size_t i = 0; i < output_numel; ++i) {
        out[i] = values[static_cast<::std::size_t>(old_index_at(i))];
      }
      return ::pluss::flame::make_intrusive<NoteImpl>(::std::move(out), new_sizes, new_dim);
    }
    case NoteDType::Float64: {
      const auto values = impl.float64_values();
      NoteImpl::Float64Storage out(output_numel);
      for (::std::size_t i = 0; i < output_numel; ++i) {
        out[i] = values[static_cast<::std::size_t>(old_index_at(i))];
      }
      return ::pluss::flame::make_intrusive<NoteImpl>(::std::move(out), new_sizes, new_dim);
    }
  }
  PLUSS_CHECK(false, "gather_note_impl: unhandled Note dtype");
  return {};
}

::pluss::flame::intrusive_ptr<NoteImpl> gather_note_impl(
    const NoteImpl& impl,
    const ::std::vector<::std::int64_t>& old_lin_for_new,
    ::std::array<::std::int64_t, 3> new_sizes,
    ::std::int64_t new_dim) {
  return map_note_impl(
      impl, old_lin_for_new.size(),
      [&](::std::size_t i) { return old_lin_for_new.at(i); },
      new_sizes, new_dim);
}

::std::int64_t checked_shape_product(
    const ::std::array<::std::int64_t, 3>& sizes,
    ::std::int64_t dim,
    ::std::string_view operation) {
  ::std::int64_t product = 1;
  for (::std::int64_t axis = 0; axis < dim; ++axis) {
    const auto extent = sizes[static_cast<::std::size_t>(axis)];
    PLUSS_CHECK(extent >= 0,
                ::std::string(operation) + ": output extents must be non-negative");
    PLUSS_CHECK(product == 0 ||
                    extent <= ::std::numeric_limits<::std::int64_t>::max() / product,
                ::std::string(operation) + ": output shape overflows int64");
    product *= extent;
  }
  return product;
}

} // namespace

Note reshape(const Note& x, ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(x.defined(), "reshape on undefined Note");
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  const auto& impl = NoteAccess::impl(x);
  const ::std::int64_t total = impl->numel();

  ::std::array<::std::int64_t,3> sz{1,1,1};
  const ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());

  // One dimension may be given as -1 and is inferred from the total element
  // count and the product of the other (explicit) dimensions.
  ::std::int64_t infer_at = -1;
  ::std::int64_t known = 1;
  for (::std::int64_t i = 0; i < dim; ++i) {
    const ::std::int64_t d = sizes[static_cast<::std::size_t>(i)];
    if (d == -1) {
      PLUSS_CHECK(infer_at == -1, "reshape allows at most one inferred (-1) dimension");
      infer_at = i;
      continue;
    }
    PLUSS_CHECK(d >= 0, "reshape sizes must be non-negative (or a single -1)");
    known *= d;
  }
  for (::std::int64_t i = 0; i < dim; ++i) {
    sz[static_cast<::std::size_t>(i)] = sizes[static_cast<::std::size_t>(i)];
  }
  if (infer_at >= 0) {
    PLUSS_CHECK(known > 0 && total % known == 0,
               "reshape cannot infer the -1 dimension: total element count is not divisible by the other dimensions");
    sz[static_cast<::std::size_t>(infer_at)] = total / known;
  }

  ::std::int64_t n = 1;
  for (::std::int64_t i = 0; i < dim; ++i) n *= sz[static_cast<::std::size_t>(i)];
  PLUSS_CHECK(n == total, "reshape numel mismatch");
  // View-only reshape: relabels the same contiguous storage under new_sizes
  // without gathering/copying, for every dtype (shares arena/spans for
  // Text/Bytes, shares the LaneBuffer for Bool/Int64/Float64).
  ::pluss::flame::intrusive_ptr<NoteImpl> new_impl;
  switch (impl->dtype()) {
    case NoteDType::Text:
    case NoteDType::Bytes:
      new_impl = ::pluss::flame::make_intrusive<NoteImpl>(
          impl->arena(), ::std::vector<NoteImpl::OffLen>(impl->spans()), sz, dim, impl->dtype());
      break;
    case NoteDType::Bool:
      new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->bool_view(), sz, dim);
      break;
    case NoteDType::Int64:
      new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->int64_view(), sz, dim);
      break;
    case NoteDType::Float64:
      new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->float64_view(), sz, dim);
      break;
  }
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note permute(const Note& x, ::std::span<const ::std::int64_t> dims) {
  PLUSS_CHECK(x.defined(), "permute on undefined Note");
  const auto& impl = NoteAccess::impl(x);
  const auto dim = impl->dim();
  PLUSS_CHECK(static_cast<::std::int64_t>(dims.size()) == dim, "permute expects dims.size() == dim");

  // Validate permutation.
  ::std::array<bool,3> seen{false,false,false};
  for (::std::int64_t i = 0; i < dim; ++i) {
    const auto d = dims[static_cast<::std::size_t>(i)];
    PLUSS_CHECK(d >= 0 && d < dim, "permute dim out of range");
    PLUSS_CHECK(!seen[static_cast<::std::size_t>(d)], "permute dims must be a permutation");
    seen[static_cast<::std::size_t>(d)] = true;
  }

  const auto old_sizes = impl->sizes3();
  const auto old_strides = strides3(old_sizes, dim);

  ::std::array<::std::int64_t,3> new_sizes{1,1,1};
  for (::std::int64_t i = 0; i < dim; ++i) {
    new_sizes[static_cast<::std::size_t>(i)] = old_sizes[static_cast<::std::size_t>(dims[static_cast<::std::size_t>(i)])];
  }
  ::std::vector<::std::int64_t> old_lin_for_new(static_cast<::std::size_t>(impl->numel()));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < impl->numel(); ++new_lin) {
    unravel(new_lin, new_sizes, dim, new_c);
    old_c = {0,0,0};
    for (::std::int64_t i = 0; i < dim; ++i) {
      const auto old_d = dims[static_cast<::std::size_t>(i)];
      old_c[static_cast<::std::size_t>(old_d)] = new_c[static_cast<::std::size_t>(i)];
    }
    old_lin_for_new[static_cast<::std::size_t>(new_lin)] = ravel(old_c, old_strides, dim);
  }

  auto new_impl = gather_note_impl(*impl, old_lin_for_new, new_sizes, dim);
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note transpose(const Note& x, ::std::int64_t dim0, ::std::int64_t dim1) {
  PLUSS_CHECK(x.defined(), "transpose on undefined Note");
  const auto d = x.dim();
  PLUSS_CHECK(dim0 >= 0 && dim0 < d && dim1 >= 0 && dim1 < d, "transpose dim out of range");
  ::std::array<::std::int64_t,3> order{0,1,2};
  if (d > 0) {
    ::std::swap(order[static_cast<::std::size_t>(dim0)], order[static_cast<::std::size_t>(dim1)]);
  }
  ::std::vector<::std::int64_t> v;
  v.reserve(static_cast<::std::size_t>(d));
  for (::std::int64_t i = 0; i < d; ++i) v.push_back(order[static_cast<::std::size_t>(i)]);
  return permute(x, v);
}

Note select(const Note& x, ::std::int64_t dim, ::std::int64_t index) {
  dim = check_dim(x, dim);
  const auto& impl = NoteAccess::impl(x);
  const auto old_sizes = impl->sizes3();
  const auto old_dim = impl->dim();
  PLUSS_CHECK(old_dim > 0, "select expects dim() > 0");
  const auto size_d = old_sizes[static_cast<::std::size_t>(dim)];
  if (index < 0) index += size_d;
  PLUSS_CHECK(index >= 0 && index < size_d, "select index out of range");

  // Build new sizes (remove dim).
  ::std::array<::std::int64_t,3> new_sizes{1,1,1};
  ::std::int64_t new_dim = old_dim - 1;
  for (::std::int64_t i = 0, j = 0; i < old_dim; ++i) {
    if (i == dim) continue;
    new_sizes[static_cast<::std::size_t>(j++)] = old_sizes[static_cast<::std::size_t>(i)];
  }

  const auto old_strides = strides3(old_sizes, old_dim);
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  ::std::vector<::std::int64_t> old_lin_for_new(static_cast<::std::size_t>(new_numel));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < new_numel; ++new_lin) {
    unravel(new_lin, new_sizes, new_dim, new_c);
    old_c = {0,0,0};
    for (::std::int64_t i = 0, j = 0; i < old_dim; ++i) {
      if (i == dim) {
        old_c[static_cast<::std::size_t>(i)] = index;
      } else {
        old_c[static_cast<::std::size_t>(i)] = new_c[static_cast<::std::size_t>(j++)];
      }
    }
    old_lin_for_new[static_cast<::std::size_t>(new_lin)] = ravel(old_c, old_strides, old_dim);
  }

  auto new_impl = gather_note_impl(*impl, old_lin_for_new, new_sizes, new_dim);
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note narrow(const Note& x, ::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) {
  dim = check_dim(x, dim);
  const auto& impl = NoteAccess::impl(x);
  const auto old_sizes = impl->sizes3();
  const auto old_dim = impl->dim();
  const auto size_d = old_sizes[static_cast<::std::size_t>(dim)];
  PLUSS_CHECK(start >= 0 && start <= size_d, "narrow start out of range");
  PLUSS_CHECK(length >= 0, "narrow length must be non-negative");
  PLUSS_CHECK(start + length <= size_d, "narrow range out of bounds");

  ::std::array<::std::int64_t,3> new_sizes = old_sizes;
  new_sizes[static_cast<::std::size_t>(dim)] = length;
  const auto old_strides = strides3(old_sizes, old_dim);

  const auto new_dim = old_dim;
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  ::std::vector<::std::int64_t> old_lin_for_new(static_cast<::std::size_t>(new_numel));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < new_numel; ++new_lin) {
    unravel(new_lin, new_sizes, new_dim, new_c);
    old_c = new_c;
    old_c[static_cast<::std::size_t>(dim)] += start;
    old_lin_for_new[static_cast<::std::size_t>(new_lin)] = ravel(old_c, old_strides, old_dim);
  }

  auto new_impl = gather_note_impl(*impl, old_lin_for_new, new_sizes, new_dim);
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note contiguous(const Note& x) {
  return x;
}

Note flatten(const Note& x) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  const ::std::array<::std::int64_t, 1> sizes{impl->numel()};
  return reshape(x, ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
}

Note unsqueeze(const Note& x, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  const ::std::int64_t old_dim = impl->dim();
  PLUSS_CHECK(old_dim < 3, "inserting an axis would exceed Note max rank 3");
  dim = wrap_dim(dim, old_dim, true);
  const auto old_sizes = impl->sizes3();
  ::std::array<::std::int64_t, 3> new_sizes{1, 1, 1};
  const ::std::int64_t new_dim = old_dim + 1;
  for (::std::int64_t i = 0, j = 0; i < new_dim; ++i) {
    if (i == dim) continue;
    new_sizes[static_cast<::std::size_t>(i)] = old_sizes[static_cast<::std::size_t>(j++)];
  }
  return reshape(x, ::std::span<const ::std::int64_t>(new_sizes.data(), static_cast<::std::size_t>(new_dim)));
}

Note squeeze(const Note& x, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  const ::std::int64_t old_dim = impl->dim();
  if (old_dim == 0) return x;
  dim = wrap_dim(dim, old_dim, false);
  const auto old_sizes = impl->sizes3();
  if (old_sizes[static_cast<::std::size_t>(dim)] != 1) return x;

  ::std::array<::std::int64_t, 3> new_sizes{1, 1, 1};
  ::std::int64_t new_dim = 0;
  for (::std::int64_t i = 0; i < old_dim; ++i) {
    if (i == dim) continue;
    new_sizes[static_cast<::std::size_t>(new_dim++)] = old_sizes[static_cast<::std::size_t>(i)];
  }
  return reshape(x, ::std::span<const ::std::int64_t>(new_sizes.data(), static_cast<::std::size_t>(new_dim)));
}

Note repeat(const Note& x, ::std::span<const ::std::int64_t> repeats) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  const ::std::int64_t input_dim = impl->dim();
  PLUSS_CHECK(static_cast<::std::int64_t>(repeats.size()) >= input_dim,
              "repeat dimensions cannot be fewer than the Note rank");
  PLUSS_CHECK(repeats.size() <= 3, "repeat would exceed Note max rank 3");

  ::std::vector<::std::int64_t> reps;
  reps.reserve(repeats.size());
  for (const auto r : repeats) {
    PLUSS_CHECK(r >= 0, "repeat values must be >= 0");
    reps.push_back(r);
  }

  const ::std::int64_t out_dim = static_cast<::std::int64_t>(reps.size());

  ::std::array<::std::int64_t, 3> aligned_src{1, 1, 1};
  const auto src_sizes = impl->sizes3();
  for (::std::int64_t i = 0; i < input_dim; ++i) {
    aligned_src[static_cast<::std::size_t>(out_dim - input_dim + i)] =
        src_sizes[static_cast<::std::size_t>(i)];
  }

  ::std::array<::std::int64_t, 3> out_sizes{1, 1, 1};
  for (::std::int64_t i = 0; i < out_dim; ++i) {
    const auto source_extent = aligned_src[static_cast<::std::size_t>(i)];
    const auto copies = reps[static_cast<::std::size_t>(i)];
    PLUSS_CHECK(source_extent == 0 ||
                    copies <= ::std::numeric_limits<::std::int64_t>::max() / source_extent,
                "repeat output shape overflows int64");
    out_sizes[static_cast<::std::size_t>(i)] = source_extent * copies;
  }

  const auto out_numel = checked_shape_product(out_sizes, out_dim, "repeat");

  const auto src_strides = strides3(aligned_src, out_dim);
  auto source_index = [&](::std::size_t output_index) {
    ::std::array<::std::int64_t, 3> out_c{0, 0, 0};
    ::std::array<::std::int64_t, 3> src_c{0, 0, 0};
    const auto out_lin = static_cast<::std::int64_t>(output_index);
    unravel(out_lin, out_sizes, out_dim, out_c);
    for (::std::int64_t d = 0; d < out_dim; ++d) {
      const auto sz = aligned_src[static_cast<::std::size_t>(d)];
      src_c[static_cast<::std::size_t>(d)] = (sz == 0) ? 0 : (out_c[static_cast<::std::size_t>(d)] % sz);
    }
    return ravel(src_c, src_strides, out_dim);
  };

  auto out_impl = map_note_impl(
      *impl, static_cast<::std::size_t>(out_numel), source_index,
      out_sizes, out_dim);
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

Note repeat_interleave(const Note& x, ::std::int64_t repeats, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  PLUSS_CHECK(repeats >= 0, "repeat_interleave repeats must be >= 0");
  const auto& impl = NoteAccess::impl(x);
  const ::std::int64_t old_dim = impl->dim();
  if (old_dim == 0) {
    PLUSS_CHECK(dim == 0 || dim == -1,
                "repeat_interleave dim out of range for a scalar Note");
    const ::std::array<::std::int64_t, 1> reps_arr{repeats};
    return repeat(x, ::std::span<const ::std::int64_t>(reps_arr.data(), reps_arr.size()));
  }
  dim = wrap_dim(dim, old_dim, false);

  const auto old_sizes = impl->sizes3();
  ::std::array<::std::int64_t, 3> new_sizes = old_sizes;
  const auto old_extent = old_sizes[static_cast<::std::size_t>(dim)];
  PLUSS_CHECK(old_extent == 0 ||
                  repeats <= ::std::numeric_limits<::std::int64_t>::max() / old_extent,
              "repeat_interleave output shape overflows int64");
  new_sizes[static_cast<::std::size_t>(dim)] = old_extent * repeats;
  const auto new_numel =
      checked_shape_product(new_sizes, old_dim, "repeat_interleave");

  const auto old_strides = strides3(old_sizes, old_dim);
  auto source_index = [&](::std::size_t output_index) {
    ::std::array<::std::int64_t, 3> new_c{0, 0, 0};
    ::std::array<::std::int64_t, 3> old_c{0, 0, 0};
    unravel(static_cast<::std::int64_t>(output_index), new_sizes, old_dim, new_c);
    old_c = new_c;
    old_c[static_cast<::std::size_t>(dim)] /= repeats;
    return ravel(old_c, old_strides, old_dim);
  };

  auto out_impl = map_note_impl(
      *impl, static_cast<::std::size_t>(new_numel), source_index,
      new_sizes, old_dim);
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

namespace {

enum class EncodedFastPathMode : ::std::uint8_t {
  Off = 0,
  Auto = 1,
  Force = 2,
};

struct EncodedSpanLane {
  bool enabled = false;
  ::std::vector<NoteImpl::OffLen> dict_spans; // first-seen dictionary entries
  ::std::vector<::std::uint32_t> codes;       // per-element dictionary code
};

static EncodedFastPathMode encoded_fastpath_mode() {
  if (coden_reference_mode_enabled()) return EncodedFastPathMode::Off;
  const char* raw = ::std::getenv("PLUSS_CODEN_ENCODE_FASTPATH");
  if (raw == nullptr || *raw == '\0') return EncodedFastPathMode::Auto;
  ::std::string_view v(raw);
  ::std::string lower;
  lower.reserve(v.size());
  for (char c : v) lower.push_back(static_cast<char>(::std::tolower(static_cast<unsigned char>(c))));
  if (lower == "0" || lower == "off" || lower == "false" || lower == "no") return EncodedFastPathMode::Off;
  if (lower == "1" || lower == "on" || lower == "true" || lower == "yes" || lower == "force") {
    return EncodedFastPathMode::Force;
  }
  return EncodedFastPathMode::Auto;
}

static bool should_try_encoded_auto(
    const ::std::vector<NoteImpl::OffLen>& src,
    const ::pluss::flame::intrusive_ptr<TextArena>& arena) {
  const auto n = src.size();
  if (n < 256) return false;
  const auto sample_n = ::std::min<::std::size_t>(n, 1024);
  ::std::unordered_set<::std::string_view> sample;
  sample.reserve(sample_n * 2);
  for (::std::size_t i = 0; i < sample_n; ++i) {
    const auto& s = src[i];
    sample.insert(arena->view(s.first, s.second));
  }
  const double distinct_ratio = static_cast<double>(sample.size()) / static_cast<double>(sample_n);
  return distinct_ratio <= 0.85;
}

static EncodedSpanLane build_encoded_lane(
    const ::std::vector<NoteImpl::OffLen>& src,
    const ::pluss::flame::intrusive_ptr<TextArena>& arena,
    bool force_enable) {
  EncodedSpanLane out;
  if (src.empty()) {
    out.enabled = true;
    return out;
  }

  out.codes.resize(src.size());
  out.dict_spans.reserve(src.size());
  ::std::unordered_map<::std::uint64_t, ::std::vector<::std::uint32_t>> buckets;
  buckets.reserve(src.size() * 2);

  ::std::vector<::std::string_view> views(src.size());
  for (::std::size_t i = 0; i < src.size(); ++i) {
    const auto& s = src[i];
    views[i] = arena->view(s.first, s.second);
  }
  ::std::vector<::std::uint64_t> hashes(src.size(), 0);
  native::cpu::hash_bytes_fnv1a_batch(
      ::std::span<const ::std::string_view>(views.data(), views.size()),
      ::std::span<::std::uint64_t>(hashes.data(), hashes.size()));

  for (::std::size_t i = 0; i < src.size(); ++i) {
    const auto h = hashes[i];
    const auto sv = views[i];
    auto& bucket = buckets[h];
    ::std::uint32_t code = ::std::numeric_limits<::std::uint32_t>::max();
    for (const auto cand : bucket) {
      const auto& d = out.dict_spans[static_cast<::std::size_t>(cand)];
      if (arena->view(d.first, d.second) == sv) {
        code = cand;
        break;
      }
    }
    if (code == ::std::numeric_limits<::std::uint32_t>::max()) {
      code = static_cast<::std::uint32_t>(out.dict_spans.size());
      out.dict_spans.push_back(src[i]);
      bucket.push_back(code);
    }
    out.codes[i] = code;
  }

  if (force_enable) {
    out.enabled = true;
    return out;
  }
  const double distinct_ratio =
      static_cast<double>(out.dict_spans.size()) / static_cast<double>(src.size());
  const double avg_reuse =
      static_cast<double>(src.size()) / static_cast<double>(::std::max<::std::size_t>(1, out.dict_spans.size()));
  out.enabled = (distinct_ratio <= 0.75 && avg_reuse >= 1.25);
  if (!out.enabled) {
    out.dict_spans.clear();
    out.codes.clear();
  }
  return out;
}

static EncodedSpanLane maybe_build_encoded_lane(
    const ::std::vector<NoteImpl::OffLen>& src,
    const ::pluss::flame::intrusive_ptr<TextArena>& arena) {
  const auto mode = encoded_fastpath_mode();
  if (mode == EncodedFastPathMode::Off) return {};
  if (mode == EncodedFastPathMode::Auto && !should_try_encoded_auto(src, arena)) return {};
  return build_encoded_lane(src, arena, mode == EncodedFastPathMode::Force);
}

static Note make_note_from_spans_1d(
    const ::pluss::flame::intrusive_ptr<TextArena>& arena,
    ::std::vector<NoteImpl::OffLen> spans,
    NoteDType dtype) {
  const ::std::array<::std::int64_t, 3> sizes{static_cast<::std::int64_t>(spans.size()), 1, 1};
  auto out_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), sizes, /*dim=*/1, dtype);
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

static ::std::vector<NoteImpl::OffLen> distinct_first_seen_spans_hashed(
    const ::std::vector<NoteImpl::OffLen>& src,
    const ::pluss::flame::intrusive_ptr<TextArena>& arena) {
  ::std::vector<NoteImpl::OffLen> out;
  out.reserve(src.size());
  if (src.empty()) return out;

  // Collision-safe hash buckets: hash narrows candidates, lexical equality confirms exact match.
  ::std::unordered_map<::std::uint64_t, ::std::vector<NoteImpl::OffLen>> seen;
  seen.reserve(src.size() * 2);

  ::std::vector<::std::string_view> views(src.size());
  for (::std::size_t i = 0; i < src.size(); ++i) {
    const auto& s = src[i];
    views[i] = arena->view(s.first, s.second);
  }
  ::std::vector<::std::uint64_t> hashes(src.size(), 0);
  native::cpu::hash_bytes_fnv1a_batch(
      ::std::span<const ::std::string_view>(views.data(), views.size()),
      ::std::span<::std::uint64_t>(hashes.data(), hashes.size()));

  for (::std::size_t i = 0; i < src.size(); ++i) {
    const auto h = hashes[i];
    const auto sv = views[i];
    auto& bucket = seen[h];
    bool exists = false;
    for (const auto& cand : bucket) {
      if (arena->view(cand.first, cand.second) == sv) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      bucket.push_back(src[i]);
      out.push_back(src[i]);
    }
  }

  return out;
}

} // namespace

NoteImpl::Int64Storage note_argsort_indices_1d(
    const Note& x,
    bool descending,
    bool stable) {
  PLUSS_CHECK(x.defined(), "note_argsort_indices_1d: Note must be defined");
  const auto& impl = NoteAccess::impl(x);
  PLUSS_CHECK(impl, "Note::argsort: receiver storage must be defined");
  PLUSS_CHECK(impl->dim() <= 1,
              "Note::argsort supports only scalar and vector Notes");

  const auto n = impl->numel();
  NoteImpl::Int64Storage order(static_cast<::std::size_t>(n));
  ::std::iota(order.begin(), order.end(), 0);

  const auto apply_sort = [&](auto&& comparator) {
    if (stable) {
      ::std::stable_sort(order.begin(), order.end(), comparator);
    } else {
      ::std::sort(order.begin(), order.end(), comparator);
    }
  };

  if (impl->is_text() || impl->is_bytes()) {
    const auto& spans = impl->spans();
    const auto arena = impl->arena();
    apply_sort([&](::std::int64_t i, ::std::int64_t j) {
      const auto& a = spans[static_cast<::std::size_t>(i)];
      const auto& b = spans[static_cast<::std::size_t>(j)];
      const auto lhs = descending ? b : a;
      const auto rhs = descending ? a : b;
      return native::cpu::compare_lex_bytes(
          arena->view(lhs.first, lhs.second),
          arena->view(rhs.first, rhs.second),
          native::cpu::LexCompareOp::Lt);
    });
    return order;
  }

  const auto sort_native = [&](auto values) {
    using value_type = typename decltype(values)::value_type;
    apply_sort([&](::std::int64_t i, ::std::int64_t j) {
      const auto a = values[static_cast<::std::size_t>(i)];
      const auto b = values[static_cast<::std::size_t>(j)];
      if constexpr (::std::is_floating_point_v<value_type>) {
        const bool a_nan = ::std::isnan(a);
        const bool b_nan = ::std::isnan(b);
        if (a_nan || b_nan) {
          if (a_nan && b_nan) return false;
          return descending ? a_nan : !a_nan;
        }
      }
      return descending ? b < a : a < b;
    });
  };

  switch (impl->dtype()) {
    case NoteDType::Bool:
      sort_native(impl->bool_values());
      break;
    case NoteDType::Int64:
      sort_native(impl->int64_values());
      break;
    case NoteDType::Float64:
      sort_native(impl->float64_values());
      break;
    case NoteDType::Text:
    case NoteDType::Bytes:
      break;
    }
  return order;
}

Note sort(const Note& x, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  const auto old_dim = impl->dim();
  if (old_dim == 0) return x;
  dim = wrap_dim(dim, old_dim, false);

  auto spans = impl->spans();
  const auto sizes = impl->sizes3();
  const auto arena = impl->arena();
  const ::std::int64_t size_d = sizes[static_cast<::std::size_t>(dim)];
  if (size_d <= 1) return x;

  ::std::int64_t stride = 1;
  for (::std::int64_t d = dim + 1; d < old_dim; ++d) {
    stride *= sizes[static_cast<::std::size_t>(d)];
  }
  ::std::int64_t outer = 1;
  for (::std::int64_t d = 0; d < dim; ++d) {
    outer *= sizes[static_cast<::std::size_t>(d)];
  }

  const ::std::int64_t lanes = outer * stride;
  auto sort_lane = [&](::std::int64_t lane_id, ::std::vector<NoteImpl::OffLen>& lane) {
    const ::std::int64_t o = lane_id / stride;
    const ::std::int64_t i = lane_id % stride;
    const ::std::int64_t base = o * size_d * stride + i;
    for (::std::int64_t k = 0; k < size_d; ++k) {
      lane[static_cast<::std::size_t>(k)] = spans[static_cast<::std::size_t>(base + k * stride)];
    }
    ::std::stable_sort(lane.begin(), lane.end(), [arena](const NoteImpl::OffLen& a, const NoteImpl::OffLen& b) {
      return arena->view(a.first, a.second) < arena->view(b.first, b.second);
    });
    for (::std::int64_t k = 0; k < size_d; ++k) {
      spans[static_cast<::std::size_t>(base + k * stride)] = lane[static_cast<::std::size_t>(k)];
    }
  };

  const ::std::int64_t lane_work = lanes * size_d;
  const bool sort_parallel_eligible =
      (size_d >= 32) && (lane_work >= 32768) &&
      (coden_parallel_workers(lanes, /*min_tasks_per_worker=*/256) > 1);
  const auto workers =
      sort_parallel_eligible
          ? coden_parallel_workers(lanes, /*min_tasks_per_worker=*/256)
          : 1;

  if (workers <= 1) {
    ::std::vector<NoteImpl::OffLen> lane(static_cast<::std::size_t>(size_d));
    for (::std::int64_t lane_id = 0; lane_id < lanes; ++lane_id) {
      sort_lane(lane_id, lane);
    }
  } else {
    const ::std::int64_t chunk = (lanes + workers - 1) / workers;
    ::std::vector<::std::thread> pool;
    pool.reserve(static_cast<::std::size_t>(workers - 1));

    for (::std::int64_t w = 1; w < workers; ++w) {
      const ::std::int64_t b = w * chunk;
      const ::std::int64_t e = ::std::min<::std::int64_t>(lanes, b + chunk);
      if (b >= e) break;
      pool.emplace_back([&, b, e]() {
        ::std::vector<NoteImpl::OffLen> lane(static_cast<::std::size_t>(size_d));
        for (::std::int64_t lane_id = b; lane_id < e; ++lane_id) {
          sort_lane(lane_id, lane);
        }
      });
    }

    ::std::vector<NoteImpl::OffLen> lane(static_cast<::std::size_t>(size_d));
    const ::std::int64_t e0 = ::std::min<::std::int64_t>(lanes, chunk);
    for (::std::int64_t lane_id = 0; lane_id < e0; ++lane_id) {
      sort_lane(lane_id, lane);
    }
    for (auto& t : pool) t.join();
  }

  auto out_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

namespace {

constexpr ::std::int64_t kFlattenedDistinct =
    ::std::numeric_limits<::std::int64_t>::min();

Note distinct_slices(const Note& x, ::std::int64_t dim, bool sorted) {
  const auto& impl = NoteAccess::impl(x);
  const auto rank = impl->dim();
  dim = wrap_dim(dim, rank, false);

  auto out_sizes = impl->sizes3();
  const auto axis_size = out_sizes[static_cast<::std::size_t>(dim)];

  ::std::int64_t inner = 1;
  for (::std::int64_t d = dim + 1; d < rank; ++d) {
    inner *= out_sizes[static_cast<::std::size_t>(d)];
  }
  ::std::int64_t outer = 1;
  for (::std::int64_t d = 0; d < dim; ++d) {
    outer *= out_sizes[static_cast<::std::size_t>(d)];
  }

  const auto& src = impl->spans();
  const auto arena = impl->arena();
  auto cell = [&](::std::int64_t axis, ::std::int64_t outer_index,
                  ::std::int64_t inner_index) {
    const auto linear = (outer_index * axis_size + axis) * inner + inner_index;
    const auto& span = src[static_cast<::std::size_t>(linear)];
    return arena->view(span.first, span.second);
  };
  auto same_slice = [&](::std::int64_t lhs, ::std::int64_t rhs) {
    for (::std::int64_t o = 0; o < outer; ++o) {
      for (::std::int64_t i = 0; i < inner; ++i) {
        if (cell(lhs, o, i) != cell(rhs, o, i)) return false;
      }
    }
    return true;
  };
  auto less_slice = [&](::std::int64_t lhs, ::std::int64_t rhs) {
    for (::std::int64_t o = 0; o < outer; ++o) {
      for (::std::int64_t i = 0; i < inner; ++i) {
        const auto a = cell(lhs, o, i);
        const auto b = cell(rhs, o, i);
        if (a < b) return true;
        if (b < a) return false;
      }
    }
    return false;
  };

  ::std::vector<::std::int64_t> kept;
  kept.reserve(static_cast<::std::size_t>(axis_size));
  if (sorted) {
    kept.resize(static_cast<::std::size_t>(axis_size));
    ::std::iota(kept.begin(), kept.end(), 0);
    ::std::stable_sort(kept.begin(), kept.end(), less_slice);
    kept.erase(::std::unique(kept.begin(), kept.end(), same_slice), kept.end());
  } else {
    ::std::unordered_map<::std::size_t, ::std::vector<::std::int64_t>> buckets;
    buckets.reserve(static_cast<::std::size_t>(axis_size));
    for (::std::int64_t axis = 0; axis < axis_size; ++axis) {
      ::std::size_t hash = 0xcbf29ce484222325ULL;
      for (::std::int64_t o = 0; o < outer; ++o) {
        for (::std::int64_t i = 0; i < inner; ++i) {
          const auto value_hash = ::std::hash<::std::string_view>{}(cell(axis, o, i));
          hash ^= value_hash + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
        }
      }
      auto& bucket = buckets[hash];
      const bool seen = ::std::any_of(
          bucket.begin(), bucket.end(),
          [&](::std::int64_t prior) { return same_slice(prior, axis); });
      if (!seen) {
        bucket.push_back(axis);
        kept.push_back(axis);
      }
    }
  }

  out_sizes[static_cast<::std::size_t>(dim)] =
      static_cast<::std::int64_t>(kept.size());
  ::std::vector<NoteImpl::OffLen> out;
  out.reserve(static_cast<::std::size_t>(outer *
                                         static_cast<::std::int64_t>(kept.size()) *
                                         inner));
  for (::std::int64_t o = 0; o < outer; ++o) {
    for (const auto axis : kept) {
      const auto base = (o * axis_size + axis) * inner;
      for (::std::int64_t i = 0; i < inner; ++i) {
        out.push_back(src[static_cast<::std::size_t>(base + i)]);
      }
    }
  }

  auto out_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(out), out_sizes, rank, impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

} // namespace

Note unique(const Note& x, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  if (dim != kFlattenedDistinct) return distinct_slices(x, dim, true);
  const auto& impl = NoteAccess::impl(x);

  if (coden_reference_mode_enabled()) {
    ::std::vector<::std::string> elems;
    elems.reserve(static_cast<::std::size_t>(impl->numel()));
    for (::std::int64_t i = 0; i < impl->numel(); ++i) {
      elems.emplace_back(impl->bytes_at(i));
    }
    ::std::sort(elems.begin(), elems.end());
    elems.erase(::std::unique(elems.begin(), elems.end()), elems.end());

    ::std::vector<::std::string_view> views;
    views.reserve(elems.size());
    for (const auto& e : elems) views.emplace_back(e);
    const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(views.size())};
    if (impl->is_bytes()) {
      return note_from_bytes(
          ::std::span<const ::std::string_view>(views.data(), views.size()),
          ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
    }
    return note_from_utf8(
        ::std::span<const ::std::string_view>(views.data(), views.size()),
        ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  }

  const auto& src = impl->spans();
  const auto arena = impl->arena();

  auto encoded = maybe_build_encoded_lane(src, arena);
  if (encoded.enabled) {
    auto dict = ::std::move(encoded.dict_spans);
    ::std::stable_sort(dict.begin(), dict.end(), [arena](const NoteImpl::OffLen& a, const NoteImpl::OffLen& b) {
      return native::cpu::compare_lex_bytes(
          arena->view(a.first, a.second),
          arena->view(b.first, b.second),
          native::cpu::LexCompareOp::Lt);
    });
    return make_note_from_spans_1d(arena, ::std::move(dict), impl->dtype());
  }

  auto distinct = distinct_first_seen_spans_hashed(src, arena);
  ::std::stable_sort(distinct.begin(), distinct.end(), [arena](const NoteImpl::OffLen& a, const NoteImpl::OffLen& b) {
    return native::cpu::compare_lex_bytes(
        arena->view(a.first, a.second),
        arena->view(b.first, b.second),
        native::cpu::LexCompareOp::Lt);
  });
  return make_note_from_spans_1d(arena, ::std::move(distinct), impl->dtype());
}

Note deduplicate(const Note& x, ::std::int64_t dim) {
  if (!x.defined()) return Note::undefined();
  if (dim != kFlattenedDistinct) return distinct_slices(x, dim, false);
  const auto& impl = NoteAccess::impl(x);

  if (coden_reference_mode_enabled()) {
    ::std::vector<::std::string> elems;
    elems.reserve(static_cast<::std::size_t>(impl->numel()));
    ::std::unordered_set<::std::string> seen;
    for (::std::int64_t i = 0; i < impl->numel(); ++i) {
      ::std::string v(impl->bytes_at(i));
      if (seen.insert(v).second) elems.push_back(::std::move(v));
    }

    ::std::vector<::std::string_view> views;
    views.reserve(elems.size());
    for (const auto& e : elems) views.emplace_back(e);
    const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(views.size())};
    if (impl->is_bytes()) {
      return note_from_bytes(
          ::std::span<const ::std::string_view>(views.data(), views.size()),
          ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
    }
    return note_from_utf8(
        ::std::span<const ::std::string_view>(views.data(), views.size()),
        ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  }

  const auto& src = impl->spans();
  const auto arena = impl->arena();

  auto encoded = maybe_build_encoded_lane(src, arena);
  if (encoded.enabled) {
    return make_note_from_spans_1d(arena, ::std::move(encoded.dict_spans), impl->dtype());
  }

  auto out = distinct_first_seen_spans_hashed(src, arena);

  const ::std::array<::std::int64_t, 3> sizes{static_cast<::std::int64_t>(out.size()), 1, 1};
  auto out_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      arena, ::std::move(out), sizes, /*dim=*/1, impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(out_impl));
}

TokenBatch split_whitespace(const Note& x) {
  return split_whitespace(x, -1);
}



// ---- Patch8: Unicode/regex/split/join hooks ----

Note normalize(const Note& x, ::std::int64_t form) {
  PLUSS_CHECK(form >= 0 && form <= 3,
              "Note::normalize: form must be NFC, NFD, NFKC, or NFKD");
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  const ::std::array<uninorm_t, 4> forms{
      UNINORM_NFC, UNINORM_NFD, UNINORM_NFKC, UNINORM_NFKD};
  return transform_note_text(
      x, "Note::normalize",
      [selected = forms[static_cast<::std::size_t>(form)]](
          ::std::string_view input) {
        size_t output_size = 0;
        auto* output = u8_normalize(
            selected,
            reinterpret_cast<const uint8_t*>(input.data()),
            input.size(), nullptr, &output_size);
        PLUSS_CHECK(output != nullptr,
                    "Note::normalize: Unicode normalization failed");
        ::std::string normalized(
            reinterpret_cast<const char*>(output), output_size);
        ::std::free(output);
        return normalized;
      });
#else
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::normalize");
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    PLUSS_CHECK(
        ascii_only_(impl->bytes_at(i)),
        "Note::normalize: non-ASCII text requires a Unicode backend");
  }
  return x;
#endif
}

Note casefold(const Note& x, ::std::string_view locale) {
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  const auto language = ::std::string(locale);
  return transform_note_text(
      x, "Note::casefold",
      [&language](::std::string_view input) {
        size_t output_size = 0;
        auto* output = u8_casefold(
            reinterpret_cast<const uint8_t*>(input.data()),
            input.size(),
            language.empty() ? nullptr : language.c_str(),
            nullptr, nullptr, &output_size);
        PLUSS_CHECK(output != nullptr,
                    "Note::casefold: Unicode case folding failed");
        ::std::string folded(
            reinterpret_cast<const char*>(output), output_size);
        ::std::free(output);
        return folded;
      });
#else
  (void)locale;
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::casefold");
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    PLUSS_CHECK(
        ascii_only_(impl->bytes_at(i)),
        "Note::casefold: non-ASCII text requires a Unicode backend");
  }
  return lower(x);
#endif
}

Note regex_match(const Note& x, ::std::string_view pattern) {
  PLUSS_CHECK(x.defined(), "regex().match: receiver must be defined");
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "regex().match");
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_match(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return note_from_bool_storage(::std::move(out), x.sizes_view());
}

Note regex_search(const Note& x, ::std::string_view pattern) {
  PLUSS_CHECK(x.defined(), "regex().search: receiver must be defined");
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "regex().search");
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_search(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return note_from_bool_storage(::std::move(out), x.sizes_view());
}

Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl) {
  return regex_replace(x, pattern, repl, 0);
}

Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl,
                   ::std::int64_t flags) {
  if (!x.defined()) return Note::undefined();
  // flags: bit 0 (1) case-insensitive, bit 1 (2) multiline `^`/`$`.
  const ::std::regex re = make_findall_regex(pattern, flags);
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "regex().replace");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));

  for (::std::int64_t i=0;i<impl->numel();++i) {
    ::std::string out = ::std::regex_replace(::std::string(impl->bytes_at(i)), re, ::std::string(repl));
    spans.push_back(arena->append(out));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->dtype());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

TokenBatch split(const Note& x, ::std::string_view delim) {
  return split(x, delim, -1);
}

TokenBatch split_whitespace(const Note& x, ::std::int64_t limit) {
  return tokenbatch_from_note(x, "Note::split_whitespace", [limit](::std::string_view s, ::std::vector<::std::string>& out) {
    split_whitespace_with_limit_to(s, limit, out);
  });
}

TokenBatch split(const Note& x, ::std::string_view delim, ::std::int64_t limit) {
  return tokenbatch_from_note(x, "Note::split(delim,limit)", [delim, limit](::std::string_view s, ::std::vector<::std::string>& out) {
    split_delim_with_limit_to(s, delim, limit, out);
  });
}

TokenBatch split_lines(const Note& x, bool keep_ends) {
  return tokenbatch_from_note(x, "Note::split_lines", [keep_ends](::std::string_view s, ::std::vector<::std::string>& out) {
    split_lines_to(s, keep_ends, out);
  });
}

TokenBatch split_chars(const Note& x) {
  return tokenbatch_from_note(x, "Note::split_chars", [](::std::string_view s, ::std::vector<::std::string>& out) {
    split_chars_to(s, out);
  });
}

TokenBatch tokenize_words(const Note& x, ::std::int64_t flags) {
  return tokenbatch_from_note(x, "Note::tokenize_words", [flags](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_words_flagged_to(s, flags, out);
  });
}

TokenBatch tokenize_sentences(const Note& x) {
  return tokenbatch_from_note(x, "Note::tokenize_sentences", [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_sentences_to(s, out);
  });
}

TokenBatch tokenize_punct(const Note& x) {
  return tokenbatch_from_note(x, "Note::tokenize_punct", [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_punct_to(s, out);
  });
}

TokenBatch tokenize_regex(const Note& x, ::std::string_view pattern) {
  const ::std::regex re{::std::string(pattern)};
  return tokenbatch_from_note(x, "Note::tokenize_regex", [&re](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_regex_to(s, re, out);
  });
}

TokenBatch regex_findall(const Note& x, ::std::string_view pattern, ::std::int64_t flags) {
  const ::std::regex re = make_findall_regex(pattern, flags);
  return tokenbatch_from_note(x, "regex().findall", [&re](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_regex_to(s, re, out);
  });
}

TokenBatch regex_split(const Note& x, ::std::string_view pattern, ::std::int64_t limit) {
  const ::std::regex re{::std::string(pattern)};
  return tokenbatch_from_note(x, "regex().split", [&re, limit](::std::string_view s, ::std::vector<::std::string>& out) {
    regex_split_to(s, re, limit, out);
  });
}

TokenBatch token_feature(const Note& x, ::std::string_view op, ::std::int64_t iarg, ::std::string_view sarg) {
  return tokenbatch_from_note(x, "Note::token_feature", [op, iarg, sarg](::std::string_view s, ::std::vector<::std::string>& out) {
    token_feature_extract_to(s, op, iarg, sarg, out);
  });
}

TokenBatch intersect_acronyms(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::intersect_acronyms",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        set_intersection_to(a, b, TokenSetKind::Acronyms, out);
      });
}

TokenBatch intersect_citations(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::intersect_citations",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        set_intersection_to(a, b, TokenSetKind::Citations, out);
      });
}

TokenBatch intersect_math_symbols(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::intersect_math_symbols",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        set_intersection_to(a, b, TokenSetKind::MathSymbols, out);
      });
}

TokenBatch intersect_tokens(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::intersect_tokens",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        set_intersection_to(a, b, TokenSetKind::Tokens, out);
      });
}

TokenBatch intersect_terms(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::intersect_terms",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        set_intersection_to(a, b, TokenSetKind::Terms, out);
      });
}

Note jaccard_tokens(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::jaccard_tokens",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_jaccard_score(a, b, TokenSetKind::Tokens));
      });
}

Note jaccard_terms(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::jaccard_terms",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_jaccard_score(a, b, TokenSetKind::Terms));
      });
}

Note jaccard_acronyms(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::jaccard_acronyms",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_jaccard_score(a, b, TokenSetKind::Acronyms));
      });
}

Note jaccard_citations(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::jaccard_citations",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_jaccard_score(a, b, TokenSetKind::Citations));
      });
}

Note jaccard_math_symbols(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::jaccard_math_symbols",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_jaccard_score(a, b, TokenSetKind::MathSymbols));
      });
}

Note overlap_tokens(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::overlap_tokens",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_overlap_score(a, b, TokenSetKind::Tokens));
      });
}

Note overlap_terms(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::overlap_terms",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_overlap_score(a, b, TokenSetKind::Terms));
      });
}

Note overlap_acronyms(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::overlap_acronyms",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_overlap_score(a, b, TokenSetKind::Acronyms));
      });
}

Note overlap_citations(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::overlap_citations",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_overlap_score(a, b, TokenSetKind::Citations));
      });
}

Note overlap_math_symbols(const Note& x, const Note& other) {
  return transform_note_pair_to_float64(
      x, other, "Note::overlap_math_symbols",
      [](::std::string_view a, ::std::string_view b) {
        return clamp01(set_overlap_score(a, b, TokenSetKind::MathSymbols));
      });
}

TokenBatch diff_words(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::diff_words",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        diff_or_align_to(a, b, SeqGranularity::Words, false, out);
      });
}

TokenBatch diff_lines(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::diff_lines",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        diff_or_align_to(a, b, SeqGranularity::Lines, false, out);
      });
}

TokenBatch diff_chars(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::diff_chars",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        diff_or_align_to(a, b, SeqGranularity::Chars, false, out);
      });
}

TokenBatch align_tokens(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::align_tokens",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        diff_or_align_to(a, b, SeqGranularity::Words, true, out);
      });
}

TokenBatch align_sentences(const Note& x, const Note& other) {
  return tokenbatch_from_note_pair(
      x, other, "Note::align_sentences",
      [](::std::string_view a, ::std::string_view b, ::std::vector<::std::string>& out) {
        diff_or_align_to(a, b, SeqGranularity::Sentences, true, out);
      });
}

Note join(const Note& x, ::std::string_view sep) {
  if (!x.defined()) return Note::undefined();
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::join");
  ::std::string out;
  for (::std::int64_t i=0;i<impl->numel();++i) {
    if (i>0) out.append(sep.data(), sep.size());
    const auto s = impl->bytes_at(i);
    out.append(s.data(), s.size());
  }
  const ::std::array<::std::string_view, 1> flat{out};
  return note_from_utf8(flat, {});
}
} // namespace pluss::coden::ops_internal
