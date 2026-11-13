<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57913c05170b -->

# Catalog::tfidf

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::tfidf(std::string_view field) const
```

## What It Does

Computes a per-row TF-IDF vector norm for a text field, and appends the result as a new
numeric stream, `"<field>_tfidf_norm"`, via [`set()`](set.md) — every original stream,
including `field` itself, is preserved. Each row's term frequencies are weighted by inverse
document frequency across the whole field (smoothed: `log((n + 1) / (df + 1)) + 1`), then
the resulting weighted-term vector is reduced to its Euclidean norm — one number
summarizing how distinctive that row's vocabulary is relative to the rest of the field, not
a full per-term TF-IDF vector.

## Parameters

**field** (`std::string_view`)
    The text stream to compute TF-IDF norms for. Must already exist.

## Returns

`Catalog`
    The original catalog with a new stream, `"<field>_tfidf_norm"`, appended.

## Examples

Example 1. Compute a TF-IDF norm per row.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.tfidf("text");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [text, text_tfidf_norm]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
text                     | Note         | Note             | 3       | [heavy rain on road, clear weather downtown, sto~
text_tfidf_norm          | Series       | Series[float64]  | 3       | [0.84657359028, 0.977538980474, 0.977538980474]  
========================================================================================================================
```

Every word in this small corpus is unique to its own row, so all three rows get a
similarly high (but not identical, due to differing word counts) norm.

## Notes

- Adds one new stream via [`set()`](set.md) rather than replacing the catalog's
  structure.
- The result is a single per-row **norm**, not the full per-term TF-IDF vector — use it as
  a row-level "distinctiveness" score, not as feature columns per vocabulary term.
- `field` must already exist, or the call throws.
- IDF uses add-one smoothing (`log((n + 1) / (df + 1)) + 1`) so a term appearing in every
  row still gets a nonzero weight.

## See Also

- [`Catalog::similarity`](similarity.md): a related bag-of-words text
  metric, comparing two fields directly instead of summarizing one.
- [`Catalog::keywords`](keywords.md): raw term frequencies across the
  field, without IDF weighting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clean.html">
    <span class="label">Previous</span>
    <span class="title">clean</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
