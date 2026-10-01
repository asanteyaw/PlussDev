<!-- pluss-hand-authored -->

# Series::isposinf

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::isposinf() const
```

## Identify Only Positive Infinity

`isposinf()` performs an elementwise directional infinity test. It returns
`true` exactly where the receiver contains positive infinity. Finite positive
values, either signed zero, negative infinity, and `NaN` return `false`.

The method is intended for logic where positive overflow or an unbounded upper
result must be distinguished from negative overflow. If direction does not
matter, [`isinf()`](isinf.md) states that intent directly. If all invalid
floating values matter, combine the relevant infinity mask with
[`isnan()`](isnan.md).

The result is a newly allocated Boolean `Series`. Rank, shape, lane order, and
platform are preserved; dtype is always `bool`. Integral and Boolean receivers
are valid and yield all false because those dtypes have no infinity encoding.
Complex receivers are rejected because positive and negative ordering is not
defined for a complex value. Project `real()` or `imag()` first when a specific
component is what should be classified.

Predicate output is non-differentiable. It intentionally forms no reverse-mode
node, regardless of whether the receiver tracks sensitivity.

## Examples

Example 1. Only positive infinity matches; positive finite values and `NaN` do
not.

```cpp
const double inf = std::numeric_limits<double>::infinity();
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({-inf, -1.0, -0.0, 0.0, 1.0, inf, nan});
pluss::series::print(std::cout, x.isposinf());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=bool, platform=cpu, numel=7, contiguous=true)
data=[false, false, false, false, false, true, false]
```

Example 2. Matrix structure survives unchanged while each lane is classified.

```cpp
const double inf = std::numeric_limits<double>::infinity();
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{-inf, 2.0}, {inf, nan}});
pluss::series::print(std::cout, x.isposinf());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡ false false ⎤
⎣  true false ⎦
```

Example 3. An integer receiver produces a shape-preserving all-false mask.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2, 0, 2}).isposinf());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, false]
```

Example 4. Directional masks can be used independently before being joined.
The joined mask still excludes `NaN`.

```cpp
const double inf = std::numeric_limits<double>::infinity();
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({-inf, -1.0, -0.0, 0.0, 1.0, inf, nan});
pluss::series::print(
    std::cout,
    x.isneginf().logical_or(x.isposinf()));
```

### Output

```text
Series(shape=[7], strides=[1], dtype=bool, platform=cpu, numel=7, contiguous=true)
data=[true, false, false, false, false, true, false]
```

Example 5. Complex input is rejected rather than assigned an arbitrary order.

```cpp
try {
  using Complex = std::complex<double>;
  const double inf = std::numeric_limits<double>::infinity();
  (void)pluss::make_series(std::vector<Complex>{{inf, 0.0}}, {1})
      .isposinf();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
isposinf: complex input is not supported
```

## See Also

- [`isneginf()`](isneginf.md) selects negative infinity only.
- [`isinf()`](isinf.md) selects infinity in either direction.
- [`isfinite()`](isfinite.md) selects finite values.
- [`isnan()`](isnan.md) selects not-a-number values.
