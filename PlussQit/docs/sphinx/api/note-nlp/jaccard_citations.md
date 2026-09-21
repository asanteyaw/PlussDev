<!-- pluss-hand-authored -->
# NoteNlpAccessor::jaccard_citations

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::jaccard_citations(const Note& other) const
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
`1.0`; one empty and one not → `0.0`. The default printer shows up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`NoteNlpAccessor::jaccard_citations requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `NoteNlpAccessor::jaccard_citations expects equal numel
or scalar broadcast`.

## Example 1: Overlapping Reference Lists

```cpp
std::cout << pluss::make_note({"[1] [2] [3]"})
                 .nlp().jaccard_citations(pluss::make_note({"[2] [3] [4]"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[0.5]
```

Sets `{[1], [2], [3]}` and `{[2], [3], [4]}` share two of four distinct
markers → `0.5`.

## See Also

- [`NoteNlpAccessor::overlap_citations`](overlap_citations.md): the containment score
  over the same sets.
- [`NoteNlpAccessor::intersect_citations`](intersect_citations.md): the shared markers
  themselves.
- [`NoteNlpAccessor::jaccard_tokens`](jaccard_tokens.md) /
  [`NoteNlpAccessor::jaccard_terms`](jaccard_terms.md) /
  [`NoteNlpAccessor::jaccard_acronyms`](jaccard_acronyms.md) /
  [`NoteNlpAccessor::jaccard_symbols`](jaccard_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jaccard_acronyms.html"><span class="label">Previous</span><span class="title">jaccard_acronyms</span></a>
  <a class="next-link" href="jaccard_symbols.html"><span class="label">Next</span><span class="title">jaccard_symbols</span></a>
</div>
