<!-- pluss-hand-authored -->

# Series::where

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::where(const Series& condition, const Series& other) const
Series Series::where(const Series& condition, double other) const
Series& Series::where_(const Series& condition, const Series& other)
Series& Series::where_(const Series& condition, double other)

Series pluss::series::where(const Series& condition, const Series& a, const Series& b)
Series pluss::series::where(const Series& condition, const Series& a, double b)
Series pluss::series::where(const Series& condition, double a, const Series& b)
Series pluss::series::where(const Series& condition, double a, double b)
```

## What It Does

`where` chooses between two values position by position. `x.where(condition, other)` keeps `x` where the condition is true and takes `other` where it is false. The free function writes the same operation with the condition first: `pluss::series::where(condition, a, b)` takes `a` where the condition is true and `b` elsewhere, and is identical to `a.where(condition, b)`.

The condition can be any dtype: a non-zero value is true. The condition, the kept values and the replacement broadcast against each other, and the result dtype follows the usual promotion of the two value operands. The in-place form `where_` overwrites the receiver with the result.

## Overload Map

**`x.where(condition, other_series)`** keeps `x` where the condition is true and takes the matching element of `other_series` elsewhere.

**`x.where(condition, scalar)`** takes the same scalar everywhere the condition is false. An integer or Bool receiver keeps its dtype for a whole-number scalar and widens to `float64` for a fractional or non-finite one, so a replacement is never truncated.

**`pluss::series::where(condition, a, b)`** is the condition-first spelling; `a` and `b` may each be a `Series` or a scalar.

**`x.where_(condition, other)`** is the in-place form.

## Parameters

**condition** (`const Series&`)

The selector. True keeps the first value (the receiver, or `a` in the free form) and false takes the second. Any dtype is accepted; a non-zero value counts as true.

**other** / **b** (`const Series&` or `double`)

The value taken where the condition is false. A scalar is broadcast to the shape of the other operand.

**a** (`const Series&` or `double`, free function only)

The value taken where the condition is true.

## Returns

`Series`
    A new series with the broadcast shape of the three operands. `where_` returns the receiver.

## Gradient

`where` is differentiable through the values that win. At each position the gradient flows into whichever operand was selected; the condition is control data and receives none.

## Examples

Example 1. Keep `x` where the condition is true; take a series elsewhere.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto big = x.gt(2.5);
std::cout << x.where(big, pluss::make_series({10.0, 20.0, 30.0, 40.0})) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 20, 3, 4]
```

Example 2. A scalar replacement.

```cpp
std::cout << x.where(big, 0.0) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 3, 4]
```

Example 3. The free function with the condition first, and with two scalars.

```cpp
std::cout << pluss::series::where(big, x, 0.0) << "\n";
std::cout << pluss::series::where(big, 1.0, 0.0) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 3, 4]
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 1, 1]
```

Example 4. A condition of any dtype: non-zero is true.

```cpp
std::cout << x.where(pluss::make_series({1.0, 0.0, 1.0, 0.0}), -1.0) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, -1, 3, -1]
```

Example 5. Broadcasting: a column condition selects whole rows of a matrix.

```cpp
auto m = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});
auto rows = pluss::make_series({true, false, true}).reshape({3, 1});
std::cout << m.where(rows, 0.0) << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [0, 0],
  [5, 6]]
```

Example 6. An integer series keeps its dtype for a whole-number replacement and widens for a fractional one.

```cpp
auto counts = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4});
std::cout << counts.where(counts.gt(2), 0.0) << "\n";
std::cout << counts.where(counts.gt(2), 0.5) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 3, 4]
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 0.5, 3, 4]
```

Example 7. In place.

```cpp
auto y = pluss::make_series({1.0, 2.0, 3.0, 4.0});
y.where_(y.gt(2.5), 0.0);
std::cout << y << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 3, 4]
```

Example 8. A NaN compares false, so a threshold condition does not keep it; test for it explicitly when that matters.

```cpp
auto with_nan = pluss::make_series({1.0, std::nan(""), 3.0});
std::cout << with_nan.where(with_nan.gt(0.0), -1.0) << "\n";
std::cout << with_nan.where(with_nan.isnan().logical_not(), -1.0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, -1, 3]
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, -1, 3]
```

## Errors

Operands whose shapes cannot broadcast fail with a `pluss::Error`.

Example 9. Five flags for four values.

```cpp
try {
  x.where(pluss::make_series({true, false, true, false, true}), 0.0);
} catch (const pluss::Error &e) {
  std::cout << "five flags for four values: " << e.message() << "\n";
}
```

### Output

```text
five flags for four values: broadcast: incompatible shapes
```

## Related

[`masked_fill`](masked_fill.md), [`masked_select`](masked_select.md), [`gt`](gt.md), [`isnan`](isnan.md).
