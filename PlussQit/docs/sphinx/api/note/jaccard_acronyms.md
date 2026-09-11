<!-- pluss-hand-authored -->
# Note::jaccard_acronyms

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::jaccard_acronyms(const Note& other) const
```

## Acronym-Set Similarity Score Between Two Notes

`jaccard_acronyms(other)` scores each cell pair by the Jaccard index of their
**acronym** sets and returns a shape-preserving `Note` of score strings. It
is the acronym-set scoring member of the set-similarity family; see also
[`overlap_acronyms`](overlap_acronyms.md) and
[`intersect_acronyms`](intersect_acronyms.md).

### How the score is computed

An acronym is a run of **two or more uppercase ASCII letters** on a word
boundary (`NASA`, `ESA`); matches are lowercased and deduplicated into sets
`A` and `B`. The score is `|A ∩ B| / |A ∪ B|`, in `[0, 1]`, symmetric.

A cell with no uppercase letters has an empty acronym set. Both sets empty →
`"1.0"`; one empty and one not → `"0.0"`. Scores use up to six decimals,
trailing zeros trimmed.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::jaccard_acronyms requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::jaccard_acronyms expects equal numel
or scalar broadcast`.

## Example 1: Two Of Four Acronyms Shared

```cpp
std::cout << pluss::make_note({"NASA ESA NOAA"})
                 .jaccard_acronyms(pluss::make_note({"NASA ESA JAXA"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.5"]
```

Sets `{nasa, esa, noaa}` and `{nasa, esa, jaxa}` share two of four distinct
acronyms → `0.5`.

## See Also

- [`Note::overlap_acronyms`](overlap_acronyms.md): the containment score over
  the same sets.
- [`Note::intersect_acronyms`](intersect_acronyms.md): the shared acronyms
  themselves.
- [`Note::jaccard_tokens`](jaccard_tokens.md) /
  [`Note::jaccard_terms`](jaccard_terms.md) /
  [`Note::jaccard_citations`](jaccard_citations.md) /
  [`Note::jaccard_math_symbols`](jaccard_math_symbols.md): other set kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_whitespace.html">
    <span class="label">Previous</span>
    <span class="title">is_whitespace</span>
  </a>
  <a class="next-link" href="jaccard_citations.html">
    <span class="label">Next</span>
    <span class="title">jaccard_citations</span>
  </a>
</div>
