<!-- pluss-hand-authored -->
# Note::NlpAccessor::intersect_citations

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::NlpAccessor::intersect_citations(const Note& other) const
```

## Shared Citation Markers Between Two Notes

`intersect_citations(other)` reduces each cell to its set of **citation
markers** and returns the markers present in both cells as a `TokenBatch`.
It is the citation-set member of the set-similarity family; the scoring
members are [`jaccard_citations`](jaccard_citations.md) and
[`overlap_citations`](overlap_citations.md).

### How the set is built

A citation marker is either

- a **bracketed number** — `[` then one to three digits then `]` (`[1]`,
  `[42]`, `[100]`), or
- an **author–year** group — `(` then a capitalized name then `,` then a
  four-digit year (optionally with a letter suffix) then `)` — for example
  `(Smith, 2020)` or `(Lee, 2021a)`.

Markers are kept **with their brackets** and lowercased for comparison. A
cell with no `[` or `(` **and** no digit produces an empty set. The set is
deduplicated.

### What is emitted

Members in both sets, in the order they first appear in `self`, once each; a
cell with no shared marker contributes an empty range. The rank-`1` Int64
Note `batch.offsets()` has length `numel + 1` and marks each cell's run.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::NlpAccessor::intersect_citations requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::NlpAccessor::intersect_citations expects equal
numel or scalar broadcast`.

## Example 1: Shared Numbered References

```cpp
const auto batch = pluss::make_note({"see [1] and [2] and [10]"})
                       .nlp().intersect_citations(pluss::make_note({"refer to [2], [3], [10]"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["[2]", "[10]"]
offsets: [0, 2]
```

`[2]` and `[10]` appear in both cells; `[1]` and `[3]` do not. The brackets
are part of each token.

## See Also

- [`Note::NlpAccessor::jaccard_citations`](jaccard_citations.md) /
  [`Note::NlpAccessor::overlap_citations`](overlap_citations.md): score the shared-citation
  overlap.
- [`Note::NlpAccessor::intersect_tokens`](intersect_tokens.md) /
  [`Note::NlpAccessor::intersect_terms`](intersect_terms.md) /
  [`Note::NlpAccessor::intersect_acronyms`](intersect_acronyms.md) /
  [`Note::NlpAccessor::intersect_symbols`](intersect_symbols.md): other set
  kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect_acronyms.html"><span class="label">Previous</span><span class="title">intersect_acronyms</span></a>
  <a class="next-link" href="intersect_symbols.html"><span class="label">Next</span><span class="title">intersect_symbols</span></a>
</div>
