<!-- pluss-hand-authored -->

# Series::grouped_median

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_median(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_median()` returns the median of the values within each group.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is the median of the values
  labelled `g`

The median is computed the usual way: sort the group's members, take the
middle one for an odd count, and average the two middle ones for an even count.
Unlike the additive and extreme reductions in this family, the median needs to
see all of a group's members together, so `grouped_median` gathers each group
before reducing it rather than folding values in one at a time.

A group with no members has median `NaN`.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in
`[0, group_count)`. Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`.

## Returns

`Series`
    A 1-D `float64` `Series` of length `group_count`. Empty groups are `NaN`.

## Gradient

`grouped_median()` is not differentiable. A `Series` produced by it does not
carry reverse tracking.

## Examples

Example 1. Three values in group `0`, five in group `1`.

```cpp
auto values = pluss::make_series({1.0, 9.0, 5.0, 2.0, 2.0, 8.0, 2.0, 7.0});
auto group_ids = pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_median(group_ids, 2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5.0000, 2.0000]
```

Group `0` sorted is `{1, 5, 9}`, middle value `5`. Group `1` sorted is
`{2, 2, 2, 7, 8}`, middle value `2`.

Example 2. Even-sized groups average the two middle values.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_median(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5000, 6.0000, nan]
```

Group `0` is `{1, 2}` so the median is `1.5`. Group `1` is `{3, 9}` so it is
`6`. Group `2` is empty, so it is `NaN`.

Example 3. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_median(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_median: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- An empty group is `NaN`.
- The result is always `float64`, and even-sized groups can produce a value
  that is not present in the data (the average of the two middle members).
- Not differentiable.
- `NaN` members participate in the sort and can push the median to `NaN`; screen
  them first if that is a concern.

## Summary

`grouped_median()` is the within-group median:

- entry `g` is the median of the values labelled `g`
- odd count takes the middle; even count averages the two middle values
- empty groups are `NaN`
- always `float64`; not differentiable

## See Also

- [`Series::grouped_mean`](grouped_mean.md): the mean, which is differentiable
  but not robust to outliers.
- [`Series::grouped_mode`](grouped_mode.md): the most frequent value per group.
- [`Series::median`](median.md): the whole-series median with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_mean.html">
    <span class="label">Previous</span>
    <span class="title">grouped_mean</span>
  </a>
  <a class="next-link" href="grouped_min.html">
    <span class="label">Next</span>
    <span class="title">grouped_min</span>
  </a>
</div>
