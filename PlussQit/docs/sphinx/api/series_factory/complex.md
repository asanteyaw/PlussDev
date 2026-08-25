<!-- pluss-hand-authored -->

# pluss::series::complex

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::complex(const Series& real,
                       const Series& imag,
                       ScalarType dtype = ScalarType::Complex64) -> Series
```

## What It Builds

`complex()` combines two real-valued tensors into one complex tensor.

That gives it a very different shape from the rest of the factory surface:

- it is not shape-first
- it is not value-scalar-first
- it is pairwise tensor-first

The contract is strict:

- `real` and `imag` must have the same shape
- they must live on the same platform
- the destination dtype must be `Complex64` or `Complex128`

## Verified Examples

Example 1. `Complex64` combines float32 real and imaginary lanes elementwise.

```cpp
auto real32 = pluss::series::make({
    {1.0f, 2.0f},
    {3.0f, 4.0f},
}, pluss::ScalarType::Float32);
auto imag32 = pluss::series::make({
    {0.5f, 1.5f},
    {2.5f, 3.5f},
}, pluss::ScalarType::Float32);

std::cout << pluss::series::complex(real32, imag32, pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=complex64, platform=cpu, numel=4, contiguous=true)
data=[[(1+0.5j), (2+1.5j)],
  [(3+2.5j), (4+3.5j)]]
```

Example 2. `Complex128` does the same construction at double precision.

```cpp
auto real64 = pluss::series::make({
    {1.0, 2.0},
    {3.0, 4.0},
}, pluss::ScalarType::Float64);
auto imag64 = pluss::series::make({
    {0.25, 0.5},
    {0.75, 1.0},
}, pluss::ScalarType::Float64);

std::cout << pluss::series::complex(real64, imag64, pluss::ScalarType::Complex128);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[[(1+0.25j), (2+0.5j)],
  [(3+0.75j), (4+1j)]]
```

Example 3. Scalar inputs are allowed too.

```cpp
std::cout << pluss::series::complex(
    pluss::series::make(2.0),
    pluss::series::make(-0.5),
    pluss::ScalarType::Complex64);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=complex64, platform=cpu, numel=1, contiguous=true)
data=(2-0.5j)
```

Example 4. Integer-valued lanes can still be packed into a complex destination.

```cpp
std::cout << pluss::series::complex(
    pluss::series::make({1, 2, 3}, pluss::ScalarType::Int32),
    pluss::series::make({4, 5, 6}, pluss::ScalarType::Int32),
    pluss::ScalarType::Complex128);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=complex128, platform=cpu, numel=3, contiguous=true)
data=[(1+4j), (2+5j), (3+6j)]
```

Example 5. Shape mismatches fail before any construction happens.

```cpp
try {
  auto bad_imag = pluss::series::make({1.0, 2.0});
  std::cout << pluss::series::complex(real64, bad_imag, pluss::ScalarType::Complex128) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
complex: real and imag must have the same shape
```

Example 6. Non-complex destination dtypes are rejected.

```cpp
try {
  std::cout << pluss::series::complex(real64, imag64, pluss::ScalarType::Float32) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
complex: dtype must be Complex64 or Complex128
```

## Notes

- `complex()` is elementwise pairing, not broadcasting composition.
- Destination dtype controls complex precision, not shape.

## See Also

- [`pluss::series::polar`](polar.md): build complex numbers from magnitude and angle instead of real and imaginary parts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="multinomial.html">
    <span class="label">Previous</span>
    <span class="title">multinomial</span>
  </a>
  <a class="next-link" href="polar.html">
    <span class="label">Next</span>
    <span class="title">polar</span>
  </a>
</div>
