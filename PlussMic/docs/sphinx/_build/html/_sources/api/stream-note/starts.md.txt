<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0c37989148a0 -->

# StreamNoteAccessor::starts

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::starts(std::string_view prefix) const
```

## What It Does

Flags whether the bound stream's text starts with `prefix`, replacing it with
a `Bool` `Series` stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**prefix** (`std::string_view`)
    The literal prefix to test for.

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row starts with `prefix`,
    `false` otherwise.

## Examples

Example 1. Flag which names start with `"A"`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"}));
auto out = stream.nt().starts("A");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | true                                                                                                     
1            | false                                                                                                    
========================================================================================================================
```

## Notes

- See the important callout above.
- Always a literal, case-sensitive prefix match — no regex, no case-insensitive
  option.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::ends`](ends.md): the mirror-image, suffix check.
- [`StreamNoteAccessor::has`](has.md): a more general substring/pattern check,
  anywhere in the text.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::starts`](../catalog-note/starts.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
