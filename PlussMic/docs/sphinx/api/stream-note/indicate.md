<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0b85bdc464b5 -->

# StreamNoteAccessor::indicate

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame StreamNoteAccessor::indicate(std::string_view delimiter = "|", std::string prefix = "") const
```

## What It Does

Multi-label-binarizes the bound stream's text: splits each value on
`delimiter`, then builds one `Bool` column per **distinct token seen anywhere
in the stream**, returning a `pluss::table::DataFrame` — like
[`findall()`](findall.md), this method leaves the kindle model. Each column is
named after the token itself (with `prefix` prepended, if given), and each
row's column is `true` if that row's value contains that token.

```{important} Returns a DataFrame, not a Stream
Same escape from the kindle model as [`findall()`](findall.md)/
[`split()`](split.md)/[`partition()`](partition.md) — see `findall()`'s own
important callout.
```

## Parameters

**delimiter** (`std::string_view`, defaults to `"|"`)
    The literal text used to split each value into tokens before binarizing.

**prefix** (`std::string`, default empty)
    Optional prefix prepended to each generated column's name.

## Returns

`pluss::table::DataFrame`
    One row per record. One `Bool` column per distinct token seen across
    every row.

## Examples

Example 1. `"a|b"` and `"c"`, split on the default `"|"`, produce three
columns — one per distinct token (`a`, `b`, `c`) — each `true` exactly where
that row's value contained the token.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"a|b", "c"}));
auto out = stream.nt().indicate("|");
std::cout << out;
```

### Output

```text
==========================
     | a      b      c    
--------------------------
0    | true   true   false
1    | false  false  true 
--------------------------
[2 cs x 3 feat]
```

## Notes

- See the important callout above.
- Column order follows the order distinct tokens are first encountered, not
  necessarily alphabetical — treat column presence/absence as the contract,
  not column position.
- The default `delimiter` is `"|"`, matching a common multi-value-tag
  convention; pass any other literal delimiter for differently-formatted tag
  text.

## See Also

- [`StreamNoteAccessor::findall`](findall.md) / [`split()`](split.md): the
  other methods on this accessor that leave the kindle model for a
  `DataFrame` result.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::indicate`](../catalog-note/indicate.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
