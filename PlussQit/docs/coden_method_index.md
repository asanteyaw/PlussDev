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
- `Chord tex_simplify_spacing() const`
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
- `Chord tex_convert_frac_to_slash() const`
- `Chord tex_convert_sqrt_to_pow() const`
- `Chord tex_convert_pm_to_plusminus() const`
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

## NLP cleanup & normalization (75)

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
- `Chord nlp_normalize_unicode() const`
- `Chord nlp_normalize_punct() const`
- `Chord nlp_remove_stopwords() const`
- `Chord nlp_remove_urls() const`
- `Chord nlp_remove_emails() const`
- `Chord nlp_remove_mentions() const`
- `Chord nlp_remove_hashtags() const`
- `Chord nlp_remove_numbers() const`
- `Chord nlp_remove_punct() const`
- `Chord nlp_remove_diacritics() const`
- `Chord nlp_expand_contractions() const`
- `Chord nlp_simplify_whitespace() const`
- `Chord nlp_sentencecase() const`
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
- `Chord nlp_remove_nonprintable() const`
- `Chord nlp_remove_accents() const`
- `Chord nlp_normalize_whitespace() const`
- `Chord nlp_normalize_newlines() const`
- `Chord nlp_normalize_case() const`
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
- `TokenBatch find_all(::std::string_view needle) const`
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

This is a **navigation index**: semantics are described at a high level here, with exact
signatures left to the per-method reference pages.

## TeX / LaTeX (152)

