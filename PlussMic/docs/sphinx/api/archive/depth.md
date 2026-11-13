<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e8678436b11 -->

# Archive::depth

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
std::int64_t Archive::depth() const
```

## What It Does

Returns the number of catalogs stacked in the archive — the archive's own dimension,
distinct from any individual catalog's key or record count. An exact synonym for
[`size()`](size.md).

## Parameters

`None`

## Returns

`std::int64_t`
    The number of catalogs.

## Examples

Example 1. Depth of a two-catalog archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})),
});
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.depth();
```

### Output

```text
2
```

Example 2. An archive built with zero catalogs has depth `0` — and is still
[`defined()`](defined.md).

```cpp
auto archive = pluss::kindle::Archive(std::vector<pluss::kindle::Catalog>{});

std::cout << "depth: " << archive.depth() << "\n";
std::cout << "defined: " << archive.defined();
```

### Output

```text
depth: 0
defined: 1
```

## Notes

- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Always equal to [`size()`](size.md) and to [`shape()`](shape.md)'s first element.

## See Also

- [`Archive::size`](size.md): the exact synonym for this method.
- [`Archive::shape`](shape.md): returns `(depth(), nrecords(), nkeys())` as a tuple.
- [`Archive::empty`](empty.md): `depth() == 0`, as a direct boolean check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
