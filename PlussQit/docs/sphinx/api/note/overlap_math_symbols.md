<!-- pluss-hand-authored -->
# Note::overlap_math_symbols

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::overlap_math_symbols(const Note& other) const
```

## Math-Symbol Containment Score Between Two Notes

`overlap_math_symbols(other)` scores each cell pair by the overlap
coefficient of their **math-symbol** sets and returns a shape-preserving
`Note` of score strings. It is the math-symbol containment member of the
set-similarity family; see also
[`jaccard_math_symbols`](jaccard_math_symbols.md) and
[`intersect_math_symbols`](intersect_math_symbols.md).

### How the score is computed

A math symbol is a **TeX-style command** (`\` plus ASCII letters, e.g.
`\alpha`, `\sum`) or a single **operator character** from `+ - * / = < > ^`.
Commands keep their backslash and are compared case-sensitively. Symbols are
deduplicated into sets `A` and `B`; the score is `|A ∩ B| / min(|A|, |B|)`,
in `[0, 1]`, symmetric — `1.0` when the smaller symbol set is contained in
the larger.

A cell with none of `\ + - * / = < > ^` has an empty set. Both sets empty →
`"1.0"`; one empty and one not → `"0.0"`. Scores use up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::overlap_math_symbols requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::overlap_math_symbols expects equal
numel or scalar broadcast`.

## Example 1: Smaller Expression Contained In The Larger

```cpp
std::cout << pluss::make_note({"\\sum + \\int"})
                 .overlap_math_symbols(pluss::make_note({"\\sum + \\int - \\lim = 0"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["1.0"]
```

`{\sum, +, \int}` is a subset of `{\sum, +, \int, -, \lim, =}` → `1.0`.

## See Also

- [`Note::jaccard_math_symbols`](jaccard_math_symbols.md): the
  mutual-similarity score over the same sets.
- [`Note::intersect_math_symbols`](intersect_math_symbols.md): the shared
  symbols themselves.
- [`Note::overlap_tokens`](overlap_tokens.md) /
  [`Note::overlap_terms`](overlap_terms.md) /
  [`Note::overlap_acronyms`](overlap_acronyms.md) /
  [`Note::overlap_citations`](overlap_citations.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="overlap_citations.html">
    <span class="label">Previous</span>
    <span class="title">overlap_citations</span>
  </a>
  <a class="next-link" href="overlap_terms.html">
    <span class="label">Next</span>
    <span class="title">overlap_terms</span>
  </a>
</div>
