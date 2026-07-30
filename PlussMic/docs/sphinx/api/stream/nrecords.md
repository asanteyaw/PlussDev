<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62bf86d0fb06 -->

# Stream::nrecords

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::nrecords() const -> std::int64_t
```

## What It Does

Returns the number of rows in the stream. An exact synonym for [`size()`](size.md) — the
two always return the same value; `nrecords()` exists so call sites can pick the name that
reads better ("how many records" vs. "how big").

## Parameters

`None`

## Returns

`std::int64_t`
    The row count, identical to [`size()`](size.md).

## Examples

Example 1. Row count of a plain numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.nrecords();
```

### Output

```text
3
```

Example 2. `nrecords()` and `size()` always agree.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << "nrecords: " << stream.nrecords() << "  size: " << stream.size();
```

### Output

```text
nrecords: 5  size: 5
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` otherwise, same as [`size()`](size.md).
- Always equal to [`size()`](size.md) and to [`shape()`](shape.md)'s first element.

## See Also

- [`Stream::size`](size.md): the exact synonym for this method.
- [`Stream::shape`](shape.md): returns `(nrecords(), 1)` as a pair.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="ptype.html">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>
