<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69ffbef8d19c -->

# StreamNoteAccessor::ends

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::ends(std::string_view suffix) const
```

## What It Does

Flags whether the bound stream's text ends with `suffix`, replacing it with a
`Bool` `Series` stream.

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**suffix** (`std::string_view`)
    The literal suffix to test for.

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row ends with `suffix`,
    `false` otherwise.

## Examples

Example 1. Flag which names end with `"a"`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"}));
auto out = stream.nt().ends("a");
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
1            | true                                                                                                     
========================================================================================================================
```

Both names end in a lowercase `"a"`.

## Notes

- See the important callout above.
- Always a literal, case-sensitive suffix match — no regex, no
  case-insensitive option.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::starts`](starts.md): the mirror-image, prefix check.
- [`StreamNoteAccessor::has`](has.md): a more general substring/pattern check,
  anywhere in the text.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::ends`](../catalog-note/ends.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="findall.html">
    <span class="label">Next</span>
    <span class="title">findall</span>
  </a>
</div>
