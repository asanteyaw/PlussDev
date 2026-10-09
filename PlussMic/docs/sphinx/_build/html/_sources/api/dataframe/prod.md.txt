<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9175300edda5 -->

# DataFrame::prod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::prod(Axis axis = Axis::Case, bool skipnull = true) const -> Slate
```

## What It Does

Reduces every numeric feature to the product of its values, returning one `Slate`
whose entries line up with the frame's numeric feature names — the same shape
[`sum()`](sum.md) returns, but multiplying instead of adding. Non-numeric features are
skipped entirely, the same rule every other single-statistic reducer in this family
follows.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) multiplies down each feature's cases,
producing one product per feature. `Axis::Feature` multiplies across each row's
features instead, producing one product per case — for example, multiplying a `units`
column by a `unit_price` column row-by-row to get each row's line total.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that reduction's result:

- `true` (the default): missing values are excluded from the product entirely, as if
  they were never part of the multiplication.
- `false`: a single missing value poisons the entire reduction it participates in,
  which comes back `NULL`.

## Returns

`Slate`
    One entry per participating numeric feature (`Axis::Case`) or per case
    (`Axis::Feature`), holding the product of that reduction's values.

## Examples

Example 1. Multiply every numeric feature's values, skipping missing values (the
default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto products = orders.prod();
std::cout << products;
```

### Output

```text
  order_id  1.228251417e+12
     units  240
unit_price  8.438976e+11
  discount  2.4e-06
ptype: Series
```

`unit_price`'s missing observation (case `5`) is excluded from the product, the same
way [`mean()`](mean.md) and [`std()`](std.md) treat that column's missing value — the
product runs over the remaining five prices only. Note the scientific notation on
`order_id`, `unit_price`, and `discount`: `prod()`'s outputs grow (or shrink) fast, and
the same `{:.10g}` formatting every `Slate`/`Series` render uses switches to
scientific notation once a value's magnitude crosses ten significant digits either way.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto products_strict = orders.prod(Axis::Case, false);
std::cout << products_strict;
```

### Output

```text
  order_id  1.228251417e+12
     units  240
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, `unit_price` and `discount` — the two features that each have
one missing value — come back `NULL` entirely, while `order_id` and `units` are
unaffected.

Example 3. Use `Axis::Feature` to multiply across a row's features instead of down a
feature's cases — here, computing each order's line total as `units * unit_price`.

```cpp
auto line_totals = orders.features({"units", "unit_price"}).prod(Axis::Feature);
std::cout << line_totals;
```

### Output

```text
0  2400
1  227.5
2  1150
3  960
4  168
5  2
ptype: Series
```

Cases `0` through `4` show the expected `units * unit_price` line total. Case `5` is
`2`, not `NULL` — its `unit_price` is missing, and with the default `skipnull = true`
the missing factor is simply excluded from that row's product rather than poisoning
it, leaving only `units` (`2`) to multiply. Pass `false` as the second argument if a
missing price should invalidate the whole row's total instead.

## Notes

- `prod()`'s result is labeled by feature name (`Axis::Case`) or by case index
  (`Axis::Feature`), and prints like a small labeled `Series`, not like a row sliced
  from the source frame.
- Non-numeric features never appear in the result; they are neither multiplied as an
  error nor silently coerced.
- A product that would overflow or underflow `double` precision follows ordinary IEEE
  754 behavior (`inf` / `0.0`), the same as any other floating-point multiplication in
  the library.

## See Also

- [`DataFrame::sum`](sum.md): the additive counterpart, sharing the same `axis` /
  `skipnull` shape.
- [`DataFrame::cumprod`](cumprod.md): the running product at every position, instead of
  one final product per feature.
- [`DataFrame::mean`](mean.md), [`DataFrame::std`](std.md): the other single-statistic
  reductions in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prep_sql.html">
    <span class="label">Previous</span>
    <span class="title">prep_sql</span>
  </a>
  <a class="next-link" href="ptypes.html">
    <span class="label">Next</span>
    <span class="title">ptypes</span>
  </a>
</div>
