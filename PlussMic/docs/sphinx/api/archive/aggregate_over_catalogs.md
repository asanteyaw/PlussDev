<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c1160e6e783 -->

# Archive::aggregate_over_catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) const -> Catalog
```

## What It Does

Reduces the **entire** archive down to a single `Catalog` by handing `fn` every catalog at
once (as a `const std::vector<Catalog>&`) and returning whatever `fn` builds — distinct
from [`map_catalogs()`](map_catalogs.md), which transforms each catalog independently and
keeps the archive's own depth.

## Parameters

**fn** (`const std::function<Catalog(const std::vector<Catalog>&)>&`)
    A callback receiving every catalog in the archive at once, returning a single `Catalog`.
    Must not be empty.

## Returns

`Catalog`
    Whatever `fn` returns — the archive collapses to this one catalog.

## Examples

Example 1. Sum a stream across every catalog into a single-row summary catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "sales"_k = pluss::kindle::Stream(pluss::make_series({100.0, 200.0})) });
auto c1 = pluss::kindle::Catalog({ "sales"_k = pluss::kindle::Stream(pluss::make_series({300.0, 400.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"jan", "feb"})));

auto total = archive.aggregate_over_catalogs([](const std::vector<pluss::kindle::Catalog>& cats) {
  double sum = 0.0;
  for (const auto& c : cats) {
    auto s = c.get("sales").value().sum().as_series().to_vec<double>();
    sum += s[0];
  }
  return pluss::kindle::Catalog({
      "total_sales"_k = pluss::kindle::Stream(pluss::make_series({sum})),
  });
});
std::cout << total;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [total_sales]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
total_sales              | Series       | Series[float64]  | 1       | [1000]                                          
========================================================================================================================
```

Example 2. An empty `std::function` throws immediately.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto archive = pluss::kindle::Archive({c0});

try {
  std::function<pluss::kindle::Catalog(const std::vector<pluss::kindle::Catalog>&)> empty_fn;
  archive.aggregate_over_catalogs(empty_fn);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Archive::aggregate_over_catalogs: reducer function is empty
```

## Notes

- Collapses the whole archive to a single `Catalog` — not depth-preserving, unlike
  [`map_catalogs()`](map_catalogs.md).
- `fn` must not be empty; throws `"Archive::aggregate_over_catalogs: reducer function is
  empty"` otherwise — see Example 2.
- `fn` receives every catalog, including from a ragged archive — it's responsible for
  handling any per-catalog key/record-count differences itself.

## See Also

- [`Archive::map_catalogs`](map_catalogs.md): transform each catalog independently,
  preserving depth, instead of reducing to one.
- [`Catalog::at`](../catalog/at.md) plus [`Stream::sum`](../stream/sum.md): a
  common per-stream reduction path used inside a reducer like the one shown above.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
