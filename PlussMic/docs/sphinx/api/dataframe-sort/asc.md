# DataFrameSortBuilder::asc

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::asc() const -> DataFrameSortBuilder
```

## What It Does

Marks the active key as ascending.

## Returns

`DataFrameSortBuilder`
    A new builder with the active key direction set to ascending.

## Example

```cpp
auto out = df.sort().by("city").asc().yield();
```

## Notes

- Call `.by(...)` or `.then(...)` first so there is an active key to mutate.
- `asc()` changes only the active key, not every key in the plan.

## See Also

- [`DataFrameSortBuilder::desc`](desc.md)
- [`DataFrameSortBuilder::nulls_last`](nulls_last.md)
