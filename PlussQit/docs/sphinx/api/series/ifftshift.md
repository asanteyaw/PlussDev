<!-- pluss-hand-authored -->

# Series::ifftshift

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::ifftshift() const
Series Series::ifftshift(pluss::msharp::RangeView::Int dims) const
```

## Restore Index-Zero Frequency Layout

`ifftshift()` circularly reorders selected axes from a centered frequency
layout back to the layout whose zero-frequency bin begins at index zero. It is
the exact inverse of [`fftshift()`](fftshift.md).

For an axis of length `N`, `ifftshift()` rolls by `ceil(N / 2)`. For an even
length this equals `N / 2`, so `fftshift()` and `ifftshift()` perform the same
half exchange. For an odd length, their rotations differ by one position:
`fftshift()` uses `floor(N / 2)`, while `ifftshift()` uses `ceil(N / 2)`.
Interchanging the two on odd axes silently misaligns bins.

The no-argument form shifts every axis. The `dims` overload shifts only the
listed axes and accepts negative indexing. An explicitly empty list has the
same all-axis meaning as omitting `dims`. Repeated axes are not deduplicated;
each occurrence applies another roll in list order.

Shape, dtype, platform, and element values are preserved. Only positions
change. The method allocates an out-of-place result through the platform's roll
operation; it does not perform an inverse Fourier transform.

## Reverse Mode

`ifftshift()` is a permutation. Reverse mode applies the complementary
permutation to incoming sensitivity, returning every output weight to the
source position that produced it. No values are scaled or combined.

## Examples

Example 1. An even-length vector exchanges equal halves.

```cpp
pluss::series::print(
    std::cout,
    pluss::series::arange(0, 8, 1, pluss::ScalarType::Int64).ifftshift());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[4, 5, 6, 7, 0, 1, 2, 3]
```

Example 2. At odd length seven, the inverse rotation begins with index three,
not index four.

```cpp
pluss::series::print(
    std::cout,
    pluss::series::arange(0, 7, 1, pluss::ScalarType::Int64).ifftshift());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[3, 4, 5, 6, 0, 1, 2]
```

Example 3. With no dimension list, every matrix axis is restored.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
pluss::series::print(std::cout, grid.ifftshift());
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡  6  7 4 5 ⎤
⎢ 10 11 8 9 ⎥
⎣  2  3 0 1 ⎦
```

Example 4. A dimension list can restore the row axis only.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::vector<std::int64_t> dims{0};
pluss::series::print(std::cout, grid.ifftshift(dims));
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 4 5  6  7 ⎤
⎢ 8 9 10 11 ⎥
⎣ 0 1  2  3 ⎦
```

Example 5. Negative `-1` selects only the last axis.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::vector<std::int64_t> dims{-1};
pluss::series::print(std::cout, grid.ifftshift(dims));
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡  2  3 0 1 ⎤
⎢  6  7 4 5 ⎥
⎣ 10 11 8 9 ⎦
```

Example 6. Applying the complementary shift restores both odd and even axes.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::cout << std::boolalpha
          << grid.ifftshift().fftshift().equal(grid);
```

### Output

```text
true
```

Example 7. Repeated dimensions are applied repeatedly. On a length-four axis,
two inverse half-rolls compose to the identity.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::vector<std::int64_t> dims{1, 1};
pluss::series::print(std::cout, grid.ifftshift(dims));
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 0 1  2  3 ⎤
⎢ 4 5  6  7 ⎥
⎣ 8 9 10 11 ⎦
```

Example 8. Reverse mode applies the inverse permutation to output weights.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);
x.ifftshift()
 .mul(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[4.0000, 5.0000, 1.0000, 2.0000, 3.0000]
```

Example 9. Every listed axis must exist.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
try {
  std::vector<std::int64_t> dims{2};
  (void)grid.ifftshift(dims);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
fftshift/ifftshift: dim out of range
```

## See Also

- [`fftshift()`](fftshift.md) moves index-zero frequency layout to the centre.
- [`roll()`](roll.md) performs arbitrary circular shifts.
- [`idct()`](idct.md) and [`idst()`](idst.md) reconstruct samples rather than reorder bins.
- [`fft()`](fft.md), [`ifft()`](ifft.md), [`rfft()`](rfft.md) and [`irfft()`](irfft.md) produce the spectra that this reorders.

