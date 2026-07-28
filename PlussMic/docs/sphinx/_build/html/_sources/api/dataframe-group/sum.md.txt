<!-- pluss-hand-authored -->

# DataFrameGrouped::sum

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::sum() const -> DataFrame
```

## What It Does

Sums each non-key value feature within each group.

## Example

```cpp
auto out = sales.features({"city", "amount", "units"})
               .group()
               .by({"city"})
               .sum();
```

This yields grouped columns such as `amount_sum` and `units_sum`.

## Notes

- Intended for additive value features.
- Equivalent in spirit to `agg({{"", "amount", GroupOp::Sum}, ...})`, but with
  automatic spec generation for the current value features.

## See Also

- [`DataFrameGrouped::mean`](mean.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
