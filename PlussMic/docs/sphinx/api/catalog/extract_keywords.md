<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9cf96bdfbcc4 -->

# Catalog::extract_keywords

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::extract_keywords(std::string_view field, int top_n = 10) const -> Catalog
```

## What It Does

Tokenizes one text field across every row, counts how often each word occurs across the
whole field, and returns a **brand-new** two-stream catalog (`keyword`, `count`) sorted by
count descending, then alphabetically for ties. This is a corpus-level summary, not a
per-row feature — the result's row count is the number of distinct keywords found, not the
original catalog's row count, and every other stream from the source catalog is dropped.

```{important} top_n <= 0 means unlimited, not zero
`top_n` only trims the result when it's strictly positive. A `top_n` of `0` (the value
used by several other Catalog methods to mean "everything") or any negative number returns
every distinct keyword found, with no cap at all.
```

## Parameters

**field** (`std::string_view`)
    The text stream to extract keywords from. Must already exist.

**top_n** (`int`, default `10`)
    Caps the result to the `top_n` most frequent keywords. `0` or negative means no cap.

## Returns

`Catalog`
    A new two-stream catalog: `keyword` (`Note`) and `count` (`Series[int64]`), one row per
    distinct keyword, sorted most-frequent first.

## Examples

Example 1. Extract the top 2 keywords from a short corpus — every word here occurs once,
so ties break alphabetically.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.extract_keywords("text", 2);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [keyword, count]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
keyword                  | Note         | Note             | 2       | [clear, downtown]                                
count                    | Series       | Series[int64]    | 2       | [1, 1]                                           
========================================================================================================================
```

Example 2. An unknown field throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({"heavy rain"})),
});

try {
  auto out = ct.extract_keywords("zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::extract_keywords: unknown key 'zzz'
```

## Notes

- Returns a brand-new catalog structure (`keyword`, `count`) — every other stream from the
  source catalog is dropped, and the row count changes to the distinct-keyword count.
- `top_n <= 0` means unlimited — see the important callout above.
- Ties in count break alphabetically by keyword.

## See Also

- [`Catalog::tfidf`](tfidf.md): a per-row, weighted alternative to raw keyword counts.
- [`Catalog::text_preprocess`](text_preprocess.md): normalize text before extracting
  keywords, if needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="end.html">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
