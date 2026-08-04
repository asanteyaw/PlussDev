# Note::split_whitespace

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::split_whitespace() const -> TokenBatch
Note::split_whitespace(std::int64_t limit) const -> TokenBatch
```

## What It Does

Splits each text cell on whitespace and returns the resulting tokens as a
`TokenBatch`.

This is one of the most practical first tokenization steps in the `Note`
surface. It is appropriate when you want a fast structural token split without
full NLP parsing.

The method groups all returned tokens into one flat token stream and uses
`offsets()` to preserve which pieces came from which original cell.

## Parameters

**limit**

Optional control over how many splits are attempted inside each cell.

- `split_whitespace()`
  Uses every whitespace boundary and returns the ordinary token stream.
- `split_whitespace(limit)`
  Consumes only the requested number of whitespace cuts within each cell and
  keeps the remainder grouped together.

When `limit` is used, the remainder of each cell stays grouped together after
the requested number of splits has been consumed.

## Returns

`TokenBatch`
    A flat token stream plus `offsets()` describing the token span that came
    from each original `Note` element.

## Examples

Example 1. Tokenize a two-row note into whitespace-separated terms.

```cpp
auto note = pluss::make_note({
    "alpha   beta\tgamma",
    "delta epsilon zeta",
});

auto words = note.split_whitespace();
std::cout << "split_whitespace tokens:";
for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
  std::cout << " [" << words.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_whitespace offsets:";
for (std::int64_t off : words.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_whitespace tokens: [alpha] [beta] [gamma] [delta] [epsilon] [zeta]
split_whitespace offsets: 0 3 6
```

Example 2. Use the `limit` overload when you want only the first whitespace
cut per cell and want the remainder to stay grouped.

```cpp
auto note = pluss::make_note({
    "alpha beta gamma",
    "delta epsilon",
});
auto words = note.split_whitespace(1);
std::cout << "split_whitespace_limit1 tokens:";
for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
  std::cout << " [" << words.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_whitespace_limit1 offsets:";
for (std::int64_t off : words.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_whitespace_limit1 tokens: [alpha] [beta gamma] [delta] [epsilon]
split_whitespace_limit1 offsets: 0 2 4
```

Example 3. Clean the boundaries first when leading and trailing padding should
not leak into the token stream.

```cpp
auto note = pluss::make_note({"  Ada Lovelace  ", "\tGrace Hopper\t"});
auto cleaned = note.strip();
auto words = cleaned.split_whitespace();
std::cout << "split_whitespace_strip tokens:";
for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
  std::cout << " [" << words.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_whitespace_strip offsets:";
for (std::int64_t off : words.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_whitespace_strip tokens: [Ada] [Lovelace] [Grace] [Hopper]
split_whitespace_strip offsets: 0 2 4
```

## Notes

- `offsets()` mark the token boundaries per input cell.
- In the example above, the first row contributes tokens `[0, 3)` and the
  second row contributes `[3, 6)`.
- This method is a structural splitter, not a linguistic parser. If you need
  richer downstream semantics, it can serve as the first pass before more
  specialized NLP methods.

## See Also

- [`Note::split_lines`](split_lines.md): preserve line structure instead of
  tokenizing by whitespace.
- [`Note::regex_findall`](regex_findall.md): extract only the pieces that match
  a regex rather than every whitespace-separated token.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_lines.html">
    <span class="label">Previous</span>
    <span class="title">split_lines</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
