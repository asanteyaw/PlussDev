<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ff0686f0323f -->

# Archive::catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
const std::vector<Catalog>& Archive::catalogs() const
```

## What It Does

Returns every catalog in the archive, in depth order, as a `const std::vector<Catalog>&` —
no copy is made. Use this when you want to iterate the whole archive directly with standard
`<algorithm>`/range-`for` code instead of going through [`catalog(i)`](catalog.md) one
index at a time.

## Parameters

`None`

## Returns

`const std::vector<Catalog>&`
    Every catalog in the archive, in depth order.

## Examples

Example 1. Iterate every catalog directly.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0, 5.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

const auto& cats = archive.catalogs();
std::cout << "count: " << cats.size() << "\n";
for (const auto& cat : cats) {
  std::cout << "nrecords: " << cat.nrecords() << "\n";
}
```

### Output

```text
count: 2
nrecords: 2
nrecords: 3
```

## Notes

- Returns by `const&` — no copy of the vector or its catalogs is made just to read it.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::catalog`](catalog.md) / [`Archive::dloc`](dloc.md): fetch one catalog by
  depth-index instead of the whole vector.
- [`Archive::depth`](depth.md): the length of the vector this method returns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="catalog.html">
    <span class="label">Previous</span>
    <span class="title">catalog</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
