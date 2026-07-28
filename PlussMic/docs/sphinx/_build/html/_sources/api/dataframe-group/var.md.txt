<!-- pluss-hand-authored -->

# DataFrameGrouped::var

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::var() const -> DataFrame
```

## What It Does

Computes grouped sample variance for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount"})
               .group()
               .by({"city"})
               .var();
```

## See Also

- [`DataFrameGrouped::std`](std.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
