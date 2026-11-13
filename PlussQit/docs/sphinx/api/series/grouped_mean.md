<!-- pluss-hand-authored -->

# Series::grouped_mean

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_mean(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_mean()` returns the average of the values in each group.

It is defined exactly as the composition:

```
grouped_mean = grouped_sum / grouped_count
```

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is the arithmetic mean of every
  `self[i]` with `group_ids[i] == g`

Because it divides by [`grouped_count`](grouped_count.md), a group with no
members produces `0 / 0`, which is `NaN`. That is the one behavior that makes
`grouped_mean` different in feel from [`grouped_sum`](grouped_sum.md): the sum
of an empty group is a clean `0`, but its mean is undefined.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

## Returns

`Series`
    A 1-D `Series` of length `group_count`. The dtype is `float64` on CPU
    (`float32` off-CPU). Empty groups are `NaN`.

## Gradient

`grouped_mean()` is differentiable.

Its backward spreads each group mean's sensitivity evenly across that group's
members, divided by the group size. A value in a group of `k` receives `1/k` of
the upstream gradient for that group.

## Examples

Example 1. Three groups with sizes `3`, `2`, `1`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_mean(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.3333, 3.5000, 4.0000]
```

Group `0` is `(1 + 3 + 6) / 3`, group `1` is `(2 + 5) / 2`, group `2` is
`4 / 1`.

Example 2. Exact halves show the plain arithmetic.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
auto group_ids = pluss::make_series({0, 0, 0, 1, 1}, pluss::ScalarType::Int64);

std::cout << values.grouped_mean(group_ids, 2);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 4.5]
```

Example 3. An empty group is `NaN`, not `0`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_mean(group_ids, 4));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.3333, 3.5000, 4.0000, nan]
```

Guard against this by checking [`grouped_count`](grouped_count.md) for zeros
before you use the result.

Example 4. The backward pass gives each member `1 / group_size` of its group
mean's sensitivity.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 0, 0, 1, 1}, pluss::ScalarType::Int64);

values.grouped_mean(group_ids, 2).sum().reverse();

std::cout << "dvalues=" << values.sensitivity();
```

### Output

```text
dvalues=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.33333333, 0.33333333, 0.33333333, 0.5, 0.5]
```

The first three values are in a group of `3`, so each gets `1/3`. The last two
are in a group of `2`, so each gets `1/2`.

Example 5. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_mean(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_sum: group id out of range; expected every id in [0, 2)
```

The check fires inside the `grouped_sum` step that `grouped_mean` calls first,
so the message names that method rather than `grouped_mean`.

## Gotchas And Failure Modes

- An empty group is `NaN`. This is the main difference from
  [`grouped_sum`](grouped_sum.md).
- The result is `float64` even for an integer value column.
- Labels outside `[0, group_count)` raise; the error message names
  `grouped_sum` because that internal step runs the validation first.

## Summary

`grouped_mean()` is `grouped_sum / grouped_count` per group:

- entry `g` is the mean of the values labelled `g`
- empty groups are `NaN`
- always floating point
- differentiable: each member gets `1 / group_size` of its group's sensitivity

## See Also

- [`Series::grouped_sum`](grouped_sum.md): the numerator.
- [`Series::grouped_count`](grouped_count.md): the denominator, and the way to
  detect the empty groups that yield `NaN`.
- [`Series::grouped_var`](grouped_var.md): spread around this mean.
- [`Series::mean`](mean.md): the whole-series average with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_max.html">
    <span class="label">Previous</span>
    <span class="title">grouped_max</span>
  </a>
  <a class="next-link" href="grouped_median.html">
    <span class="label">Next</span>
    <span class="title">grouped_median</span>
  </a>
</div>
