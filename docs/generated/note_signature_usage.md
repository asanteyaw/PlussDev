# Complete Note Method Reference: Signatures + Real Calls

Use this fixture setup once for the examples below:

```cpp
auto note = pluss::make_note("alpha beta gamma");
auto other_note = pluss::make_note("delta epsilon");
std::array<std::int64_t, 2> dims_arr{1, 1};
auto dims_span = std::span<const std::int64_t>(dims_arr.data(), dims_arr.size());
```

- Signature: `Note lower() const`
  Example: `auto out = note.lower();`
- Signature: `Note upper() const`
  Example: `auto out = note.upper();`
- Signature: `Note title() const`
  Example: `auto out = note.title();`
- Signature: `Note capitalize() const`
  Example: `auto out = note.capitalize();`
- Signature: `Note casefold() const`
  Example: `auto out = note.casefold();`
- Signature: `Note swapcase() const`
  Example: `auto out = note.swapcase();`
- Signature: `Note strip() const`
  Example: `auto out = note.strip();`
- Signature: `Note lstrip() const`
  Example: `auto out = note.lstrip();`
- Signature: `Note rstrip() const`
  Example: `auto out = note.rstrip();`
- Signature: `Note trim() const`
  Example: `auto out = note.trim();`
- Signature: `Note trim_left() const`
  Example: `auto out = note.trim_left();`
- Signature: `Note trim_right() const`
  Example: `auto out = note.trim_right();`
- Signature: `Note collapse_whitespace() const`
  Example: `auto out = note.collapse_whitespace();`
- Signature: `Note dedent() const`
  Example: `auto out = note.dedent();`
- Signature: `Note indent() const`
  Example: `auto out = note.indent();`
- Signature: `Note normalize_spaces() const`
  Example: `auto out = note.normalize_spaces();`
- Signature: `Note remove_control_chars() const`
  Example: `auto out = note.remove_control_chars();`
- Signature: `Note remove_non_ascii() const`
  Example: `auto out = note.remove_non_ascii();`
- Signature: `Note to_ascii() const`
  Example: `auto out = note.to_ascii();`
- Signature: `Note to_nfkc() const`
  Example: `auto out = note.to_nfkc();`
- Signature: `Note to_nfc() const`
  Example: `auto out = note.to_nfc();`
- Signature: `Note to_nfd() const`
  Example: `auto out = note.to_nfd();`
- Signature: `Note to_nfkd() const`
  Example: `auto out = note.to_nfkd();`
- Signature: `Note remove_punctuation() const`
  Example: `auto out = note.remove_punctuation();`
- Signature: `Note remove_digits() const`
  Example: `auto out = note.remove_digits();`
- Signature: `Note remove_letters() const`
  Example: `auto out = note.remove_letters();`
- Signature: `Note remove_symbols() const`
  Example: `auto out = note.remove_symbols();`
- Signature: `Note remove_emojis() const`
  Example: `auto out = note.remove_emojis();`
- Signature: `Note keep_alnum() const`
  Example: `auto out = note.keep_alnum();`
- Signature: `Note keep_alpha() const`
  Example: `auto out = note.keep_alpha();`
- Signature: `Note keep_digits() const`
  Example: `auto out = note.keep_digits();`
- Signature: `Note keep_ascii() const`
  Example: `auto out = note.keep_ascii();`
- Signature: `Note keep_printable() const`
  Example: `auto out = note.keep_printable();`
- Signature: `Note normalize_quotes() const`
  Example: `auto out = note.normalize_quotes();`
- Signature: `Note normalize_dashes() const`
  Example: `auto out = note.normalize_dashes();`
- Signature: `Note normalize_ellipsis() const`
  Example: `auto out = note.normalize_ellipsis();`
- Signature: `Note strip_accents() const`
  Example: `auto out = note.strip_accents();`
- Signature: `Note strip_diacritics() const`
  Example: `auto out = note.strip_diacritics();`
- Signature: `Note strip_zero_width() const`
  Example: `auto out = note.strip_zero_width();`
- Signature: `Note strip_bom() const`
  Example: `auto out = note.strip_bom();`
- Signature: `Note collapse_repeats() const`
  Example: `auto out = note.collapse_repeats();`
- Signature: `Note collapse_punct() const`
  Example: `auto out = note.collapse_punct();`
- Signature: `Note collapse_spaces() const`
  Example: `auto out = note.collapse_spaces();`
- Signature: `Note collapse_newlines() const`
  Example: `auto out = note.collapse_newlines();`
- Signature: `Note nlp_clean() const`
  Example: `auto out = note.nlp_clean();`
- Signature: `Note nlp_normalize_unicode() const`
  Example: `auto out = note.nlp_normalize_unicode();`
- Signature: `Note nlp_normalize_punct() const`
  Example: `auto out = note.nlp_normalize_punct();`
- Signature: `Note nlp_remove_stopwords() const`
  Example: `auto out = note.nlp_remove_stopwords();`
- Signature: `Note nlp_remove_urls() const`
  Example: `auto out = note.nlp_remove_urls();`
- Signature: `Note nlp_remove_emails() const`
  Example: `auto out = note.nlp_remove_emails();`
- Signature: `Note nlp_remove_mentions() const`
  Example: `auto out = note.nlp_remove_mentions();`
- Signature: `Note nlp_remove_hashtags() const`
  Example: `auto out = note.nlp_remove_hashtags();`
- Signature: `Note nlp_remove_numbers() const`
  Example: `auto out = note.nlp_remove_numbers();`
- Signature: `Note nlp_remove_punct() const`
  Example: `auto out = note.nlp_remove_punct();`
- Signature: `Note nlp_remove_diacritics() const`
  Example: `auto out = note.nlp_remove_diacritics();`
- Signature: `Note nlp_expand_contractions() const`
  Example: `auto out = note.nlp_expand_contractions();`
- Signature: `Note nlp_simplify_whitespace() const`
  Example: `auto out = note.nlp_simplify_whitespace();`
- Signature: `Note nlp_sentencecase() const`
  Example: `auto out = note.nlp_sentencecase();`
- Signature: `Note nlp_remove_html() const`
  Example: `auto out = note.nlp_remove_html();`
- Signature: `Note nlp_remove_markdown() const`
  Example: `auto out = note.nlp_remove_markdown();`
- Signature: `Note nlp_remove_code() const`
  Example: `auto out = note.nlp_remove_code();`
- Signature: `Note nlp_remove_math() const`
  Example: `auto out = note.nlp_remove_math();`
