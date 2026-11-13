<!-- pluss-hand-authored -->

# Series::grouped_mode

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_mode(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_mode()` returns the most frequent value in each group.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D label per value, same length, each label in
  `[0, group_count)`
- entry `g` of the length-`group_count` result is the value that occurs most
  often among the members labelled `g`

When two or more values are tied for most frequent, the smallest of the tied
values is returned. That rule also settles the case where every member of a
group is distinct: they all have frequency `1`, so the group's minimum is
reported.

Like [`grouped_median`](grouped_median.md), the mode needs a group's members
together rather than one at a time, so `grouped_mode` gathers each group before
reducing it.

A group with no members has mode `NaN`.

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

`grouped_mode()` is not differentiable. A `Series` produced by it does not carry
reverse tracking.

## Examples

Example 1. Group `1` has a clear mode; group `0` is all distinct, so its
minimum is reported.

```cpp
auto values = pluss::make_series({1.0, 9.0, 5.0, 2.0, 2.0, 8.0, 2.0, 7.0});
auto group_ids = pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_mode(group_ids, 2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 2.0000]
```

Group `0` is `{1, 9, 5}`, all frequency `1`, so the smallest value `1` is
returned. Group `1` is `{2, 2, 8, 2, 7}`, where `2` occurs three times.

Example 2. An empty group is `NaN`; a distinct group still reports its minimum.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_mode(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 3.0000, nan]
```

Group `0` is `{1, 2}` so the tie resolves to `1`. Group `1` is `{3, 9}` so it
resolves to `3`. Group `2` is empty.

Example 3. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, 9, 1}, pluss::ScalarType::Int64);
  values.grouped_mode(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_mode: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- On a frequency tie, the smallest tied value wins. An all-distinct group
  therefore returns its minimum, not a `NaN` or a "no mode" marker.
- An empty group is `NaN`.
- The result is `float64`, so integer values come back as whole floating-point
  numbers.
- Not differentiable.
- `grouped_mode` compares values for exact equality. On floating-point data
  that was computed rather than entered literally, near-duplicates will not be
  counted together.

## Summary

`grouped_mode()` is the most frequent value per group:

- entry `g` is the mode of the values labelled `g`
- ties, including all-distinct groups, resolve to the smallest candidate
- empty groups are `NaN`
- always `float64`; not differentiable

## See Also

- [`Series::grouped_median`](grouped_median.md): the middle value per group.
- [`Series::grouped_count`](grouped_count.md): group sizes, useful alongside a
  mode.
- [`Series::mode`](mode.md): the whole-series mode with no grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped_min.html">
    <span class="label">Previous</span>
    <span class="title">grouped_min</span>
  </a>
  <a class="next-link" href="grouped_std.html">
    <span class="label">Next</span>
    <span class="title">grouped_std</span>
  </a>
</div>
