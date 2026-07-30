<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fed44e93bc9 -->

# Catalog::compute_similarity

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::compute_similarity(std::string_view field1, std::string_view field2, std::string_view method = "cosine") const -> Catalog
```

## What It Does

Computes row-wise text similarity between two fields — bag-of-words cosine similarity by
default — and appends the result as a new numeric stream named
`"<field1>__<field2>_similarity"`, added via [`set()`](set.md). Unlike
[`extract_keywords()`](extract_keywords.md), this doesn't replace the catalog's structure:
every original stream is preserved, with the similarity column added alongside.

```{important} method currently only accepts "cosine"
Despite the parameter existing for future flexibility, only `method = "cosine"` (the
default) is currently supported — any other value throws immediately.
```

## Parameters

**field1** (`std::string_view`)
    The first text field. Must already exist.

**field2** (`std::string_view`)
    The second text field. Must already exist, and be the same length as `field1`.

**method** (`std::string_view`, default `"cosine"`)
    The similarity method — only `"cosine"` is currently supported.

## Returns

`Catalog`
    The original catalog with a new stream, `"<field1>__<field2>_similarity"`, appended.

## Examples

Example 1. Compute row-wise cosine similarity between two text fields with partial word
overlap.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_note({"heavy rain today", "sunny clear sky"})),
    "b"_k = pluss::kindle::Stream(pluss::make_note({"heavy rain tonight", "cloudy sky today"})),
});

auto out = ct.compute_similarity("a", "b");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [a, b, a__b_similarity]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
a                        | Note         | Note             | 2       | [heavy rain today, sunny clear sky]              
b                        | Note         | Note             | 2       | [heavy rain tonight, cloudy sky today]           
a__b_similarity          | Series       | Series[float64]  | 2       | [0.666666666667, 0.333333333333]                 
========================================================================================================================
```

Row `0` shares 2 of its 3 words (`"heavy"`, `"rain"`) with `b`, giving a high similarity;
row `1` shares only `"sky"`, giving a lower one.

Example 2. Requesting any method other than `"cosine"` throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_note({"text"})),
    "b"_k = pluss::kindle::Stream(pluss::make_note({"text"})),
});

try {
  auto out = ct.compute_similarity("a", "b", "jaccard");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::compute_similarity currently supports only method='cosine'
```

## Notes

- Adds a new stream via [`set()`](set.md) rather than replacing the catalog's structure —
  every original stream survives.
- Only `method = "cosine"` is currently supported — see the important callout above.
- `field1` and `field2` must be the same length, or the call throws.
- The new column's name is derived from both field names:
  `"<field1>__<field2>_similarity"`.

## See Also

- [`Catalog::tfidf`](tfidf.md): a related bag-of-words text metric.
- [`Catalog::set`](set.md): the underlying mechanism used to add the result column.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cloc.html">
    <span class="label">Previous</span>
    <span class="title">cloc</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