- Signature: `Note nlp_remove_latex() const`
  Example: `auto out = note.nlp_remove_latex();`
- Signature: `Note nlp_remove_brackets() const`
  Example: `auto out = note.nlp_remove_brackets();`
- Signature: `Note nlp_remove_parentheticals() const`
  Example: `auto out = note.nlp_remove_parentheticals();`
- Signature: `Note nlp_remove_quotes() const`
  Example: `auto out = note.nlp_remove_quotes();`
- Signature: `Note nlp_remove_bullets() const`
  Example: `auto out = note.nlp_remove_bullets();`
- Signature: `Note nlp_remove_headers() const`
  Example: `auto out = note.nlp_remove_headers();`
- Signature: `Note nlp_remove_footers() const`
  Example: `auto out = note.nlp_remove_footers();`
- Signature: `Note nlp_remove_boilerplate() const`
  Example: `auto out = note.nlp_remove_boilerplate();`
- Signature: `Note nlp_remove_tables() const`
  Example: `auto out = note.nlp_remove_tables();`
- Signature: `Note nlp_remove_nonprintable() const`
  Example: `auto out = note.nlp_remove_nonprintable();`
- Signature: `Note nlp_remove_accents() const`
  Example: `auto out = note.nlp_remove_accents();`
- Signature: `Note nlp_normalize_whitespace() const`
  Example: `auto out = note.nlp_normalize_whitespace();`
- Signature: `Note nlp_normalize_newlines() const`
  Example: `auto out = note.nlp_normalize_newlines();`
- Signature: `Note nlp_normalize_case() const`
  Example: `auto out = note.nlp_normalize_case();`
- Signature: `Note nlp_normalize_numbers() const`
  Example: `auto out = note.nlp_normalize_numbers();`
- Signature: `Note nlp_mask_numbers() const`
  Example: `auto out = note.nlp_mask_numbers();`
- Signature: `Note nlp_mask_emails() const`
  Example: `auto out = note.nlp_mask_emails();`
- Signature: `Note nlp_mask_urls() const`
  Example: `auto out = note.nlp_mask_urls();`
- Signature: `Note nlp_mask_mentions() const`
  Example: `auto out = note.nlp_mask_mentions();`
- Signature: `Note nlp_mask_hashtags() const`
  Example: `auto out = note.nlp_mask_hashtags();`
- Signature: `Note nlp_mask_dates() const`
  Example: `auto out = note.nlp_mask_dates();`
- Signature: `Note nlp_mask_times() const`
  Example: `auto out = note.nlp_mask_times();`
- Signature: `Note nlp_mask_currency() const`
  Example: `auto out = note.nlp_mask_currency();`
- Signature: `Note nlp_mask_pii() const`
  Example: `auto out = note.nlp_mask_pii();`
- Signature: `Note tex_strip_comments() const`
  Example: `auto out = note.tex_strip_comments();`
- Signature: `Note tex_strip_preamble() const`
  Example: `auto out = note.tex_strip_preamble();`
- Signature: `Note tex_strip_metadata() const`
  Example: `auto out = note.tex_strip_metadata();`
- Signature: `Note tex_normalize_newlines() const`
  Example: `auto out = note.tex_normalize_newlines();`
- Signature: `Note tex_dedup_blank_lines() const`
  Example: `auto out = note.tex_dedup_blank_lines();`
- Signature: `Note tex_collapse_whitespace() const`
  Example: `auto out = note.tex_collapse_whitespace();`
- Signature: `Note tex_fix_linebreaks() const`
  Example: `auto out = note.tex_fix_linebreaks();`
- Signature: `Note tex_sanitize() const`
  Example: `auto out = note.tex_sanitize();`
- Signature: `Note tex_escape() const`
  Example: `auto out = note.tex_escape();`
- Signature: `Note tex_unescape() const`
  Example: `auto out = note.tex_unescape();`
- Signature: `Note tex_remove_math_delimiters() const`
  Example: `auto out = note.tex_remove_math_delimiters();`
- Signature: `Note tex_add_math_delimiters() const`
  Example: `auto out = note.tex_add_math_delimiters();`
- Signature: `Note tex_math_only() const`
  Example: `auto out = note.tex_math_only();`
- Signature: `Note tex_text_only() const`
  Example: `auto out = note.tex_text_only();`
- Signature: `Note tex_remove_commands() const`
  Example: `auto out = note.tex_remove_commands();`
- Signature: `Note tex_remove_environments() const`
  Example: `auto out = note.tex_remove_environments();`
- Signature: `Note tex_remove_labels() const`
  Example: `auto out = note.tex_remove_labels();`
- Signature: `Note tex_remove_refs() const`
  Example: `auto out = note.tex_remove_refs();`
- Signature: `Note tex_remove_citations() const`
  Example: `auto out = note.tex_remove_citations();`
- Signature: `Note tex_remove_graphics() const`
  Example: `auto out = note.tex_remove_graphics();`
- Signature: `Note tex_remove_tables() const`
  Example: `auto out = note.tex_remove_tables();`
- Signature: `Note tex_remove_figures() const`
  Example: `auto out = note.tex_remove_figures();`
- Signature: `Note tex_remove_packages() const`
  Example: `auto out = note.tex_remove_packages();`
- Signature: `Note tex_remove_bibliography() const`
  Example: `auto out = note.tex_remove_bibliography();`
- Signature: `Note tex_balance_braces() const`
  Example: `auto out = note.tex_balance_braces();`
- Signature: `Note tex_balance_math_delims() const`
  Example: `auto out = note.tex_balance_math_delims();`
- Signature: `Note tex_canonicalize_macros() const`
  Example: `auto out = note.tex_canonicalize_macros();`
- Signature: `Note tex_expand_macros() const`
  Example: `auto out = note.tex_expand_macros();`
- Signature: `Note tex_inline_macros() const`
  Example: `auto out = note.tex_inline_macros();`
- Signature: `Note tex_simplify_fractions() const`
  Example: `auto out = note.tex_simplify_fractions();`
- Signature: `Note tex_simplify_roots() const`
  Example: `auto out = note.tex_simplify_roots();`
- Signature: `Note tex_simplify_spacing() const`
  Example: `auto out = note.tex_simplify_spacing();`
- Signature: `Note tex_remove_formatting() const`
  Example: `auto out = note.tex_remove_formatting();`
- Signature: `Note tex_remove_accents() const`
  Example: `auto out = note.tex_remove_accents();`
- Signature: `Note tex_remove_font_commands() const`
  Example: `auto out = note.tex_remove_font_commands();`
