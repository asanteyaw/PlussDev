# Timeline::where

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::where(const Series& mask, const Timeline& other) const
```

## What It Does

Builds a new timeline by choosing between `self` and `other` under a boolean
mask.

Current verified behavior is:

- where `mask` is `true`, keep `self`
- where `mask` is `false`, take `other`

Read the call as `choose self where true, otherwise choose other`. The verified
output below makes that branch direction concrete.

## Parameters

**mask**

A boolean `Series` that is broadcast against the timeline operands.

Use it when you already computed a predicate such as `isnat()`, `lt(...)`, or
`is_weekend()`.

**other**

The alternate timeline to draw from where the mask is `false`.

This may be a scalar or a shaped timeline as long as broadcast rules can align
it with `self`.

## Returns

`Timeline`
    A new timeline assembled from the two temporal operands.

## Examples

Example 1. Keep the original values where they are missing and otherwise take a
date-normalized replacement.

```cpp
auto out = with_nat.where(with_nat.isnat(), with_nat.floor("D"));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, NaT, 2024-12-31 00:00:00, NaT ]
```

This verified output shows the mask is selecting `self` where the predicate is
`true`.

## Notes

- `where()` is the general branch/choose surface.
- For the narrower task "replace only missing timestamps with one scalar
  timestamp", [`fill_nat`](fill_nat.md) is more direct.
- The mask must be boolean. Passing a numeric `Series` is a contract error.

## See Also

- [`Timeline::fill_nat`](fill_nat.md)
- [`Timeline::isnat`](isnat.md)
- [`Timeline::eq`](eq.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_nat.html">
    <span class="label">Previous</span>
    <span class="title">fill_nat</span>
  </a>
  <a class="next-link" href="add_nanos.html">
    <span class="label">Next</span>
    <span class="title">add_nanos</span>
  </a>
</div>
