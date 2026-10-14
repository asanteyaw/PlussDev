<!-- pluss-hand-authored -->

# Series::median

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::median() const -> Series
Series::median(std::int64_t dim) const -> Series
```

## What It Does

`median()` finds the centre of an ordered population. For an odd number of
values, that is the one middle value. For an even number there are two middle
values and the result is the lower of them, as `torch.median` does, so the
result is always a value that occurs in the source. Use
[`quantile(0.5)`](quantile.md) for the interpolated midpoint of an even
population. The result is `float64`.

The no-argument overload uses every element as one population. The axis
overload works independently along every lane of `dim`, then removes that axis
from the result shape. NaN is not treated as missing: it propagates through a
reduced population. Choose [`nanmedian()`](nanmedian.md) when omission is the
intended data policy.

## Overload Map

| Call | Population | Result |
| --- | --- | --- |
| `x.median()` | every value in `x` | scalar Series |
| `x.median(dim)` | each lane along `dim` | `x.shape()` without `dim` |

## Parameters

**dim** (`std::int64_t`) selects the axis to reduce. `0` is the outermost axis;
`-1` is the final axis. It must identify an existing axis. There is no
`keepdim` overload, so the reduced dimension is always removed.

## Returns

A new `float64` Series. The source is unchanged. Global reduction has scalar
shape `[]`; axis reduction has one result for every remaining coordinate.

## Gradient

Median is an order statistic. The identity of the selected middle observation
can change abruptly when values cross or tie, so it is not a smooth reduction
to use as a differentiable objective.

## Examples

Example 1. An even population returns its lower centre value.

```cpp
auto values = pluss::make_series({4.0, 1.0, 3.0, 2.0});
pluss::series::print(std::cout, values.median());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.0000
```

Ordering gives `[1, 2, 3, 4]`; the lower of the two centres is `2`, while
`quantile(0.5)` would interpolate to `2.5`.

Example 2. `dim=0` makes every column its own ordered population.

```cpp
auto table = pluss::make_series({
    {1.0, 8.0, 3.0},
    {4.0, 2.0, 6.0},
    {7.0, 5.0, 9.0},
});
std::cout << table.median(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 6]
```

The columns order to `[1, 4, 7]`, `[2, 5, 8]`, and `[3, 6, 9]`.

Example 3. `-1` reduces the final axis, here each row.

```cpp
std::cout << table.median(-1);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 4, 7]
```

Example 4. Ordinary median preserves the presence of NaN.

```cpp
auto missing = pluss::make_series({
    1.0, std::numeric_limits<double>::quiet_NaN(), 5.0, 9.0});
std::cout << missing.median();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 5. The NaN-skipping variant filters before ordering.

```cpp
std::cout << missing.median() << "\n";
std::cout << missing.nanmedian();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 6. Out-of-range axes are rejected rather than flattened.

```cpp
try { std::cout << table.median(3); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
median: dim out of range
```

## Gotchas And Failure Modes

- This returns a value, not its source index.
- NaN propagates; infinity remains an ordered endpoint.
- The axis diagnostic is `median: dim out of range`.
- Even populations return the lower centre, not the mean of the two. The
  difference from `quantile(0.5)` is intentional and matches libtorch.

## Summary

Use `median()` for one robust centre or `median(dim)` for one per lane. It
returns the lower centre of even populations, removes the reduced axis, and
preserves NaN rather than silently excluding it.

## See Also

- [`Series::nanmedian`](nanmedian.md)
- [`Series::quantile`](quantile.md)
- [`Series::mode`](mode.md)
