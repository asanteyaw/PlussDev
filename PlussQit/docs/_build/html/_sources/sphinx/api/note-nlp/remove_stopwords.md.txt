<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_stopwords

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_stopwords(std::string_view language = "en") const
```

## Remove Common Function Words

`remove_stopwords()` word-tokenizes each cell, removes the built-in stopword vocabulary, and joins survivors with one space. Case-insensitive matching preserves each surviving token’s source spelling.

The supported language is `en`; unsupported labels are rejected. Use `without_stopwords()` when the desired result is a ragged lowercased token sequence.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `language` | Stopword vocabulary. The default and currently supported value is `"en"`. | `"en"` |

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"This is the report from Ada", "A note with detail"});
const auto result = input.nlp().remove_stopwords();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["report Ada", "note detail"]
```

The result is a shape-preserving Text `Note`, not a `TokenBatch`: each cell's rewritten text replaces the original cell in place, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().remove_stopwords();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().remove_stopwords();
} catch (const pluss::Error&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
defined=false
Bytes: rejected
```

## Example 3: Invalid Parameters Fail Before Processing

```cpp
try {
  (void)pluss::note::make("le texte").nlp().remove_stopwords("fr");
} catch (const pluss::Error&) {
  std::cout << "unsupported stopword language: rejected\n";
}
```

### Output

```text
unsupported stopword language: rejected
```

## See Also

- [`without_stopwords`](without_stopwords.md): the same filter applied to an already-tokenized sequence instead of raw text.
- [`clean`](clean.md): run every structural cleanup method in one pass.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_quotes.html"><span class="label">Previous</span><span class="title">remove_quotes</span></a>
  <a class="next-link" href="remove_tables.html"><span class="label">Next</span><span class="title">remove_tables</span></a>
</div>
