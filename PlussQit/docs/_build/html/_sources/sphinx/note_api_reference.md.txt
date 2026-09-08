<!-- pluss-hand-authored -->

# Note API Reference

[Back to PlussQit API Reference](../api_reference.md)

This is the per-method reference for `pluss::Note` — one page per method, in
the same handwritten, compiled-and-verified style as the rest of the PlussQit
object references. For *building* a `Note`, see the separate
[Note Factory API Reference](note_factory_api_reference.md); everything on
this page assumes a `Note` already exists.

## What `Note` Is

`Note` is the text object in the plussmic data-analysis stack. A `Note` is a
shape (up to 3-D) of text cells — UTF-8 by default, or raw bytes — and its
methods transform those cells in a **vectorized, shape-preserving** way, the
same way `Series` transforms numbers and `Timeline` transforms instants.

Reach for `Note` when the data are strings, tokens, lines, snippets, prompts,
document fragments, or LaTeX, and the work is cleanup, matching, rewriting,
splitting, tokenization, regex extraction, or text-oriented natural-language
preprocessing.

## How The Methods Are Shaped

Most `Note` methods **preserve shape** and act cell by cell: a rank-`1`
`Note` of three cells maps to a rank-`1` `Note` of three cells. Three
families break that mould:

- **Predicates** — [`contains`](api/note/contains.md),
  [`starts_with`](api/note/starts_with.md), and
  [`ends_with`](api/note/ends_with.md) return a flat
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
  [`regex_findall`](api/note/regex_findall.md) return a **`TokenBatch`**:
  one flat token `Note` (`batch.tokens()`) plus an
  `offsets()` vector (`std::vector<std::int64_t>`, length `numel + 1`) that
  marks where each input cell's run of tokens begins and ends in the flat
  stream. Row `r`'s tokens are `tokens()[offsets()[r] .. offsets()[r+1])`.

## Running The Examples

Every example is a runnable fragment (assume `#include <pluss/pluss.h>` and
`#include <iostream>`), and every `### Output` block is copied verbatim from
the compiled program `examples/CodeN/note_method_reference_demo.cpp`.

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

api/note/base64_decode
api/note/base64_encode
api/note/capitalize
api/note/casefold
api/note/contains
api/note/count_substring
api/note/ends_with
api/note/hex_decode
api/note/hex_encode
api/note/lengths_bytes
api/note/lengths_chars
api/note/lengths_graphemes
api/note/lower
api/note/nlp_mask_pii
api/note/nlp_remove_urls
api/note/quote
api/note/regex_findall
api/note/remove_all
api/note/replace
api/note/split
api/note/split_lines
api/note/split_whitespace
api/note/starts_with
api/note/strip
api/note/swapcase
api/note/tex_escape
api/note/tex_to_text
api/note/title
api/note/tokenize_words
api/note/unquote
api/note/upper
api/note/url_decode
api/note/url_encode
```

## Coverage

This reference documents the methods users reach for first:

- **Case** — `lower`, `upper`, `casefold`, `title`, `capitalize`,
  `swapcase`.
- **Boundary and rewriting** — `strip`, `replace`, `remove_all`, `quote`,
  `unquote`.
- **Inspection** — `contains`, `starts_with`, `ends_with`,
  `count_substring`, `lengths_bytes`, `lengths_chars`, `lengths_graphemes`.
- **Tokenizers** — `split`, `split_lines`, `split_whitespace`,
  `tokenize_words`, `regex_findall`.
- **Targeted masking** — `nlp_mask_pii`, `nlp_remove_urls`.
- **TeX** — `tex_escape`, `tex_to_text`.
- **Transport encodings** — `base64_encode`/`base64_decode`,
  `hex_encode`/`hex_decode`, `url_encode`/`url_decode`.

The rest of the `Note` surface — broader cleanup, NLP extraction, diff/align,
Unicode normalization, transliteration, and phonetics — is documented method
by method in later passes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/note/base64_decode.html">
    <span class="label">Next</span>
    <span class="title">base64_decode</span>
  </a>
</div>