- Signature: `Note tex_strip_math() const`
  Example: `auto out = note.tex_strip_math();`
- Signature: `Note tex_strip_text() const`
  Example: `auto out = note.tex_strip_text();`
- Signature: `Note tex_strip_environments() const`
  Example: `auto out = note.tex_strip_environments();`
- Signature: `Note tex_strip_commands() const`
  Example: `auto out = note.tex_strip_commands();`
- Signature: `Note tex_collapse_macros() const`
  Example: `auto out = note.tex_collapse_macros();`
- Signature: `Note tex_expand_inputs() const`
  Example: `auto out = note.tex_expand_inputs();`
- Signature: `Note tex_resolve_includes() const`
  Example: `auto out = note.tex_resolve_includes();`
- Signature: `Note tex_resolve_graphics() const`
  Example: `auto out = note.tex_resolve_graphics();`
- Signature: `Note tex_remove_color() const`
  Example: `auto out = note.tex_remove_color();`
- Signature: `Note tex_remove_hyperref() const`
  Example: `auto out = note.tex_remove_hyperref();`
- Signature: `Note tex_remove_urls() const`
  Example: `auto out = note.tex_remove_urls();`
- Signature: `Note tex_convert_frac_to_slash() const`
  Example: `auto out = note.tex_convert_frac_to_slash();`
- Signature: `Note tex_convert_sqrt_to_pow() const`
  Example: `auto out = note.tex_convert_sqrt_to_pow();`
- Signature: `Note tex_convert_pm_to_plusminus() const`
  Example: `auto out = note.tex_convert_pm_to_plusminus();`
- Signature: `Note tex_remove_left_right() const`
  Example: `auto out = note.tex_remove_left_right();`
- Signature: `Note tex_remove_big_delims() const`
  Example: `auto out = note.tex_remove_big_delims();`
- Signature: `Note tex_remove_spacing_cmds() const`
  Example: `auto out = note.tex_remove_spacing_cmds();`
- Signature: `Note tex_standardize_vectors() const`
  Example: `auto out = note.tex_standardize_vectors();`
- Signature: `Note tex_standardize_matrices() const`
  Example: `auto out = note.tex_standardize_matrices();`
- Signature: `Note tex_standardize_integrals() const`
  Example: `auto out = note.tex_standardize_integrals();`
- Signature: `Note tex_standardize_summations() const`
  Example: `auto out = note.tex_standardize_summations();`
- Signature: `Note tex_standardize_limits() const`
  Example: `auto out = note.tex_standardize_limits();`
- Signature: `Note tex_standardize_subscripts() const`
  Example: `auto out = note.tex_standardize_subscripts();`
- Signature: `Note tex_standardize_superscripts() const`
  Example: `auto out = note.tex_standardize_superscripts();`
- Signature: `Note tex_standardize_primes() const`
  Example: `auto out = note.tex_standardize_primes();`
- Signature: `Note tex_standardize_dots() const`
  Example: `auto out = note.tex_standardize_dots();`
- Signature: `Note tex_to_text() const`
  Example: `auto out = note.tex_to_text();`
- Signature: `Note tex_to_markdown() const`
  Example: `auto out = note.tex_to_markdown();`
- Signature: `Note tex_to_html() const`
  Example: `auto out = note.tex_to_html();`
- Signature: `Note strip(std::string_view chars) const`
  Example: `auto out = note.strip("demo");`
- Signature: `Note lstrip(std::string_view chars) const`
  Example: `auto out = note.lstrip("demo");`
- Signature: `Note rstrip(std::string_view chars) const`
  Example: `auto out = note.rstrip("demo");`
- Signature: `Note trim(std::string_view chars) const`
  Example: `auto out = note.trim("demo");`
- Signature: `Note trim_left(std::string_view chars) const`
  Example: `auto out = note.trim_left("demo");`
- Signature: `Note trim_right(std::string_view chars) const`
  Example: `auto out = note.trim_right("demo");`
- Signature: `Note replace(std::string_view from, std::string_view to) const`
  Example: `auto out = note.replace("demo", "demo");`
- Signature: `Note replace_first(std::string_view from, std::string_view to) const`
  Example: `auto out = note.replace_first("demo", "demo");`
- Signature: `Note replace_last(std::string_view from, std::string_view to) const`
  Example: `auto out = note.replace_last("demo", "demo");`
- Signature: `Note replace_n(std::string_view from, std::string_view to, std::int64_t n) const`
  Example: `auto out = note.replace_n("demo", "demo", 1);`
- Signature: `Note remove(std::string_view needle) const`
  Example: `auto out = note.remove("demo");`
- Signature: `Note remove_all(std::string_view needle) const`
  Example: `auto out = note.remove_all("demo");`
- Signature: `Note remove_prefix(std::string_view prefix) const`
  Example: `auto out = note.remove_prefix("demo");`
- Signature: `Note remove_suffix(std::string_view suffix) const`
  Example: `auto out = note.remove_suffix("demo");`
- Signature: `Note ensure_prefix(std::string_view prefix) const`
  Example: `auto out = note.ensure_prefix("demo");`
- Signature: `Note ensure_suffix(std::string_view suffix) const`
  Example: `auto out = note.ensure_suffix("demo");`
- Signature: `Note surround(std::string_view left, std::string_view right) const`
  Example: `auto out = note.surround("demo", "demo");`
- Signature: `Note quote() const`
  Example: `auto out = note.quote();`
- Signature: `Note unquote() const`
  Example: `auto out = note.unquote();`
- Signature: `Note normalize(std::int64_t form) const`
  Example: `auto out = note.normalize(1);`
- Signature: `Note casefold(std::string_view locale) const`
  Example: `auto out = note.casefold("demo");`
- Signature: `Note regex_replace(std::string_view pattern, std::string_view repl) const`
  Example: `auto out = note.regex_replace("demo", "demo");`
- Signature: `Note regex_replace(std::string_view pattern, std::string_view repl, std::int64_t flags) const`
  Example: `auto out = note.regex_replace("demo", "demo", 1);`
- Signature: `Note nlp_clean(std::int64_t flags) const`
  Example: `auto out = note.nlp_clean(1);`
- Signature: `Note nlp_remove_stopwords(std::string_view lang) const`
  Example: `auto out = note.nlp_remove_stopwords("demo");`
- Signature: `Note nlp_normalize_unicode(std::int64_t form) const`
  Example: `auto out = note.nlp_normalize_unicode(1);`
- Signature: `Note nlp_normalize_punct(bool ascii_only) const`
  Example: `auto out = note.nlp_normalize_punct(true);`
