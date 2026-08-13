<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 270ec9410ddf -->

# StreamNoteAccessor::split

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
pluss::table::DataFrame StreamNoteAccessor::split(std::string_view delimiter, std::int64_t limit = -1, std::string prefix = "part") const

pluss::table::DataFrame StreamNoteAccessor::split(std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Splits the bound stream's text on `delimiter` (literal) or `pattern` (regex,
via `options`), returning the pieces as columns of a
`pluss::table::DataFrame` — one row per record, one column per split position.

```{important} Returns a DataFrame, not a Stream
Same escape from the kindle model as [`findall()`](findall.md) — see that
page's own important callout.
```

## Parameters

**delimiter** (`std::string_view`, first overload)
    The literal text to split on.

**limit** (`std::int64_t`, first overload, default `-1`)
    Maximum number of splits; `-1` means unlimited.

**prefix** (`std::string`, first overload, default `"part"`)
    The prefix used to name each output column (`"part_0"`, `"part_1"`, ...).

**pattern** (`std::string_view`, second overload)
    A regex pattern to split on, instead of a literal delimiter.

**options** (`const pluss::table::TextOptions&`, second overload)
    `options.case_sensitive` (default `true`): ignore case when `false`.

## Returns

`pluss::table::DataFrame`
    One row per record; one `{prefix}_N` column per split position, up to the
    maximum number of pieces found in any single record.

## Examples

Example 1. Split comma-separated text into columns.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"a,b,c"}));
auto out = stream.nt().split(",");
std::cout << out;
```

### Output

```text
=============================
     | part_0  part_1  part_2
-----------------------------
0    | a       b       c     
-----------------------------
[1 cs x 3 feat]
```

## Notes

- See the important callout above.
- The default column prefix is `"part"` — pass a different `prefix` to rename
  the output columns.
- A record with fewer pieces than the maximum leaves the extra columns null
  for that row.

## See Also

- [`StreamNoteAccessor::partition`](partition.md): a related split, keeping
  the delimiter itself as its own column and stopping at the first match.
- [`StreamNoteAccessor::findall`](findall.md): the other `DataFrame`-returning
  extraction method.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::split`](../catalog-note/split.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
