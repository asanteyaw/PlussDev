<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 17dc99a2ad4d -->

# StreamNoteAccessor::strip

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
Stream StreamNoteAccessor::strip() const

Stream StreamNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from **both** edges of the bound stream's text, returning a new
stream. The no-argument overload trims whitespace; the `chars`-argument overload
trims any of the given characters, in any order, from either end.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Every method on this accessor — not just strip() — routes through
`Thread::strip()` (or the equivalent) for the bound stream. If the stream isn't
Note-kind (`Series`, `Timeline`, `TimeIndex`), the call throws
`"Thread::strip is only valid for Note payload"` (the exact message names
whichever method actually ran). Cast first with
[`Stream::cast(ThreadCastTarget::Note)`](../stream/cast.md) if the stream isn't
already text.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from both edges. Every leading or trailing
    character that appears anywhere in `chars` is removed, stopping at the first
    character on each side that doesn't.

## Returns

`Stream`
    A new stream with its text trimmed on both edges.

## Examples

Example 1. No-argument overload trims leading and trailing whitespace.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "  Tema  "}));
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

Example 2. `chars`-argument overload trims a specific character set from both edges.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"xxAccraxx"}));
auto out = stream.nt().strip("x");
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

Example 3 (edge case). A non-`Note`-kind stream throws — see the important
callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1, 2, 3}));
try {
  auto out = stream.nt().strip();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::strip is only valid for Note payload
```

## Notes

- See the important callout above — this precondition applies to every method on
  `StreamNoteAccessor`, not only `strip()`.
- The `chars`-argument overload treats `chars` as a *set* of characters, not a
  literal prefix/suffix — order within `chars` doesn't matter.
- For a one-sided trim, see [`lstrip()`](lstrip.md) and [`rstrip()`](rstrip.md).

## See Also

- [`StreamNoteAccessor::lstrip`](lstrip.md) / [`rstrip()`](rstrip.md): the
  one-sided variants.
- [`StreamNoteAccessor::pad`](pad.md): the inverse operation, adding characters
  instead of removing them.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::strip`](../catalog-note/strip.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
