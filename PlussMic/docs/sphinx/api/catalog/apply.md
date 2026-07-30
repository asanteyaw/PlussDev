<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 12b12eb869b8 -->

# Catalog::apply

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::apply(std::string_view field, const std::function<Stream(const Stream&)>& func) const -> Catalog
```

## What It Does

Applies a user function to one named stream, non-mutating — the returned stream replaces
the original in a new `Catalog`; every other stream is unchanged. This is one of four
closely related methods that only differ in **scope** (one stream vs. every stream) and
**mutation** (returns a new catalog vs. rewrites in place):

| | one stream | every stream |
|---|---|---|
| **new catalog** | `apply(field, fn)` | `apply_all(fn)` / `transform(fn)` |
| **in place** | `apply_(field, fn)` | `apply_all_(fn)` |

`Catalog::transform(field, fn)` (the two-argument overload) is also a pure alias for this
exact method — see [`transform()`](transform.md).

```{important} func must return the same length, and the field must already exist
Unlike Catalog::set(), which silently adds a new key, apply() requires field to already
exist — an unknown name throws immediately. The callback's returned stream must also be
defined and exactly the same length as the input stream; a length mismatch throws too.
```

## Parameters

**field** (`std::string_view`)
    The stream name to transform. Must already exist.

**func** (`const std::function<Stream(const Stream&)>&`)
    Called once with the current stream at `field`; must return a defined `Stream` of the
    same length.

## Returns

`Catalog`
    A new catalog with `field`'s stream replaced by `func`'s result; every other stream
    unchanged.

## Examples

Example 1. Double one numeric stream, leaving the rest of the catalog untouched.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.apply("id", [](const pluss::kindle::Stream& s) {
  return pluss::kindle::Stream(s.as_series() * 2.0);
});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [2, 4, 6]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

Example 2. An unknown field name throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  auto out = ct.apply("zzz", [](const pluss::kindle::Stream& s) { return s; });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::apply: unknown key 'zzz'
```

Example 3. Returning a stream of a different length throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  auto out = ct.apply("id", [](const pluss::kindle::Stream&) {
    return pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));
  });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::apply_: output stream length must match input stream length
```

The message is prefixed `apply_` (not `apply`) because `apply()` delegates directly to
[`apply_()`](apply_.md) internally.

## Notes

- Requires `field` to already exist — see the important callout above.
- `func`'s returned stream must be defined and the same length as the input.
- `Catalog::transform(field, fn)` (two-argument form) is a pure alias for this method.
- Delegates internally to [`apply_()`](apply_.md), which is why its throw messages carry
  the `apply_` prefix.

## See Also

- [`Catalog::apply_`](apply_.md): the in-place counterpart this method delegates to.
- [`Catalog::apply_all`](apply_all.md) / [`Catalog::transform`](transform.md): the
  whole-catalog counterpart.
- [`Catalog::set`](set.md): write a stream directly, without a transform callback — and
  unlike `apply()`, tolerates a new key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_text_stats.html">
    <span class="label">Previous</span>
    <span class="title">aggregate_text_stats</span>
  </a>
  <a class="next-link" href="apply_.html">
    <span class="label">Next</span>
    <span class="title">apply_</span>
  </a>
</div>
