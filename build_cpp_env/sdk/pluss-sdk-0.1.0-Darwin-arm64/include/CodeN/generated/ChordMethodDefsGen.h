#pragma once

// AUTO-GENERATED: inline method definitions via _compat::invoke
// Included outside namespace pluss::coden (see core headers).

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden {

inline Chord Chord::lower() const {
  return ::pluss::coden::_compat::invoke<Chord>("lower", *this);
}

inline Chord Chord::upper() const {
  return ::pluss::coden::_compat::invoke<Chord>("upper", *this);
}

inline Chord Chord::title() const {
  return ::pluss::coden::_compat::invoke<Chord>("title", *this);
}

inline Chord Chord::capitalize() const {
  return ::pluss::coden::_compat::invoke<Chord>("capitalize", *this);
}

inline Chord Chord::casefold() const {
  return ::pluss::coden::_compat::invoke<Chord>("casefold", *this);
}

inline Chord Chord::swapcase() const {
  return ::pluss::coden::_compat::invoke<Chord>("swapcase", *this);
}

inline Chord Chord::strip() const {
  return ::pluss::coden::_compat::invoke<Chord>("strip", *this);
}

inline Chord Chord::lstrip() const {
  return ::pluss::coden::_compat::invoke<Chord>("lstrip", *this);
}

inline Chord Chord::rstrip() const {
  return ::pluss::coden::_compat::invoke<Chord>("rstrip", *this);
}

inline Chord Chord::trim() const {
  return ::pluss::coden::_compat::invoke<Chord>("trim", *this);
}

inline Chord Chord::trim_left() const {
  return ::pluss::coden::_compat::invoke<Chord>("trim_left", *this);
}

inline Chord Chord::trim_right() const {
  return ::pluss::coden::_compat::invoke<Chord>("trim_right", *this);
}

inline Chord Chord::collapse_whitespace() const {
  return ::pluss::coden::_compat::invoke<Chord>("collapse_whitespace", *this);
}

inline Chord Chord::dedent() const {
  return ::pluss::coden::_compat::invoke<Chord>("dedent", *this);
}

inline Chord Chord::indent() const {
  return ::pluss::coden::_compat::invoke<Chord>("indent", *this);
}

inline Chord Chord::normalize_spaces() const {
  return ::pluss::coden::_compat::invoke<Chord>("normalize_spaces", *this);
}

inline Chord Chord::remove_control_chars() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_control_chars", *this);
}

inline Chord Chord::remove_non_ascii() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_non_ascii", *this);
}

inline Chord Chord::to_ascii() const {
  return ::pluss::coden::_compat::invoke<Chord>("to_ascii", *this);
}

inline Chord Chord::to_nfkc() const {
  return ::pluss::coden::_compat::invoke<Chord>("to_nfkc", *this);
}

inline Chord Chord::to_nfc() const {
  return ::pluss::coden::_compat::invoke<Chord>("to_nfc", *this);
}

inline Chord Chord::to_nfd() const {
  return ::pluss::coden::_compat::invoke<Chord>("to_nfd", *this);
}

inline Chord Chord::to_nfkd() const {
  return ::pluss::coden::_compat::invoke<Chord>("to_nfkd", *this);
}

inline Chord Chord::replace(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Chord>("replace", *this, from, to);
}

inline Chord Chord::replace_first(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Chord>("replace_first", *this, from, to);
}

inline Chord Chord::replace_last(::std::string_view from, ::std::string_view to) const {
  return ::pluss::coden::_compat::invoke<Chord>("replace_last", *this, from, to);
}

inline Chord Chord::replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<Chord>("replace_n", *this, from, to, n);
}

inline Chord Chord::remove(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Chord>("remove", *this, needle);
}

inline Chord Chord::remove_all(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_all", *this, needle);
}

inline Chord Chord::remove_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_prefix", *this, prefix);
}

inline Chord Chord::remove_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_suffix", *this, suffix);
}

inline bool Chord::contains(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<bool>("contains", *this, needle);
}

inline bool Chord::starts_with(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<bool>("starts_with", *this, prefix);
}

inline bool Chord::ends_with(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<bool>("ends_with", *this, suffix);
}

inline bool Chord::regex_match(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<bool>("regex_match", *this, pattern);
}

inline bool Chord::regex_search(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<bool>("regex_search", *this, pattern);
}

inline bool Chord::is_ascii() const {
  return ::pluss::coden::_compat::invoke<bool>("is_ascii", *this);
}

inline bool Chord::is_utf8_valid() const {
  return ::pluss::coden::_compat::invoke<bool>("is_utf8_valid", *this);
}

