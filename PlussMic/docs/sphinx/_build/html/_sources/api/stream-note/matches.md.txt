<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f26aabdb58eb -->

# StreamNoteAccessor::matches

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::matches(std::string_view pattern, bool full = false) const
```

## What It Does

Flags whether the bound stream's text matches `pattern` as a regex, replacing
it with a `Bool` `Series` stream. With the default `full = false`, matching is
**unanchored** (`std::regex_search` — `pattern` needs to match somewhere in the
text). With `full = true`, matching is fully anchored (`std::regex_match` —
equivalent to [`fullmatch()`](fullmatch.md)).

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression.

**full** (`bool`, default `false`)
    When `true`, requires the pattern to match the entire string (anchored);
    when `false` (the default), matches if the pattern appears anywhere.

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row matches `pattern`,
    `false` otherwise.

## Examples

Example 1. Unanchored match — the pattern only needs to appear somewhere in
the text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra123"}));
auto out = stream.nt().matches("[0-9]+");
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

`"[0-9]+"` matches the trailing digits even though the whole string isn't
numeric.

## Notes

- The default `full = false` is unanchored — pass `true` for the same
  behavior as [`fullmatch()`](fullmatch.md).
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::fullmatch`](fullmatch.md): equivalent to
  `matches(pattern, true)`, always fully anchored.
- [`StreamNoteAccessor::has`](has.md): the plain-substring/regex flag, without
  the anchoring toggle.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::matches`](../catalog-note/matches.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
