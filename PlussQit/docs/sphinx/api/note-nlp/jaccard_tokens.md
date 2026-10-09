<!-- pluss-hand-authored -->
# Note::NlpAccessor::jaccard_tokens

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::jaccard_tokens(const Note& other) const
```

## Word-Set Similarity Score Between Two Notes

`jaccard_tokens(other)` reduces each cell of `self` and the matching cell of
`other` to a **set of words** and returns the **Jaccard index** of the two
sets — the size of their intersection divided by the size of their union — as
a shape-preserving Float64 `Note` of native scores. It is the word-level scoring
member of the set-similarity family, alongside
[`overlap_tokens`](overlap_tokens.md) (a containment score) and
[`intersect_tokens`](intersect_tokens.md) (which returns the shared words
themselves rather than a number).

### How the score is computed

Each cell is tokenized with the ASCII word tokenizer (as
[`tokens`](tokens.md) does), lowercased, and deduplicated
into a set `A` (from `self`) and `B` (from `other`). The score is

```text
|A ∩ B| / |A ∪ B|
```

a value in `[0, 1]`: `1` when the two word sets are identical, `0` when they
are disjoint. It is **symmetric** — `a.nlp().jaccard_tokens(b)` equals
`b.nlp().jaccard_tokens(a)`.

There is no length or stopword filter; for a score over content words only,
use [`jaccard_terms`](jaccard_terms.md).

### Edge cases

- **Both sets empty** (both cells tokenize to nothing) → `1.0`: two empty
  sets are equal.
- **One set empty, the other not** → `0.0`.

### Result dtype and shape

The result is a Float64 `Note` with the **same shape** as the pairing's
non-scalar side, one native score per comparison. Values retain binary
double precision; decimal shortening is only a presentation choice made by
the Note printer. Use `reals()` for a borrowed contiguous host view or
`float_item()` when the result contains exactly one score.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../note/numel.md) or a
scalar (`numel() == 1`) broadcast. Rejections raise `pluss::Error`:

- either side in bytes mode → `Note::NlpAccessor::jaccard_tokens requires a UTF-8 text
  Note (lhs is bytes)` (or `rhs is bytes`).
- unequal `numel` with neither side a scalar → `Note::NlpAccessor::jaccard_tokens expects
  equal numel or scalar broadcast`.

## Example 1: Per-Cell Scores

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"the quick brown fox", "alpha beta gamma"})
        .nlp().jaccard_tokens(pluss::make_note({"the quick red fox", "beta gamma delta"})))
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0.6, 0.5]
```

Cell 0: `{the, quick, brown, fox}` and `{the, quick, red, fox}` share three
of five distinct words → `3/5 = 0.6`. Cell 1: `{alpha, beta, gamma}` and
`{beta, gamma, delta}` share two of four → `0.5`.

## Example 2: Identical And Disjoint

```cpp
std::cout << pluss::make_note({"same words here"})
                 .nlp().jaccard_tokens(pluss::make_note({"same words here"}))
          << "\n";
std::cout << pluss::make_note({"a b c"}).nlp().jaccard_tokens(pluss::make_note({"d e f"}))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[1]
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[0]
```

## Example 3: Empty Sets

```cpp
std::cout << pluss::make_note({""}).nlp().jaccard_tokens(pluss::make_note({""})) << "\n";
std::cout << pluss::make_note({"a b"}).nlp().jaccard_tokens(pluss::make_note({""})) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[1]
Note(shape=[1], dim=1, numel=1, dtype=Float64)
[0]
```

Two empty word sets score `1.0`; one empty against one non-empty scores
`0.0`.

## Example 4: Scalar Broadcast

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"cat dog", "dog bird fish"})
        .nlp().jaccard_tokens(pluss::make_note({"dog"})))
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0.5, 0.333333]
```

The single right-hand cell `{dog}` is scored against each left-hand cell:
`{cat, dog}` → `1/2`; `{dog, bird, fish}` → `1/3`.

## Example 5: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"cat dog", "sun moon"}, {"red blue", "up down"}})
        .nlp().jaccard_tokens(pluss::make_note({{"cat fish", "sun moon"}, {"red green", "left right"}})))
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, dtype=Float64)
[
  [0.333333, 1],
  [0.333333, 0]
]
```

## See Also

- [`Note::NlpAccessor::overlap_tokens`](overlap_tokens.md): `|A ∩ B| / min(|A|, |B|)` —
  scores containment instead of mutual similarity.
- [`Note::NlpAccessor::intersect_tokens`](intersect_tokens.md): the shared words, not a
  score.
- [`Note::NlpAccessor::jaccard_terms`](jaccard_terms.md): the same score over content
  words only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jaccard_terms.html"><span class="label">Previous</span><span class="title">jaccard_terms</span></a>
  <a class="next-link" href="keyphrases.html"><span class="label">Next</span><span class="title">keyphrases</span></a>
</div>
