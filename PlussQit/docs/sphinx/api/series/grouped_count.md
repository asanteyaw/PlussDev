<!-- pluss-hand-authored -->

# Series::grouped_count

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::grouped_count(const Series& group_ids, std::int64_t group_count) const
```

## What It Does

`grouped_count()` counts how many values carry each group label.

- `self` is a 1-D `Series` of values, length `n`
- `group_ids` is a 1-D `Series` of the same length; `group_ids[i]` is the group
  of value `self[i]`
- the result is a 1-D `int64` `Series` of length `group_count`, where entry `g`
  is the number of positions `i` with `group_ids[i] == g`

The values themselves are not read for anything but their count. `self`
supplies the length and takes part in the shape check, but a `grouped_count`
over `[0, 0, 0, 0]` is the same as over `[9, -2, 100, 0]` as long as the labels
match. In practice you call it to normalize [`grouped_sum`](grouped_sum.md)
into a mean, or to find and skip empty groups.

A group that no element maps to has count `0`.

## Parameters

**group_ids** (`const Series&`)

A 1-D label per value, any integral or boolean dtype, each in `[0, group_count)`.
Out-of-range or negative labels raise.

**group_count** (`std::int64_t`)

The number of output groups, `>= 0`. The result length is exactly this value.

## Returns

`Series`
    A 1-D `int64` `Series` of length `group_count`. This is the one member of
    the family whose result is integer rather than floating point.

## Gradient

`grouped_count()` is not differentiable. It is a pure count of positions and
does not depend on the values.

## Examples

Example 1. Six values across three groups.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_count(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 2, 1]
```

Group `0` has positions `0`, `2`, `5`; group `1` has `1`, `4`; group `2` has
just `3`.

Example 2. The values are never inspected. All-zero values give the same
counts.

```cpp
auto values = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0});
auto group_ids = pluss::make_series({2, 2, 0, 2, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_count(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 3]
```

Example 3. An unused group has count `0`. This is how you detect the empty
groups that make [`grouped_mean`](grouped_mean.md) produce `NaN`.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 9.0});
auto group_ids = pluss::make_series({0, 0, 1, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.grouped_count(group_ids, 3));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 0]
```

Example 4. Turning a sum into a mean by hand. Cast the count to match the sum's
dtype before dividing.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto group_ids = pluss::make_series({0, 1, 0, 2, 1, 0}, pluss::ScalarType::Int64);

auto totals = values.grouped_sum(group_ids, 3);
auto counts = values.grouped_count(group_ids, 3).to(pluss::ScalarType::Float64);

pluss::series::print(std::cout, totals / counts);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.3333, 3.5000, 4.0000]
```

That is exactly what [`grouped_mean`](grouped_mean.md) computes.

Example 5. An out-of-range label is rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0, 3.0});
  auto group_ids = pluss::make_series({0, -1, 1}, pluss::ScalarType::Int64);
  values.grouped_count(group_ids, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::grouped_count: group id out of range; expected every id in [0, 2)
```

## Gotchas And Failure Modes

- The result dtype is `int64`, unlike the rest of the family.
- The values are not read; only their group labels matter.
- `self` and `group_ids` must be 1-D and the same length.
- Labels outside `[0, group_count)` raise, negatives included.

## Summary

`grouped_count()` returns the size of each group:

- entry `g` is the number of values labelled `g`
- empty groups are `0`
- result dtype is `int64`
- not differentiable

## See Also

- [`Series::grouped_sum`](grouped_sum.md): the total per group, which
  `grouped_count` normalizes into a mean.
- [`Series::grouped_mean`](grouped_mean.md): `grouped_sum / grouped_count`, done
  for you.
- [`Series::bincount`](bincount.md): counts of integer values with no separate
  label array.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gelu.html">
    <span class="label">Previous</span>
    <span class="title">gelu</span>
  </a>
  <a class="next-link" href="grouped_max.html">
    <span class="label">Next</span>
    <span class="title">grouped_max</span>
  </a>
</div>
