#pragma once

// AUTO-GENERATED: inline method definitions via _compat::invoke
// Included outside namespace pluss::coden (see core headers).

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden {

inline Note Note::lower() const {
  return ::pluss::coden::_compat::invoke<Note>("lower", *this);
}

inline Note Note::upper() const {
  return ::pluss::coden::_compat::invoke<Note>("upper", *this);
}

inline Note Note::title() const {
  return ::pluss::coden::_compat::invoke<Note>("title", *this);
}

inline Note Note::capitalize() const {
  return ::pluss::coden::_compat::invoke<Note>("capitalize", *this);
}

inline Note Note::casefold() const {
  return ::pluss::coden::_compat::invoke<Note>("casefold", *this);
}

inline Note Note::swapcase() const {
  return ::pluss::coden::_compat::invoke<Note>("swapcase", *this);
}

inline Note Note::strip() const {
  return ::pluss::coden::_compat::invoke<Note>("strip", *this);
}

inline Note Note::lstrip() const {
  return ::pluss::coden::_compat::invoke<Note>("lstrip", *this);
}

inline Note Note::rstrip() const {
  return ::pluss::coden::_compat::invoke<Note>("rstrip", *this);
}

inline Note Note::trim() const {
  return ::pluss::coden::_compat::invoke<Note>("trim", *this);
}

inline Note Note::trim_left() const {
  return ::pluss::coden::_compat::invoke<Note>("trim_left", *this);
}

inline Note Note::trim_right() const {
  return ::pluss::coden::_compat::invoke<Note>("trim_right", *this);
}

inline Note Note::collapse_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_whitespace", *this);
}

inline Note Note::dedent() const {
  return ::pluss::coden::_compat::invoke<Note>("dedent", *this);
}

inline Note Note::indent() const {
  return ::pluss::coden::_compat::invoke<Note>("indent", *this);
}

inline Note Note::normalize_spaces() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_spaces", *this);
}

inline Note Note::remove_control_chars() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_control_chars", *this);
}

inline Note Note::remove_non_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_non_ascii", *this);
}

inline Note Note::to_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("to_ascii", *this);
}

inline Note Note::to_nfkc() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfkc", *this);
}

inline Note Note::to_nfc() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfc", *this);
}

inline Note Note::to_nfd() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfd", *this);
}

inline Note Note::to_nfkd() const {
  return ::pluss::coden::_compat::invoke<Note>("to_nfkd", *this);
}

inline Note Note::remove_punctuation() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_punctuation", *this);
}

inline Note Note::remove_digits() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_digits", *this);
}

inline Note Note::remove_letters() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_letters", *this);
}

inline Note Note::remove_symbols() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_symbols", *this);
}

inline Note Note::remove_emojis() const {
  return ::pluss::coden::_compat::invoke<Note>("remove_emojis", *this);
}

inline Note Note::keep_alnum() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_alnum", *this);
}

inline Note Note::keep_alpha() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_alpha", *this);
}

inline Note Note::keep_digits() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_digits", *this);
}

inline Note Note::keep_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_ascii", *this);
}

inline Note Note::keep_printable() const {
  return ::pluss::coden::_compat::invoke<Note>("keep_printable", *this);
}

inline Note Note::normalize_quotes() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_quotes", *this);
}

inline Note Note::normalize_dashes() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_dashes", *this);
}

inline Note Note::normalize_ellipsis() const {
  return ::pluss::coden::_compat::invoke<Note>("normalize_ellipsis", *this);
}

inline Note Note::strip_accents() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_accents", *this);
}

inline Note Note::strip_diacritics() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_diacritics", *this);
}

inline Note Note::strip_zero_width() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_zero_width", *this);
}

inline Note Note::strip_bom() const {
  return ::pluss::coden::_compat::invoke<Note>("strip_bom", *this);
}

inline Note Note::collapse_repeats() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_repeats", *this);
}

inline Note Note::collapse_punct() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_punct", *this);
}

inline Note Note::collapse_spaces() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_spaces", *this);
}

inline Note Note::collapse_newlines() const {
  return ::pluss::coden::_compat::invoke<Note>("collapse_newlines", *this);
}

inline Note Note::nlp_clean() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_clean", *this);
}

inline Note Note::nlp_normalize_unicode() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_unicode", *this);
}

inline Note Note::nlp_normalize_punct() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_punct", *this);
}

inline Note Note::nlp_remove_stopwords() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_stopwords", *this);
}

inline Note Note::nlp_remove_urls() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_urls", *this);
}

inline Note Note::nlp_remove_emails() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_emails", *this);
}

inline Note Note::nlp_remove_mentions() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_mentions", *this);
}

inline Note Note::nlp_remove_hashtags() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_hashtags", *this);
}

inline Note Note::nlp_remove_numbers() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_numbers", *this);
}

