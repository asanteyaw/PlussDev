#include "CodeN/core/Errors.h"
#include "CodeN/native/cpu/TextKernels.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace pluss::coden::native::cpu {

namespace {

inline bool is_ascii_space(unsigned char c) {
  switch (c) {
    case ' ': case '\t': case '\n': case '\r': case '\f': case '\v':
      return true;
    default:
      return false;
  }
}

inline unsigned char to_lower_ascii_byte(unsigned char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c - 'A' + 'a') : c;
}

inline unsigned char to_upper_ascii_byte(unsigned char c) {
  return (c >= 'a' && c <= 'z') ? static_cast<unsigned char>(c - 'a' + 'A') : c;
}

inline bool is_ascii_alpha(unsigned char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_ascii_digit(unsigned char c) {
  return c >= '0' && c <= '9';
}

inline bool is_ascii_alnum(unsigned char c) {
  return is_ascii_alpha(c) || is_ascii_digit(c);
}

inline bool is_ascii_punct(unsigned char c) {
  return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126);
}

inline bool is_ascii_printable(unsigned char c) {
  return c >= 32 && c <= 126;
}

inline bool is_combining_diacritic(std::uint32_t cp) {
  return cp >= 0x0300 && cp <= 0x036F;
}

inline bool is_zero_width(std::uint32_t cp) {
  return cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0x2060 || cp == 0xFEFF;
}

inline bool is_emoji_codepoint(std::uint32_t cp) {
  return (cp >= 0x1F300 && cp <= 0x1FAFF) || (cp >= 0x2600 && cp <= 0x27BF) || cp == 0xFE0F;
}

inline bool decode_utf8(const std::string_view s, std::size_t& i, std::uint32_t& cp) {
  if (i >= s.size()) return false;
  const unsigned char b0 = static_cast<unsigned char>(s[i++]);
  if (b0 < 0x80) {
    cp = b0;
    return true;
  }

  auto need = [&](int k, std::uint32_t seed) -> bool {
    cp = seed;
    for (int j = 0; j < k; ++j) {
      if (i >= s.size()) return false;
      const unsigned char bx = static_cast<unsigned char>(s[i]);
      if ((bx & 0xC0) != 0x80) return false;
      cp = (cp << 6) | (bx & 0x3F);
      ++i;
    }
    return true;
  };

  if ((b0 & 0xE0) == 0xC0) return need(1, b0 & 0x1F);
  if ((b0 & 0xF0) == 0xE0) return need(2, b0 & 0x0F);
  if ((b0 & 0xF8) == 0xF0) return need(3, b0 & 0x07);
  cp = b0;
  return true;
}

inline std::array<bool, 256> make_char_mask(std::string_view chars) {
  std::array<bool, 256> mask{};
  mask.fill(false);
  for (unsigned char c : chars) {
    mask[c] = true;
  }
  return mask;
}

inline bool has_any_char(std::string_view s, std::string_view chars) {
  for (const char c : chars) {
    if (s.find(c) != std::string_view::npos) return true;
  }
  return false;
}

inline bool contains_ci(std::string_view hay, std::string_view needle) {
  if (needle.empty()) return true;
  auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
                        [](char a, char b) {
                          return std::tolower(static_cast<unsigned char>(a)) ==
                                 std::tolower(static_cast<unsigned char>(b));
                        });
  return it != hay.end();
}

inline std::string normalize_removed(std::string_view s) {
  return strip_ascii(collapse_whitespace_ascii(s));
}

inline std::string regex_replace_all(std::string_view s, const std::regex& re, std::string_view repl = " ") {
  return std::regex_replace(std::string(s), re, std::string(repl));
}

inline std::string remove_delimited(std::string_view s, char open, char close) {
  std::string out;
  out.reserve(s.size());
  int depth = 0;
  for (unsigned char c : s) {
    if (c == static_cast<unsigned char>(open)) {
      ++depth;
      continue;
    }
    if (c == static_cast<unsigned char>(close) && depth > 0) {
      --depth;
      continue;
    }
    if (depth == 0) out.push_back(static_cast<char>(c));
  }
  return out;
}

inline std::string transform_lines(std::string_view s, const std::function<bool(std::string_view, std::string&)>& fn) {
  std::string out;
  out.reserve(s.size());
  std::size_t start = 0;
  for (std::size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == '\n') {
      const std::string_view line = s.substr(start, i - start);
      std::string transformed;
      transformed.reserve(line.size());
      if (fn(line, transformed)) {
        out.append(transformed);
        if (i != s.size()) out.push_back('\n');
      }
      start = i + 1;
    }
  }
  return out;
}

} // namespace

