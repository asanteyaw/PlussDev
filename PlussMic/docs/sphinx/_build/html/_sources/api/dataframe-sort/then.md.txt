# DataFrameSortBuilder::then

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder::then(std::string_view feature) const -> DataFrameSortBuilder
DataFrameSortBuilder::then(std::initializer_list<std::string_view> features) const -> DataFrameSortBuilder
DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<std::string_view> features) const -> DataFrameSortBuilder
DataFrameSortBuilder::then(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const -> DataFrameSortBuilder
```

## What It Does

Appends additional sort keys after an existing `.by(...)` plan. This is the
tie-break path.

## Returns

`DataFrameSortBuilder`
    A new builder with the extra keys appended and the last appended key marked active.

## Example

```cpp
auto ordered = df.sort()
    .by("city")
    .then("sales")
    .desc()
    .yield();
```

## Notes

- `then(...)` requires an existing `.by(...)` plan.
- The last appended key becomes the active key for later `.asc()`, `.desc()`,
  `.nulls_first()`, and `.nulls_last()` calls.

## See Also

- [`DataFrameSortBuilder::by`](by.md)
- [`DataFrameSortBuilder::desc`](desc.md)
