# CodeN text API method index

This document indexes the **Chord** (scalar text) and **Note** (tensor-of-text) APIs.

---

# Chord method index

This is a **navigation index**: semantics are described at a high level here, with exact
signatures left to the per-method reference pages.

## TeX / LaTeX (157)

- `Chord tex_strip_comments() const`
- `Chord tex_strip_preamble() const`
- `Chord tex_strip_metadata() const`
- `Chord tex_normalize_newlines() const`
- `Chord tex_dedup_blank_lines() const`
- `Chord tex_collapse_whitespace() const`
- `Chord tex_fix_linebreaks() const`
- `Chord tex_sanitize() const`
- `Chord tex_escape() const`
- `Chord tex_unescape() const`
- `Chord tex_remove_math_delimiters() const`
- `Chord tex_add_math_delimiters() const`
- `Chord tex_math_only() const`
- `Chord tex_text_only() const`
- `Chord tex_remove_commands() const`
- `Chord tex_remove_environments() const`
- `Chord tex_remove_labels() const`
- `Chord tex_remove_refs() const`
- `Chord tex_remove_citations() const`
- `Chord tex_remove_graphics() const`
- `Chord tex_remove_tables() const`
- `Chord tex_remove_figures() const`
- `Chord tex_remove_packages() const`
- `Chord tex_remove_bibliography() const`
- `Chord tex_to_text() const`
- `Chord tex_to_markdown() const`
- `Chord tex_to_html() const`
- `Chord tex_balance_braces() const`
- `Chord tex_balance_math_delims() const`
- `Chord tex_canonicalize_macros() const`
- `Chord tex_expand_macros() const`
- `Chord tex_inline_macros() const`
- `Chord tex_simplify_fractions() const`
- `Chord tex_simplify_roots() const`
- `Chord tex_remove_formatting() const`
- `Chord tex_remove_accents() const`
- `Chord tex_remove_font_commands() const`
- `TokenBatch tex_detect_errors() const`
- `bool tex_is_valid() const`
- `TokenBatch tex_commands() const`
- `TokenBatch tex_command_names() const`
- `TokenBatch tex_environments() const`
- `TokenBatch tex_environment_names() const`
- `TokenBatch tex_math_blocks() const`
- `TokenBatch tex_inline_math() const`
- `TokenBatch tex_display_math() const`
- `TokenBatch tex_labels() const`
- `TokenBatch tex_refs() const`
- `TokenBatch tex_citations() const`
- `TokenBatch tex_packages() const`
- `TokenBatch tex_includes() const`
- `TokenBatch tex_macros_defined() const`
- `TokenBatch tex_macros_used() const`
- `TokenBatch tex_bibliography_keys() const`
- `TokenBatch tex_sections() const`
- `TokenBatch tex_subsections() const`
- `TokenBatch tex_subsubsections() const`
- `TokenBatch tex_equations() const`
- `TokenBatch tex_align_blocks() const`
- `TokenBatch tex_arrays() const`
- `TokenBatch tex_tabular_blocks() const`
- `Chord tex_extract_environment(::std::string_view name) const`
- `Chord tex_remove_environment(::std::string_view name) const`
- `TokenBatch tex_extract_command_args(::std::string_view cmd) const`
- `Chord tex_replace_command(::std::string_view cmd, ::std::string_view repl) const`
- `Chord tex_set_macro(::std::string_view name, ::std::string_view body) const`
- `Chord tex_remove_macro(::std::string_view name) const`
- `Chord tex_wrap_inline_math(::std::string_view delim) const`
- `Chord tex_wrap_display_math(::std::string_view delim) const`
- `TokenBatch tex_extract_env_equation() const`
- `Chord tex_remove_env_equation() const`
- `TokenBatch tex_extract_env_align() const`
- `Chord tex_remove_env_align() const`
- `TokenBatch tex_extract_env_align_() const`
- `Chord tex_remove_env_align_() const`
- `TokenBatch tex_extract_env_gather() const`
- `Chord tex_remove_env_gather() const`
- `TokenBatch tex_extract_env_multline() const`
- `Chord tex_remove_env_multline() const`
- `TokenBatch tex_extract_env_array() const`
- `Chord tex_remove_env_array() const`
- `TokenBatch tex_extract_env_cases() const`
- `Chord tex_remove_env_cases() const`
- `TokenBatch tex_extract_env_tabular() const`
- `Chord tex_remove_env_tabular() const`
- `TokenBatch tex_extract_env_figure() const`
- `Chord tex_remove_env_figure() const`
- `TokenBatch tex_extract_env_table() const`
- `Chord tex_remove_env_table() const`
- `TokenBatch tex_extract_env_itemize() const`
- `Chord tex_remove_env_itemize() const`
- `TokenBatch tex_extract_env_enumerate() const`
- `Chord tex_remove_env_enumerate() const`
- `TokenBatch tex_headings() const`
- `Chord tex_strip_math() const`
- `Chord tex_strip_text() const`
- `Chord tex_strip_environments() const`
- `Chord tex_strip_commands() const`
- `Chord tex_collapse_macros() const`
- `Chord tex_expand_inputs() const`
- `Chord tex_resolve_includes() const`
- `Chord tex_resolve_graphics() const`
- `Chord tex_remove_color() const`
- `Chord tex_remove_hyperref() const`
- `Chord tex_remove_urls() const`
- `TokenBatch tex_extract_plain_math() const`
- `TokenBatch tex_extract_symbols() const`
- `TokenBatch tex_extract_greek() const`
- `TokenBatch tex_extract_operators() const`
- `TokenBatch tex_extract_functions() const`
- `TokenBatch tex_extract_numbers() const`
- `TokenBatch tex_extract_identifiers() const`
- `Chord tex_remove_left_right() const`
- `Chord tex_remove_big_delims() const`
- `Chord tex_remove_spacing_cmds() const`
- `Chord tex_standardize_vectors() const`
- `Chord tex_standardize_matrices() const`
- `Chord tex_standardize_integrals() const`
- `Chord tex_standardize_summations() const`
- `Chord tex_standardize_limits() const`
- `Chord tex_standardize_subscripts() const`
- `Chord tex_standardize_superscripts() const`
- `Chord tex_standardize_primes() const`
- `Chord tex_standardize_dots() const`
- `Chord tex_replace_alpha(::std::string_view repl) const`
- `Chord tex_replace_beta(::std::string_view repl) const`
- `Chord tex_replace_gamma(::std::string_view repl) const`
- `Chord tex_replace_delta(::std::string_view repl) const`
- `Chord tex_replace_epsilon(::std::string_view repl) const`
- `Chord tex_replace_varepsilon(::std::string_view repl) const`
- `Chord tex_replace_zeta(::std::string_view repl) const`
- `Chord tex_replace_eta(::std::string_view repl) const`
- `Chord tex_replace_theta(::std::string_view repl) const`
- `Chord tex_replace_vartheta(::std::string_view repl) const`
- `Chord tex_replace_iota(::std::string_view repl) const`
- `Chord tex_replace_kappa(::std::string_view repl) const`
- `Chord tex_replace_lambda(::std::string_view repl) const`
- `Chord tex_replace_mu(::std::string_view repl) const`
- `Chord tex_replace_nu(::std::string_view repl) const`
- `Chord tex_replace_xi(::std::string_view repl) const`
- `Chord tex_replace_pi(::std::string_view repl) const`
- `Chord tex_replace_varpi(::std::string_view repl) const`
- `Chord tex_replace_rho(::std::string_view repl) const`
- `Chord tex_replace_varrho(::std::string_view repl) const`
- `Chord tex_replace_sigma(::std::string_view repl) const`
- `Chord tex_replace_varsigma(::std::string_view repl) const`
- `Chord tex_replace_tau(::std::string_view repl) const`
- `Chord tex_replace_upsilon(::std::string_view repl) const`
- `Chord tex_replace_phi(::std::string_view repl) const`
- `Chord tex_replace_varphi(::std::string_view repl) const`
- `Chord tex_replace_chi(::std::string_view repl) const`
- `Chord tex_replace_psi(::std::string_view repl) const`
- `Chord tex_replace_omega(::std::string_view repl) const`

