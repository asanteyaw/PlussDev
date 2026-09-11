<!-- pluss-hand-authored -->
# Note::overlap_tokens

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::overlap_tokens(const Note& other) const
```

## Word-Set Containment Score Between Two Notes

`overlap_tokens(other)` reduces each cell of `self` and the matching cell of
`other` to a **set of words** and returns their **overlap coefficient** — the
size of the intersection divided by the size of the *smaller* set — as a
shape-preserving `Note` of score strings. It is the word-level containment
member of the set-similarity family, next to
[`jaccard_tokens`](jaccard_tokens.md) (a mutual-similarity score) and
[`intersect_tokens`](intersect_tokens.md) (which returns the shared words).

### How the score is computed

Each cell is tokenized with the ASCII word tokenizer (as
[`tokenize_words`](tokenize_words.md) does), lowercased, and deduplicated
into sets `A` (from `self`) and `B` (from `other`). The score is

```text
|A ∩ B| / min(|A|, |B|)
```

a value in `[0, 1]`, symmetric.

**The key property:** the score is `1.0` whenever the smaller set is entirely
contained in the larger one, no matter how much bigger the larger set is. So
`overlap_tokens` answers "is one cell's vocabulary a subset of the other's?"
rather than "how similar are the two vocabularies?" — for the latter, use
[`jaccard_tokens`](jaccard_tokens.md), where the extra words in the larger
set lower the score.

### Edge cases

- **Both sets empty** → `"1.0"`.
- **One set empty, the other not** → `"0.0"` (the smaller size is `0`).

### Result shape and formatting

The result is a `Note` with the **same shape** as the pairing's non-scalar
side. Each score is formatted with up to six decimal places, trailing zeros
trimmed, always at least one digit after the point (`"1.0"`, `"0.666667"`).

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Rejections raise `pluss::Error`:

- either side in bytes mode → `Note::overlap_tokens requires a UTF-8 text
  Note (lhs is bytes)` (or `rhs is bytes`).
- unequal `numel` with neither side a scalar → `Note::overlap_tokens expects
  equal numel or scalar broadcast`.

## Example 1: A Subset Scores 1.0

```cpp
std::cout << pluss::make_note({"quick brown fox"})
                 .overlap_tokens(pluss::make_note({"the quick brown fox jumps over"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["1.0"]
```

Every word of the smaller set `{quick, brown, fox}` is in the larger set, so
the coefficient is `1.0`. [`jaccard_tokens`](jaccard_tokens.md) on the same
pair would give `3/6 = 0.5`.

## Example 2: A Genuine Partial Overlap

```cpp
std::cout << pluss::make_note({"a b c d"})
                 .overlap_tokens(pluss::make_note({"c d e"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.666667"]
```

Sets `{a, b, c, d}` and `{c, d, e}` share two words; the smaller set has
three → `2/3`.

## Example 3: Disjoint And One-Empty

```cpp
std::cout << pluss::make_note({"a b"}).overlap_tokens(pluss::make_note({"c d"})) << "\n";
std::cout << pluss::make_note({"a b"}).overlap_tokens(pluss::make_note({""})) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.0"]
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0.0"]
```

## See Also

- [`Note::jaccard_tokens`](jaccard_tokens.md): `|A ∩ B| / |A ∪ B|` — size
  differences lower the score.
- [`Note::intersect_tokens`](intersect_tokens.md): the shared words, not a
  score.
- [`Note::overlap_terms`](overlap_terms.md): the same score over content
  words only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="overlap_terms.html">
    <span class="label">Previous</span>
    <span class="title">overlap_terms</span>
  </a>
  <a class="next-link" href="owned.html">
    <span class="label">Next</span>
    <span class="title">owned</span>
  </a>
</div>
