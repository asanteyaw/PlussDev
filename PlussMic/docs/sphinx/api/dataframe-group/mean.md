<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5eb86fdf4cfb -->

# DataFrameGrouped::mean

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::mean() const -> DataFrame
DataFrameGrouped::mean(std::string_view feature) const -> DataFrame
DataFrameGrouped::mean(std::initializer_list<std::string_view> features) const -> DataFrame
DataFrameGrouped::mean(pluss::RangeView<std::string_view> features) const -> DataFrame
```

## What It Does

Computes group-wise arithmetic mean for each non-key value feature — the grouped
convenience terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Mean`
spec for every remaining value feature.

When you pass one or more feature names, `mean(...)` only aggregates those selected
value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to average. Omit them to average every
remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_mean` column per non-key value feature.

## Examples

Example 1. Mean of both remaining value features per city.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Slate(pluss::make_series({100.0, 80.0, 50.0, 40.0}))},
    {"units", Slate(pluss::make_series({2.0, 1.0, 3.0, 1.0}))},
});

auto out = sales.features({"city", "amount", "units"})
               .group()
               .by({"city"})
               .mean();
std::cout << out;
```

### Output

```text
=====================================
     | city   amount_mean  units_mean
-------------------------------------
0    | Accra  90           1.5       
1    | Tema   45           2         
-------------------------------------
[2 cs x 3 feat]
```

`"Accra"`'s `amount_mean` is `(100 + 80) / 2 = 90`; its `units_mean` is
`(2 + 1) / 2 = 1.5`, printed with the fractional part since it is not a whole number.
`"Tema"` averages to `45` and `2` respectively.

Example 2. Average only one selected value feature.

```cpp
auto out2 = sales.group()
                .by({"city"})
                .mean({"amount"});
std::cout << out2;
```

### Output

```text
========================
     | city   amount_mean
------------------------
0    | Accra  90
1    | Tema   45
------------------------
[2 cs x 2 feat]
```

## Notes

- The convenience terminal is best when all remaining value features should take the
  same grouped operation.
- Passing `mean({"amount"})` or `mean("amount")` is the targeted form when you only
  want a subset of the value side.
- If you need one mean and one count in the same output, use [`agg()`](agg.md)
  instead.
- A group where the value feature is entirely null means to null, not `0`.

## See Also

- [`DataFrameGrouped::median`](median.md)
- [`DataFrameGrouped::sum`](sum.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