- Signature: `Note tex_expand_macros(std::int64_t max_passes) const`
  Example: `auto out = note.tex_expand_macros(1);`
- Signature: `Note tex_sanitize(std::int64_t level) const`
  Example: `auto out = note.tex_sanitize(1);`
- Signature: `Note tex_to_text(std::int64_t mode) const`
  Example: `auto out = note.tex_to_text(1);`
- Signature: `Note tex_to_markdown(std::int64_t mode) const`
  Example: `auto out = note.tex_to_markdown(1);`
- Signature: `Note tex_to_html(std::int64_t mode) const`
  Example: `auto out = note.tex_to_html(1);`
- Signature: `std::vector<std::uint8_t> contains(std::string_view needle) const`
  Example: `auto mask = note.contains("demo");`
- Signature: `std::vector<std::uint8_t> starts_with(std::string_view prefix) const`
  Example: `auto mask = note.starts_with("demo");`
- Signature: `std::vector<std::uint8_t> ends_with(std::string_view suffix) const`
  Example: `auto mask = note.ends_with("demo");`
- Signature: `std::vector<std::uint8_t> regex_match(std::string_view pattern) const`
  Example: `auto mask = note.regex_match("demo");`
- Signature: `std::vector<std::uint8_t> regex_search(std::string_view pattern) const`
  Example: `auto mask = note.regex_search("demo");`
- Signature: `std::vector<std::uint8_t> is_ascii() const`
  Example: `auto mask = note.is_ascii();`
- Signature: `std::vector<std::uint8_t> is_utf8_valid() const`
  Example: `auto mask = note.is_utf8_valid();`
- Signature: `std::vector<std::uint8_t> is_bytes() const`
  Example: `auto mask = note.is_bytes();`
- Signature: `std::vector<std::uint8_t> is_blank() const`
  Example: `auto mask = note.is_blank();`
- Signature: `std::vector<std::int64_t> lengths_bytes() const`
  Example: `auto counts = note.lengths_bytes();`
- Signature: `std::vector<std::int64_t> lengths_chars() const`
  Example: `auto counts = note.lengths_chars();`
- Signature: `std::vector<std::int64_t> lengths_graphemes() const`
  Example: `auto counts = note.lengths_graphemes();`
- Signature: `TokenBatch split(std::string_view delim) const`
  Example: `auto tokens = note.split("demo");`
- Signature: `TokenBatch split_whitespace() const`
  Example: `auto tokens = note.split_whitespace();`
- Signature: `TokenBatch split_lines() const`
  Example: `auto tokens = note.split_lines();`
- Signature: `TokenBatch split_sentences() const`
  Example: `auto tokens = note.split_sentences();`
- Signature: `TokenBatch split_on_punct() const`
  Example: `auto tokens = note.split_on_punct();`
- Signature: `TokenBatch split_graphemes() const`
  Example: `auto tokens = note.split_graphemes();`
- Signature: `TokenBatch split_chars() const`
  Example: `auto tokens = note.split_chars();`
- Signature: `TokenBatch tokenize_words() const`
  Example: `auto tokens = note.tokenize_words();`
- Signature: `TokenBatch tokenize_sentences() const`
  Example: `auto tokens = note.tokenize_sentences();`
- Signature: `TokenBatch tokenize_punct() const`
  Example: `auto tokens = note.tokenize_punct();`
- Signature: `TokenBatch tokenize_regex(std::string_view pattern) const`
  Example: `auto tokens = note.tokenize_regex("demo");`
- Signature: `TokenBatch regex_findall(std::string_view pattern) const`
  Example: `auto tokens = note.regex_findall("demo");`
- Signature: `TokenBatch regex_split(std::string_view pattern) const`
  Example: `auto tokens = note.regex_split("demo");`
- Signature: `TokenBatch ngrams_char(std::int64_t n) const`
  Example: `auto tokens = note.ngrams_char(1);`
- Signature: `TokenBatch ngrams_word(std::int64_t n) const`
  Example: `auto tokens = note.ngrams_word(1);`
- Signature: `TokenBatch nlp_tokens() const`
  Example: `auto tokens = note.nlp_tokens();`
- Signature: `TokenBatch nlp_words() const`
  Example: `auto tokens = note.nlp_words();`
- Signature: `TokenBatch nlp_sentences() const`
  Example: `auto tokens = note.nlp_sentences();`
- Signature: `TokenBatch nlp_stems() const`
  Example: `auto tokens = note.nlp_stems();`
- Signature: `TokenBatch nlp_lemmas() const`
  Example: `auto tokens = note.nlp_lemmas();`
- Signature: `TokenBatch nlp_pos_tags() const`
  Example: `auto tokens = note.nlp_pos_tags();`
- Signature: `TokenBatch nlp_named_entities() const`
  Example: `auto tokens = note.nlp_named_entities();`
- Signature: `TokenBatch nlp_noun_phrases() const`
  Example: `auto tokens = note.nlp_noun_phrases();`
- Signature: `TokenBatch nlp_keywords(std::int64_t k) const`
  Example: `auto tokens = note.nlp_keywords(10);`
- Signature: `TokenBatch nlp_keyphrases(std::int64_t k) const`
  Example: `auto tokens = note.nlp_keyphrases(10);`
- Signature: `TokenBatch nlp_bigrams() const`
  Example: `auto tokens = note.nlp_bigrams();`
- Signature: `TokenBatch nlp_trigrams() const`
  Example: `auto tokens = note.nlp_trigrams();`
- Signature: `TokenBatch nlp_char_ngrams(std::int64_t n) const`
  Example: `auto tokens = note.nlp_char_ngrams(1);`
- Signature: `TokenBatch nlp_word_ngrams(std::int64_t n) const`
  Example: `auto tokens = note.nlp_word_ngrams(1);`
- Signature: `TokenBatch nlp_hashtags() const`
  Example: `auto tokens = note.nlp_hashtags();`
- Signature: `TokenBatch nlp_mentions() const`
  Example: `auto tokens = note.nlp_mentions();`
- Signature: `TokenBatch nlp_urls() const`
  Example: `auto tokens = note.nlp_urls();`
- Signature: `TokenBatch nlp_emails() const`
  Example: `auto tokens = note.nlp_emails();`
- Signature: `TokenBatch nlp_numbers() const`
  Example: `auto tokens = note.nlp_numbers();`
- Signature: `TokenBatch nlp_extract_acronyms() const`
  Example: `auto tokens = note.nlp_extract_acronyms();`
