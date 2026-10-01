<!-- pluss-hand-authored -->

# Series::cplxpair

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cplxpair() const
```

## Put Conjugate Roots Next To Each Other

`cplxpair()` reorganizes a series whose non-real values occur in complex-
conjugate pairs. Each pair is made adjacent, the member with negative imaginary
part comes first, and pairs are ordered by real part and then by imaginary
magnitude. Values with zero imaginary part follow all conjugate pairs in
ascending real order.

This is useful after a polynomial-root or eigenvalue calculation when a real
problem produces conjugate roots but does not promise a presentation order.
The method turns that unordered result into a deterministic layout without
discarding either component of a complex value.

Pairing is global, not axis-local. The receiver is read in logical flattened
order, reordered once, and reshaped to the original dimensions. Consequently,
the original shape is metadata retained around one ordered sequence; rows or
planes are not paired independently.

For floating roundoff, two values qualify as conjugates when their real parts
and opposite imaginary parts agree within a scale-aware tolerance of
`100 * epsilon * max(1, abs(a), abs(b))`. A tiny imaginary component within
that tolerance is treated as zero. Every non-real value must still have one
unused partner. An unmatched value is rejected rather than silently moved into
the real tail.

For a real-valued receiver there is nothing to pair. `cplxpair()` therefore
performs one global ascending sort and restores the original shape. Complex
input retains its complex dtype; real input retains its dtype. The result is an
out-of-place, contiguous series on the receiver's platform.

## Reverse Mode

The operation changes positions, not values. Reverse mode applies the inverse
permutation to the incoming sensitivity, so each source element receives the
sensitivity attached to its ordered destination. Pair discovery itself is
discrete: gradients describe the selected ordering and do not differentiate
through a change in which two values qualify as partners.

## Examples

Example 1. Conjugate pairs come first; real values form the sorted tail.

```cpp
using Complex = std::complex<double>;
auto roots = pluss::make_series(std::vector<Complex>{
    {3.0, 0.0}, {2.0, 4.0}, {1.0, -2.0}, {2.0, -4.0},
    {-1.0, 0.0}, {1.0, 2.0}}, {6});
pluss::series::print(std::cout, roots.cplxpair());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=complex128, platform=cpu, numel=6, contiguous=true)
data=[(1.0000-2.0000j), (1.0000+2.0000j), (2.0000-4.0000j), (2.0000+4.0000j), (-1.0000+0.0000j), (3.0000+0.0000j)]
```

Example 2. A real matrix is sorted globally, not one row at a time.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{4, 1, 3}, {0, -2, 8}}).cplxpair());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -2 0 1 ⎤
⎣  3 4 8 ⎦
```

Example 3. Higher-rank shape is retained even though pairing is global.

```cpp
using Complex = std::complex<double>;
auto paired = pluss::make_series(std::vector<Complex>{
    {2.0, 1.0}, {2.0, -1.0}, {1.0, 3.0}, {1.0, -3.0}}, {2, 2})
                  .cplxpair();
std::cout << "dtype=" << paired.scalar_type()
          << " dim=" << paired.dim()
          << " shape=[" << paired.size(0) << ',' << paired.size(1) << ']';
```

### Output

```text
dtype=complex128 dim=2 shape=[2,2]
```

Example 4. Weighted reverse mode reveals the inverse permutation. The original
first element moves to the fourth output position, so it receives weight four.

```cpp
using Complex = std::complex<double>;
auto x = pluss::make_series(std::vector<Complex>{
    {2.0, 1.0}, {1.0, -3.0}, {2.0, -1.0}, {1.0, 3.0}}, {4});
x.tracks_slope_(true);
x.cplxpair()
 .real()
 .mul(pluss::make_series({1.0, 2.0, 3.0, 4.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(4.0000+0.0000j), (1.0000+0.0000j), (3.0000+0.0000j), (2.0000+0.0000j)]
```

Example 5. An unmatched non-real value is a contract error.

```cpp
using Complex = std::complex<double>;
try {
  (void)pluss::make_series(
      std::vector<Complex>{{1.0, 2.0}, {3.0, 0.0}}, {2}).cplxpair();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
cplxpair: complex value has no conjugate partner
```

## See Also

- [`conj()`](conj.md) forms elementwise complex conjugates without reordering.
- [`sort()`](sort.md) orders values without enforcing pair adjacency.
- [`real()`](real.md) and [`imag()`](imag.md) project complex components.