inline bool Chord::is_bytes() const {
  return ::pluss::coden::_compat::invoke<bool>("is_bytes", *this);
}

inline bool Chord::is_blank() const {
  return ::pluss::coden::_compat::invoke<bool>("is_blank", *this);
}

inline bool Chord::is_whitespace() const {
  return ::pluss::coden::_compat::invoke<bool>("is_whitespace", *this);
}

inline bool Chord::is_alpha() const {
  return ::pluss::coden::_compat::invoke<bool>("is_alpha", *this);
}

inline bool Chord::is_alnum() const {
  return ::pluss::coden::_compat::invoke<bool>("is_alnum", *this);
}

inline bool Chord::is_digit() const {
  return ::pluss::coden::_compat::invoke<bool>("is_digit", *this);
}

inline bool Chord::is_lower() const {
  return ::pluss::coden::_compat::invoke<bool>("is_lower", *this);
}

inline bool Chord::is_upper() const {
  return ::pluss::coden::_compat::invoke<bool>("is_upper", *this);
}

inline bool Chord::is_title() const {
  return ::pluss::coden::_compat::invoke<bool>("is_title", *this);
}

inline Chord Chord::regex_replace(::std::string_view pattern, ::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("regex_replace", *this, pattern, repl);
}

inline TokenBatch Chord::regex_findall(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_findall", *this, pattern);
}

inline TokenBatch Chord::regex_split(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_split", *this, pattern);
}

inline TokenBatch Chord::split(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, delim);
}

inline TokenBatch Chord::split_whitespace() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this);
}

inline TokenBatch Chord::split_lines() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this);
}

inline TokenBatch Chord::split_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_sentences", *this);
}

inline TokenBatch Chord::split_graphemes() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_graphemes", *this);
}

inline TokenBatch Chord::split_chars() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_chars", *this);
}

inline TokenBatch Chord::tokenize_words() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_words", *this);
}

inline TokenBatch Chord::tokenize_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_sentences", *this);
}

inline TokenBatch Chord::tokenize_punct() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_punct", *this);
}

inline TokenBatch Chord::tokenize_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_regex", *this, pattern);
}

inline TokenBatch Chord::ngrams_char(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("ngrams_char", *this, n);
}

inline TokenBatch Chord::ngrams_word(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("ngrams_word", *this, n);
}

inline Chord Chord::base64_encode() const {
  return ::pluss::coden::_compat::invoke<Chord>("base64_encode", *this);
}

inline Chord Chord::base64_decode() const {
  return ::pluss::coden::_compat::invoke<Chord>("base64_decode", *this);
}

inline Chord Chord::hex_encode() const {
  return ::pluss::coden::_compat::invoke<Chord>("hex_encode", *this);
}

inline Chord Chord::hex_decode() const {
  return ::pluss::coden::_compat::invoke<Chord>("hex_decode", *this);
}

inline Chord Chord::url_encode() const {
  return ::pluss::coden::_compat::invoke<Chord>("url_encode", *this);
}

inline Chord Chord::url_decode() const {
  return ::pluss::coden::_compat::invoke<Chord>("url_decode", *this);
}

inline Chord Chord::html_escape() const {
  return ::pluss::coden::_compat::invoke<Chord>("html_escape", *this);
}

inline Chord Chord::html_unescape() const {
  return ::pluss::coden::_compat::invoke<Chord>("html_unescape", *this);
}

inline Chord Chord::json_escape() const {
  return ::pluss::coden::_compat::invoke<Chord>("json_escape", *this);
}

inline Chord Chord::tex_strip_comments() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_comments", *this);
}

inline Chord Chord::tex_strip_preamble() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_preamble", *this);
}

inline Chord Chord::tex_strip_metadata() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_metadata", *this);
}

inline Chord Chord::tex_normalize_newlines() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_normalize_newlines", *this);
}

inline Chord Chord::tex_dedup_blank_lines() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_dedup_blank_lines", *this);
}

inline Chord Chord::tex_collapse_whitespace() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_collapse_whitespace", *this);
}

inline Chord Chord::tex_fix_linebreaks() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_fix_linebreaks", *this);
}

inline Chord Chord::tex_sanitize() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_sanitize", *this);
}

inline Chord Chord::tex_escape() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_escape", *this);
}

inline Chord Chord::tex_unescape() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_unescape", *this);
}

inline Chord Chord::tex_remove_math_delimiters() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_math_delimiters", *this);
}

inline Chord Chord::tex_add_math_delimiters() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_add_math_delimiters", *this);
}

inline Chord Chord::tex_math_only() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_math_only", *this);
}

