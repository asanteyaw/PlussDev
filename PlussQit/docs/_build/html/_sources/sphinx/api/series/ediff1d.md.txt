<!-- pluss-hand-authored -->

# Series::ediff1d

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ediff1d() const
```

## Consecutive Differences In Logical Flattened Order

`ediff1d()` flattens the receiver in logical row-major order and subtracts each
value from its successor:

```text
out[i] = flat[i + 1] - flat[i]
```

For `N` input elements, the result is always a one-dimensional series of
length `max(N - 1, 0)`. This makes the method a concise choice for gaps in a
single logical sequence, including a higher-rank value that should deliberately
be treated as one sequence. The difference that crosses a row or plane boundary
is included.

Use [`diff()`](diff.md) instead when an axis has semantic meaning and each row,
column, or batch member must remain independent. `ediff1d()` has no axis,
order, prepend, or append option; flattening is its defining contract.

Subtraction follows the receiver's value domain. Integer input produces integer
differences, floating input preserves floating precision, and complex input
preserves both real and imaginary components. The operation allocates a new,
contiguous result on the receiver's platform. A singleton or empty receiver
produces shape `[0]` rather than throwing.

## Reverse Mode

For incoming sensitivity `g`, the flattened source sensitivity is

```text
dx[0]     = -g[0]
dx[i]     =  g[i - 1] - g[i]    for interior positions
dx[N - 1] =  g[N - 2]
```

The sensitivity is then restored to the receiver's original shape. Summing all
differences telescopes to `last - first`, while weighted differences expose the
full adjacent contribution pattern.

## Examples

Example 1. Consecutive integer gaps retain integer dtype.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({3, 8, 6, 10}).ediff1d());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[5, -2, 4]
```

Example 2. Matrix rows are not independent: `16 - 9` is the middle result.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{1, 4, 9}, {16, 25, 36}}).ediff1d());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[3, 5, 7, 9, 11]
```

Example 3. Complex subtraction retains both components.

```cpp
using Complex = std::complex<double>;
pluss::series::print(
    std::cout,
    pluss::make_series(
        std::vector<Complex>{{1.0, 1.0}, {4.0, -1.0}, {2.0, 3.0}}, {3})
        .ediff1d());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=complex128, platform=cpu, numel=2, contiguous=true)
data=[(3.0000-2.0000j), (-2.0000+4.0000j)]
```

Example 4. A singleton has no adjacent pair.

```cpp
auto gaps = pluss::make_series({7.0}).ediff1d();
pluss::series::print(std::cout, gaps);
std::cout << "dtype=" << gaps.scalar_type()
          << " dim=" << gaps.dim() << " shape=[" << gaps.size(0) << ']';
```

### Output

```text
Series(shape=[0], strides=[1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
dtype=float64 dim=1 shape=[0]
```

Example 5. Weighted differences show how neighbouring terms accumulate during
reverse mode.

```cpp
auto x = pluss::make_series({2.0, 5.0, 11.0, 20.0});
x.tracks_slope_(true);
x.ediff1d().mul(pluss::make_series({1.0, 2.0, 4.0})).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1.0000, -1.0000, -2.0000, 4.0000]
```

## See Also

- [`diff()`](diff.md) computes differences along a chosen axis and supports
  higher orders.
- [`cumsum()`](cumsum.md) forms prefix sums rather than adjacent gaps.
- [`flatten()`](flatten.md) makes the sequence interpretation explicit before
  other operations.
