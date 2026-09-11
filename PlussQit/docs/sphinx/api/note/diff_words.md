<!-- pluss-hand-authored -->
# Note::diff_words

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::diff_words(const Note& other) const
```

## Word-Level Difference Between Two Notes

`diff_words(other)` compares each cell of `self` against the matching cell of
`other` one **word** at a time and returns only the **changes** as a
`TokenBatch` of tag-prefixed tokens. It is the word-granularity member of the
diff / align family — the change-only counterpart of
[`align_tokens`](align_tokens.md), and a sibling of
[`diff_lines`](diff_lines.md), [`diff_chars`](diff_chars.md), and
[`align_sentences`](align_sentences.md).

### How the comparison works

Each cell pair is tokenized into words by the same tokenizer as
[`tokenize_words`](tokenize_words.md): a word is a run of ASCII letters and
digits, with `_`, an interior single `-`/`'`, a trailing `+`/`#`, and
recognized URLs / emails / dotted abbreviations kept whole. **Spaces and
standalone punctuation are dropped before the comparison** — `diff_words`
therefore never reports a punctuation-only edit; `"stop here"` and
`"stop, here!"` diff to nothing.

The two word lists are aligned by their **longest common subsequence**, and
only the differing words are emitted, in order, one per token, tagged in the
first byte:

| Tag | Meaning | In `self` | In `other` |
|-----|---------|-----------|------------|
| `-` | removed — word in `self`, not matched in `other` | yes | no |
| `+` | added — word in `other`, not matched in `self` | no  | yes |

Words common to both sides produce no token, so a cell with no word-level
change contributes an **empty range**. A substituted word appears as an
adjacent `-old` / `+new` pair. For the full transcript that keeps the
unchanged words as `=` tokens, use [`align_tokens`](align_tokens.md) — it
runs the same alignment.

### The `TokenBatch` result

Tokens from all cells are concatenated into one flat rank-`1` token `Note`
(`batch.tokens()`). `batch.offsets()` (length `numel + 1`) marks each cell's
run: cell `r` is `tokens()[offsets()[r] .. offsets()[r+1])`. Unchanged cells
have `offsets()[r] == offsets()[r+1]`.

### Shape, pairing, and rejections

The comparison is **per cell and pairwise**. `self` and `other` must have the
same [`numel`](../../note_api_reference.md), or one of them may be a
**scalar** (`numel() == 1`) that is broadcast against every cell of the
other; shape beyond the cell count is ignored, and `offsets()` follows the
non-scalar side. Two rejections, both raising `pluss::Error`:

- `self` in bytes mode → `Note::diff_words requires a UTF-8 text Note (lhs is
  bytes)`.
- unequal `numel` with neither side a scalar → `Note::diff_words expects
  equal numel or scalar broadcast`.

The alignment is `O(m · n)` in words per cell and fully deterministic: on a
tie the walk emits the `-` (removal) before the `+` (insertion).

## Example 1: Per-Cell Changes And Offsets

```cpp
const auto batch =
    pluss::make_note({"the quick brown fox", "hello world"})
        .diff_words(pluss::make_note({"the slow brown fox", "hello there world"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["-quick", "+slow", "+there"]
offsets: [0, 2, 3]
```

Cell 0's edits are `tokens()[0..2)` — `quick` replaced by `slow`. Cell 1's
are `tokens()[2..3)` — `there` inserted between `hello` and `world`.

## Example 2: Identical Cells Produce Nothing

```cpp
const auto batch = pluss::make_note({"no change here"})
                       .diff_words(pluss::make_note({"no change here"}));
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

## Example 3: Scalar Broadcast On The Right

```cpp
const auto batch = pluss::make_note({"a b c", "a x c"})
                       .diff_words(pluss::make_note({"a b c"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["-x", "+b"]
offsets: [0, 0, 2]
```

The single right-hand cell is compared against both left-hand cells. Cell 0
matches exactly (empty range); cell 1 differs by one word.

## Example 4: Everything Inserted

```cpp
const auto batch = pluss::make_note({""})
                       .diff_words(pluss::make_note({"brand new text"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["+brand", "+new", "+text"]
```

An empty `self` cell has no words, so every word of `other` is a `+` token.

## Example 5: Punctuation-Only Changes Are Ignored

```cpp
const auto batch = pluss::make_note({"stop here"})
                       .diff_words(pluss::make_note({"stop, here!"}));
std::cout << "token count: " << batch.tokens().numel() << "\n";
```

### Output

```text
token count: 0
```

The added comma and exclamation mark are separators, not words — no edit is
reported.

## Example 6: Rejections

```cpp
try {
  (void)pluss::make_note({"a", "b"})
      .diff_words(pluss::make_note({"a", "b", "c"}));
} catch (const pluss::Error& e) {
  const std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  (void)pluss::note::bytes({"x"}).diff_words(pluss::make_note({"y"}));
} catch (const pluss::Error& e) {
  const std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Note::diff_words expects equal numel or scalar broadcast
Note::diff_words requires a UTF-8 text Note (lhs is bytes)
```

## See Also

- [`Note::align_tokens`](align_tokens.md): the same alignment, keeping the
  unchanged words as `=` tokens.
- [`Note::diff_lines`](diff_lines.md) / [`Note::diff_chars`](diff_chars.md):
  coarser and finer change views.
- [`Note::align_sentences`](align_sentences.md): sentence-level alignment.
- [`Note::tokenize_words`](tokenize_words.md): the word tokenizer and the
  `TokenBatch` shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff_lines.html">
    <span class="label">Previous</span>
    <span class="title">diff_lines</span>
  </a>
  <a class="next-link" href="dim.html">
    <span class="label">Next</span>
    <span class="title">dim</span>
  </a>
</div>