inline Chord Chord::tex_text_only() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_text_only", *this);
}

inline Chord Chord::tex_remove_commands() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_commands", *this);
}

inline Chord Chord::tex_remove_environments() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_environments", *this);
}

inline Chord Chord::tex_remove_labels() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_labels", *this);
}

inline Chord Chord::tex_remove_refs() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_refs", *this);
}

inline Chord Chord::tex_remove_citations() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_citations", *this);
}

inline Chord Chord::tex_remove_graphics() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_graphics", *this);
}

inline Chord Chord::tex_remove_tables() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_tables", *this);
}

inline Chord Chord::tex_remove_figures() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_figures", *this);
}

inline Chord Chord::tex_remove_packages() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_packages", *this);
}

inline Chord Chord::tex_remove_bibliography() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_bibliography", *this);
}

inline Chord Chord::tex_to_text() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_text", *this);
}

inline Chord Chord::tex_to_markdown() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_markdown", *this);
}

inline Chord Chord::tex_to_html() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_html", *this);
}

inline Chord Chord::tex_balance_braces() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_balance_braces", *this);
}

inline Chord Chord::tex_balance_math_delims() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_balance_math_delims", *this);
}

inline Chord Chord::tex_canonicalize_macros() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_canonicalize_macros", *this);
}

inline Chord Chord::tex_expand_macros() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_expand_macros", *this);
}

inline Chord Chord::tex_inline_macros() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_inline_macros", *this);
}

inline Chord Chord::tex_simplify_fractions() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_simplify_fractions", *this);
}

inline Chord Chord::tex_simplify_roots() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_simplify_roots", *this);
}

inline Chord Chord::tex_simplify_spacing() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_simplify_spacing", *this);
}

inline Chord Chord::tex_remove_formatting() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_formatting", *this);
}

inline Chord Chord::tex_remove_accents() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_accents", *this);
}

inline Chord Chord::tex_remove_font_commands() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_font_commands", *this);
}

inline TokenBatch Chord::tex_detect_errors() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_detect_errors", *this);
}

inline bool Chord::tex_is_valid() const {
  return ::pluss::coden::_compat::invoke<bool>("tex_is_valid", *this);
}

inline TokenBatch Chord::tex_commands() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_commands", *this);
}

inline TokenBatch Chord::tex_command_names() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_command_names", *this);
}

inline TokenBatch Chord::tex_environments() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environments", *this);
}

inline TokenBatch Chord::tex_environment_names() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environment_names", *this);
}

inline TokenBatch Chord::tex_math_blocks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_math_blocks", *this);
}

inline TokenBatch Chord::tex_inline_math() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_inline_math", *this);
}

inline TokenBatch Chord::tex_display_math() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_display_math", *this);
}

inline TokenBatch Chord::tex_labels() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_labels", *this);
}

inline TokenBatch Chord::tex_refs() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_refs", *this);
}

inline TokenBatch Chord::tex_citations() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_citations", *this);
}

inline TokenBatch Chord::tex_packages() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_packages", *this);
}

inline TokenBatch Chord::tex_includes() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_includes", *this);
}

inline TokenBatch Chord::tex_macros_defined() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_macros_defined", *this);
}

inline TokenBatch Chord::tex_macros_used() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_macros_used", *this);
}

inline TokenBatch Chord::tex_bibliography_keys() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_bibliography_keys", *this);
}

inline TokenBatch Chord::tex_sections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_sections", *this);
}

inline TokenBatch Chord::tex_subsections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_subsections", *this);
}

inline TokenBatch Chord::tex_subsubsections() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_subsubsections", *this);
}

inline TokenBatch Chord::tex_equations() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_equations", *this);
}

inline TokenBatch Chord::tex_align_blocks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_align_blocks", *this);
}

inline TokenBatch Chord::tex_arrays() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_arrays", *this);
}

inline TokenBatch Chord::tex_tabular_blocks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_tabular_blocks", *this);
}

inline Chord Chord::tex_extract_environment(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_extract_environment", *this, name);
}

inline Chord Chord::tex_remove_environment(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_environment", *this, name);
}

inline TokenBatch Chord::tex_extract_command_args(::std::string_view cmd) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_command_args", *this, cmd);
}

inline Chord Chord::tex_replace_command(::std::string_view cmd, ::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_command", *this, cmd, repl);
}

inline Chord Chord::tex_set_macro(::std::string_view name, ::std::string_view body) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_set_macro", *this, name, body);
}

inline Chord Chord::tex_remove_macro(::std::string_view name) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_macro", *this, name);
}

inline Chord Chord::tex_wrap_inline_math(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_wrap_inline_math", *this, delim);
}