::std::string lower_ascii(::std::string_view s) {
  ::std::string out(s.size(), '\0');
  auto* dst = reinterpret_cast<unsigned char*>(out.data());
  const auto* src = reinterpret_cast<const unsigned char*>(s.data());
  std::size_t i = 0;

#if defined(__ARM_NEON)
  const uint8x16_t A = vdupq_n_u8(static_cast<uint8_t>('A'));
  const uint8x16_t Z = vdupq_n_u8(static_cast<uint8_t>('Z'));
  const uint8x16_t add = vdupq_n_u8(static_cast<uint8_t>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const uint8x16_t v = vld1q_u8(src + i);
    const uint8x16_t geA = vcgeq_u8(v, A);
    const uint8x16_t leZ = vcleq_u8(v, Z);
    const uint8x16_t m = vandq_u8(geA, leZ);
    const uint8x16_t o = vaddq_u8(v, vandq_u8(m, add));
    vst1q_u8(dst + i, o);
  }
#elif defined(__SSE2__)
  const __m128i flip = _mm_set1_epi8(static_cast<char>(0x80));
  const __m128i one = _mm_set1_epi8(1);
  const __m128i A = _mm_set1_epi8(static_cast<char>('A' ^ 0x80));
  const __m128i Z = _mm_set1_epi8(static_cast<char>('Z' ^ 0x80));
  const __m128i add = _mm_set1_epi8(static_cast<char>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
    const __m128i vf = _mm_xor_si128(v, flip);
    const __m128i geA = _mm_cmpgt_epi8(vf, _mm_sub_epi8(A, one));
    const __m128i leZ = _mm_cmpgt_epi8(_mm_add_epi8(Z, one), vf);
    const __m128i m = _mm_and_si128(geA, leZ);
    const __m128i o = _mm_add_epi8(v, _mm_and_si128(m, add));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), o);
  }
#endif

  for (; i < s.size(); ++i) {
    dst[i] = to_lower_ascii_byte(src[i]);
  }
  return out;
}

::std::string upper_ascii(::std::string_view s) {
  ::std::string out(s.size(), '\0');
  auto* dst = reinterpret_cast<unsigned char*>(out.data());
  const auto* src = reinterpret_cast<const unsigned char*>(s.data());
  std::size_t i = 0;

#if defined(__ARM_NEON)
  const uint8x16_t a = vdupq_n_u8(static_cast<uint8_t>('a'));
  const uint8x16_t z = vdupq_n_u8(static_cast<uint8_t>('z'));
  const uint8x16_t sub = vdupq_n_u8(static_cast<uint8_t>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const uint8x16_t v = vld1q_u8(src + i);
    const uint8x16_t gea = vcgeq_u8(v, a);
    const uint8x16_t lez = vcleq_u8(v, z);
    const uint8x16_t m = vandq_u8(gea, lez);
    const uint8x16_t o = vsubq_u8(v, vandq_u8(m, sub));
    vst1q_u8(dst + i, o);
  }
#elif defined(__SSE2__)
  const __m128i flip = _mm_set1_epi8(static_cast<char>(0x80));
  const __m128i one = _mm_set1_epi8(1);
  const __m128i a = _mm_set1_epi8(static_cast<char>('a' ^ 0x80));
  const __m128i z = _mm_set1_epi8(static_cast<char>('z' ^ 0x80));
  const __m128i sub = _mm_set1_epi8(static_cast<char>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
    const __m128i vf = _mm_xor_si128(v, flip);
    const __m128i gea = _mm_cmpgt_epi8(vf, _mm_sub_epi8(a, one));
    const __m128i lez = _mm_cmpgt_epi8(_mm_add_epi8(z, one), vf);
    const __m128i m = _mm_and_si128(gea, lez);
    const __m128i o = _mm_sub_epi8(v, _mm_and_si128(m, sub));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), o);
  }
#endif

  for (; i < s.size(); ++i) {
    dst[i] = to_upper_ascii_byte(src[i]);
  }
  return out;
}

::std::string title_ascii(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  bool at_word_start = true;
  for (unsigned char c : s) {
    const bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool is_alnum = is_alpha || (c >= '0' && c <= '9');
    if (is_alpha) {
      out.push_back(static_cast<char>(at_word_start ? to_upper_ascii_byte(c) : to_lower_ascii_byte(c)));
    } else {
      out.push_back(static_cast<char>(c));
    }
    at_word_start = !is_alnum;
  }
  return out;
}

::std::string capitalize_ascii(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  bool first_alpha_done = false;
  for (unsigned char c : s) {
    if (!first_alpha_done && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
      out.push_back(static_cast<char>(to_upper_ascii_byte(c)));
      first_alpha_done = true;
    } else {
      out.push_back(static_cast<char>(to_lower_ascii_byte(c)));
    }
  }
  return out;
}

