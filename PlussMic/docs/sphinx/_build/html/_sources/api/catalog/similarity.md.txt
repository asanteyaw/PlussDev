# Catalog::similarity

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::similarity(std::string_view field1, std::string_view field2, std::string_view method = "cosine") -> Catalog
```

## What It Does

Computes row-wise text similarity between two named text streams.

## Parameters

**field1**
    First named field used by this overload.

**field2**
    Second named field used by this overload.

**method**
    Method selector string understood by this overload.

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

auto out = ct.similarity("text", "text", "cosine");
```

### Output

```text
text__text_similarity -> Series([1.0, 1.0, 1.0])
```

## Practical Behavior

- `similarity(field1, field2, "cosine")` computes row-wise text similarity and appends the result as a numeric stream.
- At the moment the supported similarity method is cosine.
- Both fields must have the same length.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