- `Note tex_strip_comments() const`
- `Note tex_strip_preamble() const`
- `Note tex_strip_metadata() const`
- `Note tex_normalize_newlines() const`
- `Note tex_dedup_blank_lines() const`
- `Note tex_collapse_whitespace() const`
- `Note tex_fix_linebreaks() const`
- `Note tex_sanitize() const`
- `Note tex_escape() const`
- `Note tex_unescape() const`
- `Note tex_remove_math_delimiters() const`
- `Note tex_add_math_delimiters() const`
- `Note tex_math_only() const`
- `Note tex_text_only() const`
- `Note tex_remove_commands() const`
- `Note tex_remove_environments() const`
- `Note tex_remove_labels() const`
- `Note tex_remove_refs() const`
- `Note tex_remove_citations() const`
- `Note tex_remove_graphics() const`
- `Note tex_remove_tables() const`
- `Note tex_remove_figures() const`
- `Note tex_remove_packages() const`
- `Note tex_remove_bibliography() const`
- `Note tex_balance_braces() const`
- `Note tex_balance_math_delims() const`
- `Note tex_canonicalize_macros() const`
- `Note tex_expand_macros() const`
- `Note tex_inline_macros() const`
- `Note tex_simplify_fractions() const`
- `Note tex_simplify_roots() const`
- `Note tex_simplify_spacing() const`
- `Note tex_remove_formatting() const`
- `Note tex_remove_accents() const`
- `Note tex_remove_font_commands() const`
- `Note tex_strip_math() const`
- `Note tex_strip_text() const`
- `Note tex_strip_environments() const`
- `Note tex_strip_commands() const`
- `Note tex_collapse_macros() const`
- `Note tex_expand_inputs() const`
- `Note tex_resolve_includes() const`
- `Note tex_resolve_graphics() const`
- `Note tex_remove_color() const`
- `Note tex_remove_hyperref() const`
- `Note tex_remove_urls() const`
- `Note tex_convert_frac_to_slash() const`
- `Note tex_convert_sqrt_to_pow() const`
- `Note tex_convert_pm_to_plusminus() const`
- `Note tex_remove_left_right() const`
- `Note tex_remove_big_delims() const`
- `Note tex_remove_spacing_cmds() const`
- `Note tex_standardize_vectors() const`
- `Note tex_standardize_matrices() const`
- `Note tex_standardize_integrals() const`
- `Note tex_standardize_summations() const`
- `Note tex_standardize_limits() const`
- `Note tex_standardize_subscripts() const`
- `Note tex_standardize_superscripts() const`
- `Note tex_standardize_primes() const`
- `Note tex_standardize_dots() const`
- `Note tex_to_text() const`
- `Note tex_to_markdown() const`
- `Note tex_to_html() const`
- `TokenBatch tex_commands() const`
- `TokenBatch tex_command_names() const`
- `TokenBatch tex_environments() const`
- `TokenBatch tex_environment_names() const`
- `TokenBatch tex_math_blocks(::std::int64_t max_count) const`
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
- `TokenBatch tex_equations(::std::int64_t max_count) const`
- `TokenBatch tex_align_blocks() const`
- `TokenBatch tex_arrays() const`
- `TokenBatch tex_tabular_blocks() const`
- `TokenBatch tex_headings(::std::int64_t max_depth) const`
- `TokenBatch tex_extract_symbols() const`
- `TokenBatch tex_extract_greek() const`
- `TokenBatch tex_extract_operators() const`
- `TokenBatch tex_extract_functions() const`
- `TokenBatch tex_extract_numbers() const`
- `TokenBatch tex_extract_identifiers() const`
- `TokenBatch tex_extract_env_equation() const`
- `Note tex_remove_env_equation() const`
- `TokenBatch tex_extract_env_align() const`
- `Note tex_remove_env_align() const`
- `TokenBatch tex_extract_env_align_() const`
- `Note tex_remove_env_align_() const`
- `TokenBatch tex_extract_env_gather() const`
- `Note tex_remove_env_gather() const`
- `TokenBatch tex_extract_env_multline() const`
- `Note tex_remove_env_multline() const`
- `TokenBatch tex_extract_env_array() const`
- `Note tex_remove_env_array() const`
- `TokenBatch tex_extract_env_cases() const`
- `Note tex_remove_env_cases() const`
- `TokenBatch tex_extract_env_tabular() const`
- `Note tex_remove_env_tabular() const`
- `TokenBatch tex_extract_env_figure() const`
- `Note tex_remove_env_figure() const`
- `TokenBatch tex_extract_env_table() const`
- `Note tex_remove_env_table() const`
- `TokenBatch tex_extract_env_itemize() const`
- `Note tex_remove_env_itemize() const`
- `TokenBatch tex_extract_env_enumerate() const`
- `Note tex_remove_env_enumerate() const`
- `Note tex_replace_alpha(::std::string_view repl) const`
- `Note tex_replace_beta(::std::string_view repl) const`
- `Note tex_replace_gamma(::std::string_view repl) const`
- `Note tex_replace_delta(::std::string_view repl) const`
- `Note tex_replace_epsilon(::std::string_view repl) const`
- `Note tex_replace_varepsilon(::std::string_view repl) const`
- `Note tex_replace_theta(::std::string_view repl) const`
- `Note tex_replace_vartheta(::std::string_view repl) const`
- `Note tex_replace_lambda(::std::string_view repl) const`
- `Note tex_replace_mu(::std::string_view repl) const`
- `Note tex_replace_pi(::std::string_view repl) const`
- `Note tex_replace_sigma(::std::string_view repl) const`
- `Note tex_replace_phi(::std::string_view repl) const`
- `Note tex_replace_varphi(::std::string_view repl) const`
- `Note tex_replace_omega(::std::string_view repl) const`
- `Note tex_extract_environment(::std::string_view name) const`
- `Note tex_remove_environment(::std::string_view name) const`
- `TokenBatch tex_extract_command_args(::std::string_view cmd) const`
- `Note tex_replace_command(::std::string_view cmd, ::std::string_view repl) const`
- `Note tex_set_macro(::std::string_view name, ::std::string_view body) const`
- `Note tex_remove_macro(::std::string_view name) const`
- `Note tex_wrap_inline_math(::std::string_view delim) const`
- `Note tex_wrap_display_math(::std::string_view delim) const`
- `TokenBatch tex_theorems() const`
- `TokenBatch tex_proofs() const`
- `TokenBatch tex_definitions() const`
- `TokenBatch tex_lemmas() const`
- `TokenBatch tex_corollaries() const`
- `TokenBatch tex_examples() const`
- `TokenBatch tex_tables() const`
- `TokenBatch tex_figures() const`
- `TokenBatch tex_captions() const`
- `TokenBatch tex_footnotes() const`
- `TokenBatch tex_todos() const`
- `TokenBatch tex_warnings() const`

## NLP cleanup & normalization (89)

