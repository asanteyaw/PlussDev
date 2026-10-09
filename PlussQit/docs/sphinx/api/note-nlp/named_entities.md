<!-- pluss-hand-authored -->
# Note::NlpAccessor::named_entities

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::NlpAccessor::named_entities() const
```

## Extract Title-Cased Name Candidates

`named_entities()` finds contiguous runs of ASCII words that begin with an uppercase letter followed by lowercase letters, removes duplicates within each cell, and keeps first-occurrence order.

This is a fast lexical candidate extractor. Sentence-initial words can be candidates even when they are not semantic entities.

The result is a closed `TokenBatch`. `tokens()` is one rank-1 Text Note containing every emitted token in row-major cell order; `offsets()` is a rank-1 Int64 Note of length `receiver.numel() + 1`. Cell `r` owns the half-open token range `[offsets[r], offsets[r + 1])`. Empty cells therefore retain an empty range rather than disappearing. Tokens never cross a cell boundary.

An undefined receiver produces a defined structural batch whose token Note is undefined and whose offsets are `[0]`. An empty defined receiver produces an empty token Note and one zero offset per logical boundary. Bool, Int64, Float64, and Bytes receivers are rejected because this operation requires UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto lexical = pluss::note::make({
    "Alice visited New York. Bob is running quickly!",
    "The systems were tested and studies improved."});
const auto result = lexical.nlp().named_entities();
std::cout << "tokens=" << pluss::note::print(result.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < result.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << result.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Alice", "New York", "Bob", "The"]
offsets=[0, 3, 4]
```

For a `TokenBatch`, the offsets above are part of the result, not display metadata: they are what reconstruct the ragged sequence belonging to each source cell. For a shape-preserving Note, the printed shape confirms that analysis did not reduce or flatten the receiver.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().named_entities();
std::cout << "tokens-defined=" << std::boolalpha
          << absent.tokens().defined() << " offset0="
          << absent.offsets().integers()[0] << '\n';
try {
  (void)pluss::note::make({true}).nlp().named_entities();
} catch (const pluss::Error&) {
  std::cout << "Bool: rejected\n";
}
```

### Output

```text
tokens-defined=false offset0=0
Bool: rejected
```

## See Also

- [`acronyms`](acronyms.md): the uppercase-only sibling extractor.
- [`pos_tags`](pos_tags.md): the part-of-speech context entity detection can use.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min_length.html"><span class="label">Previous</span><span class="title">min_length</span></a>
  <a class="next-link" href="normalize_numbers.html"><span class="label">Next</span><span class="title">normalize_numbers</span></a>
</div>
