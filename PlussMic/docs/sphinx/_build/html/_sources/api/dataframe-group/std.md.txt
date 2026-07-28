<!-- pluss-hand-authored -->

# DataFrameGrouped::std

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::std() const -> DataFrame
```

## What It Does

Computes grouped sample standard deviation for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount"})
               .group()
               .by({"city"})
               .std();
```

## Notes

- This is the grouped dispersion companion to [`mean()`](mean.md).
- Use it only on value features where standard deviation is meaningful.

## See Also

- [`DataFrameGrouped::var`](var.md)
- [`DataFrameGrouped::mean`](mean.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
