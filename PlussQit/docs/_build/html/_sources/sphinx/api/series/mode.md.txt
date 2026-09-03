<!-- pluss-hand-authored -->

# Series::mode

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::mode() const -> Series
Series::mode(std::int64_t dim) const -> Series
```

## What It Does

`mode()` returns the value with the greatest frequency. The global overload
counts values across the complete Series; `mode(dim)` counts separately in
each lane along `dim` and removes that axis from the output shape.

Ties have a deterministic current rule: the smallest tied value wins. This is
not merely a display detail. It makes repeatable results possible, but it also
means `mode()` does not return every equally frequent candidate and does not
return their counts.

## Parameters

**dim** (`std::int64_t`) is the axis whose lanes are counted independently.
Negative axes count from the end. No `keepdim` or counts-returning overload is
provided here.

## Returns

A Series containing modal values, with the reduced axis removed. The result
retains value semantics; it is not an index Series.

## Gradient

Frequency grouping and tie resolution are discrete operations. Mode is not a
smooth statistic and should be treated as a decision boundary in a gradient
pipeline.

## Examples

Example 1. The global mode is the most frequently occurring value.

```cpp
auto values = pluss::make_series({4.0, 2.0, 4.0, 3.0, 2.0, 4.0});
pluss::series::print(std::cout, values.mode());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.0000
```

Example 2. Reducing the final axis computes one mode per row.

```cpp
auto table = pluss::make_series({
    {1.0, 2.0, 2.0, 3.0},
    {4.0, 4.0, 5.0, 5.0},
});
std::cout << table.mode(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 4]
```

The second row ties `4` and `5` with count two, so the smaller value `4` wins.

Example 3. `dim=0` changes the population to each column.

```cpp
std::cout << table.mode(0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 2, 3]
```

Every column is tied one-to-one, so each result is its smallest candidate.

Example 4. An empty global population has no candidate mode.

```cpp
try { std::cout << pluss::series::zeros({0}).mode(); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
mode: cannot reduce an empty Series
```

Example 5. Empty lanes are also rejected for an axis reduction.

```cpp
try { std::cout << pluss::series::zeros({2, 0}).mode(1); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
mode: cannot reduce an empty dimension
```

## Gotchas And Failure Modes

- Mode returns values, not source positions or occurrence counts.
- Ties select the smallest value.
- Exact equality defines a group; NaN does not compare equal to itself.
- Empty populations throw instead of producing a placeholder value.

## Summary

Use `mode()` for one most-frequent value or `mode(dim)` for one per lane. The
result is deterministic under ties, because the smallest tied value is chosen,
and empty reductions are explicit errors.

## See Also

- [`Series::median`](median.md)
- [`Series::unique`](unique.md)
- [`Series::bincount`](bincount.md)
