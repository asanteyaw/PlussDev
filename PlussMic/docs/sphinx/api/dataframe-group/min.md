<!-- pluss-hand-authored -->

# DataFrameGrouped::min

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::min() const -> DataFrame
```

## What It Does

Computes per-group minimum for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount"})
               .group()
               .by({"city"})
               .min();
```

## See Also

- [`DataFrameGrouped::max`](max.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="median.html">
    <span class="label">Previous</span>
    <span class="title">median</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
