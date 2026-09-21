#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden::ops_internal {

// Internal accessors for Base implementations (keeps impl() protected).
struct NoteAccess {
  static const ::pluss::flame::intrusive_ptr<NoteImpl>& impl(const Note& x) noexcept { return x.impl(); }
  static ::pluss::flame::intrusive_ptr<NoteImpl>& impl(Note& x) noexcept { return x.impl(); }
};

// ---- Factories ----
Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes);
Note note_from_utf8_packed(::std::vector<char>&& utf8_arena,
                           ::std::vector<::std::pair<::std::size_t, ::std::size_t>>&& spans,
                           ::std::span<const ::std::int64_t> sizes);
Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes);
Note note_from_bool(::std::span<const ::std::uint8_t> values,
                    ::std::span<const ::std::int64_t> sizes);
Note note_from_bool_storage(NoteImpl::BoolStorage values,
                            ::std::span<const ::std::int64_t> sizes);
Note note_from_int64(::std::span<const ::std::int64_t> values,
                     ::std::span<const ::std::int64_t> sizes);
Note note_from_int64_storage(NoteImpl::Int64Storage values,
                             ::std::span<const ::std::int64_t> sizes);
Note note_from_float64(::std::span<const double> values,
                       ::std::span<const ::std::int64_t> sizes);
Note note_from_float64_storage(NoteImpl::Float64Storage values,
                               ::std::span<const ::std::int64_t> sizes);
Note note_contiguous_view(const Note &source, ::std::int64_t offset,
                          ::std::span<const ::std::int64_t> sizes);




// ICU-optional Unicode-ish surface (no-op / ASCII fallback when ICU is absent).
// form: 0=NFC, 1=NFD, 2=NFKC, 3=NFKD

// Regex hooks (std::regex fallback; ICU optional later).

// Tokenization / splitting.

// TeX cleanup family (scalar semantics).

// ---- Note ops ----
Note lower(const Note& x);
Note upper(const Note& x);
Note title(const Note& x);
Note capitalize(const Note& x);
Note swapcase(const Note& x);
Note strip(const Note& x);
Note lstrip(const Note& x);
Note rstrip(const Note& x);
Note collapse_whitespace(const Note& x);
Note dedent(const Note& x);
Note indent(const Note& x);
Note remove_control_chars(const Note& x);
Note keep_ascii(const Note& x);
Note to_ascii(const Note& x);
Note remove_punctuation(const Note& x);
Note remove_digits(const Note& x);
Note remove_letters(const Note& x);
Note remove_symbols(const Note& x);
Note remove_emojis(const Note& x);
Note keep_alnum(const Note& x);
Note keep_alpha(const Note& x);
Note keep_digits(const Note& x);
Note keep_printable(const Note& x);
Note normalize_quotes(const Note& x);
Note normalize_dashes(const Note& x);
Note normalize_ellipsis(const Note& x);
Note strip_accents(const Note& x);
Note strip_zero_width(const Note& x);
Note strip_bom(const Note& x);
Note collapse_repeats(const Note& x);
Note collapse_punct(const Note& x);
Note collapse_spaces(const Note& x);
Note collapse_newlines(const Note& x);
Note nlp_clean(const Note& x);
Note nlp_normalize_punct(const Note& x, bool ascii_only = false);
Note nlp_normalize_numbers(const Note& x);
Note nlp_expand_contractions(const Note& x);
Note nlp_remove_stopwords(const Note& x);
Note nlp_remove_stopwords(const Note& x, ::std::string_view lang);
Note nlp_language(const Note& x);
Note nlp_script(const Note& x);
Note nlp_sentiment(const Note& x);
Note nlp_toxicity(const Note& x);
Note nlp_readability_grade(const Note& x);
Note nlp_readability_level(const Note& x);
Note nlp_spam_score(const Note& x);
Note nlp_formality(const Note& x);
Note nlp_subjectivity(const Note& x);
Note to_utf8_strict(const Note& x);
Note to_utf8_lossy(const Note& x);
Note from_latin1(const Note& x);
Note to_latin1_lossy(const Note& x);
Note from_windows1252(const Note& x);
Note to_windows1252_lossy(const Note& x);
Note transliterate_ascii(const Note& x);
Note transliterate_latin(const Note& x);
Note transliterate_greek(const Note& x);
Note transliterate_cyrillic(const Note& x);
Note phonetic_soundex(const Note& x);
Note phonetic_metaphone(const Note& x);
Note nlp_remove_urls(const Note& x);
Note nlp_remove_emails(const Note& x);
Note nlp_remove_mentions(const Note& x);
Note nlp_remove_hashtags(const Note& x);
Note nlp_remove_html(const Note& x);
Note nlp_remove_markdown(const Note& x);
Note nlp_remove_code(const Note& x);
Note nlp_remove_math(const Note& x);
Note nlp_remove_latex(const Note& x);
Note nlp_remove_brackets(const Note& x);
Note nlp_remove_parentheticals(const Note& x);
Note nlp_remove_quotes(const Note& x);
Note nlp_remove_bullets(const Note& x);
Note nlp_remove_headers(const Note& x);
Note nlp_remove_footers(const Note& x);
Note nlp_remove_boilerplate(const Note& x);
Note nlp_remove_tables(const Note& x);
Note nlp_mask_numbers(const Note& x);
Note nlp_mask_emails(const Note& x);
Note nlp_mask_urls(const Note& x);
Note nlp_mask_mentions(const Note& x);
Note nlp_mask_hashtags(const Note& x);
Note nlp_mask_dates(const Note& x);
Note nlp_mask_times(const Note& x);
Note nlp_mask_currency(const Note& x);
Note nlp_mask_pii(const Note& x, ::std::int64_t flags = 0);

