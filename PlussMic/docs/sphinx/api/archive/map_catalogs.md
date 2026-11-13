<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 51bbcdb9779d -->

# Archive::map_catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::map_catalogs(const std::function<Catalog(const Catalog&)>& fn) const
```

## What It Does

Applies `fn` to every catalog in the archive independently and returns a new archive built
from the results — depth-preserving (one output catalog per input catalog, same order) and
depth-label-preserving (any [`label()`](label.md) set on the receiver is carried over
unchanged onto the result).

## Parameters

**fn** (`const std::function<Catalog(const Catalog&)>&`)
    A callback receiving one catalog and returning its replacement. Must not be empty.

## Returns

`Archive`
    A new archive with every catalog replaced by `fn`'s result, same depth and depth
    labels as the receiver.

## Examples

Example 1. Apply the same per-catalog transform (here, `head(1)`) across the whole archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.map_catalogs([](const pluss::kindle::Catalog& c) { return c.head(1); });
std::cout << "out depth: " << out.depth() << "\n";
std::cout << "out.catalog(0).nrecords(): " << out.catalog(0).nrecords() << "\n";
std::cout << "depth labels preserved: " << out.label().has();
```

### Output

```text
out depth: 2
out.catalog(0).nrecords(): 1
depth labels preserved: 1
```

Example 2. An empty `std::function` throws immediately.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto archive = pluss::kindle::Archive({c0});

try {
  std::function<pluss::kindle::Catalog(const pluss::kindle::Catalog&)> empty_fn;
  archive.map_catalogs(empty_fn);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Archive::map_catalogs: mapper function is empty
```

## Notes

- Depth-preserving and depth-label-preserving — the output has the same depth and the same
  depth labels as the receiver; only each catalog's own content changes.
- `fn` must not be empty; throws `"Archive::map_catalogs: mapper function is empty"`
  otherwise — see Example 2.
- Nothing prevents `fn` from returning catalogs with different keys or record counts per
  call, which produces a ragged archive — see [`minkeys()`](minkeys.md)/
  [`maxkeys()`](maxkeys.md)/[`minrecords()`](minrecords.md)/[`maxrecords()`](maxrecords.md)
  for how to inspect that afterward.

## See Also

- [`Archive::aggregate_over_catalogs`](aggregate_over_catalogs.md): reduce every catalog
  down to a single `Catalog`, instead of transforming each one independently.
- [`Archive::keys`](keys.md): scope a transform to specific streams within every catalog,
  instead of a full per-catalog callback.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="loc.html">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="maxkeys.html">
    <span class="label">Next</span>
    <span class="title">maxkeys</span>
  </a>
</div>