inline Note Note::nlp_remove_punct() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_punct", *this);
}

inline Note Note::nlp_remove_diacritics() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_diacritics", *this);
}

inline Note Note::nlp_expand_contractions() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_expand_contractions", *this);
}

inline Note Note::nlp_simplify_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_simplify_whitespace", *this);
}

inline Note Note::nlp_sentencecase() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_sentencecase", *this);
}

inline Note Note::nlp_remove_html() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_html", *this);
}

inline Note Note::nlp_remove_markdown() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_markdown", *this);
}

inline Note Note::nlp_remove_code() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_code", *this);
}

inline Note Note::nlp_remove_math() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_math", *this);
}

inline Note Note::nlp_remove_latex() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_latex", *this);
}

inline Note Note::nlp_remove_brackets() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_brackets", *this);
}

inline Note Note::nlp_remove_parentheticals() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_parentheticals", *this);
}

inline Note Note::nlp_remove_quotes() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_quotes", *this);
}

inline Note Note::nlp_remove_bullets() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_bullets", *this);
}

inline Note Note::nlp_remove_headers() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_headers", *this);
}

inline Note Note::nlp_remove_footers() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_footers", *this);
}

inline Note Note::nlp_remove_boilerplate() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_boilerplate", *this);
}

inline Note Note::nlp_remove_tables() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_tables", *this);
}

inline Note Note::nlp_remove_nonprintable() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_nonprintable", *this);
}

inline Note Note::nlp_remove_accents() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_accents", *this);
}

inline Note Note::nlp_normalize_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_whitespace", *this);
}

inline Note Note::nlp_normalize_newlines() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_newlines", *this);
}

inline Note Note::nlp_normalize_case() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_case", *this);
}

inline Note Note::nlp_normalize_numbers() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_numbers", *this);
}

inline Note Note::nlp_mask_numbers() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_numbers", *this);
}

inline Note Note::nlp_mask_emails() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_emails", *this);
}

inline Note Note::nlp_mask_urls() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_urls", *this);
}

inline Note Note::nlp_mask_mentions() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_mentions", *this);
}

inline Note Note::nlp_mask_hashtags() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_hashtags", *this);
}

inline Note Note::nlp_mask_dates() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_dates", *this);
}

inline Note Note::nlp_mask_times() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_times", *this);
}

inline Note Note::nlp_mask_currency() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_currency", *this);
}

inline Note Note::nlp_mask_pii() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_pii", *this);
}

inline Note Note::tex_strip_comments() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_comments", *this);
}

inline Note Note::tex_strip_preamble() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_preamble", *this);
}

inline Note Note::tex_strip_metadata() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_metadata", *this);
}

inline Note Note::tex_normalize_newlines() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_normalize_newlines", *this);
}

inline Note Note::tex_dedup_blank_lines() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_dedup_blank_lines", *this);
}

inline Note Note::tex_collapse_whitespace() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_collapse_whitespace", *this);
}

inline Note Note::tex_fix_linebreaks() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_fix_linebreaks", *this);
}

inline Note Note::tex_sanitize() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_sanitize", *this);
}

inline Note Note::tex_escape() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_escape", *this);
}

inline Note Note::tex_unescape() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_unescape", *this);
}

inline Note Note::tex_remove_math_delimiters() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_math_delimiters", *this);
}

inline Note Note::tex_add_math_delimiters() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_add_math_delimiters", *this);
}

inline Note Note::tex_math_only() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_math_only", *this);
}

inline Note Note::tex_text_only() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_text_only", *this);
}

inline Note Note::tex_remove_commands() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_commands", *this);
}

inline Note Note::tex_remove_environments() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_environments", *this);
}

inline Note Note::tex_remove_labels() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_labels", *this);
}

inline Note Note::tex_remove_refs() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_refs", *this);
}

inline Note Note::tex_remove_citations() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_citations", *this);
}

inline Note Note::tex_remove_graphics() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_graphics", *this);
}

inline Note Note::tex_remove_tables() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_tables", *this);
}

inline Note Note::tex_remove_figures() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_figures", *this);
}

inline Note Note::tex_remove_packages() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_packages", *this);
}

inline Note Note::tex_remove_bibliography() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_bibliography", *this);
}

inline Note Note::tex_balance_braces() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_balance_braces", *this);
}

inline Note Note::tex_balance_math_delims() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_balance_math_delims", *this);
}

inline Note Note::tex_canonicalize_macros() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_canonicalize_macros", *this);
}

inline Note Note::tex_expand_macros() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_expand_macros", *this);
}

inline Note Note::tex_inline_macros() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_inline_macros", *this);
}

inline Note Note::tex_simplify_fractions() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_simplify_fractions", *this);
}

inline Note Note::tex_simplify_roots() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_simplify_roots", *this);
}

