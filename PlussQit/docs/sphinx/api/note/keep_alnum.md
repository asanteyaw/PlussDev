<!-- pluss-hand-authored -->
# Note::keep_alnum

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::keep_alnum() const
```

## Keep Only ASCII Letters And Digits In Every Cell

`keep_alnum()` returns a new `Note`, same shape as the input, with every
character that is **not** an ASCII letter or digit deleted from each cell —
including whitespace, punctuation, and all non-ASCII bytes. Use it to
collapse a value to a bare alphanumeric key for comparison, hashing, or
slug-like identifiers.

This is stricter than [`remove_punctuation`](remove_punctuation.md) /
[`remove_symbols`](remove_symbols.md): those keep spaces and non-ASCII;
`keep_alnum` keeps neither. A non-ASCII letter (`é`, `本`) is dropped along
with its accent, so `"café"` becomes `"caf"`.

`keep_alnum` is shape-preserving, cell-independent, and returns a fresh
`Note`. A cell with no ASCII alphanumerics becomes `""`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Hello, World! 42", "  x-y  ", "café"}).keep_alnum())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["HelloWorld42", "xy", "caf"]
```

Spaces, the comma, the exclamation mark, the hyphens, and the `é` are all
gone.

## See Also

- [`Note::keep_alpha`](keep_alpha.md) / [`Note::keep_digits`](keep_digits.md):
  narrower — one class only.
- [`Note::remove_punctuation`](remove_punctuation.md): keeps whitespace and
  non-ASCII.
- [`Note::keep_ascii`](keep_ascii.md): keeps every ASCII byte, punctuation
  and whitespace included.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="json_escape.html">
    <span class="label">Previous</span>
    <span class="title">json_escape</span>
  </a>
  <a class="next-link" href="keep_alpha.html">
    <span class="label">Next</span>
    <span class="title">keep_alpha</span>
  </a>
</div>
