<!-- pluss-hand-authored -->
# Note::keep_printable

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::keep_printable() const
```

## Keep Only The Visible ASCII Range In Every Cell

`keep_printable()` returns a new `Note`, same shape as the input, keeping
only bytes in the printable ASCII range `0x20`–`0x7E` (space through `~`).
Everything else is deleted: **all ASCII control bytes — including tab
(`\t`) and newline (`\n`) — and every non-ASCII byte**. Use it to sanitize
text for a single-line log field, a terminal display, or a format that
rejects control characters.

Note that tab and newline are removed, not converted to spaces, so
`"line1\tcol2\nline2"` collapses to `"line1col2line2"`. If you need to keep
line structure, split first ([`split_lines`](split_lines.md)) or normalise
whitespace before calling this.

`keep_printable` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Control Bytes Are Removed

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"line1\tcol2\nline2", "clean"}).keep_printable())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["line1col2line2", "clean"]
```

## Example 2: Non-ASCII Is Removed Too

```cpp
const pluss::Note n = pluss::make_note({"café — ok"}).keep_printable();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["caf  ok"]
```

The `é` and the em dash are non-ASCII, so their bytes are dropped, leaving
the surrounding spaces.

## See Also

- [`Note::keep_ascii`](keep_ascii.md): keeps ASCII **control** bytes; only
  non-ASCII is dropped.
- `pluss::coden::Note::remove_control_chars`: drop controls but keep
  non-ASCII text.
- [`Note::remove_symbols`](remove_symbols.md): keep letters, digits, and
  whitespace (including non-ASCII).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_digits.html">
    <span class="label">Previous</span>
    <span class="title">keep_digits</span>
  </a>
  <a class="next-link" href="lengths_bytes.html">
    <span class="label">Next</span>
    <span class="title">lengths_bytes</span>
  </a>
</div>
