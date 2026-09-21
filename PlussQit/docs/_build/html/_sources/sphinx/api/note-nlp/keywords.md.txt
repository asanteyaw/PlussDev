<!-- pluss-hand-authored -->
# NoteNlpAccessor::keywords

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::keywords(std::int64_t count) const
```

## Rank Frequent Content Words

`keywords()` lowercases word tokens, omits the built-in English stopword set, counts the remaining words independently in each cell, and emits at most `count` distinct words. Frequency descends; lexical order breaks ties.

`count` must be positive. This is deterministic frequency ranking, not semantic keyphrase inference.

The result is a closed `TokenBatch`. `tokens()` is one rank-1 Text Note containing every emitted token in row-major cell order; `offsets()` is a rank-1 Int64 Note of length `receiver.numel() + 1`. Cell `r` owns the half-open token range `[offsets[r], offsets[r + 1])`. Empty cells therefore retain an empty range rather than disappearing. Tokens never cross a cell boundary.

An undefined receiver produces a defined structural batch whose token Note is undefined and whose offsets are `[0]`. An empty defined receiver produces an empty token Note and one zero offset per logical boundary. Bool, Int64, Float64, and Bytes receivers are rejected because this operation requires UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `count` | Maximum distinct ranked results per cell; must be positive. | Required |

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto corpus = pluss::note::make({
    "red blue red green red blue",
    "machine learning improves machine learning systems"});
const auto result = corpus.nlp().keywords(2);
std::cout << "tokens=" << pluss::note::print(result.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < result.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << result.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[4], dim=1, numel=4, encoding=utf8)
["red", "blue", "learning", "machine"]
offsets=[0, 2, 4]
```

For a `TokenBatch`, the offsets above are part of the result, not display metadata: they are what reconstruct the ragged sequence belonging to each source cell. For a shape-preserving Note, the printed shape confirms that analysis did not reduce or flatten the receiver.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().keywords(2);
std::cout << "tokens-defined=" << std::boolalpha
          << absent.tokens().defined() << " offset0="
          << absent.offsets().integers()[0] << '\n';
try {
  (void)pluss::note::make({true}).nlp().keywords(2);
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
  (void)pluss::note::make("red red").nlp().keywords(0);
} catch (const pluss::Error&) {
  std::cout << "keywords zero: rejected\n";
}
```

### Output

```text
keywords zero: rejected
```

## See Also

- [`keyphrases`](keyphrases.md): the multi-word sibling of this frequency-ranked extractor.
- [`terms`](terms.md): the unranked vocabulary this ranking is built from.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keyphrases.html"><span class="label">Previous</span><span class="title">keyphrases</span></a>
  <a class="next-link" href="language.html"><span class="label">Next</span><span class="title">language</span></a>
</div>
