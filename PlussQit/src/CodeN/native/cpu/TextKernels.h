#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::coden::native::cpu {

// ASCII-only lowercasing.
::std::string lower_ascii(::std::string_view s);
::std::string upper_ascii(::std::string_view s);
::std::string title_ascii(::std::string_view s);
::std::string capitalize_ascii(::std::string_view s);
::std::string swapcase_ascii(::std::string_view s);

// ASCII whitespace strip.
::std::string strip_ascii(::std::string_view s);
::std::string lstrip_ascii(::std::string_view s);
::std::string rstrip_ascii(::std::string_view s);
::std::string strip_ascii(::std::string_view s, ::std::string_view chars);
::std::string lstrip_ascii(::std::string_view s, ::std::string_view chars);
::std::string rstrip_ascii(::std::string_view s, ::std::string_view chars);
::std::string collapse_whitespace_ascii(::std::string_view s);
::std::string dedent_ascii(::std::string_view s);
::std::string indent_ascii(::std::string_view s, ::std::int64_t spaces = 2);
::std::string remove_control_ascii(::std::string_view s);
::std::string remove_non_ascii(::std::string_view s);
::std::string to_ascii_lossy(::std::string_view s);
::std::string remove_punctuation_ascii(::std::string_view s);
::std::string remove_digits_ascii(::std::string_view s);
::std::string remove_letters_ascii(::std::string_view s);
::std::string remove_symbols_ascii(::std::string_view s);
::std::string remove_emojis_utf8(::std::string_view s);
::std::string keep_alnum_ascii(::std::string_view s);
::std::string keep_alpha_ascii(::std::string_view s);
::std::string keep_digits_ascii(::std::string_view s);
::std::string keep_printable_ascii(::std::string_view s);
::std::string normalize_quotes_utf8(::std::string_view s);
::std::string normalize_dashes_utf8(::std::string_view s);
::std::string normalize_ellipsis_utf8(::std::string_view s);
::std::string strip_accents_utf8(::std::string_view s);
::std::string strip_zero_width_utf8(::std::string_view s);
::std::string strip_bom_utf8(::std::string_view s);
::std::string collapse_repeats_ascii(::std::string_view s);
::std::string collapse_punct_ascii(::std::string_view s);
::std::string collapse_spaces_ascii(::std::string_view s);
::std::string collapse_newlines_ascii(::std::string_view s);
::std::string nlp_remove_urls_fast(::std::string_view s);
::std::string nlp_remove_emails_fast(::std::string_view s);
::std::string nlp_remove_mentions_fast(::std::string_view s);
::std::string nlp_remove_hashtags_fast(::std::string_view s);
::std::string nlp_remove_html_fast(::std::string_view s);
::std::string nlp_remove_markdown_fast(::std::string_view s);
::std::string nlp_remove_code_fast(::std::string_view s);
::std::string nlp_remove_math_fast(::std::string_view s);
::std::string nlp_remove_latex_fast(::std::string_view s);
::std::string nlp_remove_brackets_fast(::std::string_view s);
::std::string nlp_remove_parentheticals_fast(::std::string_view s);
::std::string nlp_remove_quotes_fast(::std::string_view s);
::std::string nlp_remove_bullets_fast(::std::string_view s);
::std::string nlp_remove_headers_fast(::std::string_view s);
::std::string nlp_remove_footers_fast(::std::string_view s);
::std::string nlp_remove_boilerplate_fast(::std::string_view s);
::std::string nlp_remove_tables_fast(::std::string_view s);
::std::string nlp_mask_numbers_fast(::std::string_view s);
::std::string nlp_normalize_numbers_fast(::std::string_view s);
::std::string nlp_mask_emails_fast(::std::string_view s);
::std::string nlp_mask_urls_fast(::std::string_view s);
::std::string nlp_mask_mentions_fast(::std::string_view s);
::std::string nlp_mask_hashtags_fast(::std::string_view s);
::std::string nlp_mask_dates_fast(::std::string_view s);
::std::string nlp_mask_times_fast(::std::string_view s);
::std::string nlp_mask_currency_fast(::std::string_view s);
::std::string nlp_mask_pii_fast(::std::string_view s, ::std::int64_t flags = 0);

