<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76c25b5d84d4 -->

# Stream::shape

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::shape() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Returns `(size(), 1)` — the row count paired with a fixed column count of `1`, since a
`Stream` is always exactly one column. The second element never varies; it exists so
`shape()` reads consistently alongside `Catalog::shape()` (`(nrecords, nkeys)`), where the
second element is meaningful.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `{row_count, 1}`.

## Examples

Example 1. Shape of a plain numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto [rows, cols] = stream.shape();
std::cout << "(" << rows << ", " << cols << ")";
```

### Output

```text
(3, 1)
```

Example 2. The second element is always `1`, regardless of payload kind or size.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"a", "b", "c", "d", "e"}));

auto [rows, cols] = stream.shape();
std::cout << "(" << rows << ", " << cols << ")";
```

### Output

```text
(5, 1)
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` otherwise.
- The first element always equals [`size()`](size.md)/[`nrecords()`](nrecords.md); the
  second is always `1`.

## See Also

- [`Stream::size`](size.md) / [`Stream::nrecords`](nrecords.md): the row count alone, as a
  scalar rather than a pair.
- [`Catalog::shape`](../catalog/shape.md): the collection-level analog, where the second
  element (key count) actually varies.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
