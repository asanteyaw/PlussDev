<!-- pluss-hand-authored -->
# NoteNlpAccessor::overlap_acronyms

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::overlap_acronyms(const Note& other) const
```

## Acronym-Set Containment Score Between Two Notes

`overlap_acronyms(other)` scores each cell pair by the overlap coefficient of
their **acronym** sets and returns a shape-preserving `Note` of score
strings. It is the acronym-set containment member of the set-similarity
family; see also [`jaccard_acronyms`](jaccard_acronyms.md) and
[`intersect_acronyms`](intersect_acronyms.md).

### How the score is computed

An acronym is a run of **two or more uppercase ASCII letters** on a word
boundary (`NASA`, `ESA`); matches are lowercased and deduplicated into sets
`A` and `B`. The score is `|A ∩ B| / min(|A|, |B|)`, in `[0, 1]`, symmetric —
`1.0` when the smaller acronym set is contained in the larger.

A cell with no uppercase letters has an empty acronym set. Both sets empty →
`1.0`; one empty and one not → `0.0`. The default printer shows up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`NoteNlpAccessor::overlap_acronyms requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `NoteNlpAccessor::overlap_acronyms expects equal numel
or scalar broadcast`.

## Example 1: Smaller Set Fully Contained

```cpp
std::cout << pluss::make_note({"NASA ESA"})
                 .nlp().overlap_acronyms(pluss::make_note({"NASA ESA NOAA JAXA"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[1]
```

`{nasa, esa}` is a subset of `{nasa, esa, noaa, jaxa}`, so the coefficient is
`1.0`. [`jaccard_acronyms`](jaccard_acronyms.md) here would give `2/4 = 0.5`.

## See Also

- [`NoteNlpAccessor::jaccard_acronyms`](jaccard_acronyms.md): the mutual-similarity
  score over the same sets.
- [`NoteNlpAccessor::intersect_acronyms`](intersect_acronyms.md): the shared acronyms
  themselves.
- [`NoteNlpAccessor::overlap_tokens`](overlap_tokens.md) /
  [`NoteNlpAccessor::overlap_terms`](overlap_terms.md) /
  [`NoteNlpAccessor::overlap_citations`](overlap_citations.md) /
  [`NoteNlpAccessor::overlap_symbols`](overlap_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numbers.html"><span class="label">Previous</span><span class="title">numbers</span></a>
  <a class="next-link" href="overlap_citations.html"><span class="label">Next</span><span class="title">overlap_citations</span></a>
</div>
