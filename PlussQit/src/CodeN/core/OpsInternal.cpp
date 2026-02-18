#include "CodeN/core/OpsInternal.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <string>
#include <utility>
#include <regex>

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
  ::std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && !::std::isalnum(static_cast<unsigned char>(s[i])) && s[i] != '_') ++i;
    if (i >= s.size()) break;
    const ::std::size_t start = i;
    while (i < s.size() && (::std::isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_' || s[i] == '\'')) ++i;
    out.emplace_back(s.substr(start, i - start));
  }
}

static void tokenize_sentences_to(::std::string_view s, ::std::vector<::std::string>& out) {
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

template <class TokenizeFn>
static TokenBatch tokenbatch_from_chord(const Chord& x, TokenizeFn&& fn) {
  if (!x.defined()) return TokenBatchFactoryAccess::make(Note::undefined(), {0,0});
  ::std::vector<::std::string> tokens;
  fn(x.utf8(), tokens);
  ::std::vector<::std::string_view> views;
  views.reserve(tokens.size());
  for (const auto& tok : tokens) views.push_back(tok);
  const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(tokens.size())};
  Note token_note = note_from_utf8(views, ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  return TokenBatchFactoryAccess::make(::std::move(token_note), {0, sizes[0]});
}

template <class TokenizeFn>
static TokenBatch tokenbatch_from_note(const Note& x, const char* op, TokenizeFn&& fn) {
  if (!x.defined()) return TokenBatchFactoryAccess::make(Note::undefined(), {0});
  const auto& impl = NoteAccess::impl(x);
  PLUSS_CHECK(!impl->is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (not bytes)");
  const ::std::int64_t n = impl->numel();
  ::std::vector<::std::int64_t> offsets(static_cast<::std::size_t>(n) + 1, 0);
  ::std::vector<::std::string> flat_tokens;
  for (::std::int64_t i = 0; i < n; ++i) {
    fn(impl->bytes_at(i), flat_tokens);
    offsets[static_cast<::std::size_t>(i) + 1] = static_cast<::std::int64_t>(flat_tokens.size());
  }
  ::std::vector<::std::string_view> views;
  views.reserve(flat_tokens.size());
  for (const auto& tok : flat_tokens) views.push_back(tok);
  const ::std::array<::std::int64_t, 1> sizes{static_cast<::std::int64_t>(flat_tokens.size())};
  Note token_note = note_from_utf8(views, ::std::span<const ::std::int64_t>(sizes.data(), sizes.size()));
  return TokenBatchFactoryAccess::make(::std::move(token_note), ::std::move(offsets));
}

static ::std::string tex_strip_comments_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size());
  for (::std::size_t i = 0; i < s.size();) {
    if (s[i] == '%') {
      ::std::size_t bs = 0;
      for (::std::size_t j = i; j > 0 && s[j - 1] == '\\'; --j) ++bs;
      if ((bs % 2) == 0) {
        while (i < s.size() && s[i] != '\n') ++i;
        continue;
      }
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

static ::std::string tex_strip_preamble_text(::std::string_view s) {
  constexpr ::std::string_view kBegin = "\\begin{document}";
  constexpr ::std::string_view kEnd = "\\end{document}";
  ::std::size_t start = 0;
  if (const auto p = s.find(kBegin); p != ::std::string_view::npos) start = p + kBegin.size();
  ::std::size_t end = s.size();
  if (const auto q = s.find(kEnd, start); q != ::std::string_view::npos) end = q;
  return ::pluss::coden::native::cpu::strip_ascii(s.substr(start, end - start));
}

static ::std::string tex_remove_commands_text(::std::string_view s) {
  static const ::std::regex kCmdWithArgs(R"(\\[A-Za-z@]+[*]?(?:\s*\[[^\]]*\])?(?:\s*\{[^{}]*\})?)");
  static const ::std::regex kEscapedChar(R"(\\.)");
  ::std::string out = ::std::regex_replace(::std::string(s), kCmdWithArgs, " ");
  out = ::std::regex_replace(out, kEscapedChar, " ");
  return out;
}

static ::std::string tex_to_text_text(::std::string_view s, ::std::int64_t mode) {
  (void)mode;
  ::std::string out = tex_strip_comments_text(s);
  out = tex_strip_preamble_text(out);
  out = ::pluss::coden::native::cpu::nlp_remove_math_fast(out);
  out = tex_remove_commands_text(out);
  out = ::pluss::coden::native::cpu::replace_bytes(out, "{", " ");
  out = ::pluss::coden::native::cpu::replace_bytes(out, "}", " ");
  out = ::pluss::coden::native::cpu::collapse_whitespace_ascii(out);
  return ::pluss::coden::native::cpu::strip_ascii(out);
}

static ::std::string tex_sanitize_text(::std::string_view s, ::std::int64_t level) {
  ::std::string out = tex_to_text_text(s, level);
  if (level > 0) out = ::pluss::coden::native::cpu::lower_ascii(out);
  return out;
}

static ::std::string normalize_tex_removed(::std::string_view s) {
  return ::pluss::coden::native::cpu::strip_ascii(
      ::pluss::coden::native::cpu::collapse_whitespace_ascii(s));
}

static ::std::string regex_escape_text(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() * 2);
  for (const char c : s) {
    if (::std::isalnum(static_cast<unsigned char>(c)) || c == '_') out.push_back(c);
    else {
      out.push_back('\\');
      out.push_back(c);
    }
  }
  return out;
}

static ::std::string tex_remove_environment_text(::std::string_view s, ::std::string_view name) {
  const ::std::string esc = regex_escape_text(name);
  const ::std::regex re("\\\\begin\\{" + esc + "\\}[\\s\\S]*?\\\\end\\{" + esc + "\\}");
  return normalize_tex_removed(::std::regex_replace(::std::string(s), re, " "));
}

static ::std::string tex_remove_environments_text(::std::string_view s) {
  static const ::std::regex re(R"(\\begin\{([A-Za-z*@]+)\}[\s\S]*?\\end\{\1\})");
  return normalize_tex_removed(::std::regex_replace(::std::string(s), re, " "));
}

static ::std::string tex_strip_metadata_text(::std::string_view s) {
  static const ::std::regex meta_cmd(R"(\\(title|author|date|thanks)\s*\{[^{}]*\})");
  static const ::std::regex maketitle_cmd(R"(\\maketitle\b)");
  ::std::string out = ::std::regex_replace(::std::string(s), meta_cmd, " ");
  out = ::std::regex_replace(out, maketitle_cmd, " ");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_labels_text(::std::string_view s) {
  static const ::std::regex re(R"(\\label\s*\{[^{}]*\})");
  return normalize_tex_removed(::std::regex_replace(::std::string(s), re, " "));
}

static ::std::string tex_remove_refs_text(::std::string_view s) {
  static const ::std::regex re(R"(\\(ref|eqref|autoref|pageref|nameref)\s*\{[^{}]*\})");
  return normalize_tex_removed(::std::regex_replace(::std::string(s), re, " "));
}

static ::std::string tex_remove_citations_text(::std::string_view s) {
  static const ::std::regex re(R"(\\cite[a-zA-Z*]*\s*(?:\[[^\]]*\])?\s*\{[^{}]*\})");
  return normalize_tex_removed(::std::regex_replace(::std::string(s), re, " "));
}

static ::std::string tex_remove_graphics_text(::std::string_view s) {
  static const ::std::regex cmd(R"(\\includegraphics\s*(?:\[[^\]]*\])?\s*\{[^{}]*\})");
  ::std::string out = ::std::regex_replace(::std::string(s), cmd, " ");
  out = tex_remove_environment_text(out, "figure");
  out = tex_remove_environment_text(out, "figure*");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_tables_text(::std::string_view s) {
  ::std::string out(s);
  out = tex_remove_environment_text(out, "table");
  out = tex_remove_environment_text(out, "table*");
  out = tex_remove_environment_text(out, "tabular");
  out = tex_remove_environment_text(out, "tabular*");
  out = tex_remove_environment_text(out, "longtable");
  out = tex_remove_environment_text(out, "array");
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
  static const ::std::regex usepkg(R"(\\(usepackage|RequirePackage)\s*(?:\[[^\]]*\])?\s*\{[^{}]*\})");
  static const ::std::regex docclass(R"(\\documentclass\s*(?:\[[^\]]*\])?\s*\{[^{}]*\})");
  ::std::string out = ::std::regex_replace(::std::string(s), usepkg, " ");
  out = ::std::regex_replace(out, docclass, " ");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_bibliography_text(::std::string_view s) {
  static const ::std::regex bibcmd(R"(\\(bibliography|addbibresource|printbibliography)\s*(?:\[[^\]]*\])?\s*(?:\{[^{}]*\})?)");
  ::std::string out = ::std::regex_replace(::std::string(s), bibcmd, " ");
  out = tex_remove_environment_text(out, "thebibliography");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_hyperref_text(::std::string_view s) {
  static const ::std::regex href(R"(\\href\s*\{[^{}]*\}\s*\{([^{}]*)\})");
  static const ::std::regex hyperref(R"(\\hyperref\s*\[[^\]]*\]\s*\{([^{}]*)\})");
  static const ::std::regex url(R"(\\url\s*\{[^{}]*\})");
  ::std::string out = ::std::regex_replace(::std::string(s), href, "$1");
  out = ::std::regex_replace(out, hyperref, "$1");
  out = ::std::regex_replace(out, url, " ");
  return normalize_tex_removed(out);
}

static ::std::string tex_remove_urls_text(::std::string_view s) {
  return tex_remove_hyperref_text(s);
}

static ::std::string tex_remove_color_text(::std::string_view s) {
  static const ::std::regex textcolor(R"(\\textcolor\s*\{[^{}]*\}\s*\{([^{}]*)\})");
  static const ::std::regex colorbox(R"(\\(colorbox|fcolorbox)\s*\{[^{}]*\}(?:\s*\{[^{}]*\})?\s*\{([^{}]*)\})");
  static const ::std::regex colorcmd(R"(\\(color|pagecolor)\s*\{[^{}]*\})");
  ::std::string out = ::std::regex_replace(::std::string(s), textcolor, "$1");
  out = ::std::regex_replace(out, colorbox, "$2");
  out = ::std::regex_replace(out, colorcmd, " ");
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
  static const ::std::regex math_re(R"(\$\$[\s\S]*?\$\$|\$[^$\n]+\$|\\\([\s\S]*?\\\)|\\\[[\s\S]*?\\\])");
  ::std::string owned(s);
  ::std::string out;
  for (::std::sregex_iterator it(owned.begin(), owned.end(), math_re), end; it != end; ++it) {
    ::std::string m = strip_math_delims((*it).str());
    m = ::pluss::coden::native::cpu::strip_ascii(m);
    if (m.empty()) continue;
    if (!out.empty()) out.push_back(' ');
    out.append(m);
  }
  return normalize_tex_removed(out);
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

static void check_dim(const Note& x, ::std::int64_t d) {
  PLUSS_CHECK(x.defined(), "Note op on undefined Note");
  const auto dim = x.dim();
  PLUSS_CHECK(d >= 0 && d < dim, "dim out of range");
}

} // namespace

// Internal construction hooks.
struct ChordFactoryAccess {
  static Chord wrap(::pluss::flame::intrusive_ptr<ChordImpl> impl) {
    return Chord(::std::move(impl));
  }
};

struct NoteFactoryAccess {
  static Note wrap(::pluss::flame::intrusive_ptr<NoteImpl> impl) {
    return Note(::std::move(impl));
  }
};

Chord chord_from_utf8(::std::string_view utf8) {
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  auto [off, len] = arena->append(utf8);
  auto impl = ::pluss::flame::make_intrusive<ChordImpl>(arena, off, len);
  return ChordFactoryAccess::wrap(::std::move(impl));
}

Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  ::std::array<::std::int64_t, 3> sz{1,1,1};
  for (::std::size_t i = 0; i < sizes.size(); ++i) sz[i] = sizes[i];

  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) n *= sz[i];
  PLUSS_CHECK(static_cast<::std::int64_t>(utf8_flat.size()) == n,
              "note_from_utf8 expects flat size == numel");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(utf8_flat.size());
  for (auto s : utf8_flat) {
    spans.push_back(arena->append(s));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteImpl::Encoding::Utf8Text);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  ::std::array<::std::int64_t, 3> sz{1,1,1};
  for (::std::size_t i = 0; i < sizes.size(); ++i) sz[i] = sizes[i];

  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) n *= sz[i];
  PLUSS_CHECK(static_cast<::std::int64_t>(bytes_flat.size()) == n,
              "note_from_bytes expects flat size == numel");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(bytes_flat.size());
  for (auto b : bytes_flat) {
    spans.push_back(arena->append(b));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteImpl::Encoding::Bytes);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

static void require_text_note(const NoteImpl& impl, const char* op) {
  PLUSS_CHECK(!impl.is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (not bytes)");
}

template <class Transform>
static Chord transform_chord_text(const Chord& x, Transform&& fn) {
  if (!x.defined()) return Chord::undefined();
  return chord_from_utf8(fn(x.utf8()));
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
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
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
      arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Chord lower(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::lower_ascii); }
Chord upper(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::upper_ascii); }
Chord title(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::title_ascii); }
Chord capitalize(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::capitalize_ascii); }
Chord swapcase(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::swapcase_ascii); }
Chord strip(const Chord& x) { return transform_chord_text(x, [](auto s) { return ::pluss::coden::native::cpu::strip_ascii(s); }); }
Chord lstrip(const Chord& x) { return transform_chord_text(x, [](auto s) { return ::pluss::coden::native::cpu::lstrip_ascii(s); }); }
Chord rstrip(const Chord& x) { return transform_chord_text(x, [](auto s) { return ::pluss::coden::native::cpu::rstrip_ascii(s); }); }
Chord trim(const Chord& x) { return strip(x); }
Chord trim_left(const Chord& x) { return lstrip(x); }
Chord trim_right(const Chord& x) { return rstrip(x); }
Chord collapse_whitespace(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::collapse_whitespace_ascii); }
Chord dedent(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::dedent_ascii); }
Chord indent(const Chord& x) { return transform_chord_text(x, [](auto s) { return ::pluss::coden::native::cpu::indent_ascii(s, 2); }); }
Chord normalize_spaces(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::normalize_spaces_ascii); }
Chord remove_control_chars(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_control_ascii); }
Chord remove_non_ascii(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_non_ascii); }
Chord to_ascii(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::to_ascii_lossy); }
Chord remove_punctuation(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_punctuation_ascii); }
Chord remove_digits(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_digits_ascii); }
Chord remove_letters(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_letters_ascii); }
Chord remove_symbols(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_symbols_ascii); }
Chord remove_emojis(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::remove_emojis_utf8); }
Chord keep_alnum(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::keep_alnum_ascii); }
Chord keep_alpha(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::keep_alpha_ascii); }
Chord keep_digits(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::keep_digits_ascii); }
Chord keep_ascii(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::keep_ascii); }
Chord keep_printable(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::keep_printable_ascii); }
Chord normalize_quotes(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::normalize_quotes_utf8); }
Chord normalize_dashes(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::normalize_dashes_utf8); }
Chord normalize_ellipsis(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::normalize_ellipsis_utf8); }
Chord strip_accents(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::strip_accents_utf8); }
Chord strip_diacritics(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::strip_diacritics_utf8); }
Chord strip_zero_width(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::strip_zero_width_utf8); }
Chord strip_bom(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::strip_bom_utf8); }
Chord collapse_repeats(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::collapse_repeats_ascii); }
Chord collapse_punct(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::collapse_punct_ascii); }
Chord collapse_spaces(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::collapse_spaces_ascii); }
Chord collapse_newlines(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::collapse_newlines_ascii); }
Chord nlp_normalize_punct(const Chord& x, bool ascii_only) {
  auto out = normalize_quotes(x);
  out = normalize_dashes(out);
  out = normalize_ellipsis(out);
  if (ascii_only) out = to_ascii(out);
  return out;
}
Chord nlp_clean(const Chord& x) {
  auto out = nlp_normalize_punct(x, false);
  out = remove_control_chars(out);
  out = strip_zero_width(out);
  out = normalize_spaces(out);
  out = strip(out);
  return out;
}
Chord nlp_remove_urls(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_urls_fast); }
Chord nlp_remove_emails(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_emails_fast); }
Chord nlp_remove_mentions(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_mentions_fast); }
Chord nlp_remove_hashtags(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_hashtags_fast); }
Chord nlp_remove_html(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_html_fast); }
Chord nlp_remove_markdown(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_markdown_fast); }
Chord nlp_remove_code(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_code_fast); }
Chord nlp_remove_math(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_math_fast); }
Chord nlp_remove_latex(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_latex_fast); }
Chord nlp_remove_brackets(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_brackets_fast); }
Chord nlp_remove_parentheticals(const Chord& x) {
  return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_parentheticals_fast);
}
Chord nlp_remove_quotes(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_quotes_fast); }
Chord nlp_remove_bullets(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_bullets_fast); }
Chord nlp_remove_headers(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_headers_fast); }
Chord nlp_remove_footers(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_footers_fast); }
Chord nlp_remove_boilerplate(const Chord& x) {
  return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_boilerplate_fast);
}
Chord nlp_remove_tables(const Chord& x) { return transform_chord_text(x, ::pluss::coden::native::cpu::nlp_remove_tables_fast); }
Chord tex_strip_comments(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_strip_comments_text(s); });
}
Chord tex_strip_preamble(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_strip_preamble_text(s); });
}
Chord tex_strip_metadata(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_strip_metadata_text(s); });
}
Chord tex_remove_commands(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_commands_text(s); });
}
Chord tex_remove_environments(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_environments_text(s); });
}
Chord tex_remove_environment(const Chord& x, ::std::string_view name) {
  return transform_chord_text(x, [name](auto s) { return tex_remove_environment_text(s, name); });
}
Chord tex_remove_labels(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_labels_text(s); });
}
Chord tex_remove_refs(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_refs_text(s); });
}
Chord tex_remove_citations(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_citations_text(s); });
}
Chord tex_remove_graphics(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_graphics_text(s); });
}
Chord tex_remove_tables(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_tables_text(s); });
}
Chord tex_remove_figures(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_figures_text(s); });
}
Chord tex_remove_packages(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_packages_text(s); });
}
Chord tex_remove_bibliography(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_bibliography_text(s); });
}
Chord tex_remove_color(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_color_text(s); });
}
Chord tex_remove_hyperref(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_hyperref_text(s); });
}
Chord tex_remove_urls(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_remove_urls_text(s); });
}
Chord tex_strip_math(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_strip_math_text(s); });
}
Chord tex_strip_text(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return tex_strip_text_text(s); });
}
Chord tex_strip_environments(const Chord& x) {
  return tex_remove_environments(x);
}
Chord tex_strip_commands(const Chord& x) {
  return tex_remove_commands(x);
}
Chord tex_to_text(const Chord& x, ::std::int64_t mode) {
  return transform_chord_text(x, [mode](auto s) { return tex_to_text_text(s, mode); });
}
Chord tex_sanitize(const Chord& x, ::std::int64_t level) {
  return transform_chord_text(x, [level](auto s) { return tex_sanitize_text(s, level); });
}

