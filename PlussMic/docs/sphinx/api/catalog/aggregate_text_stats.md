<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 83a8bec51c47 -->

# Catalog::aggregate_text_stats

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::aggregate_text_stats(std::string_view field, std::string_view stat_type) const -> Catalog
```

## What It Does

Collapses one text field into a single corpus-level statistic, returning a **brand-new**
two-stream, single-row catalog (`stat`, `value`) — like
[`extract_keywords()`](extract_keywords.md), this replaces the catalog's structure
entirely rather than adding a column to it.

```{important} stat_type accepts a fixed set of names — anything else throws
Supported values (case-insensitive): `"mean"`/`"avg"` (average character length),
`"min"`/`"max"` (shortest/longest character length), `"count"` (row count), and
`"vocab"`/`"vocab_size"` (number of distinct words across the whole field). Any other
value throws immediately.
```

## Parameters

**field** (`std::string_view`)
    The text stream to summarize. Must already exist.

**stat_type** (`std::string_view`)
    Which statistic to compute — see the important callout above for the full list.

## Returns

`Catalog`
    A new two-stream, single-row catalog: `stat` (the name you passed) and `value` (the
    computed number).

## Examples

Example 1. Average character length across the field.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.aggregate_text_stats("text", "mean");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [stat, value]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
stat                     | Note         | Note             | 1       | [mean]                                           
value                    | Series       | Series[float64]  | 1       | [20]                                             
========================================================================================================================
```

Example 2. Distinct-word count (vocabulary size) across the field.

```cpp
auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.aggregate_text_stats("text", "vocab");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [stat, value]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
stat                     | Note         | Note             | 1       | [vocab]                                          
value                    | Series       | Series[float64]  | 1       | [10]                                             
========================================================================================================================
```

Example 3. An unsupported `stat_type` throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({"heavy rain"})),
});

try {
  auto out = ct.aggregate_text_stats("text", "median");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::aggregate_text_stats: unsupported stat_type 'median'
```

## Notes

- Returns a brand-new catalog structure (`stat`, `value`) — every other stream from the
  source catalog is dropped.
- `value` is always `Series[float64]`, even for statistics that are conceptually integers
  (`count`, `vocab`).
- `stat_type` matching is case-insensitive but otherwise exact — see the important
  callout above for the full accepted list.

## See Also

- [`Catalog::add_text_features`](add_text_features.md): per-row length/word-count
  features, rather than one corpus-level summary.
- [`Catalog::extract_keywords`](extract_keywords.md): another corpus-level text summary,
  returning keyword frequencies instead of a single statistic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_text_features.html">
    <span class="label">Previous</span>
    <span class="title">add_text_features</span>
  </a>
  <a class="next-link" href="apply.html">
    <span class="label">Next</span>
    <span class="title">apply</span>
  </a>
</div>
