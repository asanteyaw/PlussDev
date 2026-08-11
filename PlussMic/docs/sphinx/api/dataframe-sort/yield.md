# DataFrameSortBuilder::yield

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::yield() const -> DataFrame
```

## What It Does

Executes the sort plan and returns the fully sorted frame.

## Returns

`DataFrame`
    A new frame with rows reordered by the planned value ordering.

## Example

```cpp
auto out = df.sort()
    .by("sales")
    .desc()
    .yield();
```

## Notes

- This is the explicit terminal that corresponds most closely to `arrange(...)`.
- It reuses the cached permutation when one already exists on the builder.

## See Also

- [`DataFrameSortBuilder::order`](order.md)
- [`DataFrameSortBuilder::head`](head.md)
- [`DataFrameSortBuilder::tail`](tail.md)