inline Note Note::tex_simplify_spacing() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_simplify_spacing", *this);
}

inline Note Note::tex_remove_formatting() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_formatting", *this);
}

inline Note Note::tex_remove_accents() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_accents", *this);
}

inline Note Note::tex_remove_font_commands() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_font_commands", *this);
}

inline Note Note::tex_strip_math() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_math", *this);
}

inline Note Note::tex_strip_text() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_text", *this);
}

inline Note Note::tex_strip_environments() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_environments", *this);
}

inline Note Note::tex_strip_commands() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_strip_commands", *this);
}

inline Note Note::tex_collapse_macros() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_collapse_macros", *this);
}

inline Note Note::tex_expand_inputs() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_expand_inputs", *this);
}

inline Note Note::tex_resolve_includes() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_resolve_includes", *this);
}

inline Note Note::tex_resolve_graphics() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_resolve_graphics", *this);
}

inline Note Note::tex_remove_color() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_color", *this);
}

inline Note Note::tex_remove_hyperref() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_hyperref", *this);
}

inline Note Note::tex_remove_urls() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_urls", *this);
}

inline Note Note::tex_convert_frac_to_slash() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_convert_frac_to_slash", *this);
}

inline Note Note::tex_convert_sqrt_to_pow() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_convert_sqrt_to_pow", *this);
}

inline Note Note::tex_convert_pm_to_plusminus() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_convert_pm_to_plusminus", *this);
}

inline Note Note::tex_remove_left_right() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_left_right", *this);
}

inline Note Note::tex_remove_big_delims() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_big_delims", *this);
}

inline Note Note::tex_remove_spacing_cmds() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_spacing_cmds", *this);
}

inline Note Note::tex_standardize_vectors() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_vectors", *this);
}

inline Note Note::tex_standardize_matrices() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_matrices", *this);
}

inline Note Note::tex_standardize_integrals() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_integrals", *this);
}

inline Note Note::tex_standardize_summations() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_summations", *this);
}

inline Note Note::tex_standardize_limits() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_limits", *this);
}

inline Note Note::tex_standardize_subscripts() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_subscripts", *this);
}

inline Note Note::tex_standardize_superscripts() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_superscripts", *this);
}

inline Note Note::tex_standardize_primes() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_primes", *this);
}

inline Note Note::tex_standardize_dots() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_standardize_dots", *this);
}

inline Note Note::tex_to_text() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_text", *this);
}

inline Note Note::tex_to_markdown() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_markdown", *this);
}

inline Note Note::tex_to_html() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_html", *this);
}

inline Note Note::strip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("strip", *this, chars);
}

inline Note Note::lstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("lstrip", *this, chars);
}

inline Note Note::rstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("rstrip", *this, chars);
}

inline Note Note::trim(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("trim", *this, chars);
}

inline Note Note::trim_left(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("trim_left", *this, chars);
}

inline Note Note::trim_right(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Note>("trim_right", *this, chars);
}

inline Note Note::replace(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace", *this, from, to);
}

inline Note Note::replace_first(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_first", *this, from, to);
}

inline Note Note::replace_last(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_last", *this, from, to);
}

inline Note Note::replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<Note>("replace_n", *this, from, to, n);
}

inline Note Note::remove(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("remove", *this, needle);
}

inline Note Note::remove_all(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_all", *this, needle);
}

inline Note Note::remove_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_prefix", *this, prefix);
}

inline Note Note::remove_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Note>("remove_suffix", *this, suffix);
}

inline Note Note::ensure_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Note>("ensure_prefix", *this, prefix);
}

inline Note Note::ensure_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Note>("ensure_suffix", *this, suffix);
}

inline Note Note::surround(::std::string_view left, ::std::string_view right) const {
  return ::pluss::coden::_compat::invoke<Note>("surround", *this, left, right);
}

inline Note Note::quote() const {
  return ::pluss::coden::_compat::invoke<Note>("quote", *this);
}

inline Note Note::unquote() const {
  return ::pluss::coden::_compat::invoke<Note>("unquote", *this);
}

inline Note Note::normalize(::std::int64_t form) const {
  return ::pluss::coden::_compat::invoke<Note>("normalize", *this, form);
}

inline Note Note::casefold(::std::string_view locale) const {
  return ::pluss::coden::_compat::invoke<Note>("casefold", *this, locale);
}

inline Note Note::regex_replace(::std::string_view pattern, ::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("regex_replace", *this, pattern, repl);
}

inline Note Note::regex_replace(::std::string_view pattern, ::std::string_view repl, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("regex_replace", *this, pattern, repl, flags);
}

inline Note Note::nlp_clean(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_clean", *this, flags);
}

inline Note Note::nlp_remove_stopwords(::std::string_view lang) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_stopwords", *this, lang);
}

inline Note Note::nlp_normalize_unicode(::std::int64_t form) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_unicode", *this, form);
}

