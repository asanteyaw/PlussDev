# Note::split_lines

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::split_lines() const -> TokenBatch
Note::split_lines(bool keep_ends) const -> TokenBatch
```

## What It Does

Splits each text cell into line tokens.

Use this when your `Note` elements are multi-line content such as prompts,
templates, markdown fragments, copied terminal output, or document sections and
you want explicit line-level structure.

Like the rest of the token-batch family, this returns a flattened token stream
plus `offsets()`. The flattening is not a loss of structure; it is the library
surface used to preserve ragged per-cell token counts without forcing a padded
2D text object.

## Parameters

**keep_ends**

Controls whether line terminators remain attached to each returned line token.

- `split_lines()`
  Uses the default behavior, which discards the newline markers and returns the
  logical line contents only.
- `split_lines(true)`
  Preserves the original line-ending characters on the tokens that ended with
  them, which is useful when you need closer fidelity to the serialized source.

This matters when you want to preserve the original serialized shape of the
text, not just the logical line contents.

## Returns

`TokenBatch`
    A flat line token stream plus `offsets()` describing which lines came from
    each original `Note` cell.

## Examples

Example 1. Split text into logical lines and discard the newline markers.

```cpp
auto note = pluss::make_note({"line1\nline2\n", "solo"});
auto lines = note.split_lines();
std::cout << "split_lines tokens:";
for (std::int64_t i = 0; i < lines.tokens().numel(); ++i) {
  std::cout << " [" << lines.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_lines offsets:";
for (std::int64_t off : lines.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_lines tokens: [line1] [line2] [solo]
split_lines offsets: 0 2 3
```

Example 2. Keep the line endings when you need to preserve serialized structure.

```cpp
auto note = pluss::make_note({"line1\nline2\n", "solo"});
auto lines = note.split_lines(true);
std::cout << "split_lines_keep tokens:";
for (std::int64_t i = 0; i < lines.tokens().numel(); ++i) {
  std::cout << " [" << lines.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "split_lines_keep offsets:";
for (std::int64_t off : lines.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
split_lines_keep tokens: [line1
] [line2
] [solo]
split_lines_keep offsets: 0 2 3
```

## Notes

- `offsets()` still map the flat token stream back to the original cases.
- In the first example, the first cell contributes two lines and the second
  contributes one, which is why the offsets are `0 2 3`.
- `split_lines()` is usually the right starting point before line-level cleanup,
  filtering, or diff work.

## See Also

- [`Note::split_whitespace`](split_whitespace.md): tokenize by whitespace rather
  than line boundaries.
- [`Note::regex_findall`](regex_findall.md): extract pattern matches from the
  original text instead of structural lines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="split_whitespace.html">
    <span class="label">Next</span>
    <span class="title">split_whitespace</span>
  </a>
</div>
