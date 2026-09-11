<!-- pluss-hand-authored -->
# Note::jaccard_terms

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::jaccard_terms(const Note& other) const
```

## Content-Word Similarity Score Between Two Notes

`jaccard_terms(other)` is [`jaccard_tokens`](jaccard_tokens.md) restricted to
**content words**: it scores each cell pair by the Jaccard index of their
significant-word sets, ignoring function words and short fillers. The result
is a shape-preserving `Note` of score strings. The companion members for the
same set are [`overlap_terms`](overlap_terms.md) and
[`intersect_terms`](intersect_terms.md).

### How the score is computed

Each cell is tokenized and lowercased; a word joins the set only if it is
**4 or more characters** long and **not a stopword** (`a, an, and, are, as,
at, be, by, for, from, in, into, is, it, its, of, on, or, that, the, their,
this, to, with`). No stemming is applied. With sets `A` (from `self`) and
`B` (from `other`), the score is `|A ∩ B| / |A ∪ B|`, in `[0, 1]`, symmetric.

Both sets empty (neither cell has a qualifying term) → `"1.0"`. One empty and
one not → `"0.0"`. Scores are formatted with up to six decimals, trailing
zeros trimmed (`"1.0"`, `"0.5"`).

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::jaccard_terms requires a UTF-8 text Note (lhs is bytes)` (or `rhs`); a
numel mismatch raises `Note::jaccard_terms expects equal numel or scalar
broadcast`.

## Example 1: Only Content Words Count

```cpp
std::cout << pluss::make_note({"The system processes documents"})
                 .jaccard_terms(pluss::make_note({"a system that processes files"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.5"]
```

`the`, `a`, and `that` are stopwords and drop out. The term sets are
`{system, processes, documents}` and `{system, processes, files}` — two
shared out of four distinct → `0.5`.

## See Also

- [`Note::jaccard_tokens`](jaccard_tokens.md): the same score with no content
  filter.
- [`Note::overlap_terms`](overlap_terms.md): the containment score over the
  same sets.
- [`Note::intersect_terms`](intersect_terms.md): the shared terms themselves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jaccard_math_symbols.html">
    <span class="label">Previous</span>
    <span class="title">jaccard_math_symbols</span>
  </a>
  <a class="next-link" href="jaccard_tokens.html">
    <span class="label">Next</span>
    <span class="title">jaccard_tokens</span>
  </a>
</div>