inline Note Note::nlp_normalize_punct(bool ascii_only) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_normalize_punct", *this, ascii_only);
}

inline Note Note::tex_expand_macros(::std::int64_t max_passes) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_expand_macros", *this, max_passes);
}

inline Note Note::tex_sanitize(::std::int64_t level) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_sanitize", *this, level);
}

inline Note Note::tex_to_text(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_text", *this, mode);
}

inline Note Note::tex_to_markdown(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_markdown", *this, mode);
}

inline Note Note::tex_to_html(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_to_html", *this, mode);
}

inline ::std::vector<::std::uint8_t> Note::contains(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("contains", *this, needle);
}

inline ::std::vector<::std::uint8_t> Note::starts_with(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("starts_with", *this, prefix);
}

inline ::std::vector<::std::uint8_t> Note::ends_with(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("ends_with", *this, suffix);
}

inline ::std::vector<::std::uint8_t> Note::regex_match(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("regex_match", *this, pattern);
}

inline ::std::vector<::std::uint8_t> Note::regex_search(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("regex_search", *this, pattern);
}

inline ::std::vector<::std::uint8_t> Note::is_ascii() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_ascii", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_utf8_valid() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_utf8_valid", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_bytes() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_bytes", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_blank() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_blank", *this);
}

inline ::std::vector<::std::int64_t> Note::lengths_bytes() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::int64_t>>("lengths_bytes", *this);
}

inline ::std::vector<::std::int64_t> Note::lengths_chars() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::int64_t>>("lengths_chars", *this);
}

inline ::std::vector<::std::int64_t> Note::lengths_graphemes() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::int64_t>>("lengths_graphemes", *this);
}

inline TokenBatch Note::split(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, delim);
}

inline TokenBatch Note::split_whitespace() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this);
}

inline TokenBatch Note::split_lines() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this);
}

inline TokenBatch Note::split_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_sentences", *this);
}

inline TokenBatch Note::split_on_punct() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_on_punct", *this);
}

inline TokenBatch Note::split_graphemes() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_graphemes", *this);
}

inline TokenBatch Note::split_chars() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_chars", *this);
}

inline TokenBatch Note::tokenize_words() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_words", *this);
}

inline TokenBatch Note::tokenize_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_sentences", *this);
}

inline TokenBatch Note::tokenize_punct() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_punct", *this);
}

inline TokenBatch Note::tokenize_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_regex", *this, pattern);
}

inline TokenBatch Note::regex_findall(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_findall", *this, pattern);
}

inline TokenBatch Note::regex_split(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_split", *this, pattern);
}

inline TokenBatch Note::ngrams_char(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("ngrams_char", *this, n);
}

inline TokenBatch Note::ngrams_word(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("ngrams_word", *this, n);
}

inline TokenBatch Note::nlp_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_tokens", *this);
}

inline TokenBatch Note::nlp_words() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_words", *this);
}

inline TokenBatch Note::nlp_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_sentences", *this);
}

inline TokenBatch Note::nlp_stems() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_stems", *this);
}

inline TokenBatch Note::nlp_lemmas() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_lemmas", *this);
}

inline TokenBatch Note::nlp_pos_tags() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_pos_tags", *this);
}

inline TokenBatch Note::nlp_named_entities() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_named_entities", *this);
}

inline TokenBatch Note::nlp_noun_phrases() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_noun_phrases", *this);
}

inline TokenBatch Note::nlp_keywords(::std::int64_t k) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_keywords", *this, k);
}

inline TokenBatch Note::nlp_keyphrases(::std::int64_t k) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_keyphrases", *this, k);
}

inline TokenBatch Note::nlp_bigrams() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_bigrams", *this);
}

inline TokenBatch Note::nlp_trigrams() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_trigrams", *this);
}

inline TokenBatch Note::nlp_char_ngrams(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_char_ngrams", *this, n);
}

inline TokenBatch Note::nlp_word_ngrams(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_word_ngrams", *this, n);
}

inline TokenBatch Note::nlp_hashtags() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_hashtags", *this);
}

inline TokenBatch Note::nlp_mentions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_mentions", *this);
}

inline TokenBatch Note::nlp_urls() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_urls", *this);
}

inline TokenBatch Note::nlp_emails() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_emails", *this);
}

inline TokenBatch Note::nlp_numbers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_numbers", *this);
}

inline TokenBatch Note::nlp_extract_acronyms() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_acronyms", *this);
}

inline TokenBatch Note::nlp_extract_definitions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_definitions", *this);
}

inline TokenBatch Note::nlp_extract_terms() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_terms", *this);
}

inline TokenBatch Note::nlp_extract_citations() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_citations", *this);
}

inline TokenBatch Note::nlp_extract_bibliography() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_bibliography", *this);
}