inline Chord Chord::tex_wrap_display_math(::std::string_view delim) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_wrap_display_math", *this, delim);
}

inline TokenBatch Chord::nlp_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_tokens", *this);
}

inline TokenBatch Chord::nlp_words() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_words", *this);
}

inline TokenBatch Chord::nlp_sentences() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_sentences", *this);
}

inline TokenBatch Chord::nlp_stopwords_removed_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_stopwords_removed_tokens", *this);
}

inline TokenBatch Chord::nlp_stems() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_stems", *this);
}

inline TokenBatch Chord::nlp_lemmas() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_lemmas", *this);
}

inline TokenBatch Chord::nlp_pos_tags() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_pos_tags", *this);
}

inline TokenBatch Chord::nlp_named_entities() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_named_entities", *this);
}

inline TokenBatch Chord::nlp_noun_phrases() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_noun_phrases", *this);
}

inline TokenBatch Chord::nlp_verb_phrases() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_verb_phrases", *this);
}

inline TokenBatch Chord::nlp_chunks() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_chunks", *this);
}

inline TokenBatch Chord::nlp_hashtags() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_hashtags", *this);
}

inline TokenBatch Chord::nlp_mentions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_mentions", *this);
}

inline TokenBatch Chord::nlp_urls() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_urls", *this);
}

inline TokenBatch Chord::nlp_emails() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_emails", *this);
}

inline TokenBatch Chord::nlp_numbers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_numbers", *this);
}

inline Chord Chord::nlp_language() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_language", *this);
}

inline Chord Chord::nlp_script() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_script", *this);
}

inline Chord Chord::nlp_clean() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_clean", *this);
}

inline Chord Chord::nlp_normalize_unicode() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_unicode", *this);
}

inline Chord Chord::nlp_normalize_punct() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_punct", *this);
}

inline Chord Chord::nlp_remove_stopwords() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_stopwords", *this);
}

inline Chord Chord::nlp_remove_urls() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_urls", *this);
}

inline Chord Chord::nlp_remove_emails() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_emails", *this);
}

inline Chord Chord::nlp_remove_mentions() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_mentions", *this);
}

inline Chord Chord::nlp_remove_hashtags() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_hashtags", *this);
}

inline Chord Chord::nlp_remove_numbers() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_numbers", *this);
}

inline Chord Chord::nlp_remove_punct() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_punct", *this);
}

inline Chord Chord::nlp_remove_diacritics() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_diacritics", *this);
}

inline Chord Chord::nlp_expand_contractions() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_expand_contractions", *this);
}

inline Chord Chord::nlp_simplify_whitespace() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_simplify_whitespace", *this);
}

inline Chord Chord::nlp_sentencecase() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_sentencecase", *this);
}

inline bool Chord::nlp_is_language(::std::string_view lang) const {
  return ::pluss::coden::_compat::invoke<bool>("nlp_is_language", *this, lang);
}

inline TokenBatch Chord::tokenize_words(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_words", *this, flags);
}

inline TokenBatch Chord::tokenize_sentences(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tokenize_sentences", *this, flags);
}

inline Chord Chord::normalize(::std::int64_t form) const {
  return ::pluss::coden::_compat::invoke<Chord>("normalize", *this, form);
}

inline Chord Chord::casefold(::std::string_view locale) const {
  return ::pluss::coden::_compat::invoke<Chord>("casefold", *this, locale);
}

inline Chord Chord::remove_punctuation() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_punctuation", *this);
}

inline Chord Chord::remove_digits() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_digits", *this);
}

inline Chord Chord::remove_letters() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_letters", *this);
}

inline Chord Chord::remove_symbols() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_symbols", *this);
}

inline Chord Chord::remove_emojis() const {
  return ::pluss::coden::_compat::invoke<Chord>("remove_emojis", *this);
}

inline Chord Chord::keep_alnum() const {
  return ::pluss::coden::_compat::invoke<Chord>("keep_alnum", *this);
}

inline Chord Chord::keep_alpha() const {
  return ::pluss::coden::_compat::invoke<Chord>("keep_alpha", *this);
}

inline Chord Chord::keep_digits() const {
  return ::pluss::coden::_compat::invoke<Chord>("keep_digits", *this);
}

inline Chord Chord::keep_ascii() const {
  return ::pluss::coden::_compat::invoke<Chord>("keep_ascii", *this);
}

inline Chord Chord::keep_printable() const {
  return ::pluss::coden::_compat::invoke<Chord>("keep_printable", *this);
}

inline Chord Chord::normalize_quotes() const {
  return ::pluss::coden::_compat::invoke<Chord>("normalize_quotes", *this);
}

