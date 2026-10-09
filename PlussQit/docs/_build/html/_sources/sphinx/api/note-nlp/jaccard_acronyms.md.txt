<!-- pluss-hand-authored -->
# Note::NlpAccessor::jaccard_acronyms

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::jaccard_acronyms(const Note& other) const
```

## Acronym-Set Similarity Score Between Two Notes

`jaccard_acronyms(other)` scores each cell pair by the Jaccard index of their
**acronym** sets and returns a shape-preserving Float64 `Note` of native scores. It
is the acronym-set scoring member of the set-similarity family; see also
[`overlap_acronyms`](overlap_acronyms.md) and
[`intersect_acronyms`](intersect_acronyms.md).

### How the score is computed

An acronym is a run of **two or more uppercase ASCII letters** on a word
boundary (`NASA`, `ESA`); matches are lowercased and deduplicated into sets
`A` and `B`. The score is `|A ∩ B| / |A ∪ B|`, in `[0, 1]`, symmetric.

A cell with no uppercase letters has an empty acronym set. Both sets empty →
`1.0`; one empty and one not → `0.0`. The default printer shows up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::NlpAccessor::jaccard_acronyms requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::NlpAccessor::jaccard_acronyms expects equal numel
or scalar broadcast`.

## Example 1: Two Of Four Acronyms Shared

```cpp
std::cout << pluss::make_note({"NASA ESA NOAA"})
                 .nlp().jaccard_acronyms(pluss::make_note({"NASA ESA JAXA"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[0.5]
```

Sets `{nasa, esa, noaa}` and `{nasa, esa, jaxa}` share two of four distinct
acronyms → `0.5`.

## See Also

- [`Note::NlpAccessor::overlap_acronyms`](overlap_acronyms.md): the containment score over
  the same sets.
- [`Note::NlpAccessor::intersect_acronyms`](intersect_acronyms.md): the shared acronyms
  themselves.
- [`Note::NlpAccessor::jaccard_tokens`](jaccard_tokens.md) /
  [`Note::NlpAccessor::jaccard_terms`](jaccard_terms.md) /
  [`Note::NlpAccessor::jaccard_citations`](jaccard_citations.md) /
  [`Note::NlpAccessor::jaccard_symbols`](jaccard_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect_tokens.html"><span class="label">Previous</span><span class="title">intersect_tokens</span></a>
  <a class="next-link" href="jaccard_citations.html"><span class="label">Next</span><span class="title">jaccard_citations</span></a>
</div>
