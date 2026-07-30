<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e1f958b1795 -->

# Catalog::sum

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::sum(std::string_view field) const -> Stream
```

## What It Does

Computes the sum of one named numeric stream — a thin wrapper over
[`at(field).sum()`](at.md). Returns a single-value `Stream`, not a raw scalar; unwrap it
further if you need a plain number.

```{important} Single-field only — unlike distinct()/freq()/ndistinct()
`sum()`/[`mean()`](mean.md)/[`min()`](min.md)/[`max()`](max.md) all take one field name and
return a single-value `Stream` for just that field. This is a different shape from
[`distinct()`](distinct.md)/[`freq()`](freq.md)/[`ndistinct()`](ndistinct.md), which take
no field name at all and operate across every stream in the catalog, returning a whole
`Catalog`. Don't expect a field-name argument on those three.
```

## Parameters

**field** (`std::string_view`)
    The stream name to sum. Must exist and be numeric `Series`-kind.

## Returns

`Stream`
    A single-value stream holding the sum.

## Examples

Example 1. Sum a numeric field.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
});

auto out = ct.sum("id");
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
0            | 8                                                                                                        
========================================================================================================================
```

Example 2. Summing a non-numeric field throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west"})),
});

try {
  auto out = ct.sum("region");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::sum is only valid for Series payload
```

Example 3. An unknown field name throws — the same message [`at()`](at.md) itself would
give.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

try {
  auto out = ct.sum("zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog: key 'zzz' not found
```

## Notes

- A thin wrapper over [`at(field).sum()`](at.md) — the throw messages come straight from
  `at()` (unknown key) and the underlying `Thread` (non-numeric payload), not from `sum()`
  itself.
- Single-field only — see the important callout above for how this differs from
  `distinct()`/`freq()`/`ndistinct()`.

## See Also

- [`Catalog::mean`](mean.md) / [`Catalog::min`](min.md) / [`Catalog::max`](max.md): the
  other single-field summary statistics, same shape as this method.
- [`Catalog::at`](at.md): the underlying stream lookup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_by.html">
    <span class="label">Previous</span>
    <span class="title">sort_by</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
