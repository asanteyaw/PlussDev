<!-- pluss-hand-authored -->

# DataFrameGrouped::ndistinct

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::ndistinct() const -> DataFrame
```

## What It Does

Counts the number of distinct observed values per group for each non-key value
feature.

## Example

```cpp
auto out = sales.features({"city", "product"})
               .group()
               .by({"city"})
               .ndistinct();
```

That gives one row per city with `product_ndistinct`.

## Notes

- This is per-feature distinctness, not tuple distinctness across several value
  features at once.
- It is a convenience grouped terminal; use [`agg()`](agg.md) when you need explicit
  mixed grouped specs.

## See Also

- [`DataFrame::ndistinct`](../dataframe/ndistinct.md): whole-frame distinct counting.
- [`DataFrameGrouped::count`](count.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
