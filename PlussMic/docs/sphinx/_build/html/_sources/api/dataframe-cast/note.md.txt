<!-- pluss-hand-authored -->

# DataFrameCastAccessor::note

[Back to DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)

## Signature

```cpp
DataFrameCastAccessor::note() const -> DataFrame
```

## What It Does

Casts every feature currently bound on the accessor to `Note`, preserving the same
feature names and case labels.

## Returns

`DataFrame`
    A new frame with the bound features rendered as text-backed `Note` columns.

## Notes

- This is the cleanest way to stringify a selected subframe without writing one
  `CastSpec` per feature.

## See Also

- [`DataFrame::cast`](../dataframe/cast.md): enters the accessor and supplies shared
  `SlateCastOptions`.
- [`DataFrameCastAccessor::series`](series.md),
  [`DataFrameCastAccessor::timeline`](timeline.md): the other payload-family targets.
