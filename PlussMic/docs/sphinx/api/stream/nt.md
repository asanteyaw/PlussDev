<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c9b690ca6b4c -->

# Stream::nt

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
StreamNoteAccessor Stream::nt() const
```

## What It Does

The entry point into text-processing operations on the stream. `nt()` binds the
current stream and hands back a `StreamNoteAccessor`, which exposes 23 methods:
trimming (`strip`, `lstrip`, `rstrip`), case conversion (`lower`, `upper`,
`casefold`, `titlecase`), inspection (`length`, `starts`, `ends`, `has`, `count`,
`matches`, `fullmatch`), splitting/extraction (`findall`, `split`, `partition`,
`indicate`), and transformation (`pad`, `chars`, `normalize`, `encode`, `replace`)
— see the [StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)
for every method's own dedicated page.

```{important} Requires the stream itself to be Note-kind
Unlike [`Catalog::nt()`](../catalog/nt.md), which binds every key of a multi-key
collection, `Stream::nt()` binds a single stream — but that stream must already be
`Note`-kind. Calling any `StreamNoteAccessor` method on a `Series`/`Timeline`/
`TimeIndex` stream throws (the exact message names whichever method actually ran,
e.g. `"Slate::strip is only valid for Note payload"`). Cast first with
[`Stream::cast(SlateCastTarget::Note)`](cast.md) if the stream isn't already text.
```

## Parameters

`None`

## Returns

`StreamNoteAccessor`
    A binder for the current stream — call one of its 23 methods to inspect or
    transform its text.

## Examples

Example 1. `nt().strip()` trims whitespace from a `Note`-kind stream — see
[`StreamNoteAccessor::strip`](../stream-note/strip.md) for this specific method's
own page.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "Tema"}));
auto out = stream.nt().strip();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | Accra                                                                                                    
1            | Tema                                                                                                     
========================================================================================================================
```

Example 2 (edge case). Calling any method on this accessor with a non-`Note`
stream throws — see the important callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));
try {
  auto out = stream.nt().strip();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slate::strip is only valid for Note payload
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks the bound stream's kind.
- See the important callout above — every one of the 23 methods on the returned
  accessor shares this Note-kind-required precondition.
- Requires a defined stream; throws `"Stream: undefined handle"` otherwise (checked
  when a method is actually called, not by `nt()` itself).

## See Also

- [StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference):
  the full list of 23 methods this accessor exposes.
- [`Stream::tl`](tl.md) / [`Stream::ts`](ts.md): the sibling accessors for
  timestamp-feature and time-series operations.
- [`Stream::cast`](cast.md): convert a non-`Note` stream to `Note`-kind first.
- [`Catalog::nt`](../catalog/nt.md): the whole-catalog equivalent, which binds
  every key of a multi-key collection instead of one stream.
- [`CatalogNoteAccessor::strip`](../catalog-note/strip.md): the exact same
  underlying text engine, applied across a catalog's keys.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="plot.html">
    <span class="label">Next</span>
    <span class="title">plot</span>
  </a>
</div>
