<!-- pluss-hand-authored -->
# pluss::note::full_like

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::full_like(const Note& like, string_view value)
```

## Borrow Another `Note`'s Shape, Choose A New Value

`full_like()` reads the shape of an existing `Note` and returns a fresh
`Note` of that same shape with `value` in every cell. It is `full` with the
`sizes` list replaced by "whatever shape `like` has".

Use it whenever you already hold a `Note` whose geometry you want to match —
a mask the same size as a text column, a placeholder grid congruent with a
parsed table, a reset buffer shaped like the input. Taking the shape from
`like` keeps the two in lock-step even if the source shape changes, and
avoids restating extents that are only correct by coincidence.

Only the **shape** is copied. The contents of `like` are ignored; its
encoding is ignored too — `full_like` always produces a **UTF-8-mode**
`Note`. For a bytes-mode result with a borrowed shape, use
[`full_bytes_like`](full_bytes_like.md).

`like` must be a defined `Note` (any that the factory functions return
qualifies). The result is a new, independent `Note`; later edits to it do
not touch `like`, and vice versa.

## Example 1: A Placeholder Grid Congruent With Another `Note`

```cpp
const auto grid = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(pluss::note::full_like(grid, "?")) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["?", "?", "?"], 
  ["?", "?", "?"]
]
```

Shape `[2, 3]` came from `grid`; every cell is the new value `"?"`, and
`grid` itself is untouched.

## See Also

- [`pluss::note::full`](full.md): supply the shape as an explicit `sizes`
  list instead.
- [`pluss::note::empty_like`](empty_like.md): `full_like(like, "")`.
- [`pluss::note::full_bytes_like`](full_bytes_like.md): the bytes-mode
  counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="full_bytes_like.html">
    <span class="label">Previous</span>
    <span class="title">full_bytes_like</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
