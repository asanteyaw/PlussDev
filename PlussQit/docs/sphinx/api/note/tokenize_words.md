# Note::tokenize_words

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::tokenize_words() const -> TokenBatch
Note::tokenize_words(std::int64_t flags) const -> TokenBatch
```

## What It Does

Tokenizes each text cell into word-like units.

This sits one step above raw delimiter splitting: instead of splitting on a
single literal character or on whitespace only, `tokenize_words()` applies the
library’s word tokenization behavior and returns a flattened token stream with
case offsets.

Use this when punctuation and spacing are not enough to describe the token
boundaries you want. It is the bridge between very light structural splitting
and fuller NLP surfaces.

## Parameters

**flags**

Optional tokenizer-behavior flags.

- `tokenize_words()`
  Uses the library's default word-tokenization policy.
- `tokenize_words(flags)`
  Uses an explicit flag bitmask. Reach for this overload only when you already
  know the library-defined flag contract you want to opt into.

This page does not invent semantics for unnamed flag bits. When a future pass
documents a specific flag family, it should do so with explicit flag names and
verified examples rather than hand-waving about possible behaviors.

## Returns

`TokenBatch`
    A flat word-token stream plus `offsets()` describing which tokens belong to
    each input cell.

## Examples

Example 1. Tokenize mixed punctuation and newline text into words.

```cpp
auto note = pluss::make_note({"One.\nTwo!", "A,b;c"});
auto out = note.tokenize_words();
std::cout << "tokenize_words tokens:";
for (std::int64_t i = 0; i < out.tokens().numel(); ++i) {
  std::cout << " [" << out.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "tokenize_words offsets:";
for (std::int64_t off : out.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
tokenize_words tokens: [One] [Two] [A] [b] [c]
tokenize_words offsets: 0 2 5
```

Example 2. Normalize case first when you want lowercase word tokens directly.

```cpp
auto note = pluss::make_note({"Ada LOVELACE", "Grace Hopper"});
auto lower = note.lower();
auto out = lower.tokenize_words();
std::cout << "tokenize_words_lower tokens:";
for (std::int64_t i = 0; i < out.tokens().numel(); ++i) {
  std::cout << " [" << out.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "tokenize_words_lower offsets:";
for (std::int64_t off : out.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
tokenize_words_lower tokens: [ada] [lovelace] [grace] [hopper]
tokenize_words_lower offsets: 0 2 4
```

Example 3. Compare it mentally with `split_whitespace()` when you want a more
word-like split but still need to see what the default tokenizer keeps intact.

```cpp
auto note = pluss::make_note({"price=12.50, paid!", "ref:A-17"});
auto out = note.tokenize_words();
std::cout << "tokenize_words_punct tokens:";
for (std::int64_t i = 0; i < out.tokens().numel(); ++i) {
  std::cout << " [" << out.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "tokenize_words_punct offsets:";
for (std::int64_t off : out.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
tokenize_words_punct tokens: [price] [12] [50] [paid] [ref] [A-17]
tokenize_words_punct offsets: 0 4 6
```

## Notes

- The current default behavior often strips obvious punctuation boundaries, but
  some compound fragments can remain intact, as `A-17` does in the third
  example.
- `offsets()` show that the first input cell contributed two words and the
  second contributed three.
- If your use case is pure whitespace structure without word-token logic,
  [`split_whitespace`](split_whitespace.md) is the more direct primitive.

## See Also

- [`Note::split_whitespace`](split_whitespace.md): simpler structural token
  splitting.
- [`Note::regex_findall`](regex_findall.md): extract only tokens that match a
  pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="nlp_mask_pii.html">
    <span class="label">Next</span>
    <span class="title">nlp_mask_pii</span>
  </a>
</div>