- Signature: `TokenBatch nlp_extract_definitions() const`
  Example: `auto tokens = note.nlp_extract_definitions();`
- Signature: `TokenBatch nlp_extract_terms() const`
  Example: `auto tokens = note.nlp_extract_terms();`
- Signature: `TokenBatch nlp_extract_citations() const`
  Example: `auto tokens = note.nlp_extract_citations();`
- Signature: `TokenBatch nlp_extract_bibliography() const`
  Example: `auto tokens = note.nlp_extract_bibliography();`
- Signature: `TokenBatch nlp_extract_math_symbols() const`
  Example: `auto tokens = note.nlp_extract_math_symbols();`
- Signature: `TokenBatch tex_commands() const`
  Example: `auto tokens = note.tex_commands();`
- Signature: `TokenBatch tex_command_names() const`
  Example: `auto tokens = note.tex_command_names();`
- Signature: `TokenBatch tex_environments() const`
  Example: `auto tokens = note.tex_environments();`
- Signature: `TokenBatch tex_environment_names() const`
  Example: `auto tokens = note.tex_environment_names();`
- Signature: `TokenBatch tex_math_blocks(std::int64_t max_count) const`
  Example: `auto tokens = note.tex_math_blocks(1);`
- Signature: `TokenBatch tex_inline_math() const`
  Example: `auto tokens = note.tex_inline_math();`
- Signature: `TokenBatch tex_display_math() const`
  Example: `auto tokens = note.tex_display_math();`
- Signature: `TokenBatch tex_labels() const`
  Example: `auto tokens = note.tex_labels();`
- Signature: `TokenBatch tex_refs() const`
  Example: `auto tokens = note.tex_refs();`
- Signature: `TokenBatch tex_citations() const`
  Example: `auto tokens = note.tex_citations();`
- Signature: `TokenBatch tex_packages() const`
  Example: `auto tokens = note.tex_packages();`
- Signature: `TokenBatch tex_includes() const`
  Example: `auto tokens = note.tex_includes();`
- Signature: `TokenBatch tex_macros_defined() const`
  Example: `auto tokens = note.tex_macros_defined();`
- Signature: `TokenBatch tex_macros_used() const`
  Example: `auto tokens = note.tex_macros_used();`
- Signature: `TokenBatch tex_bibliography_keys() const`
  Example: `auto tokens = note.tex_bibliography_keys();`
- Signature: `TokenBatch tex_sections() const`
  Example: `auto tokens = note.tex_sections();`
- Signature: `TokenBatch tex_subsections() const`
  Example: `auto tokens = note.tex_subsections();`
- Signature: `TokenBatch tex_subsubsections() const`
  Example: `auto tokens = note.tex_subsubsections();`
- Signature: `TokenBatch tex_equations(std::int64_t max_count) const`
  Example: `auto tokens = note.tex_equations(1);`
- Signature: `TokenBatch tex_align_blocks() const`
  Example: `auto tokens = note.tex_align_blocks();`
- Signature: `TokenBatch tex_arrays() const`
  Example: `auto tokens = note.tex_arrays();`
- Signature: `TokenBatch tex_tabular_blocks() const`
  Example: `auto tokens = note.tex_tabular_blocks();`
- Signature: `TokenBatch tex_headings(std::int64_t max_depth) const`
  Example: `auto tokens = note.tex_headings(1);`
- Signature: `TokenBatch tex_extract_symbols() const`
  Example: `auto tokens = note.tex_extract_symbols();`
- Signature: `TokenBatch tex_extract_greek() const`
  Example: `auto tokens = note.tex_extract_greek();`
- Signature: `TokenBatch tex_extract_operators() const`
  Example: `auto tokens = note.tex_extract_operators();`
- Signature: `TokenBatch tex_extract_functions() const`
  Example: `auto tokens = note.tex_extract_functions();`
- Signature: `TokenBatch tex_extract_numbers() const`
  Example: `auto tokens = note.tex_extract_numbers();`
- Signature: `TokenBatch tex_extract_identifiers() const`
  Example: `auto tokens = note.tex_extract_identifiers();`
- Signature: `Note join() const`
  Example: `auto out = note.join();`
- Signature: `Note join(std::string_view sep) const`
  Example: `auto out = note.join("demo");`
- Signature: `Note reshape(std::span<const std::int64_t> sizes) const`
  Example: `auto out = note.reshape(dims_span);`
- Signature: `Note transpose(std::int64_t dim0, std::int64_t dim1) const`
  Example: `auto out = note.transpose(1, 1);`
- Signature: `Note permute(std::span<const std::int64_t> dims) const`
  Example: `auto out = note.permute(dims_span);`
- Signature: `Note select(std::int64_t dim, std::int64_t index) const`
  Example: `auto out = note.select(1, 1);`
- Signature: `Note narrow(std::int64_t dim, std::int64_t start, std::int64_t length) const`
  Example: `auto out = note.narrow(1, 1, 1);`
- Signature: `Note flatten() const`
  Example: `auto out = note.flatten();`
- Signature: `Note contiguous() const`
  Example: `auto out = note.contiguous();`
- Signature: `TokenBatch find_all(std::string_view needle) const`
  Example: `auto tokens = note.find_all("demo");`
- Signature: `TokenBatch find_all(std::string_view needle, std::int64_t start) const`
  Example: `auto tokens = note.find_all("demo", 1);`
- Signature: `TokenBatch find_all_regex(std::string_view pattern) const`
  Example: `auto tokens = note.find_all_regex("demo");`
- Signature: `TokenBatch find_all_regex(std::string_view pattern, std::int64_t flags) const`
  Example: `auto tokens = note.find_all_regex("demo", 1);`
- Signature: `TokenBatch tex_extract_env_equation() const`
  Example: `auto tokens = note.tex_extract_env_equation();`
- Signature: `Note tex_remove_env_equation() const`
  Example: `auto out = note.tex_remove_env_equation();`
- Signature: `TokenBatch tex_extract_env_align() const`
  Example: `auto tokens = note.tex_extract_env_align();`
- Signature: `Note tex_remove_env_align() const`
  Example: `auto out = note.tex_remove_env_align();`
- Signature: `TokenBatch tex_extract_env_align_() const`
  Example: `auto tokens = note.tex_extract_env_align_();`
- Signature: `Note tex_remove_env_align_() const`
  Example: `auto out = note.tex_remove_env_align_();`
- Signature: `TokenBatch tex_extract_env_gather() const`
  Example: `auto tokens = note.tex_extract_env_gather();`
