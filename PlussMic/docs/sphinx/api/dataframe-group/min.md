<!-- pluss-hand-authored -->

# DataFrameGrouped::min

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::min() const -> DataFrame
```

## What It Does

Computes per-group minimum for each non-key value feature — the grouped convenience
terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Min` spec for every
remaining value feature.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_min` column per non-key value feature.

## Examples

Example 1. Smallest loan amount per region.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Thread(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
});

auto out = loans.features({"region", "amount"})
               .group()
               .by({"region"})
               .min();
std::cout << out;
```

### Output

```text
=========================
     | region  amount_min
-------------------------
0    | North   100       
1    | South   80        
-------------------------
[2 cs x 2 feat]
```

`"North"`'s smallest amount between `100` and `250` is `100`; `"South"`'s smallest
between `80` and `120` is `80`.

## Notes

- A group where the value feature is entirely null has a null minimum, not `0`.
- Works on any numeric value feature; comparing non-numeric features throws, same as
  [`agg()`](agg.md) with `GroupOp::Min`.

## See Also

- [`DataFrameGrouped::max`](max.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="median.html">
    <span class="label">Previous</span>
    <span class="title">median</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
