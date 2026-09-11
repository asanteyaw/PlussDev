<!-- pluss-hand-authored -->

# Note API Reference

[Back to PlussQit API Reference](../api_reference.md)

This is the per-method reference for `pluss::Note` — one page per method, in
the same handwritten, compiled-and-verified style as the rest of the PlussQit
object references. For *building* a `Note`, see the separate
[Note Factory API Reference](note_factory_api_reference.md); everything on
this page assumes a `Note` already exists.

## What `Note` Is

`Note` is PlussQit's low-level text value. A `Note` is a
shape (up to 3-D) of text cells — UTF-8 by default, or raw bytes — and its
methods transform those cells in a **vectorized, shape-preserving** way, the
same way `Series` transforms numbers and `Timeline` transforms instants.

Reach for `Note` when the data are strings, tokens, lines, snippets, prompts,
document fragments, or LaTeX, and the work is cleanup, matching, rewriting,
splitting, tokenization, regex extraction, or text-oriented natural-language
preprocessing.

## How The Methods Are Shaped

Most `Note` methods **preserve shape** and act cell by cell: a rank-`1`
`Note` of three cells maps to a rank-`1` `Note` of three cells. A few
families break that mould:

- **Predicates** — [`contains`](api/note/contains.md),
  [`starts_with`](api/note/starts_with.md),
  [`ends_with`](api/note/ends_with.md), the `is_*` character-property checks
  ([`is_alpha`](api/note/is_alpha.md), [`is_digit`](api/note/is_digit.md),
  [`is_blank`](api/note/is_blank.md), …), and
  [`regex_match`](api/note/regex_match.md) /
  [`regex_search`](api/note/regex_search.md) return a flat
  `std::vector<std::uint8_t>` of `0`/`1`, one entry per cell in row-major
  order.
- **Measurements** — [`lengths_bytes`](api/note/lengths_bytes.md),
  [`lengths_chars`](api/note/lengths_chars.md),
  [`lengths_graphemes`](api/note/lengths_graphemes.md), and
  [`count_substring`](api/note/count_substring.md) return a flat
  `std::vector<std::int64_t>`, one entry per cell.
- **Tokenizers** — [`split`](api/note/split.md),
  [`split_lines`](api/note/split_lines.md),
  [`split_whitespace`](api/note/split_whitespace.md),
  [`tokenize_words`](api/note/tokenize_words.md), and
  [`regex_findall`](api/note/regex_findall.md), and the diff / align family
  ([`diff_words`](api/note/diff_words.md),
  [`diff_lines`](api/note/diff_lines.md),
  [`diff_chars`](api/note/diff_chars.md),
  [`align_tokens`](api/note/align_tokens.md),
  [`align_sentences`](api/note/align_sentences.md)) return a **`TokenBatch`**:
  one flat token `Note` (`batch.tokens()`) plus an
  `offsets()` vector (`std::vector<std::int64_t>`, length `numel + 1`) that
  marks where each input cell's run of tokens begins and ends in the flat
  stream. Row `r`'s tokens are `tokens()[offsets()[r] .. offsets()[r+1])`.
- **Set-similarity comparisons** — the `intersect_*`, `jaccard_*`, and
  `overlap_*` methods each take a second `Note` and compare cell against
  matching cell (equal `numel`, or a scalar broadcast). `intersect_*`
  ([`intersect_tokens`](api/note/intersect_tokens.md), …) return a
  `TokenBatch` of the shared tokens; `jaccard_*`
  ([`jaccard_tokens`](api/note/jaccard_tokens.md), …) and `overlap_*`
  ([`overlap_tokens`](api/note/overlap_tokens.md), …) return a
  shape-preserving `Note` whose cells are similarity scores formatted as
  strings in `[0, 1]`.
- **Shape & layout** — [`reshape`](api/note/reshape.md),
  [`flatten`](api/note/flatten.md), [`squeeze`](api/note/squeeze.md),
  [`unsqueeze`](api/note/unsqueeze.md), [`transpose`](api/note/transpose.md),
  [`permute`](api/note/permute.md), and [`contiguous`](api/note/contiguous.md)
  change the rank or axis order of a `Note` without touching a single cell
  string.
- **Cell access & ownership** — [`at`](api/note/at.md) and
  [`at_indices`](api/note/at_indices.md) borrow one cell as a
  `std::string_view`; [`cell`](api/note/cell.md),
  [`cell_at`](api/note/cell_at.md), and [`owned`](api/note/owned.md) return a
  `TextRef` that keeps the packed arena alive; [`host`](api/note/host.md)
  materializes rank-aware owning standard-library strings.
- **Borrowed structural projections** — [`scalar`](api/note/scalar.md)
  borrows the sole cell regardless of rank; [`vector`](api/note/vector.md)
  flattens every rank; [`matrix`](api/note/matrix.md) preserves rank two and
  promotes lower ranks; [`tensor3`](api/note/tensor3.md) requires exact rank
  three; [`sizes_view`](api/note/sizes_view.md) borrows the extents without
  allocating.

