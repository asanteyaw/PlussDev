<!-- pluss-hand-authored -->
# NoteNlpAccessor::intersect_tokens

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::intersect_tokens(const Note& other) const
```

## Shared Words Between Two Notes

`intersect_tokens(other)` reduces each cell of `self` and the matching cell of
`other` to a **set of words**, then returns the words present in **both** as a
`TokenBatch`. It is the word-level member of the set-similarity family; the
scoring members are [`jaccard_tokens`](jaccard_tokens.md) and
[`overlap_tokens`](overlap_tokens.md), and the other set kinds are
[`intersect_terms`](intersect_terms.md),
[`intersect_acronyms`](intersect_acronyms.md),
[`intersect_citations`](intersect_citations.md), and
[`intersect_symbols`](intersect_symbols.md).

### How the set is built

Each cell is tokenized with the ASCII word tokenizer (the one
[`tokens`](tokens.md) uses) and every token is **lowercased**.
There is no length or stopword filter — for a content-word set that ignores
`the`, `and`, `with`, and other function words, use
[`intersect_terms`](intersect_terms.md). The set is **deduplicated** and
case-folded, so a word that occurs many times counts once and `FOX` and `fox`
are the same member.

### What is emitted

For each cell pair, `intersect_tokens` emits the members that appear in both
sets, **in the order they first appear in `self`**, once each. A cell whose
two sets share nothing contributes an **empty range**.

### The `TokenBatch` result

Shared words from all cells are concatenated into the rank-`1` Note
`batch.tokens()`. The rank-`1` Int64 Note `batch.offsets()` has length
`numel + 1` and marks each cell's run, so cell `r` owns `tokens.at(k)` for
`k` in `[offsets[r], offsets[r + 1])`.

### Shape, pairing, and rejections

The comparison is **per cell and pairwise**. `self` and `other` must have the
same [`numel`](../note/numel.md), or one may be a **scalar**
(`numel() == 1`) broadcast against every cell of the other. Unlike the
scoring members, `intersect_tokens` always returns a flat `TokenBatch`, not a
shaped `Note`. Rejections raise `pluss::Error`:

- either side in bytes mode → `NoteNlpAccessor::intersect_tokens requires a UTF-8 text
  Note (lhs is bytes)` (or `rhs is bytes`).
- unequal `numel` with neither side a scalar → `NoteNlpAccessor::intersect_tokens
  expects equal numel or scalar broadcast`.

## Example 1: Shared Words, Per Cell

```cpp
const auto batch =
    pluss::make_note({"the quick brown fox", "alpha beta gamma"})
        .nlp().intersect_tokens(pluss::make_note({"a quick red fox", "beta gamma delta"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["quick", "fox", "beta", "gamma"]
offsets: [0, 2, 4]
```

Cell 0 shares `quick` and `fox` (`tokens()[0..2)`); cell 1 shares `beta` and
`gamma` (`tokens()[2..4)`).

## Example 2: Deduplicated, In `self` Order, Case-Folded

```cpp
const auto batch = pluss::make_note({"fox fox dog cat fox"})
                       .nlp().intersect_tokens(pluss::make_note({"cat FOX"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["fox", "cat"]
```

`fox` appears three times in `self` but once in the result, before `cat`
because it is seen first; `FOX` in `other` matches it after case-folding.

## Example 3: Scalar Broadcast

```cpp
const auto batch = pluss::make_note({"cat dog", "dog bird"})
                       .nlp().intersect_tokens(pluss::make_note({"dog"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["dog", "dog"]
offsets: [0, 1, 2]
```

## Example 4: Nothing In Common

```cpp
const auto batch = pluss::make_note({"one two"})
                       .nlp().intersect_tokens(pluss::make_note({"three four"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets: [0, 0]
```

## See Also

- [`NoteNlpAccessor::jaccard_tokens`](jaccard_tokens.md) /
  [`NoteNlpAccessor::overlap_tokens`](overlap_tokens.md): score the same word sets.
- [`NoteNlpAccessor::intersect_terms`](intersect_terms.md): the content-word set.
- [`NoteNlpAccessor::tokens`](tokens.md): the word tokenizer and the
  `TokenBatch` shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect_terms.html"><span class="label">Previous</span><span class="title">intersect_terms</span></a>
  <a class="next-link" href="jaccard_acronyms.html"><span class="label">Next</span><span class="title">jaccard_acronyms</span></a>
</div>
