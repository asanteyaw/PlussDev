<!-- pluss-hand-authored -->

# Series::fftshift

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::fftshift() const
Series Series::fftshift(pluss::msharp::RangeView::Int dims) const
```

## Move Zero Frequency To The Centre

Many discrete transforms store the zero-frequency bin at index zero, followed
by positive frequencies and then wrapped negative frequencies. `fftshift()`
rotates selected axes so that the zero-frequency region is presented in the
centre instead. It changes layout, not spectral values.

For an axis of length `N`, the method rolls by `floor(N / 2)` positions. An
even-length axis splits exactly in half. On an odd-length axis, the right-hand
side contains one more element, so the result begins at index
`ceil(N / 2)`. [`ifftshift()`](ifftshift.md) uses the complementary odd-length
rotation and is the exact inverse.

The no-argument form shifts every axis. The `dims` overload shifts only the
listed axes and accepts negative indexing. Passing an explicitly empty list has
the same all-axis meaning as the no-argument form. If an axis appears more than
once, each occurrence is applied in order; callers should normally list each
axis once.

Shape, dtype, platform, and values are preserved. The output is an out-of-place
reordering assembled through the platform's roll operation. No Fourier
transform is performed: call this after a transform when centred presentation
or centred filtering is needed, and call `ifftshift()` before an inverse
transform if the consuming operation expects index-zero layout.

## Reverse Mode

`fftshift()` is a permutation. Reverse mode applies the inverse roll to the
incoming sensitivity. It neither scales nor combines lanes, so weighted output
sensitivities return to their corresponding pre-shift positions exactly.

## Examples

Example 1. An even vector exchanges its two equal halves.

```cpp
pluss::series::print(
    std::cout,
    pluss::series::arange(0, 8, 1, pluss::ScalarType::Int64).fftshift());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[4, 5, 6, 7, 0, 1, 2, 3]
```

Example 2. For odd length seven, index four becomes the first element.

```cpp
pluss::series::print(
    std::cout,
    pluss::series::arange(0, 7, 1, pluss::ScalarType::Int64).fftshift());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[4, 5, 6, 0, 1, 2, 3]
```

Example 3. With no `dims`, both matrix axes move.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
pluss::series::print(std::cout, grid.fftshift());
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 10 11 8 9 ⎤
⎢  2  3 0 1 ⎥
⎣  6  7 4 5 ⎦
```

Example 4. A dimension list can shift rows only.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::vector<std::int64_t> dims{0};
pluss::series::print(std::cout, grid.fftshift(dims));
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 8 9 10 11 ⎤
⎢ 0 1  2  3 ⎥
⎣ 4 5  6  7 ⎦
```

Example 5. Negative `-1` selects the last axis only.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::vector<std::int64_t> dims{-1};
pluss::series::print(std::cout, grid.fftshift(dims));
```

### Output

```text
Series(shape=[3,4], strides=[4,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡  2  3 0 1 ⎤
⎢  6  7 4 5 ⎥
⎣ 10 11 8 9 ⎦
```

Example 6. The inverse round trip is exact for odd and even axes.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
std::cout << std::boolalpha << grid.fftshift().ifftshift().equal(grid);
```

### Output

```text
true
```

Example 7. Reverse mode returns shifted weights to their source positions.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);
x.fftshift()
 .mul(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[3.0000, 4.0000, 5.0000, 1.0000, 2.0000]
```

Example 8. Every selected axis must exist.

```cpp
auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64)
                .reshape({3, 4});
try {
  std::vector<std::int64_t> dims{2};
  (void)grid.fftshift(dims);
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

- [`ifftshift()`](ifftshift.md) restores index-zero frequency layout.
- [`roll()`](roll.md) performs a general circular shift with explicit amounts.
- [`dct()`](dct.md) and [`dst()`](dst.md) compute real transform coefficients.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factorial.html">
    <span class="label">Previous</span>
    <span class="title">factorial</span>
  </a>
  <a class="next-link" href="fill.html">
    <span class="label">Next</span>
    <span class="title">fill</span>
  </a>
</div>
