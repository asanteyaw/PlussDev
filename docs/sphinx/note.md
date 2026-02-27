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


## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

### `align_sentences`

#### Signature

```cpp
TokenBatch align_sentences(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `align_sentences` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.align_sentences(other_note);
```

### `align_tokens`

#### Signature

```cpp
TokenBatch align_tokens(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `align_tokens` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.align_tokens(other_note);
```

### `base64_decode`

#### Signature

```cpp
Note base64_decode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `base64_decode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.base64_decode();
```

### `base64_encode`

#### Signature

```cpp
Note base64_encode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `base64_encode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.base64_encode();
```

### `capitalize`

#### Signature

```cpp
Note capitalize(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.capitalize();
```

### `casefold`

#### Signature

```cpp
Note casefold(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note casefold(self, std::string_view locale) const`

#### Examples

```cpp
auto out = note.casefold();
```

### `casefold`

#### Signature

```cpp
Note casefold(self, std::string_view locale) const
```

#### Parameters

`locale` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view locale`.

#### See also

- `Note casefold(self) const`

#### Examples

```cpp
auto out = note.casefold("sample_text");
```

### `collapse_newlines`

#### Signature

```cpp
Note collapse_newlines(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `collapse_newlines` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.collapse_newlines();
```

### `collapse_punct`

#### Signature

```cpp
Note collapse_punct(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `collapse_punct` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.collapse_punct();
```

### `collapse_repeats`

#### Signature

```cpp
Note collapse_repeats(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `collapse_repeats` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.collapse_repeats();
```

### `collapse_spaces`

#### Signature

```cpp
Note collapse_spaces(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `collapse_spaces` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.collapse_spaces();
```

### `collapse_whitespace`

#### Signature

```cpp
Note collapse_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.collapse_whitespace();
```

### `contains`

#### Signature

```cpp
std::vector<std::uint8_t> contains(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view needle`.

#### Examples

```cpp
auto mask = note.contains("sample_text");
```

### `contains_any_alpha`

#### Signature

```cpp
std::vector<std::uint8_t> contains_any_alpha(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.contains_any_alpha();
```

### `contains_any_digit`

#### Signature

```cpp
std::vector<std::uint8_t> contains_any_digit(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.contains_any_digit();
```

### `contiguous`

#### Signature

```cpp
Note contiguous(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `contiguous` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.contiguous();
```

### `count_regex`

#### Signature

```cpp
std::vector<std::int64_t> count_regex(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Applies `count_regex` to `self`. Arguments: `std::string_view pattern`.

#### Examples

```cpp
auto counts = note.count_regex("sample_text");
```

### `count_substring`

#### Signature

```cpp
std::vector<std::int64_t> count_substring(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Applies `count_substring` to `self`. Arguments: `std::string_view needle`.

#### Examples

```cpp
auto counts = note.count_substring("sample_text");
```

### `dedent`

#### Signature

```cpp
Note dedent(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.dedent();
```

### `deduplicate`

#### Signature

```cpp
Note deduplicate(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `deduplicate` to `self`. No additional arguments.

#### See also

- `Note deduplicate(self, std::int64_t dim) const`

#### Examples

```cpp
auto out = note.deduplicate();
```

### `deduplicate`

#### Signature

```cpp
Note deduplicate(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `deduplicate` to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Note deduplicate(self) const`

#### Examples

```cpp
auto out = note.deduplicate(1);
```

### `diff_chars`

#### Signature

```cpp
TokenBatch diff_chars(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `diff_chars` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.diff_chars(other_note);
```

### `diff_lines`

#### Signature

```cpp
TokenBatch diff_lines(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `diff_lines` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.diff_lines(other_note);
```

### `diff_words`

#### Signature

```cpp
TokenBatch diff_words(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `diff_words` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.diff_words(other_note);
```

### `ends_with`

#### Signature

```cpp
std::vector<std::uint8_t> ends_with(self, std::string_view suffix) const
```

#### Parameters

`suffix` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view suffix`.

#### Examples

```cpp
auto mask = note.ends_with("sample_text");
```

### `ensure_prefix`

#### Signature

```cpp
Note ensure_prefix(self, std::string_view prefix) const
```

#### Parameters

`prefix` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view prefix`.

#### Examples

```cpp
auto out = note.ensure_prefix("sample_text");
```

### `ensure_suffix`

#### Signature

```cpp
Note ensure_suffix(self, std::string_view suffix) const
```

#### Parameters

`suffix` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view suffix`.

#### Examples

```cpp
auto out = note.ensure_suffix("sample_text");
```

### `expand_dims`

#### Signature

```cpp
Note expand_dims(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `expand_dims` to `self`. Arguments: `std::int64_t dim`.

#### Examples

```cpp
auto out = note.expand_dims(1);
```

### `find_all`

#### Signature

```cpp
TokenBatch find_all(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.

#### See also

- `TokenBatch find_all(self, std::string_view needle, std::int64_t start) const`

#### Examples

```cpp
auto tokens = note.find_all("sample_text");
```

### `find_all`

#### Signature

```cpp
TokenBatch find_all(self, std::string_view needle, std::int64_t start) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

`start` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view needle, std::int64_t start`.

#### See also

- `TokenBatch find_all(self, std::string_view needle) const`

#### Examples

```cpp
auto tokens = note.find_all("sample_text", 1);
```

### `find_all_regex`

#### Signature

```cpp
TokenBatch find_all_regex(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view pattern`.

#### See also

- `TokenBatch find_all_regex(self, std::string_view pattern, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.find_all_regex("sample_text");
```

### `find_all_regex`

#### Signature

```cpp
TokenBatch find_all_regex(self, std::string_view pattern, std::int64_t flags) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::int64_t flags`.

#### See also

- `TokenBatch find_all_regex(self, std::string_view pattern) const`

#### Examples

```cpp
auto tokens = note.find_all_regex("sample_text", 1);
```

### `flatten`

#### Signature

```cpp
Note flatten(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### Examples

```cpp
auto out = note.flatten();
```

### `from_latin1`

#### Signature

```cpp
Note from_latin1(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.from_latin1();
```

### `from_windows1252`

#### Signature

```cpp
Note from_windows1252(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.from_windows1252();
```

### `hex_decode`

#### Signature

```cpp
Note hex_decode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `hex_decode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.hex_decode();
```

### `hex_encode`

#### Signature

```cpp
Note hex_encode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `hex_encode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.hex_encode();
```

### `html_escape`

#### Signature

```cpp
Note html_escape(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `html_escape` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.html_escape();
```

### `html_unescape`

#### Signature

```cpp
Note html_unescape(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `html_unescape` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.html_unescape();
```

### `indent`

#### Signature

```cpp
Note indent(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.indent();
```

### `intersect_acronyms`

#### Signature

```cpp
TokenBatch intersect_acronyms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `intersect_acronyms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.intersect_acronyms(other_note);
```

### `intersect_citations`

#### Signature

```cpp
TokenBatch intersect_citations(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `intersect_citations` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.intersect_citations(other_note);
```

### `intersect_math_symbols`

#### Signature

```cpp
TokenBatch intersect_math_symbols(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `intersect_math_symbols` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.intersect_math_symbols(other_note);
```

### `intersect_terms`

#### Signature

```cpp
TokenBatch intersect_terms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `intersect_terms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.intersect_terms(other_note);
```

### `intersect_tokens`

#### Signature

```cpp
TokenBatch intersect_tokens(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `intersect_tokens` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto tokens = note.intersect_tokens(other_note);
```

### `is_alnum`

#### Signature

```cpp
std::vector<std::uint8_t> is_alnum(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_alnum();
```

### `is_alpha`

#### Signature

```cpp
std::vector<std::uint8_t> is_alpha(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_alpha();
```

### `is_ascii`

#### Signature

```cpp
std::vector<std::uint8_t> is_ascii(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_ascii();
```

### `is_blank`

#### Signature

```cpp
std::vector<std::uint8_t> is_blank(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_blank();
```

### `is_bytes`

#### Signature

```cpp
std::vector<std::uint8_t> is_bytes(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_bytes();
```

### `is_digit`

#### Signature

```cpp
std::vector<std::uint8_t> is_digit(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_digit();
```

### `is_lower`

#### Signature

```cpp
std::vector<std::uint8_t> is_lower(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_lower();
```

### `is_title`

#### Signature

```cpp
std::vector<std::uint8_t> is_title(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_title();
```

### `is_upper`

#### Signature

```cpp
std::vector<std::uint8_t> is_upper(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_upper();
```

### `is_utf8_valid`

#### Signature

```cpp
std::vector<std::uint8_t> is_utf8_valid(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_utf8_valid();
```

### `is_whitespace`

#### Signature

```cpp
std::vector<std::uint8_t> is_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto mask = note.is_whitespace();
```

### `jaccard_acronyms`

#### Signature

```cpp
Note jaccard_acronyms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `jaccard_acronyms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.jaccard_acronyms(other_note);
```

### `jaccard_citations`

#### Signature

```cpp
Note jaccard_citations(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `jaccard_citations` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.jaccard_citations(other_note);
```

### `jaccard_math_symbols`

#### Signature

```cpp
Note jaccard_math_symbols(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `jaccard_math_symbols` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.jaccard_math_symbols(other_note);
```

### `jaccard_terms`

#### Signature

```cpp
Note jaccard_terms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `jaccard_terms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.jaccard_terms(other_note);
```

### `jaccard_tokens`

#### Signature

```cpp
Note jaccard_tokens(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `jaccard_tokens` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.jaccard_tokens(other_note);
```

### `join`

#### Signature

```cpp
Note join(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note join(self, std::string_view sep) const`

#### Examples

```cpp
auto out = note.join();
```

### `join`

#### Signature

```cpp
Note join(self, std::string_view sep) const
```

#### Parameters

`sep` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view sep`.

#### See also

- `Note join(self) const`

#### Examples

```cpp
auto out = note.join("sample_text");
```

### `json_escape`

#### Signature

```cpp
Note json_escape(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `json_escape` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.json_escape();
```

### `keep_alnum`

#### Signature

```cpp
Note keep_alnum(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `keep_alnum` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.keep_alnum();
```

### `keep_alpha`

#### Signature

```cpp
Note keep_alpha(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `keep_alpha` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.keep_alpha();
```

### `keep_ascii`

#### Signature

```cpp
Note keep_ascii(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `keep_ascii` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.keep_ascii();
```

### `keep_digits`

#### Signature

```cpp
Note keep_digits(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `keep_digits` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.keep_digits();
```

### `keep_printable`

#### Signature

```cpp
Note keep_printable(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `keep_printable` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.keep_printable();
```

### `lengths_bytes`

#### Signature

```cpp
std::vector<std::int64_t> lengths_bytes(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Applies `lengths_bytes` to `self`. No additional arguments.

#### Examples

```cpp
auto counts = note.lengths_bytes();
```

### `lengths_chars`

#### Signature

```cpp
std::vector<std::int64_t> lengths_chars(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Applies `lengths_chars` to `self`. No additional arguments.

#### Examples

```cpp
auto counts = note.lengths_chars();
```

### `lengths_graphemes`

#### Signature

```cpp
std::vector<std::int64_t> lengths_graphemes(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Applies `lengths_graphemes` to `self`. No additional arguments.

#### Examples

```cpp
auto counts = note.lengths_graphemes();
```

### `lower`

#### Signature

```cpp
Note lower(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.lower();
```

### `lstrip`

#### Signature

```cpp
Note lstrip(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note lstrip(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.lstrip();
```

### `lstrip`

#### Signature

```cpp
Note lstrip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note lstrip(self) const`

#### Examples

```cpp
auto out = note.lstrip("sample_text");
```

### `narrow`

#### Signature

```cpp
Note narrow(self, std::int64_t dim, std::int64_t start, std::int64_t length) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

`start` : `std::int64_t`
    Required method argument.

`length` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `narrow` to `self`. Arguments: `std::int64_t dim, std::int64_t start, std::int64_t length`.

#### Examples

```cpp
auto out = note.narrow(1, 1, 1);
```

### `ngrams_char`

#### Signature

```cpp
TokenBatch ngrams_char(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `ngrams_char` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.ngrams_char(1);
```

### `ngrams_word`

#### Signature

```cpp
TokenBatch ngrams_word(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `ngrams_word` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.ngrams_word(1);
```

### `nlp_bigrams`

#### Signature

```cpp
TokenBatch nlp_bigrams(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_bigrams` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_bigrams();
```

### `nlp_char_ngrams`

#### Signature

```cpp
TokenBatch nlp_char_ngrams(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_char_ngrams` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.nlp_char_ngrams(1);
```

### `nlp_clean`

#### Signature

```cpp
Note nlp_clean(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_clean` to `self`. No additional arguments.

#### See also

- `Note nlp_clean(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_clean();
```

### `nlp_clean`

#### Signature

```cpp
Note nlp_clean(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_clean` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_clean(self) const`

#### Examples

```cpp
auto out = note.nlp_clean(1);
```

### `nlp_emails`

#### Signature

```cpp
TokenBatch nlp_emails(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_emails` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_emails();
```

### `nlp_expand_contractions`

#### Signature

```cpp
Note nlp_expand_contractions(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_expand_contractions` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_expand_contractions();
```

### `nlp_extract_acronyms`

#### Signature

```cpp
TokenBatch nlp_extract_acronyms(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_acronyms` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_acronyms();
```

### `nlp_extract_bibliography`

#### Signature

```cpp
TokenBatch nlp_extract_bibliography(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_bibliography` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_bibliography();
```

### `nlp_extract_citations`

#### Signature

```cpp
TokenBatch nlp_extract_citations(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_citations` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_citations();
```

### `nlp_extract_definitions`

#### Signature

```cpp
TokenBatch nlp_extract_definitions(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_definitions` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_definitions();
```

### `nlp_extract_math_symbols`

#### Signature

```cpp
TokenBatch nlp_extract_math_symbols(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_math_symbols` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_math_symbols();
```

### `nlp_extract_terms`

#### Signature

```cpp
TokenBatch nlp_extract_terms(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_extract_terms` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_extract_terms();
```

### `nlp_filter_tokens_alnum`

#### Signature

```cpp
TokenBatch nlp_filter_tokens_alnum(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_filter_tokens_alnum` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_filter_tokens_alnum();
```

### `nlp_filter_tokens_alpha`

#### Signature

```cpp
TokenBatch nlp_filter_tokens_alpha(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_filter_tokens_alpha` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_filter_tokens_alpha();
```

### `nlp_filter_tokens_len_ge`

#### Signature

```cpp
TokenBatch nlp_filter_tokens_len_ge(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_filter_tokens_len_ge` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.nlp_filter_tokens_len_ge(1);
```

### `nlp_filter_tokens_len_le`

#### Signature

```cpp
TokenBatch nlp_filter_tokens_len_le(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_filter_tokens_len_le` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.nlp_filter_tokens_len_le(1);
```

### `nlp_filter_tokens_regex`

#### Signature

```cpp
TokenBatch nlp_filter_tokens_regex(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_filter_tokens_regex` to `self`. Arguments: `std::string_view pattern`.

#### Examples

```cpp
auto tokens = note.nlp_filter_tokens_regex("sample_text");
```

### `nlp_formality`

#### Signature

```cpp
Note nlp_formality(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_formality` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_formality();
```

### `nlp_hashtags`

#### Signature

```cpp
TokenBatch nlp_hashtags(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_hashtags` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_hashtags();
```

### `nlp_keyphrases`

#### Signature

```cpp
TokenBatch nlp_keyphrases(self, std::int64_t k) const
```

#### Parameters

`k` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_keyphrases` to `self`. Arguments: `std::int64_t k`.

#### Examples

```cpp
auto tokens = note.nlp_keyphrases(10);
```

### `nlp_keywords`

#### Signature

```cpp
TokenBatch nlp_keywords(self, std::int64_t k) const
```

#### Parameters

`k` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_keywords` to `self`. Arguments: `std::int64_t k`.

#### Examples

```cpp
auto tokens = note.nlp_keywords(10);
```

### `nlp_language`

#### Signature

```cpp
Note nlp_language(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_language` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_language();
```

### `nlp_lemmas`

#### Signature

```cpp
TokenBatch nlp_lemmas(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_lemmas` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_lemmas();
```

### `nlp_lowercase_tokens`

#### Signature

```cpp
TokenBatch nlp_lowercase_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_lowercase_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_lowercase_tokens();
```

### `nlp_mask_currency`

#### Signature

```cpp
Note nlp_mask_currency(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_currency` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_currency();
```

### `nlp_mask_dates`

#### Signature

```cpp
Note nlp_mask_dates(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_dates` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_dates();
```

### `nlp_mask_emails`

#### Signature

```cpp
Note nlp_mask_emails(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_emails` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_emails();
```

### `nlp_mask_hashtags`

#### Signature

```cpp
Note nlp_mask_hashtags(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_hashtags` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_hashtags();
```

### `nlp_mask_mentions`

#### Signature

```cpp
Note nlp_mask_mentions(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_mentions` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_mentions();
```

### `nlp_mask_numbers`

#### Signature

```cpp
Note nlp_mask_numbers(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_numbers` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_numbers();
```

### `nlp_mask_pii`

#### Signature

```cpp
Note nlp_mask_pii(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_pii` to `self`. No additional arguments.

#### See also

- `Note nlp_mask_pii(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_mask_pii();
```

### `nlp_mask_pii`

#### Signature

```cpp
Note nlp_mask_pii(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_pii` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_mask_pii(self) const`

#### Examples

```cpp
auto out = note.nlp_mask_pii(1);
```

### `nlp_mask_times`

#### Signature

```cpp
Note nlp_mask_times(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_times` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_times();
```

### `nlp_mask_urls`

#### Signature

```cpp
Note nlp_mask_urls(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_mask_urls` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_mask_urls();
```

### `nlp_mentions`

#### Signature

```cpp
TokenBatch nlp_mentions(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_mentions` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_mentions();
```

### `nlp_named_entities`

#### Signature

```cpp
TokenBatch nlp_named_entities(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_named_entities` to `self`. No additional arguments.

#### See also

- `TokenBatch nlp_named_entities(self, std::string_view scheme) const`

#### Examples

```cpp
auto tokens = note.nlp_named_entities();
```

### `nlp_named_entities`

#### Signature

```cpp
TokenBatch nlp_named_entities(self, std::string_view scheme) const
```

#### Parameters

`scheme` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_named_entities` to `self`. Arguments: `std::string_view scheme`.

#### See also

- `TokenBatch nlp_named_entities(self) const`

#### Examples

```cpp
auto tokens = note.nlp_named_entities("sample_text");
```

### `nlp_normalize_case`

#### Signature

```cpp
Note nlp_normalize_case(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_case` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_normalize_case();
```

### `nlp_normalize_newlines`

#### Signature

```cpp
Note nlp_normalize_newlines(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_newlines` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_normalize_newlines();
```

### `nlp_normalize_numbers`

#### Signature

```cpp
Note nlp_normalize_numbers(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_numbers` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_normalize_numbers();
```

### `nlp_normalize_punct`

#### Signature

```cpp
Note nlp_normalize_punct(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_punct` to `self`. No additional arguments.

#### See also

- `Note nlp_normalize_punct(self, bool ascii_only) const`

#### Examples

```cpp
auto out = note.nlp_normalize_punct();
```

### `nlp_normalize_punct`

#### Signature

```cpp
Note nlp_normalize_punct(self, bool ascii_only) const
```

#### Parameters

`ascii_only` : `bool`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_punct` to `self`. Arguments: `bool ascii_only`.

#### See also

- `Note nlp_normalize_punct(self) const`

#### Examples

```cpp
auto out = note.nlp_normalize_punct(true);
```

### `nlp_normalize_unicode`

#### Signature

```cpp
Note nlp_normalize_unicode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_unicode` to `self`. No additional arguments.

#### See also

- `Note nlp_normalize_unicode(self, std::int64_t form) const`

#### Examples

```cpp
auto out = note.nlp_normalize_unicode();
```

### `nlp_normalize_unicode`

#### Signature

```cpp
Note nlp_normalize_unicode(self, std::int64_t form) const
```

#### Parameters

`form` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_unicode` to `self`. Arguments: `std::int64_t form`.

#### See also

- `Note nlp_normalize_unicode(self) const`

#### Examples

```cpp
auto out = note.nlp_normalize_unicode(1);
```

### `nlp_normalize_whitespace`

#### Signature

```cpp
Note nlp_normalize_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_normalize_whitespace` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_normalize_whitespace();
```

### `nlp_noun_phrases`

#### Signature

```cpp
TokenBatch nlp_noun_phrases(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_noun_phrases` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_noun_phrases();
```

### `nlp_numbers`

#### Signature

```cpp
TokenBatch nlp_numbers(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_numbers` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_numbers();
```

### `nlp_pos_tags`

#### Signature

```cpp
TokenBatch nlp_pos_tags(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_pos_tags` to `self`. No additional arguments.

#### See also

- `TokenBatch nlp_pos_tags(self, std::string_view tagset) const`

#### Examples

```cpp
auto tokens = note.nlp_pos_tags();
```

### `nlp_pos_tags`

#### Signature

```cpp
TokenBatch nlp_pos_tags(self, std::string_view tagset) const
```

#### Parameters

`tagset` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_pos_tags` to `self`. Arguments: `std::string_view tagset`.

#### See also

- `TokenBatch nlp_pos_tags(self) const`

#### Examples

```cpp
auto tokens = note.nlp_pos_tags("sample_text");
```

### `nlp_readability_grade`

#### Signature

```cpp
Note nlp_readability_grade(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_readability_grade` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_readability_grade();
```

### `nlp_readability_level`

#### Signature

```cpp
Note nlp_readability_level(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_readability_level` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_readability_level();
```

### `nlp_remove_accents`

#### Signature

```cpp
Note nlp_remove_accents(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_accents` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_accents();
```

### `nlp_remove_boilerplate`

#### Signature

```cpp
Note nlp_remove_boilerplate(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_boilerplate` to `self`. No additional arguments.

#### See also

- `Note nlp_remove_boilerplate(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_remove_boilerplate();
```

### `nlp_remove_boilerplate`

#### Signature

```cpp
Note nlp_remove_boilerplate(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_boilerplate` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_remove_boilerplate(self) const`

#### Examples

```cpp
auto out = note.nlp_remove_boilerplate(1);
```

### `nlp_remove_brackets`

#### Signature

```cpp
Note nlp_remove_brackets(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_brackets` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_brackets();
```

### `nlp_remove_bullets`

#### Signature

```cpp
Note nlp_remove_bullets(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_bullets` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_bullets();
```

### `nlp_remove_code`

#### Signature

```cpp
Note nlp_remove_code(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_code` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_code();
```

### `nlp_remove_diacritics`

#### Signature

```cpp
Note nlp_remove_diacritics(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_diacritics` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_diacritics();
```

### `nlp_remove_emails`

#### Signature

```cpp
Note nlp_remove_emails(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_emails` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_emails();
```

### `nlp_remove_footers`

#### Signature

```cpp
Note nlp_remove_footers(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_footers` to `self`. No additional arguments.

#### See also

- `Note nlp_remove_footers(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_remove_footers();
```

### `nlp_remove_footers`

#### Signature

```cpp
Note nlp_remove_footers(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_footers` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_remove_footers(self) const`

#### Examples

```cpp
auto out = note.nlp_remove_footers(1);
```

### `nlp_remove_hashtags`

#### Signature

```cpp
Note nlp_remove_hashtags(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_hashtags` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_hashtags();
```

### `nlp_remove_headers`

#### Signature

```cpp
Note nlp_remove_headers(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_headers` to `self`. No additional arguments.

#### See also

- `Note nlp_remove_headers(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_remove_headers();
```

### `nlp_remove_headers`

#### Signature

```cpp
Note nlp_remove_headers(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_headers` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_remove_headers(self) const`

#### Examples

```cpp
auto out = note.nlp_remove_headers(1);
```

### `nlp_remove_html`

#### Signature

```cpp
Note nlp_remove_html(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_html` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_html();
```

### `nlp_remove_latex`

#### Signature

```cpp
Note nlp_remove_latex(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_latex` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_latex();
```

### `nlp_remove_markdown`

#### Signature

```cpp
Note nlp_remove_markdown(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_markdown` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_markdown();
```

### `nlp_remove_math`

#### Signature

```cpp
Note nlp_remove_math(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_math` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_math();
```

### `nlp_remove_mentions`

#### Signature

```cpp
Note nlp_remove_mentions(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_mentions` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_mentions();
```

### `nlp_remove_nonprintable`

#### Signature

```cpp
Note nlp_remove_nonprintable(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_nonprintable` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_nonprintable();
```

### `nlp_remove_number_tokens`

#### Signature

```cpp
TokenBatch nlp_remove_number_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_number_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_remove_number_tokens();
```

### `nlp_remove_numbers`

#### Signature

```cpp
Note nlp_remove_numbers(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_numbers` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_numbers();
```

### `nlp_remove_parentheticals`

#### Signature

```cpp
Note nlp_remove_parentheticals(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_parentheticals` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_parentheticals();
```

### `nlp_remove_punct`

#### Signature

```cpp
Note nlp_remove_punct(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_punct` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_punct();
```

### `nlp_remove_punct_tokens`

#### Signature

```cpp
TokenBatch nlp_remove_punct_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_punct_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_remove_punct_tokens();
```

### `nlp_remove_quotes`

#### Signature

```cpp
Note nlp_remove_quotes(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_quotes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_quotes();
```

### `nlp_remove_stopword_tokens`

#### Signature

```cpp
TokenBatch nlp_remove_stopword_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_stopword_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_remove_stopword_tokens();
```

### `nlp_remove_stopwords`

#### Signature

```cpp
Note nlp_remove_stopwords(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_stopwords` to `self`. No additional arguments.

#### See also

- `Note nlp_remove_stopwords(self, std::string_view lang) const`

#### Examples

```cpp
auto out = note.nlp_remove_stopwords();
```

### `nlp_remove_stopwords`

#### Signature

```cpp
Note nlp_remove_stopwords(self, std::string_view lang) const
```

#### Parameters

`lang` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_stopwords` to `self`. Arguments: `std::string_view lang`.

#### See also

- `Note nlp_remove_stopwords(self) const`

#### Examples

```cpp
auto out = note.nlp_remove_stopwords("sample_text");
```

### `nlp_remove_tables`

#### Signature

```cpp
Note nlp_remove_tables(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_tables` to `self`. No additional arguments.

#### See also

- `Note nlp_remove_tables(self, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.nlp_remove_tables();
```

### `nlp_remove_tables`

#### Signature

```cpp
Note nlp_remove_tables(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_tables` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `Note nlp_remove_tables(self) const`

#### Examples

```cpp
auto out = note.nlp_remove_tables(1);
```

### `nlp_remove_urls`

#### Signature

```cpp
Note nlp_remove_urls(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_remove_urls` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_remove_urls();
```

### `nlp_script`

#### Signature

```cpp
Note nlp_script(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_script` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_script();
```

### `nlp_sentencecase`

#### Signature

```cpp
Note nlp_sentencecase(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_sentencecase` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_sentencecase();
```

### `nlp_sentences`

#### Signature

```cpp
TokenBatch nlp_sentences(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_sentences` to `self`. No additional arguments.

#### See also

- `TokenBatch nlp_sentences(self, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.nlp_sentences();
```

### `nlp_sentences`

#### Signature

```cpp
TokenBatch nlp_sentences(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_sentences` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `TokenBatch nlp_sentences(self) const`

#### Examples

```cpp
auto tokens = note.nlp_sentences(1);
```

### `nlp_sentiment`

#### Signature

```cpp
Note nlp_sentiment(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_sentiment` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_sentiment();
```

### `nlp_shuffle_tokens`

#### Signature

```cpp
TokenBatch nlp_shuffle_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_shuffle_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_shuffle_tokens();
```

### `nlp_simplify_whitespace`

#### Signature

```cpp
Note nlp_simplify_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_simplify_whitespace` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_simplify_whitespace();
```

### `nlp_sort_tokens`

#### Signature

```cpp
TokenBatch nlp_sort_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_sort_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_sort_tokens();
```

### `nlp_spam_score`

#### Signature

```cpp
Note nlp_spam_score(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_spam_score` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_spam_score();
```

### `nlp_stems`

#### Signature

```cpp
TokenBatch nlp_stems(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_stems` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_stems();
```

### `nlp_subjectivity`

#### Signature

```cpp
Note nlp_subjectivity(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_subjectivity` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_subjectivity();
```

### `nlp_tokens`

#### Signature

```cpp
TokenBatch nlp_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_tokens` to `self`. No additional arguments.

#### See also

- `TokenBatch nlp_tokens(self, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.nlp_tokens();
```

### `nlp_tokens`

#### Signature

```cpp
TokenBatch nlp_tokens(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_tokens` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `TokenBatch nlp_tokens(self) const`

#### Examples

```cpp
auto tokens = note.nlp_tokens(1);
```

### `nlp_toxicity`

#### Signature

```cpp
Note nlp_toxicity(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `nlp_toxicity` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.nlp_toxicity();
```

### `nlp_trigrams`

#### Signature

```cpp
TokenBatch nlp_trigrams(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_trigrams` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_trigrams();
```

### `nlp_unique_tokens`

#### Signature

```cpp
TokenBatch nlp_unique_tokens(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_unique_tokens` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_unique_tokens();
```

### `nlp_urls`

#### Signature

```cpp
TokenBatch nlp_urls(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_urls` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.nlp_urls();
```

### `nlp_word_ngrams`

#### Signature

```cpp
TokenBatch nlp_word_ngrams(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_word_ngrams` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto tokens = note.nlp_word_ngrams(1);
```

### `nlp_words`

#### Signature

```cpp
TokenBatch nlp_words(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_words` to `self`. No additional arguments.

#### See also

- `TokenBatch nlp_words(self, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.nlp_words();
```

### `nlp_words`

#### Signature

```cpp
TokenBatch nlp_words(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `nlp_words` to `self`. Arguments: `std::int64_t flags`.

#### See also

- `TokenBatch nlp_words(self) const`

#### Examples

```cpp
auto tokens = note.nlp_words(1);
```

### `normalize`

#### Signature

```cpp
Note normalize(self, std::int64_t form) const
```

#### Parameters

`form` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `normalize` to `self`. Arguments: `std::int64_t form`.

#### Examples

```cpp
auto out = note.normalize(1);
```

### `normalize_dashes`

#### Signature

```cpp
Note normalize_dashes(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `normalize_dashes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.normalize_dashes();
```

### `normalize_ellipsis`

#### Signature

```cpp
Note normalize_ellipsis(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `normalize_ellipsis` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.normalize_ellipsis();
```

### `normalize_quotes`

#### Signature

```cpp
Note normalize_quotes(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `normalize_quotes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.normalize_quotes();
```

### `normalize_spaces`

#### Signature

```cpp
Note normalize_spaces(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.normalize_spaces();
```

### `overlap_acronyms`

#### Signature

```cpp
Note overlap_acronyms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `overlap_acronyms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.overlap_acronyms(other_note);
```

### `overlap_citations`

#### Signature

```cpp
Note overlap_citations(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `overlap_citations` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.overlap_citations(other_note);
```

### `overlap_math_symbols`

#### Signature

```cpp
Note overlap_math_symbols(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `overlap_math_symbols` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.overlap_math_symbols(other_note);
```

### `overlap_terms`

#### Signature

```cpp
Note overlap_terms(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `overlap_terms` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.overlap_terms(other_note);
```

### `overlap_tokens`

#### Signature

```cpp
Note overlap_tokens(self, const Note& other) const
```

#### Parameters

`other` : `const Note&`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `overlap_tokens` to `self`. Arguments: `const Note& other`.

#### Examples

```cpp
auto out = note.overlap_tokens(other_note);
```

### `permute`

#### Signature

```cpp
Note permute(self, std::span<const std::int64_t> dims) const
```

#### Parameters

`dims` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> dims`.

#### Examples

```cpp
auto out = note.permute(dims_span);
```

### `phonetic_double_metaphone`

#### Signature

```cpp
Note phonetic_double_metaphone(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `phonetic_double_metaphone` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.phonetic_double_metaphone();
```

### `phonetic_metaphone`

#### Signature

```cpp
Note phonetic_metaphone(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `phonetic_metaphone` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.phonetic_metaphone();
```

### `phonetic_soundex`

#### Signature

```cpp
Note phonetic_soundex(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `phonetic_soundex` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.phonetic_soundex();
```

### `quote`

#### Signature

```cpp
Note quote(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.quote();
```

### `regex_findall`

#### Signature

```cpp
TokenBatch regex_findall(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `regex_findall` to `self`. Arguments: `std::string_view pattern`.

#### See also

- `TokenBatch regex_findall(self, std::string_view pattern, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.regex_findall("sample_text");
```

### `regex_findall`

#### Signature

```cpp
TokenBatch regex_findall(self, std::string_view pattern, std::int64_t flags) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `regex_findall` to `self`. Arguments: `std::string_view pattern, std::int64_t flags`.

#### See also

- `TokenBatch regex_findall(self, std::string_view pattern) const`

#### Examples

```cpp
auto tokens = note.regex_findall("sample_text", 1);
```

### `regex_match`

#### Signature

```cpp
std::vector<std::uint8_t> regex_match(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view pattern`.

#### Examples

```cpp
auto mask = note.regex_match("sample_text");
```

### `regex_replace`

#### Signature

```cpp
Note regex_replace(self, std::string_view pattern, std::string_view repl) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::string_view repl`.

#### See also

- `Note regex_replace(self, std::string_view pattern, std::string_view repl, std::int64_t flags) const`

#### Examples

```cpp
auto out = note.regex_replace("old_value", "new_value");
```

### `regex_replace`

#### Signature

```cpp
Note regex_replace(self, std::string_view pattern, std::string_view repl, std::int64_t flags) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

`repl` : `std::string_view`
    Required method argument.

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view pattern, std::string_view repl, std::int64_t flags`.

#### See also

- `Note regex_replace(self, std::string_view pattern, std::string_view repl) const`

#### Examples

```cpp
auto out = note.regex_replace("old_value", "new_value", 1);
```

### `regex_search`

#### Signature

```cpp
std::vector<std::uint8_t> regex_search(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view pattern`.

#### Examples

```cpp
auto mask = note.regex_search("sample_text");
```

### `regex_split`

#### Signature

```cpp
TokenBatch regex_split(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `regex_split` to `self`. Arguments: `std::string_view pattern`.

#### See also

- `TokenBatch regex_split(self, std::string_view pattern, std::int64_t limit) const`

#### Examples

```cpp
auto tokens = note.regex_split("sample_text");
```

### `regex_split`

#### Signature

```cpp
TokenBatch regex_split(self, std::string_view pattern, std::int64_t limit) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

`limit` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `regex_split` to `self`. Arguments: `std::string_view pattern, std::int64_t limit`.

#### See also

- `TokenBatch regex_split(self, std::string_view pattern) const`

#### Examples

```cpp
auto tokens = note.regex_split("sample_text", 1);
```

### `remove`

#### Signature

```cpp
Note remove(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.

#### Examples

```cpp
auto out = note.remove("sample_text");
```

### `remove_all`

#### Signature

```cpp
Note remove_all(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view needle`.

#### Examples

```cpp
auto out = note.remove_all("sample_text");
```

### `remove_control_chars`

#### Signature

```cpp
Note remove_control_chars(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_control_chars` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_control_chars();
```

### `remove_digits`

#### Signature

```cpp
Note remove_digits(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_digits` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_digits();
```

### `remove_emojis`

#### Signature

```cpp
Note remove_emojis(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_emojis` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_emojis();
```

### `remove_letters`

#### Signature

```cpp
Note remove_letters(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_letters` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_letters();
```

### `remove_non_ascii`

#### Signature

```cpp
Note remove_non_ascii(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_non_ascii` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_non_ascii();
```

### `remove_prefix`

#### Signature

```cpp
Note remove_prefix(self, std::string_view prefix) const
```

#### Parameters

`prefix` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view prefix`.

#### Examples

```cpp
auto out = note.remove_prefix("sample_text");
```

### `remove_punctuation`

#### Signature

```cpp
Note remove_punctuation(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_punctuation` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_punctuation();
```

### `remove_suffix`

#### Signature

```cpp
Note remove_suffix(self, std::string_view suffix) const
```

#### Parameters

`suffix` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view suffix`.

#### Examples

```cpp
auto out = note.remove_suffix("sample_text");
```

### `remove_symbols`

#### Signature

```cpp
Note remove_symbols(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `remove_symbols` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.remove_symbols();
```

### `repeat`

#### Signature

```cpp
Note repeat(self, std::span<const std::int64_t> repeats) const
```

#### Parameters

`repeats` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> repeats`.

#### Examples

```cpp
auto out = note.repeat(dims_span);
```

### `repeat_interleave`

#### Signature

```cpp
Note repeat_interleave(self, std::int64_t repeats, std::int64_t dim) const
```

#### Parameters

`repeats` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `repeat_interleave` to `self`. Arguments: `std::int64_t repeats, std::int64_t dim`.

#### Examples

```cpp
auto out = note.repeat_interleave(1, 1);
```

### `replace`

#### Signature

```cpp
Note replace(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    Required method argument.

`to` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.

#### Examples

```cpp
auto out = note.replace("old_value", "new_value");
```

### `replace_first`

#### Signature

```cpp
Note replace_first(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    Required method argument.

`to` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.

#### Examples

```cpp
auto out = note.replace_first("old_value", "new_value");
```

### `replace_last`

#### Signature

```cpp
Note replace_last(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    Required method argument.

`to` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to`.

#### Examples

```cpp
auto out = note.replace_last("old_value", "new_value");
```

### `replace_n`

#### Signature

```cpp
Note replace_n(self, std::string_view from, std::string_view to, std::int64_t n) const
```

#### Parameters

`from` : `std::string_view`
    Required method argument.

`to` : `std::string_view`
    Required method argument.

`n` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view from, std::string_view to, std::int64_t n`.

#### Examples

```cpp
auto out = note.replace_n("old_value", "new_value", 1);
```

### `reshape`

#### Signature

```cpp
Note reshape(self, std::span<const std::int64_t> sizes) const
```

#### Parameters

`sizes` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> sizes`.

#### Examples

```cpp
auto out = note.reshape(dims_span);
```

### `rstrip`

#### Signature

```cpp
Note rstrip(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note rstrip(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.rstrip();
```

### `rstrip`

#### Signature

```cpp
Note rstrip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note rstrip(self) const`

#### Examples

```cpp
auto out = note.rstrip("sample_text");
```

### `select`

#### Signature

```cpp
Note select(self, std::int64_t dim, std::int64_t index) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

`index` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim, std::int64_t index`.

#### Examples

```cpp
auto out = note.select(1, 1);
```

### `sort`

#### Signature

```cpp
Note sort(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### See also

- `Note sort(self, std::int64_t dim) const`

#### Examples

```cpp
auto out = note.sort();
```

### `sort`

#### Signature

```cpp
Note sort(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t dim`.

#### See also

- `Note sort(self) const`

#### Examples

```cpp
auto out = note.sort(1);
```

### `split`

#### Signature

```cpp
TokenBatch split(self, std::string_view delim) const
```

#### Parameters

`delim` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::string_view delim`.

#### See also

- `TokenBatch split(self, std::string_view delim, std::int64_t limit) const`

#### Examples

```cpp
auto tokens = note.split("sample_text");
```

### `split`

#### Signature

```cpp
TokenBatch split(self, std::string_view delim, std::int64_t limit) const
```

#### Parameters

`delim` : `std::string_view`
    Required method argument.

`limit` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::string_view delim, std::int64_t limit`.

#### See also

- `TokenBatch split(self, std::string_view delim) const`

#### Examples

```cpp
auto tokens = note.split("sample_text", 1);
```

### `split_chars`

#### Signature

```cpp
TokenBatch split_chars(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.split_chars();
```

### `split_graphemes`

#### Signature

```cpp
TokenBatch split_graphemes(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.split_graphemes();
```

### `split_lines`

#### Signature

```cpp
TokenBatch split_lines(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `TokenBatch split_lines(self, bool keep_ends) const`

#### Examples

```cpp
auto tokens = note.split_lines();
```

### `split_lines`

#### Signature

```cpp
TokenBatch split_lines(self, bool keep_ends) const
```

#### Parameters

`keep_ends` : `bool`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `bool keep_ends`.

#### See also

- `TokenBatch split_lines(self) const`

#### Examples

```cpp
auto tokens = note.split_lines(true);
```

### `split_on_punct`

#### Signature

```cpp
TokenBatch split_on_punct(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `split_on_punct` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.split_on_punct();
```

### `split_sentences`

#### Signature

```cpp
TokenBatch split_sentences(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `split_sentences` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.split_sentences();
```

### `split_whitespace`

#### Signature

```cpp
TokenBatch split_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `TokenBatch split_whitespace(self, std::int64_t limit) const`

#### Examples

```cpp
auto tokens = note.split_whitespace();
```

### `split_whitespace`

#### Signature

```cpp
TokenBatch split_whitespace(self, std::int64_t limit) const
```

#### Parameters

`limit` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::int64_t limit`.

#### See also

- `TokenBatch split_whitespace(self) const`

#### Examples

```cpp
auto tokens = note.split_whitespace(1);
```

### `squeeze`

#### Signature

```cpp
Note squeeze(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim`.

#### Examples

```cpp
auto out = note.squeeze(1);
```

### `starts_with`

#### Signature

```cpp
std::vector<std::uint8_t> starts_with(self, std::string_view prefix) const
```

#### Parameters

`prefix` : `std::string_view`
    Required method argument.

#### Returns

`std::vector<std::uint8_t>`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. Arguments: `std::string_view prefix`.

#### Examples

```cpp
auto mask = note.starts_with("sample_text");
```

### `strip`

#### Signature

```cpp
Note strip(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note strip(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.strip();
```

### `strip`

#### Signature

```cpp
Note strip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note strip(self) const`

#### Examples

```cpp
auto out = note.strip("sample_text");
```

### `strip_accents`

#### Signature

```cpp
Note strip_accents(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `strip_accents` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.strip_accents();
```

### `strip_bom`

#### Signature

```cpp
Note strip_bom(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `strip_bom` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.strip_bom();
```

### `strip_diacritics`

#### Signature

```cpp
Note strip_diacritics(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `strip_diacritics` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.strip_diacritics();
```

### `strip_zero_width`

#### Signature

```cpp
Note strip_zero_width(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `strip_zero_width` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.strip_zero_width();
```

### `surround`

#### Signature

```cpp
Note surround(self, std::string_view left, std::string_view right) const
```

#### Parameters

`left` : `std::string_view`
    Required method argument.

`right` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view left, std::string_view right`.

#### Examples

```cpp
auto out = note.surround("old_value", "new_value");
```

### `swapcase`

#### Signature

```cpp
Note swapcase(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.swapcase();
```

### `tex_add_math_delimiters`

#### Signature

```cpp
Note tex_add_math_delimiters(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_add_math_delimiters` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_add_math_delimiters();
```

### `tex_align_blocks`

#### Signature

```cpp
TokenBatch tex_align_blocks(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_align_blocks` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_align_blocks();
```

### `tex_arrays`

#### Signature

```cpp
TokenBatch tex_arrays(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_arrays` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_arrays();
```

### `tex_balance_braces`

#### Signature

```cpp
Note tex_balance_braces(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_balance_braces` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_balance_braces();
```

### `tex_balance_math_delims`

#### Signature

```cpp
Note tex_balance_math_delims(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_balance_math_delims` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_balance_math_delims();
```

### `tex_bibliography_keys`

#### Signature

```cpp
TokenBatch tex_bibliography_keys(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_bibliography_keys` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_bibliography_keys();
```

### `tex_canonicalize_macros`

#### Signature

```cpp
Note tex_canonicalize_macros(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_canonicalize_macros` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_canonicalize_macros();
```

### `tex_captions`

#### Signature

```cpp
TokenBatch tex_captions(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_captions` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_captions();
```

### `tex_citations`

#### Signature

```cpp
TokenBatch tex_citations(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_citations` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_citations();
```

### `tex_collapse_macros`

#### Signature

```cpp
Note tex_collapse_macros(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_collapse_macros` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_collapse_macros();
```

### `tex_collapse_whitespace`

#### Signature

```cpp
Note tex_collapse_whitespace(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_collapse_whitespace` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_collapse_whitespace();
```

### `tex_command_names`

#### Signature

```cpp
TokenBatch tex_command_names(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_command_names` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_command_names();
```

### `tex_commands`

#### Signature

```cpp
TokenBatch tex_commands(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_commands` to `self`. No additional arguments.

#### See also

- `TokenBatch tex_commands(self, bool with_args) const`

#### Examples

```cpp
auto tokens = note.tex_commands();
```

### `tex_commands`

#### Signature

```cpp
TokenBatch tex_commands(self, bool with_args) const
```

#### Parameters

`with_args` : `bool`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_commands` to `self`. Arguments: `bool with_args`.

#### See also

- `TokenBatch tex_commands(self) const`

#### Examples

```cpp
auto tokens = note.tex_commands(true);
```

### `tex_convert_frac_to_slash`

#### Signature

```cpp
Note tex_convert_frac_to_slash(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_convert_frac_to_slash` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_convert_frac_to_slash();
```

### `tex_convert_pm_to_plusminus`

#### Signature

```cpp
Note tex_convert_pm_to_plusminus(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_convert_pm_to_plusminus` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_convert_pm_to_plusminus();
```

### `tex_convert_sqrt_to_pow`

#### Signature

```cpp
Note tex_convert_sqrt_to_pow(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_convert_sqrt_to_pow` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_convert_sqrt_to_pow();
```

### `tex_corollaries`

#### Signature

```cpp
TokenBatch tex_corollaries(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_corollaries` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_corollaries();
```

### `tex_dedup_blank_lines`

#### Signature

```cpp
Note tex_dedup_blank_lines(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_dedup_blank_lines` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_dedup_blank_lines();
```

### `tex_definitions`

#### Signature

```cpp
TokenBatch tex_definitions(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_definitions` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_definitions();
```

### `tex_display_math`

#### Signature

```cpp
TokenBatch tex_display_math(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_display_math` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_display_math();
```

### `tex_environment_names`

#### Signature

```cpp
TokenBatch tex_environment_names(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_environment_names` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_environment_names();
```

### `tex_environments`

#### Signature

```cpp
TokenBatch tex_environments(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_environments` to `self`. No additional arguments.

#### See also

- `TokenBatch tex_environments(self, bool with_body) const`

#### Examples

```cpp
auto tokens = note.tex_environments();
```

### `tex_environments`

#### Signature

```cpp
TokenBatch tex_environments(self, bool with_body) const
```

#### Parameters

`with_body` : `bool`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_environments` to `self`. Arguments: `bool with_body`.

#### See also

- `TokenBatch tex_environments(self) const`

#### Examples

```cpp
auto tokens = note.tex_environments(true);
```

### `tex_equations`

#### Signature

```cpp
TokenBatch tex_equations(self, std::int64_t max_count) const
```

#### Parameters

`max_count` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_equations` to `self`. Arguments: `std::int64_t max_count`.

#### Examples

```cpp
auto tokens = note.tex_equations(1);
```

### `tex_escape`

#### Signature

```cpp
Note tex_escape(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_escape` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_escape();
```

### `tex_examples`

#### Signature

```cpp
TokenBatch tex_examples(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_examples` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_examples();
```

### `tex_expand_inputs`

#### Signature

```cpp
Note tex_expand_inputs(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_expand_inputs` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_expand_inputs();
```

### `tex_expand_macros`

#### Signature

```cpp
Note tex_expand_macros(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_expand_macros` to `self`. No additional arguments.

#### See also

- `Note tex_expand_macros(self, std::int64_t max_passes) const`

#### Examples

```cpp
auto out = note.tex_expand_macros();
```

### `tex_expand_macros`

#### Signature

```cpp
Note tex_expand_macros(self, std::int64_t max_passes) const
```

#### Parameters

`max_passes` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_expand_macros` to `self`. Arguments: `std::int64_t max_passes`.

#### See also

- `Note tex_expand_macros(self) const`

#### Examples

```cpp
auto out = note.tex_expand_macros(1);
```

### `tex_extract_command_args`

#### Signature

```cpp
TokenBatch tex_extract_command_args(self, std::string_view cmd) const
```

#### Parameters

`cmd` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_command_args` to `self`. Arguments: `std::string_view cmd`.

#### Examples

```cpp
auto tokens = note.tex_extract_command_args("sample_text");
```

### `tex_extract_env_align`

#### Signature

```cpp
TokenBatch tex_extract_env_align(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_align` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_align();
```

### `tex_extract_env_align_`

#### Signature

```cpp
TokenBatch tex_extract_env_align_(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

In-place variant of `tex_extract_env_align`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_align_();
```

### `tex_extract_env_array`

#### Signature

```cpp
TokenBatch tex_extract_env_array(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_array` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_array();
```

### `tex_extract_env_cases`

#### Signature

```cpp
TokenBatch tex_extract_env_cases(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_cases` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_cases();
```

### `tex_extract_env_enumerate`

#### Signature

```cpp
TokenBatch tex_extract_env_enumerate(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_enumerate` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_enumerate();
```

### `tex_extract_env_equation`

#### Signature

```cpp
TokenBatch tex_extract_env_equation(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_equation` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_equation();
```

### `tex_extract_env_figure`

#### Signature

```cpp
TokenBatch tex_extract_env_figure(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_figure` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_figure();
```

### `tex_extract_env_gather`

#### Signature

```cpp
TokenBatch tex_extract_env_gather(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_gather` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_gather();
```

### `tex_extract_env_itemize`

#### Signature

```cpp
TokenBatch tex_extract_env_itemize(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_itemize` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_itemize();
```

### `tex_extract_env_multline`

#### Signature

```cpp
TokenBatch tex_extract_env_multline(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_multline` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_multline();
```

### `tex_extract_env_table`

#### Signature

```cpp
TokenBatch tex_extract_env_table(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_table` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_table();
```

### `tex_extract_env_tabular`

#### Signature

```cpp
TokenBatch tex_extract_env_tabular(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_env_tabular` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_env_tabular();
```

### `tex_extract_environment`

#### Signature

```cpp
Note tex_extract_environment(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_environment` to `self`. Arguments: `std::string_view name`.

#### Examples

```cpp
auto out = note.tex_extract_environment("sample_text");
```

### `tex_extract_functions`

#### Signature

```cpp
TokenBatch tex_extract_functions(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_functions` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_functions();
```

### `tex_extract_greek`

#### Signature

```cpp
TokenBatch tex_extract_greek(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_greek` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_greek();
```

### `tex_extract_identifiers`

#### Signature

```cpp
TokenBatch tex_extract_identifiers(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_identifiers` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_identifiers();
```

### `tex_extract_numbers`

#### Signature

```cpp
TokenBatch tex_extract_numbers(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_numbers` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_numbers();
```

### `tex_extract_operators`

#### Signature

```cpp
TokenBatch tex_extract_operators(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_operators` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_operators();
```

### `tex_extract_symbols`

#### Signature

```cpp
TokenBatch tex_extract_symbols(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_extract_symbols` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_extract_symbols();
```

### `tex_figures`

#### Signature

```cpp
TokenBatch tex_figures(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_figures` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_figures();
```

### `tex_fix_linebreaks`

#### Signature

```cpp
Note tex_fix_linebreaks(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_fix_linebreaks` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_fix_linebreaks();
```

### `tex_footnotes`

#### Signature

```cpp
TokenBatch tex_footnotes(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_footnotes` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_footnotes();
```

### `tex_headings`

#### Signature

```cpp
TokenBatch tex_headings(self, std::int64_t max_depth) const
```

#### Parameters

`max_depth` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_headings` to `self`. Arguments: `std::int64_t max_depth`.

#### Examples

```cpp
auto tokens = note.tex_headings(1);
```

### `tex_includes`

#### Signature

```cpp
TokenBatch tex_includes(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_includes` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_includes();
```

### `tex_inline_macros`

#### Signature

```cpp
Note tex_inline_macros(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_inline_macros` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_inline_macros();
```

### `tex_inline_math`

#### Signature

```cpp
TokenBatch tex_inline_math(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_inline_math` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_inline_math();
```

### `tex_labels`

#### Signature

```cpp
TokenBatch tex_labels(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_labels` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_labels();
```

### `tex_lemmas`

#### Signature

```cpp
TokenBatch tex_lemmas(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_lemmas` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_lemmas();
```

### `tex_macros_defined`

#### Signature

```cpp
TokenBatch tex_macros_defined(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_macros_defined` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_macros_defined();
```

### `tex_macros_used`

#### Signature

```cpp
TokenBatch tex_macros_used(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_macros_used` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_macros_used();
```

### `tex_math_blocks`

#### Signature

```cpp
TokenBatch tex_math_blocks(self, std::int64_t max_count) const
```

#### Parameters

`max_count` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_math_blocks` to `self`. Arguments: `std::int64_t max_count`.

#### Examples

```cpp
auto tokens = note.tex_math_blocks(1);
```

### `tex_math_only`

#### Signature

```cpp
Note tex_math_only(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_math_only` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_math_only();
```

### `tex_normalize_newlines`

#### Signature

```cpp
Note tex_normalize_newlines(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_normalize_newlines` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_normalize_newlines();
```

### `tex_packages`

#### Signature

```cpp
TokenBatch tex_packages(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_packages` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_packages();
```

### `tex_proofs`

#### Signature

```cpp
TokenBatch tex_proofs(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_proofs` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_proofs();
```

### `tex_refs`

#### Signature

```cpp
TokenBatch tex_refs(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_refs` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_refs();
```

### `tex_remove_accents`

#### Signature

```cpp
Note tex_remove_accents(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_accents` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_accents();
```

### `tex_remove_bibliography`

#### Signature

```cpp
Note tex_remove_bibliography(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_bibliography` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_bibliography();
```

### `tex_remove_big_delims`

#### Signature

```cpp
Note tex_remove_big_delims(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_big_delims` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_big_delims();
```

### `tex_remove_citations`

#### Signature

```cpp
Note tex_remove_citations(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_citations` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_citations();
```

### `tex_remove_color`

#### Signature

```cpp
Note tex_remove_color(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_color` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_color();
```

### `tex_remove_commands`

#### Signature

```cpp
Note tex_remove_commands(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_commands` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_commands();
```

### `tex_remove_env_align`

#### Signature

```cpp
Note tex_remove_env_align(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_align` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_align();
```

### `tex_remove_env_align_`

#### Signature

```cpp
Note tex_remove_env_align_(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

In-place variant of `tex_remove_env_align`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_align_();
```

### `tex_remove_env_array`

#### Signature

```cpp
Note tex_remove_env_array(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_array` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_array();
```

### `tex_remove_env_cases`

#### Signature

```cpp
Note tex_remove_env_cases(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_cases` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_cases();
```

### `tex_remove_env_enumerate`

#### Signature

```cpp
Note tex_remove_env_enumerate(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_enumerate` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_enumerate();
```

### `tex_remove_env_equation`

#### Signature

```cpp
Note tex_remove_env_equation(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_equation` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_equation();
```

### `tex_remove_env_figure`

#### Signature

```cpp
Note tex_remove_env_figure(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_figure` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_figure();
```

### `tex_remove_env_gather`

#### Signature

```cpp
Note tex_remove_env_gather(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_gather` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_gather();
```

### `tex_remove_env_itemize`

#### Signature

```cpp
Note tex_remove_env_itemize(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_itemize` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_itemize();
```

### `tex_remove_env_multline`

#### Signature

```cpp
Note tex_remove_env_multline(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_multline` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_multline();
```

### `tex_remove_env_table`

#### Signature

```cpp
Note tex_remove_env_table(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_table` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_table();
```

### `tex_remove_env_tabular`

#### Signature

```cpp
Note tex_remove_env_tabular(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_env_tabular` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_env_tabular();
```

### `tex_remove_environment`

#### Signature

```cpp
Note tex_remove_environment(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_environment` to `self`. Arguments: `std::string_view name`.

#### Examples

```cpp
auto out = note.tex_remove_environment("sample_text");
```

### `tex_remove_environments`

#### Signature

```cpp
Note tex_remove_environments(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_environments` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_environments();
```

### `tex_remove_figures`

#### Signature

```cpp
Note tex_remove_figures(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_figures` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_figures();
```

### `tex_remove_font_commands`

#### Signature

```cpp
Note tex_remove_font_commands(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_font_commands` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_font_commands();
```

### `tex_remove_formatting`

#### Signature

```cpp
Note tex_remove_formatting(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_formatting` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_formatting();
```

### `tex_remove_graphics`

#### Signature

```cpp
Note tex_remove_graphics(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_graphics` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_graphics();
```

### `tex_remove_hyperref`

#### Signature

```cpp
Note tex_remove_hyperref(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_hyperref` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_hyperref();
```

### `tex_remove_labels`

#### Signature

```cpp
Note tex_remove_labels(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_labels` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_labels();
```

### `tex_remove_left_right`

#### Signature

```cpp
Note tex_remove_left_right(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_left_right` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_left_right();
```

### `tex_remove_macro`

#### Signature

```cpp
Note tex_remove_macro(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_macro` to `self`. Arguments: `std::string_view name`.

#### Examples

```cpp
auto out = note.tex_remove_macro("sample_text");
```

### `tex_remove_math_delimiters`

#### Signature

```cpp
Note tex_remove_math_delimiters(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_math_delimiters` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_math_delimiters();
```

### `tex_remove_packages`

#### Signature

```cpp
Note tex_remove_packages(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_packages` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_packages();
```

### `tex_remove_refs`

#### Signature

```cpp
Note tex_remove_refs(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_refs` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_refs();
```

### `tex_remove_spacing_cmds`

#### Signature

```cpp
Note tex_remove_spacing_cmds(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_spacing_cmds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_spacing_cmds();
```

### `tex_remove_tables`

#### Signature

```cpp
Note tex_remove_tables(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_tables` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_tables();
```

### `tex_remove_urls`

#### Signature

```cpp
Note tex_remove_urls(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_remove_urls` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_remove_urls();
```

### `tex_replace_alpha`

#### Signature

```cpp
Note tex_replace_alpha(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_alpha` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_alpha("sample_text");
```

### `tex_replace_beta`

#### Signature

```cpp
Note tex_replace_beta(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_beta` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_beta("sample_text");
```

### `tex_replace_command`

#### Signature

```cpp
Note tex_replace_command(self, std::string_view cmd, std::string_view repl) const
```

#### Parameters

`cmd` : `std::string_view`
    Required method argument.

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_command` to `self`. Arguments: `std::string_view cmd, std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_command("old_value", "new_value");
```

### `tex_replace_delta`

#### Signature

```cpp
Note tex_replace_delta(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_delta` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_delta("sample_text");
```

### `tex_replace_epsilon`

#### Signature

```cpp
Note tex_replace_epsilon(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_epsilon` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_epsilon("sample_text");
```

### `tex_replace_gamma`

#### Signature

```cpp
Note tex_replace_gamma(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_gamma` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_gamma("sample_text");
```

### `tex_replace_lambda`

#### Signature

```cpp
Note tex_replace_lambda(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_lambda` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_lambda("sample_text");
```

### `tex_replace_mu`

#### Signature

```cpp
Note tex_replace_mu(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_mu` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_mu("sample_text");
```

### `tex_replace_omega`

#### Signature

```cpp
Note tex_replace_omega(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_omega` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_omega("sample_text");
```

### `tex_replace_phi`

#### Signature

```cpp
Note tex_replace_phi(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_phi` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_phi("sample_text");
```

### `tex_replace_pi`

#### Signature

```cpp
Note tex_replace_pi(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_pi` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_pi("sample_text");
```

### `tex_replace_sigma`

#### Signature

```cpp
Note tex_replace_sigma(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_sigma` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_sigma("sample_text");
```

### `tex_replace_theta`

#### Signature

```cpp
Note tex_replace_theta(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_theta` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_theta("sample_text");
```

### `tex_replace_varepsilon`

#### Signature

```cpp
Note tex_replace_varepsilon(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_varepsilon` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_varepsilon("sample_text");
```

### `tex_replace_varphi`

#### Signature

```cpp
Note tex_replace_varphi(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_varphi` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_varphi("sample_text");
```

### `tex_replace_vartheta`

#### Signature

```cpp
Note tex_replace_vartheta(self, std::string_view repl) const
```

#### Parameters

`repl` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_replace_vartheta` to `self`. Arguments: `std::string_view repl`.

#### Examples

```cpp
auto out = note.tex_replace_vartheta("sample_text");
```

### `tex_resolve_graphics`

#### Signature

```cpp
Note tex_resolve_graphics(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_resolve_graphics` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_resolve_graphics();
```

### `tex_resolve_includes`

#### Signature

```cpp
Note tex_resolve_includes(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_resolve_includes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_resolve_includes();
```

### `tex_sanitize`

#### Signature

```cpp
Note tex_sanitize(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_sanitize` to `self`. No additional arguments.

#### See also

- `Note tex_sanitize(self, std::int64_t level) const`

#### Examples

```cpp
auto out = note.tex_sanitize();
```

### `tex_sanitize`

#### Signature

```cpp
Note tex_sanitize(self, std::int64_t level) const
```

#### Parameters

`level` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_sanitize` to `self`. Arguments: `std::int64_t level`.

#### See also

- `Note tex_sanitize(self) const`

#### Examples

```cpp
auto out = note.tex_sanitize(1);
```

### `tex_sections`

#### Signature

```cpp
TokenBatch tex_sections(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_sections` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_sections();
```

### `tex_set_macro`

#### Signature

```cpp
Note tex_set_macro(self, std::string_view name, std::string_view body) const
```

#### Parameters

`name` : `std::string_view`
    Required method argument.

`body` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_set_macro` to `self`. Arguments: `std::string_view name, std::string_view body`.

#### Examples

```cpp
auto out = note.tex_set_macro("old_value", "new_value");
```

### `tex_simplify_fractions`

#### Signature

```cpp
Note tex_simplify_fractions(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_simplify_fractions` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_simplify_fractions();
```

### `tex_simplify_roots`

#### Signature

```cpp
Note tex_simplify_roots(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_simplify_roots` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_simplify_roots();
```

### `tex_simplify_spacing`

#### Signature

```cpp
Note tex_simplify_spacing(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_simplify_spacing` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_simplify_spacing();
```

### `tex_standardize_dots`

#### Signature

```cpp
Note tex_standardize_dots(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_dots` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_dots();
```

### `tex_standardize_integrals`

#### Signature

```cpp
Note tex_standardize_integrals(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_integrals` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_integrals();
```

### `tex_standardize_limits`

#### Signature

```cpp
Note tex_standardize_limits(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_limits` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_limits();
```

### `tex_standardize_matrices`

#### Signature

```cpp
Note tex_standardize_matrices(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_matrices` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_matrices();
```

### `tex_standardize_primes`

#### Signature

```cpp
Note tex_standardize_primes(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_primes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_primes();
```

### `tex_standardize_subscripts`

#### Signature

```cpp
Note tex_standardize_subscripts(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_subscripts` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_subscripts();
```

### `tex_standardize_summations`

#### Signature

```cpp
Note tex_standardize_summations(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_summations` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_summations();
```

### `tex_standardize_superscripts`

#### Signature

```cpp
Note tex_standardize_superscripts(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_superscripts` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_superscripts();
```

### `tex_standardize_vectors`

#### Signature

```cpp
Note tex_standardize_vectors(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_standardize_vectors` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_standardize_vectors();
```

### `tex_strip_commands`

#### Signature

```cpp
Note tex_strip_commands(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_commands` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_commands();
```

### `tex_strip_comments`

#### Signature

```cpp
Note tex_strip_comments(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_comments` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_comments();
```

### `tex_strip_environments`

#### Signature

```cpp
Note tex_strip_environments(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_environments` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_environments();
```

### `tex_strip_math`

#### Signature

```cpp
Note tex_strip_math(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_math` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_math();
```

### `tex_strip_metadata`

#### Signature

```cpp
Note tex_strip_metadata(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_metadata` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_metadata();
```

### `tex_strip_preamble`

#### Signature

```cpp
Note tex_strip_preamble(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_preamble` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_preamble();
```

### `tex_strip_text`

#### Signature

```cpp
Note tex_strip_text(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_strip_text` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_strip_text();
```

### `tex_subsections`

#### Signature

```cpp
TokenBatch tex_subsections(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_subsections` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_subsections();
```

### `tex_subsubsections`

#### Signature

```cpp
TokenBatch tex_subsubsections(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_subsubsections` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_subsubsections();
```

### `tex_tables`

#### Signature

```cpp
TokenBatch tex_tables(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_tables` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_tables();
```

### `tex_tabular_blocks`

#### Signature

```cpp
TokenBatch tex_tabular_blocks(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_tabular_blocks` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_tabular_blocks();
```

### `tex_text_only`

#### Signature

```cpp
Note tex_text_only(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_text_only` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_text_only();
```

### `tex_theorems`

#### Signature

```cpp
TokenBatch tex_theorems(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_theorems` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_theorems();
```

### `tex_to_html`

#### Signature

```cpp
Note tex_to_html(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_html` to `self`. No additional arguments.

#### See also

- `Note tex_to_html(self, std::int64_t mode) const`

#### Examples

```cpp
auto out = note.tex_to_html();
```

### `tex_to_html`

#### Signature

```cpp
Note tex_to_html(self, std::int64_t mode) const
```

#### Parameters

`mode` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_html` to `self`. Arguments: `std::int64_t mode`.

#### See also

- `Note tex_to_html(self) const`

#### Examples

```cpp
auto out = note.tex_to_html(1);
```

### `tex_to_markdown`

#### Signature

```cpp
Note tex_to_markdown(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_markdown` to `self`. No additional arguments.

#### See also

- `Note tex_to_markdown(self, std::int64_t mode) const`

#### Examples

```cpp
auto out = note.tex_to_markdown();
```

### `tex_to_markdown`

#### Signature

```cpp
Note tex_to_markdown(self, std::int64_t mode) const
```

#### Parameters

`mode` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_markdown` to `self`. Arguments: `std::int64_t mode`.

#### See also

- `Note tex_to_markdown(self) const`

#### Examples

```cpp
auto out = note.tex_to_markdown(1);
```

### `tex_to_text`

#### Signature

```cpp
Note tex_to_text(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_text` to `self`. No additional arguments.

#### See also

- `Note tex_to_text(self, std::int64_t mode) const`

#### Examples

```cpp
auto out = note.tex_to_text();
```

### `tex_to_text`

#### Signature

```cpp
Note tex_to_text(self, std::int64_t mode) const
```

#### Parameters

`mode` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_to_text` to `self`. Arguments: `std::int64_t mode`.

#### See also

- `Note tex_to_text(self) const`

#### Examples

```cpp
auto out = note.tex_to_text(1);
```

### `tex_todos`

#### Signature

```cpp
TokenBatch tex_todos(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_todos` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_todos();
```

### `tex_unescape`

#### Signature

```cpp
Note tex_unescape(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_unescape` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.tex_unescape();
```

### `tex_warnings`

#### Signature

```cpp
TokenBatch tex_warnings(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tex_warnings` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tex_warnings();
```

### `tex_wrap_display_math`

#### Signature

```cpp
Note tex_wrap_display_math(self, std::string_view delim) const
```

#### Parameters

`delim` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_wrap_display_math` to `self`. Arguments: `std::string_view delim`.

#### Examples

```cpp
auto out = note.tex_wrap_display_math("sample_text");
```

### `tex_wrap_inline_math`

#### Signature

```cpp
Note tex_wrap_inline_math(self, std::string_view delim) const
```

#### Parameters

`delim` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `tex_wrap_inline_math` to `self`. Arguments: `std::string_view delim`.

#### Examples

```cpp
auto out = note.tex_wrap_inline_math("sample_text");
```

### `tile`

#### Signature

```cpp
Note tile(self, std::span<const std::int64_t> reps) const
```

#### Parameters

`reps` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t> reps`.

#### Examples

```cpp
auto out = note.tile(dims_span);
```

### `title`

#### Signature

```cpp
Note title(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.title();
```

### `to_ascii`

#### Signature

```cpp
Note to_ascii(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_ascii();
```

### `to_latin1_lossy`

#### Signature

```cpp
Note to_latin1_lossy(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_latin1_lossy();
```

### `to_nfc`

#### Signature

```cpp
Note to_nfc(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_nfc();
```

### `to_nfd`

#### Signature

```cpp
Note to_nfd(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_nfd();
```

### `to_nfkc`

#### Signature

```cpp
Note to_nfkc(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_nfkc();
```

### `to_nfkd`

#### Signature

```cpp
Note to_nfkd(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_nfkd();
```

### `to_utf8_lossy`

#### Signature

```cpp
Note to_utf8_lossy(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_utf8_lossy();
```

### `to_utf8_strict`

#### Signature

```cpp
Note to_utf8_strict(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_utf8_strict();
```

### `to_windows1252_lossy`

#### Signature

```cpp
Note to_windows1252_lossy(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = note.to_windows1252_lossy();
```

### `tokenize_punct`

#### Signature

```cpp
TokenBatch tokenize_punct(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies `tokenize_punct` to `self`. No additional arguments.

#### Examples

```cpp
auto tokens = note.tokenize_punct();
```

### `tokenize_regex`

#### Signature

```cpp
TokenBatch tokenize_regex(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view pattern`.

#### Examples

```cpp
auto tokens = note.tokenize_regex("sample_text");
```

### `tokenize_sentences`

#### Signature

```cpp
TokenBatch tokenize_sentences(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `TokenBatch tokenize_sentences(self, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.tokenize_sentences();
```

### `tokenize_sentences`

#### Signature

```cpp
TokenBatch tokenize_sentences(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::int64_t flags`.

#### See also

- `TokenBatch tokenize_sentences(self) const`

#### Examples

```cpp
auto tokens = note.tokenize_sentences(1);
```

### `tokenize_words`

#### Signature

```cpp
TokenBatch tokenize_words(self) const
```

#### Parameters

`None`

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `TokenBatch tokenize_words(self, std::int64_t flags) const`

#### Examples

```cpp
auto tokens = note.tokenize_words();
```

### `tokenize_words`

#### Signature

```cpp
TokenBatch tokenize_words(self, std::int64_t flags) const
```

#### Parameters

`flags` : `std::int64_t`
    Required method argument.

#### Returns

`TokenBatch`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::int64_t flags`.

#### See also

- `TokenBatch tokenize_words(self) const`

#### Examples

```cpp
auto tokens = note.tokenize_words(1);
```

### `transliterate_ascii`

#### Signature

```cpp
Note transliterate_ascii(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `transliterate_ascii` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.transliterate_ascii();
```

### `transliterate_cyrillic`

#### Signature

```cpp
Note transliterate_cyrillic(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `transliterate_cyrillic` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.transliterate_cyrillic();
```

### `transliterate_greek`

#### Signature

```cpp
Note transliterate_greek(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `transliterate_greek` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.transliterate_greek();
```

### `transliterate_latin`

#### Signature

```cpp
Note transliterate_latin(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `transliterate_latin` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.transliterate_latin();
```

### `transpose`

#### Signature

```cpp
Note transpose(self, std::int64_t dim0, std::int64_t dim1) const
```

#### Parameters

`dim0` : `std::int64_t`
    Required method argument.

`dim1` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim0, std::int64_t dim1`.

#### Examples

```cpp
auto out = note.transpose(1, 1);
```

### `trim`

#### Signature

```cpp
Note trim(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note trim(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.trim();
```

### `trim`

#### Signature

```cpp
Note trim(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note trim(self) const`

#### Examples

```cpp
auto out = note.trim("sample_text");
```

### `trim_left`

#### Signature

```cpp
Note trim_left(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note trim_left(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.trim_left();
```

### `trim_left`

#### Signature

```cpp
Note trim_left(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note trim_left(self) const`

#### Examples

```cpp
auto out = note.trim_left("sample_text");
```

### `trim_right`

#### Signature

```cpp
Note trim_right(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### See also

- `Note trim_right(self, std::string_view chars) const`

#### Examples

```cpp
auto out = note.trim_right();
```

### `trim_right`

#### Signature

```cpp
Note trim_right(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. Arguments: `std::string_view chars`.

#### See also

- `Note trim_right(self) const`

#### Examples

```cpp
auto out = note.trim_right("sample_text");
```

### `unique`

#### Signature

```cpp
Note unique(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### See also

- `Note unique(self, std::int64_t dim) const`

#### Examples

```cpp
auto out = note.unique();
```

### `unique`

#### Signature

```cpp
Note unique(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t dim`.

#### See also

- `Note unique(self) const`

#### Examples

```cpp
auto out = note.unique(1);
```

### `unquote`

#### Signature

```cpp
Note unquote(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.unquote();
```

### `unsqueeze`

#### Signature

```cpp
Note unsqueeze(self, std::int64_t dim) const
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t dim`.

#### Examples

```cpp
auto out = note.unsqueeze(1);
```

### `upper`

#### Signature

```cpp
Note upper(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies a text transform or text query to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.upper();
```

### `url_decode`

#### Signature

```cpp
Note url_decode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `url_decode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.url_decode();
```

### `url_encode`

#### Signature

```cpp
Note url_encode(self) const
```

#### Parameters

`None`

#### Returns

`Note`
    Return value produced by this method call.

#### Notes

Applies `url_encode` to `self`. No additional arguments.

#### Examples

```cpp
auto out = note.url_encode();
```

## Demos to run

- `PlussQit/examples/CodeN/note_factory_demo.cpp`
- `PlussQit/examples/CodeN/note_base_methods_demo.cpp`
- `PlussQit/examples/CodeN/note_cleanup_demo.cpp`
- `PlussQit/examples/CodeN/note_method_families_demo.cpp`
- `PlussQit/examples/CodeN/note_printing_demo.cpp`
