<!-- pluss-hand-authored -->

# DataFrameGrouped::count

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::count() const -> DataFrame
```

## What It Does

Counts non-null observed values per group for the current non-key value features.

This is the grouped convenience terminal for “how many observed entries does each
value feature contribute inside each group?”

## Example

```cpp
using namespace pluss::table;

auto out = sales.features({"city", "amount", "units"})
               .group()
               .by({"city"})
               .count();
```

That yields one row per city with `amount_count` and `units_count`.

## Notes

- Convenience terminals exclude ordinary grouping keys from the value side before
  aggregating.
- If no non-key value features remain, `.count()` still returns an explicit row-count
  column named `count`.
- If you need row count beside other explicit grouped metrics, prefer
  [`agg()`](agg.md) with `{"n", "", GroupOp::Count}`.

## See Also

- [`DataFrameGrouped::agg`](agg.md)
- [`DataFrameGrouped::ndistinct`](ndistinct.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