inline Chord Chord::normalize_dashes() const {
  return ::pluss::coden::_compat::invoke<Chord>("normalize_dashes", *this);
}

inline Chord Chord::normalize_ellipsis() const {
  return ::pluss::coden::_compat::invoke<Chord>("normalize_ellipsis", *this);
}

inline Chord Chord::strip_accents() const {
  return ::pluss::coden::_compat::invoke<Chord>("strip_accents", *this);
}

inline Chord Chord::strip_diacritics() const {
  return ::pluss::coden::_compat::invoke<Chord>("strip_diacritics", *this);
}

inline Chord Chord::strip_zero_width() const {
  return ::pluss::coden::_compat::invoke<Chord>("strip_zero_width", *this);
}

inline Chord Chord::strip_bom() const {
  return ::pluss::coden::_compat::invoke<Chord>("strip_bom", *this);
}

inline Chord Chord::collapse_repeats() const {
  return ::pluss::coden::_compat::invoke<Chord>("collapse_repeats", *this);
}

inline Chord Chord::collapse_punct() const {
  return ::pluss::coden::_compat::invoke<Chord>("collapse_punct", *this);
}

inline Chord Chord::collapse_spaces() const {
  return ::pluss::coden::_compat::invoke<Chord>("collapse_spaces", *this);
}

inline Chord Chord::collapse_newlines() const {
  return ::pluss::coden::_compat::invoke<Chord>("collapse_newlines", *this);
}

inline Chord Chord::ensure_prefix(::std::string_view prefix) const {
  return ::pluss::coden::_compat::invoke<Chord>("ensure_prefix", *this, prefix);
}

inline Chord Chord::ensure_suffix(::std::string_view suffix) const {
  return ::pluss::coden::_compat::invoke<Chord>("ensure_suffix", *this, suffix);
}

inline Chord Chord::surround(::std::string_view left, ::std::string_view right) const {
  return ::pluss::coden::_compat::invoke<Chord>("surround", *this, left, right);
}

inline Chord Chord::quote() const {
  return ::pluss::coden::_compat::invoke<Chord>("quote", *this);
}

inline Chord Chord::unquote() const {
  return ::pluss::coden::_compat::invoke<Chord>("unquote", *this);
}

inline TokenBatch Chord::split_on_punct() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_on_punct", *this);
}

inline TokenBatch Chord::split_on_camelcase() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_on_camelcase", *this);
}

inline TokenBatch Chord::split_on_snakecase() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_on_snakecase", *this);
}

inline TokenBatch Chord::split_on_casechange() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_on_casechange", *this);
}

inline TokenBatch Chord::split_csv() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_csv", *this);
}

inline TokenBatch Chord::split_tsv() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_tsv", *this);
}

inline TokenBatch Chord::split_unicode_words() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_unicode_words", *this);
}

inline TokenBatch Chord::unique_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("unique_tokens", *this);
}

inline TokenBatch Chord::sort_tokens() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("sort_tokens", *this);
}

inline TokenBatch Chord::filter_tokens(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("filter_tokens", *this, pattern);
}

inline TokenBatch Chord::map_tokens(::std::string_view op) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("map_tokens", *this, op);
}

inline Chord Chord::strip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("strip", *this, chars);
}

inline Chord Chord::lstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("lstrip", *this, chars);
}

inline Chord Chord::rstrip(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("rstrip", *this, chars);
}

inline Chord Chord::trim(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("trim", *this, chars);
}

inline Chord Chord::trim_left(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("trim_left", *this, chars);
}

inline Chord Chord::trim_right(::std::string_view chars) const {
  return ::pluss::coden::_compat::invoke<Chord>("trim_right", *this, chars);
}

inline TokenBatch Chord::find_all(::std::string_view needle) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all", *this, needle);
}

inline TokenBatch Chord::find_all(::std::string_view needle, ::std::int64_t start) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all", *this, needle, start);
}

inline TokenBatch Chord::find_all_regex(::std::string_view pattern) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all_regex", *this, pattern);
}

inline TokenBatch Chord::find_all_regex(::std::string_view pattern, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("find_all_regex", *this, pattern, flags);
}

inline TokenBatch Chord::split(::std::string_view delim, ::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split", *this, delim, limit);
}

inline TokenBatch Chord::split_lines(bool keep_ends) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_lines", *this, keep_ends);
}

inline TokenBatch Chord::split_whitespace(::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("split_whitespace", *this, limit);
}

inline TokenBatch Chord::regex_split(::std::string_view pattern, ::std::int64_t limit) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_split", *this, pattern, limit);
}

