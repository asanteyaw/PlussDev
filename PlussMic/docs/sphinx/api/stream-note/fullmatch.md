<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91bae205ffcb -->

# StreamNoteAccessor::fullmatch

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::fullmatch(std::string_view pattern) const
```

## What It Does

Flags whether the bound stream's **entire** text matches `pattern` as a regex,
replacing it with a `Bool` `Series` stream. Equivalent to
[`matches(pattern, true)`](matches.md) — fully anchored
(`std::regex_match`), unlike `matches()`'s default unanchored behavior.

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression that must match the whole string.

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row's entire text matches
    `pattern`, `false` otherwise.

## Examples

Example 1. A fully alphanumeric pattern matches the whole string.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra123"}));
auto out = stream.nt().fullmatch("[A-Za-z0-9]+");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | true                                                                                                     
========================================================================================================================
```

## Notes

- Fully anchored — the entire string must match, not just a substring; see
  [`matches()`](matches.md) for the unanchored default.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::matches`](matches.md): the general form, with an
  optional `full` toggle instead of always anchoring.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::fullmatch`](../catalog-note/fullmatch.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
