<!-- pluss-hand-authored -->
# pluss::note::full_bytes_like

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::full_bytes_like(const Note& like, string_view value)
```

## Borrow A Shape, Fill It In Bytes Mode

`full_bytes_like()` is [`full_like`](full_like.md) with a bytes-mode result:
it takes the shape of an existing `Note` and returns a fresh **bytes-mode**
`Note` (`encoding=bytes`) of that shape, with `value` copied into every
cell.

This is the shape-first way into bytes mode. There is deliberately no
free-standing `full_bytes(sizes, value)` factory — when you want a bytes
block of a known geometry, you almost always have a `Note` of that geometry
already (the text you are about to re-tag, mask, or pair with a binary
column), so the shape is borrowed rather than restated. If you truly have
only a `sizes` list, pass it to [`bytes`](bytes.md) with a flat cell range.

As with `full_like`, only the shape of `like` is read; its contents and its
own encoding do not matter. `value` is stored as raw bytes with no UTF-8
validation — that is the whole point of bytes mode. Print the result with
`opt.bytes_mode = pluss::NotePrintBytesMode::Hex` when the content is not
mostly printable ASCII.

`like` must be a defined `Note`. The result is independent of `like`.

## Example 1: A Bytes Block Shaped Like An Existing `Note`

```cpp
pluss::NotePrintOptions hex;
hex.bytes_mode = pluss::NotePrintBytesMode::Hex;

const auto grid = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(pluss::note::full_bytes_like(grid, "\xDE\xAD"), hex)
          << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=bytes)
[
  [hex[de ad], hex[de ad], hex[de ad]], 
  [hex[de ad], hex[de ad], hex[de ad]]
]
```

Shape `[2, 3]` came from `grid`; every cell holds the two raw bytes
`DE AD`, and the `Note` is in bytes mode.

## See Also

- [`pluss::note::full_like`](full_like.md): the UTF-8-mode counterpart.
- [`pluss::note::empty_bytes_like`](empty_bytes_like.md):
  `full_bytes_like(like, "")`.
- [`pluss::note::bytes`](bytes.md): build bytes mode directly from raw
  content or a flat range plus a `sizes` list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="full.html">
    <span class="label">Previous</span>
    <span class="title">full</span>
  </a>
  <a class="next-link" href="full_like.html">
    <span class="label">Next</span>
    <span class="title">full_like</span>
  </a>
</div>