## NLP cleanup & normalization (64)

- `TokenBatch nlp_tokens() const`
- `TokenBatch nlp_words() const`
- `TokenBatch nlp_sentences() const`
- `TokenBatch nlp_stopwords_removed_tokens() const`
- `TokenBatch nlp_stems() const`
- `TokenBatch nlp_lemmas() const`
- `TokenBatch nlp_pos_tags() const`
- `TokenBatch nlp_named_entities() const`
- `TokenBatch nlp_noun_phrases() const`
- `TokenBatch nlp_verb_phrases() const`
- `TokenBatch nlp_chunks() const`
- `TokenBatch nlp_hashtags() const`
- `TokenBatch nlp_mentions() const`
- `TokenBatch nlp_urls() const`
- `TokenBatch nlp_emails() const`
- `TokenBatch nlp_numbers() const`
- `Chord nlp_language() const`
- `Chord nlp_script() const`
- `Chord nlp_clean() const`
- `Chord nlp_normalize_punct() const`
- `Chord nlp_remove_stopwords() const`
- `Chord nlp_remove_urls() const`
- `Chord nlp_remove_emails() const`
- `Chord nlp_remove_mentions() const`
- `Chord nlp_remove_hashtags() const`
- `Chord nlp_expand_contractions() const`
- `bool nlp_is_language(::std::string_view lang) const`
- `TokenBatch nlp_stopwords(::std::string_view lang) const`
- `Chord nlp_language_confidence() const`
- `TokenBatch nlp_keywords(::std::int64_t k) const`
- `TokenBatch nlp_keyphrases(::std::int64_t k) const`
- `TokenBatch nlp_bigrams() const`
- `TokenBatch nlp_trigrams() const`
- `TokenBatch nlp_char_ngrams(::std::int64_t n) const`
- `TokenBatch nlp_word_ngrams(::std::int64_t n) const`
- `Chord nlp_remove_html() const`
- `Chord nlp_remove_markdown() const`
- `Chord nlp_remove_code() const`
- `Chord nlp_remove_math() const`
- `Chord nlp_remove_latex() const`
- `Chord nlp_remove_brackets() const`
- `Chord nlp_remove_parentheticals() const`
- `Chord nlp_remove_quotes() const`
- `Chord nlp_remove_bullets() const`
- `Chord nlp_remove_headers() const`
- `Chord nlp_remove_footers() const`
- `Chord nlp_remove_boilerplate() const`
- `Chord nlp_remove_tables() const`
- `Chord nlp_normalize_numbers() const`
- `Chord nlp_mask_numbers() const`
- `Chord nlp_mask_emails() const`
- `Chord nlp_mask_urls() const`
- `Chord nlp_mask_mentions() const`
- `Chord nlp_mask_hashtags() const`
- `Chord nlp_mask_dates() const`
- `Chord nlp_mask_times() const`
- `Chord nlp_mask_currency() const`
- `Chord nlp_mask_pii() const`
- `TokenBatch nlp_extract_acronyms() const`
- `TokenBatch nlp_extract_definitions() const`
- `TokenBatch nlp_extract_terms() const`
- `TokenBatch nlp_extract_citations() const`
- `TokenBatch nlp_extract_bibliography() const`
- `TokenBatch nlp_extract_math_symbols() const`

