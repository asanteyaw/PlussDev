<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ad2fbc49df9 -->

# StreamCastAccessor::note

[Back to StreamCastAccessor API Reference](../../stream_cast_accessor_api_reference)

## Signature

```cpp
Stream StreamCastAccessor::note() const
```

## What It Does

Casts the bound stream's values to `Note`-kind (text) and returns a new stream.
Unlike casting toward `Series`, `Note`-conversion always succeeds — every domain
kind has a printable text form.

## Parameters

`None`

## Returns

`Stream`
    A new stream cast to `Note`-kind.

## Examples

Example 1. Cast a numeric stream to text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1, 2, 3}));
auto out = stream.cast().note();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

## Notes

- Unlike [`series()`](series.md), casting to `Note` doesn't have a
  silent-failure mode — any value can be rendered as text.

## See Also

- [`StreamCastAccessor::series`](series.md): the inverse direction, with a
  silent-null failure mode for unparseable text.
- [`Stream::cast`](../stream/cast.md): the direct-target overload of casting,
  covering the same conversions.
- [`CatalogCastAccessor::note`](../catalog-cast/note.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
