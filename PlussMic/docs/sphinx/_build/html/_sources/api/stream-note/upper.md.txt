<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bc803ab42d8e -->

# StreamNoteAccessor::upper

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::upper() const
```

## What It Does

Converts the bound stream's text to uppercase, returning a new stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its text converted to uppercase.

## Examples

Example 1. Uppercase mixed-case text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra City"}));
auto out = stream.nt().upper();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | ACCRA CITY                                                                                               
========================================================================================================================
```

## Notes

- See the important callout above.

## See Also

- [`StreamNoteAccessor::lower`](lower.md): the inverse conversion.
- [`StreamNoteAccessor::casefold`](casefold.md): a more aggressive
  normalization intended for comparison, not display.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::upper`](../catalog-note/upper.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
