<!-- pluss-hand-authored -->

# DataFrameGrouped::max

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::max() const -> DataFrame
```

## What It Does

Computes per-group maximum for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount"})
               .group()
               .by({"city"})
               .max();
```

## See Also

- [`DataFrameGrouped::min`](min.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
