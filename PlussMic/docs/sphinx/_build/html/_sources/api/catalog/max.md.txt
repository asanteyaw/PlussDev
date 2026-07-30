<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: de3d8f520b07 -->

# Catalog::max

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::max(std::string_view field) const -> Stream
```

## What It Does

Computes the maximum of one named numeric stream — a thin wrapper over
[`at(field).max()`](at.md). Same single-field shape as [`sum()`](sum.md),
[`mean()`](mean.md), and [`min()`](min.md) — see [`sum()`](sum.md)'s important callout for
how this differs from the whole-catalog `distinct()`/`freq()`/`ndistinct()` family.

## Parameters

**field** (`std::string_view`)
    The stream name to inspect. Must exist and be numeric `Series`-kind.

## Returns

`Stream`
    A single-value stream holding the maximum.

## Examples

Example 1. Find the maximum of a numeric field.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
});

auto out = ct.max("id");
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
0            | 3                                                                                                        
========================================================================================================================
```

## Notes

- A thin wrapper over [`at(field).max()`](at.md) — throw messages for an unknown field or
  non-numeric payload come from `at()`/the underlying `Thread`, not this method.
- Single-field only — see [`sum()`](sum.md)'s important callout.

## See Also

- [`Catalog::sum`](sum.md) / [`Catalog::mean`](mean.md) / [`Catalog::min`](min.md): the
  other single-field summary statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="masked_select.html">
    <span class="label">Previous</span>
    <span class="title">masked_select</span>
  </a>
  <a class="next-link" href="maxrecords.html">
    <span class="label">Next</span>
    <span class="title">maxrecords</span>
  </a>
</div>