## Tokenization & splitting (17)

- `TokenBatch split(::std::string_view delim) const`
- `TokenBatch split_whitespace() const`
- `TokenBatch split_lines() const`
- `TokenBatch split_sentences() const`
- `TokenBatch split_graphemes() const`
- `TokenBatch split_chars() const`
- `TokenBatch tokenize_words() const`
- `TokenBatch tokenize_sentences() const`
- `TokenBatch tokenize_punct() const`
- `TokenBatch tokenize_regex(::std::string_view pattern) const`
- `TokenBatch split_on_punct() const`
- `TokenBatch split_on_camelcase() const`
- `TokenBatch split_on_snakecase() const`
- `TokenBatch split_on_casechange() const`
- `TokenBatch split_csv() const`
- `TokenBatch split_tsv() const`
- `TokenBatch split_unicode_words() const`

## Regex (5)

- `bool regex_match(::std::string_view pattern) const`
- `bool regex_search(::std::string_view pattern) const`
- `Chord regex_replace(::std::string_view pattern, ::std::string_view repl) const`
- `TokenBatch regex_findall(::std::string_view pattern) const`
- `TokenBatch regex_split(::std::string_view pattern) const`

## Unicode & casing (17)

- `Chord lower() const`
- `Chord upper() const`
- `Chord title() const`
- `Chord capitalize() const`
- `Chord casefold() const`
- `Chord swapcase() const`
- `Chord normalize_spaces() const`
- `Chord to_ascii() const`
- `Chord to_nfkc() const`
- `Chord to_nfc() const`
- `Chord to_nfd() const`
- `Chord to_nfkd() const`
- `Chord normalize_quotes() const`
- `Chord normalize_dashes() const`
- `Chord normalize_ellipsis() const`
- `Chord strip_accents() const`
- `Chord strip_diacritics() const`

## Whitespace & layout (12)

