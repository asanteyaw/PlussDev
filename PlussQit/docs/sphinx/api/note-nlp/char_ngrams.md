<!-- pluss-hand-authored -->
# NoteNlpAccessor::char_ngrams

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::char_ngrams(std::int64_t width) const
```

## Build Overlapping Character Windows

`char_ngrams()` splits each cell into UTF-8 code points, then emits every contiguous window of exactly `width` code points. Windows overlap by `width - 1`; no separators are inserted.

Use `word_ngrams()` for token windows. `width` must be positive; a cell shorter than the width contributes an empty run.

The result is a closed `TokenBatch`. `tokens()` is one rank-1 Text Note containing every emitted token in row-major cell order; `offsets()` is a rank-1 Int64 Note of length `receiver.numel() + 1`. Cell `r` owns the half-open token range `[offsets[r], offsets[r + 1])`. Empty cells therefore retain an empty range rather than disappearing. Tokens never cross a cell boundary.

An undefined receiver produces a defined structural batch whose token Note is undefined and whose offsets are `[0]`. An empty defined receiver produces an empty token Note and one zero offset per logical boundary. Bool, Int64, Float64, and Bytes receivers are rejected because this operation requires UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `width` | Exact number of code points or words in each window; must be positive. | Required |

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"cafe", "go"});
const auto result = input.nlp().char_ngrams(3);
std::cout << "tokens=" << pluss::note::print(result.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < result.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << result.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[2], dim=1, numel=2, encoding=utf8)
["caf", "afe"]
offsets=[0, 2, 2]
```

For a `TokenBatch`, the offsets above are part of the result, not display metadata: they are what reconstruct the ragged sequence belonging to each source cell. For a shape-preserving Note, the printed shape confirms that analysis did not reduce or flatten the receiver.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().char_ngrams(3);
std::cout << "tokens-defined=" << std::boolalpha
          << absent.tokens().defined() << " offset0="
          << absent.offsets().integers()[0] << '\n';
try {
  (void)pluss::note::make({true}).nlp().char_ngrams(3);
} catch (const pluss::Error&) {
  std::cout << "Bool: rejected\n";
}
```

### Output

```text
tokens-defined=false offset0=0
Bool: rejected
```

## Example 3: Invalid Parameters Fail Before Processing

```cpp
try {
  (void)pluss::note::make("cafe").nlp().char_ngrams(0);
} catch (const pluss::Error&) {
  std::cout << "char_ngrams zero: rejected\n";
}
```

### Output

```text
char_ngrams zero: rejected
```

## See Also

- [`word_ngrams`](word_ngrams.md): the word-level sibling of this character-level n-gram extractor.
- [`keywords`](keywords.md) / [`keyphrases`](keyphrases.md): frequency-ranked extraction rather than a raw sliding window.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="alpha_tokens.html"><span class="label">Previous</span><span class="title">alpha_tokens</span></a>
  <a class="next-link" href="citations.html"><span class="label">Next</span><span class="title">citations</span></a>
</div>
