<!-- pluss-hand-authored -->

# DataFrameGrouped::sum

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::sum() const -> DataFrame
```

## What It Does

Sums each non-key value feature within each group — the grouped convenience terminal
equivalent to calling [`agg()`](agg.md) with a `GroupOp::Sum` spec for every
remaining value feature.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_sum` column per non-key value feature.

## Examples

Example 1. Sum both remaining value features per city.

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
               .sum();
std::cout << out;
```

### Output

```text
===================================
     | city   amount_sum  units_sum
-----------------------------------
0    | Accra  180         3        
1    | Tema   90          4        
-----------------------------------
[2 cs x 3 feat]
```

`"Accra"`'s two rows sum to `amount_sum = 100 + 80 = 180` and
`units_sum = 2 + 1 = 3`; `"Tema"` sums to `50 + 40 = 90` and `3 + 1 = 4`.

## Notes

- Intended for additive value features; summing non-numeric features throws, the
  same as [`agg()`](agg.md) with `GroupOp::Sum`.
- Equivalent in spirit to `agg({{"", "amount", GroupOp::Sum}, ...})`, but with
  automatic spec generation (and automatic `{feature}_sum` naming) for every current
  value feature.
- A group where the value feature is entirely null sums to null, not `0`.

## See Also

- [`DataFrameGrouped::mean`](mean.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