Note strip(const Note& x, ::std::string_view chars);
Note lstrip(const Note& x, ::std::string_view chars);
Note rstrip(const Note& x, ::std::string_view chars);

Note replace(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_first(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_last(const Note& x, ::std::string_view from, ::std::string_view to);
Note replace_n(const Note& x, ::std::string_view from, ::std::string_view to, ::std::int64_t n);
Note remove(const Note& x, ::std::string_view needle);
Note remove_all(const Note& x, ::std::string_view needle);
Note remove_prefix(const Note& x, ::std::string_view prefix);
Note remove_suffix(const Note& x, ::std::string_view suffix);
Note ensure_prefix(const Note& x, ::std::string_view prefix);
Note ensure_suffix(const Note& x, ::std::string_view suffix);
Note surround(const Note& x, ::std::string_view left, ::std::string_view right);
Note quote(const Note& x);
Note unquote(const Note& x);
Note base64_encode(const Note& x);
Note base64_decode(const Note& x);
Note hex_encode(const Note& x);
Note hex_decode(const Note& x);
Note url_encode(const Note& x);
Note url_decode(const Note& x);
Note html_escape(const Note& x);
Note html_unescape(const Note& x);
Note json_escape(const Note& x);
Note contains(const Note& x, ::std::string_view needle);
Note reshape(const Note& x, ::std::span<const ::std::int64_t> sizes);
Note transpose(const Note& x, ::std::int64_t dim0, ::std::int64_t dim1);
Note permute(const Note& x, ::std::span<const ::std::int64_t> dims);
Note select(const Note& x, ::std::int64_t dim, ::std::int64_t index);
Note narrow(const Note& x, ::std::int64_t dim, ::std::int64_t start, ::std::int64_t length);
Note contiguous(const Note& x);
Note flatten(const Note& x);
Note unsqueeze(const Note& x, ::std::int64_t dim);
Note squeeze(const Note& x, ::std::int64_t dim);
Note repeat(const Note& x, ::std::span<const ::std::int64_t> repeats);
Note repeat_interleave(const Note& x, ::std::int64_t repeats, ::std::int64_t dim);
NoteImpl::Int64Storage note_argsort_indices_1d(
    const Note& x,
    bool descending = false,
    bool stable = true);
Note sort(const Note& x, ::std::int64_t dim = -1);
Note unique(const Note& x, ::std::int64_t dim = -1);
Note deduplicate(const Note& x, ::std::int64_t dim = -1);

Note normalize(const Note& x, ::std::int64_t form);
Note casefold(const Note& x, ::std::string_view locale = {});

Note regex_match(const Note& x, ::std::string_view pattern);
Note regex_search(const Note& x, ::std::string_view pattern);
Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl);
Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl,
                   ::std::int64_t flags);

TokenBatch split_whitespace(const Note& x);
TokenBatch split(const Note& x, ::std::string_view delim);
TokenBatch split_whitespace(const Note& x, ::std::int64_t limit);
TokenBatch split(const Note& x, ::std::string_view delim, ::std::int64_t limit);
TokenBatch split_lines(const Note& x, bool keep_ends = false);
TokenBatch split_chars(const Note& x);
TokenBatch tokenize_words(const Note& x, ::std::int64_t flags = 0);
TokenBatch tokenize_sentences(const Note& x);
TokenBatch tokenize_punct(const Note& x);
TokenBatch tokenize_regex(const Note& x, ::std::string_view pattern);
TokenBatch regex_findall(const Note& x, ::std::string_view pattern, ::std::int64_t flags = 0);
TokenBatch regex_split(const Note& x, ::std::string_view pattern, ::std::int64_t limit = -1);
TokenBatch token_feature(const Note& x,
                         ::std::string_view op,
                         ::std::int64_t iarg = -1,
                         ::std::string_view sarg = {});