- `Chord strip() const`
- `Chord lstrip() const`
- `Chord rstrip() const`
- `Chord trim() const`
- `Chord trim_left() const`
- `Chord trim_right() const`
- `Chord collapse_whitespace() const`
- `Chord dedent() const`
- `Chord indent() const`
- `bool is_whitespace() const`
- `Chord collapse_spaces() const`
- `Chord collapse_newlines() const`

## Filtering & cleaning (19)

- `Chord remove_control_chars() const`
- `Chord remove_non_ascii() const`
- `Chord remove_all(::std::string_view needle) const`
- `Chord remove_prefix(::std::string_view prefix) const`
- `Chord remove_suffix(::std::string_view suffix) const`
- `Chord remove_punctuation() const`
- `Chord remove_digits() const`
- `Chord remove_letters() const`
- `Chord remove_symbols() const`
- `Chord remove_emojis() const`
- `Chord keep_alnum() const`
- `Chord keep_alpha() const`
- `Chord keep_digits() const`
- `Chord keep_ascii() const`
- `Chord keep_printable() const`
- `Chord strip_zero_width() const`
- `Chord strip_bom() const`
- `Chord collapse_repeats() const`
- `Chord collapse_punct() const`

## Encoding / bytes (5)

- `bool is_bytes() const`
- `Chord base64_encode() const`
- `Chord base64_decode() const`
- `Chord hex_encode() const`
- `Chord hex_decode() const`

## Search & match (5)

- `bool contains(::std::string_view needle) const`
- `bool starts_with(::std::string_view prefix) const`
- `bool ends_with(::std::string_view suffix) const`
- `TokenBatch find_all_regex(::std::string_view pattern) const`

## Replace & transform (5)

- `Chord replace(::std::string_view from, ::std::string_view to) const`
- `Chord replace_first(::std::string_view from, ::std::string_view to) const`
- `Chord replace_last(::std::string_view from, ::std::string_view to) const`
- `Chord replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const`
- `TokenBatch map_tokens(::std::string_view op) const`

## Other (26)

- `Chord remove(::std::string_view needle) const`
- `bool is_ascii() const`
- `bool is_utf8_valid() const`
- `bool is_blank() const`
- `bool is_alpha() const`
- `bool is_alnum() const`
- `bool is_digit() const`
- `bool is_lower() const`
- `bool is_upper() const`
- `bool is_title() const`
- `TokenBatch ngrams_char(::std::int64_t n) const`
- `TokenBatch ngrams_word(::std::int64_t n) const`
- `Chord url_encode() const`
- `Chord url_decode() const`
- `Chord html_escape() const`
- `Chord html_unescape() const`
- `Chord json_escape() const`
- `Chord normalize(::std::int64_t form) const`
- `Chord ensure_prefix(::std::string_view prefix) const`
- `Chord ensure_suffix(::std::string_view suffix) const`
- `Chord surround(::std::string_view left, ::std::string_view right) const`
- `Chord quote() const`
- `Chord unquote() const`
- `TokenBatch unique_tokens() const`
- `TokenBatch sort_tokens() const`
- `TokenBatch filter_tokens(::std::string_view pattern) const`


---

# Note method index

This file is generated from the public surface of CodeN and is meant as a **navigation index**.
Semantics are described at a high level; see the corresponding header for exact signatures.

## TeX / LaTeX (1 accessor)

- `Note::TexAccessor tex() const noexcept`

The former flat `Note::tex_*` spellings have been removed. TeX operations
live on `note.tex()`; use the [Note TeX Accessor API Reference](sphinx/note_tex_accessor_api_reference.md) for the
canonical method inventory and per-operation pages.

## Accessors (2)

- `Note::NlpAccessor nlp() const noexcept`
- `Note::RegexAccessor regex() const noexcept`

## Tokenization & splitting (4)

- `TokenBatch split(::std::string_view delim) const`
- `TokenBatch split_whitespace() const`
- `TokenBatch split_lines() const`
- `TokenBatch split_chars() const`

## Unicode & casing (19)

- `Note lower() const`
- `Note upper() const`
- `Note title() const`
- `Note capitalize() const`
- `Note casefold() const`
- `Note swapcase() const`
- `Note to_ascii() const`
- `Note to_nfkc() const`
- `Note to_nfc() const`
- `Note to_nfd() const`
- `Note to_nfkd() const`
- `Note normalize_quotes() const`
- `Note normalize_dashes() const`
- `Note normalize_ellipsis() const`
- `Note strip_accents() const`
- `Note to_utf8_strict() const`
- `Note to_utf8_lossy() const`
- `Note to_latin1_lossy() const`
- `Note to_windows1252_lossy() const`

