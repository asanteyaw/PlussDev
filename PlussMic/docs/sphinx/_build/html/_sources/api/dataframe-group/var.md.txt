<!-- pluss-hand-authored -->

# DataFrameGrouped::var

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::var() const -> DataFrame
```

## What It Does

Computes grouped sample variance (`n - 1` denominator) for each non-key value
feature — the grouped convenience terminal equivalent to calling [`agg()`](agg.md)
with a `GroupOp::Var` spec for every remaining value feature. Like [`std()`](std.md),
a group needs at least **two** observed values to produce a defined variance; a
group with zero or one observed value produces null instead of `0`.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_var` column per non-key value feature.

## Examples

Example 1. One region with three observations, one with a single observation.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "North", "South"}))},
    {"amount", Thread(pluss::make_series({10.0, 20.0, 30.0, 50.0}))},
});

auto out = loans.features({"region", "amount"})
               .group()
               .by({"region"})
               .var();
std::cout << out;
```

### Output

```text
=========================
     | region  amount_var
-------------------------
0    | North   100       
1    | South   NULL      
-------------------------
[2 cs x 2 feat]
```

`"North"`'s three values (`10, 20, 30`, mean `20`) have squared deviations
`100 + 0 + 100 = 200`; dividing by `n - 1 = 2` gives a sample variance of `100` — the
square of the `10` reported by [`std()`](std.md) on the same data. `"South"` has only
one value, so `amount_var` is `NULL`, not `0`.

## Notes

- `var()` and `std()` are related by `std = sqrt(var)`; both null out under the same
  condition (fewer than two observed values in the group).
- Use it only on value features where variance is meaningful (numeric, additive
  quantities).

## See Also

- [`DataFrameGrouped::std`](std.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