## Running The Examples

Every example is a runnable fragment (assume `#include <pluss/pluss.h>` and
`#include <iostream>`), and every `### Output` block is copied verbatim from a
permanent compiled reference executable. The cell-access family is verified by
`examples/CodeN/note_access_reference_demo.cpp`, with its exact transcript in
`docs/sphinx/_verified/note_access_reference_output.txt`; broader transformation
examples remain in `examples/CodeN/note_method_reference_demo.cpp`. Structural
projection examples are verified by
`examples/CodeN/note_projection_reference_demo.cpp`, with its exact transcript
in `docs/sphinx/_verified/note_projection_reference_output.txt`. Collection
reduction and ordering examples are verified by
`examples/CodeN/note_collection_reference_demo.cpp`, with its exact transcript
in `docs/sphinx/_verified/note_collection_reference_output.txt`.

To print a `Note`, any of these render byte-identical text, and the pages
rotate through them:

```cpp
std::cout << pluss::note::print(n) << "\n";  // print(n) -> std::string
std::cout << n << "\n";                       // operator<< (default options)
pluss::note::print(std::cout, n) << "\n";     // print(os, n) -> the stream
```

The non-`Note` results are printed plainly:

```cpp
// contains / lengths_bytes: a flat vector, one entry per cell
const auto mask = n.contains("x");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";

// a TokenBatch: the flat token Note, then the offsets
const auto batch = n.split(",");
std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

Examples that show a rejection call the method inside a `try` block and print
the first line of `pluss::Error::what()` (the message); `what()` also appends
a `Triggered at:` location line, which the output blocks omit.

```{toctree}
:hidden:
:maxdepth: 1

