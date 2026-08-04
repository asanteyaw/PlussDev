# Note::split

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::split(std::string_view delim) const -> TokenBatch
Note::split(std::string_view delim, std::int64_t limit) const -> TokenBatch
```

## What It Does

Splits each text cell on a literal delimiter and returns the pieces as a
`TokenBatch`.

This is the direct structural splitter for cases where whitespace tokenization
is not the right boundary rule. It is useful for comma-separated fragments,
lightweight field extraction, or ad hoc delimiter-based parsing.

The result is flattened because each input cell can yield a different number of
pieces. `offsets()` is the map back from that flat token stream to the original
cell boundaries.

## Parameters

**delim**

The literal delimiter to split on.

This is not a regex pattern. If you need pattern-driven splitting, use the
regex-based methods instead of `split(...)`.

**limit**

Optional cap on splitting within each cell.

- `split(delim)`
  Uses every matching delimiter boundary in each cell.
- `split(delim, limit)`
  Stops splitting once the requested number of delimiter cuts has been consumed
  inside a cell and leaves the remainder grouped together as one token.

## Returns

`TokenBatch`
    A flat token stream plus `offsets()` describing which token range came from
    each original `Note` element.

## Examples

Example 1. Split on the first comma only and preserve the remaining suffix.

```cpp
auto note = pluss::make_note({"A,b;c", "x,y,z"});
auto tokens = note.split(",", 1);
std::cout << "split_comma_limit1 tokens:";
for (std::int64_t i = 0; i < tokens.tokens().numel(); ++i) {
  std::cout << " [" << tokens.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_comma_limit1 offsets:";
for (std::int64_t off : tokens.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_comma_limit1 tokens: [A] [b;c] [x] [y,z]
split_comma_limit1 offsets: 0 2 4
```

Example 2. Use the one-argument form when every delimiter boundary should be
honored.

```cpp
auto note = pluss::make_note({"k:v:w", "id:42"});
auto tokens = note.split(":");
std::cout << "split_colon tokens:";
for (std::int64_t i = 0; i < tokens.tokens().numel(); ++i) {
  std::cout << " [" << tokens.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_colon offsets:";
for (std::int64_t off : tokens.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_colon tokens: [k] [v] [w] [id] [42]
split_colon offsets: 0 3 5
```

Example 3. Use a multi-character literal delimiter when the text is only
lightly structured.

```cpp
auto note = pluss::make_note({"left -> right", "keep -> watch"});
auto tokens = note.split(" -> ");
std::cout << "split_arrow tokens:";
for (std::int64_t i = 0; i < tokens.tokens().numel(); ++i) {
  std::cout << " [" << tokens.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_arrow offsets:";
for (std::int64_t off : tokens.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_arrow tokens: [left] [right] [keep] [watch]
split_arrow offsets: 0 2 4
```

## Notes

- The `limit` overload is the important variant when you are parsing lightly
  structured text instead of fully tokenizing it.
- The delimiter is always literal text, so multi-character separators such as
  `" -> "` are valid and are matched as a single boundary unit.
- In the example above, each row contributes exactly two output tokens because
  the split is capped after the first comma.
- If your real boundary rule is simply whitespace, [`split_whitespace`](split_whitespace.md)
  is the simpler starting point.

## See Also

- [`Note::split_lines`](split_lines.md): line-aware splitting.
- [`Note::split_whitespace`](split_whitespace.md): whitespace-aware splitting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quote.html">
    <span class="label">Previous</span>
    <span class="title">quote</span>
  </a>
  <a class="next-link" href="tokenize_words.html">
    <span class="label">Next</span>
    <span class="title">tokenize_words</span>
  </a>
</div>
