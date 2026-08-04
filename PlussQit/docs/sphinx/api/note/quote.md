# Note::quote

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::quote() const -> Note
```

## What It Does

Wraps each text cell in a quoting pair.

This is a small method, but it is useful in practice when you want explicit
quoted rendering for export, debugging, prompt construction, or downstream
serialization where boundaries should be visually obvious.

`quote()` is presentation-oriented. It does not tokenize, escape, or inspect
the text. It simply surrounds each cell with a quote wrapper.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A new `Note` with the same shape as the input and each cell wrapped in
    quotes.

## Examples

Example 1. Quote two text cells for display-safe rendering.

```cpp
auto note = pluss::make_note({"alpha beta beta", "already"});
auto quoted = note.quote();
std::cout << "quote:\n";
for (std::int64_t i = 0; i < quoted.numel(); ++i) {
  std::cout << "  [" << i << "] " << quoted.at(i) << "\n";
}
```

### Output

```text
quote:
  [0] "alpha beta beta"
  [1] "already"
```

Example 2. Quote after cleanup when the input still has padded boundaries.

```cpp
auto note = pluss::make_note({"  spaced  ", ""});
auto cleaned = note.strip();
auto quoted = cleaned.quote();
std::cout << "quote_strip:\n";
for (std::int64_t i = 0; i < quoted.numel(); ++i) {
  std::cout << "  [" << i << "] " << quoted.at(i) << "\n";
}
```

### Output

```text
quote_strip:
  [0] "spaced"
  [1] ""
```

Example 3. Quote after a structural rewrite when you want the final rendered
surface to make cell boundaries obvious.

```cpp
auto note = pluss::make_note({"Ada_Lovelace", "Grace_Hopper"});
auto display = note.replace("_", " ").quote();
std::cout << "quote_display:\n";
for (std::int64_t i = 0; i < display.numel(); ++i) {
  std::cout << "  [" << i << "] " << display.at(i) << "\n";
}
```

### Output

```text
quote_display:
  [0] "Ada Lovelace"
  [1] "Grace Hopper"
```

## Notes

- `quote()` is shape-preserving and purely textual.
- This is a formatting transform, not a tokenizer or parser.
- If your goal is TeX-safe rendering rather than ordinary quoted rendering, use
  [`tex_escape`](tex_escape.md).

## See Also

- [`Note::replace`](replace.md): custom formatting rewrites beyond simple
  quoting.
- [`Note::tex_escape`](tex_escape.md): escape special TeX characters rather than
  adding ordinary quotes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_all.html">
    <span class="label">Previous</span>
    <span class="title">remove_all</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
