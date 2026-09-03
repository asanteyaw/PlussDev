<!-- pluss-hand-authored -->

# Series::grouped_var

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::grouped_var(
    const Series& group_ids,
    std::int64_t group_count,
    std::int64_t ddof = 1) const -> Series
```

## What It Does

`grouped_var()` returns the variance of the values within each group.

For a group with members `x` of size `k` and mean `m`, the result is

```
sum_i (x_i - m)^2 / (k - ddof)
```

computed in the numerically stable "sum of squares minus square of sum" form
and then clamped at `0` so that rounding cannot produce a small negative
variance.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is the variance over the values
  labelled `g`

The default `ddof = 1` is the sample (unbiased) variance, dividing by `k - 1`.
Pass `ddof = 0` for the population variance, dividing by `k`. This default is
the opposite of the moving-window [`movvar`](../series-stat/movvar.md), which
always uses population normalization.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

**ddof** (`std::int64_t`, default `1`)

The delta degrees of freedom. The divisor is `group_size - ddof`. Must be
`>= 0`.

## Returns

`Series`
    A 1-D `Series` of length `group_count`, dtype `float64` on CPU (`float32`
    off-CPU). A group whose size is `<= ddof` (for example a one-element group
    with the default `ddof = 1`) divides by zero or a negative number and
    yields `NaN`. An empty group is also `NaN`.

## Gradient

`grouped_var()` is differentiable. The sensitivity of a group's variance flows
back to each member proportionally to its deviation from the group mean.

## Examples

Example 1. Two groups of four. Default `ddof = 1`.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_var(group_ids, 2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 3.6667]
```

Group `0` is `{2, 4, 4, 4}` with mean `3.5`, sum of squared deviations `3`,
divided by `4 - 1`. Group `1` is `{5, 5, 7, 9}` with mean `6.5`, sum of squared
deviations `11`, divided by `3`.

Example 2. The same data with `ddof = 0` divides by the group size instead.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_var(group_ids, 2, 0));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.7500, 2.7500]
```

Example 3. A one-element group divides by `1 - 1 = 0` under the default and
yields `NaN`.

```cpp
auto values = pluss::make_series({5.0, 8.0, 2.0});
auto group_ids = pluss::make_series({0, 1, 2}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_var(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, nan, nan]
```

Example 4. An empty group is also `NaN`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_var(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5000, 18.0000, nan]
```

Example 5. The backward pass. Each member's gradient is proportional to its
deviation from the group mean.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

values.grouped_var(group_ids, 2).sum().reverse();

pluss::series::print(std::cout, values.sensitivity());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-1.0000, 0.3333, 0.3333, 0.3333, -1.0000, -1.0000, 0.3333, 1.6667]
```

Within each group the sensitivities sum to `0`, because shifting every member
by the same amount does not change the variance.

Example 6. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_var(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_var: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- The default is `ddof = 1` (sample variance). Pass `ddof = 0` for population
  variance. This differs from [`movvar`](../series-stat/movvar.md), which is
  always population.
- A group of size `<= ddof`, and any empty group, yields `NaN`.
- The result is clamped at `0`, so you will never see a small negative value
  from rounding, but you also cannot distinguish "exactly zero spread" from
  "clamped".
- The result is `float64` even for an integer value column.

## Summary

`grouped_var()` is the within-group variance:

- entry `g` is `sum (x - mean)^2 / (group_size - ddof)` over values labelled `g`
- `ddof` defaults to `1` (sample); pass `0` for population
- one-element groups (default `ddof`) and empty groups are `NaN`
- clamped at `0`; differentiable

## See Also

- [`Series::grouped_std`](grouped_std.md): the square root of this.
- [`Series::grouped_mean`](grouped_mean.md): the center this spread is measured
  around.
- [`Series::var`](var.md): the whole-series variance with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_sum.html">
    <span class="label">Previous</span>
    <span class="title">grouped_sum</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
