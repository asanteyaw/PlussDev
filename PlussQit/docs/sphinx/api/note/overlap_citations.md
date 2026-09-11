<!-- pluss-hand-authored -->
# Note::overlap_citations

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::overlap_citations(const Note& other) const
```

## Citation-Set Containment Score Between Two Notes

`overlap_citations(other)` scores each cell pair by the overlap coefficient
of their **citation-marker** sets and returns a shape-preserving `Note` of
score strings. It is the citation-set containment member of the
set-similarity family; see also [`jaccard_citations`](jaccard_citations.md)
and [`intersect_citations`](intersect_citations.md).

### How the score is computed

A citation marker is a **bracketed number** (`[1]`, `[42]`, one to three
digits) or an **author–year** group (`(Smith, 2020)`, `(Lee, 2021a)`).
Markers keep their brackets, are lowercased, and are deduplicated into sets
`A` and `B`. The score is `|A ∩ B| / min(|A|, |B|)`, in `[0, 1]`, symmetric —
`1.0` when the smaller reference list is contained in the larger.

A cell with no bracket-or-paren and digit has an empty set. Both sets empty →
`"1.0"`; one empty and one not → `"0.0"`. Scores use up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::overlap_citations requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::overlap_citations expects equal numel
or scalar broadcast`.

## Example 1: One Reference List Inside Another

```cpp
std::cout << pluss::make_note({"[1] [2]"})
                 .overlap_citations(pluss::make_note({"[1] [2] [3] [4]"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["1.0"]
```

`{[1], [2]}` is a subset of `{[1], [2], [3], [4]}` → `1.0`.
[`jaccard_citations`](jaccard_citations.md) here would give `2/4 = 0.5`.

## See Also

- [`Note::jaccard_citations`](jaccard_citations.md): the mutual-similarity
  score over the same sets.
- [`Note::intersect_citations`](intersect_citations.md): the shared markers
  themselves.
- [`Note::overlap_tokens`](overlap_tokens.md) /
  [`Note::overlap_terms`](overlap_terms.md) /
  [`Note::overlap_acronyms`](overlap_acronyms.md) /
  [`Note::overlap_math_symbols`](overlap_math_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="overlap_acronyms.html">
    <span class="label">Previous</span>
    <span class="title">overlap_acronyms</span>
  </a>
  <a class="next-link" href="overlap_math_symbols.html">
    <span class="label">Next</span>
    <span class="title">overlap_math_symbols</span>
  </a>
</div>
