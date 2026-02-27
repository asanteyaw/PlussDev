# Note Guide (Usage-First)

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/coden/coden.h>
#include <pluss/coden/note_factory.h>
```

## How to read this page

- Snippets are practical, runnable use cases.
- Comments explain why a method is useful.
- Full current call-pattern coverage is included at the end.

## Setup Once, Reuse Everywhere

```cpp
auto note = pluss::content("One. Two!  Email: bob@example.com");
auto docs = pluss::content({
  "Line A\nLine B",
  "A,b;c and more text"
});
```

## Construction: Real Ways You Will Use

```cpp
// Scalar / 1D / ND UTF-8
auto n0 = pluss::content("hello");
auto n1 = pluss::content({"alpha", "beta", "gamma"});
auto n2 = pluss::content({{"r1c1", "r1c2"}, {"r2c1", "r2c2"}});

// Flat data + shape
std::vector<std::string> flat{"a","b","c","d"};
auto n3 = pluss::make_note(flat, std::array<std::int64_t,2>{2,2});

// Derived constructors
auto by_lines = pluss::note_from_lines("l1\nl2\nl3");
auto by_words = pluss::note_from_words("alpha beta gamma");

// Fill helpers
auto fulln = pluss::full_note({2,3}, "x");
auto emptyn = pluss::empty_note_like(fulln);
```

```cpp
// Bytes mode (raw bytes, not UTF-8 semantic text)
auto b0 = pluss::bytes_content("\x01\x02");
auto b1 = pluss::bytes_content({"\x01", "\x02", "\x03"});
auto b2 = pluss::make_bytes_note({{"\x0A", "\x0B"}, {"\x0C", "\x0D"}});
```

## Core Text Cleanup and Normalization

```cpp
// Typical cleaning chain for user input.
auto clean = note
  .lower()                   // normalize case
  .collapse_whitespace()     // shrink runs of spaces/newlines
  .strip();                  // trim edges

// Character-set / punctuation cleanup.
auto ascii = note.to_ascii();
auto no_punct = note.remove_punctuation();
auto alnum = note.keep_alnum();
```

Helpful note:

- Prefer small composable chains (`lower` + `collapse_whitespace` + `strip`) instead of one huge transform so behavior is explicit.

## Replace, Regex, Prefix/Suffix Shaping

```cpp
auto r1 = note.replace("Email", "Contact");
auto r2 = note.replace_first("One", "1");
auto r3 = note.remove_all("!");

auto rgx = note.regex_replace("\\b\\w+@\\w+\\.\\w+\\b", "<EMAIL>");

auto p1 = note.ensure_prefix("[DOC] ");
auto p2 = note.ensure_suffix(".");
auto q  = note.quote();
auto uq = q.unquote();
```

## Predicates and Lengths

```cpp
// Vector<uint8_t> outputs: one element per Note cell.
auto has_email = note.contains("@");
auto starts = note.starts_with("One");
auto ends = note.ends_with("com");

// Length metrics
auto bytes = note.lengths_bytes();
auto chars = note.lengths_chars();
auto graphemes = note.lengths_graphemes();
```

## Tokenization and TokenBatch Usage

```cpp
// TokenBatch gives tokens + offsets per input cell.
auto words = docs.tokenize_words();
auto sents = docs.split_sentences();
auto punct = docs.split_on_punct();

for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
  std::cout << words.tokens().at(i) << "\n";
}
```

Helpful note:

- When working on search/retrieval features, tokenize once and reuse tokens instead of repeatedly re-splitting strings.

## NLP Family: Cleaning, Masking, Extraction

```cpp
auto nlp0 = note
  .nlp_remove_urls()
  .nlp_remove_emails()
  .nlp_remove_html()
  .nlp_simplify_whitespace();

auto masked = note.nlp_mask_pii();
auto masked_nums = note.nlp_mask_numbers();

auto ents = note.nlp_named_entities();
auto keyw = note.nlp_keywords(10);
auto acr  = note.nlp_extract_acronyms();
```

## TeX Family: Convert, Clean, Extract

```cpp
auto tex = pluss::content(
  "\\begin{document} Text \\textbf{bold} and $x+y$. \\label{l1} \\ref{l1} \\end{document}");

// Convert TeX to text for downstream NLP.
auto plain = tex.tex_to_text();

// Remove citation/ref metadata noise.
auto stripped = tex
  .tex_remove_labels()
  .tex_remove_refs()
  .tex_remove_citations();

// Structured extraction
auto cmds = tex.tex_commands();
auto envs = tex.tex_environments();
auto eqs  = tex.tex_equations(64);
```

Helpful note:

- TeX preprocessing works best as a pipeline: normalize -> strip metadata -> extract tokens.

## Encoding and Escaping Utilities

```cpp
auto codec = pluss::content("Hi!");

auto b64 = codec.base64_encode().base64_decode();
auto hex = codec.hex_encode().hex_decode();
auto url = pluss::content("a b+c").url_encode().url_decode();

auto html = codec.html_escape().html_unescape();
auto json = codec.json_escape();
```

## Shape Ops on Note Tensors

```cpp
auto grid = pluss::content({{"a","b","c"}, {"d","e","f"}});

auto r = grid.reshape({3,2});
auto t = grid.transpose(0,1);
auto p = grid.permute({1,0});
auto s = grid.select(0,1);
auto n = grid.narrow(1,0,2);
auto f = grid.flatten();
auto c = grid.contiguous();
```

## Compare, Diff, Alignment, and Similarity

```cpp
auto d1 = pluss::content("alpha beta gamma");
auto d2 = pluss::content("alpha theta gamma delta");

auto diff_words = d1.diff_words(d2);
auto align_tokens = d1.align_tokens(d2);
auto inter_tokens = d1.intersect_tokens(d2);

