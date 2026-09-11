<!-- pluss-hand-authored -->
# Note::jaccard_citations

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::jaccard_citations(const Note& other) const
```

## Citation-Set Similarity Score Between Two Notes

`jaccard_citations(other)` scores each cell pair by the Jaccard index of
their **citation-marker** sets and returns a shape-preserving `Note` of score
strings. It is the citation-set scoring member of the set-similarity family;
see also [`overlap_citations`](overlap_citations.md) and
[`intersect_citations`](intersect_citations.md).

### How the score is computed

A citation marker is a **bracketed number** (`[1]`, `[42]`, one to three
digits) or an **author–year** group (`(Smith, 2020)`, `(Lee, 2021a)`).
Markers keep their brackets, are lowercased, and are deduplicated into sets
`A` and `B`. The score is `|A ∩ B| / |A ∪ B|`, in `[0, 1]`, symmetric.

A cell with no bracket-or-paren and digit has an empty set. Both sets empty →
`"1.0"`; one empty and one not → `"0.0"`. Scores use up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::jaccard_citations requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::jaccard_citations expects equal numel
or scalar broadcast`.

## Example 1: Overlapping Reference Lists

```cpp
std::cout << pluss::make_note({"[1] [2] [3]"})
                 .jaccard_citations(pluss::make_note({"[2] [3] [4]"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.5"]
```

Sets `{[1], [2], [3]}` and `{[2], [3], [4]}` share two of four distinct
markers → `0.5`.

## See Also

- [`Note::overlap_citations`](overlap_citations.md): the containment score
  over the same sets.
- [`Note::intersect_citations`](intersect_citations.md): the shared markers
  themselves.
- [`Note::jaccard_tokens`](jaccard_tokens.md) /
  [`Note::jaccard_terms`](jaccard_terms.md) /
  [`Note::jaccard_acronyms`](jaccard_acronyms.md) /
  [`Note::jaccard_math_symbols`](jaccard_math_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jaccard_acronyms.html">
    <span class="label">Previous</span>
    <span class="title">jaccard_acronyms</span>
  </a>
  <a class="next-link" href="jaccard_math_symbols.html">
    <span class="label">Next</span>
    <span class="title">jaccard_math_symbols</span>
  </a>
</div>
