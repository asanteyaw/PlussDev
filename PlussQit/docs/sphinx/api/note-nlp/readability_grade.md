<!-- pluss-hand-authored -->
# NoteNlpAccessor::readability_grade

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::readability_grade() const
```

## Estimate Reading Grade

`readability_grade()` returns a non-negative Float64 grade for each cell from sentence count, word count, and heuristic vowel-run syllables. Negative raw values are clamped to zero.

Use `readability_level()` when a coarse label is easier to consume. Very short text can produce unstable grades because the formula has little evidence.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Float64 `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"Cats run.", "Extraordinary terminology complicates comprehension considerably."});
const auto result = input.nlp().readability_grade();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0, 40.64]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().readability_grade();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().readability_grade();
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

- [`readability_level`](readability_level.md): the same estimate expressed as a named band instead of a raw grade number.
- [`formality`](formality.md) / [`subjectivity`](subjectivity.md): the other lexical register scores.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="punct_tokens.html"><span class="label">Previous</span><span class="title">punct_tokens</span></a>
  <a class="next-link" href="readability_level.html"><span class="label">Next</span><span class="title">readability_level</span></a>
</div>