## Whitespace & layout (9)

- `Note strip() const`
- `Note lstrip() const`
- `Note rstrip() const`
- `Note collapse_whitespace() const`
- `Note dedent() const`
- `Note indent() const`
- `Note collapse_spaces() const`
- `Note collapse_newlines() const`
- `Note is_whitespace() const`

## Filtering & cleaning (18)

- `Note remove_control_chars() const`
- `Note remove_punctuation() const`
- `Note remove_digits() const`
- `Note remove_letters() const`
- `Note remove_symbols() const`
- `Note remove_emojis() const`
- `Note keep_alnum() const`
- `Note keep_alpha() const`
- `Note keep_digits() const`
- `Note keep_ascii() const`
- `Note keep_printable() const`
- `Note strip_zero_width() const`
- `Note strip_bom() const`
- `Note collapse_repeats() const`
- `Note collapse_punct() const`
- `Note remove_all(::std::string_view needle) const`
- `Note remove_prefix(::std::string_view prefix) const`
- `Note remove_suffix(::std::string_view suffix) const`

## Encoding / bytes (6)

- `bool is_bytes() const`
- `::std::vector<::std::int64_t> lengths_bytes() const`
- `Note base64_encode() const`
- `Note base64_decode() const`
- `Note hex_encode() const`
- `Note hex_decode() const`

## Search & match (7)

- `Note contains(::std::string_view needle) const`
- `Note starts_with(::std::string_view prefix) const`
- `Note ends_with(::std::string_view suffix) const`
- `Note contains_any_digit() const`
- `Note contains_any_alpha() const`
- `::std::vector<::std::int64_t> count_substring(::std::string_view needle) const`

## Replace & transform (4)

- `Note replace(::std::string_view from, ::std::string_view to) const`
- `Note replace_first(::std::string_view from, ::std::string_view to) const`
- `Note replace_last(::std::string_view from, ::std::string_view to) const`
- `Note replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const`

## Tensor / shape (Note) (9)

- `Chord join() const`
- `Note reshape(::std::span<const ::std::int64_t> sizes) const`
- `Note flatten() const`
- `Note sort() const`
- `Note argsort(bool descending = false, bool stable = true) const`
- `Note unique() const`
- `Note squeeze(::std::int64_t dim) const`
- `Note unsqueeze(::std::int64_t dim) const`
- `Note repeat(::pluss::msharp::RangeView::Int repeats) const`

## Other (33)

- `Note remove(::std::string_view needle) const`
- `Note ensure_prefix(::std::string_view prefix) const`
- `Note ensure_suffix(::std::string_view suffix) const`
- `Note surround(::std::string_view left, ::std::string_view right) const`
- `Note quote() const`
- `Note unquote() const`
- `Note is_ascii() const`
- `Note is_utf8_valid() const`
- `Note is_blank() const`
- `::std::vector<::std::int64_t> lengths_chars() const`
- `::std::vector<::std::int64_t> lengths_graphemes() const`
- `Note transpose(::std::int64_t dim0, ::std::int64_t dim1) const`
- `Note permute(::std::span<const ::std::int64_t> dims) const`
- `Note select(::std::int64_t dim, ::std::int64_t index) const`
- `Note narrow(::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) const`
- `Note contiguous() const`
- `Note url_encode() const`
- `Note url_decode() const`
- `Note html_escape() const`
- `Note html_unescape() const`
- `Note json_escape() const`
- `Note is_alpha() const`
- `Note is_alnum() const`
- `Note is_digit() const`
- `Note is_lower() const`
- `Note is_upper() const`
- `Note is_title() const`
- `Note from_latin1() const`
- `Note from_windows1252() const`
- `TokenBatch diff_lines(const Note& other) const`
- `TokenBatch diff_chars(const Note& other) const`
- `Note deduplicate() const`
- `Note repeat_interleave(::std::int64_t repeats, ::std::int64_t dim) const`

# Note::NlpAccessor method index

The accessor returned by `Note::nlp()` owns the linguistic surface. Its 100
canonical methods intentionally omit forwarding aliases.

## Cleanup, analysis, masking, and extraction

