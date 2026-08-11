# DataFrameSortBuilder::tail

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::tail(std::int64_t n) const -> DataFrame
```

## What It Does

Executes the plan and returns only the bottom-ranked `n` rows.

## Returns

`DataFrame`
    The last `n` rows under the planned ordering.

## Example

```cpp
auto bottom3 = df.sort().by("sales").desc().tail(3);
```

## Notes

- `n` must be non-negative.
- `tail(n)` uses the planned ordering; it is not equivalent to unsorted frame tail.

## See Also

- [`DataFrameSortBuilder::head`](head.md)
- [`DataFrameSortBuilder::yield`](yield.md)
