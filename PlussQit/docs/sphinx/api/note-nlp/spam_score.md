<!-- pluss-hand-authored -->
# Note::NlpAccessor::spam_score

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::spam_score() const
```

## Score Spam-Like Surface Signals

`spam_score()` returns a Float64 score in `[0, 1]` from promotional vocabulary, links, unsubscribe language, and numeric pressure, normalized by document length.

It is a transparent heuristic signal rather than a classifier verdict. Inspect thresholds against the target corpus.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Float64 `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto analysis = pluss::note::make({
    "I love this excellent release!",
    "FREE money now http://spam.test unsubscribe",
    "This ordinary sentence describes a system.",
    "Привет мир"});
const auto result = analysis.nlp().spam_score();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, dtype=Float64)
[0, 1, 0, 0]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().spam_score();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().spam_score();
} catch (const pluss::Error&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
defined=false
Bytes: rejected
```

## See Also

- [`toxicity`](toxicity.md) / [`sentiment`](sentiment.md): the other content-moderation-oriented scores.
- [`formality`](formality.md): a related lexical register signal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="soundex.html"><span class="label">Previous</span><span class="title">soundex</span></a>
  <a class="next-link" href="stems.html"><span class="label">Next</span><span class="title">stems</span></a>
</div>