::std::string swapcase_ascii(::std::string_view s) {
  ::std::string out(s.size(), '\0');
  auto* dst = reinterpret_cast<unsigned char*>(out.data());
  const auto* src = reinterpret_cast<const unsigned char*>(s.data());
  std::size_t i = 0;

#if defined(__ARM_NEON)
  const uint8x16_t A = vdupq_n_u8(static_cast<uint8_t>('A'));
  const uint8x16_t Z = vdupq_n_u8(static_cast<uint8_t>('Z'));
  const uint8x16_t a = vdupq_n_u8(static_cast<uint8_t>('a'));
  const uint8x16_t z = vdupq_n_u8(static_cast<uint8_t>('z'));
  const uint8x16_t d = vdupq_n_u8(static_cast<uint8_t>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const uint8x16_t v = vld1q_u8(src + i);
    const uint8x16_t is_upper = vandq_u8(vcgeq_u8(v, A), vcleq_u8(v, Z));
    const uint8x16_t is_lower = vandq_u8(vcgeq_u8(v, a), vcleq_u8(v, z));
    const uint8x16_t to_lower = vandq_u8(is_upper, d);
    const uint8x16_t to_upper = vandq_u8(is_lower, d);
    const uint8x16_t o = vsubq_u8(vaddq_u8(v, to_lower), to_upper);
    vst1q_u8(dst + i, o);
  }
#elif defined(__SSE2__)
  const __m128i flip = _mm_set1_epi8(static_cast<char>(0x80));
  const __m128i one = _mm_set1_epi8(1);
  const __m128i A = _mm_set1_epi8(static_cast<char>('A' ^ 0x80));
  const __m128i Z = _mm_set1_epi8(static_cast<char>('Z' ^ 0x80));
  const __m128i a = _mm_set1_epi8(static_cast<char>('a' ^ 0x80));
  const __m128i z = _mm_set1_epi8(static_cast<char>('z' ^ 0x80));
  const __m128i d = _mm_set1_epi8(static_cast<char>(0x20));
  for (; i + 16 <= s.size(); i += 16) {
    const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
    const __m128i vf = _mm_xor_si128(v, flip);

    const __m128i geA = _mm_cmpgt_epi8(vf, _mm_sub_epi8(A, one));
    const __m128i leZ = _mm_cmpgt_epi8(_mm_add_epi8(Z, one), vf);
    const __m128i is_upper = _mm_and_si128(geA, leZ);

    const __m128i gea = _mm_cmpgt_epi8(vf, _mm_sub_epi8(a, one));
    const __m128i lez = _mm_cmpgt_epi8(_mm_add_epi8(z, one), vf);
    const __m128i is_lower = _mm_and_si128(gea, lez);

    const __m128i to_lower = _mm_and_si128(is_upper, d);
    const __m128i to_upper = _mm_and_si128(is_lower, d);
    const __m128i o = _mm_sub_epi8(_mm_add_epi8(v, to_lower), to_upper);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), o);
  }
#endif

  for (; i < s.size(); ++i) {
    const unsigned char c = src[i];
    if (c >= 'a' && c <= 'z') dst[i] = to_upper_ascii_byte(c);
    else if (c >= 'A' && c <= 'Z') dst[i] = to_lower_ascii_byte(c);
    else dst[i] = c;
  }
  return out;
}

::std::string lstrip_ascii(::std::string_view s) {
  ::std::size_t b = 0;
  while (b < s.size() && is_ascii_space(static_cast<unsigned char>(s[b]))) ++b;
  return ::std::string(s.substr(b));
}

::std::string rstrip_ascii(::std::string_view s) {
  ::std::size_t e = s.size();
  while (e > 0 && is_ascii_space(static_cast<unsigned char>(s[e - 1]))) --e;
  return ::std::string(s.substr(0, e));
}

