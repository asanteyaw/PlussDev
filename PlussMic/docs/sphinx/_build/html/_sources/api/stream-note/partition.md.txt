<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e0119463168 -->

# StreamNoteAccessor::partition

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame StreamNoteAccessor::partition(std::string_view separator, std::string prefix = "part") const
```

## What It Does

Splits the bound stream's text into exactly three pieces at the *first*
occurrence of `separator`: the text before it, the separator itself, and the
text after it — returned as three columns of a `pluss::table::DataFrame`.

```{important} Returns a DataFrame, not a Stream
Same escape from the kindle model as [`findall()`](findall.md) — see that
page's own important callout.
```

## Parameters

**separator** (`std::string_view`)
    The literal text to partition on.

**prefix** (`std::string`, default `"part"`)
    The prefix used to name each of the three output columns.

## Returns

`pluss::table::DataFrame`
    One row per record, with exactly three columns: `{prefix}_0` (before),
    `{prefix}_1` (the separator itself), `{prefix}_2` (after).

## Examples

Example 1. Partition a `key=value` string around `"="`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"key=value"}));
auto out = stream.nt().partition("=");
std::cout << out;
```

### Output

```text
=============================
     | part_0  part_1  part_2
-----------------------------
0    | key     =       value 
-----------------------------
[1 cs x 3 feat]
```

The middle column, `part_1`, holds the separator itself (`"="`), not an empty
placeholder — distinguishing `partition()` from a plain two-piece split.

## Notes

- See the important callout above.
- Always exactly three columns, unlike [`split()`](split.md)'s variable column
  count — `partition()` only ever splits at the *first* occurrence.
- The middle column always holds the separator text itself.

## See Also

- [`StreamNoteAccessor::split`](split.md): splits at every occurrence, with a
  variable number of output columns.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::partition`](../catalog-note/partition.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
