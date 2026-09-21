<!-- pluss-hand-authored -->
# NoteNlpAccessor::formality

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::formality() const
```

## Score Formal Register

`formality()` returns a Float64 score in `[0, 1]` for each cell. Longer vocabulary raises the score; a small informal-word lexicon lowers it. Empty text receives the neutral score `0.5`.

This deterministic lexical indicator is useful for ranking or filtering; it is not a learned authorship judgment.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Float64 `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"yo gonna chill", "The committee requests comprehensive documentation."});
const auto result = input.nlp().formality();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Float64)
[0.166667, 1]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().formality();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().formality();
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

- [`readability_grade`](readability_grade.md) / [`readability_level`](readability_level.md): reading-difficulty scores with the same shape-preserving contract.
- [`subjectivity`](subjectivity.md) / [`sentiment`](sentiment.md): the other opinion-oriented lexical scores.
- [`spam_score`](spam_score.md): a promotional-language score built the same way.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expand_contractions.html"><span class="label">Previous</span><span class="title">expand_contractions</span></a>
  <a class="next-link" href="hashtags.html"><span class="label">Next</span><span class="title">hashtags</span></a>
</div>