inline TokenBatch Note::nlp_extract_math_symbols() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_math_symbols", *this);
}

inline TokenBatch Note::tex_commands() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_commands", *this);
}

inline TokenBatch Note::tex_command_names() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_command_names", *this);
}

inline TokenBatch Note::tex_environments() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environments", *this);
}

inline TokenBatch Note::tex_environment_names() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environment_names", *this);
}

inline TokenBatch Note::tex_math_blocks(::std::int64_t max_count) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_math_blocks", *this, max_count);
}

inline TokenBatch Note::tex_inline_math() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_inline_math", *this);
}

inline TokenBatch Note::tex_display_math() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_display_math", *this);
}

inline TokenBatch Note::tex_labels() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_labels", *this);
}

inline TokenBatch Note::tex_refs() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_refs", *this);
}

inline TokenBatch Note::tex_citations() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_citations", *this);
}

inline TokenBatch Note::tex_packages() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_packages", *this);
}

inline TokenBatch Note::tex_includes() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_includes", *this);
}

inline TokenBatch Note::tex_macros_defined() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_macros_defined", *this);
}

inline TokenBatch Note::tex_macros_used() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_macros_used", *this);
}

inline TokenBatch Note::tex_bibliography_keys() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_bibliography_keys", *this);
}

inline TokenBatch Note::tex_sections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_sections", *this);
}

inline TokenBatch Note::tex_subsections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_subsections", *this);
}

inline TokenBatch Note::tex_subsubsections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_subsubsections", *this);
}

inline TokenBatch Note::tex_equations(::std::int64_t max_count) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_equations", *this, max_count);
}

inline TokenBatch Note::tex_align_blocks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_align_blocks", *this);
}

inline TokenBatch Note::tex_arrays() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_arrays", *this);
}

inline TokenBatch Note::tex_tabular_blocks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_tabular_blocks", *this);
}

inline TokenBatch Note::tex_headings(::std::int64_t max_depth) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_headings", *this, max_depth);
}

inline TokenBatch Note::tex_extract_symbols() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_symbols", *this);
}

inline TokenBatch Note::tex_extract_greek() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_greek", *this);
}

inline TokenBatch Note::tex_extract_operators() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_operators", *this);
}

inline TokenBatch Note::tex_extract_functions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_functions", *this);
}

inline TokenBatch Note::tex_extract_numbers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_numbers", *this);
}

inline TokenBatch Note::tex_extract_identifiers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_identifiers", *this);
}

inline Note Note::join() const {
  return ::pluss::coden::_compat::invoke<Note>("join", *this);
}

inline Note Note::join(::std::string_view sep) const {
  return ::pluss::coden::_compat::invoke<Note>("join", *this, sep);
}

inline Note Note::reshape(::std::span<const ::std::int64_t> sizes) const {
  return ::pluss::coden::_compat::invoke<Note>("reshape", *this, sizes);
}

inline Note Note::transpose(::std::int64_t dim0, ::std::int64_t dim1) const {
  return ::pluss::coden::_compat::invoke<Note>("transpose", *this, dim0, dim1);
}

inline Note Note::permute(::std::span<const ::std::int64_t> dims) const {
  return ::pluss::coden::_compat::invoke<Note>("permute", *this, dims);
}

inline Note Note::select(::std::int64_t dim, ::std::int64_t index) const {
  return ::pluss::coden::_compat::invoke<Note>("select", *this, dim, index);
}

inline Note Note::narrow(::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) const {
  return ::pluss::coden::_compat::invoke<Note>("narrow", *this, dim, start, length);
}

inline Note Note::flatten() const {
  return ::pluss::coden::_compat::invoke<Note>("flatten", *this);
}

inline Note Note::contiguous() const {
  return ::pluss::coden::_compat::invoke<Note>("contiguous", *this);
}

inline TokenBatch Note::find_all(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all", *this, needle);
}

inline TokenBatch Note::find_all(::std::string_view needle, ::std::int64_t start) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all", *this, needle, start);
}

inline TokenBatch Note::find_all_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all_regex", *this, pattern);
}

inline TokenBatch Note::find_all_regex(::std::string_view pattern, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all_regex", *this, pattern, flags);
}

inline TokenBatch Note::tex_extract_env_equation() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_equation", *this);
}

inline Note Note::tex_remove_env_equation() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_equation", *this);
}

inline TokenBatch Note::tex_extract_env_align() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_align", *this);
}

inline Note Note::tex_remove_env_align() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_align", *this);
}

inline TokenBatch Note::tex_extract_env_align_() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_align_", *this);
}

inline Note Note::tex_remove_env_align_() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_align_", *this);
}

inline TokenBatch Note::tex_extract_env_gather() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_gather", *this);
}

inline Note Note::tex_remove_env_gather() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_gather", *this);
}

