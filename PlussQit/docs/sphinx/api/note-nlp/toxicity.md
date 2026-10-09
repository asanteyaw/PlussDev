<!-- pluss-hand-authored -->
# Note::NlpAccessor::toxicity

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::toxicity() const
```

## Score Toxic Lexical Density

`toxicity()` returns a Float64 score in `[0, 1]` from the proportion of words found in the built-in toxic-language lexicon, with saturation at one.

This is a transparent screening signal, not a contextual moderation decision.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Float64 `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"kind and helpful", "idiot hate stupid"});
const auto result = input.nlp().toxicity();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0, 1]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().toxicity();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().toxicity();
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

- [`sentiment`](sentiment.md) / [`spam_score`](spam_score.md): the other content-moderation-oriented scores.
- [`mask_pii`](mask_pii.md): sanitize identifying spans rather than scoring tone.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tokens.html"><span class="label">Previous</span><span class="title">tokens</span></a>
  <a class="next-link" href="unique_tokens.html"><span class="label">Next</span><span class="title">unique_tokens</span></a>
</div>
