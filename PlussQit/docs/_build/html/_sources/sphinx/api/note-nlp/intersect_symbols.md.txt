<!-- pluss-hand-authored -->
# Note::NlpAccessor::intersect_symbols

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::NlpAccessor::intersect_symbols(const Note& other) const
```

## Shared Math Symbols Between Two Notes

`intersect_symbols(other)` reduces each cell to its set of **math
symbols** and returns the symbols present in both cells as a `TokenBatch`.
It is the math-symbol member of the set-similarity family; the scoring
members are [`jaccard_symbols`](jaccard_symbols.md) and
[`overlap_symbols`](overlap_symbols.md).

### How the set is built

A math symbol is either

- a **TeX-style command** — `\` followed by one or more ASCII letters
  (`\alpha`, `\sum`, `\int`), or
- a single **operator character** from `+ - * / = < > ^`.

Commands keep their leading backslash. A cell with none of the characters
`\ + - * / = < > ^` produces an empty set. The set is deduplicated and
compared case-sensitively (commands are not lowercased).

### What is emitted

Members in both sets, in the order they first appear in `self`, once each; a
cell with no shared symbol contributes an empty range. The rank-`1` Int64
Note `batch.offsets()` has length `numel + 1` and marks each cell's run.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::NlpAccessor::intersect_symbols requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::NlpAccessor::intersect_symbols expects equal
numel or scalar broadcast`.

## Example 1: Shared Operators And Commands

```cpp
const auto batch =
    pluss::make_note({"a + b = c \\alpha \\beta"})
        .nlp().intersect_symbols(pluss::make_note({"x = y - z \\beta"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["=", "\\beta"]
offsets: [0, 2]
```

Both cells contain `=` and `\beta`. `self` also has `+` and `\alpha`; `other`
also has `-`. Only the two shared symbols are emitted, in `self` order. (The
printed `"\\beta"` is a single backslash followed by `beta` — the `Note`
printer escapes it.)

## See Also

- [`Note::NlpAccessor::jaccard_symbols`](jaccard_symbols.md) /
  [`Note::NlpAccessor::overlap_symbols`](overlap_symbols.md): score the
  shared-symbol overlap.
- [`Note::NlpAccessor::intersect_tokens`](intersect_tokens.md) /
  [`Note::NlpAccessor::intersect_terms`](intersect_terms.md) /
  [`Note::NlpAccessor::intersect_acronyms`](intersect_acronyms.md) /
  [`Note::NlpAccessor::intersect_citations`](intersect_citations.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect_citations.html"><span class="label">Previous</span><span class="title">intersect_citations</span></a>
  <a class="next-link" href="intersect_terms.html"><span class="label">Next</span><span class="title">intersect_terms</span></a>
</div>