inline TokenBatch Note::tex_extract_env_multline() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_multline", *this);
}

inline Note Note::tex_remove_env_multline() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_multline", *this);
}

inline TokenBatch Note::tex_extract_env_array() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_array", *this);
}

inline Note Note::tex_remove_env_array() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_array", *this);
}

inline TokenBatch Note::tex_extract_env_cases() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_cases", *this);
}

inline Note Note::tex_remove_env_cases() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_cases", *this);
}

inline TokenBatch Note::tex_extract_env_tabular() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_tabular", *this);
}

inline Note Note::tex_remove_env_tabular() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_tabular", *this);
}

inline TokenBatch Note::tex_extract_env_figure() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_figure", *this);
}

inline Note Note::tex_remove_env_figure() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_figure", *this);
}

inline TokenBatch Note::tex_extract_env_table() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_table", *this);
}

inline Note Note::tex_remove_env_table() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_table", *this);
}

inline TokenBatch Note::tex_extract_env_itemize() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_itemize", *this);
}

inline Note Note::tex_remove_env_itemize() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_itemize", *this);
}

inline TokenBatch Note::tex_extract_env_enumerate() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_enumerate", *this);
}

inline Note Note::tex_remove_env_enumerate() const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_env_enumerate", *this);
}

inline Note Note::tex_replace_alpha(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_alpha", *this, repl);
}

inline Note Note::tex_replace_beta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_beta", *this, repl);
}

inline Note Note::tex_replace_gamma(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_gamma", *this, repl);
}

inline Note Note::tex_replace_delta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_delta", *this, repl);
}

inline Note Note::tex_replace_epsilon(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_epsilon", *this, repl);
}

inline Note Note::tex_replace_varepsilon(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_varepsilon", *this, repl);
}

inline Note Note::tex_replace_theta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_theta", *this, repl);
}

inline Note Note::tex_replace_vartheta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_vartheta", *this, repl);
}

inline Note Note::tex_replace_lambda(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_lambda", *this, repl);
}

inline Note Note::tex_replace_mu(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_mu", *this, repl);
}

inline Note Note::tex_replace_pi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_pi", *this, repl);
}

inline Note Note::tex_replace_sigma(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_sigma", *this, repl);
}

inline Note Note::tex_replace_phi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_phi", *this, repl);
}

inline Note Note::tex_replace_varphi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_varphi", *this, repl);
}

inline Note Note::tex_replace_omega(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_omega", *this, repl);
}

inline Note Note::base64_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("base64_encode", *this);
}

inline Note Note::base64_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("base64_decode", *this);
}

inline Note Note::hex_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("hex_encode", *this);
}

inline Note Note::hex_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("hex_decode", *this);
}

inline Note Note::url_encode() const {
  return ::pluss::coden::_compat::invoke<Note>("url_encode", *this);
}

inline Note Note::url_decode() const {
  return ::pluss::coden::_compat::invoke<Note>("url_decode", *this);
}

inline Note Note::html_escape() const {
  return ::pluss::coden::_compat::invoke<Note>("html_escape", *this);
}

inline Note Note::html_unescape() const {
  return ::pluss::coden::_compat::invoke<Note>("html_unescape", *this);
}

inline Note Note::json_escape() const {
  return ::pluss::coden::_compat::invoke<Note>("json_escape", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_alpha() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_alpha", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_alnum() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_alnum", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_digit() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_digit", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_lower() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_lower", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_upper() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_upper", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_title() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_title", *this);
}

inline ::std::vector<::std::uint8_t> Note::is_whitespace() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("is_whitespace", *this);
}

inline ::std::vector<::std::uint8_t> Note::contains_any_digit() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("contains_any_digit", *this);
}

inline ::std::vector<::std::uint8_t> Note::contains_any_alpha() const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::uint8_t>>("contains_any_alpha", *this);
}

inline ::std::vector<::std::int64_t> Note::count_substring(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::int64_t>>("count_substring", *this, needle);
}

inline ::std::vector<::std::int64_t> Note::count_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<::std::vector<::std::int64_t>>("count_regex", *this, pattern);
}

inline TokenBatch Note::split(::std::string_view delim, ::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, delim, limit);
}

inline TokenBatch Note::split_lines(bool keep_ends) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this, keep_ends);
}

inline TokenBatch Note::split_whitespace(::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this, limit);
}

inline TokenBatch Note::regex_findall(::std::string_view pattern, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_findall", *this, pattern, flags);
}

inline TokenBatch Note::regex_split(::std::string_view pattern, ::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_split", *this, pattern, limit);
}

inline TokenBatch Note::tokenize_words(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_words", *this, flags);
}

inline TokenBatch Note::tokenize_sentences(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_sentences", *this, flags);
}

inline TokenBatch Note::nlp_tokens(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_tokens", *this, flags);
}

