<!-- pluss-hand-authored -->
# Note::diff_chars

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::diff_chars(const Note& other) const
```

## Character-Level Difference Between Two Notes

`diff_chars(other)` compares each cell of `self` against the matching cell of
`other` one character at a time and returns only the **changes** as a
`TokenBatch` of tag-prefixed tokens. It is the finest-grained member of the
diff / align family; the coarser ones are [`diff_words`](diff_words.md),
[`diff_lines`](diff_lines.md), and (sentence granularity, with equal tokens)
[`align_sentences`](align_sentences.md).

### How the comparison works

Each cell pair is split into characters, and the two character lists are
aligned by their **longest common subsequence**. Because `diff_chars` is a
`diff_*` method, only the characters that differ are emitted — one per token,
in reading order, with a one-byte tag:

| Tag | Meaning | In `self` | In `other` |
|-----|---------|-----------|------------|
| `-` | removed — in `self`, not matched in `other` | yes | no |
| `+` | added — in `other`, not matched in `self` | no  | yes |

The tag is the first byte of the token; the character follows. Characters
common to both sides produce no token, so a cell whose two sides are
identical contributes an **empty range**. For the full transcript that keeps
the unchanged characters as well, run [`align_tokens`](align_tokens.md) — but
note that aligns words, not characters; there is no equal-included character
spelling, so reconstruct from the `-`/`+` stream and the original text when
you need one.

Splitting is by Unicode character, not raw byte: a multi-byte character such
as `é` or `🎯` is one token, tag included.

### The `TokenBatch` result

Tokens from all cells are concatenated into one flat rank-`1` token `Note`
(`batch.tokens()`); `batch.offsets()` (length `numel + 1`) marks each cell's
run — cell `r` is `tokens()[offsets()[r] .. offsets()[r+1])`. An unchanged
cell's range is empty (`offsets()[r] == offsets()[r+1]`).

### Shape, pairing, and rejections

Comparison is per cell and pairwise. `self` and `other` must share
[`numel`](../../note_api_reference.md), or one may be a **scalar**
(`numel() == 1`) broadcast against the other. Rejections raise
`pluss::Error`:

- bytes-mode `self` → `Note::diff_chars requires a UTF-8 text Note (lhs is
  bytes)`.
- unequal `numel`, neither side scalar → `Note::diff_chars expects equal
  numel or scalar broadcast`.

The alignment is `O(m · n)` in characters per cell — practical for words,
identifiers, and short strings; for long documents prefer
[`diff_words`](diff_words.md) or [`diff_lines`](diff_lines.md). It is
deterministic: on a tie the `-` is emitted before the `+`.

## Example 1: A Character Edit Script

```cpp
const auto batch = pluss::make_note({"kitten"})
                       .diff_chars(pluss::make_note({"sitting"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["-k", "+s", "-e", "+i", "+g"]
offsets: [0, 5]
```

`k → s`, `e → i`, then a trailing `g` is inserted; `itt` and `n` are shared
and produce nothing.

## Example 2: A Pure Insertion

```cpp
const auto batch = pluss::make_note({"abc"})
                       .diff_chars(pluss::make_note({"abXYZc"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["+X", "+Y", "+Z"]
```

Only `+` tokens — nothing was removed from `self`.

## Example 3: Identical Cells Produce Nothing

```cpp
const auto batch = pluss::make_note({"same"})
                       .diff_chars(pluss::make_note({"same"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets: [0, 0]
```

An empty token `Note` and an empty range for the one cell.

## See Also

- [`Note::diff_words`](diff_words.md) / [`Note::diff_lines`](diff_lines.md):
  coarser change-only views.
- [`Note::align_tokens`](align_tokens.md) /
  [`Note::align_sentences`](align_sentences.md): transcripts that keep the
  unchanged tokens.
- [`Note::tokenize_words`](tokenize_words.md): the `TokenBatch` shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="diff_lines.html">
    <span class="label">Next</span>
    <span class="title">diff_lines</span>
  </a>
</div>
