# DataFrameSortBuilder::order

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::order() const -> Thread
```

## What It Does

Materializes the planned row permutation as a `Thread`. This is the reusable
ranking artifact path.

## Returns

`Thread`
    An `int64` position thread. If the frame had case labels, the returned order
    thread carries those labels in sorted order too.

## Example

```cpp
auto ord = df.sort().by("sales").desc().order();
auto left = df.take(ord);
auto right = peer.take(ord);
```

## Notes

- `order()` caches the full permutation inside the builder.
- Use this when several aligned objects need the same ordering.

## See Also

- [`DataFrameSortBuilder::yield`](yield.md)
- [`DataFrameSortBuilder::head`](head.md)
