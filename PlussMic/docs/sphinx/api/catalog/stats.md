# Catalog::stats

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::stats(std::string_view field, std::string_view stat_type)
```

## What It Does

Computes one text summary statistic for a named text stream.

## Parameters

**field**
    Name of the target stream in the catalog.

**stat_type**
    Statistic selector string understood by this overload.

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

auto out = ct.stats("text", "mean");
```

### Output

```text
text_mean_char_len -> Series([20.0])
```

## Practical Behavior

- `stats(field, stat_type)` collapses one text field into a one-value summary stream.
- Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
