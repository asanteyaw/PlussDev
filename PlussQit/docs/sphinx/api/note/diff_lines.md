<!-- pluss-hand-authored -->
# Note::diff_lines

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::diff_lines(const Note& other) const
```

## Line-Level Difference Between Two Notes

`diff_lines(other)` compares each cell of `self` against the matching cell of
`other` one **line** at a time and returns only the changed lines as a
`TokenBatch` of tag-prefixed tokens. It is the line-granularity member of the
diff / align family, alongside [`nlp().diff_words()`](../note-nlp/diff_words.md),
[`diff_chars`](diff_chars.md), and the equal-included
[`nlp().align_sentences()`](../note-nlp/align_sentences.md) /
[`nlp().align_tokens()`](../note-nlp/align_tokens.md).

### How the comparison works

Each cell pair is split on `\n` — as [`split_lines`](split_lines.md) does,
**without** keeping the line terminator, so tokens carry no trailing newline.
The two line lists are aligned by their **longest common subsequence**, and
because this is a `diff_*` method only the lines that differ are emitted, in
order, one per token, tagged in the first byte:

| Tag | Meaning | In `self` | In `other` |
|-----|---------|-----------|------------|
| `-` | removed — line in `self`, not matched in `other` | yes | no |
| `+` | added — line in `other`, not matched in `self` | no  | yes |

Lines identical on both sides produce no token, so a cell with no line-level
change contributes an **empty range**. A line that is edited rather than
moved shows as a `-`/`+` pair: the old text leaves, the new text arrives.
For a transcript that also keeps the unchanged lines, there is no
`align_lines` spelling — run `diff_lines` and merge the `-`/`+` tokens back
against the original line list.

### The `TokenBatch` result

Tokens from all cells are concatenated into the rank-`1` Note
`batch.tokens()`. The rank-`1` Int64 Note `batch.offsets()` has length
`numel + 1` and marks each cell's run: cell `r` owns `tokens.at(k)` for `k`
in `[offsets[r], offsets[r + 1])`. Unchanged
cells have an empty range.

### Shape, pairing, and rejections

Comparison is per cell and pairwise. `self` and `other` must share
[`numel`](../../note_api_reference.md), or one may be a **scalar**
(`numel() == 1`) broadcast against the other. Rejections raise
`pluss::Error`:

- bytes-mode `self` → `Note::diff_lines requires a UTF-8 text Note (lhs is
  bytes)`.
- unequal `numel`, neither side scalar → `Note::diff_lines expects equal
  numel or scalar broadcast`.

The alignment is `O(m · n)` in lines per cell and deterministic: on a tie the
`-` is emitted before the `+`.

## Example 1: An Edited Line And An Appended Line

```cpp
const auto batch = pluss::make_note({"alpha\nbeta\ngamma"})
                       .diff_lines(pluss::make_note({"alpha\nBETA\ngamma\ndelta"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["-beta", "+BETA", "+delta"]
offsets: [0, 3]
```

`alpha` and `gamma` are unchanged. `beta` is re-cased — `-beta` out,
`+BETA` in — and `delta` is appended.

## Example 2: Identical Cells Produce Nothing

```cpp
const auto batch = pluss::make_note({"one\ntwo"})
                       .diff_lines(pluss::make_note({"one\ntwo"}));
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

## Example 3: A Single Inserted Line

```cpp
const auto batch = pluss::make_note({"one\ntwo"})
                       .diff_lines(pluss::make_note({"one\nEXTRA\ntwo"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["+EXTRA"]
```

`one` and `two` stay; only the inserted line is reported.

## See Also

- [`Note::split_lines`](split_lines.md): the line split used here.
- [`NoteNlpAccessor::diff_words`](../note-nlp/diff_words.md) / [`Note::diff_chars`](diff_chars.md):
  finer-grained change views.
- [`NoteNlpAccessor::align_tokens`](../note-nlp/align_tokens.md) /
  [`NoteNlpAccessor::align_sentences`](../note-nlp/align_sentences.md): equal-included transcripts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff_chars.html">
    <span class="label">Previous</span>
    <span class="title">diff_chars</span>
  </a>
  <a class="next-link" href="dim.html">
    <span class="label">Next</span>
    <span class="title">dim</span>
  </a>
</div>
