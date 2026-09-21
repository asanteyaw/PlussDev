<!-- pluss-hand-authored -->
# NoteNlpAccessor::readability_level

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::readability_level() const
```

## Classify A Readability Band

`readability_level()` maps the same raw readability estimate used by `readability_grade()` to `elementary`, `middle_school`, `high_school`, or `college`.

Use the numeric grade when ordering close cases; the label intentionally discards within-band differences.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"Cats run.", "Extraordinary terminology complicates comprehension considerably."});
const auto result = input.nlp().readability_level();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["elementary", "college"]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().readability_level();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().readability_level();
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

- [`readability_grade`](readability_grade.md): the raw numeric grade this level is bucketed from.
- [`formality`](formality.md) / [`subjectivity`](subjectivity.md): the other lexical register scores.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="readability_grade.html"><span class="label">Previous</span><span class="title">readability_grade</span></a>
  <a class="next-link" href="remove_boilerplate.html"><span class="label">Next</span><span class="title">remove_boilerplate</span></a>
</div>
