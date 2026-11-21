<!-- pluss-hand-authored -->

# pluss::series::logspace

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::logspace(double start, double end, std::int64_t steps, double base = 10.0, ScalarType dt = ScalarType::Float32)
Series pluss::series::logspace(double start, double end, std::int64_t steps, double base, Config cfg)
Series pluss::series::logspace(double start, double end, std::int64_t steps, Config cfg)
```

## What It Builds

Builds `steps` points spaced evenly on a logarithmic scale: `base^start` to `base^end`. The exponents are evenly spaced, so the
ratio between neighbouring points is constant.

## Parameters

**start**, **end** (`double`)

The first and last exponents.

**steps** (`std::int64_t`)

The number of points. It must be positive.

**base** (`double`, default `10.0`)

The base of the exponent. It must be positive and not `1`.

**dt** (`ScalarType`, default `Float32`) or **cfg** (`Config`)

The dtype, or the dtype and platform. With a `Config` the dtype must be `Float32` or `Float64`.

## Returns

`Series`
    A rank-1 series of length `steps`.

## Errors

`steps <= 0` is refused with `logspace: expected steps > 0`, and a base that is not positive or equals `1` with
`logspace: expected base > 0 and base != 1`.

## Examples

Example 1. Four points from 10^0 to 10^3.

```cpp
std::cout << pluss::series::logspace(0.0, 3.0, 4);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1, 10, 100, 1000]
```

Example 2. A different base, in double precision.

```cpp
std::cout << pluss::series::logspace(0.0, 4.0, 5, 2.0, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 4, 8, 16]
```

Example 3. Choose dtype and platform with a `Config`.

```cpp
std::cout << pluss::series::logspace(1.0, 3.0, 3, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 100, 1000]
```

Example 4. `steps` must be positive and the base must not be 1.

```cpp
try {
  auto out = pluss::series::logspace(0.0, 1.0, 0);
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
try {
  auto out = pluss::series::logspace(0.0, 1.0, 3, 1.0);
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
logspace: expected steps > 0
logspace: expected base > 0 and base != 1
```

## Notes

- The default dtype is `Float32`. Ask for `Float64` when the large end of the range needs the precision.

## See Also

- [`pluss::series::linspace`](linspace.md): evenly spaced points.
