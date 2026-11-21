<!-- pluss-hand-authored -->

# pluss::series::nan

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::nan(ScalarType dt = ScalarType::Float32)
Series pluss::series::nan(Config cfg)
```

## What It Builds

Builds a rank-0 series holding "not a number". `NaN` is a value, not a missing cell: it is different from `NULL`, `isnan` finds it, and
it is not equal to itself.

## Parameters

**dt** (`ScalarType`, default `Float32`) or **cfg** (`Config`)

The dtype, or the dtype and platform. It must be a floating-point dtype.

## Returns

`Series`
    A rank-0 series.

## Examples

Example 1. A `NaN` in the default `Float32`.

```cpp
std::cout << pluss::series::nan();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 2. A `NaN` in double precision.

```cpp
std::cout << pluss::series::nan(pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 3. NaN is not equal to itself.

```cpp
auto x = pluss::series::nan(pluss::ScalarType::Float64);

std::cout << x.eq(x) << x.isnan();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=falsepluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Notes

- Use [`pluss::series::inf`](inf.md) for infinity.

## See Also

- [`pluss::series::inf`](inf.md): infinity.
- [`Series::isnan`](../series/isnan.md): find `NaN` values.