TokenBatch intersect_acronyms(const Note& x, const Note& other);
TokenBatch intersect_citations(const Note& x, const Note& other);
TokenBatch intersect_math_symbols(const Note& x, const Note& other);
TokenBatch intersect_tokens(const Note& x, const Note& other);
TokenBatch intersect_terms(const Note& x, const Note& other);
TokenBatch diff_words(const Note& x, const Note& other);
TokenBatch diff_lines(const Note& x, const Note& other);
TokenBatch diff_chars(const Note& x, const Note& other);
TokenBatch align_tokens(const Note& x, const Note& other);
TokenBatch align_sentences(const Note& x, const Note& other);
Note jaccard_acronyms(const Note& x, const Note& other);
Note jaccard_citations(const Note& x, const Note& other);
Note jaccard_math_symbols(const Note& x, const Note& other);
Note jaccard_tokens(const Note& x, const Note& other);
Note jaccard_terms(const Note& x, const Note& other);
Note overlap_acronyms(const Note& x, const Note& other);
Note overlap_citations(const Note& x, const Note& other);
Note overlap_math_symbols(const Note& x, const Note& other);
Note overlap_tokens(const Note& x, const Note& other);
Note overlap_terms(const Note& x, const Note& other);

Note tex_strip_comments(const Note& x);
Note tex_strip_preamble(const Note& x);
Note tex_strip_metadata(const Note& x);
Note tex_remove_commands(const Note& x);
Note tex_remove_environments(const Note& x);
Note tex_remove_environment(const Note& x, ::std::string_view name);
Note tex_remove_labels(const Note& x);
Note tex_remove_refs(const Note& x);
Note tex_remove_citations(const Note& x);
Note tex_remove_graphics(const Note& x);
Note tex_remove_tables(const Note& x);
Note tex_remove_figures(const Note& x);
Note tex_remove_packages(const Note& x);
Note tex_remove_bibliography(const Note& x);
Note tex_remove_color(const Note& x);
Note tex_remove_hyperref(const Note& x);
Note tex_remove_urls(const Note& x);
Note tex_remove_left_right(const Note& x);
Note tex_remove_big_delims(const Note& x);
Note tex_remove_spacing_cmds(const Note& x);
Note tex_balance_braces(const Note& x);
Note tex_balance_math_delims(const Note& x);
Note tex_canonicalize_macros(const Note& x);
Note tex_expand_macros(const Note& x, ::std::int64_t max_passes = 1);
Note tex_inline_macros(const Note& x);
Note tex_collapse_macros(const Note& x);
Note tex_expand_inputs(const Note& x);
Note tex_resolve_graphics(const Note& x);
Note tex_set_macro(const Note& x, ::std::string_view name, ::std::string_view body);
Note tex_remove_macro(const Note& x, ::std::string_view name);
Note tex_wrap_inline_math(const Note& x, ::std::string_view delim);
Note tex_wrap_display_math(const Note& x, ::std::string_view delim);
Note tex_simplify_fractions(const Note& x);
Note tex_simplify_roots(const Note& x);
Note tex_simplify_spacing(const Note& x);
Note tex_convert_frac_to_slash(const Note& x);
Note tex_convert_sqrt_to_pow(const Note& x);
Note tex_convert_pm_to_plusminus(const Note& x);
Note tex_standardize_vectors(const Note& x);
Note tex_standardize_matrices(const Note& x);
Note tex_standardize_integrals(const Note& x);
Note tex_standardize_summations(const Note& x);
Note tex_standardize_limits(const Note& x);
Note tex_standardize_subscripts(const Note& x);
Note tex_standardize_superscripts(const Note& x);
Note tex_standardize_primes(const Note& x);
Note tex_standardize_dots(const Note& x);
Note tex_remove_accents(const Note& x);
Note tex_replace_command(const Note& x, ::std::string_view cmd, ::std::string_view repl);
Note tex_to_text(const Note& x, ::std::int64_t mode = 0);
Note text_to_tex(const Note& x, ::std::int64_t mode = 0);
Note tex_render(const Note& x, ::std::int64_t mode = 0);
Note tex_sanitize(const Note& x, ::std::int64_t level = 0);
Note tex_normalize_newlines(const Note& x);
Note tex_dedup_blank_lines(const Note& x);
Note tex_fix_linebreaks(const Note& x);
Note tex_escape(const Note& x);
Note tex_unescape(const Note& x);
Note tex_remove_math_delimiters(const Note& x);
Note tex_add_math_delimiters(const Note& x);
Note tex_math_only(const Note& x);
Note tex_text_only(const Note& x);
Note tex_remove_formatting(const Note& x);
Note tex_remove_font_commands(const Note& x);
Note tex_to_markdown(const Note& x, ::std::int64_t mode = 0);
Note tex_to_html(const Note& x, ::std::int64_t mode = 0);

// Join flattened tokens into a scalar (0D) Note.
Note join(const Note& x, ::std::string_view sep = {});

} // namespace pluss::coden::ops_internal
