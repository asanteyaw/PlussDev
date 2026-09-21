<!-- pluss-hand-authored -->
# NoteNlpAccessor::min_length

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::min_length(std::int64_t length) const
```

## Keep Tokens At Or Above A Length

`min_length()` word-tokenizes each cell and keeps tokens whose encoded byte length is at least `length`. The comparison is inclusive and preserves source order.

`length` must be non-negative. `min_length(0)` therefore retains every token.

The result is a closed `TokenBatch`. `tokens()` is one rank-1 Text Note containing every emitted token in row-major cell order; `offsets()` is a rank-1 Int64 Note of length `receiver.numel() + 1`. Cell `r` owns the half-open token range `[offsets[r], offsets[r + 1])`. Empty cells therefore retain an empty range rather than disappearing. Tokens never cross a cell boundary.

An undefined receiver produces a defined structural batch whose token Note is undefined and whose offsets are `[0]`. An empty defined receiver produces an empty token Note and one zero offset per logical boundary. Bool, Int64, Float64, and Bytes receivers are rejected because this operation requires UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `length` | Inclusive encoded-byte threshold; must be non-negative. | Required |

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto sequence = pluss::note::make({
    "The Fox fox 42 jumps quickly",
    "beta Alpha beta 7"});
const auto result = sequence.nlp().min_length(5);
std::cout << "tokens=" << pluss::note::print(result.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < result.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << result.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[3], dim=1, numel=3, encoding=utf8)
["jumps", "quickly", "Alpha"]
offsets=[0, 2, 3]
```

For a `TokenBatch`, the offsets above are part of the result, not display metadata: they are what reconstruct the ragged sequence belonging to each source cell. For a shape-preserving Note, the printed shape confirms that analysis did not reduce or flatten the receiver.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().min_length(5);
std::cout << "tokens-defined=" << std::boolalpha
          << absent.tokens().defined() << " offset0="
          << absent.offsets().integers()[0] << '\n';
try {
  (void)pluss::note::make({true}).nlp().min_length(5);
} catch (const pluss::Error&) {
  std::cout << "Bool: rejected\n";
}
```

### Output

```text
tokens-defined=false offset0=0
Bool: rejected
```

## Example 3: Invalid Parameters Fail Before Processing

```cpp
try {
  (void)pluss::note::make("red blue").nlp().min_length(-1);
} catch (const pluss::Error&) {
  std::cout << "min_length negative: rejected\n";
}
```

### Output

```text
min_length negative: rejected
```

## See Also

- [`max_length`](max_length.md): the paired upper-bound filter.
- [`match_tokens`](match_tokens.md): a pattern-based filter instead of a length-based one.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="metaphone.html"><span class="label">Previous</span><span class="title">metaphone</span></a>
  <a class="next-link" href="named_entities.html"><span class="label">Next</span><span class="title">named_entities</span></a>
</div>
