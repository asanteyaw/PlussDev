#include <cstdint>

#if defined(PLUSSQIT_HAS_LIBUNISTRING)
#include <unigbrk.h>
// Some libunistring packages expose fixed-width C types as macros; keep them
// from rewriting std::uint*_t through the rest of this translation unit.
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
#ifdef int64_t
#undef int64_t
#endif
#ifdef uint64_t
#undef uint64_t
#endif
#endif

#include "CodeN/compat/Dispatch.h"

#include <array>
#include <cctype>
#include <limits>
#include <regex>
#include <sstream>

#include "CodeN/core/OpsInternal.h"
#include "CodeN/generated/DispatchDeclFallbacks.inc"
#include "CodeN/generated/DispatchRegistry.inc"

namespace pluss::coden::_compat {

[[noreturn]] static void fail(::std::string_view group, ::std::string_view op, ::std::size_t nargs) {
  ::std::ostringstream oss;
  oss << "No such op for " << group << ": " << op << " (nargs=" << nargs << ")\n";
  oss << "Known signatures for '" << op << "':\n";
  bool any = false;
  for (const auto& s : kOpSigs) {
    if (s.name == op) {
      any = true;
      oss << "  - " << s.pretty << "\n";
    }
  }
  if (!any) {
    oss << "  (none)\n";
  }
  PLUSS_CHECK(false, oss.str());
}

[[noreturn]] static void rethrow_dispatch_error_(::std::string_view lane, ::std::string_view op, const ::std::exception& e) {
  // Present a public-facing message: `<op>: <underlying error>`. The `lane`
  // (adapter routing label) is internal and is not surfaced to callers.
  (void)lane;
  ::pluss::codem::throw_error(::std::string(op) + ": " + e.what());
}

template <class DeclArray>
static bool has_decl_sig(const DeclArray& decls, ::std::string_view op, ::std::size_t nargs) {
  for (const auto& sig : decls) {
    if (sig.name == op && sig.argc == nargs) return true;
  }
  return false;
}

static ::std::string_view as_str(const CodenValue& a);

static bool starts_with_sv(::std::string_view s, ::std::string_view p) {
  return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

static bool ends_with_sv(::std::string_view s, ::std::string_view suf) {
  return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
}

template <::std::size_t N, class T>
static bool in_set_sv(::std::string_view op, const ::std::array<T, N>& set) {
  for (const auto& v : set) if (::std::string_view(v) == op) return true;
  return false;
}

static bool is_token_feature_op(::std::string_view op) {
  static constexpr auto kOps = ::std::array{
      "split_sentences", "find_all", "find_all_regex", "ngrams_char", "ngrams_word",
      "filter_tokens", "map_tokens", "sort_tokens", "unique_tokens",
      "split_csv", "split_tsv", "split_on_punct", "split_on_camelcase", "split_on_casechange",
      "split_on_snakecase", "split_unicode_words",
      "nlp_tokens", "nlp_words", "nlp_sentences", "nlp_stems", "nlp_lemmas",
      "nlp_pos_tags", "nlp_named_entities", "nlp_noun_phrases", "nlp_keywords", "nlp_keyphrases",
      "nlp_bigrams", "nlp_trigrams", "nlp_char_ngrams", "nlp_word_ngrams", "nlp_hashtags",
      "nlp_mentions", "nlp_urls", "nlp_emails", "nlp_numbers", "nlp_extract_acronyms",
      "nlp_extract_definitions", "nlp_extract_terms", "nlp_extract_citations",
      "nlp_extract_bibliography", "nlp_extract_math_symbols", "nlp_stopwords", "nlp_chunks",
      "nlp_verb_phrases", "nlp_stopwords_removed_tokens", "nlp_filter_tokens_alpha",
      "nlp_filter_tokens_alnum", "nlp_filter_tokens_len_ge", "nlp_filter_tokens_len_le",
      "nlp_filter_tokens_regex", "nlp_lowercase_tokens", "nlp_unique_tokens", "nlp_sort_tokens",
      "nlp_shuffle_tokens", "nlp_remove_stopword_tokens", "nlp_remove_number_tokens",
      "nlp_remove_punct_tokens"};
  if (in_set_sv(op, kOps)) return true;
  if (starts_with_sv(op, "tex_extract_env_")) return true;
  if (starts_with_sv(op, "tex_extract_")) return true;
  static constexpr auto kTexOps = ::std::array{
      "tex_commands", "tex_command_names", "tex_environments", "tex_environment_names",
      "tex_math_blocks", "tex_inline_math", "tex_display_math", "tex_labels", "tex_refs",
      "tex_citations", "tex_packages", "tex_includes", "tex_macros_defined", "tex_macros_used",
      "tex_bibliography_keys", "tex_sections", "tex_subsections", "tex_subsubsections",
      "tex_equations", "tex_headings", "tex_captions", "tex_corollaries", "tex_definitions",
      "tex_examples", "tex_figures", "tex_footnotes", "tex_lemmas", "tex_proofs", "tex_tables",
      "tex_theorems", "tex_todos", "tex_warnings", "tex_detect_errors"};
  if (in_set_sv(op, kTexOps)) return true;
  static constexpr auto kTexOps2 = ::std::array{
      "tex_align_blocks", "tex_arrays", "tex_tabular_blocks"};
  return in_set_sv(op, kTexOps2) || op == "tex_extract_command_args";
}

static bool token_feature_uses_str_arg(::std::string_view op) {
  return op == "find_all" || op == "find_all_regex" || op == "tex_extract_command_args" ||
         op == "nlp_named_entities" || op == "nlp_pos_tags" || op == "filter_tokens" ||
         op == "map_tokens" || op == "nlp_filter_tokens_regex" || op == "nlp_stopwords";
}

static bool is_ascii_sv(::std::string_view s) {
  for (unsigned char c : s) if (c >= 0x80) return false;
  return true;
}

// is_blank: empty OR all-whitespace (a field with no visible content).
static bool is_blank_sv(::std::string_view s) {
  for (unsigned char c : s) {
    if (!::std::isspace(c)) return false;
  }
  return true;
}

// is_whitespace: at least one character, all of them whitespace (matches
// Python str.isspace(), so the empty string is NOT whitespace).
static bool is_whitespace_sv(::std::string_view s) {
  if (s.empty()) return false;
  return is_blank_sv(s);
}

static bool is_alpha_sv(::std::string_view s) {
  if (s.empty()) return false;
  for (unsigned char c : s) if (!::std::isalpha(c)) return false;
  return true;
}

static bool is_alnum_sv(::std::string_view s) {
  if (s.empty()) return false;
  for (unsigned char c : s) if (!::std::isalnum(c)) return false;
  return true;
}

static bool is_digit_sv(::std::string_view s) {
  if (s.empty()) return false;
  for (unsigned char c : s) if (!::std::isdigit(c)) return false;
  return true;
}

static bool is_lower_sv(::std::string_view s) {
  bool has_alpha = false;
  for (unsigned char c : s) {
    if (::std::isalpha(c)) {
      has_alpha = true;
      if (!::std::islower(c)) return false;
    }
  }
  return has_alpha;
}

static bool is_upper_sv(::std::string_view s) {
  bool has_alpha = false;
  for (unsigned char c : s) {
    if (::std::isalpha(c)) {
      has_alpha = true;
      if (!::std::isupper(c)) return false;
    }
  }
  return has_alpha;
}

// is_title: every alphabetic word starts with an upper-case letter and
// continues lower-case, and there is at least one such cased word (matches
// Python str.istitle(), so a digits-only string is NOT title-cased).
static bool is_title_sv(::std::string_view s) {
  bool in_word = false;
  bool saw_cased = false;
  for (unsigned char c : s) {
    if (::std::isalpha(c)) {
      if (!in_word) {
        if (!::std::isupper(c)) return false;
        in_word = true;
        saw_cased = true;
      } else if (!::std::islower(c)) {
        return false;
      }
    } else if (::std::isalnum(c)) {
      in_word = true;
    } else {
      in_word = false;
    }
  }
  return saw_cased;
}

static bool validate_utf8(::std::string_view s) {
  const auto* p = reinterpret_cast<const unsigned char*>(s.data());
  ::std::size_t i = 0;
  while (i < s.size()) {
    const unsigned char c = p[i++];
    if (c < 0x80) continue;
    int need = 0;
    if ((c & 0xE0) == 0xC0) need = 1;
    else if ((c & 0xF0) == 0xE0) need = 2;
    else if ((c & 0xF8) == 0xF0) need = 3;
    else return false;
    if (i + static_cast<::std::size_t>(need) > s.size()) return false;
    for (int j = 0; j < need; ++j) {
      if ((p[i + static_cast<::std::size_t>(j)] & 0xC0) != 0x80) return false;
    }
    i += static_cast<::std::size_t>(need);
  }
  return true;
}

static ::std::int64_t count_utf8_codepoints(::std::string_view s) {
  ::std::int64_t n = 0;
  const auto* p = reinterpret_cast<const unsigned char*>(s.data());
  for (::std::size_t i = 0; i < s.size();) {
    const unsigned char c = p[i];
    if (c < 0x80) { ++i; }
    else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) { i += 2; }
    else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) { i += 3; }
    else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) { i += 4; }
    else { ++i; }
    ++n;
  }
  return n;
}

