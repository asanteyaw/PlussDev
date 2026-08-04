# TimeIndex::rolling_sum

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::rolling_sum(const pluss::Series& values, const Duration& window) const -> pluss::Series
```

## What It Does

Computes a trailing time-window sum over a 1D numeric `Series`.

This is the rolling counterpart to [`resample_sum`](resample_sum.md): instead of
coarsening into disjoint bins, it emits one trailing-window sum at each original
timestamp.

The distinction matters:

- resampling changes the axis and produces fewer cases
- rolling keeps the axis and produces one value per original case

## Parameters

**values**

The 1D numeric values aligned with the time index.

**window**

The trailing duration to aggregate over.

The window is temporal, not positional. A value stays inside the accumulation
while its timestamp is still within the trailing duration boundary.

## Returns

`pluss::Series`
    A 1D numeric `Series` containing one rolling sum per original timestamp.

## Examples

Example 1. Compute a trailing 45-minute sum.

```cpp
auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
auto roll = idx.rolling_sum(values, pluss::duration::minutes(45));
std::cout << pluss::print_series(roll) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 3.0000, 6.0000, 9.0000, 12.0000]
```

Example 2. Widen the duration when you want each rolling value to include more
history.

```cpp
auto roll = idx.rolling_sum(values, pluss::duration::minutes(60));
std::cout << pluss::print_series(roll) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 3.0000, 6.0000, 10.0000, 14.0000]
```

## Notes

- The window is temporal, not row-count based.
- `values` must line up case-for-case with the index.
- This is usually the right primitive for trailing flow-like quantities.
- If you only need rolling observation density, use
  [`rolling_count`](rolling_count.md).

## See Also

- [`TimeIndex::rolling_count`](rolling_count.md)
- [`TimeIndex::resample_sum`](resample_sum.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_count.html">
    <span class="label">Previous</span>
    <span class="title">rolling_count</span>
  </a>
  <a class="next-link" href="join_asof.html">
    <span class="label">Next</span>
    <span class="title">join_asof</span>
  </a>
</div>
