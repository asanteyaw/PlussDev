# DataFrameSortBuilder::by

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder::by(std::string_view feature) const -> DataFrameSortBuilder
DataFrameSortBuilder::by(std::initializer_list<std::string_view> features) const -> DataFrameSortBuilder
DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<std::string_view> features) const -> DataFrameSortBuilder
DataFrameSortBuilder::by(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const -> DataFrameSortBuilder
```

## What It Does

Starts the sort plan by choosing the primary value key or key list.

When you pass plain feature names, each one inherits the current builder defaults
for direction and null placement. When you pass `ArrangeSpec`s directly, each spec
carries its own `ascending` and `nulls_last` policy.

## Returns

`DataFrameSortBuilder`
    A new builder with the active primary key set to the last supplied key.

## Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Tema", "Accra", "Accra"}))},
    {"sales", pluss::table::Thread(pluss::make_series({120.0, 100.0, 90.0}))},
});

auto out = df.sort().by({"city", "sales"}).yield();
std::cout << out;
```

## Notes

- `by(...)` replaces the current key plan.
- Use [`then()`](then.md) to append tie-break keys instead of replacing the plan.
- Feature names must exist on the frame.

## See Also

- [`DataFrame::sort`](../dataframe/sort.md)
- [`DataFrameSortBuilder::then`](then.md)
- [`DataFrameSortBuilder::yield`](yield.md)
