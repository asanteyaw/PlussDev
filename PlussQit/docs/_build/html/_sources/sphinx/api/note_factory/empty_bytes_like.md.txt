<!-- pluss-hand-authored -->
# pluss::note::empty_bytes_like

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::empty_bytes_like(const Note& like)
```

## Another `Note`'s Shape, Empty, In Bytes Mode

`empty_bytes_like()` returns a fresh **bytes-mode** `Note` (`encoding=bytes`)
shaped exactly like `like`, with every cell holding the empty byte string.
It is `full_bytes_like(like, "")`.

Use it to stage a same-shaped bytes buffer beside a `Note` you already have
— somewhere to assemble binary output row by row, a bytes-mode reset copy,
an accumulator congruent with a text input. Every cell is a valid
zero-length byte string, so the result is immediately usable; unwritten
cells read as empty.

Only the **shape** of `like` is read; its contents and its own encoding do
not matter. The result is always bytes mode. For a UTF-8-mode blank buffer,
use [`empty_like`](empty_like.md).

`like` must be a defined `Note`. The result is independent of `like`.

## Example 1: A Same-Shaped Empty Bytes Buffer

```cpp
const auto grid = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
const pluss::Note buf = pluss::note::empty_bytes_like(grid);
pluss::note::print(std::cout, buf) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=bytes)
[
  [b"", b"", b""], 
  [b"", b"", b""]
]
```

Shape `[2, 3]` borrowed from `grid`; every cell an empty byte string, and
the `Note` is in bytes mode (`b""`, not `""`).

## See Also

- [`pluss::note::empty_like`](empty_like.md): the UTF-8-mode counterpart.
- [`pluss::note::full_bytes_like`](full_bytes_like.md): choose a non-empty
  byte fill.
- [`pluss::note::bytes`](bytes.md): build bytes mode directly from raw
  content.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="empty_like.html">
    <span class="label">Next</span>
    <span class="title">empty_like</span>
  </a>
</div>
