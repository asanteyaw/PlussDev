<!-- pluss-hand-authored -->
# Note::NlpAccessor::script

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::script() const
```

## Detect The Dominant Writing System

`script()` returns a shape-preserving Text Note naming the dominant supported script in each cell. Empty or script-free text yields `Unknown`.

Use `language()` when a language label is required; script detection is the more conservative choice for names and short fragments.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto analysis = pluss::note::make({
    "I love this excellent release!",
    "FREE money now http://spam.test unsubscribe",
    "This ordinary sentence describes a system.",
    "Привет мир"});
const auto result = analysis.nlp().script();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Latin", "Latin", "Latin", "Cyrillic"]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().script();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().script();
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

- [`language`](language.md): the fuller language label this writing-system signal feeds into.
- [`to_ascii`](to_ascii.md) / [`to_cyrillic`](to_cyrillic.md) / [`to_greek`](to_greek.md): transliterate once the script is known.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_urls.html"><span class="label">Previous</span><span class="title">remove_urls</span></a>
  <a class="next-link" href="sentences.html"><span class="label">Next</span><span class="title">sentences</span></a>
</div>
