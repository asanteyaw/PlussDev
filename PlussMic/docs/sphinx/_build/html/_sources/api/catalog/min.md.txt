<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 736117f150b5 -->

# Catalog::min

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::min(std::string_view field) const -> Stream
```

## What It Does

Computes the minimum of one named numeric stream — a thin wrapper over
[`at(field).min()`](at.md). Same single-field shape as [`sum()`](sum.md),
[`mean()`](mean.md), and [`max()`](max.md) — see [`sum()`](sum.md)'s important callout for
how this differs from the whole-catalog `distinct()`/`freq()`/`ndistinct()` family.

## Parameters

**field** (`std::string_view`)
    The stream name to inspect. Must exist and be numeric `Series`-kind.

## Returns

`Stream`
    A single-value stream holding the minimum.

## Examples

Example 1. Find the minimum of a numeric field.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
});

auto out = ct.min("id");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
========================================================================================================================
```

## Notes

- A thin wrapper over [`at(field).min()`](at.md) — throw messages for an unknown field or
  non-numeric payload come from `at()`/the underlying `Thread`, not this method.
- Single-field only — see [`sum()`](sum.md)'s important callout.

## See Also

- [`Catalog::sum`](sum.md) / [`Catalog::mean`](mean.md) / [`Catalog::max`](max.md): the
  other single-field summary statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="merge.html">
    <span class="label">Previous</span>
    <span class="title">merge</span>
  </a>
  <a class="next-link" href="minrecords.html">
    <span class="label">Next</span>
    <span class="title">minrecords</span>
  </a>
</div>