::std::string strip_ascii(::std::string_view s) {
  ::std::size_t b = 0;
  ::std::size_t e = s.size();
  while (b < e && is_ascii_space(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && is_ascii_space(static_cast<unsigned char>(s[e - 1]))) --e;
  return ::std::string(s.substr(b, e - b));
}

::std::string lstrip_ascii(::std::string_view s, ::std::string_view chars) {
  if (chars.empty()) return lstrip_ascii(s);
  const auto mask = make_char_mask(chars);
  ::std::size_t b = 0;
  while (b < s.size() && mask[static_cast<unsigned char>(s[b])]) ++b;
  return ::std::string(s.substr(b));
}

::std::string rstrip_ascii(::std::string_view s, ::std::string_view chars) {
  if (chars.empty()) return rstrip_ascii(s);
  const auto mask = make_char_mask(chars);
  ::std::size_t e = s.size();
  while (e > 0 && mask[static_cast<unsigned char>(s[e - 1])]) --e;
  return ::std::string(s.substr(0, e));
}

::std::string strip_ascii(::std::string_view s, ::std::string_view chars) {
  if (chars.empty()) return strip_ascii(s);
  const auto mask = make_char_mask(chars);
  ::std::size_t b = 0;
  ::std::size_t e = s.size();
  while (b < e && mask[static_cast<unsigned char>(s[b])]) ++b;
  while (e > b && mask[static_cast<unsigned char>(s[e - 1])]) --e;
  return ::std::string(s.substr(b, e - b));
}

::std::string collapse_whitespace_ascii(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  bool in_ws = false;
  bool emitted_non_ws = false;
  for (unsigned char c : s) {
    if (is_ascii_space(c)) {
      in_ws = emitted_non_ws;
    } else {
      if (in_ws) {
        out.push_back(' ');
        in_ws = false;
      }
      out.push_back(static_cast<char>(c));
      emitted_non_ws = true;
    }
  }
  return out;
}

::std::string normalize_spaces_ascii(::std::string_view s) {
  return collapse_whitespace_ascii(s);
}

::std::string dedent_ascii(::std::string_view s) {
  std::vector<std::pair<std::size_t, std::size_t>> lines;
  lines.reserve(16);
  std::size_t start = 0;
  for (std::size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == '\n') {
      lines.emplace_back(start, i - start);
      start = i + 1;
    }
  }

  std::size_t min_indent = static_cast<std::size_t>(-1);
  for (const auto& [off, len] : lines) {
    std::size_t i = 0;
    while (i < len && (s[off + i] == ' ' || s[off + i] == '\t')) ++i;
    if (i == len) continue;
    min_indent = std::min(min_indent, i);
  }
  if (min_indent == static_cast<std::size_t>(-1) || min_indent == 0) {
    return std::string(s);
  }

  std::string out;
  out.reserve(s.size());
  for (std::size_t li = 0; li < lines.size(); ++li) {
    const auto [off, len] = lines[li];
    std::size_t drop = 0;
    while (drop < len && drop < min_indent && (s[off + drop] == ' ' || s[off + drop] == '\t')) ++drop;
    out.append(s.substr(off + drop, len - drop));
    if (li + 1 < lines.size()) out.push_back('\n');
  }
  return out;
}

::std::string indent_ascii(::std::string_view s, ::std::int64_t spaces) {
  const auto n_spaces = spaces <= 0 ? 0 : static_cast<std::size_t>(spaces);
  if (n_spaces == 0) return std::string(s);

  std::string pad(n_spaces, ' ');
  std::string out;
  out.reserve(s.size() + pad.size() * 4);
  out += pad;
  for (char c : s) {
    out.push_back(c);
    if (c == '\n') out += pad;
  }
  return out;
}

::std::string remove_control_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if ((c >= 0x20 && c != 0x7F) || c == '\n' || c == '\t' || c == '\r') {
      out.push_back(static_cast<char>(c));
    }
  }
  return out;
}

::std::string remove_non_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (c < 0x80) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string to_ascii_lossy(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    out.push_back(static_cast<char>(c < 0x80 ? c : '?'));
  }
  return out;
}

