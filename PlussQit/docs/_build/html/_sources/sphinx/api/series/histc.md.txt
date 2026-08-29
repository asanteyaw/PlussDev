<!-- pluss-hand-authored -->

# Series::histc

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::histc(std::int64_t bins, double min, double max) const -> Series
```

## What It Does

`histc()` counts how many values fall into numeric ranges.

This is the range-bucket sibling of [`bincount()`](bincount.md):

- `bincount()` groups by exact non-negative integer value
- `histc()` groups by numeric interval

The output has exactly `bins` buckets. Values outside the chosen range are
ignored. Non-finite values are also ignored.

## Parameters

**bins** (`std::int64_t`)

How many histogram buckets to create.

- must be greater than `0`
- larger values mean narrower buckets
- `1` means one inclusive bucket for the whole range

**min** (`double`)

The lower edge of the histogram range.

**max** (`double`)

The upper edge of the histogram range.

These two parameters work together:

- if `min == 0.0` and `max == 0.0`, the range is inferred from the finite
  values in the input
- otherwise the explicit range `[min, max]` is used

Values below `min` or above `max` do not contribute to any bucket.

## Returns

`Series`
    A 1D `int64` series of length `bins`.

Each element is the count for one numeric bucket.

## Gradient

`histc()` is not a differentiable numeric transform.

It produces bucket counts from piecewise range membership, so it should be
treated as counting/control logic.

## Examples

Example 1. With `min = 0.0` and `max = 0.0`, the method infers the finite data
range first.

```cpp
auto hist_values = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0, 3.5});

pluss::series::print(std::cout, hist_values.histc(4, 0.0, 0.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 2, 1, 1]
```

Example 2. An explicit range changes which values count and how the buckets are
partitioned.

```cpp
std::cout << hist_values.histc(4, 0.0, 2.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 2]
```

Here `3.5` is outside the explicit range and is ignored.

Example 3. Out-of-range values and non-finite values are skipped rather than
forcing an error.

```cpp
std::cout << pluss::make_series({
    -1.0,
    0.25,
    0.75,
    std::numeric_limits<double>::infinity(),
    1.25,
    2.25,
}).histc(4, 0.0, 2.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 0]
```

Only `0.25`, `0.75`, and `1.25` contribute.

Example 4. With one bin, every in-range finite value lands in that single
bucket.

```cpp
std::cout << hist_values.histc(1, 0.0, 2.0);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[5]
```

The value `3.5` stays out because it is above `max = 2.0`.

Example 5. Invalid bin counts fail explicitly.

```cpp
try {
  std::cout << hist_values.histc(0, 0.0, 2.0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
histc: bins must be > 0
```

## Gotchas And Failure Modes

- `histc()` counts ranges, not exact values.
- `min == 0.0` and `max == 0.0` is a special inference mode, not literally the
  fixed range `[0, 0]`.
- Out-of-range and non-finite values are skipped, not clamped.
- `bins = 1` is legal and means one inclusive bucket.

## Summary

Use `histc()` when you want range frequencies rather than exact label counts.

- choose `bins` to control resolution
- pass `0.0, 0.0` when you want automatic finite-range inference
- pass explicit `min` and `max` when the analysis range must be fixed
- expect ignored contributions from out-of-range and non-finite values

## See Also

- [`Series::bincount`](bincount.md): exact integer-label counting.
- [`Series::unique`](unique.md): distinct extraction rather than counting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="has_sensitivity.html">
    <span class="label">Next</span>
    <span class="title">has_sensitivity</span>
  </a>
</div>
