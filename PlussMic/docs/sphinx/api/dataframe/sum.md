<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2d381d2bfe75 -->

# DataFrame::sum

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::sum(Axis axis = Axis::Case, bool skipnull = true) const -> Thread
```

## What It Does

Reduces every numeric feature to a single total, returning one `Thread` whose entries
line up with the frame's numeric feature names — a column-wise sum, one number per
feature. Non-numeric features (`Note`, `Timeline`, `TimeIndex`) are skipped entirely
rather than coerced into a total that wouldn't mean anything.

Note the return type: `sum()` returns a `Thread`, not a `DataFrame`. It behaves like a
labeled, one-dimensional result — printed the way a `Thread`/`Series` prints, with each
feature name as its row label and a `ptype:` footer — not like the boxed, multi-column
table that `DataFrame`-returning methods produce.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) sums down each feature's cases,
producing one total per feature — this is the usual reading, and the one shown below.
`Axis::Feature` sums across each row's features instead, producing one total per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's total:

- `true` (the default): missing values are ignored; the total is the sum of whatever was
  actually observed for that feature.
- `false`: a single missing value in a feature poisons that feature's entire total,
  which comes back `NULL`.

## Returns

`Thread`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's total.

## Examples

Example 1. Sum every numeric feature, skipping missing values (the default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto totals = orders.sum();
std::cout << totals;
```

### Output

```text
  order_id  621
     units  17
unit_price  2757.5
  discount  0.4
ptype: Series
```

`unit_price`'s total (`2757.5`) is the sum of its five *observed* values — case `5`'s
missing entry simply didn't contribute, rather than forcing the whole total to `NULL`.
`order_id` and `units`, which have no missing values, sum normally.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto totals_strict = orders.sum(pluss::table::Axis::Case, false);
std::cout << totals_strict;
```

### Output

```text
  order_id  621
     units  17
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, both `unit_price` and `discount` — the two features that each
have exactly one missing value — come back `NULL` entirely, because at least one input
to their total was missing. `order_id` and `units` are unaffected, since neither had any
missing values to begin with.

## Notes

- `sum()`'s result is labeled by feature name, not by case — printing it looks like a
  small labeled `Series`, not like a row from the source frame.
- Non-numeric features never appear in the result at all; they are not summed as `0` or
  reported as an error, they are simply absent from the returned `Thread`.
- For a summary that also frees you from committing to one specific statistic, see
  [`describe()`](describe.md), which reports `sum`-adjacent statistics (`mean`, `std`,
  quartiles) all at once.

## See Also

- [`DataFrame::mean`](mean.md), [`DataFrame::std`](std.md), [`DataFrame::min`](min.md),
  [`DataFrame::max`](max.md): the other single-statistic feature reductions, sharing the
  same `axis` / `skipnull` shape.
- [`DataFrame::cumsum`](cumsum.md): a running total per case, instead of one final total
  per feature.
- [`DataFrame::describe`](describe.md): several statistics per feature in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