Chord strip(const Chord& x, ::std::string_view chars) {
  return transform_chord_text(x, [chars](auto s) { return ::pluss::coden::native::cpu::strip_ascii(s, chars); });
}
Chord lstrip(const Chord& x, ::std::string_view chars) {
  return transform_chord_text(x, [chars](auto s) { return ::pluss::coden::native::cpu::lstrip_ascii(s, chars); });
}
Chord rstrip(const Chord& x, ::std::string_view chars) {
  return transform_chord_text(x, [chars](auto s) { return ::pluss::coden::native::cpu::rstrip_ascii(s, chars); });
}
Chord trim(const Chord& x, ::std::string_view chars) { return strip(x, chars); }
Chord trim_left(const Chord& x, ::std::string_view chars) { return lstrip(x, chars); }
Chord trim_right(const Chord& x, ::std::string_view chars) { return rstrip(x, chars); }

Chord replace(const Chord& x, ::std::string_view from, ::std::string_view to) {
  if (!x.defined()) return Chord::undefined();
  ::std::string out = ::pluss::coden::native::cpu::replace_bytes(x.utf8(), from, to);
  return chord_from_utf8(out);
}

Chord replace_first(const Chord& x, ::std::string_view from, ::std::string_view to) {
  return transform_chord_text(x, [from, to](auto s) { return replace_first_text(s, from, to); });
}

