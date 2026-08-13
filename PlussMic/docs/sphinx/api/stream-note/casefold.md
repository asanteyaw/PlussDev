<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0cf2e0614913 -->

# StreamNoteAccessor::casefold

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::casefold() const
```

## What It Does

Applies aggressive case-folding to the bound stream's text, returning a new
stream — intended for case-insensitive comparison, not display.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its text casefolded.

## Examples

Example 1. Casefold mixed-case text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra City"}));
auto out = stream.nt().casefold();
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

On plain ASCII text, `casefold()` produces the same result as
[`lower()`](lower.md) — the difference matters for certain Unicode characters
with special casing rules (e.g. German ß), where casefold applies a more
aggressive normalization than a simple lowercase conversion.

## Notes

- See the important callout above.
- Prefer this over [`lower()`](lower.md) specifically for case-insensitive
  *comparison*; prefer `lower()` for *display* purposes.

## See Also

- [`StreamNoteAccessor::lower`](lower.md) / [`upper()`](upper.md): the
  simpler case-conversion methods.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::casefold`](../catalog-note/casefold.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
