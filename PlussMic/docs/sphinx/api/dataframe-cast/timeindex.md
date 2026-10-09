<!-- pluss-hand-authored -->

# DataFrameCastAccessor::timeindex

[Back to DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)

## Signature

```cpp
DataFrameCastAccessor::timeindex() const -> DataFrame
```

## What It Does

Casts every feature currently bound on the accessor to `TimeIndex`, using the shared
`SlateCastOptions` from `DataFrame::cast(...)`.

## Returns

`DataFrame`
    A new frame with the bound features converted to `TimeIndex`.

## Notes

- Use this when the target should be indexed temporal values rather than plain
  `Timeline`.

## See Also

- [`DataFrame::cast`](../dataframe/cast.md): enters the accessor and supplies shared
  `SlateCastOptions`.
- [`DataFrameCastAccessor::timeline`](timeline.md): the non-indexed temporal target.