- Signature: `Note tex_remove_env_gather() const`
  Example: `auto out = note.tex_remove_env_gather();`
- Signature: `TokenBatch tex_extract_env_multline() const`
  Example: `auto tokens = note.tex_extract_env_multline();`
- Signature: `Note tex_remove_env_multline() const`
  Example: `auto out = note.tex_remove_env_multline();`
- Signature: `TokenBatch tex_extract_env_array() const`
  Example: `auto tokens = note.tex_extract_env_array();`
- Signature: `Note tex_remove_env_array() const`
  Example: `auto out = note.tex_remove_env_array();`
- Signature: `TokenBatch tex_extract_env_cases() const`
  Example: `auto tokens = note.tex_extract_env_cases();`
- Signature: `Note tex_remove_env_cases() const`
  Example: `auto out = note.tex_remove_env_cases();`
- Signature: `TokenBatch tex_extract_env_tabular() const`
  Example: `auto tokens = note.tex_extract_env_tabular();`
- Signature: `Note tex_remove_env_tabular() const`
  Example: `auto out = note.tex_remove_env_tabular();`
- Signature: `TokenBatch tex_extract_env_figure() const`
  Example: `auto tokens = note.tex_extract_env_figure();`
- Signature: `Note tex_remove_env_figure() const`
  Example: `auto out = note.tex_remove_env_figure();`
- Signature: `TokenBatch tex_extract_env_table() const`
  Example: `auto tokens = note.tex_extract_env_table();`
- Signature: `Note tex_remove_env_table() const`
  Example: `auto out = note.tex_remove_env_table();`
- Signature: `TokenBatch tex_extract_env_itemize() const`
  Example: `auto tokens = note.tex_extract_env_itemize();`
- Signature: `Note tex_remove_env_itemize() const`
  Example: `auto out = note.tex_remove_env_itemize();`
- Signature: `TokenBatch tex_extract_env_enumerate() const`
  Example: `auto tokens = note.tex_extract_env_enumerate();`
- Signature: `Note tex_remove_env_enumerate() const`
  Example: `auto out = note.tex_remove_env_enumerate();`
- Signature: `Note tex_replace_alpha(std::string_view repl) const`
  Example: `auto out = note.tex_replace_alpha("demo");`
- Signature: `Note tex_replace_beta(std::string_view repl) const`
  Example: `auto out = note.tex_replace_beta("demo");`
- Signature: `Note tex_replace_gamma(std::string_view repl) const`
  Example: `auto out = note.tex_replace_gamma("demo");`
- Signature: `Note tex_replace_delta(std::string_view repl) const`
  Example: `auto out = note.tex_replace_delta("demo");`
- Signature: `Note tex_replace_epsilon(std::string_view repl) const`
  Example: `auto out = note.tex_replace_epsilon("demo");`
- Signature: `Note tex_replace_varepsilon(std::string_view repl) const`
  Example: `auto out = note.tex_replace_varepsilon("demo");`
- Signature: `Note tex_replace_theta(std::string_view repl) const`
  Example: `auto out = note.tex_replace_theta("demo");`
- Signature: `Note tex_replace_vartheta(std::string_view repl) const`
  Example: `auto out = note.tex_replace_vartheta("demo");`
- Signature: `Note tex_replace_lambda(std::string_view repl) const`
  Example: `auto out = note.tex_replace_lambda("demo");`
- Signature: `Note tex_replace_mu(std::string_view repl) const`
  Example: `auto out = note.tex_replace_mu("demo");`
- Signature: `Note tex_replace_pi(std::string_view repl) const`
  Example: `auto out = note.tex_replace_pi("demo");`
- Signature: `Note tex_replace_sigma(std::string_view repl) const`
  Example: `auto out = note.tex_replace_sigma("demo");`
- Signature: `Note tex_replace_phi(std::string_view repl) const`
  Example: `auto out = note.tex_replace_phi("demo");`
- Signature: `Note tex_replace_varphi(std::string_view repl) const`
  Example: `auto out = note.tex_replace_varphi("demo");`
- Signature: `Note tex_replace_omega(std::string_view repl) const`
  Example: `auto out = note.tex_replace_omega("demo");`
- Signature: `Note base64_encode() const`
  Example: `auto out = note.base64_encode();`
- Signature: `Note base64_decode() const`
  Example: `auto out = note.base64_decode();`
- Signature: `Note hex_encode() const`
  Example: `auto out = note.hex_encode();`
- Signature: `Note hex_decode() const`
  Example: `auto out = note.hex_decode();`
- Signature: `Note url_encode() const`
  Example: `auto out = note.url_encode();`
- Signature: `Note url_decode() const`
  Example: `auto out = note.url_decode();`
- Signature: `Note html_escape() const`
  Example: `auto out = note.html_escape();`
- Signature: `Note html_unescape() const`
  Example: `auto out = note.html_unescape();`
- Signature: `Note json_escape() const`
  Example: `auto out = note.json_escape();`
- Signature: `std::vector<std::uint8_t> is_alpha() const`
  Example: `auto mask = note.is_alpha();`
- Signature: `std::vector<std::uint8_t> is_alnum() const`
  Example: `auto mask = note.is_alnum();`
- Signature: `std::vector<std::uint8_t> is_digit() const`
  Example: `auto mask = note.is_digit();`
- Signature: `std::vector<std::uint8_t> is_lower() const`
  Example: `auto mask = note.is_lower();`
- Signature: `std::vector<std::uint8_t> is_upper() const`
  Example: `auto mask = note.is_upper();`
- Signature: `std::vector<std::uint8_t> is_title() const`
  Example: `auto mask = note.is_title();`
- Signature: `std::vector<std::uint8_t> is_whitespace() const`
  Example: `auto mask = note.is_whitespace();`
- Signature: `std::vector<std::uint8_t> contains_any_digit() const`
  Example: `auto mask = note.contains_any_digit();`
- Signature: `std::vector<std::uint8_t> contains_any_alpha() const`
  Example: `auto mask = note.contains_any_alpha();`
- Signature: `std::vector<std::int64_t> count_substring(std::string_view needle) const`
  Example: `auto counts = note.count_substring("demo");`
- Signature: `std::vector<std::int64_t> count_regex(std::string_view pattern) const`
  Example: `auto counts = note.count_regex("demo");`
- Signature: `TokenBatch split(std::string_view delim, std::int64_t limit) const`
  Example: `auto tokens = note.split("demo", 1);`
- Signature: `TokenBatch split_lines(bool keep_ends) const`
  Example: `auto tokens = note.split_lines(true);`
