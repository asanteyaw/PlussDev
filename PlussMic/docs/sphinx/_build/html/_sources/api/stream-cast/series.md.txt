<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f236fceb2ad5 -->

# StreamCastAccessor::series

[Back to StreamCastAccessor API Reference](../../stream_cast_accessor_api_reference)

## Signature

```cpp
Stream StreamCastAccessor::series() const
```

## What It Does

Casts the bound stream's values to `Series`-kind and returns a new stream.

```{important} Unparseable text becomes null, not a throw
Casting a `Note` value that can't be parsed as numeric (e.g. `"Accra"`)
doesn't throw — it silently becomes a null value in the resulting stream.
Check [`Stream::isnull`](../stream/isnull.md) after casting if you need to
detect values that failed to convert.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream cast to `Series`-kind.

## Examples

Example 1. Cast a `Note` stream of numeric-looking text to `Series`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"}));
auto out = stream.cast().series();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 2 (edge case). Unparseable text becomes null rather than throwing —
see the important callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"}));
auto out = stream.cast().series();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | NA                                                                                                       
1            | NA                                                                                                       
========================================================================================================================
```

## Notes

- See the important callout above for the silent-null behavior on unparseable
  text.

## See Also

- [`StreamCastAccessor::note`](note.md): the inverse direction, casting
  numeric streams to text.
- [`Stream::cast`](../stream/cast.md): the direct-target overload of casting,
  covering the same conversions.
- [`CatalogCastAccessor::series`](../catalog-cast/series.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
</div>
