<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2bee23589c1d -->

# StreamNoteAccessor::lower

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::lower() const
```

## What It Does

Converts the bound stream's text to lowercase, returning a new stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its text converted to lowercase.

## Examples

Example 1. Lowercase mixed-case text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra City"}));
auto out = stream.nt().lower();
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
0            | accra city                                                                                               
========================================================================================================================
```

## Notes

- See the important callout above.
- For Unicode text with special casing rules, prefer [`casefold()`](casefold.md)
  for case-insensitive comparisons.

## See Also

- [`StreamNoteAccessor::upper`](upper.md): the inverse conversion.
- [`StreamNoteAccessor::casefold`](casefold.md): a more aggressive
  normalization intended for comparison, not display.
- [`StreamNoteAccessor::titlecase`](titlecase.md): capitalizes the first
  letter of each word instead.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::lower`](../catalog-note/lower.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
