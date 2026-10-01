<!-- pluss-hand-authored -->

# Series::isneginf

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::isneginf() const
```

## Identify Only Negative Infinity

`isneginf()` performs an elementwise directional infinity test. An output lane
is `true` exactly when the corresponding receiver value is negative infinity.
Finite negative numbers, signed zero, positive infinity, and `NaN` are all
`false`.

Use this method when the sign of an infinite result carries meaning, such as
distinguishing negative overflow from positive overflow. Use [`isinf()`](isinf.md)
when both directions should be selected, or [`isfinite()`](isfinite.md) when
the desired mask is the complement of all infinities and `NaN` values.

The output is a newly allocated Boolean `Series` with the same rank, shape,
element order, and platform as the receiver. Floating dtypes can contain the
matching value. Integral and Boolean receivers are accepted and produce an
all-false result. Complex receivers are rejected because a complex number has
no single ordered sign; test a deliberate component with `real()` or `imag()`
when component-wise classification is intended.

The predicate is non-differentiable. A Boolean result does not carry a reverse
node, even when the receiver tracks sensitivity. Use the mask to select or
replace values in a subsequent expression, not as a numeric surrogate for a
derivative.

## Examples

Example 1. Only the first lane is negative infinity. Neither negative zero nor
`NaN` matches.

```cpp
const double inf = std::numeric_limits<double>::infinity();
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({-inf, -1.0, -0.0, 0.0, 1.0, inf, nan});
pluss::series::print(std::cout, x.isneginf());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=bool, platform=cpu, numel=7, contiguous=true)
data=[true, false, false, false, false, false, false]
```

Example 2. Higher-rank shape is preserved lane for lane.

```cpp
const double inf = std::numeric_limits<double>::infinity();
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{-inf, 2.0}, {inf, nan}});
pluss::series::print(std::cout, x.isneginf());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡  true false ⎤
⎣ false false ⎦
```

Example 3. Integers cannot encode infinity, so every lane is false.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2, 0, 2}).isneginf());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, false]
```

Example 4. Combine the two directional predicates when later logic needs an
all-infinity mask while retaining the option to inspect direction separately.

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

Example 5. Complex input requires an explicit component choice.

```cpp
try {
  using Complex = std::complex<double>;
  const double inf = std::numeric_limits<double>::infinity();
  (void)pluss::make_series(std::vector<Complex>{{inf, 0.0}}, {1})
      .isneginf();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
isneginf: complex input is not supported
```

## See Also

- [`isposinf()`](isposinf.md) selects positive infinity only.
- [`isinf()`](isinf.md) selects infinity in either direction.
- [`isfinite()`](isfinite.md) selects finite values.
- [`isnan()`](isnan.md) selects not-a-number values.
