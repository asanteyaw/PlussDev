<!-- pluss-hand-authored -->
# Note::NlpAccessor::align_tokens

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::NlpAccessor::align_tokens(const Note& other) const
```

## Word-Level Alignment Of Two Notes

`align_tokens(other)` compares each cell of `self` against the matching cell
of `other` one **word** at a time and returns the full alignment as a
`TokenBatch` of tag-prefixed tokens. It is the word-granularity member of the
diff / align family — the counterpart of
[`align_sentences`](align_sentences.md), and the equal-included sibling of
[`diff_words`](diff_words.md).

### How the comparison works

Each cell pair is first tokenized into words with the same tokenizer as
[`tokens`](tokens.md): a word is a run of ASCII letters and
digits (with `_`, interior single `-`/`'`, trailing `+`/`#`, and recognized
URLs / emails / dotted abbreviations kept whole). **Everything between words
— spaces and standalone punctuation — is dropped before the comparison**, so
`align_tokens` never reports a punctuation-only edit; `"stop here"` and
`"stop, here!"` align as two `=` tokens.

The two word lists are aligned by their **longest common subsequence** and
every word is emitted once, in order, with a one-byte tag:

| Tag | Meaning | In `self` | In `other` |
|-----|---------|-----------|------------|
| `=` | unchanged — part of the common subsequence | yes | yes |
| `-` | removed — in `self`, not matched in `other` | yes | no |
| `+` | added — in `other`, not matched in `self` | no  | yes |

The tag is the first character of the token; the word follows immediately.
Keeping the `=` tokens makes each cell's stream a full transcript that
rebuilds `other` from `self`. For the change-only view, use
[`diff_words`](diff_words.md) — it runs the same alignment and omits the `=`
tokens.

### The `TokenBatch` result

Tags from all cells are concatenated into one rank-`1` token `Note`,
`batch.tokens()`. The rank-`1` Int64 Note `batch.offsets()` has length
`numel + 1` and marks each cell's run. Cell `r` owns `tokens.at(k)` for `k`
in `[offsets[r], offsets[r + 1])`. Identical cells contribute a run of `=`
tokens, one per word.

### Shape, pairing, and rejections

Comparison is per cell and pairwise. `self` and `other` must share the same
[`numel`](../note/numel.md), or one may be a **scalar**
(`numel() == 1`) broadcast against every cell of the other. Rejections raise
`pluss::Error`:

- bytes-mode `self` → `Note::NlpAccessor::align_tokens requires a UTF-8 text Note (lhs is
  bytes)`.
- unequal `numel`, neither side scalar → `Note::NlpAccessor::align_tokens expects equal
  numel or scalar broadcast`.

The alignment is `O(m · n)` in words per cell and deterministic: on a tie the
`-` (removal) is emitted before the `+` (insertion).

## Example 1: One Word Substituted

```cpp
const auto batch = pluss::make_note({"the quick brown fox"})
                       .nlp().align_tokens(pluss::make_note({"the slow brown fox"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["=the", "-quick", "+slow", "=brown", "=fox"]
offsets: [0, 5]
```

`the`, `brown`, and `fox` are shared. `quick` is replaced by `slow`, which
shows as the adjacent `-quick` / `+slow` pair.

## Example 2: Identical Cells Are All `=`

```cpp
const auto batch = pluss::make_note({"same words"})
                       .nlp().align_tokens(pluss::make_note({"same words"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["=same", "=words"]
```

## Example 3: Per-Cell Runs

```cpp
const auto batch = pluss::make_note({"a b c", "x y"})
                       .nlp().align_tokens(pluss::make_note({"a c", "x z"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["=a", "-b", "=c", "=x", "-y", "+z"]
offsets: [0, 3, 6]
```

Cell 0's alignment is `tokens()[0..3)` (`b` deleted); cell 1's is
`tokens()[3..6)` (`y` replaced by `z`).

## See Also

- [`Note::NlpAccessor::diff_words`](diff_words.md): the same alignment without the `=`
  tokens.
- [`Note::NlpAccessor::align_sentences`](align_sentences.md): alignment at sentence
  granularity.
- [`Note::NlpAccessor::tokens`](tokens.md): the word tokenizer and the
  `TokenBatch` shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_sentences.html"><span class="label">Previous</span><span class="title">align_sentences</span></a>
  <a class="next-link" href="alnum_tokens.html"><span class="label">Next</span><span class="title">alnum_tokens</span></a>
</div>