inline TokenBatch Chord::regex_findall(::std::string_view pattern, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("regex_findall", *this, pattern, flags);
}

inline Chord Chord::regex_replace(::std::string_view pattern, ::std::string_view repl, ::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("regex_replace", *this, pattern, repl, flags);
}

inline TokenBatch Chord::tex_extract_env_equation() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_equation", *this);
}

inline Chord Chord::tex_remove_env_equation() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_equation", *this);
}

inline TokenBatch Chord::tex_extract_env_align() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_align", *this);
}

inline Chord Chord::tex_remove_env_align() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_align", *this);
}

inline TokenBatch Chord::tex_extract_env_align_() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_align_", *this);
}

inline Chord Chord::tex_remove_env_align_() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_align_", *this);
}

inline TokenBatch Chord::tex_extract_env_gather() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_gather", *this);
}

inline Chord Chord::tex_remove_env_gather() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_gather", *this);
}

inline TokenBatch Chord::tex_extract_env_multline() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_multline", *this);
}

inline Chord Chord::tex_remove_env_multline() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_multline", *this);
}

inline TokenBatch Chord::tex_extract_env_array() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_array", *this);
}

inline Chord Chord::tex_remove_env_array() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_array", *this);
}

inline TokenBatch Chord::tex_extract_env_cases() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_cases", *this);
}

inline Chord Chord::tex_remove_env_cases() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_cases", *this);
}

inline TokenBatch Chord::tex_extract_env_tabular() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_tabular", *this);
}

inline Chord Chord::tex_remove_env_tabular() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_tabular", *this);
}

inline TokenBatch Chord::tex_extract_env_figure() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_figure", *this);
}

inline Chord Chord::tex_remove_env_figure() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_figure", *this);
}

inline TokenBatch Chord::tex_extract_env_table() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_table", *this);
}

inline Chord Chord::tex_remove_env_table() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_table", *this);
}

inline TokenBatch Chord::tex_extract_env_itemize() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_itemize", *this);
}

inline Chord Chord::tex_remove_env_itemize() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_itemize", *this);
}

inline TokenBatch Chord::tex_extract_env_enumerate() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_env_enumerate", *this);
}

inline Chord Chord::tex_remove_env_enumerate() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_env_enumerate", *this);
}

inline TokenBatch Chord::tex_headings() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_headings", *this);
}

inline TokenBatch Chord::tex_headings(::std::int64_t max_depth) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_headings", *this, max_depth);
}

inline TokenBatch Chord::tex_equations(::std::int64_t max_count) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_equations", *this, max_count);
}

inline TokenBatch Chord::tex_math_blocks(::std::int64_t max_count) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_math_blocks", *this, max_count);
}

inline TokenBatch Chord::tex_commands(bool with_args) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_commands", *this, with_args);
}

inline TokenBatch Chord::tex_environments(bool with_body) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_environments", *this, with_body);
}

inline Chord Chord::tex_expand_macros(::std::int64_t max_passes) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_expand_macros", *this, max_passes);
}

inline Chord Chord::tex_sanitize(::std::int64_t level) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_sanitize", *this, level);
}

inline Chord Chord::tex_to_text(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_text", *this, mode);
}

inline Chord Chord::tex_to_markdown(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_markdown", *this, mode);
}

inline Chord Chord::tex_to_html(::std::int64_t mode) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_to_html", *this, mode);
}

inline TokenBatch Chord::nlp_tokens(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_tokens", *this, flags);
}

inline TokenBatch Chord::nlp_words(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_words", *this, flags);
}

inline TokenBatch Chord::nlp_sentences(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_sentences", *this, flags);
}

inline Chord Chord::nlp_clean(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_clean", *this, flags);
}

inline Chord Chord::nlp_remove_stopwords(::std::string_view lang) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_stopwords", *this, lang);
}

inline TokenBatch Chord::nlp_stopwords(::std::string_view lang) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_stopwords", *this, lang);
}

inline TokenBatch Chord::nlp_named_entities(::std::string_view scheme) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_named_entities", *this, scheme);
}

inline TokenBatch Chord::nlp_pos_tags(::std::string_view tagset) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_pos_tags", *this, tagset);
}

inline Chord Chord::nlp_language_confidence() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_language_confidence", *this);
}

inline Chord Chord::nlp_normalize_unicode(::std::int64_t form) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_unicode", *this, form);
}

inline Chord Chord::nlp_normalize_punct(bool ascii_only) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_punct", *this, ascii_only);
}

inline TokenBatch Chord::nlp_keywords(::std::int64_t k) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_keywords", *this, k);
}

