<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1bd1a9f681cc -->

# StreamNoteAccessor::findall

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
pluss::table::DataFrame StreamNoteAccessor::findall(std::string_view pattern) const

pluss::table::DataFrame StreamNoteAccessor::findall(std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Finds every regex match of `pattern` in the bound stream's text and returns
them as columns of a `pluss::table::DataFrame` — one row per record, one
column per match position found across all records.

```{important} Returns a DataFrame, not a Stream
Unlike every other method on this accessor, `findall()` leaves the kindle
model entirely — its result is a `pluss::table::DataFrame`, not a `Stream`.
[`split()`](split.md), [`partition()`](partition.md), and
[`indicate()`](indicate.md) share this same escape hatch.
```

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression.

**options** (`const pluss::table::TextOptions&`, two-argument overload)
    `options.case_sensitive` (default `true`): ignore case when `false`.

## Returns

`pluss::table::DataFrame`
    One row per record; one `match_N` column per match position, up to the
    maximum number of matches found in any single record.

## Examples

Example 1. Extract every run of digits from a string.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"a1 b2 c3"}));
auto out = stream.nt().findall("[0-9]+");
std::cout << out;
```

### Output

```text
================================
     | match_0  match_1  match_2
--------------------------------
0    | 1        2        3      
--------------------------------
[1 cs x 3 feat]
```

## Notes

- See the important callout above.
- Columns are named `match_0`, `match_1`, ... in order of appearance.
- A record with fewer matches than the maximum leaves the extra columns null
  for that row.

## See Also

- [`StreamNoteAccessor::split`](split.md) / [`partition()`](partition.md): the
  other methods on this accessor that leave the kindle model for a
  `DataFrame` result.
- [`StreamNoteAccessor::count`](count.md): counts matches instead of
  extracting their text.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::findall`](../catalog-note/findall.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