- `Note nlp_clean() const`
- `Note nlp_normalize_unicode() const`
- `Note nlp_normalize_punct() const`
- `Note nlp_remove_stopwords() const`
- `Note nlp_remove_urls() const`
- `Note nlp_remove_emails() const`
- `Note nlp_remove_mentions() const`
- `Note nlp_remove_hashtags() const`
- `Note nlp_remove_numbers() const`
- `Note nlp_remove_punct() const`
- `Note nlp_remove_diacritics() const`
- `Note nlp_expand_contractions() const`
- `Note nlp_simplify_whitespace() const`
- `Note nlp_sentencecase() const`
- `Note nlp_remove_html() const`
- `Note nlp_remove_markdown() const`
- `Note nlp_remove_code() const`
- `Note nlp_remove_math() const`
- `Note nlp_remove_latex() const`
- `Note nlp_remove_brackets() const`
- `Note nlp_remove_parentheticals() const`
- `Note nlp_remove_quotes() const`
- `Note nlp_remove_bullets() const`
- `Note nlp_remove_headers() const`
- `Note nlp_remove_footers() const`
- `Note nlp_remove_boilerplate() const`
- `Note nlp_remove_tables() const`
- `Note nlp_remove_nonprintable() const`
- `Note nlp_remove_accents() const`
- `Note nlp_normalize_whitespace() const`
- `Note nlp_normalize_newlines() const`
- `Note nlp_normalize_case() const`
- `Note nlp_normalize_numbers() const`
- `Note nlp_mask_numbers() const`
- `Note nlp_mask_emails() const`
- `Note nlp_mask_urls() const`
- `Note nlp_mask_mentions() const`
- `Note nlp_mask_hashtags() const`
- `Note nlp_mask_dates() const`
- `Note nlp_mask_times() const`
- `Note nlp_mask_currency() const`
- `Note nlp_mask_pii() const`
- `TokenBatch nlp_tokens() const`
- `TokenBatch nlp_words() const`
- `TokenBatch nlp_sentences() const`
- `TokenBatch nlp_stems() const`
- `TokenBatch nlp_lemmas() const`
- `TokenBatch nlp_pos_tags() const`
- `TokenBatch nlp_named_entities() const`
- `TokenBatch nlp_noun_phrases() const`
- `TokenBatch nlp_keywords(::std::int64_t k) const`
- `TokenBatch nlp_keyphrases(::std::int64_t k) const`
- `TokenBatch nlp_bigrams() const`
- `TokenBatch nlp_trigrams() const`
- `TokenBatch nlp_char_ngrams(::std::int64_t n) const`
- `TokenBatch nlp_word_ngrams(::std::int64_t n) const`
- `TokenBatch nlp_hashtags() const`
- `TokenBatch nlp_mentions() const`
- `TokenBatch nlp_urls() const`
- `TokenBatch nlp_emails() const`
- `TokenBatch nlp_numbers() const`
- `TokenBatch nlp_extract_acronyms() const`
- `TokenBatch nlp_extract_definitions() const`
- `TokenBatch nlp_extract_terms() const`
- `TokenBatch nlp_extract_citations() const`
- `TokenBatch nlp_extract_bibliography() const`
- `TokenBatch nlp_extract_math_symbols() const`
- `Note nlp_language() const`
- `Note nlp_script() const`
- `Note nlp_sentiment() const`
- `Note nlp_toxicity() const`
- `Note nlp_readability_grade() const`
- `Note nlp_readability_level() const`
- `Note nlp_spam_score() const`
- `Note nlp_formality() const`
- `Note nlp_subjectivity() const`
- `TokenBatch nlp_lowercase_tokens() const`
- `TokenBatch nlp_remove_stopword_tokens() const`
- `TokenBatch nlp_remove_punct_tokens() const`
- `TokenBatch nlp_remove_number_tokens() const`
- `TokenBatch nlp_filter_tokens_alpha() const`
- `TokenBatch nlp_filter_tokens_alnum() const`
- `TokenBatch nlp_filter_tokens_len_ge(::std::int64_t n) const`
- `TokenBatch nlp_filter_tokens_len_le(::std::int64_t n) const`
- `TokenBatch nlp_filter_tokens_regex(::std::string_view pattern) const`
- `Note nlp_join_tokens(::std::string_view sep) const`
- `TokenBatch nlp_shuffle_tokens() const`
- `TokenBatch nlp_unique_tokens() const`
- `TokenBatch nlp_sort_tokens() const`

## Tokenization & splitting (10)

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

## Regex (5)

- `Note regex_replace(::std::string_view pattern, ::std::string_view repl) const`
- `::std::vector<::std::uint8_t> regex_match(::std::string_view pattern) const`
- `::std::vector<::std::uint8_t> regex_search(::std::string_view pattern) const`
- `TokenBatch regex_findall(::std::string_view pattern) const`
- `TokenBatch regex_split(::std::string_view pattern) const`

## Unicode & casing (21)

