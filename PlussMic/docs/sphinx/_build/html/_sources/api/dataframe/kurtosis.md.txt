<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 091847478bfe -->

# DataFrame::kurtosis

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::kurtosis(Axis axis = Axis::Case, std::int64_t min_count = 4) const -> Slate
```

## What It Does

Reduces every numeric feature to its (bias-adjusted) sample **excess** kurtosis — a
measure of how heavy a distribution's tails are relative to a normal distribution —
returning one `Slate` whose entries line up with the frame's numeric feature names,
the same shape [`skew()`](skew.md) returns. Because this is *excess* kurtosis (not raw
kurtosis), a normal distribution scores `0`: a positive value describes heavier tails
and a sharper peak than normal, a negative value describes lighter tails and a flatter
peak. Non-numeric features are skipped entirely.

As with `skew()`, nulls are always excluded from the moment computation — there is no
`skipnull` parameter — and there is a hard statistical floor instead: a feature needs
at least **four** observed values before excess kurtosis is even defined (the fourth
moment's bias correction divides by `(n - 2)(n - 3)`, which is undefined below that).
`min_count` can raise that floor further but never lower it below `4`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one kurtosis value per feature. `Axis::Feature` reduces across each row's
features instead, producing one value per case.

**min_count** (`std::int64_t`, default `4`)

The minimum number of non-null observations a feature must have before it is assigned
a real kurtosis value at all. Passing a value below `4` has no effect — the method
always enforces `max(4, min_count)` — but passing a larger value raises the bar.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's sample excess kurtosis.

## Examples

Example 1. Compute the excess kurtosis of every numeric feature, the default
`min_count`.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto kurt = orders.kurtosis();
std::cout << kurt;
```

### Output

```text
  order_id  -1.2
     units  -0.8591715976
unit_price  -3.146074484
  discount  -1.988919668
ptype: Series
```

Every feature here has negative excess kurtosis — flatter, thinner-tailed than normal
— which is typical of small, evenly-spread samples like these. `order_id`'s six evenly
spaced values give it the most negative (most "platykurtic") result of the four.
`unit_price`'s and `discount`'s values are computed over their five observed values
only; the missing case in each (case `5` for `unit_price`, case `2` for `discount`) is
excluded from the moment computation.

Example 2. Raise `min_count` above the statistical floor to require every case be
present.

```cpp
auto kurt_strict = orders.kurtosis(Axis::Case, 6);
std::cout << kurt_strict;
```

### Output

```text
  order_id  -1.2
     units  -0.8591715976
unit_price  NULL
  discount  NULL
ptype: Series
```

`order_id` and `units` have all six values observed, so `min_count = 6` doesn't change
their result. `unit_price` and `discount` each have only five observed values, one
short of six, so both report `NULL`.

Example 3. See the hard floor in effect: a three-case frame can never produce a
kurtosis, no matter what `min_count` is passed.

```cpp
auto kurt_too_small = orders.head(3).kurtosis();
std::cout << kurt_too_small;
```

### Output

```text
  order_id  NULL
     units  NULL
unit_price  NULL
  discount  NULL
ptype: Series
```

Every feature has at most three observed values in this three-case slice, below the
`max(4, min_count)` floor `kurtosis()` always enforces, so every feature comes back
`NULL` regardless of `min_count`.

## Notes

- `kurtosis()`'s result is labeled by feature name and prints like a small labeled
  `Series`, not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither computed as an
  error nor silently coerced.
- A feature whose values have zero variance reports a kurtosis of exactly `0` rather
  than an undefined `0/0`, the same zero-variance short-circuit `skew()` applies.
- This is **excess** kurtosis (normal distribution scores `0`), not raw kurtosis
  (normal distribution scores `3`) — the `-3.0` term baked into the formula is what
  makes it "excess."

## See Also

- [`DataFrame::skew`](skew.md): the asymmetry counterpart to this measure of tail
  weight, sharing the same `axis` / `min_count` shape (with a floor of `3` instead of
  `4`).
- [`DataFrame::std`](std.md), [`DataFrame::var`](var.md): the second-moment statistics
  this method's fourth moment builds on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
