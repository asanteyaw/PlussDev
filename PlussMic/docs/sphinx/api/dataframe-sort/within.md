# DataFrameSortBuilder::within

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder::within(std::string_view feature) const -> DataFrameSortBuilder
DataFrameSortBuilder::within(std::initializer_list<std::string_view> features) const -> DataFrameSortBuilder
DataFrameSortBuilder::within(pluss::msharp::RangeView::Of<std::string_view> features) const -> DataFrameSortBuilder
```

## What It Does

Adds grouping keys that partition the frame before sorting. Group order stays
first-seen; rows are reordered only inside each group.

## Returns

`DataFrameSortBuilder`
    A new builder with the within-group partition set.

## Example

```cpp
auto out = df.sort()
    .within({"city"})
    .by("sales")
    .desc()
    .yield();
```

## Notes

- `within(...)` does not define the value sort keys by itself; pair it with
  `.by(...)`.
- Group order is preserved from the original frame.

## See Also

- [`DataFrameSortBuilder::by`](by.md)
- [`DataFrameSortBuilder::yield`](yield.md)
