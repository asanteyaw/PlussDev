<!-- pluss-hand-authored -->

# DataFrameGrouped::median

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::median() const -> DataFrame
```

## What It Does

Computes group-wise median for each non-key value feature.

## Example

```cpp
auto out = sales.features({"city", "amount"})
               .group()
               .by({"city"})
               .median();
```

## Notes

- Median is robust to extreme values in ways mean is not.
- Like the other grouped terminals, it applies to every remaining non-key value
  feature uniformly.

## See Also

- [`DataFrameGrouped::mean`](mean.md)
- [`DataFrameGrouped::mode`](mode.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
