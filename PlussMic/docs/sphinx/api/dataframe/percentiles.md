<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 38c3847c43a8 -->

# DataFrame::percentiles

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::percentiles(pluss::RangeView<double> points = {0.25, 0.5, 0.75}, std::int64_t min_count = 1) const -> DataFrame
```

## What It Does

Computes one or more quantile points for every numeric feature, returning a new
`DataFrame` — not a `Slate` — with one row per requested point and one column per
participating numeric feature. This is the one statistical reducer in this family
whose result is boxed and multi-column rather than a bare labeled `Series`, because it
naturally produces a small table (points × features) rather than a single value per
feature.

Each requested point is interpolated linearly between the two nearest ranked
observations (the conventional definition used for `0.5` as the median, and the same
interpolation [`median()`](median.md) itself uses internally). Non-numeric features
are skipped entirely; a frame with no numeric features raises an error, since there
would be nothing to tabulate.

## Parameters

**points** (`pluss::RangeView<double>`, default `{0.25, 0.5, 0.75}`)

The quantile points to compute, each in `[0, 1]`. The default reproduces the quartiles
`describe()` reports. Each point becomes one row of the result frame, labeled with a
percentage string (`0.25` becomes the row label `"25%"`, `0.9` becomes `"90%"`, and so
on) — the row order follows the order you pass the points in, not sorted order.

**min_count** (`std::int64_t`, default `1`)

The minimum number of non-null observations a feature must have before any of its
requested points are computed. A feature with fewer observed values than `min_count`
reports `NULL` for **every** requested point in that column — the check applies once
per feature, not once per point.

## Returns

`DataFrame`
    A new frame with one row per requested point (labeled by percentage string) and
    one column per participating numeric feature, holding that feature's value at
    that point.

## Examples

Example 1. Compute the default quartiles for every numeric feature.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto quartiles = orders.percentiles();
std::cout << quartiles;
```

### Output

```text
============================================
     | order_id  units  unit_price  discount
--------------------------------------------
25%  | 102.25    2      45.5        0.05    
50%  | 103.5     2.5    320         0.08    
75%  | 104.75    3.75   1150        0.1     
--------------------------------------------
[3 cs x 4 feat]
```

`unit_price` and `discount` are each computed over their five observed values (case
`5` and case `2` respectively are missing and excluded before ranking), the same
null-handling [`median()`](median.md) and [`std()`](std.md) apply. These are the exact
same `25%` / `50%` / `75%` numbers [`describe()`](describe.md) reports for this frame —
`percentiles()` is what `describe()` calls internally for its quartile rows.

Example 2. Request a custom set of points — here, the 10th and 90th percentiles
instead of quartiles.

```cpp
auto deciles = orders.percentiles({0.1, 0.9});
std::cout << deciles;
```

### Output

```text
============================================
     | order_id  units  unit_price  discount
--------------------------------------------
10%  | 101.5     1.5    43.4        0.05    
90%  | 105.5     4.5    1180        0.112   
--------------------------------------------
[2 cs x 4 feat]
```

The result has exactly two rows, `"10%"` and `"90%"`, in the order the points were
passed in — `points` controls both how many rows come back and what they're labeled.

Example 3. Use `min_count` to suppress features that don't have enough observations.

```cpp
auto strict = orders.percentiles({0.5}, 6);
std::cout << strict;
```

### Output

```text
============================================
     | order_id  units  unit_price  discount
--------------------------------------------
50%  | 103.5     2.5    NULL        NULL    
--------------------------------------------
[1 cs x 4 feat]
```

`order_id` and `units` have all six values observed, so `min_count = 6` doesn't affect
them. `unit_price` and `discount` each have only five observed values — one short of
six — so **every** requested point for those two columns comes back `NULL`, not just
the ones that would have needed the missing observation.

## Notes

- Unlike every other reducer documented alongside it (`sum`, `mean`, `std`, `var`,
  `skew`, `kurtosis`), `percentiles()` returns a `DataFrame`, not a `Slate` — because
  its result is inherently a small table (points × features), not one value per
  feature.
- Non-numeric features never appear in the result; they are neither computed as an
  error nor silently coerced.
- Row labels are percentage strings derived from `points`, not the numeric point
  values themselves — `0.25` becomes `"25%"`.
- For a single statistic instead of a table of points, see [`median()`](median.md)
  (equivalent to the `0.5` point) or [`describe()`](describe.md) for a broader summary
  that includes the default quartiles alongside other statistics.

## See Also

- [`DataFrame::median`](median.md): the single-point special case (`0.5`) of this
  method.
- [`DataFrame::describe`](describe.md): reports the default quartiles from this method
  alongside count, mean, std, min, and max.
- [`DataFrame::mode`](mode.md): the other numeric reducer in this family that returns a
  `DataFrame` rather than a `Slate`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="observe.html">
    <span class="label">Previous</span>
    <span class="title">observe</span>
  </a>
  <a class="next-link" href="pivot.html">
    <span class="label">Next</span>
    <span class="title">pivot</span>
  </a>
</div>
