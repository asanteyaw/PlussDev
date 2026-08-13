<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e1f7166a8681 -->

# CatalogTimeSeriesAccessor::lag

[Back to CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimeSeriesAccessor::lag(std::int64_t periods = 1) const
```

## What It Does

Shifts every bound key `periods` positions forward: `out[i] = x[i - periods]`
where that index is in bounds, `NULL` otherwise. A negative `periods` behaves
as [`lead(-periods)`](lead.md) — `lag(-periods)` and `lead(periods)` are the
same operation.

```{important} CatalogTimeSeriesAccessor requires every bound key to be numeric Series-kind
Same precondition as every method on this accessor — see
[`CatalogTimeSeriesAccessor::diff`](diff.md)'s own important callout for the
full explanation and how to project down to numeric keys first.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift forward. Negative values behave as
    [`lead(-periods)`](lead.md) instead.

## Returns

`Catalog`
    A new catalog with every bound key shifted by `periods` positions.

## Examples

Example 1. Lag a numeric key by one position.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().lag(1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [nan, 10, 20, 15]                                
========================================================================================================================
```

Each position now shows the previous position's value; position `0` has no
predecessor, so it's `nan`.

Example 2. A negative `periods` behaves like `lead()` with the sign flipped.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out_lag = ct.ts().lag(-1);
auto out_lead = ct.ts().lead(1);
std::cout << "lag(-1):\n" << out_lag;
std::cout << "lead(1):\n" << out_lead;
```

### Output

```text
lag(-1):
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [20, 15, 30, nan]                                
========================================================================================================================
lead(1):
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [20, 15, 30, nan]                                
========================================================================================================================
```

`lag(-1)` and `lead(1)` produce identical results.

Example 3. A two-key catalog: `lag()` shifts every bound key independently, in
the same call.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto out = ct.ts().lag(1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [a, b]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
a                        | Series       | Series[float64]  | 3       | [nan, 10, 20]                                    
b                        | Series       | Series[float64]  | 3       | [nan, 1, 2]                                      
========================================================================================================================
```

## Notes

- `lag(periods)` with a negative `periods` is exactly `lead(-periods)` — the
  two methods share one implementation, differing only in shift direction.
- A pre-existing missing entry in a bound key is honored, not just the `NULL`s
  the shift itself introduces.
- See the important callout above.

## See Also

- [`CatalogTimeSeriesAccessor::lead`](lead.md): the mirror-image shift.
- [`Catalog::ts`](../catalog/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::lag`](../thread-ts/lag.md): the underlying
  single-thread engine this method delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="lead.html">
    <span class="label">Next</span>
    <span class="title">lead</span>
  </a>
</div>
