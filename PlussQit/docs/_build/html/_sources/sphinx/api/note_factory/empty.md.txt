<!-- pluss-hand-authored -->
# pluss::note::empty

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::empty(integer_range sizes)
```

## A Shape Pre-Filled With The Empty String

`empty()` allocates a `Note` of shape `sizes` with every cell holding `""`.
It is defined as `full(sizes, "")` and shares that function's contract
exactly — only the fill value is fixed.

Unlike numeric `empty` factories in some libraries, this one does **not**
leave cells uninitialised: text cells have no meaningful "uninitialised"
state, so every cell is a real, valid empty string. The name follows the
`series`/`timeline` family for consistency; read it as "empty *contents*,
known shape", not "empty of cells".

Use it to stage a block you will fill in afterwards — a results grid whose
rows are computed one at a time, a fixed-width table you will populate
column by column, a scratch buffer of the right geometry. Because every cell
already satisfies the "is text" contract, the `Note` is immediately usable
with every `Note` method; unwritten cells simply read as `""`.

`sizes` follows the same rules as [`full`](full.md): any contiguous range of
`std::int64_t`, an empty range for a scalar, non-negative extents, zero
extents allowed, rank at most three, validated before allocation. The result
is **UTF-8 mode**.

### `empty` versus a zero-length `Note`

These are different things:

- `empty({2, 2})` — four cells, each `""`.
- `full({0}, "x")` — a rank-`1` `Note` with *no* cells.

`empty` never removes cells; it only chooses their starting value.

## Example 1: A Matrix Of Empty Strings

```cpp
std::array<std::int64_t, 2> sizes{2, 2};
const pluss::Note n = pluss::note::empty(
    pluss::msharp::RangeView::Of<std::int64_t>(sizes));
std::cout << pluss::note::print(n) << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", ""], 
  ["", ""]
]
```

Four addressable cells, all `""`, ready to be overwritten.

## Example 2: A Scalar Empty String

```cpp
std::array<std::int64_t, 0> sizes{};
const pluss::Note n = pluss::note::empty(
    pluss::msharp::RangeView::Of<std::int64_t>(sizes));
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
""
```

The same value `pluss::note::make("")` builds, reached shape-first.

## See Also

- [`pluss::note::full`](full.md): the general form — choose the fill value.
- [`pluss::note::empty_like`](empty_like.md): take the shape from an
  existing `Note`.
- [`pluss::note::empty_bytes_like`](empty_bytes_like.md): the bytes-mode
  counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bytes.html">
    <span class="label">Previous</span>
    <span class="title">bytes</span>
  </a>
  <a class="next-link" href="empty_bytes_like.html">
    <span class="label">Next</span>
    <span class="title">empty_bytes_like</span>
  </a>
</div>