api/note/align_sentences
api/note/align_tokens
api/note/at
api/note/at_indices
api/note/base64_decode
api/note/base64_encode
api/note/capitalize
api/note/casefold
api/note/cell
api/note/cell_at
api/note/collapse_newlines
api/note/collapse_punct
api/note/collapse_repeats
api/note/collapse_spaces
api/note/collapse_whitespace
api/note/contains
api/note/contains_any_alpha
api/note/contains_any_digit
api/note/contiguous
api/note/count_substring
api/note/dedent
api/note/deduplicate
api/note/defined
api/note/diff_chars
api/note/diff_lines
api/note/diff_words
api/note/dim
api/note/empty
api/note/ends_with
api/note/ensure_prefix
api/note/ensure_suffix
api/note/flatten
api/note/from_latin1
api/note/from_windows1252
api/note/hex_decode
api/note/hex_encode
api/note/host
api/note/html_escape
api/note/html_unescape
api/note/indent
api/note/intersect_acronyms
api/note/intersect_citations
api/note/intersect_math_symbols
api/note/intersect_terms
api/note/intersect_tokens
api/note/is_alnum
api/note/is_alpha
api/note/is_ascii
api/note/is_blank
api/note/is_bytes
api/note/is_digit
api/note/is_lower
api/note/is_matrix
api/note/is_scalar
api/note/is_tensor3
api/note/is_title
api/note/is_upper
api/note/is_utf8_valid
api/note/is_vector
api/note/is_whitespace
api/note/jaccard_acronyms
api/note/jaccard_citations
api/note/jaccard_math_symbols
api/note/jaccard_terms
api/note/jaccard_tokens
api/note/join
api/note/join_flat
api/note/json_escape
api/note/keep_alnum
api/note/keep_alpha
api/note/keep_ascii
api/note/keep_digits
api/note/keep_printable
api/note/lengths_bytes
api/note/lengths_chars
api/note/lengths_graphemes
api/note/lower
api/note/lstrip
api/note/matrix
api/note/narrow
api/note/nlp_mask_pii
api/note/nlp_remove_urls
api/note/normalize_dashes
api/note/normalize_ellipsis
api/note/normalize_quotes
api/note/numel
api/note/overlap_acronyms
api/note/overlap_citations
api/note/overlap_math_symbols
api/note/overlap_terms
api/note/overlap_tokens
api/note/owned
api/note/permute
api/note/phonetic_metaphone
api/note/phonetic_soundex
api/note/quote
api/note/regex_findall
api/note/regex_match
api/note/regex_replace
api/note/regex_search
api/note/remove
api/note/remove_all
api/note/remove_control_chars
api/note/remove_digits
api/note/remove_emojis
api/note/remove_letters
api/note/remove_prefix
api/note/remove_punctuation
api/note/remove_suffix
api/note/remove_symbols
api/note/replace
api/note/replace_first
api/note/replace_last
api/note/replace_n
api/note/reshape
api/note/rstrip
api/note/scalar
api/note/select
api/note/size
api/note/sizes
api/note/sizes_view
api/note/sort
api/note/split
api/note/split_lines
api/note/split_whitespace
api/note/squeeze
api/note/starts_with
api/note/strip
api/note/strip_accents
api/note/strip_bom
api/note/strip_zero_width
api/note/surround
api/note/swapcase
api/note/take
api/note/tensor3
api/note/tex_escape
api/note/tex_to_text
api/note/title
api/note/to_ascii
api/note/to_latin1_lossy
api/note/to_nfc
api/note/to_nfd
api/note/to_nfkc
api/note/to_nfkd
api/note/to_utf8_lossy
api/note/to_utf8_strict
api/note/to_windows1252_lossy
api/note/tokenize_words
api/note/transliterate_ascii
api/note/transliterate_cyrillic
api/note/transliterate_greek
api/note/transpose
api/note/unique
api/note/unquote
api/note/unsqueeze
api/note/upper
api/note/url_decode
api/note/url_encode
api/note/vector
```

## Coverage

This reference documents the methods users reach for first:

- **Metadata and rank** — `defined`, `dim`, `size`, `sizes`, `numel`,
  `empty`, `is_scalar`, `is_vector`, `is_matrix`, and `is_tensor3`.

- **Case** — `lower`, `upper`, `casefold`, `title`, `capitalize`,
  `swapcase`.
- **Boundary trimming** — `strip`, `lstrip`, `rstrip`.
- **Whitespace & layout** — `collapse_whitespace`, `collapse_spaces`,
  `collapse_newlines`, `collapse_repeats`, `collapse_punct`, `dedent`,
  `indent`, `remove_control_chars`, `to_ascii`.
- **Unicode normalization** — `to_nfc`, `to_nfd`, `to_nfkc`, `to_nfkd`,
  `strip_accents`, `strip_bom`, `strip_zero_width`, `normalize_quotes`,
  `normalize_dashes`, `normalize_ellipsis`.
- **Character-class filters** — `remove_punctuation`, `remove_symbols`,
  `remove_digits`, `remove_letters`, `remove_emojis`, `keep_alnum`,
  `keep_alpha`, `keep_digits`, `keep_ascii`, `keep_printable`.
- **Rewriting** — `replace`, `replace_first`, `replace_last`, `replace_n`,
  `regex_replace`, `remove`, `remove_all`, `remove_prefix`, `remove_suffix`,
  `ensure_prefix`, `ensure_suffix`, `surround`, `quote`, `unquote`.
- **Inspection** — `contains`, `contains_any_alpha`, `contains_any_digit`,
  `starts_with`, `ends_with`, `count_substring`, `regex_match`,
  `regex_search`, `is_alpha`, `is_alnum`, `is_digit`, `is_lower`, `is_upper`,
  `is_title`, `is_whitespace`, `is_blank`, `is_ascii`, `is_utf8_valid`,
  `is_bytes`, `lengths_bytes`, `lengths_chars`, `lengths_graphemes`.
- **Tokenizers** — `split`, `split_lines`, `split_whitespace`,
  `tokenize_words`, `regex_findall`.
- **Diff & align** — `diff_words`, `diff_lines`, `diff_chars`,
  `align_tokens`, `align_sentences`.
- **Set similarity** — `intersect_tokens`, `intersect_terms`,
  `intersect_acronyms`, `intersect_citations`, `intersect_math_symbols`;
  `jaccard_*` and `overlap_*` for the same five token-set kinds.
- **Targeted masking** — `nlp_mask_pii`, `nlp_remove_urls`.
- **TeX** — `tex_escape`, `tex_to_text`.
- **Transport encodings** — `base64_encode`/`base64_decode`,
  `hex_encode`/`hex_decode`, `url_encode`/`url_decode`.
- **Escaping** — `html_escape`/`html_unescape`, `json_escape`,
  `tex_escape`/`tex_to_text`.
- **Encoding & transcoding** — `to_utf8_strict`, `to_utf8_lossy`,
  `from_latin1`/`to_latin1_lossy`,
  `from_windows1252`/`to_windows1252_lossy` — convert between a bytes-mode
  payload and UTF-8 text.
- **Transliteration & phonetics** — `transliterate_ascii` (and its synonym
  `transliterate_latin`), `transliterate_greek`, `transliterate_cyrillic`;
  `phonetic_soundex`, `phonetic_metaphone`.
- **Shape & layout** — `reshape`, `flatten`, `squeeze`,
  `unsqueeze` (synonym `expand_dims`), `transpose`, `permute`, `contiguous`.
  These are metadata-only: the cell strings are untouched, only the rank and
  axis order change.
- **Selection & slicing** — `select` (one slab along an axis, that axis
  dropped), `narrow` (a contiguous range along an axis, rank kept), `take`
  (gather cells by an index list, rank-1 result).
- **Collection reduction & ordering** — `join` reduces all text cells to a
  scalar Note, `join_flat` extracts one owning standard string, `sort` orders
  lanes, `unique` returns sorted distinct values or slices, and `deduplicate`
  keeps their first-seen order.

The rest of the `Note` surface — broader cleanup, NLP extraction, and the
remaining structural operations (`repeat`, `repeat_interleave`) — is
documented method by method in later passes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/note/align_sentences.html">
    <span class="label">Next</span>
    <span class="title">align_sentences</span>
  </a>
</div>
