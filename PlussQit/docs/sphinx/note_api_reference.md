<!-- pluss-hand-authored -->

# Note API Reference

[Back to PlussQit API Reference](../api_reference.md)

This is the per-method reference for `pluss::Note` — one page per method, in
the same handwritten, compiled-and-verified style as the rest of the PlussQit
object references. For *building* a `Note`, see the separate
[Note Factory API Reference](note_factory_api_reference.md); everything on
this page assumes a `Note` already exists.

## What `Note` Is

`Note` is PlussQit's low-level, homogeneously typed value family for text
work. A defined `Note` has one dtype — `Text`, `Bytes`, `Bool`, `Int64`, or
`Float64` — and a shape of rank zero through three. Text methods consume text
or byte cells; predicates, measurements, comparisons, and scores remain Notes
by changing the result dtype instead of abandoning shape and ownership for a
host container.

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
  [`is_blank`](api/note/is_blank.md), and the
  [`regex()` accessor](note_regex_accessor_api_reference.md) match/search
  predicates return a shape-preserving Bool
  `Note`. Compose masks with `&`, `|`, `^`, and `~`; cross the host boundary
  explicitly with `bools()` only when native contiguous lanes are required.
- **Measurements** — [`lengths_bytes`](api/note/lengths_bytes.md),
  [`lengths_chars`](api/note/lengths_chars.md),
  [`lengths_graphemes`](api/note/lengths_graphemes.md), and
  [`count_substring`](api/note/count_substring.md) return a shape-preserving
  Int64 `Note`. `integers()` exposes its contiguous lanes without copying.
- **Variable-length structural results** — [`split`](api/note/split.md),
  [`split_lines`](api/note/split_lines.md),
  [`split_whitespace`](api/note/split_whitespace.md), the regex accessor's
  [`findall`](api/note-regex/findall.md), and the line/character
  difference methods return a **`TokenBatch`**:
  one flat Text/Bytes `Note` ([`batch.tokens()`](api/tokenbatch/tokens.md)) plus
  an Int64 `Note` ([`batch.offsets()`](api/tokenbatch/offsets.md), shape
  `[numel + 1]`) that
  marks where each input cell's run of tokens begins and ends in the flat
  stream. Row `r`'s tokens are `tokens.at(k)` for `k` in `[offsets[r], offsets[r + 1])`.
  These are methods of the result object, not root `Note` methods; the complete
  result contract lives in the [TokenBatch API Reference](tokenbatch_api_reference.md).
- **Linguistic operations** — [`nlp()`](api/note/nlp.md) opens the dedicated
  [Note NLP Accessor API Reference](note_nlp_accessor_api_reference.md) for
  tokenization, cleanup, extraction, scoring, phonetics, alignment, and
  language-oriented set comparison.
- **Regular expressions** — [`regex()`](api/note/regex.md) opens the dedicated
  [Note Regex Accessor API Reference](note_regex_accessor_api_reference.md) for
  whole-cell matching, substring search, counting, extraction, rewriting, and
  splitting.
- **TeX/LaTeX source** — [`tex()`](api/note/tex.md) opens the dedicated
  [Note TeX Accessor API Reference](note_tex_accessor_api_reference.md) for
  TeX-aware cleanup, math-delimiter handling, construct removal, environment
  extraction, macro expansion, math simplification and standardization,
  format conversion, and structural/semantic extraction.
- **Shape & layout** — [`reshape`](api/note/reshape.md),
  [`flatten`](api/note/flatten.md), [`squeeze`](api/note/squeeze.md),
  [`unsqueeze`](api/note/unsqueeze.md), [`transpose`](api/note/transpose.md),
  [`permute`](api/note/permute.md), and [`contiguous`](api/note/contiguous.md)
  change rank or axis order without touching cell content;
  [`repeat`](api/note/repeat.md) copies complete patterns across dimensions,
  while [`repeat_interleave`](api/note/repeat_interleave.md) makes adjacent
  copies along one selected axis.
- **Cell access & ownership** — [`at`](api/note/at.md) and
  [`at_indices`](api/note/at_indices.md) borrow one cell as a
  `std::string_view`; [`cell`](api/note/cell.md),
  [`cell_at`](api/note/cell_at.md), and [`owned`](api/note/owned.md) return a
  `TextRef` that keeps the packed arena alive; [`host`](api/note/host.md)
  materializes rank-aware owning standard-library strings.
- **Structural projections** — [`scalar`](api/note/scalar.md) and
  [`vector`](api/note/vector.md) are explicit borrowed text boundaries;
  [`matrix`](api/note/matrix.md) and [`slabs`](api/note/slabs.md) return
  storage-sharing row or slab Notes and therefore preserve dtype and ownership;
  [`sizes_view`](api/note/sizes_view.md) borrows extents without allocating.

## Running The Examples

Every example is a runnable fragment (assume `#include <pluss/pluss.h>` and
`#include <iostream>`), and every `### Output` block is copied verbatim from a
permanent compiled reference executable. Those verifiers cover cell access,
structural projections, collection ordering, operators, Boolean reductions,
structural repetition, and targeted masking; their saved transcripts are
compared byte for byte with fresh executable output during documentation work.

To print a `Note`, any of these render byte-identical text, and the pages
rotate through them:

```cpp
std::cout << pluss::note::print(n) << "\n";  // print(n) -> std::string
std::cout << n << "\n";                       // operator<< (default options)
the stream pluss::note::print(std::cout, n) << "\n";     // print(os, n)
```

Typed Note results can be printed without first extracting host storage:

```cpp
// Bool and Int64 results preserve n's shape.
const auto mask = n.contains("x");
const auto lengths = n.lengths_bytes();
std::cout << mask << '\n' << lengths << '\n';

// A TokenBatch contains two Notes.
const auto batch = n.split(",");
std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << pluss::note::print(batch.offsets()) << "\n";
```

Examples that show a rejection call the method inside a `try` block and print
the first line of `pluss::Error::what()` (the message); `what()` also appends
a `Triggered at:` location line, which the output blocks omit.

```{toctree}
:hidden:
:maxdepth: 1

api/note/all
api/note/any
api/note/argsort
api/note/at
api/note/at_indices
api/note/base64_decode
api/note/base64_encode
api/note/begin
api/note/bool_item
api/note/bools
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
api/note/copy_flat_to
api/note/count_substring
api/note/debug
api/note/dedent
api/note/deduplicate
api/note/defined
api/note/diff_chars
api/note/diff_lines
api/note/dim
api/note/dtype
api/note/empty
api/note/end
api/note/ends_with
api/note/ensure_prefix
api/note/ensure_suffix
api/note/flatten
api/note/float_item
api/note/for_each_flat
api/note/from_latin1
api/note/from_windows1252
api/note/hex_decode
api/note/hex_encode
api/note/host
api/note/html_escape
api/note/html_unescape
api/note/indent
api/note/int_item
api/note/integers
api/note/is_alnum
api/note/is_alpha
api/note/is_ascii
api/note/is_blank
api/note/is_bool
api/note/is_bytes
api/note/is_digit
api/note/is_float64
api/note/is_int64
api/note/is_lower
api/note/is_matrix
api/note/is_rank3
api/note/is_scalar
api/note/is_text
api/note/is_title
api/note/is_upper
api/note/is_utf8_valid
api/note/is_vector
api/note/is_whitespace
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
api/note/nlp
note_nlp_accessor_api_reference
api/note/normalize_dashes
api/note/normalize_ellipsis
api/note/normalize_quotes
api/note/numel
api/note/operators
api/note/owned
api/note/pack
api/note/permute
api/note/quote
api/note/reals
api/note/regex
note_regex_accessor_api_reference
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
api/note/repeat
api/note/repeat_interleave
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
api/note/slabs
api/note/sort
api/note/split
api/note/split_chars
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
api/note/tex
note_tex_accessor_api_reference
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
api/note/transpose
api/note/undefined
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
  `empty`, `is_scalar`, `is_vector`, `is_matrix`, and `is_rank3`.

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
  `remove`, `remove_all`, `remove_prefix`, `remove_suffix`,
  `ensure_prefix`, `ensure_suffix`, `surround`, `quote`, `unquote`.
- **Inspection** — `contains`, `contains_any_alpha`, `contains_any_digit`,
  `starts_with`, `ends_with`, `count_substring`, `is_alpha`, `is_alnum`,
  `is_digit`, `is_lower`, `is_upper`,
  `is_title`, `is_whitespace`, `is_blank`, `is_ascii`, `is_utf8_valid`,
  `is_bytes`, `lengths_bytes`, `lengths_chars`, `lengths_graphemes`.
- **Value algebra** — `+` concatenates Text or Bytes cells, `*` repeats cell
  contents, comparisons return Bool Notes for every dtype, and `&`, `|`, `^`,
  and `~` compose Bool Notes. `any` and `all` reduce a complete Bool Note to a
  rank-zero Bool Note.
- **Structural tokenization** — `split`, `split_lines`, and `split_whitespace`.
  Pattern extraction lives on the [`regex()` accessor](note_regex_accessor_api_reference.md),
  while linguistic tokenization and sentence analysis live on
  the [`nlp()` accessor](note_nlp_accessor_api_reference.md).
- **TeX/LaTeX source** — cleanup, extraction, conversion, and rendering live
  on the [`tex()` accessor](note_tex_accessor_api_reference.md).
- **Transport encodings** — `base64_encode`/`base64_decode`,
  `hex_encode`/`hex_decode`, `url_encode`/`url_decode`.
- **Escaping** — `html_escape`/`html_unescape`, `json_escape`; TeX escaping
  lives on the [`tex()` accessor](note_tex_accessor_api_reference.md)
  (`tex().escape()`/`tex().unescape()`).
- **Encoding & transcoding** — `to_utf8_strict`, `to_utf8_lossy`,
  `from_latin1`/`to_latin1_lossy`,
  `from_windows1252`/`to_windows1252_lossy` — convert between a bytes-mode
  payload and UTF-8 text.
- **Linguistic analysis** — cleanup, masking, extraction, transliteration,
  phonetics, scoring, and token comparison live on the
  [`nlp()` accessor](note_nlp_accessor_api_reference.md).
- **Shape & layout** — `reshape`, `flatten`, `squeeze`,
  `unsqueeze`, `transpose`, `permute`, `contiguous`.
  These are metadata-only: the cell strings are untouched, only the rank and
  axis order change.
- **Selection & slicing** — `select` (one slab along an axis, that axis
  dropped), `narrow` (a contiguous range along an axis, rank kept), `take`
  (gather cells by an index list, rank-1 result).
- **Collection reduction & ordering** — `join` reduces all text cells to a
  scalar Note, `join_flat` extracts one owning standard string, `sort` orders
  lanes, `argsort` returns the Int64 positions of that order, `unique` returns
  sorted distinct values or slices, and `deduplicate` keeps their first-seen
  order.

The root remains focused on storage, shape, and general text mechanics. The
NLP accessor has its own A-Z method reference and navigation chain.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/note/all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
