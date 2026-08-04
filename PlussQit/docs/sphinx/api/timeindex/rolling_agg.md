# TimeIndex::rolling_agg

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::rolling_agg(
    const pluss::Series& values,
    const Duration& window,
    std::string_view op) const -> pluss::Series

TimeIndex::rolling_mean(const pluss::Series& values, const Duration& window) const -> pluss::Series
TimeIndex::rolling_min(const pluss::Series& values, const Duration& window) const -> pluss::Series
TimeIndex::rolling_max(const pluss::Series& values, const Duration& window) const -> pluss::Series
TimeIndex::rolling_first(const pluss::Series& values, const Duration& window) const -> pluss::Series
TimeIndex::rolling_last(const pluss::Series& values, const Duration& window) const -> pluss::Series
```

## What It Does

Computes trailing window aggregations selected either dynamically by operation
name or directly through named wrapper methods.

Use `rolling_agg(...)` when the operation is configuration-driven. Use the named
wrapper when you want the call site to state the aggregation explicitly.

This is the rolling counterpart to [`resample_agg`](resample_agg.md). The
trailing `window` decides membership. The wrapper name or `op` string decides
how the visible values are reduced.

## Parameters

**values**

The 1D numeric payload aligned case-for-case with the index.

**window**

The trailing duration window.

The same window semantics apply to every named wrapper here. Only the reducer
changes.

**op**

The aggregation name when using the dynamic form.

Common choices include:

- `"mean"`
- `"min"`
- `"max"`
- `"first"`
- `"last"`

## Returns

`pluss::Series`
    A 1D numeric `Series` with one rolling result per original timestamp.

## Examples

Example 1. Rolling mean over 45 minutes.

```cpp
auto values = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5});
auto win = pluss::duration::minutes(45);
auto out = base.rolling_mean(values, win);
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.5000, 2.0000, 3.0000, 4.0000]
```

Example 2. Rolling minimum over the same window.

```cpp
auto out = base.rolling_min(values, win);
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
```

Example 3. Rolling maximum over the same window.

```cpp
auto out = base.rolling_max(values, win);
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 4. Rolling first keeps the earliest value still visible inside each
trailing window.

```cpp
auto out = idx.rolling_first(values, win);
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 5. Rolling last keeps the most recent value still visible inside each
window.

```cpp
auto out = idx.rolling_last(values, win);
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
```

Example 6. Dynamic operation dispatch.

```cpp
auto out = idx.rolling_agg(values, win, "mean");
std::cout << pluss::print_series(out) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.5000, 2.0000, 3.0000, 4.0000]
```

## Notes

- The dynamic and named forms share the same window semantics.
- Use the named wrappers when the reduction is known in code.
- Use `rolling_agg()` when the reduction is chosen at runtime.
- Keep the named wrappers when the operation is fixed in code; they document
  intent better.

## See Also

- [`TimeIndex::rolling_sum`](rolling_sum.md)
- [`TimeIndex::rolling_count`](rolling_count.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_sum.html">
    <span class="label">Previous</span>
    <span class="title">rolling_sum</span>
  </a>
  <a class="next-link" href="join_asof.html">
    <span class="label">Next</span>
    <span class="title">join_asof</span>
  </a>
</div>
