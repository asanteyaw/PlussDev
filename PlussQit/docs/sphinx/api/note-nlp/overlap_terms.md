<!-- pluss-hand-authored -->
# NoteNlpAccessor::overlap_terms

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::overlap_terms(const Note& other) const
```

## Content-Word Containment Score Between Two Notes

`overlap_terms(other)` is [`overlap_tokens`](overlap_tokens.md) restricted to
**content words**: it scores each cell pair by the overlap coefficient of
their significant-word sets, ignoring function words and short fillers. The
result is a shape-preserving Float64 `Note` of native scores. The companion members
for the same set are [`jaccard_terms`](jaccard_terms.md) and
[`intersect_terms`](intersect_terms.md).

### How the score is computed

Each cell is tokenized and lowercased; a word joins the set only if it is
**4 or more characters** long and **not a stopword** (`a, an, and, are, as,
at, be, by, for, from, in, into, is, it, its, of, on, or, that, the, their,
this, to, with`). No stemming. With sets `A` and `B`, the score is
`|A ∩ B| / min(|A|, |B|)`, in `[0, 1]`, symmetric — `1.0` when the smaller
term set is contained in the larger.

Both sets empty → `1.0`; one empty and one not → `0.0`. The default printer
shows up to six decimals and trims trailing zeros; storage remains Float64.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`NoteNlpAccessor::overlap_terms requires a UTF-8 text Note (lhs is bytes)` (or `rhs`); a
numel mismatch raises `NoteNlpAccessor::overlap_terms expects equal numel or scalar
broadcast`.

## Example 1: Two Of Three Terms Contained

```cpp
std::cout << pluss::make_note({"machine learning models"})
                 .nlp().overlap_terms(pluss::make_note({"statistical learning models with data"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[0.666667]
```

Term sets `{machine, learning, models}` and `{statistical, learning, models,
data}` — `with` is a stopword. They share two terms; the smaller set has
three → `2/3`.

## See Also

- [`NoteNlpAccessor::overlap_tokens`](overlap_tokens.md): the same score with no content
  filter.
- [`NoteNlpAccessor::jaccard_terms`](jaccard_terms.md): the mutual-similarity score over
  the same sets.
- [`NoteNlpAccessor::intersect_terms`](intersect_terms.md): the shared terms themselves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="overlap_symbols.html"><span class="label">Previous</span><span class="title">overlap_symbols</span></a>
  <a class="next-link" href="overlap_tokens.html"><span class="label">Next</span><span class="title">overlap_tokens</span></a>
</div>
