# Catalog::keywords

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::keywords(std::string_view field, int top_n = 10)
```

## What It Does

Extracts high-frequency keywords from one text stream.

## Parameters

**field**
    Name of the target stream in the catalog.

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

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

auto out = ct.keywords("text", 5);
```

### Output

```text
keyword -> Note(["clear", "downtown", "heavy", "issued", "on"])
count   -> Series([1, 1, 1, 1, 1])
```

## Practical Behavior

- `keywords(field, top_n)` tokenizes the text field, counts tokens, sorts by frequency descending then alphabetically, and returns a small keyword/count catalog.
- Use it for quick corpus-level keyword previews rather than row-wise text features.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keys.html">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
