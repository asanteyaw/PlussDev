<!-- pluss-hand-authored -->
# Note::strip_zero_width

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::strip_zero_width() const
```

## Remove Invisible Zero-Width Characters From Every Cell

`strip_zero_width()` returns a new `Note`, same shape as the input, with
zero-width characters removed from **anywhere** in each cell: the zero-width
space (U+200B), zero-width non-joiner (U+200C), zero-width joiner (U+200D),
the word joiner (U+2060), and the zero-width no-break space / BOM (U+FEFF).

These characters render as nothing but still count as bytes, so they defeat
exact-match lookups, inflate length checks, and can be used to smuggle
markers through text. Stripping them normalises a cell to what a reader
actually sees.

`strip_zero_width` removes the characters wherever they appear — unlike
[`strip_bom`](strip_bom.md), which only removes a U+FEFF at the very start.
It is shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {std::string("a") + "\xE2\x80\x8B" + "b" + "\xEF\xBB\xBF" + "c", "clean"})
    .strip_zero_width();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["abc", "clean"]
```

The zero-width space between `a` and `b` and the U+FEFF between `b` and `c`
are both removed.

## See Also

- [`Note::strip_bom`](strip_bom.md): remove only a leading U+FEFF.
- [`Note::remove_control_chars`](remove_control_chars.md): remove ASCII
  control bytes.
- [`Note::to_nfkc`](to_nfkc.md): fold other invisible variants such as
  non-breaking spaces.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_bom.html">
    <span class="label">Previous</span>
    <span class="title">strip_bom</span>
  </a>
  <a class="next-link" href="surround.html">
    <span class="label">Next</span>
    <span class="title">surround</span>
  </a>
</div>
