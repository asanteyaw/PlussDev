# Catalog::featurize

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::featurize(std::string_view field) -> Catalog
```

## What It Does

Adds derived text features from one named text stream.

## Parameters

**field**
    Name of the target stream in the catalog.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.featurize("text");
```

### Output

```text
ct.head():
text -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])

out.head():
text            -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])
text_char_len   -> Series([18, 22, 20])
text_word_count -> Series([4, 3, 3])
```

## Practical Behavior

- `featurize(field)` adds `<field>_char_len` and `<field>_word_count` streams.
- The source field is coerced through note/text handling before the text statistics are computed.



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
