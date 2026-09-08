<!-- pluss-hand-authored -->
# pluss::note::empty_like

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::empty_like(const Note& like)
```

## Another `Note`'s Shape, Filled With The Empty String

`empty_like()` returns a fresh `Note` shaped exactly like `like`, with every
cell holding `""`. It is `full_like(like, "")`.

Use it to stage a same-shaped scratch buffer next to a `Note` you already
have — a place to write transformed rows without disturbing the source, an
output grid congruent with the input, a reset copy. Every cell is a valid
empty string, so the result works with every `Note` method immediately;
cells you have not written yet simply read as `""`.

Only the **shape** of `like` is used. Its contents and its encoding are
ignored — `empty_like` always produces a **UTF-8-mode** `Note`. For a
bytes-mode result, use [`empty_bytes_like`](empty_bytes_like.md).

`like` must be a defined `Note`. The result is independent of `like`.

## Example 1: A Same-Shaped Blank Buffer

```cpp
const auto grid = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
const pluss::Note blank = pluss::note::empty_like(grid);
std::cout << blank << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["", "", ""], 
  ["", "", ""]
]
```

Shape `[2, 3]` borrowed from `grid`; every cell reset to `""`.

## See Also

- [`pluss::note::empty`](empty.md): supply the shape as an explicit `sizes`
  list instead.
- [`pluss::note::full_like`](full_like.md): choose a non-empty fill value.
- [`pluss::note::empty_bytes_like`](empty_bytes_like.md): the bytes-mode
  counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty_bytes_like.html">
    <span class="label">Previous</span>
    <span class="title">empty_bytes_like</span>
  </a>
  <a class="next-link" href="format.html">
    <span class="label">Next</span>
    <span class="title">format</span>
  </a>
</div>