::std::string remove_punctuation_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (!is_ascii_punct(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string remove_digits_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (!is_ascii_digit(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string remove_letters_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (!is_ascii_alpha(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string remove_symbols_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (is_ascii_alnum(c) || is_ascii_space(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string remove_emojis_utf8(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  std::size_t i = 0;
  while (i < s.size()) {
    const auto start = i;
    std::uint32_t cp = 0;
    if (!decode_utf8(s, i, cp)) break;
    if (cp < 0x80) {
      out.push_back(static_cast<char>(cp));
      continue;
    }
    if (!is_emoji_codepoint(cp)) {
      out.append(s.substr(start, i - start));
    }
  }
  return out;
}

::std::string keep_alnum_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (is_ascii_alnum(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string keep_alpha_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (is_ascii_alpha(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string keep_digits_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (is_ascii_digit(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string keep_ascii(::std::string_view s) {
  return remove_non_ascii(s);
}

::std::string keep_printable_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (is_ascii_printable(c)) out.push_back(static_cast<char>(c));
  }
  return out;
}

::std::string normalize_quotes_utf8(::std::string_view s) {
  std::string out(s);
  out = replace_bytes(out, "\xE2\x80\x98", "'");
  out = replace_bytes(out, "\xE2\x80\x99", "'");
  out = replace_bytes(out, "\xE2\x80\x9A", "'");
  out = replace_bytes(out, "\xE2\x80\x9B", "'");
  out = replace_bytes(out, "\xE2\x80\x9C", "\"");
  out = replace_bytes(out, "\xE2\x80\x9D", "\"");
  out = replace_bytes(out, "\xE2\x80\x9E", "\"");
  out = replace_bytes(out, "\xE2\x80\x9F", "\"");
  return out;
}

::std::string normalize_dashes_utf8(::std::string_view s) {
  std::string out(s);
  out = replace_bytes(out, "\xE2\x80\x90", "-");
  out = replace_bytes(out, "\xE2\x80\x91", "-");
  out = replace_bytes(out, "\xE2\x80\x92", "-");
  out = replace_bytes(out, "\xE2\x80\x93", "-");
  out = replace_bytes(out, "\xE2\x80\x94", "-");
  out = replace_bytes(out, "\xE2\x80\x95", "-");
  out = replace_bytes(out, "\xE2\x88\x92", "-");
  return out;
}

::std::string normalize_ellipsis_utf8(::std::string_view s) {
  return replace_bytes(s, "\xE2\x80\xA6", "...");
}

::std::string strip_accents_utf8(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  std::size_t i = 0;
  while (i < s.size()) {
    const auto start = i;
    std::uint32_t cp = 0;
    if (!decode_utf8(s, i, cp)) break;
    if (cp < 0x80) {
      out.push_back(static_cast<char>(cp));
      continue;
    }
    if (is_combining_diacritic(cp)) continue;

    switch (cp) {
      case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: case 0x00C4: case 0x00C5: out.push_back('A'); continue;
      case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4: case 0x00E5: out.push_back('a'); continue;
      case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB: out.push_back('E'); continue;
      case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: out.push_back('e'); continue;
      case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF: out.push_back('I'); continue;
      case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: out.push_back('i'); continue;
      case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: case 0x00D6: case 0x00D8: out.push_back('O'); continue;
      case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6: case 0x00F8: out.push_back('o'); continue;
      case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC: out.push_back('U'); continue;
      case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: out.push_back('u'); continue;
      case 0x00DD: case 0x0178: out.push_back('Y'); continue;
      case 0x00FD: case 0x00FF: out.push_back('y'); continue;
      case 0x00C7: out.push_back('C'); continue;
      case 0x00E7: out.push_back('c'); continue;
      case 0x00D1: out.push_back('N'); continue;
      case 0x00F1: out.push_back('n'); continue;
      case 0x00C6: out.append("AE"); continue;
      case 0x00E6: out.append("ae"); continue;
      case 0x0152: out.append("OE"); continue;
      case 0x0153: out.append("oe"); continue;
      default: break;
    }

    out.append(s.substr(start, i - start));
  }
  return out;
}

::std::string strip_diacritics_utf8(::std::string_view s) {
  return strip_accents_utf8(s);
}

::std::string strip_zero_width_utf8(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  std::size_t i = 0;
  while (i < s.size()) {
    const auto start = i;
    std::uint32_t cp = 0;
    if (!decode_utf8(s, i, cp)) break;
    if (cp < 0x80) {
      out.push_back(static_cast<char>(cp));
      continue;
    }
    if (is_zero_width(cp)) continue;
    out.append(s.substr(start, i - start));
  }
  return out;
}

::std::string strip_bom_utf8(::std::string_view s) {
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF) {
    return std::string(s.substr(3));
  }
  return std::string(s);
}

::std::string collapse_repeats_ascii(::std::string_view s) {
  if (s.empty()) return {};
  std::string out;
  out.reserve(s.size());
  unsigned char prev = static_cast<unsigned char>(s[0]);
  out.push_back(static_cast<char>(prev));
  for (std::size_t i = 1; i < s.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c != prev) {
      out.push_back(static_cast<char>(c));
      prev = c;
    }
  }
  return out;
}

::std::string collapse_punct_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  bool in_punct = false;
  for (unsigned char c : s) {
    if (is_ascii_punct(c)) {
      if (!in_punct) out.push_back(static_cast<char>(c));
      in_punct = true;
    } else {
      out.push_back(static_cast<char>(c));
      in_punct = false;
    }
  }
  return out;
}

::std::string collapse_spaces_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  bool in_space = false;
  for (unsigned char c : s) {
    if (c == ' ' || c == '\t') {
      if (!in_space) out.push_back(' ');
      in_space = true;
    } else {
      out.push_back(static_cast<char>(c));
      in_space = false;
    }
  }
  return out;
}

::std::string collapse_newlines_ascii(::std::string_view s) {
  std::string out;
  out.reserve(s.size());
  bool in_nl = false;
  for (unsigned char c : s) {
    if (c == '\n' || c == '\r') {
      if (!in_nl) out.push_back('\n');
      in_nl = true;
    } else {
      out.push_back(static_cast<char>(c));
      in_nl = false;
    }
  }
  return out;
}

::std::string nlp_remove_urls_fast(::std::string_view s) {
  if (!contains_ci(s, "http") && !contains_ci(s, "www.")) return std::string(s);
  static const std::regex kUrlRe(R"((https?:\/\/[^\s<>"')\]]+|www\.[^\s<>"')\]]+))",
                                 std::regex::icase);
  return normalize_removed(regex_replace_all(s, kUrlRe));
}

::std::string nlp_remove_emails_fast(::std::string_view s) {
  if (s.find('@') == std::string_view::npos) return std::string(s);
  static const std::regex kEmailRe(R"(([A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}))");
  return normalize_removed(regex_replace_all(s, kEmailRe));
}

::std::string nlp_remove_mentions_fast(::std::string_view s) {
  if (s.find('@') == std::string_view::npos) return std::string(s);
  static const std::regex kMentionRe(R"((^|[\s(])@[A-Za-z0-9_]+)", std::regex::icase);
  return normalize_removed(regex_replace_all(s, kMentionRe, "$1"));
}

::std::string nlp_remove_hashtags_fast(::std::string_view s) {
  if (s.find('#') == std::string_view::npos) return std::string(s);
  static const std::regex kHashRe(R"((^|[\s(])#[A-Za-z0-9_]+)", std::regex::icase);
  return normalize_removed(regex_replace_all(s, kHashRe, "$1"));
}

::std::string nlp_remove_html_fast(::std::string_view s) {
  if (s.find('<') == std::string_view::npos || s.find('>') == std::string_view::npos) return std::string(s);
  static const std::regex kHtmlComment(R"(<!--[\s\S]*?-->)", std::regex::icase);
  static const std::regex kHtmlTag(R"(<[^>]+>)", std::regex::icase);
  std::string out = regex_replace_all(s, kHtmlComment);
  out = regex_replace_all(out, kHtmlTag);
  return normalize_removed(out);
}

::std::string nlp_remove_markdown_fast(::std::string_view s) {
  if (!has_any_char(s, "*_`[]()!#>|")) return std::string(s);
  static const std::regex kFencedCode(R"rgx((```[\s\S]*?```|~~~[\s\S]*?~~~))rgx");
  static const std::regex kInlineCode(R"rgx(`[^`\n]*`)rgx");
  static const std::regex kImages(R"(!\[[^\]]*\]\([^)]+\))");
  static const std::regex kLinks(R"(\[([^\]]+)\]\([^)]+\))");
  static const std::regex kAutolink(R"(<https?:\/\/[^>]+>)", std::regex::icase);
  static const std::regex kBlockQuote(R"((^|\n)\s*>\s*)");
  static const std::regex kHeading(R"((^|\n)\s*#{1,6}\s*)");
  std::string out = regex_replace_all(s, kFencedCode);
  out = regex_replace_all(out, kInlineCode);
  out = regex_replace_all(out, kImages);
  out = std::regex_replace(out, kLinks, "$1");
  out = regex_replace_all(out, kAutolink);
  out = regex_replace_all(out, kBlockQuote, "$1");
  out = regex_replace_all(out, kHeading, "$1");
  out = replace_bytes(out, "**", "");
  out = replace_bytes(out, "__", "");
  out = replace_bytes(out, "*", "");
  out = replace_bytes(out, "_", "");
  out = replace_bytes(out, "~~", "");
  return normalize_removed(out);
}

::std::string nlp_remove_code_fast(::std::string_view s) {
  if (!has_any_char(s, "`\t")) {
    bool has_indent = false;
    for (std::size_t i = 0; i + 3 < s.size(); ++i) {
      if (s[i] == '\n' && s[i + 1] == ' ' && s[i + 2] == ' ' && s[i + 3] == ' ') {
        has_indent = true;
        break;
      }
    }
    if (!has_indent) return std::string(s);
  }
  static const std::regex kFencedCode(R"rgx((```[\s\S]*?```|~~~[\s\S]*?~~~))rgx");
  static const std::regex kInlineCode(R"rgx(`[^`\n]*`)rgx");
  std::string out = regex_replace_all(s, kFencedCode);
  out = regex_replace_all(out, kInlineCode);

  out = transform_lines(out, [](std::string_view line, std::string& transformed) {
    std::size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;
    if (i >= 4 || (!line.empty() && line[0] == '\t')) return false;
    transformed.assign(line);
    return true;
  });
  return normalize_removed(out);
}

::std::string nlp_remove_math_fast(::std::string_view s) {
  if (!has_any_char(s, "$\\")) return std::string(s);
  static const std::regex kDisplayMath(R"(\$\$[\s\S]*?\$\$)");
  static const std::regex kInlineMath(R"(\$[^$\n]+\$)");
  static const std::regex kParenMath(R"(\\\([\s\S]*?\\\))");
  static const std::regex kBracketMath(R"(\\\[[\s\S]*?\\\])");
  std::string out = regex_replace_all(s, kDisplayMath);
  out = regex_replace_all(out, kInlineMath);
  out = regex_replace_all(out, kParenMath);
  out = regex_replace_all(out, kBracketMath);
  return normalize_removed(out);
}

::std::string nlp_remove_latex_fast(::std::string_view s) {
  if (!has_any_char(s, "\\{}$")) return std::string(s);
  static const std::regex kEnv(R"(\\(begin|end)\{[^}]+\})");
  static const std::regex kCmd(R"(\\[A-Za-z]+(?:\*?)\s*(?:\[[^\]]*\])?)");
  std::string out = nlp_remove_math_fast(s);
  out = regex_replace_all(out, kEnv);
  out = regex_replace_all(out, kCmd);
  out = replace_bytes(out, "{", " ");
  out = replace_bytes(out, "}", " ");
  out = replace_bytes(out, "\\", " ");
  return normalize_removed(out);
}

::std::string nlp_remove_brackets_fast(::std::string_view s) {
  if (!has_any_char(s, "[](){}")) return std::string(s);
  std::string out = remove_delimited(s, '(', ')');
  out = remove_delimited(out, '[', ']');
  out = remove_delimited(out, '{', '}');
  return normalize_removed(out);
}

::std::string nlp_remove_parentheticals_fast(::std::string_view s) {
  if (s.find('(') == std::string_view::npos || s.find(')') == std::string_view::npos) return std::string(s);
  return normalize_removed(remove_delimited(s, '(', ')'));
}

::std::string nlp_remove_quotes_fast(::std::string_view s) {
  if (!has_any_char(s, "'\"")) {
    if (!contains_ci(s, "\xE2\x80\x98") && !contains_ci(s, "\xE2\x80\x99") &&
        !contains_ci(s, "\xE2\x80\x9C") && !contains_ci(s, "\xE2\x80\x9D")) {
      return std::string(s);
    }
  }

  const std::string normalized = normalize_quotes_utf8(s);
  std::string out;
  out.reserve(normalized.size());

  bool in_single = false;
  bool in_double = false;
  for (std::size_t i = 0; i < normalized.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(normalized[i]);
    if (in_double) {
      if (c == '"' && (i == 0 || normalized[i - 1] != '\\')) in_double = false;
      continue;
    }
    if (in_single) {
      if (c == '\'' && (i == 0 || normalized[i - 1] != '\\')) in_single = false;
      continue;
    }

    if (c == '"') {
      in_double = true;
      continue;
    }
    if (c == '\'') {
      const bool apostrophe =
          i > 0 && i + 1 < normalized.size() &&
          std::isalnum(static_cast<unsigned char>(normalized[i - 1])) &&
          std::isalnum(static_cast<unsigned char>(normalized[i + 1]));
      if (apostrophe) out.push_back('\'');
      else in_single = true;
      continue;
    }
    out.push_back(static_cast<char>(c));
  }
  return normalize_removed(out);
}

::std::string nlp_remove_bullets_fast(::std::string_view s) {
  if (!has_any_char(s, "-*+.\n")) return std::string(s);
  std::string out = transform_lines(s, [](std::string_view line, std::string& transformed) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;

    std::size_t start = i;
    if (i < line.size() &&
        (line[i] == '-' || line[i] == '*' || line[i] == '+' ||
         static_cast<unsigned char>(line[i]) == 0xE2)) {
      // Also catches UTF-8 bullet ('•') by leading byte check.
      ++i;
      while (i < line.size() && line[i] == ' ') ++i;
      start = i;
    } else {
      std::size_t j = i;
      while (j < line.size() && std::isdigit(static_cast<unsigned char>(line[j]))) ++j;
      if (j > i && j + 1 < line.size() && (line[j] == '.' || line[j] == ')') && line[j + 1] == ' ') {
        start = j + 2;
      }
    }
    transformed.assign(line.substr(start));
    return true;
  });
  return normalize_removed(out);
}

::std::string nlp_remove_headers_fast(::std::string_view s) {
  if (!has_any_char(s, "#:\n")) return std::string(s);
  std::string out = transform_lines(s, [](std::string_view line, std::string& transformed) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i < line.size() && line[i] == '#') return false; // markdown heading
    transformed.assign(line);
    return true;
  });
  return normalize_removed(out);
}

