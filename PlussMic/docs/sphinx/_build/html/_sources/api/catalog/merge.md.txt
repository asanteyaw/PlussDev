<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f0bd84044918 -->

# Catalog::merge

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::merge(const Catalog& other, std::string_view on, std::string_view how = "inner") const -> Catalog
```

## What It Does

Joins two catalogs on a shared key stream — internally, both catalogs are
[`tabelize()`](tabelize.md)d into `DataFrame`s, joined via `DataFrame::unify`, and the
result is [`kindelize()`](kindelize.md)d back into a `Catalog`.

```{important} how currently only accepts "inner" — every other value throws
Despite the parameter existing for future flexibility, the current implementation supports
**only** `how = "inner"` (the default). Passing anything else — `"left"`, `"outer"`, or any
other join mode `DataFrame::unify` itself supports — throws immediately, before any joining
happens. See Example 2.
```

## Parameters

**other** (`const Catalog&`)
    The right-hand catalog to join against. Must be defined.

**on** (`std::string_view`)
    The shared key stream name. Must be non-empty, and must exist in **both** catalogs.

**how** (`std::string_view`, default `"inner"`)
    The join mode. Only `"inner"` is currently supported — see the important callout
    above.

## Returns

`Catalog`
    A new catalog with the streams from both inputs, restricted to rows whose `on` value
    matched on both sides.

## Examples

Example 1. Inner-join two catalogs on a shared `id` key — only rows present in both survive.

```cpp
using namespace pluss::kindle::literals;

auto left = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "name"_k = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
});
auto right = pluss::kindle::Catalog({
    "id"_k    = pluss::kindle::Stream(pluss::make_series({2.0, 3.0, 4.0})),
    "score"_k = pluss::kindle::Stream(pluss::make_series({80.0, 90.0, 70.0})),
});

auto out = left.merge(right, "id");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [id, name, score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [2, 3]                                           
name                     | Note         | Note             | 2       | [Bola, Chen]                                     
score                    | Series       | Series[float64]  | 2       | [80, 90]                                         
========================================================================================================================
```

Only `id` values `2` and `3` (present on both sides) survive; `1` (left-only) and `4`
(right-only) are dropped.

Example 2. Requesting any `how` other than `"inner"` throws.

```cpp
auto left = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1.0}))});
auto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1.0}))});

try {
  auto out = left.merge(right, "id", "left");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::merge currently supports only how='inner'
```

Example 3. A key that doesn't exist on one side throws, naming which side.

```cpp
auto left = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1.0}))});
auto right = pluss::kindle::Catalog({"other_id"_k = pluss::kindle::Stream(pluss::make_series({1.0}))});

try {
  auto out = left.merge(right, "id");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::merge: right key not found 'id'
```

## Notes

- Only `how = "inner"` is currently supported — see the important callout above.
- `on` must exist in both catalogs; the throw message names which side ("left" or
  "right") is missing it.
- Implemented via [`tabelize()`](tabelize.md) + `DataFrame::unify` +
  [`kindelize()`](kindelize.md) — a round trip through `DataFrame`, not a direct
  stream-to-stream join.

## See Also

- [`Catalog::tabelize`](tabelize.md) / [`Catalog::kindelize`](kindelize.md): the two
  conversions this method uses internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
