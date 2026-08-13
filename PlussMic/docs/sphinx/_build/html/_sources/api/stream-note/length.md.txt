<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f9a5962efaab -->

# StreamNoteAccessor::length

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::length() const
```

## What It Does

Measures the character length of the bound stream's text, replacing it with an
`Int64` `Series` stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Int64` `Series` stream of character lengths.

## Examples

Example 1. Measure the length of two names.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"}));
auto out = stream.nt().length();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 5                                                                                                        
1            | 4                                                                                                        
========================================================================================================================
```

## Notes

- See the important callout above.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::chars`](chars.md): extract a substring rather than
  measuring the whole string.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::length`](../catalog-note/length.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