::std::string nlp_remove_footers_fast(::std::string_view s) {
  if (!contains_ci(s, "page") && !contains_ci(s, "copyright") && !contains_ci(s, "all rights reserved")) {
    return std::string(s);
  }

  static const std::regex kFooterPage(R"(^\s*page\s+\d+(\s+of\s+\d+)?\s*$)", std::regex::icase);
  static const std::regex kFooterCopy(R"(^\s*(copyright|all rights reserved|confidential)\b.*$)", std::regex::icase);

  std::string out = transform_lines(s, [&](std::string_view line, std::string& transformed) {
    const std::string l(line);
    if (std::regex_match(l, kFooterPage)) return false;
    if (std::regex_match(l, kFooterCopy)) return false;
    transformed = l;
    return true;
  });
  return normalize_removed(out);
}

::std::string nlp_remove_boilerplate_fast(::std::string_view s) {
  if (!contains_ci(s, "copyright") && !contains_ci(s, "privacy") &&
      !contains_ci(s, "terms") && !contains_ci(s, "disclaimer") &&
      !contains_ci(s, "all rights reserved")) {
    return std::string(s);
  }

  std::string out = transform_lines(s, [](std::string_view line, std::string& transformed) {
    const std::string lower = lower_ascii(line);
    if (contains_ci(lower, "copyright")) return false;
    if (contains_ci(lower, "all rights reserved")) return false;
    if (contains_ci(lower, "privacy policy")) return false;
    if (contains_ci(lower, "terms of service")) return false;
    if (contains_ci(lower, "disclaimer")) return false;
    transformed.assign(line);
    return true;
  });
  return normalize_removed(out);
}

