<!-- pluss-hand-authored -->

# Series::grouped_max

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_max(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_max()` returns the largest value in each group.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is `max { self[i] : group_ids[i] == g }`

The reduction starts each group's accumulator at `-inf` and folds in the
members with a running maximum. That starting value is what a group with no
members is left holding, so an empty group reports `-inf`.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

## Returns

`Series`
    A 1-D `Series` of length `group_count`, dtype `float64` on CPU (`float32`
    off-CPU). Empty groups are `-inf`.

## Gradient

`grouped_max()` is differentiable.

Backward sends each group maximum's sensitivity to the member that achieved it.
When several members tie for the maximum, the gradient is split evenly among
them.

## Examples

Example 1. Largest value per group, three groups.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

std::cout << values.grouped_max(group_ids, 3);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6, 5, 4]
```

Group `0` holds `{1, 3, 6}`, group `1` holds `{2, 5}`, group `2` holds `{4}`.

Example 2. An unused group is left at the `-inf` starting value.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

std::cout << values.grouped_max(group_ids, 4);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[6, 5, 4, -inf]
```

Example 3. The backward pass routes each group maximum's sensitivity to the
winning member, splitting ties.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

values.grouped_max(group_ids, 2).sum().reverse();

pluss::series::print(std::cout, values.sensitivity());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0.0000, 0.3333, 0.3333, 0.3333, 0.0000, 0.0000, 0.0000, 1.0000]
```

Group `0`'s maximum `4` is shared by three positions, so each receives `1/3`.
Group `1`'s maximum `9` is unique, so it takes the whole `1`.

Example 4. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_max(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_max: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- An empty group reports `-inf`, the reduction's starting value. Check
  [`grouped_count`](grouped_count.md) if that is possible in your data.
- A `NaN` in a group can leak into the result depending on comparison order;
  screen values first if `NaN` is expected.
- The result is `float64` even for an integer value column.

## Summary

`grouped_max()` reduces each group to its largest value:

- entry `g` is the maximum over values labelled `g`
- empty groups are `-inf`
- always floating point
- differentiable: the group maximum's sensitivity goes to the winning member,
  ties split evenly

## See Also

- [`Series::grouped_min`](grouped_min.md): the mirror reduction, starting at
  `+inf`.
- [`Series::grouped_sum`](grouped_sum.md): additive reduction over the same
  layout.
- [`Series::max`](max.md): the whole-series maximum with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_count.html">
    <span class="label">Previous</span>
    <span class="title">grouped_count</span>
  </a>
  <a class="next-link" href="grouped_mean.html">
    <span class="label">Next</span>
    <span class="title">grouped_mean</span>
  </a>
</div>
