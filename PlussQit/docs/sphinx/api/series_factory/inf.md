<!-- pluss-hand-authored -->

# pluss::series::inf

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::inf(ScalarType dt = ScalarType::Float32)
Series pluss::series::inf(Config cfg)
```

## What It Builds

Builds a rank-0 series holding positive infinity. Negate it for negative infinity. Infinity is a value, not a missing cell: it is different
from `NULL` and from `NaN`.

## Parameters

**dt** (`ScalarType`, default `Float32`) or **cfg** (`Config`)

The dtype, or the dtype and platform. It must be a floating-point dtype.

## Returns

`Series`
    A rank-0 series.

## Examples

Example 1. Positive infinity in the default `Float32`.

```cpp
std::cout << pluss::series::inf();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=inf
```

Example 2. In double precision, and its negation.

```cpp
auto x = pluss::series::inf(pluss::ScalarType::Float64);

std::cout << x << x.neg();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=infpluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-inf
```

Example 3. Infinity is a value, not a missing cell.

```cpp
auto x = pluss::series::inf(pluss::ScalarType::Float64);

std::cout << x.isinf() << x.isnan();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=truepluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

## Notes

- Use [`pluss::series::nan`](nan.md) for "not a number".

## See Also

- [`pluss::series::nan`](nan.md): not a number.
- [`Series::isinf`](../series/isinf.md): find infinite values.
