<!-- pluss-hand-authored -->

# DataFrameGrouped::mean

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::mean() const -> DataFrame
```

## What It Does

Computes group-wise arithmetic mean for each non-key value feature — the grouped
convenience terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Mean`
spec for every remaining value feature.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_mean` column per non-key value feature.

## Examples

Example 1. Mean of both remaining value features per city.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Thread(pluss::make_series({100.0, 80.0, 50.0, 40.0}))},
    {"units", Thread(pluss::make_series({2.0, 1.0, 3.0, 1.0}))},
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

## Notes

- The convenience terminal is best when all remaining value features should take the
  same grouped operation.
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
