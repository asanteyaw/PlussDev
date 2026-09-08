<!-- pluss-hand-authored -->
# pluss::note::full

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::full(integer_range sizes, string_view value)
```

## Fill A Known Shape With One Repeated String

`full()` is the shape-first constructor: you already know the geometry you
want, and you want every cell to start life holding the same string. It
allocates a `Note` of shape `sizes` and writes `value` into all `numel`
cells.

Reach for it when the shape comes from somewhere other than the text — a
column count fixed by a schema, a grid whose dimensions are computed, a
placeholder block you will overwrite cell by cell later. It is the text
counterpart of `pluss::series::full` and `pluss::timeline::full`: same
argument order (shape, then fill), same "allocate and broadcast one value"
contract.

`sizes` is any contiguous range of `std::int64_t`. Its length is the rank and
its entries are the extents:

- an empty range → a rank-`0` scalar (one cell)
- `{n}` → a rank-`1` column of `n` cells
- `{r, c}` → a rank-`2` block
- `{a, b, c}` → a rank-`3` block

The result is always **UTF-8 mode** (`encoding=utf8`); `value` is stored
verbatim in every cell, with no trimming or re-encoding. For a bytes-mode
fill, there is no free-standing `full_bytes` — build the shape from an
existing `Note` with [`full_bytes_like`](full_bytes_like.md), or pass an
explicit `sizes` list to [`bytes`](bytes.md).

### Relationship to `empty` and `rep`

- [`empty(sizes)`](empty.md) is exactly `full(sizes, "")` — a shape filled
  with the empty string.
- [`rep(value, sizes)`](rep.md) is `full` with the arguments swapped to
  value-first, plus an `initializer_list` overload so `rep("-", {5})` needs
  no `RangeView` wrapper. Use `rep` when the *value* is the point of the
  call and the shape is a small literal; use `full` when the *shape* is the
  point.

### Shape rules

Every extent must be non-negative; a zero extent is allowed and yields a
`Note` with no cells but a retained shape. Rank may not exceed three. Shape
is validated before any allocation.

## Example 1: Fill A Matrix With A Placeholder

```cpp
std::array<std::int64_t, 2> sizes{2, 3};
const pluss::Note n = pluss::note::full(
    pluss::msharp::RangeView::Of<std::int64_t>(sizes), "TODO");
std::cout << pluss::note::print(n) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["TODO", "TODO", "TODO"], 
  ["TODO", "TODO", "TODO"]
]
```

Every cell holds the same string; the shape came entirely from `sizes`.

## Example 2: An Empty `sizes` Range Produces A Scalar

```cpp
std::array<std::int64_t, 0> sizes{};
const pluss::Note n = pluss::note::full(
    pluss::msharp::RangeView::Of<std::int64_t>(sizes), "x");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"x"
```

Rank `0`, one cell — the same scalar a bare `pluss::note::make("x")` builds,
just reached shape-first.

## Example 3: A Zero Extent Is Legal

```cpp
std::array<std::int64_t, 1> sizes{0};
const pluss::Note n = pluss::note::full(
    pluss::msharp::RangeView::Of<std::int64_t>(sizes), "x");
pluss::note::print(std::cout, n) << "\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

The `Note` has no cells but keeps its rank-`1` shape — a valid, typed empty
column, distinct from a scalar holding `""`.

## Example 4: A Rank Above Three Is Rejected

```cpp
std::array<std::int64_t, 4> sizes{2, 2, 2, 2};
try {
  pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes), "x");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Note supports up to 3D
```

## Example 5: A Negative Extent Is Rejected

```cpp
std::array<std::int64_t, 1> sizes{-1};
try {
  pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes), "x");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
sizes must be non-negative
```

Validation happens before allocation.

## See Also

- [`pluss::note::empty`](empty.md): `full(sizes, "")` — a shape of empty
  strings.
- [`pluss::note::rep`](rep.md): the value-first spelling, with an
  `initializer_list` shape overload.
- [`pluss::note::full_like`](full_like.md): take the shape from an existing
  `Note` instead of a `sizes` list.
- [`pluss::note::make`](make.md): build from the text itself when the shape
  is implied by the data.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_words.html">
    <span class="label">Previous</span>
    <span class="title">from_words</span>
  </a>
  <a class="next-link" href="full_bytes_like.html">
    <span class="label">Next</span>
    <span class="title">full_bytes_like</span>
  </a>
</div>
