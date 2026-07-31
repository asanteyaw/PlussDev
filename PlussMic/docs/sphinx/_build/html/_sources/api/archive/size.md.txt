<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Archive::size

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::size() const -> std::int64_t
```

## What It Does

Returns the number of catalogs stacked in the archive. An exact synonym for
[`depth()`](depth.md) — the two always return the same value; `size()` exists so call
sites can pick the name that reads better.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of catalogs, identical to [`depth()`](depth.md).

## Examples

Example 1. `size()` and `depth()` always agree.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})),
});
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << "size: " << archive.size() << "  depth: " << archive.depth();
```

### Output

```text
size: 2  depth: 2
```

## Notes

- Requires a defined archive; throws `"Archive: undefined handle"` otherwise, same as
  [`depth()`](depth.md).
- Always equal to [`depth()`](depth.md).

## See Also

- [`Archive::depth`](depth.md): the exact synonym for this method.
- [`Archive::shape`](shape.md): returns `(size(), nrecords(), nkeys())` as a tuple.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice_depth.html">
    <span class="label">Next</span>
    <span class="title">slice_depth</span>
  </a>
</div>
