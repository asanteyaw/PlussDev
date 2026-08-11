# DataFrameSortBuilder::nulls_first

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::nulls_first() const -> DataFrameSortBuilder
```

## What It Does

Moves null values to the front for the active key.

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's null placement set to front.

## Example

```cpp
auto out = df.sort().by("sales").nulls_first().yield();
```

## See Also

- [`DataFrameSortBuilder::nulls_last`](nulls_last.md)
- [`DataFrameSortBuilder::by`](by.md)
