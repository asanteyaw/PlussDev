<!-- pluss-hand-authored -->

# Series::grouped_std

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_std(
    const Series& group_ids,
    std::int64_t group_count,
    std::int64_t ddof = 1) const
```

## What It Does

`grouped_std()` returns the standard deviation of the values within each group.

It is exactly the square root of [`grouped_var`](grouped_var.md):

```
grouped_std = sqrt(grouped_var(group_ids, group_count, ddof))
```

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is the standard deviation over
  the values labelled `g`

Everything that holds for [`grouped_var`](grouped_var.md) carries through the
square root: the default `ddof = 1` gives the sample standard deviation, `ddof`
of `0` gives the population form, and a group of size `<= ddof` or an empty
group yields `NaN` (here via `sqrt(NaN)`).

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

**ddof** (`std::int64_t`, default `1`)

Delta degrees of freedom; the variance divisor is `group_size - ddof`.

## Returns

`Series`
    A 1-D `Series` of length `group_count`, dtype `float64` on CPU (`float32`
    off-CPU). Groups of size `<= ddof` and empty groups are `NaN`.

## Gradient

`grouped_std()` is differentiable, through the square root and into
[`grouped_var`](grouped_var.md). As with any `sqrt`, the gradient grows without
bound as a group's spread approaches zero.

## Examples

Example 1. Two groups of four, default `ddof = 1`.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_std(group_ids, 2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 1.9149]
```

These are `sqrt(1.0)` and `sqrt(3.6667)` from the matching
[`grouped_var`](grouped_var.md) example.

Example 2. An empty group is `NaN`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_std(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.7071, 4.2426, nan]
```

Example 3. One-element groups yield `NaN` under the default `ddof`.

```cpp
auto values = pluss::make_series({5.0, 8.0, 2.0});
auto group_ids = pluss::make_series({0, 1, 2}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_std(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, nan, nan]
```

Example 4. The backward pass, through the square root into the variance rule.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

values.grouped_std(group_ids, 2).sum().reverse();

pluss::series::print(std::cout, values.sensitivity());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-0.5000, 0.1667, 0.1667, 0.1667, -0.2611, -0.2611, 0.0870, 0.4352]
```

Within each group the sensitivities still sum to `0`, for the same reason as in
`grouped_var`: a uniform shift does not change spread.

Example 5. An out-of-range label is rejected. The error names the internal
`grouped_var` step.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_std(group_ids, 2);
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

- Same `ddof` story as [`grouped_var`](grouped_var.md): default `1` (sample),
  pass `0` for population.
- Groups of size `<= ddof`, and empty groups, are `NaN`.
- The result is `float64` even for an integer value column.
- The error message on a bad label names `grouped_var`, the step that runs the
  check.

## Summary

`grouped_std()` is `sqrt(grouped_var(...))` per group:

- entry `g` is the standard deviation over values labelled `g`
- `ddof` defaults to `1` (sample); pass `0` for population
- one-element groups (default `ddof`) and empty groups are `NaN`
- differentiable, with the usual `sqrt` blow-up near zero spread

## See Also

- [`Series::grouped_var`](grouped_var.md): the value under the square root, and
  where the `ddof` and empty-group behavior is explained in full.
- [`Series::grouped_mean`](grouped_mean.md): the center this spread is measured
  around.
- [`Series::std`](std.md): the whole-series standard deviation with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_mode.html">
    <span class="label">Previous</span>
    <span class="title">grouped_mode</span>
  </a>
  <a class="next-link" href="grouped_sum.html">
    <span class="label">Next</span>
    <span class="title">grouped_sum</span>
  </a>
</div>
