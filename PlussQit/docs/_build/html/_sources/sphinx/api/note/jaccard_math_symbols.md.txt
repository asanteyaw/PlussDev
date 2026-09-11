<!-- pluss-hand-authored -->
# Note::jaccard_math_symbols

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::jaccard_math_symbols(const Note& other) const
```

## Math-Symbol Similarity Score Between Two Notes

`jaccard_math_symbols(other)` scores each cell pair by the Jaccard index of
their **math-symbol** sets and returns a shape-preserving `Note` of score
strings. It is the math-symbol scoring member of the set-similarity family;
see also [`overlap_math_symbols`](overlap_math_symbols.md) and
[`intersect_math_symbols`](intersect_math_symbols.md).

### How the score is computed

A math symbol is a **TeX-style command** (`\` plus ASCII letters, e.g.
`\alpha`, `\sum`) or a single **operator character** from `+ - * / = < > ^`.
Commands keep their backslash and are compared case-sensitively. Symbols are
deduplicated into sets `A` and `B`; the score is `|A ∩ B| / |A ∪ B|`, in
`[0, 1]`, symmetric.

A cell with none of `\ + - * / = < > ^` has an empty set. Both sets empty →
`"1.0"`; one empty and one not → `"0.0"`. Scores use up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::jaccard_math_symbols requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::jaccard_math_symbols expects equal
numel or scalar broadcast`.

## Example 1: One Shared Operator Out Of Three

```cpp
std::cout << pluss::make_note({"a + b = c"})
                 .jaccard_math_symbols(pluss::make_note({"a - b = c"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.333333"]
```

Sets `{+, =}` and `{-, =}` share only `=` — one of three distinct symbols →
`1/3`.

## See Also

- [`Note::overlap_math_symbols`](overlap_math_symbols.md): the containment
  score over the same sets.
- [`Note::intersect_math_symbols`](intersect_math_symbols.md): the shared
  symbols themselves.
- [`Note::jaccard_tokens`](jaccard_tokens.md) /
  [`Note::jaccard_terms`](jaccard_terms.md) /
  [`Note::jaccard_acronyms`](jaccard_acronyms.md) /
  [`Note::jaccard_citations`](jaccard_citations.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jaccard_citations.html">
    <span class="label">Previous</span>
    <span class="title">jaccard_citations</span>
  </a>
  <a class="next-link" href="jaccard_terms.html">
    <span class="label">Next</span>
    <span class="title">jaccard_terms</span>
  </a>
</div>
