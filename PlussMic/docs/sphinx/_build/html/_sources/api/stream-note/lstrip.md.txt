<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 50f95971b78e -->

# StreamNoteAccessor::lstrip

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
Stream StreamNoteAccessor::lstrip() const

Stream StreamNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **left** edge only of the bound stream's text,
returning a new stream. The no-argument overload trims whitespace; the
`chars`-argument overload trims any of the given characters.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the left edge.

## Returns

`Stream`
    A new stream with its text trimmed on the left edge.

## Examples

Example 1. No-argument overload trims leading whitespace, leaving trailing
whitespace intact.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"  Accra  "}));
auto out = stream.nt().lstrip();
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
0            | Accra                                                                                                    
========================================================================================================================
```

## Notes

- See the important callout above.
- Only affects the left edge — the trailing whitespace/characters in the source
  text are preserved.

## See Also

- [`StreamNoteAccessor::rstrip`](rstrip.md): the mirror-image, right-edge-only
  trim.
- [`StreamNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::lstrip`](../catalog-note/lstrip.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
