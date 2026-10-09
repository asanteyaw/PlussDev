<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7519a44e084a -->

# DataFrame::skew

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::skew(Axis axis = Axis::Case, std::int64_t min_count = 3) const -> Slate
```

## What It Does

Reduces every numeric feature to its (bias-adjusted) sample skewness — a measure of how
asymmetric the distribution of values is around its mean — returning one `Slate` whose
entries line up with the frame's numeric feature names, the same shape
[`std()`](std.md) and [`var()`](var.md) return. A skewness of `0` describes a
symmetric distribution; a positive value describes a right (long upper) tail; a
negative value describes a left (long lower) tail. Non-numeric features are skipped
entirely.

Nulls are always excluded from the underlying moment computation regardless of any
parameter — there is no `skipnull` toggle here, because skewness has a stricter
statistical floor instead: a feature needs at least **three** observed values before a
skewness is even mathematically defined (a sample of one or two points has no
meaningful notion of asymmetry). `min_count` lets you raise that floor further, but it
can never lower it below `3`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one skewness value per feature. `Axis::Feature` reduces across each row's
features instead, producing one value per case.

**min_count** (`std::int64_t`, default `3`)

The minimum number of non-null observations a feature must have before it is assigned
a real skewness value at all. Passing a value below `3` has no effect — the method
always enforces `max(3, min_count)` — but passing a larger value raises the bar: a
feature with fewer than `min_count` observations reports `NULL` even if it has the
statistically-required minimum of three.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's sample skewness.

## Examples

Example 1. Compute the skewness of every numeric feature, the default `min_count`.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto skewness = orders.skew();
std::cout << skewness;
```

### Output

```text
  order_id  0
     units  0.4180715203
unit_price  0.4534323226
  discount  0.256139086
ptype: Series
```

`order_id` is exactly `0` because its six values are evenly spaced — a perfectly
symmetric run with no skew. `unit_price`, `units`, and `discount` are all positive
because each has a handful of low values pulled up by one or two much larger ones (a
right tail), computed over the observed values only: `unit_price`'s missing case (`5`)
and `discount`'s missing case (`2`) are excluded from the moment computation, the same
null-handling every other statistic in this family applies.

Example 2. Raise `min_count` above the statistical floor to require every case be
present.

```cpp
auto skewness_strict = orders.skew(Axis::Case, 6);
std::cout << skewness_strict;
```

### Output

```text
  order_id  0
     units  0.4180715203
unit_price  NULL
  discount  NULL
ptype: Series
```

`order_id` and `units` have all six values observed, so `min_count = 6` doesn't change
their result. `unit_price` and `discount` each have only five observed values — one
short of the demanded six — so both report `NULL`, even though five values is well
above the statistical minimum of three.

Example 3. See the hard floor in effect: a two-case frame can never produce a
skewness, no matter what `min_count` is passed.

```cpp
auto skewness_too_small = orders.head(2).skew();
std::cout << skewness_too_small;
```

### Output

```text
  order_id  NULL
     units  NULL
unit_price  NULL
  discount  NULL
ptype: Series
```

Every feature has at most two observed values in this two-case slice, below the
`max(3, min_count)` floor `skew()` always enforces — so every feature comes back
`NULL`, independent of what `min_count` you pass.

## Notes

- `skew()`'s result is labeled by feature name and prints like a small labeled
  `Series`, not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither computed as an
  error nor silently coerced.
- A feature whose values have zero variance (every observed value identical) reports a
  skewness of exactly `0` rather than an undefined `0/0` — the implementation detects a
  zero second moment and short-circuits to `0` instead of propagating `NaN`.
- For the corresponding measure of tail weight rather than asymmetry, see
  [`kurtosis()`](kurtosis.md), which shares the same `axis` / `min_count` shape (with a
  floor of `4` instead of `3`).

## See Also

- [`DataFrame::kurtosis`](kurtosis.md): the tail-weight counterpart to this measure of
  asymmetry.
- [`DataFrame::std`](std.md), [`DataFrame::var`](var.md): the second-moment statistics
  this method's third moment builds on.
- [`DataFrame::describe`](describe.md): a broader summary that does not include skew or
  kurtosis, for those on a tighter statistical budget.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shuffle.html">
    <span class="label">Previous</span>
    <span class="title">shuffle</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
