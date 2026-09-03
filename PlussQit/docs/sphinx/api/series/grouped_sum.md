<!-- pluss-hand-authored -->

# Series::grouped_sum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::grouped_sum(const Series& group_ids, std::int64_t group_count) const -> Series
```

## What It Does

`grouped_sum()` is a segmented reduction. It adds up the values in `self` that
share a group label and returns one total per group.

The shape contract is the important part:

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D `Series` of the same length `n`; `group_ids[i]` is the
  group that value `self[i]` belongs to
- `group_count` is the number of groups
- the result is a 1-D `Series` of length `group_count`, where entry `g` is the
  sum of every `self[i]` with `group_ids[i] == g`

So `grouped_sum` collapses `n` values down to `group_count` totals in a single
pass. It is the building block the rest of the family is written on top of:
[`grouped_mean`](grouped_mean.md) is `grouped_sum / grouped_count`, and
[`grouped_var`](grouped_var.md) combines the sum of values with the sum of
squares.

A group that no element maps to contributes a total of `0`.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value. Any integral or boolean dtype is accepted and read as an
integer. Every label must lie in `[0, group_count)`; a negative label or one
`>= group_count` raises rather than being wrapped or clamped.

**group_count** (`std::int64_t`)

The number of output groups. Must be `>= 0`. The result length is exactly this
value. `group_count = 0` is only valid for an empty input, because any label
would otherwise be out of range.

## Returns

`Series`
    A 1-D `Series` of length `group_count`. On CPU the accumulation and result
    dtype is `float64` (`float32` on other backends), regardless of the input
    dtype, so summing an integer column still returns a floating-point result.

## Gradient

`grouped_sum()` is differentiable.

Its backward is a scatter: each value's sensitivity is the sensitivity of the
group total it landed in. If two values share a group, they receive the same
upstream gradient.

## Examples

Example 1. Six values, three groups. Group `0` collects positions `0`, `2`,
and `5`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_sum(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10.0000, 7.0000, 4.0000]
```

Group `0` is `1 + 3 + 6 = 10`, group `1` is `2 + 5 = 7`, group `2` is just `4`.

Example 2. An integer value column still returns `float64`.

```cpp
auto values = pluss::make_series({1, 2, 3, 4}, pluss::ScalarType::Int64);
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_sum(group_ids, 2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.0000, 7.0000]
```

Example 3. Asking for more groups than appear in the labels. The unused group
sums to `0`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_sum(group_ids, 4));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 7.0000, 4.0000, 0.0000]
```

Example 4. The backward pass routes each total's sensitivity back to its
members. Here the three group totals are weighted `10`, `100`, `1000` before
the reduction.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 1, 0, 2, 1}, pluss::ScalarType::Int64);

auto totals = values.grouped_sum(group_ids, 3);
(totals * pluss::make_series({10.0, 100.0, 1000.0})).sum().reverse();

std::cout << "dvalues=" << values.sensitivity();
```

### Output

```text
dvalues=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10, 100, 10, 1000, 100]
```

Positions `0` and `2` are in group `0`, so both get `10`. Positions `1` and
`4` are in group `1`, so both get `100`. Position `3` alone is in group `2` and
gets `1000`.

Example 5. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 5, 1}, pluss::ScalarType::Int64);
  values.grouped_sum(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_sum: group id out of range; expected every id in [0, 2)
```

A negative label is rejected the same way; it is never treated as an index from
the end.

Example 6. A length mismatch between values and labels is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 1}, pluss::ScalarType::Int64);
  values.grouped_sum(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_sum: group_ids length must match values length
```

## Gotchas And Failure Modes

- `self` and `group_ids` must both be 1-D and the same length.
- Every label must be in `[0, group_count)`. Negative labels are not
  from-the-end indices here; they raise.
- The result is `float64` even for an integer value column.
- An empty group sums to `0`, which is indistinguishable from a group whose
  members happen to cancel. Use [`grouped_count`](grouped_count.md) if you need
  to tell them apart.

## Summary

`grouped_sum()` reduces `n` labelled values to `group_count` totals:

- entry `g` is the sum over values labelled `g`
- empty groups are `0`
- always returns `float64` (`float32` off-CPU)
- differentiable: each value gets its group total's sensitivity

## See Also

- [`Series::grouped_count`](grouped_count.md): how many values landed in each
  group.
- [`Series::grouped_mean`](grouped_mean.md): `grouped_sum` divided by
  `grouped_count`.
- [`Series::sum`](sum.md): the whole-series reduction with no grouping.
- [`Series::scatter_add`](scatter_add.md): the lower-level indexed accumulation
  `grouped_sum` is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_std.html">
    <span class="label">Previous</span>
    <span class="title">grouped_std</span>
  </a>
  <a class="next-link" href="grouped_var.html">
    <span class="label">Next</span>
    <span class="title">grouped_var</span>
  </a>
</div>
