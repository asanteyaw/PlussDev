<!-- pluss-hand-authored -->

# Series::kthvalue

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::kthvalue(std::int64_t k) const
Series Series::kthvalue(std::int64_t k, std::int64_t dim) const
Series Series::kthvalue(std::int64_t k, std::int64_t dim, bool keepdim) const
```

## What It Does

`kthvalue()` selects one order statistic: the `k`-th smallest value in each
chosen lane. It does not return an index, and it does not require materializing
a full user-visible sorted result. Its rank is one-based, so `k=1` means the
smallest value and `k=size(dim)` means the largest.

The default uses the final axis. `dim` chooses another axis. `keepdim` decides
whether the reduced axis disappears or survives as length `1`; that is useful
when the selected value will broadcast back against the original series.

## Parameters

**k** (`std::int64_t`)

The one-based ascending rank. It must satisfy `1 <= k <= size(dim)`. Duplicate
values occupy separate ranks: for `[1, 1, 3]`, rank `1` and rank `2` are both
`1`.

**dim** (`std::int64_t`)

The axis within which ranks are evaluated. Negative dimensions normalize from
the end.

**keepdim** (`bool`)

- `false`: remove `dim` from the result shape.
- `true`: retain it with length `1`.

The selected value is unchanged; only result alignment changes.

## Returns

`Series`
    The selected value or values, represented as `float64` on the current
    surface. It does not return the source positions.

## Examples

Example 1. The third smallest value is the middle ordered value here.

```cpp
auto scores = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
std::cout << scores.kthvalue(3);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3
```

The ordered values are `[1, 1, 3, 4, 5]`; `k=3` therefore selects `3`.

Example 2. The endpoints recover the smallest and largest order statistics.

```cpp
std::cout << scores.kthvalue(1) << "\n";
std::cout << scores.kthvalue(5);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 3. `dim=0` selects one ranked value per column.

```cpp
auto table = pluss::make_series({{4.0, 1.0, 3.0}, {2.0, 5.0, 1.0}});
std::cout << table.kthvalue(2, 0);
```

### Output

```text
pluss::Series(shape=[3], strides=[2], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[4, 5, 3]
```

Example 4. `keepdim=true` preserves the ranked axis for later broadcasting.

```cpp
std::cout << table.kthvalue(2, 1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[2,2], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[[3],
  [2]]
```

Example 5. Zero is not a valid rank.

```cpp
try { std::cout << scores.kthvalue(0); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
kthvalue: k must be >= 1
```

Example 6. A rank beyond the lane length also fails.

```cpp
try { std::cout << scores.kthvalue(6); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
kthvalue: k must be <= size(dim)
```

## Gotchas And Failure Modes

- `k` is one-based; it is not a zero-based element offset.
- `kthvalue()` returns values. Use [`argsort()`](argsort.md) or
  [`topk_pair()`](topk.md) when source positions are required.
- Complex-valued inputs are rejected because this operation requires ordering.
- This is an ascending rank. To obtain the `r`-th largest value in a lane of
  length `n`, request `k = n - r + 1`.

## Summary

Use `kthvalue()` when one rank is the answer: thresholds, medians of odd-sized
lanes, and rank-based cutoffs. Use `topk()` or `sort()` only when several or
all ordered values are actually needed.

## See Also

- [`Series::topk`](topk.md)
- [`Series::sort`](sort.md)
- [`Series::median`](median.md)