- Signature: `TokenBatch split_whitespace(std::int64_t limit) const`
  Example: `auto tokens = note.split_whitespace(1);`
- Signature: `TokenBatch regex_findall(std::string_view pattern, std::int64_t flags) const`
  Example: `auto tokens = note.regex_findall("demo", 1);`
- Signature: `TokenBatch regex_split(std::string_view pattern, std::int64_t limit) const`
  Example: `auto tokens = note.regex_split("demo", 1);`
- Signature: `TokenBatch tokenize_words(std::int64_t flags) const`
  Example: `auto tokens = note.tokenize_words(1);`
- Signature: `TokenBatch tokenize_sentences(std::int64_t flags) const`
  Example: `auto tokens = note.tokenize_sentences(1);`
- Signature: `TokenBatch nlp_tokens(std::int64_t flags) const`
  Example: `auto tokens = note.nlp_tokens(1);`
- Signature: `TokenBatch nlp_words(std::int64_t flags) const`
  Example: `auto tokens = note.nlp_words(1);`
- Signature: `TokenBatch nlp_sentences(std::int64_t flags) const`
  Example: `auto tokens = note.nlp_sentences(1);`
- Signature: `TokenBatch nlp_named_entities(std::string_view scheme) const`
  Example: `auto tokens = note.nlp_named_entities("demo");`
- Signature: `TokenBatch nlp_pos_tags(std::string_view tagset) const`
  Example: `auto tokens = note.nlp_pos_tags("demo");`
- Signature: `Note tex_extract_environment(std::string_view name) const`
  Example: `auto out = note.tex_extract_environment("demo");`
- Signature: `Note tex_remove_environment(std::string_view name) const`
  Example: `auto out = note.tex_remove_environment("demo");`
- Signature: `TokenBatch tex_extract_command_args(std::string_view cmd) const`
  Example: `auto tokens = note.tex_extract_command_args("demo");`
- Signature: `Note tex_replace_command(std::string_view cmd, std::string_view repl) const`
  Example: `auto out = note.tex_replace_command("demo", "demo");`
- Signature: `Note tex_set_macro(std::string_view name, std::string_view body) const`
  Example: `auto out = note.tex_set_macro("demo", "demo");`
- Signature: `Note tex_remove_macro(std::string_view name) const`
  Example: `auto out = note.tex_remove_macro("demo");`
- Signature: `Note tex_wrap_inline_math(std::string_view delim) const`
  Example: `auto out = note.tex_wrap_inline_math("demo");`
- Signature: `Note tex_wrap_display_math(std::string_view delim) const`
  Example: `auto out = note.tex_wrap_display_math("demo");`
- Signature: `TokenBatch tex_commands(bool with_args) const`
  Example: `auto tokens = note.tex_commands(true);`
- Signature: `TokenBatch tex_environments(bool with_body) const`
  Example: `auto tokens = note.tex_environments(true);`
- Signature: `TokenBatch tex_theorems() const`
  Example: `auto tokens = note.tex_theorems();`
- Signature: `TokenBatch tex_proofs() const`
  Example: `auto tokens = note.tex_proofs();`
- Signature: `TokenBatch tex_definitions() const`
  Example: `auto tokens = note.tex_definitions();`
- Signature: `TokenBatch tex_lemmas() const`
  Example: `auto tokens = note.tex_lemmas();`
- Signature: `TokenBatch tex_corollaries() const`
  Example: `auto tokens = note.tex_corollaries();`
- Signature: `TokenBatch tex_examples() const`
  Example: `auto tokens = note.tex_examples();`
- Signature: `TokenBatch tex_tables() const`
  Example: `auto tokens = note.tex_tables();`
- Signature: `TokenBatch tex_figures() const`
  Example: `auto tokens = note.tex_figures();`
- Signature: `TokenBatch tex_captions() const`
  Example: `auto tokens = note.tex_captions();`
- Signature: `TokenBatch tex_footnotes() const`
  Example: `auto tokens = note.tex_footnotes();`
- Signature: `TokenBatch tex_todos() const`
  Example: `auto tokens = note.tex_todos();`
- Signature: `TokenBatch tex_warnings() const`
  Example: `auto tokens = note.tex_warnings();`
- Signature: `Note nlp_language() const`
  Example: `auto out = note.nlp_language();`
- Signature: `Note nlp_script() const`
  Example: `auto out = note.nlp_script();`
- Signature: `Note nlp_sentiment() const`
  Example: `auto out = note.nlp_sentiment();`
- Signature: `Note nlp_toxicity() const`
  Example: `auto out = note.nlp_toxicity();`
- Signature: `Note nlp_readability_grade() const`
  Example: `auto out = note.nlp_readability_grade();`
- Signature: `Note nlp_readability_level() const`
  Example: `auto out = note.nlp_readability_level();`
- Signature: `Note nlp_spam_score() const`
  Example: `auto out = note.nlp_spam_score();`
- Signature: `Note nlp_formality() const`
  Example: `auto out = note.nlp_formality();`
- Signature: `Note nlp_subjectivity() const`
  Example: `auto out = note.nlp_subjectivity();`
- Signature: `Note nlp_mask_pii(std::int64_t flags) const`
  Example: `auto out = note.nlp_mask_pii(1);`
- Signature: `Note nlp_remove_boilerplate(std::int64_t flags) const`
  Example: `auto out = note.nlp_remove_boilerplate(1);`
- Signature: `Note nlp_remove_tables(std::int64_t flags) const`
  Example: `auto out = note.nlp_remove_tables(1);`
- Signature: `Note nlp_remove_headers(std::int64_t flags) const`
  Example: `auto out = note.nlp_remove_headers(1);`
- Signature: `Note nlp_remove_footers(std::int64_t flags) const`
  Example: `auto out = note.nlp_remove_footers(1);`
- Signature: `Note to_utf8_strict() const`
  Example: `auto out = note.to_utf8_strict();`
- Signature: `Note to_utf8_lossy() const`
  Example: `auto out = note.to_utf8_lossy();`
- Signature: `Note from_latin1() const`
  Example: `auto out = note.from_latin1();`
- Signature: `Note to_latin1_lossy() const`
  Example: `auto out = note.to_latin1_lossy();`
- Signature: `Note from_windows1252() const`
  Example: `auto out = note.from_windows1252();`
- Signature: `Note to_windows1252_lossy() const`
  Example: `auto out = note.to_windows1252_lossy();`
- Signature: `TokenBatch nlp_lowercase_tokens() const`
  Example: `auto tokens = note.nlp_lowercase_tokens();`
