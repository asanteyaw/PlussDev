<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2a4dc54ebff2 -->

# Catalog::add_text_features

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::add_text_features(std::string_view field) const -> Catalog
```

## What It Does

Derives two per-row numeric features from a text field and adds them as new streams,
`"<field>_char_len"` and `"<field>_word_count"`, via [`set()`](set.md) — every original
stream, including `field` itself, is preserved unchanged alongside the two new ones.

## Parameters

**field** (`std::string_view`)
    The text stream to derive features from. Must already exist.

## Returns

`Catalog`
    The original catalog with `"<field>_char_len"` and `"<field>_word_count"` streams
    appended.

## Examples

Example 1. Derive character-length and word-count features from a text field.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.add_text_features("text");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [text, text_char_len, text_word_count]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
text                     | Note         | Note             | 3       | [heavy rain on road, clear weather downtown, sto~
text_char_len            | Series       | Series[int64]    | 3       | [18, 22, 20]                                     
text_word_count          | Series       | Series[int64]    | 3       | [4, 3, 3]                                        
========================================================================================================================
```

`"heavy rain on road"` is 18 characters and 4 words; the two new streams capture that per
row.

## Notes

- Adds two new streams via [`set()`](set.md) — the source field and every other stream are
  preserved unchanged.
- The source field is coerced to text first, so this also works on numeric or temporal
  fields (formatting each value as text before measuring it).
- `field` must already exist, or the call throws.

## See Also

- [`Catalog::aggregate_text_stats`](aggregate_text_stats.md): a corpus-level summary of the
  same length statistics, rather than per-row features.
- [`Catalog::text_preprocess`](text_preprocess.md): normalize text before deriving
  features from it.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="aggregate_text_stats.html">
    <span class="label">Next</span>
    <span class="title">aggregate_text_stats</span>
  </a>
</div>
