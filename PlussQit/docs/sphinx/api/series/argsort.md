<!-- pluss-hand-authored -->

# Series::argsort

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::argsort() const -> Series
Series::argsort(std::int64_t dim) const -> Series
Series::argsort(std::int64_t dim, bool descending) const -> Series
Series::argsort(std::int64_t dim, bool descending, bool stable) const -> Series
```

## What It Does

`argsort()` returns an ordering plan rather than reordered values. Every output
entry is an `int64` position identifying which source entry should appear at
that place in the sorted lane. The result has the same shape as the source,
but its dtype is always `int64`.

This distinction is fundamental:

- [`sort()`](sort.md) answers "what are the values in order?"
- `argsort()` answers "which source positions produce that order?"

The no-argument overload orders the final axis. An explicit `dim` chooses the
axis whose local positions are returned. All other axes remain independent
lanes. `descending` reverses the desired order; `stable` decides whether equal
values retain their original relative order.

## Overload Map

- `argsort()` is ascending along the final axis.
- `argsort(dim)` is ascending along `dim`.
- `argsort(dim, descending)` chooses ascending or descending direction.
- `argsort(dim, descending, stable)` also gives equal values a deterministic
  source-order policy when `stable=true`.

## Parameters

**dim** (`std::int64_t`)

The axis that each returned index addresses. Positive dimensions count from the
front; negative dimensions count from the end. The numbers in the output are
local positions on this axis, not flattened addresses into the entire series.

**descending** (`bool`)

- `false`: smallest values appear first.
- `true`: largest values appear first.

It changes comparison direction only. The result remains an index series.

**stable** (`bool`)

- `true`: if two values compare equal, their source order is preserved.
- `false`: equal-value order is deliberately unspecified.

Use stable ordering when indices carry records, labels, or any other aligned
meaning that must survive ties. Do not infer stability from a particular run
when this toggle is `false`.

## Returns

`Series`
    An `int64` series whose shape matches the input. Applying those indices
    along the same axis yields the ordered values.

## Gradient

`argsort()` creates control information, not a differentiable numeric value
path. Treat its output as indices for a subsequent selection or reordering
operation rather than as part of a reverse-mode objective.

## Examples

Example 1. The default orders the final axis ascending.

```cpp
auto scores = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
pluss::series::print(std::cout, scores.argsort());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[1, 3, 0, 2, 4]
```

Read this as `scores[1], scores[3], scores[0], scores[2], scores[4]`, which
are `1, 1, 3, 4, 5` in ascending order.

Example 2. Descending order reverses the plan.

```cpp
std::cout << scores.argsort(0, true);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[4, 2, 0, 1, 3]
```

Now the plan reads the values as `5, 4, 3, 1, 1`.

Example 3. `dim=0` orders each column independently, so the results are row
positions.

```cpp
auto table = pluss::make_series({{4.0, 1.0, 3.0}, {2.0, 5.0, 1.0}});
std::cout << table.argsort(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=int64, platform=cpu, numel=6, contiguous=false)
data=[[1, 0, 1],
  [0, 1, 0]]
```

For column `0`, row `1` (`2`) comes before row `0` (`4`), so that column's
index lane is `[1, 0]`.

Example 4. `dim=1` instead returns column positions for each row.

```cpp
std::cout << table.argsort(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 0],
  [2, 0, 1]]
```

The same numeric index can therefore mean a row or a column position; `dim`
defines that interpretation.

Example 5. Stable ordering makes equal-value provenance explicit.

```cpp
auto tied = pluss::make_series({2LL, 1LL, 2LL, 1LL});
std::cout << tied.argsort(0, false, true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 3, 0, 2]
```

The equal `1`s retain source order `1, 3`; the equal `2`s retain `0, 2`.

Example 6. Apply an ordering plan with `index_select()` to obtain the values.

```cpp
auto order = scores.argsort();
std::cout << scores.index_select(0, order);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 3, 4, 5]
```

This separates discovery of the ordering from use of that ordering, which is
useful when the same indices must reorder several aligned series.

Example 7. The selected dimension must exist.

```cpp
try { std::cout << scores.argsort(1); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
argsort: dim out of range
```

## Gotchas And Failure Modes

- `argsort()` returns indices; streaming it does not show sorted values.
- Indices are local to `dim`; they cannot be used as flat `take()` positions
  for a multi-dimensional source without changing their meaning.
- Complex-valued inputs are rejected because this operation requires an order.
- Equal values have a reliable relative-order guarantee only with
  `stable=true`.

## Summary

Use `argsort()` when ordering itself is reusable information. Keep the result
with the series it describes and apply it along the same axis to all aligned
data that must be reordered consistently.

## See Also

- [`Series::sort`](sort.md)
- [`Series::topk`](topk.md)
- [`Series::index_select`](index_select.md)

