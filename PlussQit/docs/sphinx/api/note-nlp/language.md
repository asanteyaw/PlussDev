<!-- pluss-hand-authored -->
# NoteNlpAccessor::language

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::language() const
```

## Detect A Dominant Language Label

`language()` returns a shape-preserving Text Note containing a short language label for each cell. Script evidence is considered first, then lightweight lexical cues distinguish supported Latin-script languages; empty or unsupported text yields `unknown`.

Use `script()` when writing system alone is the useful fact. Detection is deterministic and local, with no model or network dependency.

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
const auto result = analysis.nlp().language();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["en", "en", "en", "ru"]
```

The result is a shape-preserving Note, not a `TokenBatch`: there are no offsets to reconstruct, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().language();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().language();
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

- [`tokens`](tokens.md): the canonical word tokenizer and its detailed boundary rules.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keywords.html"><span class="label">Previous</span><span class="title">keywords</span></a>
  <a class="next-link" href="lemmas.html"><span class="label">Next</span><span class="title">lemmas</span></a>
</div>
