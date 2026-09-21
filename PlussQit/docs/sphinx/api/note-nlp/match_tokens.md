<!-- pluss-hand-authored -->
# NoteNlpAccessor::match_tokens

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteNlpAccessor::match_tokens(std::string_view pattern) const
```

## Filter Tokens With A Pattern

`match_tokens()` word-tokenizes each cell, then retains tokens for which the ECMAScript regular expression finds a match. Matching is search-based unless the pattern supplies anchors.

The pattern must be non-empty and syntactically valid. Use `regex().findall()` to match spans in the original cell rather than filtering word tokens.

The result is a closed `TokenBatch`. `tokens()` is one rank-1 Text Note containing every emitted token in row-major cell order; `offsets()` is a rank-1 Int64 Note of length `receiver.numel() + 1`. Cell `r` owns the half-open token range `[offsets[r], offsets[r + 1])`. Empty cells therefore retain an empty range rather than disappearing. Tokens never cross a cell boundary.

An undefined receiver produces a defined structural batch whose token Note is undefined and whose offsets are `[0]`. An empty defined receiver produces an empty token Note and one zero offset per logical boundary. Bool, Int64, Float64, and Bytes receivers are rejected because this operation requires UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `pattern` | Non-empty ECMAScript regular expression searched within each token. | Required |

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto sequence = pluss::note::make({
    "The Fox fox 42 jumps quickly",
    "beta Alpha beta 7"});
const auto result = sequence.nlp().match_tokens("^[A-Z]");
std::cout << "tokens=" << pluss::note::print(result.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < result.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << result.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[3], dim=1, numel=3, encoding=utf8)
["The", "Fox", "Alpha"]
offsets=[0, 2, 3]
```

For a `TokenBatch`, the offsets above are part of the result, not display metadata: they are what reconstruct the ragged sequence belonging to each source cell. For a shape-preserving Note, the printed shape confirms that analysis did not reduce or flatten the receiver.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().match_tokens("^[A-Z]");
std::cout << "tokens-defined=" << std::boolalpha
          << absent.tokens().defined() << " offset0="
          << absent.offsets().integers()[0] << '\n';
try {
  (void)pluss::note::make({true}).nlp().match_tokens("^[A-Z]");
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
  (void)pluss::note::make("red blue").nlp().match_tokens("");
} catch (const pluss::Error&) {
  std::cout << "empty match pattern: rejected\n";
}
```

### Output

```text
empty match pattern: rejected
```

## See Also

- [`min_length`](min_length.md) / [`max_length`](max_length.md): the sibling token-sequence filters.
- [`without_stopwords`](without_stopwords.md) / [`without_numbers`](without_numbers.md): other predicate-based filters.
- [TokenBatch API Reference](../../tokenbatch_api_reference.md): ownership, offsets, emptiness, and row reconstruction for ragged results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_urls.html"><span class="label">Previous</span><span class="title">mask_urls</span></a>
  <a class="next-link" href="math_symbols.html"><span class="label">Next</span><span class="title">math_symbols</span></a>
</div>
