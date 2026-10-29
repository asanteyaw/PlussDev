// AUTO-GENERATED: Note method declarations
// Included inside class definition. No access specifiers.

  Note lower() const;
  Note upper() const;
  Note title() const;
  Note capitalize() const;
  Note casefold() const;
  Note swapcase() const;
  Note strip() const;
  Note lstrip() const;
  Note rstrip() const;
  Note collapse_whitespace() const;
  Note dedent() const;
  Note indent() const;
  Note remove_control_chars() const;
  Note keep_ascii() const;
  Note to_ascii() const;
  Note to_nfkc() const;
  Note to_nfc() const;
  Note to_nfd() const;
  Note to_nfkd() const;
  Note remove_punctuation() const;
  Note remove_digits() const;
  Note remove_letters() const;
  Note remove_symbols() const;
  Note remove_emojis() const;
  Note keep_alnum() const;
  Note keep_alpha() const;
  Note keep_digits() const;
  Note keep_printable() const;
  Note normalize_quotes() const;
  Note normalize_dashes() const;
  Note normalize_ellipsis() const;
  Note strip_accents() const;
  Note strip_zero_width() const;
  Note strip_bom() const;
  Note collapse_repeats() const;
  Note collapse_punct() const;
  Note collapse_spaces() const;
  Note collapse_newlines() const;

  Note strip(::std::string_view chars) const;
  Note lstrip(::std::string_view chars) const;
  Note rstrip(::std::string_view chars) const;
  Note replace(::std::string_view from, ::std::string_view to) const;
  Note replace_first(::std::string_view from, ::std::string_view to) const;
  Note replace_last(::std::string_view from, ::std::string_view to) const;
  Note replace_n(::std::string_view from, ::std::string_view to, ::std::int64_t n) const;
  Note remove(::std::string_view needle) const;
  Note remove_all(::std::string_view needle) const;
  Note remove_prefix(::std::string_view prefix) const;
  Note remove_suffix(::std::string_view suffix) const;
  Note ensure_prefix(::std::string_view prefix) const;
  Note ensure_suffix(::std::string_view suffix) const;
  Note surround(::std::string_view left, ::std::string_view right) const;
  Note quote() const;
  Note unquote() const;
  Note casefold(::std::string_view locale) const;

  Note contains(::std::string_view needle) const;
  Note starts_with(::std::string_view prefix) const;
  Note ends_with(::std::string_view suffix) const;

  Note is_ascii() const;
  Note is_utf8_valid() const;
  Note is_blank() const;
  Note lengths_bytes() const;
  Note lengths_chars() const;
  Note lengths_graphemes() const;
  TokenBatch split(::std::string_view delim) const;
  TokenBatch split_whitespace() const;
  TokenBatch split_lines() const;

  TokenBatch split_chars() const;

  Note join() const;
  Note join(::std::string_view sep) const;
  Note reshape(::pluss::msharp::RangeView::Int sizes) const;
  Note transpose(::std::int64_t dim0, ::std::int64_t dim1) const;
  Note permute(::pluss::msharp::RangeView::Int dims) const;
  Note select(::std::int64_t dim, ::std::int64_t index) const;
  Note narrow(::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) const;
  Note flatten() const;
  Note contiguous() const;
  Note base64_encode() const;
  Note base64_decode() const;
  Note hex_encode() const;
  Note hex_decode() const;
  Note url_encode() const;
  Note url_decode() const;
  Note html_escape() const;
  Note html_unescape() const;
  Note json_escape() const;
  Note is_alpha() const;
  Note is_alnum() const;
  Note is_digit() const;
  Note is_lower() const;
  Note is_upper() const;
  Note is_title() const;
  Note is_whitespace() const;
  Note contains_any_digit() const;
  Note contains_any_alpha() const;
  Note count_substring(::std::string_view needle) const;

  TokenBatch split(::std::string_view delim, ::std::int64_t limit) const;
  TokenBatch split_lines(bool keep_ends) const;
  TokenBatch split_whitespace(::std::int64_t limit) const;

  Note to_utf8_strict() const;
  Note to_utf8_lossy() const;
  Note from_latin1() const;
  Note to_latin1_lossy() const;
  Note from_windows1252() const;
  Note to_windows1252_lossy() const;

  TokenBatch diff_lines(const Note& other) const;
  TokenBatch diff_chars(const Note& other) const;

  Note deduplicate() const;
  Note deduplicate(::std::int64_t dim) const;
  Note sort() const;
  Note sort(::std::int64_t dim) const;
  Note unique() const;
  Note unique(::std::int64_t dim) const;
  Note squeeze(::std::int64_t dim) const;
  Note unsqueeze(::std::int64_t dim) const;
  Note repeat(::pluss::msharp::RangeView::Int repeats) const;
  Note repeat_interleave(::std::int64_t repeats, ::std::int64_t dim) const;
