# CodeN text API method index

This document indexes the **Note** (tensor-of-text) API.
The signatures below match the generated decl headers in `src/CodeN/generated/`.

---

# Note method index

This file is generated from the public surface of CodeN and is meant as a **navigation index**.
Semantics are described at a high level; see the corresponding header for exact signatures.

## TeX / LaTeX (1 accessor)

- `NoteTexAccessor tex() const noexcept`

The former flat `Note::tex_*` spellings have been removed. TeX operations
live on `note.tex()`; use the `docs/sphinx/note_tex_accessor_api_reference.md` for the
canonical method inventory and per-operation pages.

## Accessors (2)

- `NoteNlpAccessor nlp() const noexcept`
- `NoteRegexAccessor regex() const noexcept`

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

# NoteNlpAccessor method index

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

# NoteRegexAccessor method index

The accessor returned by `Note::regex()` owns the six canonical pattern
operations. Default arguments cover the former convenience overloads.

- `Note count(::std::string_view pattern) const`
- `TokenBatch findall(::std::string_view pattern, ::std::int64_t flags = 0) const`
- `Note match(::std::string_view pattern) const`
- `Note search(::std::string_view pattern) const`
- `TokenBatch split(::std::string_view pattern, ::std::int64_t limit = -1) const`