inline TokenBatch Note::nlp_words(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_words", *this, flags);
}

inline TokenBatch Note::nlp_sentences(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_sentences", *this, flags);
}

inline TokenBatch Note::nlp_named_entities(::std::string_view scheme) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_named_entities", *this, scheme);
}

inline TokenBatch Note::nlp_pos_tags(::std::string_view tagset) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_pos_tags", *this, tagset);
}

inline Note Note::tex_extract_environment(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_extract_environment", *this, name);
}

inline Note Note::tex_remove_environment(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_environment", *this, name);
}

inline TokenBatch Note::tex_extract_command_args(::std::string_view cmd) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_command_args", *this, cmd);
}

inline Note Note::tex_replace_command(::std::string_view cmd, ::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_replace_command", *this, cmd, repl);
}

inline Note Note::tex_set_macro(::std::string_view name, ::std::string_view body) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_set_macro", *this, name, body);
}

inline Note Note::tex_remove_macro(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_remove_macro", *this, name);
}

inline Note Note::tex_wrap_inline_math(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_wrap_inline_math", *this, delim);
}

inline Note Note::tex_wrap_display_math(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<Note>("tex_wrap_display_math", *this, delim);
}

inline TokenBatch Note::tex_commands(bool with_args) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_commands", *this, with_args);
}

inline TokenBatch Note::tex_environments(bool with_body) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environments", *this, with_body);
}

inline TokenBatch Note::tex_theorems() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_theorems", *this);
}

inline TokenBatch Note::tex_proofs() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_proofs", *this);
}

inline TokenBatch Note::tex_definitions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_definitions", *this);
}

inline TokenBatch Note::tex_lemmas() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_lemmas", *this);
}

inline TokenBatch Note::tex_corollaries() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_corollaries", *this);
}

inline TokenBatch Note::tex_examples() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_examples", *this);
}

inline TokenBatch Note::tex_tables() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_tables", *this);
}

inline TokenBatch Note::tex_figures() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_figures", *this);
}

inline TokenBatch Note::tex_captions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_captions", *this);
}

inline TokenBatch Note::tex_footnotes() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_footnotes", *this);
}

inline TokenBatch Note::tex_todos() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_todos", *this);
}

inline TokenBatch Note::tex_warnings() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_warnings", *this);
}

inline Note Note::nlp_language() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_language", *this);
}

inline Note Note::nlp_script() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_script", *this);
}

inline Note Note::nlp_sentiment() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_sentiment", *this);
}

inline Note Note::nlp_toxicity() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_toxicity", *this);
}

inline Note Note::nlp_readability_grade() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_readability_grade", *this);
}

inline Note Note::nlp_readability_level() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_readability_level", *this);
}

inline Note Note::nlp_spam_score() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_spam_score", *this);
}

inline Note Note::nlp_formality() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_formality", *this);
}

inline Note Note::nlp_subjectivity() const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_subjectivity", *this);
}

inline Note Note::nlp_mask_pii(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_mask_pii", *this, flags);
}

inline Note Note::nlp_remove_boilerplate(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_boilerplate", *this, flags);
}

inline Note Note::nlp_remove_tables(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_tables", *this, flags);
}

inline Note Note::nlp_remove_headers(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_headers", *this, flags);
}

inline Note Note::nlp_remove_footers(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_remove_footers", *this, flags);
}

inline Note Note::to_utf8_strict() const {
  return ::pluss::coden::_compat::invoke<Note>("to_utf8_strict", *this);
}

inline Note Note::to_utf8_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_utf8_lossy", *this);
}

inline Note Note::from_latin1() const {
  return ::pluss::coden::_compat::invoke<Note>("from_latin1", *this);
}

inline Note Note::to_latin1_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_latin1_lossy", *this);
}

inline Note Note::from_windows1252() const {
  return ::pluss::coden::_compat::invoke<Note>("from_windows1252", *this);
}

inline Note Note::to_windows1252_lossy() const {
  return ::pluss::coden::_compat::invoke<Note>("to_windows1252_lossy", *this);
}

inline TokenBatch Note::nlp_lowercase_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_lowercase_tokens", *this);
}

inline TokenBatch Note::nlp_remove_stopword_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_remove_stopword_tokens", *this);
}

inline TokenBatch Note::nlp_remove_punct_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_remove_punct_tokens", *this);
}

inline TokenBatch Note::nlp_remove_number_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_remove_number_tokens", *this);
}

inline TokenBatch Note::nlp_filter_tokens_alpha() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_filter_tokens_alpha", *this);
}

inline TokenBatch Note::nlp_filter_tokens_alnum() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_filter_tokens_alnum", *this);
}

inline TokenBatch Note::nlp_filter_tokens_len_ge(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_filter_tokens_len_ge", *this, n);
}

inline TokenBatch Note::nlp_filter_tokens_len_le(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_filter_tokens_len_le", *this, n);
}