// Count extended grapheme clusters (UAX #29) when a Unicode backend is
// available, so a base letter plus its combining marks counts as one. Falls
// back to the code-point count otherwise. (libunistring's iterator does not
// join ZWJ emoji sequences, so those still count per code point.)
static ::std::int64_t count_utf8_graphemes(::std::string_view s) {
#if defined(PLUSSQIT_HAS_LIBUNISTRING)
  if (s.empty()) return 0;
  const auto* begin = reinterpret_cast<const unsigned char*>(s.data());
  const auto* end = begin + s.size();
  ::std::int64_t n = 0;
  for (const unsigned char* cur = begin; cur != nullptr && cur < end;) {
    cur = ::u8_grapheme_next(cur, end);
    ++n;
  }
  return n;
#else
  return count_utf8_codepoints(s);
#endif
}

static ::std::vector<::std::uint8_t> default_u8list_note(::std::string_view op,
                                                          const ::pluss::coden::Note& self,
                                                          const CodenValues& args) {
  ::std::vector<::std::uint8_t> out;
  if (!self.defined()) return out;
  out.resize(static_cast<::std::size_t>(self.numel()), 0);
  for (::std::int64_t i = 0; i < self.numel(); ++i) {
    const auto s = self.at(i);
    bool v = false;
    if (op == "starts_with" && args.size() == 1) v = starts_with_sv(s, as_str(args[0]));
    else if (op == "ends_with" && args.size() == 1) v = ends_with_sv(s, as_str(args[0]));
    else if (op == "is_ascii" && args.empty()) v = is_ascii_sv(s);
    else if (op == "is_utf8_valid" && args.empty()) v = validate_utf8(s);
    else if (op == "is_blank" && args.empty()) v = is_blank_sv(s);
    else if (op == "is_whitespace" && args.empty()) v = is_whitespace_sv(s);
    else if (op == "is_alpha" && args.empty()) v = is_alpha_sv(s);
    else if (op == "is_alnum" && args.empty()) v = is_alnum_sv(s);
    else if (op == "is_digit" && args.empty()) v = is_digit_sv(s);
    else if (op == "is_lower" && args.empty()) v = is_lower_sv(s);
    else if (op == "is_upper" && args.empty()) v = is_upper_sv(s);
    else if (op == "is_title" && args.empty()) v = is_title_sv(s);
    else if (op == "contains_any_digit" && args.empty()) v = s.find_first_of("0123456789") != ::std::string_view::npos;
    else if (op == "contains_any_alpha" && args.empty()) {
      for (unsigned char c : s) {
        if (::std::isalpha(c)) { v = true; break; }
      }
    }
    out[static_cast<::std::size_t>(i)] = v ? 1 : 0;
  }
  return out;
}

static ::std::vector<::std::int64_t> default_i64list_note(::std::string_view op,
                                                           const ::pluss::coden::Note& self,
                                                           const CodenValues& args) {
  ::std::vector<::std::int64_t> out;
  if (!self.defined()) return out;
  out.resize(static_cast<::std::size_t>(self.numel()), 0);
  const bool has_sub = (op == "count_substring" && args.size() == 1);
  const bool has_re = (op == "count_regex" && args.size() == 1);
  const ::std::string_view needle = has_sub ? as_str(args[0]) : ::std::string_view{};
  ::std::regex re;
  if (has_re) re = ::std::regex(::std::string(as_str(args[0])));
  for (::std::int64_t i = 0; i < self.numel(); ++i) {
    const auto s = self.at(i);
    if (op == "lengths_bytes") out[static_cast<::std::size_t>(i)] = static_cast<::std::int64_t>(s.size());
    else if (op == "lengths_chars") out[static_cast<::std::size_t>(i)] = count_utf8_codepoints(s);
    else if (op == "lengths_graphemes") out[static_cast<::std::size_t>(i)] = count_utf8_graphemes(s);
    else if (has_sub) {
      if (needle.empty()) {
        out[static_cast<::std::size_t>(i)] = 0;
      } else {
        ::std::int64_t count = 0;
        for (::std::size_t p = 0;;) {
          const ::std::size_t f = s.find(needle, p);
          if (f == ::std::string_view::npos) break;
          ++count;
          p = f + needle.size();
        }
        out[static_cast<::std::size_t>(i)] = count;
      }
    } else if (has_re) {
      const ::std::string owned(s);
      out[static_cast<::std::size_t>(i)] = static_cast<::std::int64_t>(
          ::std::distance(::std::sregex_iterator(owned.begin(), owned.end(), re), ::std::sregex_iterator{}));
    }
  }
  return out;
}

