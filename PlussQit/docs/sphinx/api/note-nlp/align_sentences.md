<!-- pluss-hand-authored -->
# NoteNlpAccessor::align_sentences

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::align_sentences(const Note& other) const
```

## Sentence-Level Alignment Of Two Notes

`align_sentences(other)` compares each cell of `self` against the matching
cell of `other` one sentence at a time and returns the full alignment as a
`TokenBatch` of **tag-prefixed** tokens. It is the sentence-granularity
member of the diff / align family; see also
[`align_tokens`](align_tokens.md) (word granularity),
[`diff_words`](diff_words.md), [`diff_lines`](../note/diff_lines.md), and
[`diff_chars`](../note/diff_chars.md).

### How the comparison works

For each cell pair the two texts are first cut into sentences (the same
sentence segmentation used elsewhere in `Note` — a run ending at `.`, `!`, or
`?` followed by whitespace, with common abbreviations kept whole). The two
sentence lists are then aligned by their **longest common subsequence**: the
longest ordered set of sentences that appear, unchanged, in both. Every
sentence is emitted exactly once, in reading order, carrying a one-byte tag:

| Tag | Meaning | Present in `self` | Present in `other` |
|-----|---------|-------------------|--------------------|
| `=` | unchanged — part of the common subsequence | yes | yes |
| `-` | removed — in `self`, not matched in `other` | yes | no |
| `+` | added — in `other`, not matched in `self` | no  | yes |

The tag is the **first character** of each emitted token; the sentence text
follows with no separator. Recover the text with
[`remove_prefix`](../note/remove_prefix.md) or a one-character substring, and filter
by reading `token[0]`.

Because `align_*` keeps the `=` tokens, the token stream for a cell is a
complete transcript: replay it in order and you reproduce `other` from
`self`. For a change-only view — the `-` and `+` tokens with the `=`
sentences dropped — there is no separate `diff_sentences` spelling; call
`align_sentences` and skip the tokens whose first byte is `=`.

### The `TokenBatch` result

All tags from all cells are concatenated into the rank-`1` Note
`batch.tokens()`. The rank-`1` Int64 Note `batch.offsets()` has length
`numel + 1`; cell `r` owns `tokens.at(k)` for `k` in
`[offsets[r], offsets[r + 1])`. A cell whose two sides are
identical contributes a run of `=` tokens (one per sentence), never an empty
range — that empty-range case is what [`diff_words`](diff_words.md) and the
other `diff_*` methods produce.

### Shape, pairing, and rejections

The comparison is **per cell and pairwise**. `self` and `other` must have the
same [`numel`](../note/numel.md), or one of them may be a **scalar**
(`numel() == 1`) that is broadcast against every cell of the other; any other
shape is ignored beyond its cell count. Two rejections, both raising
`pluss::Error`:

- `self` in bytes mode → `NoteNlpAccessor::align_sentences requires a UTF-8 text Note
  (lhs is bytes)`.
- unequal `numel` with neither side a scalar → `NoteNlpAccessor::align_sentences expects
  equal numel or scalar broadcast`.

The alignment is `O(m · n)` in sentences per cell and fully deterministic:
on a tie the walk emits the `-` (removal) before the `+` (insertion).

## Example 1: A Substituted Sentence

```cpp
const auto batch = pluss::make_note({"One. Two. Three."})
                       .nlp().align_sentences(pluss::make_note({"One. Two changed. Three."}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["=One.", "-Two.", "+Two changed.", "=Three."]
offsets: [0, 4]
```

`"One."` and `"Three."` are shared, so they carry `=`. The middle sentence
is a substitution: the old form leaves as `-Two.` and the new form arrives as
`+Two changed.`.

## Example 2: Identical Cells Are All `=`

```cpp
const auto batch = pluss::make_note({"Stay put. Do not move."})
                       .nlp().align_sentences(pluss::make_note({"Stay put. Do not move."}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["=Stay put.", "=Do not move."]
```

Every sentence matched — two `=` tokens, no `-` or `+`.

## Example 3: An Appended Sentence

```cpp
const auto batch = pluss::make_note({"First one. Second one."})
                       .nlp().align_sentences(pluss::make_note({"First one. Second one. Third one."}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["=First one.", "=Second one.", "+Third one."]
```

The two original sentences are unchanged; the extra sentence in `other` is
the single `+` token.

## See Also

- [`NoteNlpAccessor::align_tokens`](align_tokens.md): the same alignment at word
  granularity.
- [`NoteNlpAccessor::diff_words`](diff_words.md) / [`Note::diff_lines`](../note/diff_lines.md) /
  [`Note::diff_chars`](../note/diff_chars.md): change-only views at other
  granularities.
- [`NoteNlpAccessor::tokens`](tokens.md): the `TokenBatch` shape in
  detail.
- [`Note::remove_prefix`](../note/remove_prefix.md): strip the one-byte tag back off
  a token.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="acronyms.html"><span class="label">Previous</span><span class="title">acronyms</span></a>
  <a class="next-link" href="align_tokens.html"><span class="label">Next</span><span class="title">align_tokens</span></a>
</div>
