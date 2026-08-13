<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 67659145262f -->

# StreamNoteAccessor::has

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::has(std::string_view needle, const pluss::table::TextOptions& options = {}) const
```

## What It Does

Flags whether the bound stream's text contains `needle`, replacing it with a
`Bool` `Series` stream.

```{important} has() defaults to LITERAL matching
`has()`'s pattern is treated as a literal substring by default
(`options.regex = false`). This is the opposite default from
[`count()`](count.md)'s single-argument overload, which defaults to *regex*
matching — a recurring asymmetry across this accessor's pattern-based methods.
Pass `TextOptions{.regex = true}` explicitly to search by regex here.
```

```{important} StreamNoteAccessor requires the bound stream to be Note-kind
Same precondition as every method on this accessor — see
[`StreamNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**needle** (`std::string_view`)
    The text to search for — literal by default, or regex if
    `options.regex = true`.

**options** (`pluss::table::TextOptions`, optional)
    `options.regex` (default `false`): treat `needle` as a regex pattern.
    `options.case_sensitive` (default `true`): ignore case when `false`.

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row contains `needle`,
    `false` otherwise.

## Examples

Example 1. Default literal-substring search.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"banana"}));
auto out = stream.nt().has("an");
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

- See the important callouts above — the literal-by-default asymmetry with
  `count()` is a common source of confusion.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::count`](count.md): the sibling method, which defaults
  to regex matching on its single-argument overload — the opposite default.
- [`StreamNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  always-regex pattern methods.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::has`](../catalog-note/has.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