static const CodenValue::IntList& as_intlist(const CodenValue& a) {
  if (auto p = ::std::get_if<CodenValue::IntList>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected intlist argument");
  static const CodenValue::IntList empty{};
  return empty;
}
static ::std::int64_t as_i64(const CodenValue& a) {
  if (auto p = ::std::get_if<::std::int64_t>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected int64 argument");
  return 0;
}
static bool as_bool(const CodenValue& a) {
  if (auto p = ::std::get_if<bool>(&a.v)) return *p;
  if (auto p = ::std::get_if<::std::int64_t>(&a.v)) return *p != 0;
  PLUSS_CHECK(false, "CodeN: expected bool argument");
  return false;
}
static ::std::string_view as_str(const CodenValue& a) {
  if (auto p = ::std::get_if<::std::string>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected string argument");
  return {};
}
static const ::pluss::coden::Note& as_note(const CodenValue& a) {
  if (auto p = ::std::get_if<const ::pluss::coden::Note*>(&a.v)) {
    PLUSS_CHECK(*p != nullptr, "CodeN: null Note argument");
    return **p;
  }
  PLUSS_CHECK(false, "CodeN: expected Note argument");
  return *::std::get<const ::pluss::coden::Note*>(a.v);
}

::pluss::coden::Note call_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  const ::std::string op_snapshot(op);
  try {
  if (op == "lower" && args.empty()) return ops_internal::lower(self);
  if (op == "upper" && args.empty()) return ops_internal::upper(self);
  if (op == "title" && args.empty()) return ops_internal::title(self);
  if (op == "capitalize" && args.empty()) return ops_internal::capitalize(self);
  if (op == "swapcase" && args.empty()) return ops_internal::swapcase(self);
  if (op == "strip" && args.empty()) return ops_internal::strip(self);
  if (op == "lstrip" && args.empty()) return ops_internal::lstrip(self);
  if (op == "rstrip" && args.empty()) return ops_internal::rstrip(self);
  if (op == "trim" && args.empty()) return ops_internal::trim(self);
  if (op == "trim_left" && args.empty()) return ops_internal::trim_left(self);
  if (op == "trim_right" && args.empty()) return ops_internal::trim_right(self);
  if (op == "collapse_whitespace" && args.empty()) return ops_internal::collapse_whitespace(self);
  if (op == "dedent" && args.empty()) return ops_internal::dedent(self);
  if (op == "indent" && args.empty()) return ops_internal::indent(self);
  if (op == "normalize_spaces" && args.empty()) return ops_internal::normalize_spaces(self);
  if (op == "remove_control_chars" && args.empty()) return ops_internal::remove_control_chars(self);
  if (op == "remove_non_ascii" && args.empty()) return ops_internal::remove_non_ascii(self);
  if (op == "to_ascii" && args.empty()) return ops_internal::to_ascii(self);
  if (op == "to_nfc" && args.empty()) return ops_internal::normalize(self, 0);
  if (op == "to_nfd" && args.empty()) return ops_internal::normalize(self, 1);
  if (op == "to_nfkc" && args.empty()) return ops_internal::normalize(self, 2);
  if (op == "to_nfkd" && args.empty()) return ops_internal::normalize(self, 3);
  if (op == "remove_punctuation" && args.empty()) return ops_internal::remove_punctuation(self);
  if (op == "remove_digits" && args.empty()) return ops_internal::remove_digits(self);
  if (op == "remove_letters" && args.empty()) return ops_internal::remove_letters(self);
  if (op == "remove_symbols" && args.empty()) return ops_internal::remove_symbols(self);
  if (op == "remove_emojis" && args.empty()) return ops_internal::remove_emojis(self);
  if (op == "keep_alnum" && args.empty()) return ops_internal::keep_alnum(self);
  if (op == "keep_alpha" && args.empty()) return ops_internal::keep_alpha(self);
  if (op == "keep_digits" && args.empty()) return ops_internal::keep_digits(self);
  if (op == "keep_ascii" && args.empty()) return ops_internal::keep_ascii(self);
  if (op == "keep_printable" && args.empty()) return ops_internal::keep_printable(self);
  if (op == "normalize_quotes" && args.empty()) return ops_internal::normalize_quotes(self);
  if (op == "normalize_dashes" && args.empty()) return ops_internal::normalize_dashes(self);
  if (op == "normalize_ellipsis" && args.empty()) return ops_internal::normalize_ellipsis(self);
  if (op == "strip_accents" && args.empty()) return ops_internal::strip_accents(self);
  if (op == "strip_diacritics" && args.empty()) return ops_internal::strip_diacritics(self);
  if (op == "strip_zero_width" && args.empty()) return ops_internal::strip_zero_width(self);
  if (op == "strip_bom" && args.empty()) return ops_internal::strip_bom(self);
  if (op == "collapse_repeats" && args.empty()) return ops_internal::collapse_repeats(self);
  if (op == "collapse_punct" && args.empty()) return ops_internal::collapse_punct(self);
  if (op == "collapse_spaces" && args.empty()) return ops_internal::collapse_spaces(self);
  if (op == "collapse_newlines" && args.empty()) return ops_internal::collapse_newlines(self);
  if (op == "nlp_clean" && args.empty()) return ops_internal::nlp_clean(self);
  if (op == "nlp_normalize_unicode" && args.empty()) return ops_internal::normalize(self, 2);
  if (op == "nlp_normalize_punct" && args.empty()) return ops_internal::nlp_normalize_punct(self, false);
  if (op == "nlp_normalize_numbers" && args.empty()) return ops_internal::nlp_normalize_numbers(self);
  if (op == "nlp_expand_contractions" && args.empty()) return ops_internal::nlp_expand_contractions(self);
  if (op == "nlp_remove_stopwords" && args.empty()) return ops_internal::nlp_remove_stopwords(self);
  if (op == "nlp_language" && args.empty()) return ops_internal::nlp_language(self);
  if (op == "nlp_script" && args.empty()) return ops_internal::nlp_script(self);
  if (op == "nlp_sentiment" && args.empty()) return ops_internal::nlp_sentiment(self);
  if (op == "nlp_toxicity" && args.empty()) return ops_internal::nlp_toxicity(self);
  if (op == "nlp_readability_grade" && args.empty()) return ops_internal::nlp_readability_grade(self);
  if (op == "nlp_readability_level" && args.empty()) return ops_internal::nlp_readability_level(self);
  if (op == "nlp_spam_score" && args.empty()) return ops_internal::nlp_spam_score(self);
  if (op == "nlp_formality" && args.empty()) return ops_internal::nlp_formality(self);
  if (op == "nlp_subjectivity" && args.empty()) return ops_internal::nlp_subjectivity(self);
  if (op == "to_utf8_strict" && args.empty()) return ops_internal::to_utf8_strict(self);
  if (op == "to_utf8_lossy" && args.empty()) return ops_internal::to_utf8_lossy(self);
  if (op == "from_latin1" && args.empty()) return ops_internal::from_latin1(self);
  if (op == "to_latin1_lossy" && args.empty()) return ops_internal::to_latin1_lossy(self);
  if (op == "from_windows1252" && args.empty()) return ops_internal::from_windows1252(self);
  if (op == "to_windows1252_lossy" && args.empty()) return ops_internal::to_windows1252_lossy(self);
  if (op == "transliterate_ascii" && args.empty()) return ops_internal::transliterate_ascii(self);
  if (op == "transliterate_latin" && args.empty()) return ops_internal::transliterate_latin(self);
  if (op == "transliterate_greek" && args.empty()) return ops_internal::transliterate_greek(self);
  if (op == "transliterate_cyrillic" && args.empty()) return ops_internal::transliterate_cyrillic(self);
  if (op == "phonetic_soundex" && args.empty()) return ops_internal::phonetic_soundex(self);
  if (op == "phonetic_metaphone" && args.empty()) return ops_internal::phonetic_metaphone(self);
  if (op == "phonetic_double_metaphone" && args.empty()) return ops_internal::phonetic_double_metaphone(self);
  if (op == "nlp_remove_numbers" && args.empty()) return ops_internal::remove_digits(self);
  if (op == "nlp_remove_punct" && args.empty()) return ops_internal::remove_punctuation(self);
  if (op == "nlp_remove_diacritics" && args.empty()) return ops_internal::strip_diacritics(self);
  if (op == "nlp_remove_urls" && args.empty()) return ops_internal::nlp_remove_urls(self);
  if (op == "nlp_remove_emails" && args.empty()) return ops_internal::nlp_remove_emails(self);
  if (op == "nlp_remove_mentions" && args.empty()) return ops_internal::nlp_remove_mentions(self);
  if (op == "nlp_remove_hashtags" && args.empty()) return ops_internal::nlp_remove_hashtags(self);
  if (op == "nlp_remove_html" && args.empty()) return ops_internal::nlp_remove_html(self);
  if (op == "nlp_remove_markdown" && args.empty()) return ops_internal::nlp_remove_markdown(self);
  if (op == "nlp_remove_code" && args.empty()) return ops_internal::nlp_remove_code(self);
  if (op == "nlp_remove_math" && args.empty()) return ops_internal::nlp_remove_math(self);
  if (op == "nlp_remove_latex" && args.empty()) return ops_internal::nlp_remove_latex(self);
  if (op == "nlp_remove_brackets" && args.empty()) return ops_internal::nlp_remove_brackets(self);
  if (op == "nlp_remove_parentheticals" && args.empty()) return ops_internal::nlp_remove_parentheticals(self);
  if (op == "nlp_remove_quotes" && args.empty()) return ops_internal::nlp_remove_quotes(self);
  if (op == "nlp_remove_bullets" && args.empty()) return ops_internal::nlp_remove_bullets(self);
  if (op == "nlp_remove_headers" && args.empty()) return ops_internal::nlp_remove_headers(self);
  if (op == "nlp_remove_footers" && args.empty()) return ops_internal::nlp_remove_footers(self);
  if (op == "nlp_remove_boilerplate" && args.empty()) return ops_internal::nlp_remove_boilerplate(self);
  if (op == "nlp_remove_tables" && args.empty()) return ops_internal::nlp_remove_tables(self);
  if (op == "nlp_mask_numbers" && args.empty()) return ops_internal::nlp_mask_numbers(self);
  if (op == "nlp_mask_emails" && args.empty()) return ops_internal::nlp_mask_emails(self);
  if (op == "nlp_mask_urls" && args.empty()) return ops_internal::nlp_mask_urls(self);
  if (op == "nlp_mask_mentions" && args.empty()) return ops_internal::nlp_mask_mentions(self);
  if (op == "nlp_mask_hashtags" && args.empty()) return ops_internal::nlp_mask_hashtags(self);
  if (op == "nlp_mask_dates" && args.empty()) return ops_internal::nlp_mask_dates(self);
  if (op == "nlp_mask_times" && args.empty()) return ops_internal::nlp_mask_times(self);
  if (op == "nlp_mask_currency" && args.empty()) return ops_internal::nlp_mask_currency(self);
  if (op == "nlp_mask_pii" && args.empty()) return ops_internal::nlp_mask_pii(self);
  if (op == "tex_strip_comments" && args.empty()) return ops_internal::tex_strip_comments(self);
  if (op == "tex_strip_preamble" && args.empty()) return ops_internal::tex_strip_preamble(self);
  if (op == "tex_strip_metadata" && args.empty()) return ops_internal::tex_strip_metadata(self);
  if (op == "tex_normalize_newlines" && args.empty()) return ops_internal::tex_normalize_newlines(self);
  if (op == "tex_dedup_blank_lines" && args.empty()) return ops_internal::tex_dedup_blank_lines(self);
  if (op == "tex_collapse_whitespace" && args.empty()) return ops_internal::tex_collapse_whitespace(self);
  if (op == "tex_fix_linebreaks" && args.empty()) return ops_internal::tex_fix_linebreaks(self);
  if (op == "tex_escape" && args.empty()) return ops_internal::tex_escape(self);
  if (op == "tex_unescape" && args.empty()) return ops_internal::tex_unescape(self);
  if (op == "tex_remove_math_delimiters" && args.empty()) return ops_internal::tex_remove_math_delimiters(self);
  if (op == "tex_add_math_delimiters" && args.empty()) return ops_internal::tex_add_math_delimiters(self);
  if (op == "tex_math_only" && args.empty()) return ops_internal::tex_math_only(self);
  if (op == "tex_text_only" && args.empty()) return ops_internal::tex_text_only(self);
  if (op == "tex_remove_commands" && args.empty()) return ops_internal::tex_remove_commands(self);
  if (op == "tex_remove_environments" && args.empty()) return ops_internal::tex_remove_environments(self);
  if (op == "tex_remove_labels" && args.empty()) return ops_internal::tex_remove_labels(self);
  if (op == "tex_remove_refs" && args.empty()) return ops_internal::tex_remove_refs(self);
  if (op == "tex_remove_citations" && args.empty()) return ops_internal::tex_remove_citations(self);
  if (op == "tex_remove_graphics" && args.empty()) return ops_internal::tex_remove_graphics(self);
  if (op == "tex_remove_tables" && args.empty()) return ops_internal::tex_remove_tables(self);
  if (op == "tex_remove_figures" && args.empty()) return ops_internal::tex_remove_figures(self);
  if (op == "tex_remove_packages" && args.empty()) return ops_internal::tex_remove_packages(self);
  if (op == "tex_remove_bibliography" && args.empty()) return ops_internal::tex_remove_bibliography(self);
  if (op == "tex_remove_color" && args.empty()) return ops_internal::tex_remove_color(self);
  if (op == "tex_remove_hyperref" && args.empty()) return ops_internal::tex_remove_hyperref(self);
  if (op == "tex_remove_urls" && args.empty()) return ops_internal::tex_remove_urls(self);
  if (op == "tex_remove_formatting" && args.empty()) return ops_internal::tex_remove_formatting(self);
  if (op == "tex_remove_font_commands" && args.empty()) return ops_internal::tex_remove_font_commands(self);
  if (op == "tex_remove_left_right" && args.empty()) return ops_internal::tex_remove_left_right(self);
  if (op == "tex_remove_big_delims" && args.empty()) return ops_internal::tex_remove_big_delims(self);
  if (op == "tex_remove_spacing_cmds" && args.empty()) return ops_internal::tex_remove_spacing_cmds(self);
  if (op == "tex_strip_math" && args.empty()) return ops_internal::tex_strip_math(self);
  if (op == "tex_strip_text" && args.empty()) return ops_internal::tex_strip_text(self);
  if (op == "tex_strip_environments" && args.empty()) return ops_internal::tex_strip_environments(self);
  if (op == "tex_strip_commands" && args.empty()) return ops_internal::tex_strip_commands(self);
  if (op == "tex_balance_braces" && args.empty()) return ops_internal::tex_balance_braces(self);
  if (op == "tex_balance_math_delims" && args.empty()) return ops_internal::tex_balance_math_delims(self);
  if (op == "tex_canonicalize_macros" && args.empty()) return ops_internal::tex_canonicalize_macros(self);
  if (op == "tex_expand_macros" && args.empty()) return ops_internal::tex_expand_macros(self, 1);
  if (op == "tex_inline_macros" && args.empty()) return ops_internal::tex_inline_macros(self);
  if (op == "tex_collapse_macros" && args.empty()) return ops_internal::tex_collapse_macros(self);
  if (op == "tex_expand_inputs" && args.empty()) return ops_internal::tex_expand_inputs(self);
  if (op == "tex_resolve_includes" && args.empty()) return ops_internal::tex_resolve_includes(self);
  if (op == "tex_resolve_graphics" && args.empty()) return ops_internal::tex_resolve_graphics(self);
  if (op == "tex_simplify_fractions" && args.empty()) return ops_internal::tex_simplify_fractions(self);
  if (op == "tex_simplify_roots" && args.empty()) return ops_internal::tex_simplify_roots(self);
  if (op == "tex_simplify_spacing" && args.empty()) return ops_internal::tex_simplify_spacing(self);
  if (op == "tex_convert_frac_to_slash" && args.empty()) return ops_internal::tex_convert_frac_to_slash(self);
  if (op == "tex_convert_sqrt_to_pow" && args.empty()) return ops_internal::tex_convert_sqrt_to_pow(self);
  if (op == "tex_convert_pm_to_plusminus" && args.empty()) return ops_internal::tex_convert_pm_to_plusminus(self);
  if (op == "tex_standardize_vectors" && args.empty()) return ops_internal::tex_standardize_vectors(self);
  if (op == "tex_standardize_matrices" && args.empty()) return ops_internal::tex_standardize_matrices(self);
  if (op == "tex_standardize_integrals" && args.empty()) return ops_internal::tex_standardize_integrals(self);
  if (op == "tex_standardize_summations" && args.empty()) return ops_internal::tex_standardize_summations(self);
  if (op == "tex_standardize_limits" && args.empty()) return ops_internal::tex_standardize_limits(self);
  if (op == "tex_standardize_subscripts" && args.empty()) return ops_internal::tex_standardize_subscripts(self);
  if (op == "tex_standardize_superscripts" && args.empty()) return ops_internal::tex_standardize_superscripts(self);
  if (op == "tex_standardize_primes" && args.empty()) return ops_internal::tex_standardize_primes(self);
  if (op == "tex_standardize_dots" && args.empty()) return ops_internal::tex_standardize_dots(self);
  if (op == "tex_remove_accents" && args.empty()) return ops_internal::tex_remove_accents(self);
  if (op == "tex_remove_env_equation" && args.empty()) return ops_internal::tex_remove_environment(self, "equation");
  if (op == "tex_remove_env_align" && args.empty()) return ops_internal::tex_remove_environment(self, "align");
  if (op == "tex_remove_env_align_" && args.empty()) return ops_internal::tex_remove_environment(self, "align*");
  if (op == "tex_remove_env_gather" && args.empty()) return ops_internal::tex_remove_environment(self, "gather");
  if (op == "tex_remove_env_multline" && args.empty()) return ops_internal::tex_remove_environment(self, "multline");
  if (op == "tex_remove_env_array" && args.empty()) return ops_internal::tex_remove_environment(self, "array");
  if (op == "tex_remove_env_cases" && args.empty()) return ops_internal::tex_remove_environment(self, "cases");
  if (op == "tex_remove_env_tabular" && args.empty()) return ops_internal::tex_remove_environment(self, "tabular");
  if (op == "tex_remove_env_figure" && args.empty()) return ops_internal::tex_remove_environment(self, "figure");
  if (op == "tex_remove_env_table" && args.empty()) return ops_internal::tex_remove_environment(self, "table");
  if (op == "tex_remove_env_itemize" && args.empty()) return ops_internal::tex_remove_environment(self, "itemize");
  if (op == "tex_remove_env_enumerate" && args.empty()) return ops_internal::tex_remove_environment(self, "enumerate");
  if (op == "tex_to_text" && args.empty()) return ops_internal::tex_to_text(self, 0);
  if (op == "tex_to_markdown" && args.empty()) return ops_internal::tex_to_markdown(self, 0);
  if (op == "tex_to_html" && args.empty()) return ops_internal::tex_to_html(self, 0);
  if (op == "tex_sanitize" && args.empty()) return ops_internal::tex_sanitize(self, 0);
  if (op == "nlp_simplify_whitespace" && args.empty()) return ops_internal::collapse_whitespace(self);
  if (op == "nlp_sentencecase" && args.empty()) return ops_internal::capitalize(self);
  if (op == "nlp_remove_nonprintable" && args.empty()) return ops_internal::remove_control_chars(self);
  if (op == "nlp_remove_accents" && args.empty()) return ops_internal::strip_accents(self);
  if (op == "nlp_normalize_whitespace" && args.empty()) return ops_internal::normalize_spaces(self);
  if (op == "nlp_normalize_newlines" && args.empty()) return ops_internal::collapse_newlines(self);
  if (op == "nlp_normalize_case" && args.empty()) return ops_internal::lower(self);

  if (op == "strip" && args.size() == 1) return ops_internal::strip(self, as_str(args[0]));
  if (op == "lstrip" && args.size() == 1) return ops_internal::lstrip(self, as_str(args[0]));
  if (op == "rstrip" && args.size() == 1) return ops_internal::rstrip(self, as_str(args[0]));
  if (op == "trim" && args.size() == 1) return ops_internal::trim(self, as_str(args[0]));
  if (op == "trim_left" && args.size() == 1) return ops_internal::trim_left(self, as_str(args[0]));
  if (op == "trim_right" && args.size() == 1) return ops_internal::trim_right(self, as_str(args[0]));

  if (op == "join" && (args.empty() || args.size() == 1)) {
    ::std::string_view sep = args.empty() ? ::std::string_view{} : as_str(args[0]);
    return ops_internal::join(self, sep);
  }
  if (op == "replace" && args.size() == 2) return ops_internal::replace(self, as_str(args[0]), as_str(args[1]));
  if (op == "replace_first" && args.size() == 2) return ops_internal::replace_first(self, as_str(args[0]), as_str(args[1]));
  if (op == "replace_last" && args.size() == 2) return ops_internal::replace_last(self, as_str(args[0]), as_str(args[1]));
  if (op == "replace_n" && args.size() == 3) return ops_internal::replace_n(self, as_str(args[0]), as_str(args[1]), as_i64(args[2]));
  if (op == "remove" && args.size() == 1) return ops_internal::remove(self, as_str(args[0]));
  if (op == "remove_all" && args.size() == 1) return ops_internal::remove_all(self, as_str(args[0]));
  if (op == "remove_prefix" && args.size() == 1) return ops_internal::remove_prefix(self, as_str(args[0]));
  if (op == "remove_suffix" && args.size() == 1) return ops_internal::remove_suffix(self, as_str(args[0]));
  if (op == "ensure_prefix" && args.size() == 1) return ops_internal::ensure_prefix(self, as_str(args[0]));
  if (op == "ensure_suffix" && args.size() == 1) return ops_internal::ensure_suffix(self, as_str(args[0]));
  if (op == "surround" && args.size() == 2) return ops_internal::surround(self, as_str(args[0]), as_str(args[1]));
  if (op == "quote" && args.empty()) return ops_internal::quote(self);
  if (op == "unquote" && args.empty()) return ops_internal::unquote(self);
  if (op == "base64_encode" && args.empty()) return ops_internal::base64_encode(self);
  if (op == "base64_decode" && args.empty()) return ops_internal::base64_decode(self);
  if (op == "hex_encode" && args.empty()) return ops_internal::hex_encode(self);
  if (op == "hex_decode" && args.empty()) return ops_internal::hex_decode(self);
  if (op == "url_encode" && args.empty()) return ops_internal::url_encode(self);
  if (op == "url_decode" && args.empty()) return ops_internal::url_decode(self);
  if (op == "html_escape" && args.empty()) return ops_internal::html_escape(self);
  if (op == "html_unescape" && args.empty()) return ops_internal::html_unescape(self);
  if (op == "json_escape" && args.empty()) return ops_internal::json_escape(self);
  if (op == "contiguous" && args.empty()) return ops_internal::contiguous(self);
  if (op == "flatten" && args.empty()) return ops_internal::flatten(self);
  if (op == "sort" && args.empty()) return ops_internal::sort(self, -1);
  if (op == "unique" && args.empty()) {
    return ops_internal::unique(self, ::std::numeric_limits<::std::int64_t>::min());
  }
  if (op == "deduplicate" && args.empty()) {
    return ops_internal::deduplicate(self, ::std::numeric_limits<::std::int64_t>::min());
  }
  if (op == "nlp_clean" && args.size() == 1) return ops_internal::nlp_clean(self); // flags reserved
  if (op == "nlp_normalize_unicode" && args.size() == 1) return ops_internal::normalize(self, as_i64(args[0]));
  if (op == "nlp_normalize_punct" && args.size() == 1) return ops_internal::nlp_normalize_punct(self, as_bool(args[0]));
  if (op == "nlp_remove_stopwords" && args.size() == 1) return ops_internal::nlp_remove_stopwords(self, as_str(args[0]));
  if (op == "nlp_remove_html" && args.size() == 1) return ops_internal::nlp_remove_html(self);
  if (op == "nlp_remove_markdown" && args.size() == 1) return ops_internal::nlp_remove_markdown(self);
  if (op == "nlp_remove_code" && args.size() == 1) return ops_internal::nlp_remove_code(self);
  if (op == "nlp_remove_math" && args.size() == 1) return ops_internal::nlp_remove_math(self);
  if (op == "nlp_remove_latex" && args.size() == 1) return ops_internal::nlp_remove_latex(self);
  if (op == "nlp_remove_boilerplate" && args.size() == 1) return ops_internal::nlp_remove_boilerplate(self);
  if (op == "nlp_remove_tables" && args.size() == 1) return ops_internal::nlp_remove_tables(self);
  if (op == "nlp_remove_headers" && args.size() == 1) return ops_internal::nlp_remove_headers(self);
  if (op == "nlp_remove_footers" && args.size() == 1) return ops_internal::nlp_remove_footers(self);
  if (op == "nlp_mask_pii" && args.size() == 1) return ops_internal::nlp_mask_pii(self, as_i64(args[0]));
  if (op == "nlp_join_tokens" && args.size() == 1) return ops_internal::join(self, as_str(args[0]));
  if (op == "tex_extract_environment" && args.size() == 1) {
    ::std::string extract_op = "tex_extract_env_";
    extract_op.append(as_str(args[0]));
    const auto batch = ops_internal::token_feature(self, extract_op);
    return batch.tokens();
  }
  if (op == "tex_expand_macros" && args.size() == 1) return ops_internal::tex_expand_macros(self, as_i64(args[0]));
  if (op == "tex_remove_environment" && args.size() == 1) return ops_internal::tex_remove_environment(self, as_str(args[0]));
  if (op == "tex_remove_macro" && args.size() == 1) return ops_internal::tex_remove_macro(self, as_str(args[0]));
  if (op == "tex_wrap_inline_math" && args.size() == 1) return ops_internal::tex_wrap_inline_math(self, as_str(args[0]));
  if (op == "tex_wrap_display_math" && args.size() == 1) return ops_internal::tex_wrap_display_math(self, as_str(args[0]));
  if (starts_with_sv(op, "tex_replace_") && args.size() == 1 && op != "tex_replace_command") {
    return ops_internal::tex_replace_command(self, op.substr(12), as_str(args[0]));
  }
  if (op == "tex_set_macro" && args.size() == 2) return ops_internal::tex_set_macro(self, as_str(args[0]), as_str(args[1]));
  if (op == "tex_replace_command" && args.size() == 2) return ops_internal::tex_replace_command(self, as_str(args[0]), as_str(args[1]));
  if (op == "tex_to_text" && args.size() == 1) return ops_internal::tex_to_text(self, as_i64(args[0]));
  if (op == "tex_to_markdown" && args.size() == 1) return ops_internal::tex_to_markdown(self, as_i64(args[0]));
  if (op == "tex_to_html" && args.size() == 1) return ops_internal::tex_to_html(self, as_i64(args[0]));
  if (op == "tex_sanitize" && args.size() == 1) return ops_internal::tex_sanitize(self, as_i64(args[0]));

  if (op == "reshape" && args.size() == 1) return ops_internal::reshape(self, as_intlist(args[0]));
  if (op == "expand_dims" && args.size() == 1) return ops_internal::expand_dims(self, as_i64(args[0]));
  if (op == "unsqueeze" && args.size() == 1) return ops_internal::unsqueeze(self, as_i64(args[0]));
  if (op == "squeeze" && args.size() == 1) return ops_internal::squeeze(self, as_i64(args[0]));
  if (op == "repeat" && args.size() == 1) return ops_internal::repeat(self, as_intlist(args[0]));
  if (op == "tile" && args.size() == 1) return ops_internal::tile(self, as_intlist(args[0]));
  if (op == "sort" && args.size() == 1) return ops_internal::sort(self, as_i64(args[0]));
  if (op == "unique" && args.size() == 1) return ops_internal::unique(self, as_i64(args[0]));
  if (op == "deduplicate" && args.size() == 1) return ops_internal::deduplicate(self, as_i64(args[0]));
  if (op == "repeat_interleave" && args.size() == 2) return ops_internal::repeat_interleave(self, as_i64(args[0]), as_i64(args[1]));
  if (op == "transpose" && args.size() == 2) return ops_internal::transpose(self, as_i64(args[0]), as_i64(args[1]));
  if (op == "permute" && args.size() == 1) return ops_internal::permute(self, as_intlist(args[0]));
  if (op == "select" && args.size() == 2) return ops_internal::select(self, as_i64(args[0]), as_i64(args[1]));
  if (op == "narrow" && args.size() == 3) return ops_internal::narrow(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));

  if (op == "normalize" && args.size() == 1) return ops_internal::normalize(self, as_i64(args[0]));
  if (op == "casefold" && (args.size() == 0 || args.size() == 1)) {
    ::std::string_view loc = args.empty() ? ::std::string_view{} : as_str(args[0]);
    return ops_internal::casefold(self, loc);
  }

  if (op == "regex_replace" && args.size() == 2) return ops_internal::regex_replace(self, as_str(args[0]), as_str(args[1]));
  if (op == "regex_replace" && args.size() == 3) return ops_internal::regex_replace(self, as_str(args[0]), as_str(args[1]), as_i64(args[2]));
  if (op == "jaccard_acronyms" && args.size() == 1) return ops_internal::jaccard_acronyms(self, as_note(args[0]));
  if (op == "jaccard_citations" && args.size() == 1) return ops_internal::jaccard_citations(self, as_note(args[0]));
  if (op == "jaccard_math_symbols" && args.size() == 1) return ops_internal::jaccard_math_symbols(self, as_note(args[0]));
  if (op == "jaccard_tokens" && args.size() == 1) return ops_internal::jaccard_tokens(self, as_note(args[0]));
  if (op == "jaccard_terms" && args.size() == 1) return ops_internal::jaccard_terms(self, as_note(args[0]));
  if (op == "overlap_acronyms" && args.size() == 1) return ops_internal::overlap_acronyms(self, as_note(args[0]));
  if (op == "overlap_citations" && args.size() == 1) return ops_internal::overlap_citations(self, as_note(args[0]));
  if (op == "overlap_math_symbols" && args.size() == 1) return ops_internal::overlap_math_symbols(self, as_note(args[0]));
  if (op == "overlap_tokens" && args.size() == 1) return ops_internal::overlap_tokens(self, as_note(args[0]));
  if (op == "overlap_terms" && args.size() == 1) return ops_internal::overlap_terms(self, as_note(args[0]));

  fail("Note", op, args.size());
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const ::std::exception& e) {
    rethrow_dispatch_error_("Note", op_snapshot, e);
  } catch (...) {
    ::pluss::codem::throw_error(
        ::std::string("CodeN Note dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

::std::vector<::std::uint8_t> call_u8list_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  const ::std::string op_snapshot(op);
  try {
  if (op == "contains" && args.size() == 1) return ops_internal::contains(self, as_str(args[0]));
  if (op == "regex_match" && args.size() == 1) return ops_internal::regex_match(self, as_str(args[0]));
  if (op == "regex_search" && args.size() == 1) return ops_internal::regex_search(self, as_str(args[0]));
  // The remaining per-cell predicates below all migrated their PUBLIC Note
  // signature to a shape-preserving Bool Note (pass 4 of the Note redesign);
  // kDeclNoteU8ListSigs is auto-derived from that same public declaration
  // list and no longer lists them, so they're allow-listed explicitly here
  // instead of going through has_decl_sig -- this is the internal u8list
  // path the Note-returning wrappers in NoteMethodDefsGen.h still call to do
  // the actual per-element computation before packaging the result as a
  // Bool Note.
  struct BoolNoteInternalOp { ::std::string_view name; ::std::size_t argc; };
  static constexpr ::std::array<BoolNoteInternalOp, 14> kBoolNoteInternalOps{{
      {"starts_with", 1}, {"ends_with", 1}, {"is_ascii", 0}, {"is_utf8_valid", 0},
      {"is_blank", 0}, {"is_whitespace", 0}, {"is_alpha", 0}, {"is_alnum", 0},
      {"is_digit", 0}, {"is_lower", 0}, {"is_upper", 0}, {"is_title", 0},
      {"contains_any_digit", 0}, {"contains_any_alpha", 0}}};
  for (const auto& allowed : kBoolNoteInternalOps) {
    if (op == allowed.name && args.size() == allowed.argc) return default_u8list_note(op, self, args);
  }
  if (has_decl_sig(kDeclNoteU8ListSigs, op, args.size())) return default_u8list_note(op, self, args);
  fail("Note->u8list", op, args.size());
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const ::std::exception& e) {
    rethrow_dispatch_error_("Note->u8list", op_snapshot, e);
  } catch (...) {
    ::pluss::codem::throw_error(
        ::std::string("CodeN Note->u8list dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

::std::vector<::std::int64_t> call_i64list_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  const ::std::string op_snapshot(op);
  try {
  if (op == "lengths_bytes" && args.empty()) return ops_internal::lengths_bytes(self);
  // The remaining per-cell measurement/count ops below all migrated their
  // PUBLIC Note signature to a shape-preserving Int64 Note (pass 5 of the
  // Note redesign); kDeclNoteI64ListSigs is auto-derived from that same
  // public declaration list and no longer lists them, so they're
  // allow-listed explicitly here instead of going through has_decl_sig --
  // this is the internal i64list path the Note-returning wrappers in
  // NoteMethodDefsGen.h still call to do the actual per-element computation
  // before packaging the result as an Int64 Note. (Mirrors the analogous
  // kBoolNoteInternalOps allowlist in call_u8list_note above.)
  struct IntNoteInternalOp { ::std::string_view name; ::std::size_t argc; };
  static constexpr ::std::array<IntNoteInternalOp, 4> kIntNoteInternalOps{{
      {"lengths_chars", 0}, {"lengths_graphemes", 0},
      {"count_substring", 1}, {"count_regex", 1}}};
  for (const auto& allowed : kIntNoteInternalOps) {
    if (op == allowed.name && args.size() == allowed.argc) return default_i64list_note(op, self, args);
  }
  if (has_decl_sig(kDeclNoteI64ListSigs, op, args.size())) return default_i64list_note(op, self, args);
  fail("Note->i64list", op, args.size());
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const ::std::exception& e) {
    rethrow_dispatch_error_("Note->i64list", op_snapshot, e);
  } catch (...) {
    ::pluss::codem::throw_error(
        ::std::string("CodeN Note->i64list dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

TokenBatch call_tokenbatch_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  const ::std::string op_snapshot(op);
  try {
  if (op == "split_whitespace" && args.empty()) return ops_internal::split_whitespace(self);
  if (op == "split_whitespace" && args.size() == 1) return ops_internal::split_whitespace(self, as_i64(args[0]));
  if (op == "split" && args.size() == 1) return ops_internal::split(self, as_str(args[0]));
  if (op == "split" && args.size() == 2) return ops_internal::split(self, as_str(args[0]), as_i64(args[1]));
  if (op == "split_lines" && args.empty()) return ops_internal::split_lines(self, false);
  if (op == "split_lines" && args.size() == 1) return ops_internal::split_lines(self, as_bool(args[0]));
  if (op == "split_chars" && args.empty()) return ops_internal::split_chars(self);
  if (op == "split_graphemes" && args.empty()) return ops_internal::split_graphemes(self);
  if (op == "tokenize_words" && args.empty()) return ops_internal::tokenize_words(self, 0);
  if (op == "tokenize_words" && args.size() == 1) return ops_internal::tokenize_words(self, as_i64(args[0]));
  if (op == "tokenize_sentences" && args.empty()) return ops_internal::tokenize_sentences(self, 0);
  if (op == "tokenize_sentences" && args.size() == 1) return ops_internal::tokenize_sentences(self, as_i64(args[0]));
  if (op == "tokenize_punct" && args.empty()) return ops_internal::tokenize_punct(self);
  if (op == "tokenize_regex" && args.size() == 1) return ops_internal::tokenize_regex(self, as_str(args[0]));
  if (op == "regex_findall" && args.size() == 1) return ops_internal::regex_findall(self, as_str(args[0]), 0);
  if (op == "regex_findall" && args.size() == 2) return ops_internal::regex_findall(self, as_str(args[0]), as_i64(args[1]));
  if (op == "regex_split" && args.size() == 1) return ops_internal::regex_split(self, as_str(args[0]), -1);
  if (op == "regex_split" && args.size() == 2) return ops_internal::regex_split(self, as_str(args[0]), as_i64(args[1]));
  if (op == "intersect_acronyms" && args.size() == 1) return ops_internal::intersect_acronyms(self, as_note(args[0]));
  if (op == "intersect_citations" && args.size() == 1) return ops_internal::intersect_citations(self, as_note(args[0]));
  if (op == "intersect_math_symbols" && args.size() == 1) return ops_internal::intersect_math_symbols(self, as_note(args[0]));
  if (op == "intersect_tokens" && args.size() == 1) return ops_internal::intersect_tokens(self, as_note(args[0]));
  if (op == "intersect_terms" && args.size() == 1) return ops_internal::intersect_terms(self, as_note(args[0]));
  if (op == "diff_words" && args.size() == 1) return ops_internal::diff_words(self, as_note(args[0]));
  if (op == "diff_lines" && args.size() == 1) return ops_internal::diff_lines(self, as_note(args[0]));
  if (op == "diff_chars" && args.size() == 1) return ops_internal::diff_chars(self, as_note(args[0]));
  if (op == "align_tokens" && args.size() == 1) return ops_internal::align_tokens(self, as_note(args[0]));
  if (op == "align_sentences" && args.size() == 1) return ops_internal::align_sentences(self, as_note(args[0]));
  if (is_token_feature_op(op)) {
    if (args.empty()) return ops_internal::token_feature(self, op);
    if (args.size() == 1) {
      if (token_feature_uses_str_arg(op)) return ops_internal::token_feature(self, op, -1, as_str(args[0]));
      return ops_internal::token_feature(self, op, as_i64(args[0]));
    }
    if (args.size() == 2 && token_feature_uses_str_arg(op)) {
      return ops_internal::token_feature(self, op, as_i64(args[1]), as_str(args[0]));
    }
  }
  if (has_decl_sig(kDeclNoteTokenBatchSigs, op, args.size())) return ops_internal::split_whitespace(self);
  fail("Note->TokenBatch", op, args.size());
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const ::std::exception& e) {
    rethrow_dispatch_error_("Note->TokenBatch", op_snapshot, e);
  } catch (...) {
    ::pluss::codem::throw_error(
        ::std::string("CodeN Note->TokenBatch dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

} // namespace pluss::coden::_compat
