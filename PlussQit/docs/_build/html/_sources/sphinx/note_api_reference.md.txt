<!-- pluss-hand-authored -->

# Note API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::Note` and `pluss::coden::Note` in the same
handwritten, usage-first style as the rest of the PlussQit object references:
one method page at a time, with compiled-and-run examples under each method
page rather than generated stubs.

The examples are intentionally written as standalone snippets: each example
defines its own input object, stores intermediate results in named variables,
and explicitly prints the result it is discussing. That makes the pages more
reliable as copy-paste starting points and as templates for future `Note`
documentation passes.

## What `Note` Is

`Note` is the text payload object in PlussQit.

Use it when your data are strings, tokens, lines, snippets, prompts, LaTeX, or
document fragments and you want to transform them in a shape-preserving,
vectorized way.

That is the main difference from the other foundational objects:

- `Series` carries numeric and boolean values.
- `Note` carries UTF-8 text or raw byte-oriented textual content.
- `Timeline` carries temporal values.
- `TimeIndex` is the ordered 1D time-axis surface built for lookup, slicing,
  and alignment.

## Includes

```cpp
#include <pluss/pluss.h>
// or the narrower surface
#include <pluss/coden/coden.h>
#include <pluss/coden/note_factory.h>
```

## Practical Construction Patterns

Example 1. Build a scalar text value.

```cpp
auto title = pluss::make_note("Final Exam Review");
```

Example 2. Build a 1D batch of text cases.

```cpp
auto names = pluss::make_note({"Ada Lovelace", "Grace Hopper", "Linus"});
```

Example 3. Build from flat storage plus explicit shape when the text already
arrives from another system in flattened form.

```cpp
std::vector<std::string> flat{"r0c0", "r0c1", "r1c0", "r1c1"};
std::array<std::int64_t, 2> sizes{2, 2};

auto grid = pluss::make_note(flat, sizes);
```

## How to Read the Examples

Most `Note` methods preserve shape and operate cell by cell.

That means when you see a 1D `Note` with three cases, the transformed result is
usually still a 1D `Note` with three cases. Tokenizing methods are the main
exception: they return a `TokenBatch`, which flattens tokens into one `Note`
plus an `offsets()` vector describing where each original case begins and ends
inside that flattened token stream.

## Using This Reference

- Use the left sidebar to open one `Note` method page at a time.
- This first handwritten slice focuses on the methods users reach for early:
  cleanup, substring matching, replacement, tokenization, regex extraction,
  NLP-adjacent cleanup, and TeX escaping.
- Every method page below embeds its output directly under the example that
  produced it.
- Those outputs were verified against the current library state from the
  `examples/CodeN/note_docverify_demo.cpp` run, but the page itself stays
  self-contained instead of delegating readers into one large verifier dump.

```{toctree}
:hidden:
:maxdepth: 1

api/note/contains
api/note/lengths_bytes
api/note/lower
api/note/nlp_remove_urls
api/note/regex_findall
api/note/replace
api/note/split_lines
api/note/split_whitespace
api/note/strip
api/note/tex_escape
api/note/tex_to_text
api/note/remove_all
api/note/quote
api/note/split
api/note/tokenize_words
api/note/nlp_mask_pii
api/note/base64_encode
api/note/url_encode
```

## Coverage

This first `Note` pass now covers:

- substring predicates: `contains`
- byte-length inspection: `lengths_bytes`
- case normalization: `lower`
- URL cleanup for text preprocessing: `nlp_remove_urls`
- regex token extraction: `regex_findall`
- elementwise substring replacement: `replace`
- literal substring removal: `remove_all`
- boundary quoting: `quote`
- delimiter-based and whitespace-based tokenization: `split`, `split_lines`,
  `split_whitespace`, `tokenize_words`
- line and whitespace tokenization: `split_lines`, `split_whitespace`
- boundary trimming: `strip`
- targeted text masking: `nlp_mask_pii`, `nlp_remove_urls`
- TeX-safe escaping and TeX-to-text reduction: `tex_escape`, `tex_to_text`
- transport encodings: `base64_encode`, `url_encode`

The rest of the `Note` surface is much larger than this first slice. It
includes broader cleanup families, NLP helpers, TeX parsers, token comparison
helpers, and encoding transforms. Those will be expanded method by method in
later passes rather than bulk-filled with shallow prose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/note/contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
