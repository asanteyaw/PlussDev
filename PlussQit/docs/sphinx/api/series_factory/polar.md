<!-- pluss-hand-authored -->

# pluss::series::polar

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::polar(const Series& abs,
                            const Series& angle,
                            ScalarType dtype = ScalarType::Complex64)
```

## What It Builds

`polar()` constructs complex numbers from magnitude and angle instead of from
real and imaginary parts directly.

In other words:

- `abs` supplies the radius
- `angle` supplies the phase
- the factory converts those into complex Cartesian coordinates

Like [`complex()`](complex.md), this is a pairwise tensor constructor with a
strict shape contract.

## Verified Examples

Example 1. `Complex64` polar construction turns magnitudes and angles into
complex coordinates.

```cpp
auto magnitudes = pluss::series::make({1.0, 2.0}, pluss::ScalarType::Float64);
auto angles = pluss::series::make({0.0, 1.5707963267948966}, pluss::ScalarType::Float64);

std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=complex64, platform=cpu, numel=2, contiguous=true)
data=[(1+0j), (-8.7422777e-08+2j)]
```

That tiny real residue on the second value is the ordinary floating-point
approximation artifact you should expect around `cos(pi / 2)`.

Example 2. `Complex128` keeps the same geometry with higher complex precision.

```cpp
std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Complex128);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=complex128, platform=cpu, numel=2, contiguous=true)
data=[(1+0j), (1.2246468e-16+2j)]
```

Example 3. Scalar polar construction is allowed.

```cpp
std::cout << pluss::series::polar(
    pluss::series::make(2.0),
    pluss::series::make(3.14159265358979323846),
    pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=complex64, platform=cpu, numel=1, contiguous=true)
data=(-2-1.7484555e-07j)
```

Example 4. Matrix inputs are handled elementwise, just like `complex()`.

```cpp
std::cout << pluss::series::polar(
    pluss::series::make({
        {1.0, 2.0},
        {3.0, 4.0},
    }),
    pluss::series::make({
        {0.0, 1.5707963267948966},
        {3.14159265358979323846, 4.7123889803846898577},
    }),
    pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=[[(1+0j), (-8.7422777e-08+2j)],
  [(-3-2.6226832e-07j), (4.7699523e-08-4j)]]
```

Example 5. Zero magnitudes dominate the result regardless of angle.

```cpp
std::cout << pluss::series::polar(
    pluss::series::make({0.0, 0.0}),
    pluss::series::make({0.0, 1.0}),
    pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=complex64, platform=cpu, numel=2, contiguous=true)
data=[(0+0j), (0+0j)]
```

Example 6. Non-complex destination dtypes are rejected.

```cpp
try {
  std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Float32) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
polar: dtype must be Complex64 or Complex128
```

Example 7. Shape mismatches fail immediately.

```cpp
try {
  auto bad_angles = pluss::series::make({
      {0.0, 1.0},
      {2.0, 3.0},
  });
  std::cout << pluss::series::polar(magnitudes, bad_angles, pluss::ScalarType::Complex64) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
polar: abs and angle must have the same shape
```

## Notes

- `polar()` is mathematically different from `complex()`, even though both end in a complex tensor.
- Small near-zero residuals are normal floating-point trigonometric artifacts.

## See Also

- [`pluss::series::complex`](complex.md): pair real and imaginary parts directly instead.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="complex.html">
    <span class="label">Previous</span>
    <span class="title">complex</span>
  </a>
  <a class="next-link" href="from_torch.html">
    <span class="label">Next</span>
    <span class="title">from_torch</span>
  </a>
</div>
