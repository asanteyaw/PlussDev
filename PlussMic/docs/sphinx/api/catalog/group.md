<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14d0032da3e0 -->

# Catalog::group

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::group() const -> CatalogGroupBuilder
```

## What It Does

The entry point into `Catalog`'s group-by-key-stream split. `group()` itself binds the
current catalog and hands back a `CatalogGroupBuilder`, whose single method actually does
the work:

### `CatalogGroupBuilder` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `by` | Method | `std::unordered_map<std::string, Catalog> by(std::string_view key_field) const` |

`.by(key_field)` reads one named stream as the grouping key and partitions the whole
catalog into one **complete sub-`Catalog`** per distinct key value — each sub-catalog keeps
every stream (including the grouping key itself), trimmed down to just the rows belonging
to that group.

```{important} The result is an unordered_map — iteration order is not guaranteed
`by()` returns `std::unordered_map<std::string, Catalog>`, not an ordered container. Don't
rely on groups coming back in first-occurrence or sorted order; look them up by key
(`groups.at("east")` or `groups["east"]`) or accept that iteration order is unspecified.
```

## Parameters

`None` (the grouping key is supplied to the returned builder's `.by(key_field)`, not to
`group()` itself).

## Returns

`CatalogGroupBuilder`
    A binder for the current catalog — call `.by(key_field)` on it to actually split into
    groups.

## Examples

Example 1. Split a catalog into one sub-catalog per distinct value of a `Note`-kind key
stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "west"})),
    "sales"_k  = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0})),
});

auto groups = ct.group().by("region");
std::cout << "groups: " << groups.size() << "\n";
std::cout << groups.at("east");
```

### Output

```text
groups: 2
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, sales]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 2       | [east, east]                                     
sales                    | Series       | Series[float64]  | 2       | [10, 30]                                         
========================================================================================================================
```

Every stream — including `region`, the grouping key itself — is carried into each
sub-catalog, restricted to that group's rows.

Example 2. An unknown or empty key field throws immediately, before any splitting happens.

```cpp
auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west"})),
    "sales"_k  = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});

try {
  auto out = ct.group().by("nope");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
try {
  auto out = ct.group().by("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::group().by: unknown key 'nope'
Catalog::group().by: key field cannot be empty
```

## Notes

- `group()` alone does no work — it only binds the catalog; `.by(key_field)` performs the
  actual split.
- The returned `std::unordered_map` has unspecified iteration order — see the important
  callout above.
- Every sub-catalog keeps the *full* stream set, including the grouping key, just
  restricted to its own rows — this is a row-wise partition, not a projection down to a
  subset of streams.
- This is a first-cut split into whole sub-catalogs, not an aggregation pipeline — for
  per-group summary statistics on a single stream, see [`sum`](sum.md)/[`mean`](mean.md)/
  etc., applied to each group's `Stream` after splitting.

## See Also

- [`Catalog::keys`](keys.md): select a subset of *streams* rather than partitioning rows by
  a key value.
- [`Catalog::has`](has.md): check whether a key field exists before grouping by it, to
  avoid the unknown-key throw.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
