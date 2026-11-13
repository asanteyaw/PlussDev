<!-- pluss-hand-authored -->

# DataFrameCastAccessor::timeline

[Back to DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCastAccessor::timeline() const
```

## What It Does

Casts every feature currently bound on the accessor to `Timeline`, using the shared
`format` and `invalid` policy from `SlateCastOptions` if the sources are text.

## Returns

`DataFrame`
    A new frame with the bound features converted to `Timeline`.

## Notes

- `options.format` is consulted when parsing `Note` text as datetimes.
- `options.invalid = ParseInvalid::Null` turns unparseable text cells into `NULL`
  instead of raising.

## See Also

- [`DataFrame::cast`](../dataframe/cast.md): enters the accessor and supplies shared
  `SlateCastOptions`.
