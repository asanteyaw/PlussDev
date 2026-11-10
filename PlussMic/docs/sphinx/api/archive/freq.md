<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ca53393cd70 -->

# Archive::freq

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::freq(FreqOptions options = {}) const -> Archive
```

### `FreqOptions` fields

| Field | Type | Default |
|---|---|---|
| `top` | `std::int64_t` | `0` |
| `dropnull` | `bool` | `true` |

## What It Does

Returns a new archive where every catalog is replaced by its own
[`Catalog::freq()`](../catalog/freq.md) result — each stream reduced to occurrence counts
per distinct value, per catalog. The distinct **values become the resulting stream's row
labels**; the counts become the resulting stream's own numeric values.
`options.top` (default `0`, meaning no limit) caps the result to the `top` most frequent
values; `options.dropnull` (default `true`) excludes `NULL` from consideration.

## Parameters

**options** (`FreqOptions`, optional)
    `top` limits the result to the N most frequent values (`0` = no limit); `dropnull`
    excludes `NULL` from the counts.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog replaced by a count-per-distinct-value `Series[int64]` stream, labeled by the
    original distinct values.

## Examples

Example 1. Frequency counts of a text stream — note the distinct values appear as row
labels, not as the stream's own values.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "north"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.freq();
std::cout << out.catalog(0).get("region").value();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
east         | 2                                                                                                        
west         | 1                                                                                                        
north        | 1                                                                                                        
========================================================================================================================
```

## Notes

- The distinct values become the resulting stream's row **labels** (via
  [`lbl()`](../stream/lbl.md)); the counts become the numeric `Series[int64]` values — easy
  to misread if you expect the original values inline like [`distinct()`](distinct.md).
- Each stream, in every catalog, is reduced independently — resulting streams within one
  catalog can end up different lengths.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::distinct`](distinct.md): the distinct values alone, without counts.
- [`Archive::ndistinct`](ndistinct.md): just the count of distinct values.
- [`Catalog::freq`](../catalog/freq.md): the single-catalog delegate this method applies to
  every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