- `Note lower() const`
- `Note upper() const`
- `Note title() const`
- `Note capitalize() const`
- `Note casefold() const`
- `Note swapcase() const`
- `Note normalize_spaces() const`
- `Note to_ascii() const`
- `Note to_nfkc() const`
- `Note to_nfc() const`
- `Note to_nfd() const`
- `Note to_nfkd() const`
- `Note normalize_quotes() const`
- `Note normalize_dashes() const`
- `Note normalize_ellipsis() const`
- `Note strip_accents() const`
- `Note strip_diacritics() const`
- `Note to_utf8_strict() const`
- `Note to_utf8_lossy() const`
- `Note to_latin1_lossy() const`
- `Note to_windows1252_lossy() const`

## Whitespace & layout (12)

- `Note strip() const`
- `Note lstrip() const`
- `Note rstrip() const`
- `Note trim() const`
- `Note trim_left() const`
- `Note trim_right() const`
- `Note collapse_whitespace() const`
- `Note dedent() const`
- `Note indent() const`
- `Note collapse_spaces() const`
- `Note collapse_newlines() const`
- `::std::vector<::std::uint8_t> is_whitespace() const`

## Filtering & cleaning (19)

- `Note remove_control_chars() const`
- `Note remove_non_ascii() const`
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

- `::std::vector<::std::uint8_t> is_bytes() const`
- `::std::vector<::std::int64_t> lengths_bytes() const`
- `Note base64_encode() const`
- `Note base64_decode() const`
- `Note hex_encode() const`
- `Note hex_decode() const`

## Search & match (9)

- `::std::vector<::std::uint8_t> contains(::std::string_view needle) const`
- `::std::vector<::std::uint8_t> starts_with(::std::string_view prefix) const`
- `::std::vector<::std::uint8_t> ends_with(::std::string_view suffix) const`
- `TokenBatch find_all(::std::string_view needle) const`
- `TokenBatch find_all_regex(::std::string_view pattern) const`
- `::std::vector<::std::uint8_t> contains_any_digit() const`
- `::std::vector<::std::uint8_t> contains_any_alpha() const`
- `::std::vector<::std::int64_t> count_substring(::std::string_view needle) const`
- `::std::vector<::std::int64_t> count_regex(::std::string_view pattern) const`

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
- `Note unique() const`
- `Note squeeze(::std::int64_t dim) const`
- `Note unsqueeze(::std::int64_t dim) const`
- `Note repeat(::std::span<const ::std::int64_t> repeats) const`
- `Note tile(::std::span<const ::std::int64_t> reps) const`

## Other (47)

- `Note remove(::std::string_view needle) const`
- `Note ensure_prefix(::std::string_view prefix) const`
- `Note ensure_suffix(::std::string_view suffix) const`
- `Note surround(::std::string_view left, ::std::string_view right) const`
- `Note quote() const`
- `Note unquote() const`
- `Note normalize(::std::int64_t form) const`
- `::std::vector<::std::uint8_t> is_ascii() const`
- `::std::vector<::std::uint8_t> is_utf8_valid() const`
- `::std::vector<::std::uint8_t> is_blank() const`
- `::std::vector<::std::int64_t> lengths_chars() const`
- `::std::vector<::std::int64_t> lengths_graphemes() const`
- `TokenBatch ngrams_char(::std::int64_t n) const`
- `TokenBatch ngrams_word(::std::int64_t n) const`
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
- `::std::vector<::std::uint8_t> is_alpha() const`
- `::std::vector<::std::uint8_t> is_alnum() const`
- `::std::vector<::std::uint8_t> is_digit() const`
- `::std::vector<::std::uint8_t> is_lower() const`
- `::std::vector<::std::uint8_t> is_upper() const`
- `::std::vector<::std::uint8_t> is_title() const`
- `Note from_latin1() const`
- `Note from_windows1252() const`
- `Note transliterate_ascii() const`
- `Note transliterate_latin() const`
- `Note transliterate_greek() const`
- `Note transliterate_cyrillic() const`
- `Note phonetic_soundex() const`
- `Note phonetic_metaphone() const`
- `Note phonetic_double_metaphone() const`
- `TokenBatch diff_words(const Note& other) const`
- `TokenBatch diff_lines(const Note& other) const`
- `TokenBatch diff_chars(const Note& other) const`
- `TokenBatch align_tokens(const Note& other) const`
- `TokenBatch align_sentences(const Note& other) const`
- `Note deduplicate() const`
- `Note expand_dims(::std::int64_t dim) const`
- `Note repeat_interleave(::std::int64_t repeats, ::std::int64_t dim) const`