- `Note clean() const`
- `Note expand_contractions() const`
- `Note normalize_numbers() const`
- `Note normalize_punct(bool ascii_only = false) const`
- `Note remove_stopwords(::std::string_view language = "en") const`
- `Note language() const`
- `Note script() const`
- `Note sentiment() const`
- `Note toxicity() const`
- `Note readability_grade() const`
- `Note readability_level() const`
- `Note spam_score() const`
- `Note formality() const`
- `Note subjectivity() const`
- `Note mask_currency() const`
- `Note mask_dates() const`
- `Note mask_emails() const`
- `Note mask_hashtags() const`
- `Note mask_mentions() const`
- `Note mask_numbers() const`
- `Note mask_pii(::std::int64_t flags = 0) const`
- `Note mask_times() const`
- `Note mask_urls() const`
- `Note remove_boilerplate() const`
- `Note remove_brackets() const`
- `Note remove_bullets() const`
- `Note remove_code() const`
- `Note remove_emails() const`
- `Note remove_footers() const`
- `Note remove_hashtags() const`
- `Note remove_headers() const`
- `Note remove_html() const`
- `Note remove_latex() const`
- `Note remove_markdown() const`
- `Note remove_math() const`
- `Note remove_mentions() const`
- `Note remove_parentheticals() const`
- `Note remove_quotes() const`
- `Note remove_tables() const`
- `Note remove_urls() const`
- `TokenBatch tokens(::std::int64_t flags = 0) const`
- `TokenBatch sentences() const`
- `TokenBatch punct_tokens() const`
- `TokenBatch stems() const`
- `TokenBatch lemmas() const`
- `TokenBatch pos_tags() const`
- `TokenBatch named_entities() const`
- `TokenBatch keywords(::std::int64_t count) const`
- `TokenBatch keyphrases(::std::int64_t count) const`
- `TokenBatch char_ngrams(::std::int64_t width) const`
- `TokenBatch word_ngrams(::std::int64_t width) const`
- `TokenBatch hashtags() const`
- `TokenBatch mentions() const`
- `TokenBatch urls() const`
- `TokenBatch emails() const`
- `TokenBatch numbers() const`
- `TokenBatch acronyms() const`
- `TokenBatch definitions() const`
- `TokenBatch terms() const`
- `TokenBatch citations() const`
- `TokenBatch math_symbols() const`

## Token transforms and representations

- `TokenBatch lower_tokens() const`
- `TokenBatch without_stopwords() const`
- `TokenBatch without_numbers() const`
- `TokenBatch alpha_tokens() const`
- `TokenBatch alnum_tokens() const`
- `TokenBatch min_length(::std::int64_t length) const`
- `TokenBatch max_length(::std::int64_t length) const`
- `TokenBatch match_tokens(::std::string_view pattern) const`
- `TokenBatch shuffle_tokens() const`
- `TokenBatch unique_tokens() const`
- `TokenBatch sort_tokens() const`
- `Note to_ascii() const`
- `Note to_greek() const`
- `Note to_cyrillic() const`
- `Note soundex() const`
- `Note metaphone() const`

## Linguistic comparison and alignment

- `TokenBatch intersect_acronyms(const Note& other) const`
- `TokenBatch intersect_citations(const Note& other) const`
- `TokenBatch intersect_symbols(const Note& other) const`
- `TokenBatch intersect_tokens(const Note& other) const`
- `TokenBatch intersect_terms(const Note& other) const`
- `Note jaccard_acronyms(const Note& other) const`
- `Note jaccard_citations(const Note& other) const`
- `Note jaccard_symbols(const Note& other) const`
- `Note jaccard_tokens(const Note& other) const`
- `Note jaccard_terms(const Note& other) const`
- `Note overlap_acronyms(const Note& other) const`
- `Note overlap_citations(const Note& other) const`
- `Note overlap_symbols(const Note& other) const`
- `Note overlap_tokens(const Note& other) const`
- `Note overlap_terms(const Note& other) const`
- `TokenBatch diff_words(const Note& other) const`
- `TokenBatch align_tokens(const Note& other) const`
- `TokenBatch align_sentences(const Note& other) const`

# Note::RegexAccessor method index

The accessor returned by `Note::regex()` owns the six canonical pattern
operations. Default arguments cover the former convenience overloads.

- `Note count(::std::string_view pattern) const`
- `TokenBatch findall(::std::string_view pattern, ::std::int64_t flags = 0) const`
- `Note match(::std::string_view pattern) const`
- `Note search(::std::string_view pattern) const`
- `TokenBatch split(::std::string_view pattern, ::std::int64_t limit = -1) const`
