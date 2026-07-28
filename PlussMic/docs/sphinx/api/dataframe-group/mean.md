<!-- pluss-hand-authored -->

# DataFrameGrouped::mean

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::mean() const -> DataFrame
```

## What It Does

Computes group-wise arithmetic mean for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount", "units"})
               .group({.sort = true})
               .by({"city"})
               .mean();
```

## Notes

- The convenience terminal is best when all remaining value features should take the
  same grouped operation.
- If you need one mean and one count in the same output, use [`agg()`](agg.md)
  instead.

## See Also

- [`DataFrameGrouped::median`](median.md)
- [`DataFrameGrouped::sum`](sum.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
