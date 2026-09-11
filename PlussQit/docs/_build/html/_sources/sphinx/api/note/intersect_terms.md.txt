<!-- pluss-hand-authored -->
# Note::intersect_terms

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::intersect_terms(const Note& other) const
```

## Shared Content Words Between Two Notes

`intersect_terms(other)` is [`intersect_tokens`](intersect_tokens.md)
restricted to **content words**: it reduces each cell to the set of
significant words and returns the ones present in both cells as a
`TokenBatch`. Use it when function words (`the`, `and`, `with`) and short
fillers would only add noise to the comparison. The scoring members for the
same set are [`jaccard_terms`](jaccard_terms.md) and
[`overlap_terms`](overlap_terms.md).

### How the set is built

Each cell is tokenized with the ASCII word tokenizer and every token is
lowercased. A token joins the set only if it is

- **4 or more characters** long, and
- **not a stopword** — the short closed-class list `a, an, and, are, as, at,
  be, by, for, from, in, into, is, it, its, of, on, or, that, the, their,
  this, to, with`.

The set is then deduplicated and case-folded. No stemming is applied, so
`process` and `processes` are different members.

### What is emitted

For each cell pair, the members in both sets are emitted in the order they
first appear in `self`, once each; a cell with no shared term contributes an
empty range. `batch.offsets()` (length `numel + 1`) marks each cell's run.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::intersect_terms requires a UTF-8 text Note (lhs is bytes)` (or `rhs`);
a numel mismatch raises `Note::intersect_terms expects equal numel or scalar
broadcast`.

## Example 1: Fillers And Short Words Drop Out

```cpp
const auto batch = pluss::make_note({"The cat sat on the warm mat"})
                       .intersect_terms(pluss::make_note({"a warm mat and a cat"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["warm"]
offsets: [0, 1]
```

`cat`, `sat`, and `mat` are only three characters, so they never enter either
set; `the`, `on`, `a`, and `and` are stopwords. `warm` is the one term the
two cells share.

## See Also

- [`Note::intersect_tokens`](intersect_tokens.md): the same operation with no
  content filter.
- [`Note::jaccard_terms`](jaccard_terms.md) /
  [`Note::overlap_terms`](overlap_terms.md): score the shared-term overlap.
- [`Note::intersect_acronyms`](intersect_acronyms.md) /
  [`Note::intersect_citations`](intersect_citations.md) /
  [`Note::intersect_math_symbols`](intersect_math_symbols.md): other set
  kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect_math_symbols.html">
    <span class="label">Previous</span>
    <span class="title">intersect_math_symbols</span>
  </a>
  <a class="next-link" href="intersect_tokens.html">
    <span class="label">Next</span>
    <span class="title">intersect_tokens</span>
  </a>
</div>
