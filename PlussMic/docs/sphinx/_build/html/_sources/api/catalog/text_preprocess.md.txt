<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4350632b0206 -->

# Catalog::text_preprocess

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::text_preprocess(std::string_view field) const -> Catalog
```

## What It Does

Normalizes whitespace in one text field — tokenizes each row into ASCII words and rejoins
them with single spaces, collapsing any run of whitespace (tabs, multiple spaces, leading
or trailing blanks) into one space each. Implemented as
[`apply(field, ...)`](apply.md), so `field` must already exist; a non-`Note` field is
coerced to text first via `.cast().note()` before tokenizing.

## Parameters

**field** (`std::string_view`)
    The text stream to normalize. Must already exist.

## Returns

`Catalog`
    A new catalog with `field`'s text normalized; every other stream unchanged.

## Examples

Example 1. Normalize already-clean text — words are preserved, joined by single spaces.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.text_preprocess("text");
std::cout << out.at("text");
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | heavy rain on road                                                                                       
1            | clear weather downtown                                                                                   
2            | storm warning issued                                                                                     
========================================================================================================================
```

## Notes

- Implemented as [`apply(field, ...)`](apply.md) — `field` must already exist, or the
  call throws.
- Coerces non-`Note` fields to text first, so it also works on numeric or temporal
  streams (formatting each value as its own text representation before tokenizing).
- Collapses whitespace to single spaces between tokens; it doesn't lowercase, strip
  punctuation attached to words, or remove stopwords.

## See Also

- [`Catalog::add_text_features`](add_text_features.md): derive length/word-count features
  from a text field, rather than normalizing it.
- [`Catalog::tfidf`](tfidf.md) / [`Catalog::extract_keywords`](extract_keywords.md):
  further text-analysis operations that tokenize the same way internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="tfidf.html">
    <span class="label">Next</span>
    <span class="title">tfidf</span>
  </a>
</div>
