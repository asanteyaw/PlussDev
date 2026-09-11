<!-- pluss-hand-authored -->
# Note::strip_bom

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::strip_bom() const
```

## Remove A Leading Byte-Order Mark From Every Cell

`strip_bom()` returns a new `Note`, same shape as the input, with a UTF-8
byte-order mark (U+FEFF, the bytes `EF BB BF`) removed from the **start** of
each cell that begins with one. A BOM commonly sneaks in when reading files
exported by editors or spreadsheets; left in place it breaks a leading-token
check, a numeric parse, or an exact-match lookup on the first field.

Only a BOM at position 0 is removed. A U+FEFF elsewhere in the cell is left
alone — that is a zero-width no-break space, and
[`strip_zero_width`](strip_zero_width.md) is the method that removes those
wherever they occur.

`strip_bom` is shape-preserving, cell-independent, and returns a fresh
`Note`. A cell without a leading BOM is unchanged.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {std::string("\xEF\xBB\xBF") + "hello", "plain",
     std::string("mid") + "\xEF\xBB\xBF" + "bom"}).strip_bom();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["hello", "plain", "mid﻿bom"]
```

The leading BOM is gone from the first cell; the mid-cell U+FEFF in the
third is kept.

## See Also

- [`Note::strip_zero_width`](strip_zero_width.md): remove U+FEFF and other
  zero-width characters **anywhere** in the cell.
- [`Note::strip`](strip.md): trim whitespace from the boundary.
- [`Note::remove_prefix`](remove_prefix.md): remove a known literal prefix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_accents.html">
    <span class="label">Previous</span>
    <span class="title">strip_accents</span>
  </a>
  <a class="next-link" href="strip_zero_width.html">
    <span class="label">Next</span>
    <span class="title">strip_zero_width</span>
  </a>
</div>