::std::string nlp_remove_tables_fast(::std::string_view s) {
  if (!has_any_char(s, "|\t+-")) return std::string(s);
  std::string out = transform_lines(s, [](std::string_view line, std::string& transformed) {
    std::size_t pipes = 0;
    std::size_t tabs = 0;
    std::size_t dashes = 0;
    for (const unsigned char c : line) {
      if (c == '|') ++pipes;
      if (c == '\t') ++tabs;
      if (c == '-' || c == '+' || c == '=') ++dashes;
    }
    if (pipes >= 2) return false;
    if (tabs >= 2) return false;
    if (line.size() >= 3 && dashes + pipes + tabs == line.size()) return false;
    transformed.assign(line);
    return true;
  });
  return normalize_removed(out);
}

bool contains_bytes(::std::string_view hay, ::std::string_view needle) {
  if (needle.empty()) return true;
  return hay.find(needle) != ::std::string_view::npos;
}

::std::string replace_bytes(::std::string_view s, ::std::string_view from, ::std::string_view to) {
  if (from.empty()) return ::std::string(s);
  ::std::string out;
  ::std::size_t pos = 0;
  while (true) {
    ::std::size_t p = s.find(from, pos);
    if (p == ::std::string_view::npos) {
      out.append(s.substr(pos));
      break;
    }
    out.append(s.substr(pos, p - pos));
    out.append(to);
    pos = p + from.size();
  }
  return out;
}

::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_whitespace_spans(::std::string_view s) {
  ::std::vector<::std::pair<::std::size_t, ::std::size_t>> out;
  const ::std::size_t n = s.size();
  ::std::size_t i = 0;

  auto is_ws = [](unsigned char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  };

  while (i < n) {
    while (i < n && is_ws(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= n) break;
    const ::std::size_t start = i;
    while (i < n && !is_ws(static_cast<unsigned char>(s[i]))) ++i;
    out.emplace_back(start, i - start);
  }
  return out;
}

} // namespace pluss::coden::native::cpu

::std::vector<::std::pair<::std::size_t, ::std::size_t>>
pluss::coden::native::cpu::split_delim_spans(::std::string_view s, ::std::string_view delim) {
  ::std::vector<::std::pair<::std::size_t, ::std::size_t>> out;
  PLUSS_CHECK(!delim.empty(), "split_delim_spans: delimiter must be non-empty");

  ::std::size_t pos = 0;
  while (pos <= s.size()) {
    const ::std::size_t p = s.find(delim, pos);
    if (p == ::std::string_view::npos) {
      out.emplace_back(pos, s.size() - pos);
      break;
    }
    out.emplace_back(pos, p - pos);
    pos = p + delim.size();
  }
  return out;
}
