<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e80a8e1df68b -->

# DataFrameGrouped::min

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::min() const -> DataFrame
DataFrameGrouped::min(std::string_view feature) const -> DataFrame
DataFrameGrouped::min(std::initializer_list<std::string_view> features) const -> DataFrame
DataFrameGrouped::min(pluss::RangeView<std::string_view> features) const -> DataFrame
```

## What It Does

Computes per-group minimum for each non-key value feature — the grouped convenience
terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Min` spec for every
remaining value feature.

When you pass one or more feature names, `min(...)` only aggregates those selected
value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to min-reduce. Omit them to reduce every
remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_min` column per non-key value feature.

## Examples

Example 1. Smallest loan amount per region.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Slate(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
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

Example 2. Min only one selected feature on a wider value side.

```cpp
auto wide = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Slate(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
    {"units", Slate(pluss::make_series({5.0, 1.0, 3.0, 4.0}))},
});

auto out2 = wide.group().by({"region"}).min({"units"});
std::cout << out2;
```

### Output (out2)

```text
========================
     | region  units_min
------------------------
0    | North   1
1    | South   3
------------------------
[2 cs x 2 feat]
```

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