inline TokenBatch Chord::nlp_keyphrases(::std::int64_t k) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_keyphrases", *this, k);
}

inline TokenBatch Chord::nlp_bigrams() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_bigrams", *this);
}

inline TokenBatch Chord::nlp_trigrams() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_trigrams", *this);
}

inline TokenBatch Chord::nlp_char_ngrams(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_char_ngrams", *this, n);
}

inline TokenBatch Chord::nlp_word_ngrams(::std::int64_t n) const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_word_ngrams", *this, n);
}

inline Chord Chord::tex_strip_math() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_math", *this);
}

inline Chord Chord::tex_strip_text() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_text", *this);
}

inline Chord Chord::tex_strip_environments() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_environments", *this);
}

inline Chord Chord::tex_strip_commands() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_strip_commands", *this);
}

inline Chord Chord::tex_collapse_macros() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_collapse_macros", *this);
}

inline Chord Chord::tex_expand_inputs() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_expand_inputs", *this);
}

inline Chord Chord::tex_resolve_includes() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_resolve_includes", *this);
}

inline Chord Chord::tex_resolve_graphics() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_resolve_graphics", *this);
}

inline Chord Chord::tex_remove_color() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_color", *this);
}

inline Chord Chord::tex_remove_hyperref() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_hyperref", *this);
}

inline Chord Chord::tex_remove_urls() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_urls", *this);
}

inline TokenBatch Chord::tex_extract_plain_math() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_plain_math", *this);
}

inline TokenBatch Chord::tex_extract_symbols() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_symbols", *this);
}

inline TokenBatch Chord::tex_extract_greek() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_greek", *this);
}

inline TokenBatch Chord::tex_extract_operators() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_operators", *this);
}

inline TokenBatch Chord::tex_extract_functions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_functions", *this);
}

inline TokenBatch Chord::tex_extract_numbers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_numbers", *this);
}

inline TokenBatch Chord::tex_extract_identifiers() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("tex_extract_identifiers", *this);
}

inline Chord Chord::tex_convert_frac_to_slash() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_convert_frac_to_slash", *this);
}

inline Chord Chord::tex_convert_sqrt_to_pow() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_convert_sqrt_to_pow", *this);
}

inline Chord Chord::tex_convert_pm_to_plusminus() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_convert_pm_to_plusminus", *this);
}

inline Chord Chord::tex_remove_left_right() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_left_right", *this);
}

inline Chord Chord::tex_remove_big_delims() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_big_delims", *this);
}

inline Chord Chord::tex_remove_spacing_cmds() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_remove_spacing_cmds", *this);
}

inline Chord Chord::tex_standardize_vectors() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_vectors", *this);
}

inline Chord Chord::tex_standardize_matrices() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_matrices", *this);
}

inline Chord Chord::tex_standardize_integrals() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_integrals", *this);
}

inline Chord Chord::tex_standardize_summations() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_summations", *this);
}

inline Chord Chord::tex_standardize_limits() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_limits", *this);
}

inline Chord Chord::tex_standardize_subscripts() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_subscripts", *this);
}

inline Chord Chord::tex_standardize_superscripts() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_superscripts", *this);
}

inline Chord Chord::tex_standardize_primes() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_primes", *this);
}

inline Chord Chord::tex_standardize_dots() const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_standardize_dots", *this);
}

inline Chord Chord::tex_replace_alpha(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_alpha", *this, repl);
}

inline Chord Chord::tex_replace_beta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_beta", *this, repl);
}

inline Chord Chord::tex_replace_gamma(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_gamma", *this, repl);
}

inline Chord Chord::tex_replace_delta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_delta", *this, repl);
}

inline Chord Chord::tex_replace_epsilon(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_epsilon", *this, repl);
}

inline Chord Chord::tex_replace_varepsilon(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_varepsilon", *this, repl);
}

inline Chord Chord::tex_replace_zeta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_zeta", *this, repl);
}

inline Chord Chord::tex_replace_eta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_eta", *this, repl);
}

inline Chord Chord::tex_replace_theta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_theta", *this, repl);
}

inline Chord Chord::tex_replace_vartheta(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_vartheta", *this, repl);
}

inline Chord Chord::tex_replace_iota(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_iota", *this, repl);
}

inline Chord Chord::tex_replace_kappa(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_kappa", *this, repl);
}

inline Chord Chord::tex_replace_lambda(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_lambda", *this, repl);
}

inline Chord Chord::tex_replace_mu(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_mu", *this, repl);
}

inline Chord Chord::tex_replace_nu(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_nu", *this, repl);
}