// Byte substring contains.
bool contains_bytes(::std::string_view hay, ::std::string_view needle);

enum class LexCompareOp : ::std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

// Lexical byte compare on UTF-8/bytes payloads (byte-order semantics).
bool compare_lex_bytes(::std::string_view lhs, ::std::string_view rhs, LexCompareOp op) noexcept;

// FNV-1a 64-bit hash for byte payloads.
::std::uint64_t hash_bytes_fnv1a(::std::string_view s) noexcept;

// Batch kernels (input and output spans must have matching size).
void compare_lex_bytes_batch(::std::span<const ::std::string_view> lhs,
                             ::std::span<const ::std::string_view> rhs,
                             LexCompareOp op,
                             ::std::span<::std::uint8_t> out);
void hash_bytes_fnv1a_batch(::std::span<const ::std::string_view> in,
                            ::std::span<::std::uint64_t> out);

// Byte replace (non-overlapping, left-to-right).
::std::string replace_bytes(::std::string_view s, ::std::string_view from, ::std::string_view to);

// ASCII whitespace split, returning (offset,len) spans into the input view.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_whitespace_spans(::std::string_view s);

// Split by an explicit delimiter (non-empty), returning (offset,len) spans into the input view.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_delim_spans(::std::string_view s, ::std::string_view delim);

// Split on ASCII punctuation boundaries (delimiter = non-alnum).
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_on_punct_spans(::std::string_view s);

// Split on NLP-aware token boundaries. This keeps technical text such as
// C++, C#, URLs, emails, apostrophes, hyphenated words, and common
// abbreviations intact. When keep_punct=false, punctuation-only tokens are
// omitted from the output.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_nlp_token_spans(::std::string_view s,
                                                                                bool keep_punct = true);

// Split on ASCII word boundaries with NLP-aware technical token handling.
// Punctuation-only tokens are excluded.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_word_spans(::std::string_view s);

// ASCII sentence splitting with abbreviation/decimal-aware heuristics.
::std::vector<::std::pair<::std::size_t, ::std::size_t>> split_sentences_spans_ascii(::std::string_view s);

// Lightweight ASCII lemma normalization for token-like inputs.
::std::string simple_lemma_ascii(::std::string_view token);

struct AsciiTokenTraits {
  bool is_ascii = true;
  bool has_alpha = false;
  bool has_digit = false;
  bool has_hyphen = false;
  bool has_apostrophe = false;
  bool is_titlecase = false;
  bool is_uppercase = false;
  bool is_lowercase = false;
  bool is_number_like = false;
  bool is_url = false;
  bool is_email = false;
  bool is_punct_only = false;
};

// Lightweight ASCII token-shape helpers used by higher-level NLP runtimes.
AsciiTokenTraits analyze_ascii_token(::std::string_view token) noexcept;
bool is_ascii_number_like(::std::string_view token) noexcept;
bool is_ascii_titlecase_token(::std::string_view token) noexcept;
bool is_ascii_uppercase_token(::std::string_view token) noexcept;
bool is_ascii_url_token(::std::string_view token) noexcept;
bool is_ascii_email_token(::std::string_view token) noexcept;
::std::string token_shape_ascii(::std::string_view token);
::std::string prefix_ascii(::std::string_view token, ::std::size_t width = 3);
::std::string suffix_ascii(::std::string_view token, ::std::size_t width = 3);

// Lowercase with SIMD lower_ascii + single-pass token extraction/filtering.
void tokenize_words_ascii_lower_filtered(::std::string_view s,
                                         ::std::vector<::std::string>& out,
                                         bool alpha_only = false,
                                         bool alnum_only = false,
                                         ::std::int64_t min_len = 0,
                                         ::std::int64_t max_len = -1);

} // namespace pluss::coden::native::cpu