inline TokenBatch Note::nlp_filter_tokens_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_filter_tokens_regex", *this, pattern);
}

inline Note Note::nlp_join_tokens(::std::string_view sep) const {
  return ::pluss::coden::_compat::invoke<Note>("nlp_join_tokens", *this, sep);
}

inline TokenBatch Note::nlp_shuffle_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_shuffle_tokens", *this);
}

inline TokenBatch Note::nlp_unique_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_unique_tokens", *this);
}

inline TokenBatch Note::nlp_sort_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_sort_tokens", *this);
}

inline Note Note::transliterate_ascii() const {
  return ::pluss::coden::_compat::invoke<Note>("transliterate_ascii", *this);
}

inline Note Note::transliterate_latin() const {
  return ::pluss::coden::_compat::invoke<Note>("transliterate_latin", *this);
}

inline Note Note::transliterate_greek() const {
  return ::pluss::coden::_compat::invoke<Note>("transliterate_greek", *this);
}

inline Note Note::transliterate_cyrillic() const {
  return ::pluss::coden::_compat::invoke<Note>("transliterate_cyrillic", *this);
}

inline Note Note::phonetic_soundex() const {
  return ::pluss::coden::_compat::invoke<Note>("phonetic_soundex", *this);
}

inline Note Note::phonetic_metaphone() const {
  return ::pluss::coden::_compat::invoke<Note>("phonetic_metaphone", *this);
}

inline Note Note::phonetic_double_metaphone() const {
  return ::pluss::coden::_compat::invoke<Note>("phonetic_double_metaphone", *this);
}

inline TokenBatch Note::intersect_acronyms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("intersect_acronyms", *this, other);
}

inline TokenBatch Note::intersect_citations(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("intersect_citations", *this, other);
}

inline TokenBatch Note::intersect_math_symbols(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("intersect_math_symbols", *this, other);
}

inline TokenBatch Note::intersect_tokens(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("intersect_tokens", *this, other);
}

inline TokenBatch Note::intersect_terms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("intersect_terms", *this, other);
}

inline Note Note::jaccard_acronyms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("jaccard_acronyms", *this, other);
}

inline Note Note::jaccard_citations(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("jaccard_citations", *this, other);
}

inline Note Note::jaccard_math_symbols(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("jaccard_math_symbols", *this, other);
}

inline Note Note::jaccard_tokens(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("jaccard_tokens", *this, other);
}

inline Note Note::jaccard_terms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("jaccard_terms", *this, other);
}

inline Note Note::overlap_acronyms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("overlap_acronyms", *this, other);
}

inline Note Note::overlap_citations(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("overlap_citations", *this, other);
}

inline Note Note::overlap_math_symbols(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("overlap_math_symbols", *this, other);
}

inline Note Note::overlap_tokens(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("overlap_tokens", *this, other);
}

inline Note Note::overlap_terms(const Note& other) const {
  return ::pluss::coden::_compat::invoke<Note>("overlap_terms", *this, other);
}

inline TokenBatch Note::diff_words(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("diff_words", *this, other);
}

inline TokenBatch Note::diff_lines(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("diff_lines", *this, other);
}

inline TokenBatch Note::diff_chars(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("diff_chars", *this, other);
}

inline TokenBatch Note::align_tokens(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("align_tokens", *this, other);
}

inline TokenBatch Note::align_sentences(const Note& other) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("align_sentences", *this, other);
}

inline Note Note::deduplicate() const {
  return ::pluss::coden::_compat::invoke<Note>("deduplicate", *this);
}

inline Note Note::deduplicate(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("deduplicate", *this, dim);
}

inline Note Note::sort() const {
  return ::pluss::coden::_compat::invoke<Note>("sort", *this);
}

inline Note Note::sort(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("sort", *this, dim);
}

inline Note Note::unique() const {
  return ::pluss::coden::_compat::invoke<Note>("unique", *this);
}

inline Note Note::unique(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("unique", *this, dim);
}

inline Note Note::squeeze(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("squeeze", *this, dim);
}

inline Note Note::unsqueeze(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("unsqueeze", *this, dim);
}

inline Note Note::expand_dims(::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("expand_dims", *this, dim);
}

inline Note Note::repeat(::std::span<const ::std::int64_t> repeats) const {
  return ::pluss::coden::_compat::invoke<Note>("repeat", *this, repeats);
}

inline Note Note::repeat_interleave(::std::int64_t repeats, ::std::int64_t dim) const {
  return ::pluss::coden::_compat::invoke<Note>("repeat_interleave", *this, repeats, dim);
}

inline Note Note::tile(::std::span<const ::std::int64_t> reps) const {
  return ::pluss::coden::_compat::invoke<Note>("tile", *this, reps);
}

} // namespace pluss::coden