Chord replace_last(const Chord& x, ::std::string_view from, ::std::string_view to) {
  return transform_chord_text(x, [from, to](auto s) { return replace_last_text(s, from, to); });
}

Chord replace_n(const Chord& x, ::std::string_view from, ::std::string_view to, ::std::int64_t n) {
  return transform_chord_text(x, [from, to, n](auto s) { return replace_n_text(s, from, to, n); });
}

Chord remove(const Chord& x, ::std::string_view needle) {
  return replace_first(x, needle, {});
}

Chord remove_all(const Chord& x, ::std::string_view needle) {
  return replace(x, needle, {});
}

Chord remove_prefix(const Chord& x, ::std::string_view prefix) {
  return transform_chord_text(x, [prefix](auto s) { return remove_prefix_text(s, prefix); });
}

Chord remove_suffix(const Chord& x, ::std::string_view suffix) {
  return transform_chord_text(x, [suffix](auto s) { return remove_suffix_text(s, suffix); });
}

Chord ensure_prefix(const Chord& x, ::std::string_view prefix) {
  return transform_chord_text(x, [prefix](auto s) { return ensure_prefix_text(s, prefix); });
}

Chord ensure_suffix(const Chord& x, ::std::string_view suffix) {
  return transform_chord_text(x, [suffix](auto s) { return ensure_suffix_text(s, suffix); });
}

