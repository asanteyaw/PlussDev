<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76d20050395e -->

# StreamNoteAccessor::rstrip

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
Stream StreamNoteAccessor::rstrip() const

Stream StreamNoteAccessor::rstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **right** edge only of the bound stream's text,
returning a new stream. The no-argument overload trims whitespace; the
`chars`-argument overload trims any of the given characters.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the right edge.

## Returns

`Stream`
    A new stream with its text trimmed on the right edge.

## Examples

Example 1. No-argument overload trims trailing whitespace, leaving leading
whitespace intact.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"  Accra  "}));
auto out = stream.nt().rstrip();
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
0            |   Accra                                                                                                  
========================================================================================================================
```

The two leading spaces are still visible in the preview; only the trailing pair
was removed.

## Notes

- See the important callout above.
- Only affects the right edge — the leading whitespace/characters in the
  source text are preserved.

## See Also

- [`StreamNoteAccessor::lstrip`](lstrip.md): the mirror-image, left-edge-only
  trim.
- [`StreamNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::rstrip`](../catalog-note/rstrip.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
