<!-- pluss-hand-authored -->
# Note::NlpAccessor::subjectivity

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::subjectivity() const
```

## Score Subjective Language

`subjectivity()` returns a Float64 score in `[0, 1]` from a neutral baseline plus the density of opinion-bearing vocabulary. Empty text receives `0.5`.

The score measures lexical cues, not truth, certainty, or author identity.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Float64 `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"The vessel weighs 20 tonnes.", "I believe this wonderful idea is awful."});
const auto result = input.nlp().subjectivity();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0.2, 0.771429]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().subjectivity();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().subjectivity();
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

- [`sentiment`](sentiment.md): positive/negative polarity, the companion axis to subjectivity's opinion-vs-fact measure.
- [`formality`](formality.md) / [`readability_grade`](readability_grade.md): the other lexical register scores.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stems.html"><span class="label">Previous</span><span class="title">stems</span></a>
  <a class="next-link" href="terms.html"><span class="label">Next</span><span class="title">terms</span></a>
</div>
