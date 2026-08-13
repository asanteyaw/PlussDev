<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 87ad3a85ee5e -->

# StreamNoteAccessor::titlecase

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::titlecase() const
```

## What It Does

Capitalizes the first letter of each word in the bound stream's text (and
lowercases the rest), returning a new stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its text titlecased.

## Examples

Example 1. Titlecase lowercase text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"accra city"}));
auto out = stream.nt().titlecase();
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
0            | Accra City                                                                                               
========================================================================================================================
```

## Notes

- See the important callout above.
- Every word is affected, not just the first — the whole-string equivalent of
  a "capitalize each word" transform, not a "capitalize the sentence" one.

## See Also

- [`StreamNoteAccessor::lower`](lower.md) / [`upper()`](upper.md): the
  simpler, uniform-case conversions.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::titlecase`](../catalog-note/titlecase.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
