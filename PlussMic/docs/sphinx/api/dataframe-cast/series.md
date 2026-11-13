<!-- pluss-hand-authored -->

# DataFrameCastAccessor::series

[Back to DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCastAccessor::series() const
```

## What It Does

Casts every feature currently bound on the accessor to `Series`, using the shared
`SlateCastOptions` that were supplied at `DataFrame::cast(...)` entry time.

## Returns

`DataFrame`
    A new frame with the same feature names, same case labels, and every bound
    feature converted to `Series`.

## Notes

- `options.series_dtype` controls the concrete numeric dtype when the source is
  `Note`, `Timeline`, or `TimeIndex`.
- Unparseable `Note -> Series` cells become `NULL`.

## See Also

- [`DataFrame::cast`](../dataframe/cast.md): enters the accessor and supplies shared
  `SlateCastOptions`.
- [`DataFrameCastAccessor::timeline`](timeline.md),
  [`DataFrameCastAccessor::note`](note.md): the other payload-family targets.