- Signature: `TokenBatch nlp_remove_stopword_tokens() const`
  Example: `auto tokens = note.nlp_remove_stopword_tokens();`
- Signature: `TokenBatch nlp_remove_punct_tokens() const`
  Example: `auto tokens = note.nlp_remove_punct_tokens();`
- Signature: `TokenBatch nlp_remove_number_tokens() const`
  Example: `auto tokens = note.nlp_remove_number_tokens();`
- Signature: `TokenBatch nlp_filter_tokens_alpha() const`
  Example: `auto tokens = note.nlp_filter_tokens_alpha();`
- Signature: `TokenBatch nlp_filter_tokens_alnum() const`
  Example: `auto tokens = note.nlp_filter_tokens_alnum();`
- Signature: `TokenBatch nlp_filter_tokens_len_ge(std::int64_t n) const`
  Example: `auto tokens = note.nlp_filter_tokens_len_ge(1);`
- Signature: `TokenBatch nlp_filter_tokens_len_le(std::int64_t n) const`
  Example: `auto tokens = note.nlp_filter_tokens_len_le(1);`
- Signature: `TokenBatch nlp_filter_tokens_regex(std::string_view pattern) const`
  Example: `auto tokens = note.nlp_filter_tokens_regex("demo");`
- Signature: `Note nlp_join_tokens(std::string_view sep) const`
  Example: `auto out = note.nlp_join_tokens("demo");`
- Signature: `TokenBatch nlp_shuffle_tokens() const`
  Example: `auto tokens = note.nlp_shuffle_tokens();`
- Signature: `TokenBatch nlp_unique_tokens() const`
  Example: `auto tokens = note.nlp_unique_tokens();`
- Signature: `TokenBatch nlp_sort_tokens() const`
  Example: `auto tokens = note.nlp_sort_tokens();`
- Signature: `Note transliterate_ascii() const`
  Example: `auto out = note.transliterate_ascii();`
- Signature: `Note transliterate_latin() const`
  Example: `auto out = note.transliterate_latin();`
- Signature: `Note transliterate_greek() const`
  Example: `auto out = note.transliterate_greek();`
- Signature: `Note transliterate_cyrillic() const`
  Example: `auto out = note.transliterate_cyrillic();`
- Signature: `Note phonetic_soundex() const`
  Example: `auto out = note.phonetic_soundex();`
- Signature: `Note phonetic_metaphone() const`
  Example: `auto out = note.phonetic_metaphone();`
- Signature: `Note phonetic_double_metaphone() const`
  Example: `auto out = note.phonetic_double_metaphone();`
- Signature: `TokenBatch intersect_acronyms(const Note& other) const`
  Example: `auto tokens = note.intersect_acronyms(other_note);`
- Signature: `TokenBatch intersect_citations(const Note& other) const`
  Example: `auto tokens = note.intersect_citations(other_note);`
- Signature: `TokenBatch intersect_math_symbols(const Note& other) const`
  Example: `auto tokens = note.intersect_math_symbols(other_note);`
- Signature: `TokenBatch intersect_tokens(const Note& other) const`
  Example: `auto tokens = note.intersect_tokens(other_note);`
- Signature: `TokenBatch intersect_terms(const Note& other) const`
  Example: `auto tokens = note.intersect_terms(other_note);`
- Signature: `Note jaccard_acronyms(const Note& other) const`
  Example: `auto out = note.jaccard_acronyms(other_note);`
- Signature: `Note jaccard_citations(const Note& other) const`
  Example: `auto out = note.jaccard_citations(other_note);`
- Signature: `Note jaccard_math_symbols(const Note& other) const`
  Example: `auto out = note.jaccard_math_symbols(other_note);`
- Signature: `Note jaccard_tokens(const Note& other) const`
  Example: `auto out = note.jaccard_tokens(other_note);`
- Signature: `Note jaccard_terms(const Note& other) const`
  Example: `auto out = note.jaccard_terms(other_note);`
- Signature: `Note overlap_acronyms(const Note& other) const`
  Example: `auto out = note.overlap_acronyms(other_note);`
- Signature: `Note overlap_citations(const Note& other) const`
  Example: `auto out = note.overlap_citations(other_note);`
- Signature: `Note overlap_math_symbols(const Note& other) const`
  Example: `auto out = note.overlap_math_symbols(other_note);`
- Signature: `Note overlap_tokens(const Note& other) const`
  Example: `auto out = note.overlap_tokens(other_note);`
- Signature: `Note overlap_terms(const Note& other) const`
  Example: `auto out = note.overlap_terms(other_note);`
- Signature: `TokenBatch diff_words(const Note& other) const`
  Example: `auto tokens = note.diff_words(other_note);`
- Signature: `TokenBatch diff_lines(const Note& other) const`
  Example: `auto tokens = note.diff_lines(other_note);`
- Signature: `TokenBatch diff_chars(const Note& other) const`
  Example: `auto tokens = note.diff_chars(other_note);`
- Signature: `TokenBatch align_tokens(const Note& other) const`
  Example: `auto tokens = note.align_tokens(other_note);`
- Signature: `TokenBatch align_sentences(const Note& other) const`
  Example: `auto tokens = note.align_sentences(other_note);`
- Signature: `Note deduplicate() const`
  Example: `auto out = note.deduplicate();`
- Signature: `Note deduplicate(std::int64_t dim) const`
  Example: `auto out = note.deduplicate(1);`
- Signature: `Note sort() const`
  Example: `auto out = note.sort();`
- Signature: `Note sort(std::int64_t dim) const`
  Example: `auto out = note.sort(1);`
- Signature: `Note unique() const`
  Example: `auto out = note.unique();`
- Signature: `Note unique(std::int64_t dim) const`
  Example: `auto out = note.unique(1);`
- Signature: `Note squeeze(std::int64_t dim) const`
  Example: `auto out = note.squeeze(1);`
- Signature: `Note unsqueeze(std::int64_t dim) const`
  Example: `auto out = note.unsqueeze(1);`
- Signature: `Note expand_dims(std::int64_t dim) const`
  Example: `auto out = note.expand_dims(1);`
- Signature: `Note repeat(std::span<const std::int64_t> repeats) const`
  Example: `auto out = note.repeat(dims_span);`
- Signature: `Note repeat_interleave(std::int64_t repeats, std::int64_t dim) const`
  Example: `auto out = note.repeat_interleave(1, 1);`
- Signature: `Note tile(std::span<const std::int64_t> reps) const`
  Example: `auto out = note.tile(dims_span);`
