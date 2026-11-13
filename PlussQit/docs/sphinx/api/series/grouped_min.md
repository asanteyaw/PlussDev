<!-- pluss-hand-authored -->

# Series::grouped_min

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_min(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_min()` returns the smallest value in each group.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is `min { self[i] : group_ids[i] == g }`

Each group's accumulator starts at `+inf` and folds in members with a running
minimum. A group with no members keeps that `+inf`.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

## Returns

`Series`
    A 1-D `Series` of length `group_count`, dtype `float64` on CPU (`float32`
    off-CPU). Empty groups are `+inf`.

## Gradient

`grouped_min()` is differentiable.

Backward sends each group minimum's sensitivity to the member that achieved it,
splitting evenly on ties.

## Examples

Example 1. Smallest value per group, three groups.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

std::cout << values.grouped_min(group_ids, 3);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 4]
```

Example 2. An unused group is left at the `+inf` starting value.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

std::cout << values.grouped_min(group_ids, 4);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 4, inf]
```

Example 3. Backward routes each group minimum's sensitivity to the member that
achieved it, splitting ties.

```cpp
auto values = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
values.tracks_slope_(true);
auto group_ids = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, pluss::ScalarType::Int64);

values.grouped_min(group_ids, 2).sum().reverse();

pluss::series::print(std::cout, values.sensitivity());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000, 0.0000, 0.0000, 0.0000, 0.5000, 0.5000, 0.0000, 0.0000]
```

Group `0`'s minimum `2` is unique, so position `0` takes the whole `1`. Group
`1`'s minimum `5` is shared by two positions, so each gets `1/2`.

Example 4. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, -1, 1}, pluss::ScalarType::Int64);
  values.grouped_min(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_min: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- An empty group reports `+inf`, the reduction's starting value.
- `NaN` handling depends on comparison order; screen values first if `NaN` is
  expected.
- The result is `float64` even for an integer value column.

## Summary

`grouped_min()` reduces each group to its smallest value:

- entry `g` is the minimum over values labelled `g`
- empty groups are `+inf`
- always floating point
- differentiable: the group minimum's sensitivity goes to the achieving
  member, ties split evenly

## See Also

- [`Series::grouped_max`](grouped_max.md): the mirror reduction, starting at
  `-inf`.
- [`Series::grouped_median`](grouped_median.md): a robust middle instead of the
  extreme.
- [`Series::min`](min.md): the whole-series minimum with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_median.html">
    <span class="label">Previous</span>
    <span class="title">grouped_median</span>
  </a>
  <a class="next-link" href="grouped_mode.html">
    <span class="label">Next</span>
    <span class="title">grouped_mode</span>
  </a>
</div>