inline Chord Chord::tex_replace_xi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_xi", *this, repl);
}

inline Chord Chord::tex_replace_pi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_pi", *this, repl);
}

inline Chord Chord::tex_replace_varpi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_varpi", *this, repl);
}

inline Chord Chord::tex_replace_rho(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_rho", *this, repl);
}

inline Chord Chord::tex_replace_varrho(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_varrho", *this, repl);
}

inline Chord Chord::tex_replace_sigma(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_sigma", *this, repl);
}

inline Chord Chord::tex_replace_varsigma(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_varsigma", *this, repl);
}

inline Chord Chord::tex_replace_tau(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_tau", *this, repl);
}

inline Chord Chord::tex_replace_upsilon(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_upsilon", *this, repl);
}

inline Chord Chord::tex_replace_phi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_phi", *this, repl);
}

inline Chord Chord::tex_replace_varphi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_varphi", *this, repl);
}

inline Chord Chord::tex_replace_chi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_chi", *this, repl);
}

inline Chord Chord::tex_replace_psi(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_psi", *this, repl);
}

inline Chord Chord::tex_replace_omega(::std::string_view repl) const {
  return ::pluss::coden::_compat::invoke<Chord>("tex_replace_omega", *this, repl);
}

inline Chord Chord::nlp_remove_html() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_html", *this);
}

inline Chord Chord::nlp_remove_markdown() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_markdown", *this);
}

inline Chord Chord::nlp_remove_code() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_code", *this);
}

inline Chord Chord::nlp_remove_math() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_math", *this);
}

inline Chord Chord::nlp_remove_latex() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_latex", *this);
}

inline Chord Chord::nlp_remove_brackets() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_brackets", *this);
}

inline Chord Chord::nlp_remove_parentheticals() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_parentheticals", *this);
}

inline Chord Chord::nlp_remove_quotes() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_quotes", *this);
}

inline Chord Chord::nlp_remove_bullets() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_bullets", *this);
}

inline Chord Chord::nlp_remove_headers() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_headers", *this);
}

inline Chord Chord::nlp_remove_footers() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_footers", *this);
}

inline Chord Chord::nlp_remove_boilerplate() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_boilerplate", *this);
}

inline Chord Chord::nlp_remove_tables() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_tables", *this);
}

inline Chord Chord::nlp_remove_nonprintable() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_nonprintable", *this);
}

inline Chord Chord::nlp_remove_accents() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_accents", *this);
}

inline Chord Chord::nlp_normalize_whitespace() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_whitespace", *this);
}

inline Chord Chord::nlp_normalize_newlines() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_newlines", *this);
}

inline Chord Chord::nlp_normalize_case() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_case", *this);
}

inline Chord Chord::nlp_normalize_numbers() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_normalize_numbers", *this);
}

inline Chord Chord::nlp_mask_numbers() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_numbers", *this);
}

inline Chord Chord::nlp_mask_emails() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_emails", *this);
}

inline Chord Chord::nlp_mask_urls() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_urls", *this);
}

inline Chord Chord::nlp_mask_mentions() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_mentions", *this);
}

inline Chord Chord::nlp_mask_hashtags() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_hashtags", *this);
}

inline Chord Chord::nlp_mask_dates() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_dates", *this);
}

inline Chord Chord::nlp_mask_times() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_times", *this);
}

inline Chord Chord::nlp_mask_currency() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_currency", *this);
}

inline Chord Chord::nlp_mask_pii() const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_mask_pii", *this);
}

inline Chord Chord::nlp_remove_html(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_html", *this, flags);
}

inline Chord Chord::nlp_remove_markdown(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_markdown", *this, flags);
}

inline Chord Chord::nlp_remove_code(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_code", *this, flags);
}

inline Chord Chord::nlp_remove_math(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_math", *this, flags);
}

inline Chord Chord::nlp_remove_latex(::std::int64_t flags) const {
  return ::pluss::coden::_compat::invoke<Chord>("nlp_remove_latex", *this, flags);
}

inline TokenBatch Chord::nlp_extract_acronyms() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_acronyms", *this);
}

inline TokenBatch Chord::nlp_extract_definitions() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_definitions", *this);
}

inline TokenBatch Chord::nlp_extract_terms() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_terms", *this);
}

inline TokenBatch Chord::nlp_extract_citations() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_citations", *this);
}

inline TokenBatch Chord::nlp_extract_bibliography() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_bibliography", *this);
}

inline TokenBatch Chord::nlp_extract_math_symbols() const {
  return ::pluss::coden::_compat::invoke<TokenBatch>("nlp_extract_math_symbols", *this);
}

} // namespace pluss::coden
