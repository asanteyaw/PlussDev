<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 413beeea0a2e -->

# DataFrameGrouped::max

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::max() const -> DataFrame
```

## What It Does

Computes per-group maximum for each non-key value feature — the grouped convenience
terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Max` spec for every
remaining value feature.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_max` column per non-key value feature.

## Examples

Example 1. Largest loan amount per region.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Thread(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
});

auto out = loans.features({"region", "amount"})
               .group()
               .by({"region"})
               .max();
std::cout << out;
```

### Output

```text
=========================
     | region  amount_max
-------------------------
0    | North   250       
1    | South   120       
-------------------------
[2 cs x 2 feat]
```

`"North"`'s largest amount between `100` and `250` is `250`; `"South"`'s largest
between `80` and `120` is `120`.

## Notes

- A group where the value feature is entirely null has a null maximum, not `0`.
- Works on any numeric value feature; comparing non-numeric features throws, same as
  [`agg()`](agg.md) with `GroupOp::Max`.

## See Also

- [`DataFrameGrouped::min`](min.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
