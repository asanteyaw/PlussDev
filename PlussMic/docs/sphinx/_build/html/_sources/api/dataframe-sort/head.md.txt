# DataFrameSortBuilder::head

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder::head(std::int64_t n) const -> DataFrame
```

## What It Does

Executes the plan and returns only the top-ranked `n` rows.

## Returns

`DataFrame`
    The first `n` rows under the planned ordering.

## Example

```cpp
auto top3 = df.sort().by("sales").desc().head(3);
```

## Notes

- `n` must be non-negative.
- The builder caches ranked slices separately from the full order so repeated
  `head(n)` calls do not always recompute.

## See Also

- [`DataFrameSortBuilder::tail`](tail.md)
- [`DataFrameSortBuilder::yield`](yield.md)