Chord surround(const Chord& x, ::std::string_view left, ::std::string_view right) {
  return transform_chord_text(x, [left, right](auto s) { return surround_text(s, left, right); });
}

Chord quote(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return quote_text(s); });
}

Chord unquote(const Chord& x) {
  return transform_chord_text(x, [](auto s) { return unquote_text(s); });
}

bool contains(const Chord& x, ::std::string_view needle) {
  if (!x.defined()) return false;
  return ::pluss::coden::native::cpu::contains_bytes(x.utf8(), needle);
}

Note lower(const Note& x) { return transform_note_text(x, "Note::lower", ::pluss::coden::native::cpu::lower_ascii); }
Note upper(const Note& x) { return transform_note_text(x, "Note::upper", ::pluss::coden::native::cpu::upper_ascii); }
Note title(const Note& x) { return transform_note_text(x, "Note::title", ::pluss::coden::native::cpu::title_ascii); }
Note capitalize(const Note& x) { return transform_note_text(x, "Note::capitalize", ::pluss::coden::native::cpu::capitalize_ascii); }
Note swapcase(const Note& x) { return transform_note_text(x, "Note::swapcase", ::pluss::coden::native::cpu::swapcase_ascii); }
Note strip(const Note& x) { return transform_note_text(x, "Note::strip", [](auto s) { return ::pluss::coden::native::cpu::strip_ascii(s); }); }
Note lstrip(const Note& x) { return transform_note_text(x, "Note::lstrip", [](auto s) { return ::pluss::coden::native::cpu::lstrip_ascii(s); }); }
Note rstrip(const Note& x) { return transform_note_text(x, "Note::rstrip", [](auto s) { return ::pluss::coden::native::cpu::rstrip_ascii(s); }); }
Note trim(const Note& x) { return strip(x); }
Note trim_left(const Note& x) { return lstrip(x); }
Note trim_right(const Note& x) { return rstrip(x); }
Note collapse_whitespace(const Note& x) {
  return transform_note_text(x, "Note::collapse_whitespace", ::pluss::coden::native::cpu::collapse_whitespace_ascii);
}
Note dedent(const Note& x) { return transform_note_text(x, "Note::dedent", ::pluss::coden::native::cpu::dedent_ascii); }
Note indent(const Note& x) {
  return transform_note_text(x, "Note::indent", [](auto s) { return ::pluss::coden::native::cpu::indent_ascii(s, 2); });
}
Note normalize_spaces(const Note& x) {
  return transform_note_text(x, "Note::normalize_spaces", ::pluss::coden::native::cpu::normalize_spaces_ascii);
}
Note remove_control_chars(const Note& x) {
  return transform_note_text(x, "Note::remove_control_chars", ::pluss::coden::native::cpu::remove_control_ascii);
}
Note remove_non_ascii(const Note& x) {
  return transform_note_text(x, "Note::remove_non_ascii", ::pluss::coden::native::cpu::remove_non_ascii);
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
Note keep_ascii(const Note& x) {
  return transform_note_text(x, "Note::keep_ascii", ::pluss::coden::native::cpu::keep_ascii);
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
Note strip_diacritics(const Note& x) {
  return transform_note_text(x, "Note::strip_diacritics", ::pluss::coden::native::cpu::strip_diacritics_utf8);
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
  out = normalize_spaces(out);
  out = strip(out);
  return out;
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
Note tex_strip_math(const Note& x) {
  return transform_note_text(x, "Note::tex_strip_math", [](auto s) { return tex_strip_math_text(s); });
}
Note tex_strip_text(const Note& x) {
  return transform_note_text(x, "Note::tex_strip_text", [](auto s) { return tex_strip_text_text(s); });
}
Note tex_strip_environments(const Note& x) {
  return tex_remove_environments(x);
}
Note tex_strip_commands(const Note& x) {
  return tex_remove_commands(x);
}
Note tex_to_text(const Note& x, ::std::int64_t mode) {
  return transform_note_text(x, "Note::tex_to_text", [mode](auto s) { return tex_to_text_text(s, mode); });
}
Note tex_sanitize(const Note& x, ::std::int64_t level) {
  return transform_note_text(x, "Note::tex_sanitize", [level](auto s) { return tex_sanitize_text(s, level); });
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
Note trim(const Note& x, ::std::string_view chars) { return strip(x, chars); }
Note trim_left(const Note& x, ::std::string_view chars) { return lstrip(x, chars); }
Note trim_right(const Note& x, ::std::string_view chars) { return rstrip(x, chars); }

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
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
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

::std::vector<::std::uint8_t> contains(const Note& x, ::std::string_view needle) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const auto& impl = NoteAccess::impl(x);
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    out[static_cast<::std::size_t>(i)] = ::pluss::coden::native::cpu::contains_bytes(impl->bytes_at(i), needle) ? 1 : 0;
  }
  return out;
}

::std::vector<::std::int64_t> lengths_bytes(const Note& x) {
  ::std::vector<::std::int64_t> out;
  if (!x.defined()) return out;
  const auto& impl = NoteAccess::impl(x);
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    out[static_cast<::std::size_t>(i)] = static_cast<::std::int64_t>(impl->bytes_at(i).size());
  }
  return out;
}

Note reshape(const Note& x, ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(x.defined(), "reshape on undefined Note");
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  const auto& impl = NoteAccess::impl(x);
  ::std::array<::std::int64_t,3> sz{1,1,1};
  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) {
    sz[static_cast<::std::size_t>(i)] = sizes[static_cast<::std::size_t>(i)];
    n *= sizes[static_cast<::std::size_t>(i)];
  }
  PLUSS_CHECK(n == impl->numel(), "reshape numel mismatch");
  // share arena/spans: view-only reshape
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::vector<NoteImpl::OffLen>(impl->spans()), sz, dim, impl->encoding());
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
  ::std::vector<NoteImpl::OffLen> new_spans;
  new_spans.resize(static_cast<::std::size_t>(impl->numel()));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < impl->numel(); ++new_lin) {
    unravel(new_lin, new_sizes, dim, new_c);
    old_c = {0,0,0};
    for (::std::int64_t i = 0; i < dim; ++i) {
      const auto old_d = dims[static_cast<::std::size_t>(i)];
      old_c[static_cast<::std::size_t>(old_d)] = new_c[static_cast<::std::size_t>(i)];
    }
    const auto old_lin = ravel(old_c, old_strides, dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, dim, impl->encoding());
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
  check_dim(x, dim);
  const auto& impl = NoteAccess::impl(x);
  const auto old_sizes = impl->sizes3();
  const auto old_dim = impl->dim();
  PLUSS_CHECK(old_dim > 0, "select expects dim() > 0");
  const auto size_d = old_sizes[static_cast<::std::size_t>(dim)];
  PLUSS_CHECK(index >= 0 && index < size_d, "select index out of range");

  // Build new sizes (remove dim).
  ::std::array<::std::int64_t,3> new_sizes{1,1,1};
  ::std::int64_t new_dim = old_dim - 1;
  for (::std::int64_t i = 0, j = 0; i < old_dim; ++i) {
    if (i == dim) continue;
    new_sizes[static_cast<::std::size_t>(j++)] = old_sizes[static_cast<::std::size_t>(i)];
  }

  const auto old_strides = strides3(old_sizes, old_dim);
  ::std::vector<NoteImpl::OffLen> new_spans;
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  new_spans.resize(static_cast<::std::size_t>(new_numel));

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
    const auto old_lin = ravel(old_c, old_strides, old_dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, new_dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note narrow(const Note& x, ::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) {
  check_dim(x, dim);
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

  ::std::vector<NoteImpl::OffLen> new_spans;
  new_spans.resize(static_cast<::std::size_t>(impl->numel() / size_d * length));

  const auto new_dim = old_dim;
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  new_spans.resize(static_cast<::std::size_t>(new_numel));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < new_numel; ++new_lin) {
    unravel(new_lin, new_sizes, new_dim, new_c);
    old_c = new_c;
    old_c[static_cast<::std::size_t>(dim)] += start;
    const auto old_lin = ravel(old_c, old_strides, old_dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, new_dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

TokenBatch split_whitespace(const Note& x) {
  return split_whitespace(x, -1);
}



// ---- Patch8: Unicode/regex/split/join hooks ----

Chord normalize(const Chord& x, ::std::int64_t /*form*/) {
  // ICU-optional hook. Without ICU we conservatively no-op.
  return x;
}

Chord casefold(const Chord& x, ::std::string_view /*locale*/) {
  // ICU-optional hook. ASCII fallback = lower.
  return lower(x);
}

bool regex_match(const Chord& x, ::std::string_view pattern) {
  if (!x.defined()) return false;
  const ::std::regex re{::std::string(pattern)};
  return ::std::regex_match(::std::string(x.utf8()), re);
}

bool regex_search(const Chord& x, ::std::string_view pattern) {
  if (!x.defined()) return false;
  const ::std::regex re{::std::string(pattern)};
  return ::std::regex_search(::std::string(x.utf8()), re);
}

Chord regex_replace(const Chord& x, ::std::string_view pattern, ::std::string_view repl) {
  if (!x.defined()) return Chord::undefined();
  const ::std::regex re{::std::string(pattern)};
  ::std::string out = ::std::regex_replace(::std::string(x.utf8()), re, ::std::string(repl));
  return chord_from_utf8(out);
}

TokenBatch split_whitespace(const Chord& x) {
  return split_whitespace(x, -1);
}

TokenBatch split(const Chord& x, ::std::string_view delim) {
  return split(x, delim, -1);
}

TokenBatch split_whitespace(const Chord& x, ::std::int64_t limit) {
  return tokenbatch_from_chord(x, [limit](::std::string_view s, ::std::vector<::std::string>& out) {
    split_whitespace_with_limit_to(s, limit, out);
  });
}

TokenBatch split(const Chord& x, ::std::string_view delim, ::std::int64_t limit) {
  return tokenbatch_from_chord(x, [delim, limit](::std::string_view s, ::std::vector<::std::string>& out) {
    split_delim_with_limit_to(s, delim, limit, out);
  });
}

TokenBatch split_lines(const Chord& x, bool keep_ends) {
  return tokenbatch_from_chord(x, [keep_ends](::std::string_view s, ::std::vector<::std::string>& out) {
    split_lines_to(s, keep_ends, out);
  });
}

TokenBatch split_chars(const Chord& x) {
  return tokenbatch_from_chord(x, [](::std::string_view s, ::std::vector<::std::string>& out) {
    split_chars_to(s, out);
  });
}

TokenBatch split_graphemes(const Chord& x) {
  return split_chars(x);
}

TokenBatch tokenize_words(const Chord& x, ::std::int64_t flags) {
  (void)flags;
  return tokenbatch_from_chord(x, [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_words_to(s, out);
  });
}

TokenBatch tokenize_sentences(const Chord& x, ::std::int64_t flags) {
  (void)flags;
  return tokenbatch_from_chord(x, [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_sentences_to(s, out);
  });
}

TokenBatch tokenize_punct(const Chord& x) {
  return tokenbatch_from_chord(x, [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_punct_to(s, out);
  });
}

TokenBatch tokenize_regex(const Chord& x, ::std::string_view pattern) {
  const ::std::regex re{::std::string(pattern)};
  return tokenbatch_from_chord(x, [&re](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_regex_to(s, re, out);
  });
}

TokenBatch regex_findall(const Chord& x, ::std::string_view pattern, ::std::int64_t flags) {
  (void)flags;
  return tokenize_regex(x, pattern);
}

TokenBatch regex_split(const Chord& x, ::std::string_view pattern, ::std::int64_t limit) {
  const ::std::regex re{::std::string(pattern)};
  return tokenbatch_from_chord(x, [&re, limit](::std::string_view s, ::std::vector<::std::string>& out) {
    regex_split_to(s, re, limit, out);
  });
}

Note normalize(const Note& x, ::std::int64_t form) {
  // no-op normalization for now
  (void)form;
  if (x.defined()) {
    const auto& impl = NoteAccess::impl(x);
    require_text_note(*impl, "Note::normalize");
  }
  return x;
}

Note casefold(const Note& x, ::std::string_view locale) {
  (void)locale;
  return lower(x);
}

::std::vector<::std::uint8_t> regex_match(const Note& x, ::std::string_view pattern) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_match");
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_match(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return out;
}

::std::vector<::std::uint8_t> regex_search(const Note& x, ::std::string_view pattern) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_search");
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_search(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return out;
}

Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl) {
  if (!x.defined()) return Note::undefined();
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_replace");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));

  for (::std::int64_t i=0;i<impl->numel();++i) {
    ::std::string out = ::std::regex_replace(::std::string(impl->bytes_at(i)), re, ::std::string(repl));
    spans.push_back(arena->append(out));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
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

TokenBatch split_graphemes(const Note& x) {
  return split_chars(x);
}

TokenBatch tokenize_words(const Note& x, ::std::int64_t flags) {
  (void)flags;
  return tokenbatch_from_note(x, "Note::tokenize_words", [](::std::string_view s, ::std::vector<::std::string>& out) {
    tokenize_words_to(s, out);
  });
}

TokenBatch tokenize_sentences(const Note& x, ::std::int64_t flags) {
  (void)flags;
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
  (void)flags;
  return tokenize_regex(x, pattern);
}

TokenBatch regex_split(const Note& x, ::std::string_view pattern, ::std::int64_t limit) {
  const ::std::regex re{::std::string(pattern)};
  return tokenbatch_from_note(x, "Note::regex_split", [&re, limit](::std::string_view s, ::std::vector<::std::string>& out) {
    regex_split_to(s, re, limit, out);
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

#ifdef CODEN_ENABLE_SERIES_INTEROP
#include "CodeM/core/ScalarType.h"

namespace pluss::coden::ops_internal {

pluss::codem::Series note_contains_series(const Note& a, ::std::string_view needle) {
  auto mask = a.contains(needle); // uint8_t per element
  ::std::vector<double> tmp(mask.size());
  for (::std::size_t i = 0; i < mask.size(); ++i) tmp[i] = static_cast<double>(mask[i] != 0);
  return pluss::codem::Series::make_from_doubles(tmp, a.sizes(), pluss::codem::ScalarType::Bool);
}

pluss::codem::Series note_lengths_bytes_series(const Note& a) {
  auto lens = a.lengths_bytes();
  ::std::vector<double> tmp(lens.size());
  for (::std::size_t i = 0; i < lens.size(); ++i) tmp[i] = static_cast<double>(lens[i]);
  return pluss::codem::Series::make_from_doubles(tmp, a.sizes(), pluss::codem::ScalarType::Int64);
}

} // namespace pluss::coden::ops_internal
#endif // CODEN_ENABLE_SERIES_INTEROP