auto jac = d1.jaccard_tokens(d2);  // similarity score note
auto ov  = d1.overlap_tokens(d2);  // overlap score note
```

## Ordering and Deduping

```cpp
auto arr = pluss::content({"b", "a", "b"});
auto sorted = arr.sort();
auto uniq = sorted.unique();
auto dedup = arr.deduplicate();
```

## Implemented Note API: Signatures + Real Calls

Only methods with concrete runtime routes in `CodeN/compat/Dispatch.cpp` are documented here.
Signature convention: `self` is the receiver object (the `Note` you call the method on).

# Complete Note Method Reference: Signatures + Real Calls

Use this fixture setup once for the examples below:

```cpp
auto note = pluss::content("alpha beta gamma");
auto other_note = pluss::content("delta epsilon");
std::array<std::int64_t, 2> dims_arr{1, 1};
auto dims_span = std::span<const std::int64_t>(dims_arr.data(), dims_arr.size());
```

- Signature:
  `Note lower(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.lower();`
- Signature:
  `Note upper(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.upper();`
- Signature:
  `Note title(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.title();`
- Signature:
  `Note capitalize(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.capitalize();`
- Signature:
  `Note casefold(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.casefold();`
- Signature:
  `Note swapcase(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.swapcase();`
- Signature:
  `Note strip(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.strip();`
- Signature:
  `Note lstrip(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.lstrip();`
- Signature:
  `Note rstrip(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.rstrip();`
- Signature:
  `Note trim(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.trim();`
- Signature:
  `Note trim_left(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.trim_left();`
- Signature:
  `Note trim_right(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.trim_right();`
- Signature:
  `Note collapse_whitespace(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.collapse_whitespace();`
- Signature:
  `Note dedent(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.dedent();`
- Signature:
  `Note indent(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.indent();`
- Signature:
  `Note normalize_spaces(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.normalize_spaces();`
- Signature:
  `Note remove_control_chars(self) const`
- Explanation:
  Applies `remove_control_chars` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_control_chars();`
- Signature:
  `Note remove_non_ascii(self) const`
- Explanation:
  Applies `remove_non_ascii` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_non_ascii();`
- Signature:
  `Note to_ascii(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_ascii();`
- Signature:
  `Note to_nfkc(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_nfkc();`
- Signature:
  `Note to_nfc(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_nfc();`
- Signature:
  `Note to_nfd(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_nfd();`
- Signature:
  `Note to_nfkd(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_nfkd();`
- Signature:
  `Note remove_punctuation(self) const`
- Explanation:
  Applies `remove_punctuation` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_punctuation();`
- Signature:
  `Note remove_digits(self) const`
- Explanation:
  Applies `remove_digits` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_digits();`
- Signature:
  `Note remove_letters(self) const`
- Explanation:
  Applies `remove_letters` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_letters();`
- Signature:
  `Note remove_symbols(self) const`
- Explanation:
  Applies `remove_symbols` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_symbols();`
- Signature:
  `Note remove_emojis(self) const`
- Explanation:
  Applies `remove_emojis` to `self`. No additional arguments.
- Example:
  `auto out = note.remove_emojis();`
- Signature:
  `Note keep_alnum(self) const`
- Explanation:
  Applies `keep_alnum` to `self`. No additional arguments.
- Example:
  `auto out = note.keep_alnum();`
- Signature:
  `Note keep_alpha(self) const`
- Explanation:
  Applies `keep_alpha` to `self`. No additional arguments.
- Example:
  `auto out = note.keep_alpha();`
- Signature:
  `Note keep_digits(self) const`
- Explanation:
  Applies `keep_digits` to `self`. No additional arguments.
- Example:
  `auto out = note.keep_digits();`
- Signature:
  `Note keep_ascii(self) const`
- Explanation:
  Applies `keep_ascii` to `self`. No additional arguments.
- Example:
  `auto out = note.keep_ascii();`
- Signature:
  `Note keep_printable(self) const`
- Explanation:
  Applies `keep_printable` to `self`. No additional arguments.
- Example:
  `auto out = note.keep_printable();`
- Signature:
  `Note normalize_quotes(self) const`
- Explanation:
  Applies `normalize_quotes` to `self`. No additional arguments.
- Example:
  `auto out = note.normalize_quotes();`
- Signature:
  `Note normalize_dashes(self) const`
- Explanation:
  Applies `normalize_dashes` to `self`. No additional arguments.
- Example:
  `auto out = note.normalize_dashes();`
- Signature:
  `Note normalize_ellipsis(self) const`
- Explanation:
  Applies `normalize_ellipsis` to `self`. No additional arguments.
- Example:
  `auto out = note.normalize_ellipsis();`
- Signature:
  `Note strip_accents(self) const`
- Explanation:
  Applies `strip_accents` to `self`. No additional arguments.
- Example:
  `auto out = note.strip_accents();`
- Signature:
  `Note strip_diacritics(self) const`
- Explanation:
  Applies `strip_diacritics` to `self`. No additional arguments.
- Example:
  `auto out = note.strip_diacritics();`
- Signature:
  `Note strip_zero_width(self) const`
- Explanation:
  Applies `strip_zero_width` to `self`. No additional arguments.
- Example:
  `auto out = note.strip_zero_width();`
- Signature:
  `Note strip_bom(self) const`
- Explanation:
  Applies `strip_bom` to `self`. No additional arguments.
- Example:
  `auto out = note.strip_bom();`
- Signature:
  `Note collapse_repeats(self) const`
- Explanation:
  Applies `collapse_repeats` to `self`. No additional arguments.
- Example:
  `auto out = note.collapse_repeats();`
- Signature:
  `Note collapse_punct(self) const`
- Explanation:
  Applies `collapse_punct` to `self`. No additional arguments.
- Example:
  `auto out = note.collapse_punct();`
- Signature:
  `Note collapse_spaces(self) const`
- Explanation:
  Applies `collapse_spaces` to `self`. No additional arguments.
- Example:
  `auto out = note.collapse_spaces();`
- Signature:
  `Note collapse_newlines(self) const`
- Explanation:
  Applies `collapse_newlines` to `self`. No additional arguments.
- Example:
  `auto out = note.collapse_newlines();`
- Signature:
  `Note nlp_clean(self) const`
- Explanation:
  Applies `nlp_clean` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_clean();`
- Signature:
  `Note nlp_normalize_unicode(self) const`
- Explanation:
  Applies `nlp_normalize_unicode` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_unicode();`
- Signature:
  `Note nlp_normalize_punct(self) const`
- Explanation:
  Applies `nlp_normalize_punct` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_punct();`
- Signature:
  `Note nlp_remove_stopwords(self) const`
- Explanation:
  Applies `nlp_remove_stopwords` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_stopwords();`
- Signature:
  `Note nlp_remove_urls(self) const`
- Explanation:
  Applies `nlp_remove_urls` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_urls();`
- Signature:
  `Note nlp_remove_emails(self) const`
- Explanation:
  Applies `nlp_remove_emails` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_emails();`
- Signature:
  `Note nlp_remove_mentions(self) const`
- Explanation:
  Applies `nlp_remove_mentions` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_mentions();`
- Signature:
  `Note nlp_remove_hashtags(self) const`
- Explanation:
  Applies `nlp_remove_hashtags` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_hashtags();`
- Signature:
  `Note nlp_remove_numbers(self) const`
- Explanation:
  Applies `nlp_remove_numbers` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_numbers();`
- Signature:
  `Note nlp_remove_punct(self) const`
- Explanation:
  Applies `nlp_remove_punct` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_punct();`
- Signature:
  `Note nlp_remove_diacritics(self) const`
- Explanation:
  Applies `nlp_remove_diacritics` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_diacritics();`
- Signature:
  `Note nlp_expand_contractions(self) const`
- Explanation:
  Applies `nlp_expand_contractions` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_expand_contractions();`
- Signature:
  `Note nlp_simplify_whitespace(self) const`
- Explanation:
  Applies `nlp_simplify_whitespace` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_simplify_whitespace();`
- Signature:
  `Note nlp_sentencecase(self) const`
- Explanation:
  Applies `nlp_sentencecase` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_sentencecase();`
- Signature:
  `Note nlp_remove_html(self) const`
- Explanation:
  Applies `nlp_remove_html` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_html();`
- Signature:
  `Note nlp_remove_markdown(self) const`
- Explanation:
  Applies `nlp_remove_markdown` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_markdown();`
- Signature:
  `Note nlp_remove_code(self) const`
- Explanation:
  Applies `nlp_remove_code` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_code();`
- Signature:
  `Note nlp_remove_math(self) const`
- Explanation:
  Applies `nlp_remove_math` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_math();`
- Signature:
  `Note nlp_remove_latex(self) const`
- Explanation:
  Applies `nlp_remove_latex` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_latex();`
- Signature:
  `Note nlp_remove_brackets(self) const`
- Explanation:
  Applies `nlp_remove_brackets` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_brackets();`
- Signature:
  `Note nlp_remove_parentheticals(self) const`
- Explanation:
  Applies `nlp_remove_parentheticals` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_parentheticals();`
- Signature:
  `Note nlp_remove_quotes(self) const`
- Explanation:
  Applies `nlp_remove_quotes` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_quotes();`
- Signature:
  `Note nlp_remove_bullets(self) const`
- Explanation:
  Applies `nlp_remove_bullets` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_bullets();`
- Signature:
  `Note nlp_remove_headers(self) const`
- Explanation:
  Applies `nlp_remove_headers` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_headers();`
- Signature:
  `Note nlp_remove_footers(self) const`
- Explanation:
  Applies `nlp_remove_footers` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_footers();`
- Signature:
  `Note nlp_remove_boilerplate(self) const`
- Explanation:
  Applies `nlp_remove_boilerplate` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_boilerplate();`
- Signature:
  `Note nlp_remove_tables(self) const`
- Explanation:
  Applies `nlp_remove_tables` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_tables();`
- Signature:
  `Note nlp_remove_nonprintable(self) const`
- Explanation:
  Applies `nlp_remove_nonprintable` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_nonprintable();`
- Signature:
  `Note nlp_remove_accents(self) const`
- Explanation:
  Applies `nlp_remove_accents` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_remove_accents();`
- Signature:
  `Note nlp_normalize_whitespace(self) const`
- Explanation:
  Applies `nlp_normalize_whitespace` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_whitespace();`
- Signature:
  `Note nlp_normalize_newlines(self) const`
- Explanation:
  Applies `nlp_normalize_newlines` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_newlines();`
- Signature:
  `Note nlp_normalize_case(self) const`
- Explanation:
  Applies `nlp_normalize_case` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_case();`
- Signature:
  `Note nlp_normalize_numbers(self) const`
- Explanation:
  Applies `nlp_normalize_numbers` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_normalize_numbers();`
- Signature:
  `Note nlp_mask_numbers(self) const`
- Explanation:
  Applies `nlp_mask_numbers` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_numbers();`
- Signature:
  `Note nlp_mask_emails(self) const`
- Explanation:
  Applies `nlp_mask_emails` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_emails();`
- Signature:
  `Note nlp_mask_urls(self) const`
- Explanation:
  Applies `nlp_mask_urls` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_urls();`
- Signature:
  `Note nlp_mask_mentions(self) const`
- Explanation:
  Applies `nlp_mask_mentions` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_mentions();`
- Signature:
  `Note nlp_mask_hashtags(self) const`
- Explanation:
  Applies `nlp_mask_hashtags` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_hashtags();`
- Signature:
  `Note nlp_mask_dates(self) const`
- Explanation:
  Applies `nlp_mask_dates` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_dates();`
- Signature:
  `Note nlp_mask_times(self) const`
- Explanation:
  Applies `nlp_mask_times` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_times();`
- Signature:
  `Note nlp_mask_currency(self) const`
- Explanation:
  Applies `nlp_mask_currency` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_currency();`
- Signature:
  `Note nlp_mask_pii(self) const`
- Explanation:
  Applies `nlp_mask_pii` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_mask_pii();`
- Signature:
  `Note tex_strip_comments(self) const`
- Explanation:
  Applies `tex_strip_comments` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_comments();`
- Signature:
  `Note tex_strip_preamble(self) const`
- Explanation:
  Applies `tex_strip_preamble` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_preamble();`
- Signature:
  `Note tex_strip_metadata(self) const`
- Explanation:
  Applies `tex_strip_metadata` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_metadata();`
- Signature:
  `Note tex_normalize_newlines(self) const`
- Explanation:
  Applies `tex_normalize_newlines` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_normalize_newlines();`
- Signature:
  `Note tex_dedup_blank_lines(self) const`
- Explanation:
  Applies `tex_dedup_blank_lines` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_dedup_blank_lines();`
- Signature:
  `Note tex_collapse_whitespace(self) const`
- Explanation:
  Applies `tex_collapse_whitespace` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_collapse_whitespace();`
- Signature:
  `Note tex_fix_linebreaks(self) const`
- Explanation:
  Applies `tex_fix_linebreaks` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_fix_linebreaks();`
- Signature:
  `Note tex_sanitize(self) const`
- Explanation:
  Applies `tex_sanitize` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_sanitize();`
- Signature:
  `Note tex_escape(self) const`
- Explanation:
  Applies `tex_escape` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_escape();`
- Signature:
  `Note tex_unescape(self) const`
- Explanation:
  Applies `tex_unescape` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_unescape();`
- Signature:
  `Note tex_remove_math_delimiters(self) const`
- Explanation:
  Applies `tex_remove_math_delimiters` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_math_delimiters();`
- Signature:
  `Note tex_add_math_delimiters(self) const`
- Explanation:
  Applies `tex_add_math_delimiters` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_add_math_delimiters();`
- Signature:
  `Note tex_math_only(self) const`
- Explanation:
  Applies `tex_math_only` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_math_only();`
- Signature:
  `Note tex_text_only(self) const`
- Explanation:
  Applies `tex_text_only` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_text_only();`
- Signature:
  `Note tex_remove_commands(self) const`
- Explanation:
  Applies `tex_remove_commands` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_commands();`
- Signature:
  `Note tex_remove_environments(self) const`
- Explanation:
  Applies `tex_remove_environments` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_environments();`
- Signature:
  `Note tex_remove_labels(self) const`
- Explanation:
  Applies `tex_remove_labels` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_labels();`
- Signature:
  `Note tex_remove_refs(self) const`
- Explanation:
  Applies `tex_remove_refs` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_refs();`
- Signature:
  `Note tex_remove_citations(self) const`
- Explanation:
  Applies `tex_remove_citations` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_citations();`
- Signature:
  `Note tex_remove_graphics(self) const`
- Explanation:
  Applies `tex_remove_graphics` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_graphics();`
- Signature:
  `Note tex_remove_tables(self) const`
- Explanation:
  Applies `tex_remove_tables` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_tables();`
- Signature:
  `Note tex_remove_figures(self) const`
- Explanation:
  Applies `tex_remove_figures` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_figures();`
- Signature:
  `Note tex_remove_packages(self) const`
- Explanation:
  Applies `tex_remove_packages` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_packages();`
- Signature:
  `Note tex_remove_bibliography(self) const`
- Explanation:
  Applies `tex_remove_bibliography` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_bibliography();`
- Signature:
  `Note tex_balance_braces(self) const`
- Explanation:
  Applies `tex_balance_braces` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_balance_braces();`
- Signature:
  `Note tex_balance_math_delims(self) const`
- Explanation:
  Applies `tex_balance_math_delims` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_balance_math_delims();`
- Signature:
  `Note tex_canonicalize_macros(self) const`
- Explanation:
  Applies `tex_canonicalize_macros` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_canonicalize_macros();`
- Signature:
  `Note tex_expand_macros(self) const`
- Explanation:
  Applies `tex_expand_macros` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_expand_macros();`
- Signature:
  `Note tex_inline_macros(self) const`
- Explanation:
  Applies `tex_inline_macros` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_inline_macros();`
- Signature:
  `Note tex_simplify_fractions(self) const`
- Explanation:
  Applies `tex_simplify_fractions` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_simplify_fractions();`
- Signature:
  `Note tex_simplify_roots(self) const`
- Explanation:
  Applies `tex_simplify_roots` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_simplify_roots();`
- Signature:
  `Note tex_simplify_spacing(self) const`
- Explanation:
  Applies `tex_simplify_spacing` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_simplify_spacing();`
- Signature:
  `Note tex_remove_formatting(self) const`
- Explanation:
  Applies `tex_remove_formatting` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_formatting();`
- Signature:
  `Note tex_remove_accents(self) const`
- Explanation:
  Applies `tex_remove_accents` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_accents();`
- Signature:
  `Note tex_remove_font_commands(self) const`
- Explanation:
  Applies `tex_remove_font_commands` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_font_commands();`
- Signature:
  `Note tex_strip_math(self) const`
- Explanation:
  Applies `tex_strip_math` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_math();`
- Signature:
  `Note tex_strip_text(self) const`
- Explanation:
  Applies `tex_strip_text` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_text();`
- Signature:
  `Note tex_strip_environments(self) const`
- Explanation:
  Applies `tex_strip_environments` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_environments();`
- Signature:
  `Note tex_strip_commands(self) const`
- Explanation:
  Applies `tex_strip_commands` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_strip_commands();`
- Signature:
  `Note tex_collapse_macros(self) const`
- Explanation:
  Applies `tex_collapse_macros` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_collapse_macros();`
- Signature:
  `Note tex_expand_inputs(self) const`
- Explanation:
  Applies `tex_expand_inputs` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_expand_inputs();`
- Signature:
  `Note tex_resolve_includes(self) const`
- Explanation:
  Applies `tex_resolve_includes` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_resolve_includes();`
- Signature:
  `Note tex_resolve_graphics(self) const`
- Explanation:
  Applies `tex_resolve_graphics` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_resolve_graphics();`
- Signature:
  `Note tex_remove_color(self) const`
- Explanation:
  Applies `tex_remove_color` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_color();`
- Signature:
  `Note tex_remove_hyperref(self) const`
- Explanation:
  Applies `tex_remove_hyperref` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_hyperref();`
- Signature:
  `Note tex_remove_urls(self) const`
- Explanation:
  Applies `tex_remove_urls` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_urls();`
- Signature:
  `Note tex_convert_frac_to_slash(self) const`
- Explanation:
  Applies `tex_convert_frac_to_slash` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_convert_frac_to_slash();`
- Signature:
  `Note tex_convert_sqrt_to_pow(self) const`
- Explanation:
  Applies `tex_convert_sqrt_to_pow` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_convert_sqrt_to_pow();`
- Signature:
  `Note tex_convert_pm_to_plusminus(self) const`
- Explanation:
  Applies `tex_convert_pm_to_plusminus` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_convert_pm_to_plusminus();`
- Signature:
  `Note tex_remove_left_right(self) const`
- Explanation:
  Applies `tex_remove_left_right` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_left_right();`
- Signature:
  `Note tex_remove_big_delims(self) const`
- Explanation:
  Applies `tex_remove_big_delims` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_big_delims();`
- Signature:
  `Note tex_remove_spacing_cmds(self) const`
- Explanation:
  Applies `tex_remove_spacing_cmds` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_spacing_cmds();`
- Signature:
  `Note tex_standardize_vectors(self) const`
- Explanation:
  Applies `tex_standardize_vectors` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_vectors();`
- Signature:
  `Note tex_standardize_matrices(self) const`
- Explanation:
  Applies `tex_standardize_matrices` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_matrices();`
- Signature:
  `Note tex_standardize_integrals(self) const`
- Explanation:
  Applies `tex_standardize_integrals` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_integrals();`
- Signature:
  `Note tex_standardize_summations(self) const`
- Explanation:
  Applies `tex_standardize_summations` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_summations();`
- Signature:
  `Note tex_standardize_limits(self) const`
- Explanation:
  Applies `tex_standardize_limits` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_limits();`
- Signature:
  `Note tex_standardize_subscripts(self) const`
- Explanation:
  Applies `tex_standardize_subscripts` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_subscripts();`
- Signature:
  `Note tex_standardize_superscripts(self) const`
- Explanation:
  Applies `tex_standardize_superscripts` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_superscripts();`
- Signature:
  `Note tex_standardize_primes(self) const`
- Explanation:
  Applies `tex_standardize_primes` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_primes();`
- Signature:
  `Note tex_standardize_dots(self) const`
- Explanation:
  Applies `tex_standardize_dots` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_standardize_dots();`
- Signature:
  `Note tex_to_text(self) const`
- Explanation:
  Applies `tex_to_text` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_to_text();`
- Signature:
  `Note tex_to_markdown(self) const`
- Explanation:
  Applies `tex_to_markdown` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_to_markdown();`
- Signature:
  `Note tex_to_html(self) const`
- Explanation:
  Applies `tex_to_html` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_to_html();`
- Signature:
  `Note strip(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.strip("sample_text");`
- Signature:
  `Note lstrip(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.lstrip("sample_text");`
- Signature:
  `Note rstrip(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.rstrip("sample_text");`
- Signature:
  `Note trim(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.trim("sample_text");`
- Signature:
  `Note trim_left(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.trim_left("sample_text");`
- Signature:
  `Note trim_right(self, std::string_view chars) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.
- Example:
  `auto out = note.trim_right("sample_text");`
- Signature:
  `Note replace(self, std::string_view from, std::string_view to) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.
- Example:
  `auto out = note.replace("old_value", "new_value");`
- Signature:
  `Note replace_first(self, std::string_view from, std::string_view to) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.
- Example:
  `auto out = note.replace_first("old_value", "new_value");`
- Signature:
  `Note replace_last(self, std::string_view from, std::string_view to) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.
- Example:
  `auto out = note.replace_last("old_value", "new_value");`
- Signature:
  `Note replace_n(self, std::string_view from, std::string_view to, std::int64_t n) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to, std::int64_t n`.
- Example:
  `auto out = note.replace_n("old_value", "new_value", 1);`
- Signature:
  `Note remove(self, std::string_view needle) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.
- Example:
  `auto out = note.remove("sample_text");`
- Signature:
  `Note remove_all(self, std::string_view needle) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.
- Example:
  `auto out = note.remove_all("sample_text");`
- Signature:
  `Note remove_prefix(self, std::string_view prefix) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view prefix`.
- Example:
  `auto out = note.remove_prefix("sample_text");`
- Signature:
  `Note remove_suffix(self, std::string_view suffix) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view suffix`.
- Example:
  `auto out = note.remove_suffix("sample_text");`
- Signature:
  `Note ensure_prefix(self, std::string_view prefix) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view prefix`.
- Example:
  `auto out = note.ensure_prefix("sample_text");`
- Signature:
  `Note ensure_suffix(self, std::string_view suffix) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view suffix`.
- Example:
  `auto out = note.ensure_suffix("sample_text");`
- Signature:
  `Note surround(self, std::string_view left, std::string_view right) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view left, std::string_view right`.
- Example:
  `auto out = note.surround("old_value", "new_value");`
- Signature:
  `Note quote(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.quote();`
- Signature:
  `Note unquote(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.unquote();`
- Signature:
  `Note normalize(self, std::int64_t form) const`
- Explanation:
  Applies `normalize` to `self`. Arguments: `std::int64_t form`.
- Example:
  `auto out = note.normalize(1);`
- Signature:
  `Note casefold(self, std::string_view locale) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view locale`.
- Example:
  `auto out = note.casefold("sample_text");`
- Signature:
  `Note regex_replace(self, std::string_view pattern, std::string_view repl) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::string_view repl`.
- Example:
  `auto out = note.regex_replace("old_value", "new_value");`
- Signature:
  `Note regex_replace(self, std::string_view pattern, std::string_view repl, std::int64_t flags) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::string_view repl, std::int64_t flags`.
- Example:
  `auto out = note.regex_replace("old_value", "new_value", 1);`
- Signature:
  `Note nlp_clean(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_clean` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_clean(1);`
- Signature:
  `Note nlp_remove_stopwords(self, std::string_view lang) const`
- Explanation:
  Applies `nlp_remove_stopwords` to `self`. Arguments: `std::string_view lang`.
- Example:
  `auto out = note.nlp_remove_stopwords("sample_text");`
- Signature:
  `Note nlp_normalize_unicode(self, std::int64_t form) const`
- Explanation:
  Applies `nlp_normalize_unicode` to `self`. Arguments: `std::int64_t form`.
- Example:
  `auto out = note.nlp_normalize_unicode(1);`
- Signature:
  `Note nlp_normalize_punct(self, bool ascii_only) const`
- Explanation:
  Applies `nlp_normalize_punct` to `self`. Arguments: `bool ascii_only`.
- Example:
  `auto out = note.nlp_normalize_punct(true);`
- Signature:
  `Note tex_expand_macros(self, std::int64_t max_passes) const`
- Explanation:
  Applies `tex_expand_macros` to `self`. Arguments: `std::int64_t max_passes`.
- Example:
  `auto out = note.tex_expand_macros(1);`
- Signature:
  `Note tex_sanitize(self, std::int64_t level) const`
- Explanation:
  Applies `tex_sanitize` to `self`. Arguments: `std::int64_t level`.
- Example:
  `auto out = note.tex_sanitize(1);`
- Signature:
  `Note tex_to_text(self, std::int64_t mode) const`
- Explanation:
  Applies `tex_to_text` to `self`. Arguments: `std::int64_t mode`.
- Example:
  `auto out = note.tex_to_text(1);`
- Signature:
  `Note tex_to_markdown(self, std::int64_t mode) const`
- Explanation:
  Applies `tex_to_markdown` to `self`. Arguments: `std::int64_t mode`.
- Example:
  `auto out = note.tex_to_markdown(1);`
- Signature:
  `Note tex_to_html(self, std::int64_t mode) const`
- Explanation:
  Applies `tex_to_html` to `self`. Arguments: `std::int64_t mode`.
- Example:
  `auto out = note.tex_to_html(1);`
- Signature:
  `std::vector<std::uint8_t> contains(self, std::string_view needle) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view needle`.
- Example:
  `auto mask = note.contains("sample_text");`
- Signature:
  `std::vector<std::uint8_t> starts_with(self, std::string_view prefix) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view prefix`.
- Example:
  `auto mask = note.starts_with("sample_text");`
- Signature:
  `std::vector<std::uint8_t> ends_with(self, std::string_view suffix) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view suffix`.
- Example:
  `auto mask = note.ends_with("sample_text");`
- Signature:
  `std::vector<std::uint8_t> regex_match(self, std::string_view pattern) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view pattern`.
- Example:
  `auto mask = note.regex_match("sample_text");`
- Signature:
  `std::vector<std::uint8_t> regex_search(self, std::string_view pattern) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view pattern`.
- Example:
  `auto mask = note.regex_search("sample_text");`
- Signature:
  `std::vector<std::uint8_t> is_ascii(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_ascii();`
- Signature:
  `std::vector<std::uint8_t> is_utf8_valid(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_utf8_valid();`
- Signature:
  `std::vector<std::uint8_t> is_bytes(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_bytes();`
- Signature:
  `std::vector<std::uint8_t> is_blank(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_blank();`
- Signature:
  `std::vector<std::int64_t> lengths_bytes(self) const`
- Explanation:
  Applies `lengths_bytes` to `self`. No additional arguments.
- Example:
  `auto counts = note.lengths_bytes();`
- Signature:
  `std::vector<std::int64_t> lengths_chars(self) const`
- Explanation:
  Applies `lengths_chars` to `self`. No additional arguments.
- Example:
  `auto counts = note.lengths_chars();`
- Signature:
  `std::vector<std::int64_t> lengths_graphemes(self) const`
- Explanation:
  Applies `lengths_graphemes` to `self`. No additional arguments.
- Example:
  `auto counts = note.lengths_graphemes();`
- Signature:
  `TokenBatch split(self, std::string_view delim) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::string_view delim`.
- Example:
  `auto tokens = note.split("sample_text");`
- Signature:
  `TokenBatch split_whitespace(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_whitespace();`
- Signature:
  `TokenBatch split_lines(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_lines();`
- Signature:
  `TokenBatch split_sentences(self) const`
- Explanation:
  Applies `split_sentences` to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_sentences();`
- Signature:
  `TokenBatch split_on_punct(self) const`
- Explanation:
  Applies `split_on_punct` to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_on_punct();`
- Signature:
  `TokenBatch split_graphemes(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_graphemes();`
- Signature:
  `TokenBatch split_chars(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.split_chars();`
- Signature:
  `TokenBatch tokenize_words(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.tokenize_words();`
- Signature:
  `TokenBatch tokenize_sentences(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto tokens = note.tokenize_sentences();`
- Signature:
  `TokenBatch tokenize_punct(self) const`
- Explanation:
  Applies `tokenize_punct` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tokenize_punct();`
- Signature:
  `TokenBatch tokenize_regex(self, std::string_view pattern) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto tokens = note.tokenize_regex("sample_text");`
- Signature:
  `TokenBatch regex_findall(self, std::string_view pattern) const`
- Explanation:
  Applies `regex_findall` to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto tokens = note.regex_findall("sample_text");`
- Signature:
  `TokenBatch regex_split(self, std::string_view pattern) const`
- Explanation:
  Applies `regex_split` to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto tokens = note.regex_split("sample_text");`
- Signature:
  `TokenBatch ngrams_char(self, std::int64_t n) const`
- Explanation:
  Applies `ngrams_char` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.ngrams_char(1);`
- Signature:
  `TokenBatch ngrams_word(self, std::int64_t n) const`
- Explanation:
  Applies `ngrams_word` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.ngrams_word(1);`
- Signature:
  `TokenBatch nlp_tokens(self) const`
- Explanation:
  Applies `nlp_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_tokens();`
- Signature:
  `TokenBatch nlp_words(self) const`
- Explanation:
  Applies `nlp_words` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_words();`
- Signature:
  `TokenBatch nlp_sentences(self) const`
- Explanation:
  Applies `nlp_sentences` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_sentences();`
- Signature:
  `TokenBatch nlp_stems(self) const`
- Explanation:
  Applies `nlp_stems` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_stems();`
- Signature:
  `TokenBatch nlp_lemmas(self) const`
- Explanation:
  Applies `nlp_lemmas` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_lemmas();`
- Signature:
  `TokenBatch nlp_pos_tags(self) const`
- Explanation:
  Applies `nlp_pos_tags` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_pos_tags();`
- Signature:
  `TokenBatch nlp_named_entities(self) const`
- Explanation:
  Applies `nlp_named_entities` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_named_entities();`
- Signature:
  `TokenBatch nlp_noun_phrases(self) const`
- Explanation:
  Applies `nlp_noun_phrases` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_noun_phrases();`
- Signature:
  `TokenBatch nlp_keywords(self, std::int64_t k) const`
- Explanation:
  Applies `nlp_keywords` to `self`. Arguments: `std::int64_t k`.
- Example:
  `auto tokens = note.nlp_keywords(10);`
- Signature:
  `TokenBatch nlp_keyphrases(self, std::int64_t k) const`
- Explanation:
  Applies `nlp_keyphrases` to `self`. Arguments: `std::int64_t k`.
- Example:
  `auto tokens = note.nlp_keyphrases(10);`
- Signature:
  `TokenBatch nlp_bigrams(self) const`
- Explanation:
  Applies `nlp_bigrams` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_bigrams();`
- Signature:
  `TokenBatch nlp_trigrams(self) const`
- Explanation:
  Applies `nlp_trigrams` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_trigrams();`
- Signature:
  `TokenBatch nlp_char_ngrams(self, std::int64_t n) const`
- Explanation:
  Applies `nlp_char_ngrams` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.nlp_char_ngrams(1);`
- Signature:
  `TokenBatch nlp_word_ngrams(self, std::int64_t n) const`
- Explanation:
  Applies `nlp_word_ngrams` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.nlp_word_ngrams(1);`
- Signature:
  `TokenBatch nlp_hashtags(self) const`
- Explanation:
  Applies `nlp_hashtags` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_hashtags();`
- Signature:
  `TokenBatch nlp_mentions(self) const`
- Explanation:
  Applies `nlp_mentions` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_mentions();`
- Signature:
  `TokenBatch nlp_urls(self) const`
- Explanation:
  Applies `nlp_urls` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_urls();`
- Signature:
  `TokenBatch nlp_emails(self) const`
- Explanation:
  Applies `nlp_emails` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_emails();`
- Signature:
  `TokenBatch nlp_numbers(self) const`
- Explanation:
  Applies `nlp_numbers` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_numbers();`
- Signature:
  `TokenBatch nlp_extract_acronyms(self) const`
- Explanation:
  Applies `nlp_extract_acronyms` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_acronyms();`
- Signature:
  `TokenBatch nlp_extract_definitions(self) const`
- Explanation:
  Applies `nlp_extract_definitions` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_definitions();`
- Signature:
  `TokenBatch nlp_extract_terms(self) const`
- Explanation:
  Applies `nlp_extract_terms` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_terms();`
- Signature:
  `TokenBatch nlp_extract_citations(self) const`
- Explanation:
  Applies `nlp_extract_citations` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_citations();`
- Signature:
  `TokenBatch nlp_extract_bibliography(self) const`
- Explanation:
  Applies `nlp_extract_bibliography` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_bibliography();`
- Signature:
  `TokenBatch nlp_extract_math_symbols(self) const`
- Explanation:
  Applies `nlp_extract_math_symbols` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_extract_math_symbols();`
- Signature:
  `TokenBatch tex_commands(self) const`
- Explanation:
  Applies `tex_commands` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_commands();`
- Signature:
  `TokenBatch tex_command_names(self) const`
- Explanation:
  Applies `tex_command_names` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_command_names();`
- Signature:
  `TokenBatch tex_environments(self) const`
- Explanation:
  Applies `tex_environments` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_environments();`
- Signature:
  `TokenBatch tex_environment_names(self) const`
- Explanation:
  Applies `tex_environment_names` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_environment_names();`
- Signature:
  `TokenBatch tex_math_blocks(self, std::int64_t max_count) const`
- Explanation:
  Applies `tex_math_blocks` to `self`. Arguments: `std::int64_t max_count`.
- Example:
  `auto tokens = note.tex_math_blocks(1);`
- Signature:
  `TokenBatch tex_inline_math(self) const`
- Explanation:
  Applies `tex_inline_math` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_inline_math();`
- Signature:
  `TokenBatch tex_display_math(self) const`
- Explanation:
  Applies `tex_display_math` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_display_math();`
- Signature:
  `TokenBatch tex_labels(self) const`
- Explanation:
  Applies `tex_labels` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_labels();`
- Signature:
  `TokenBatch tex_refs(self) const`
- Explanation:
  Applies `tex_refs` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_refs();`
- Signature:
  `TokenBatch tex_citations(self) const`
- Explanation:
  Applies `tex_citations` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_citations();`
- Signature:
  `TokenBatch tex_packages(self) const`
- Explanation:
  Applies `tex_packages` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_packages();`
- Signature:
  `TokenBatch tex_includes(self) const`
- Explanation:
  Applies `tex_includes` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_includes();`
- Signature:
  `TokenBatch tex_macros_defined(self) const`
- Explanation:
  Applies `tex_macros_defined` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_macros_defined();`
- Signature:
  `TokenBatch tex_macros_used(self) const`
- Explanation:
  Applies `tex_macros_used` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_macros_used();`
- Signature:
  `TokenBatch tex_bibliography_keys(self) const`
- Explanation:
  Applies `tex_bibliography_keys` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_bibliography_keys();`
- Signature:
  `TokenBatch tex_sections(self) const`
- Explanation:
  Applies `tex_sections` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_sections();`
- Signature:
  `TokenBatch tex_subsections(self) const`
- Explanation:
  Applies `tex_subsections` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_subsections();`
- Signature:
  `TokenBatch tex_subsubsections(self) const`
- Explanation:
  Applies `tex_subsubsections` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_subsubsections();`
- Signature:
  `TokenBatch tex_equations(self, std::int64_t max_count) const`
- Explanation:
  Applies `tex_equations` to `self`. Arguments: `std::int64_t max_count`.
- Example:
  `auto tokens = note.tex_equations(1);`
- Signature:
  `TokenBatch tex_align_blocks(self) const`
- Explanation:
  Applies `tex_align_blocks` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_align_blocks();`
- Signature:
  `TokenBatch tex_arrays(self) const`
- Explanation:
  Applies `tex_arrays` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_arrays();`
- Signature:
  `TokenBatch tex_tabular_blocks(self) const`
- Explanation:
  Applies `tex_tabular_blocks` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_tabular_blocks();`
- Signature:
  `TokenBatch tex_headings(self, std::int64_t max_depth) const`
- Explanation:
  Applies `tex_headings` to `self`. Arguments: `std::int64_t max_depth`.
- Example:
  `auto tokens = note.tex_headings(1);`
- Signature:
  `TokenBatch tex_extract_symbols(self) const`
- Explanation:
  Applies `tex_extract_symbols` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_symbols();`
- Signature:
  `TokenBatch tex_extract_greek(self) const`
- Explanation:
  Applies `tex_extract_greek` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_greek();`
- Signature:
  `TokenBatch tex_extract_operators(self) const`
- Explanation:
  Applies `tex_extract_operators` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_operators();`
- Signature:
  `TokenBatch tex_extract_functions(self) const`
- Explanation:
  Applies `tex_extract_functions` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_functions();`
- Signature:
  `TokenBatch tex_extract_numbers(self) const`
- Explanation:
  Applies `tex_extract_numbers` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_numbers();`
- Signature:
  `TokenBatch tex_extract_identifiers(self) const`
- Explanation:
  Applies `tex_extract_identifiers` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_identifiers();`
- Signature:
  `Note join(self) const`
- Explanation:
  Applies a text transform or text query to `self`. No additional arguments.
- Example:
  `auto out = note.join();`
- Signature:
  `Note join(self, std::string_view sep) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view sep`.
- Example:
  `auto out = note.join("sample_text");`
- Signature:
  `Note reshape(self, std::span<const std::int64_t> sizes) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> sizes`.
- Example:
  `auto out = note.reshape(dims_span);`
- Signature:
  `Note transpose(self, std::int64_t dim0, std::int64_t dim1) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim0, std::int64_t dim1`.
- Example:
  `auto out = note.transpose(1, 1);`
- Signature:
  `Note permute(self, std::span<const std::int64_t> dims) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> dims`.
- Example:
  `auto out = note.permute(dims_span);`
- Signature:
  `Note select(self, std::int64_t dim, std::int64_t index) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim, std::int64_t index`.
- Example:
  `auto out = note.select(1, 1);`
- Signature:
  `Note narrow(self, std::int64_t dim, std::int64_t start, std::int64_t length) const`
- Explanation:
  Applies `narrow` to `self`. Arguments: `std::int64_t dim, std::int64_t start, std::int64_t length`.
- Example:
  `auto out = note.narrow(1, 1, 1);`
- Signature:
  `Note flatten(self) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. No additional arguments.
- Example:
  `auto out = note.flatten();`
- Signature:
  `Note contiguous(self) const`
- Explanation:
  Applies `contiguous` to `self`. No additional arguments.
- Example:
  `auto out = note.contiguous();`
- Signature:
  `TokenBatch find_all(self, std::string_view needle) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.
- Example:
  `auto tokens = note.find_all("sample_text");`
- Signature:
  `TokenBatch find_all(self, std::string_view needle, std::int64_t start) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view needle, std::int64_t start`.
- Example:
  `auto tokens = note.find_all("sample_text", 1);`
- Signature:
  `TokenBatch find_all_regex(self, std::string_view pattern) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto tokens = note.find_all_regex("sample_text");`
- Signature:
  `TokenBatch find_all_regex(self, std::string_view pattern, std::int64_t flags) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::int64_t flags`.
- Example:
  `auto tokens = note.find_all_regex("sample_text", 1);`
- Signature:
  `TokenBatch tex_extract_env_equation(self) const`
- Explanation:
  Applies `tex_extract_env_equation` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_equation();`
- Signature:
  `Note tex_remove_env_equation(self) const`
- Explanation:
  Applies `tex_remove_env_equation` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_equation();`
- Signature:
  `TokenBatch tex_extract_env_align(self) const`
- Explanation:
  Applies `tex_extract_env_align` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_align();`
- Signature:
  `Note tex_remove_env_align(self) const`
- Explanation:
  Applies `tex_remove_env_align` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_align();`
- Signature:
  `TokenBatch tex_extract_env_align_(self) const`
- Explanation:
  In-place variant of `tex_extract_env_align`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_align_();`
- Signature:
  `Note tex_remove_env_align_(self) const`
- Explanation:
  In-place variant of `tex_remove_env_align`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_align_();`
- Signature:
  `TokenBatch tex_extract_env_gather(self) const`
- Explanation:
  Applies `tex_extract_env_gather` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_gather();`
- Signature:
  `Note tex_remove_env_gather(self) const`
- Explanation:
  Applies `tex_remove_env_gather` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_gather();`
- Signature:
  `TokenBatch tex_extract_env_multline(self) const`
- Explanation:
  Applies `tex_extract_env_multline` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_multline();`
- Signature:
  `Note tex_remove_env_multline(self) const`
- Explanation:
  Applies `tex_remove_env_multline` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_multline();`
- Signature:
  `TokenBatch tex_extract_env_array(self) const`
- Explanation:
  Applies `tex_extract_env_array` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_array();`
- Signature:
  `Note tex_remove_env_array(self) const`
- Explanation:
  Applies `tex_remove_env_array` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_array();`
- Signature:
  `TokenBatch tex_extract_env_cases(self) const`
- Explanation:
  Applies `tex_extract_env_cases` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_cases();`
- Signature:
  `Note tex_remove_env_cases(self) const`
- Explanation:
  Applies `tex_remove_env_cases` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_cases();`
- Signature:
  `TokenBatch tex_extract_env_tabular(self) const`
- Explanation:
  Applies `tex_extract_env_tabular` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_tabular();`
- Signature:
  `Note tex_remove_env_tabular(self) const`
- Explanation:
  Applies `tex_remove_env_tabular` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_tabular();`
- Signature:
  `TokenBatch tex_extract_env_figure(self) const`
- Explanation:
  Applies `tex_extract_env_figure` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_figure();`
- Signature:
  `Note tex_remove_env_figure(self) const`
- Explanation:
  Applies `tex_remove_env_figure` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_figure();`
- Signature:
  `TokenBatch tex_extract_env_table(self) const`
- Explanation:
  Applies `tex_extract_env_table` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_table();`
- Signature:
  `Note tex_remove_env_table(self) const`
- Explanation:
  Applies `tex_remove_env_table` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_table();`
- Signature:
  `TokenBatch tex_extract_env_itemize(self) const`
- Explanation:
  Applies `tex_extract_env_itemize` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_itemize();`
- Signature:
  `Note tex_remove_env_itemize(self) const`
- Explanation:
  Applies `tex_remove_env_itemize` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_itemize();`
- Signature:
  `TokenBatch tex_extract_env_enumerate(self) const`
- Explanation:
  Applies `tex_extract_env_enumerate` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_extract_env_enumerate();`
- Signature:
  `Note tex_remove_env_enumerate(self) const`
- Explanation:
  Applies `tex_remove_env_enumerate` to `self`. No additional arguments.
- Example:
  `auto out = note.tex_remove_env_enumerate();`
- Signature:
  `Note tex_replace_alpha(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_alpha` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_alpha("sample_text");`
- Signature:
  `Note tex_replace_beta(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_beta` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_beta("sample_text");`
- Signature:
  `Note tex_replace_gamma(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_gamma` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_gamma("sample_text");`
- Signature:
  `Note tex_replace_delta(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_delta` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_delta("sample_text");`
- Signature:
  `Note tex_replace_epsilon(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_epsilon` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_epsilon("sample_text");`
- Signature:
  `Note tex_replace_varepsilon(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_varepsilon` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_varepsilon("sample_text");`
- Signature:
  `Note tex_replace_theta(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_theta` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_theta("sample_text");`
- Signature:
  `Note tex_replace_vartheta(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_vartheta` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_vartheta("sample_text");`
- Signature:
  `Note tex_replace_lambda(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_lambda` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_lambda("sample_text");`
- Signature:
  `Note tex_replace_mu(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_mu` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_mu("sample_text");`
- Signature:
  `Note tex_replace_pi(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_pi` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_pi("sample_text");`
- Signature:
  `Note tex_replace_sigma(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_sigma` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_sigma("sample_text");`
- Signature:
  `Note tex_replace_phi(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_phi` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_phi("sample_text");`
- Signature:
  `Note tex_replace_varphi(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_varphi` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_varphi("sample_text");`
- Signature:
  `Note tex_replace_omega(self, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_omega` to `self`. Arguments: `std::string_view repl`.
- Example:
  `auto out = note.tex_replace_omega("sample_text");`
- Signature:
  `Note base64_encode(self) const`
- Explanation:
  Applies `base64_encode` to `self`. No additional arguments.
- Example:
  `auto out = note.base64_encode();`
- Signature:
  `Note base64_decode(self) const`
- Explanation:
  Applies `base64_decode` to `self`. No additional arguments.
- Example:
  `auto out = note.base64_decode();`
- Signature:
  `Note hex_encode(self) const`
- Explanation:
  Applies `hex_encode` to `self`. No additional arguments.
- Example:
  `auto out = note.hex_encode();`
- Signature:
  `Note hex_decode(self) const`
- Explanation:
  Applies `hex_decode` to `self`. No additional arguments.
- Example:
  `auto out = note.hex_decode();`
- Signature:
  `Note url_encode(self) const`
- Explanation:
  Applies `url_encode` to `self`. No additional arguments.
- Example:
  `auto out = note.url_encode();`
- Signature:
  `Note url_decode(self) const`
- Explanation:
  Applies `url_decode` to `self`. No additional arguments.
- Example:
  `auto out = note.url_decode();`
- Signature:
  `Note html_escape(self) const`
- Explanation:
  Applies `html_escape` to `self`. No additional arguments.
- Example:
  `auto out = note.html_escape();`
- Signature:
  `Note html_unescape(self) const`
- Explanation:
  Applies `html_unescape` to `self`. No additional arguments.
- Example:
  `auto out = note.html_unescape();`
- Signature:
  `Note json_escape(self) const`
- Explanation:
  Applies `json_escape` to `self`. No additional arguments.
- Example:
  `auto out = note.json_escape();`
- Signature:
  `std::vector<std::uint8_t> is_alpha(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_alpha();`
- Signature:
  `std::vector<std::uint8_t> is_alnum(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_alnum();`
- Signature:
  `std::vector<std::uint8_t> is_digit(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_digit();`
- Signature:
  `std::vector<std::uint8_t> is_lower(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_lower();`
- Signature:
  `std::vector<std::uint8_t> is_upper(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_upper();`
- Signature:
  `std::vector<std::uint8_t> is_title(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_title();`
- Signature:
  `std::vector<std::uint8_t> is_whitespace(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.is_whitespace();`
- Signature:
  `std::vector<std::uint8_t> contains_any_digit(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.contains_any_digit();`
- Signature:
  `std::vector<std::uint8_t> contains_any_alpha(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto mask = note.contains_any_alpha();`
- Signature:
  `std::vector<std::int64_t> count_substring(self, std::string_view needle) const`
- Explanation:
  Applies `count_substring` to `self`. Arguments: `std::string_view needle`.
- Example:
  `auto counts = note.count_substring("sample_text");`
- Signature:
  `std::vector<std::int64_t> count_regex(self, std::string_view pattern) const`
- Explanation:
  Applies `count_regex` to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto counts = note.count_regex("sample_text");`
- Signature:
  `TokenBatch split(self, std::string_view delim, std::int64_t limit) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::string_view delim, std::int64_t limit`.
- Example:
  `auto tokens = note.split("sample_text", 1);`
- Signature:
  `TokenBatch split_lines(self, bool keep_ends) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `bool keep_ends`.
- Example:
  `auto tokens = note.split_lines(true);`
- Signature:
  `TokenBatch split_whitespace(self, std::int64_t limit) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::int64_t limit`.
- Example:
  `auto tokens = note.split_whitespace(1);`
- Signature:
  `TokenBatch regex_findall(self, std::string_view pattern, std::int64_t flags) const`
- Explanation:
  Applies `regex_findall` to `self`. Arguments: `std::string_view pattern, std::int64_t flags`.
- Example:
  `auto tokens = note.regex_findall("sample_text", 1);`
- Signature:
  `TokenBatch regex_split(self, std::string_view pattern, std::int64_t limit) const`
- Explanation:
  Applies `regex_split` to `self`. Arguments: `std::string_view pattern, std::int64_t limit`.
- Example:
  `auto tokens = note.regex_split("sample_text", 1);`
- Signature:
  `TokenBatch tokenize_words(self, std::int64_t flags) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto tokens = note.tokenize_words(1);`
- Signature:
  `TokenBatch tokenize_sentences(self, std::int64_t flags) const`
- Explanation:
  Applies a text transform or text query to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto tokens = note.tokenize_sentences(1);`
- Signature:
  `TokenBatch nlp_tokens(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_tokens` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto tokens = note.nlp_tokens(1);`
- Signature:
  `TokenBatch nlp_words(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_words` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto tokens = note.nlp_words(1);`
- Signature:
  `TokenBatch nlp_sentences(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_sentences` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto tokens = note.nlp_sentences(1);`
- Signature:
  `TokenBatch nlp_named_entities(self, std::string_view scheme) const`
- Explanation:
  Applies `nlp_named_entities` to `self`. Arguments: `std::string_view scheme`.
- Example:
  `auto tokens = note.nlp_named_entities("sample_text");`
- Signature:
  `TokenBatch nlp_pos_tags(self, std::string_view tagset) const`
- Explanation:
  Applies `nlp_pos_tags` to `self`. Arguments: `std::string_view tagset`.
- Example:
  `auto tokens = note.nlp_pos_tags("sample_text");`
- Signature:
  `Note tex_extract_environment(self, std::string_view name) const`
- Explanation:
  Applies `tex_extract_environment` to `self`. Arguments: `std::string_view name`.
- Example:
  `auto out = note.tex_extract_environment("sample_text");`
- Signature:
  `Note tex_remove_environment(self, std::string_view name) const`
- Explanation:
  Applies `tex_remove_environment` to `self`. Arguments: `std::string_view name`.
- Example:
  `auto out = note.tex_remove_environment("sample_text");`
- Signature:
  `TokenBatch tex_extract_command_args(self, std::string_view cmd) const`
- Explanation:
  Applies `tex_extract_command_args` to `self`. Arguments: `std::string_view cmd`.
- Example:
  `auto tokens = note.tex_extract_command_args("sample_text");`
- Signature:
  `Note tex_replace_command(self, std::string_view cmd, std::string_view repl) const`
- Explanation:
  Applies `tex_replace_command` to `self`. Arguments: `std::string_view cmd, std::string_view repl`.
- Example:
  `auto out = note.tex_replace_command("old_value", "new_value");`
- Signature:
  `Note tex_set_macro(self, std::string_view name, std::string_view body) const`
- Explanation:
  Applies `tex_set_macro` to `self`. Arguments: `std::string_view name, std::string_view body`.
- Example:
  `auto out = note.tex_set_macro("old_value", "new_value");`
- Signature:
  `Note tex_remove_macro(self, std::string_view name) const`
- Explanation:
  Applies `tex_remove_macro` to `self`. Arguments: `std::string_view name`.
- Example:
  `auto out = note.tex_remove_macro("sample_text");`
- Signature:
  `Note tex_wrap_inline_math(self, std::string_view delim) const`
- Explanation:
  Applies `tex_wrap_inline_math` to `self`. Arguments: `std::string_view delim`.
- Example:
  `auto out = note.tex_wrap_inline_math("sample_text");`
- Signature:
  `Note tex_wrap_display_math(self, std::string_view delim) const`
- Explanation:
  Applies `tex_wrap_display_math` to `self`. Arguments: `std::string_view delim`.
- Example:
  `auto out = note.tex_wrap_display_math("sample_text");`
- Signature:
  `TokenBatch tex_commands(self, bool with_args) const`
- Explanation:
  Applies `tex_commands` to `self`. Arguments: `bool with_args`.
- Example:
  `auto tokens = note.tex_commands(true);`
- Signature:
  `TokenBatch tex_environments(self, bool with_body) const`
- Explanation:
  Applies `tex_environments` to `self`. Arguments: `bool with_body`.
- Example:
  `auto tokens = note.tex_environments(true);`
- Signature:
  `TokenBatch tex_theorems(self) const`
- Explanation:
  Applies `tex_theorems` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_theorems();`
- Signature:
  `TokenBatch tex_proofs(self) const`
- Explanation:
  Applies `tex_proofs` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_proofs();`
- Signature:
  `TokenBatch tex_definitions(self) const`
- Explanation:
  Applies `tex_definitions` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_definitions();`
- Signature:
  `TokenBatch tex_lemmas(self) const`
- Explanation:
  Applies `tex_lemmas` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_lemmas();`
- Signature:
  `TokenBatch tex_corollaries(self) const`
- Explanation:
  Applies `tex_corollaries` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_corollaries();`
- Signature:
  `TokenBatch tex_examples(self) const`
- Explanation:
  Applies `tex_examples` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_examples();`
- Signature:
  `TokenBatch tex_tables(self) const`
- Explanation:
  Applies `tex_tables` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_tables();`
- Signature:
  `TokenBatch tex_figures(self) const`
- Explanation:
  Applies `tex_figures` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_figures();`
- Signature:
  `TokenBatch tex_captions(self) const`
- Explanation:
  Applies `tex_captions` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_captions();`
- Signature:
  `TokenBatch tex_footnotes(self) const`
- Explanation:
  Applies `tex_footnotes` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_footnotes();`
- Signature:
  `TokenBatch tex_todos(self) const`
- Explanation:
  Applies `tex_todos` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_todos();`
- Signature:
  `TokenBatch tex_warnings(self) const`
- Explanation:
  Applies `tex_warnings` to `self`. No additional arguments.
- Example:
  `auto tokens = note.tex_warnings();`
- Signature:
  `Note nlp_language(self) const`
- Explanation:
  Applies `nlp_language` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_language();`
- Signature:
  `Note nlp_script(self) const`
- Explanation:
  Applies `nlp_script` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_script();`
- Signature:
  `Note nlp_sentiment(self) const`
- Explanation:
  Applies `nlp_sentiment` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_sentiment();`
- Signature:
  `Note nlp_toxicity(self) const`
- Explanation:
  Applies `nlp_toxicity` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_toxicity();`
- Signature:
  `Note nlp_readability_grade(self) const`
- Explanation:
  Applies `nlp_readability_grade` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_readability_grade();`
- Signature:
  `Note nlp_readability_level(self) const`
- Explanation:
  Applies `nlp_readability_level` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_readability_level();`
- Signature:
  `Note nlp_spam_score(self) const`
- Explanation:
  Applies `nlp_spam_score` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_spam_score();`
- Signature:
  `Note nlp_formality(self) const`
- Explanation:
  Applies `nlp_formality` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_formality();`
- Signature:
  `Note nlp_subjectivity(self) const`
- Explanation:
  Applies `nlp_subjectivity` to `self`. No additional arguments.
- Example:
  `auto out = note.nlp_subjectivity();`
- Signature:
  `Note nlp_mask_pii(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_mask_pii` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_mask_pii(1);`
- Signature:
  `Note nlp_remove_boilerplate(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_remove_boilerplate` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_remove_boilerplate(1);`
- Signature:
  `Note nlp_remove_tables(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_remove_tables` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_remove_tables(1);`
- Signature:
  `Note nlp_remove_headers(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_remove_headers` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_remove_headers(1);`
- Signature:
  `Note nlp_remove_footers(self, std::int64_t flags) const`
- Explanation:
  Applies `nlp_remove_footers` to `self`. Arguments: `std::int64_t flags`.
- Example:
  `auto out = note.nlp_remove_footers(1);`
- Signature:
  `Note to_utf8_strict(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_utf8_strict();`
- Signature:
  `Note to_utf8_lossy(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_utf8_lossy();`
- Signature:
  `Note from_latin1(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.from_latin1();`
- Signature:
  `Note to_latin1_lossy(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_latin1_lossy();`
- Signature:
  `Note from_windows1252(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.from_windows1252();`
- Signature:
  `Note to_windows1252_lossy(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = note.to_windows1252_lossy();`
- Signature:
  `TokenBatch nlp_lowercase_tokens(self) const`
- Explanation:
  Applies `nlp_lowercase_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_lowercase_tokens();`
- Signature:
  `TokenBatch nlp_remove_stopword_tokens(self) const`
- Explanation:
  Applies `nlp_remove_stopword_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_remove_stopword_tokens();`
- Signature:
  `TokenBatch nlp_remove_punct_tokens(self) const`
- Explanation:
  Applies `nlp_remove_punct_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_remove_punct_tokens();`
- Signature:
  `TokenBatch nlp_remove_number_tokens(self) const`
- Explanation:
  Applies `nlp_remove_number_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_remove_number_tokens();`
- Signature:
  `TokenBatch nlp_filter_tokens_alpha(self) const`
- Explanation:
  Applies `nlp_filter_tokens_alpha` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_filter_tokens_alpha();`
- Signature:
  `TokenBatch nlp_filter_tokens_alnum(self) const`
- Explanation:
  Applies `nlp_filter_tokens_alnum` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_filter_tokens_alnum();`
- Signature:
  `TokenBatch nlp_filter_tokens_len_ge(self, std::int64_t n) const`
- Explanation:
  Applies `nlp_filter_tokens_len_ge` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.nlp_filter_tokens_len_ge(1);`
- Signature:
  `TokenBatch nlp_filter_tokens_len_le(self, std::int64_t n) const`
- Explanation:
  Applies `nlp_filter_tokens_len_le` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto tokens = note.nlp_filter_tokens_len_le(1);`
- Signature:
  `TokenBatch nlp_filter_tokens_regex(self, std::string_view pattern) const`
- Explanation:
  Applies `nlp_filter_tokens_regex` to `self`. Arguments: `std::string_view pattern`.
- Example:
  `auto tokens = note.nlp_filter_tokens_regex("sample_text");`
- Signature:
  `TokenBatch nlp_shuffle_tokens(self) const`
- Explanation:
  Applies `nlp_shuffle_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_shuffle_tokens();`
- Signature:
  `TokenBatch nlp_unique_tokens(self) const`
- Explanation:
  Applies `nlp_unique_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_unique_tokens();`
- Signature:
  `TokenBatch nlp_sort_tokens(self) const`
- Explanation:
  Applies `nlp_sort_tokens` to `self`. No additional arguments.
- Example:
  `auto tokens = note.nlp_sort_tokens();`
- Signature:
  `Note transliterate_ascii(self) const`
- Explanation:
  Applies `transliterate_ascii` to `self`. No additional arguments.
- Example:
  `auto out = note.transliterate_ascii();`
- Signature:
  `Note transliterate_latin(self) const`
- Explanation:
  Applies `transliterate_latin` to `self`. No additional arguments.
- Example:
  `auto out = note.transliterate_latin();`
- Signature:
  `Note transliterate_greek(self) const`
- Explanation:
  Applies `transliterate_greek` to `self`. No additional arguments.
- Example:
  `auto out = note.transliterate_greek();`
- Signature:
  `Note transliterate_cyrillic(self) const`
- Explanation:
  Applies `transliterate_cyrillic` to `self`. No additional arguments.
- Example:
  `auto out = note.transliterate_cyrillic();`
- Signature:
  `Note phonetic_soundex(self) const`
- Explanation:
  Applies `phonetic_soundex` to `self`. No additional arguments.
- Example:
  `auto out = note.phonetic_soundex();`
- Signature:
  `Note phonetic_metaphone(self) const`
- Explanation:
  Applies `phonetic_metaphone` to `self`. No additional arguments.
- Example:
  `auto out = note.phonetic_metaphone();`
- Signature:
  `Note phonetic_double_metaphone(self) const`
- Explanation:
  Applies `phonetic_double_metaphone` to `self`. No additional arguments.
- Example:
  `auto out = note.phonetic_double_metaphone();`
- Signature:
  `TokenBatch intersect_acronyms(self, const Note& other) const`
- Explanation:
  Applies `intersect_acronyms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.intersect_acronyms(other_note);`
- Signature:
  `TokenBatch intersect_citations(self, const Note& other) const`
- Explanation:
  Applies `intersect_citations` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.intersect_citations(other_note);`
- Signature:
  `TokenBatch intersect_math_symbols(self, const Note& other) const`
- Explanation:
  Applies `intersect_math_symbols` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.intersect_math_symbols(other_note);`
- Signature:
  `TokenBatch intersect_tokens(self, const Note& other) const`
- Explanation:
  Applies `intersect_tokens` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.intersect_tokens(other_note);`
- Signature:
  `TokenBatch intersect_terms(self, const Note& other) const`
- Explanation:
  Applies `intersect_terms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.intersect_terms(other_note);`
- Signature:
  `Note jaccard_acronyms(self, const Note& other) const`
- Explanation:
  Applies `jaccard_acronyms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.jaccard_acronyms(other_note);`
- Signature:
  `Note jaccard_citations(self, const Note& other) const`
- Explanation:
  Applies `jaccard_citations` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.jaccard_citations(other_note);`
- Signature:
  `Note jaccard_math_symbols(self, const Note& other) const`
- Explanation:
  Applies `jaccard_math_symbols` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.jaccard_math_symbols(other_note);`
- Signature:
  `Note jaccard_tokens(self, const Note& other) const`
- Explanation:
  Applies `jaccard_tokens` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.jaccard_tokens(other_note);`
- Signature:
  `Note jaccard_terms(self, const Note& other) const`
- Explanation:
  Applies `jaccard_terms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.jaccard_terms(other_note);`
- Signature:
  `Note overlap_acronyms(self, const Note& other) const`
- Explanation:
  Applies `overlap_acronyms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.overlap_acronyms(other_note);`
- Signature:
  `Note overlap_citations(self, const Note& other) const`
- Explanation:
  Applies `overlap_citations` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.overlap_citations(other_note);`
- Signature:
  `Note overlap_math_symbols(self, const Note& other) const`
- Explanation:
  Applies `overlap_math_symbols` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.overlap_math_symbols(other_note);`
- Signature:
  `Note overlap_tokens(self, const Note& other) const`
- Explanation:
  Applies `overlap_tokens` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.overlap_tokens(other_note);`
- Signature:
  `Note overlap_terms(self, const Note& other) const`
- Explanation:
  Applies `overlap_terms` to `self`. Arguments: `const Note& other`.
- Example:
  `auto out = note.overlap_terms(other_note);`
- Signature:
  `TokenBatch diff_words(self, const Note& other) const`
- Explanation:
  Applies `diff_words` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.diff_words(other_note);`
- Signature:
  `TokenBatch diff_lines(self, const Note& other) const`
- Explanation:
  Applies `diff_lines` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.diff_lines(other_note);`
- Signature:
  `TokenBatch diff_chars(self, const Note& other) const`
- Explanation:
  Applies `diff_chars` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.diff_chars(other_note);`
- Signature:
  `TokenBatch align_tokens(self, const Note& other) const`
- Explanation:
  Applies `align_tokens` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.align_tokens(other_note);`
- Signature:
  `TokenBatch align_sentences(self, const Note& other) const`
- Explanation:
  Applies `align_sentences` to `self`. Arguments: `const Note& other`.
- Example:
  `auto tokens = note.align_sentences(other_note);`
- Signature:
  `Note deduplicate(self) const`
- Explanation:
  Applies `deduplicate` to `self`. No additional arguments.
- Example:
  `auto out = note.deduplicate();`
- Signature:
  `Note deduplicate(self, std::int64_t dim) const`
- Explanation:
  Applies `deduplicate` to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.deduplicate(1);`
- Signature:
  `Note sort(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = note.sort();`
- Signature:
  `Note sort(self, std::int64_t dim) const`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.sort(1);`
- Signature:
  `Note unique(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = note.unique();`
- Signature:
  `Note unique(self, std::int64_t dim) const`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.unique(1);`
- Signature:
  `Note squeeze(self, std::int64_t dim) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.squeeze(1);`
- Signature:
  `Note unsqueeze(self, std::int64_t dim) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.unsqueeze(1);`
- Signature:
  `Note expand_dims(self, std::int64_t dim) const`
- Explanation:
  Applies `expand_dims` to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = note.expand_dims(1);`
- Signature:
  `Note repeat(self, std::span<const std::int64_t> repeats) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> repeats`.
- Example:
  `auto out = note.repeat(dims_span);`
- Signature:
  `Note repeat_interleave(self, std::int64_t repeats, std::int64_t dim) const`
- Explanation:
  Applies `repeat_interleave` to `self`. Arguments: `std::int64_t repeats, std::int64_t dim`.
- Example:
  `auto out = note.repeat_interleave(1, 1);`
- Signature:
  `Note tile(self, std::span<const std::int64_t> reps) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> reps`.
- Example:
  `auto out = note.tile(dims_span);`
## Demos to run

- `PlussQit/examples/CodeN/note_factory_demo.cpp`
- `PlussQit/examples/CodeN/note_base_methods_demo.cpp`
- `PlussQit/examples/CodeN/note_cleanup_demo.cpp`
- `PlussQit/examples/CodeN/note_method_families_demo.cpp`
- `PlussQit/examples/CodeN/note_printing_demo.cpp`
