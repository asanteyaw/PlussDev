# DataFrameSortBuilder::nulls_last

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::nulls_last() const -> DataFrameSortBuilder
```

## What It Does

Moves null values to the end for the active key.

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's null placement set to end.

## Example

```cpp
auto out = df.sort().by("sales").nulls_last().yield();
```

## See Also

- [`DataFrameSortBuilder::nulls_first`](nulls_first.md)
- [`DataFrameSortBuilder::by`](by.md)
