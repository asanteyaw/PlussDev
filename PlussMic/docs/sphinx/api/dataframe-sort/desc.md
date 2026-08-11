# DataFrameSortBuilder::desc

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::desc() const -> DataFrameSortBuilder
```

## What It Does

Marks the active key as descending.

## Returns

`DataFrameSortBuilder`
    A new builder with the active key direction set to descending.

## Example

```cpp
auto out = df.sort().by("sales").desc().yield();
```

## Notes

- Like `asc()`, this changes only the active key.
- Use `.then(...)` before `.desc()` when you want the later tie-break key to be the one modified.

## See Also

- [`DataFrameSortBuilder::asc`](asc.md)
- [`DataFrameSortBuilder::then`](then.md)
