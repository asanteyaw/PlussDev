<!-- pluss-hand-authored -->

# Series::isreal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::isreal() const
```

## What It Answers

`isreal()` classifies every element independently and returns a Boolean
`Series` with exactly the receiver's shape. A non-complex element is always
real. A complex element is real precisely when its imaginary component
compares equal to zero; the real component may have any finite or infinite
value.

This is a value predicate, not a dtype query. Use `is_complex()` when the
question is whether the storage dtype can represent imaginary components. Use
`isreal()` when a complex-valued computation has already happened and the
question is which resulting lanes actually lie on the real axis.

The distinction matters because one complex `Series` can contain both kinds of
lanes. A value such as `2 - 0i` is real, including when the imaginary zero is
negative. A value with an imaginary NaN is not real because NaN does not equal
zero.

## Result Contract

- The result has dtype `bool`, the same rank and extents as the receiver, and
  lives on the same platform.
- Real, integer, and Boolean receivers produce `true` in every lane.
- Complex receivers compare only the imaginary component with zero. The real
  component does not affect classification.
- The operation allocates a new Boolean result and does not mutate the
  receiver.
- The predicate is discrete and does not create a reverse-mode derivative.
  Apply it as a mask; do not expect sensitivity to pass through the decision.
- Empty inputs produce empty Boolean results with the same shape.

## Examples

### 1. A real-valued matrix is real in every lane

```cpp
const auto values = pluss::make_series({{1.0, -2.0}, {0.0, 4.0}});
pluss::series::print(std::cout, values.isreal());
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡ true true ⎤
⎣ true true ⎦
```

The method does not ask whether the values are positive, finite, or nonzero.
All native real-valued lanes satisfy the predicate.

### 2. A complex matrix is classified lane by lane

```cpp
using Complex = std::complex<double>;

const auto values = pluss::make_series(
    std::vector<Complex>{{1.0, 0.0}, {2.0, -0.0},
                         {3.0, 4.0}, {0.0, -2.0}},
    {2, 2});

pluss::series::print(std::cout, values.isreal());
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡  true  true ⎤
⎣ false false ⎦
```

Both positive and negative zero imaginary components compare equal to zero.
The final two values have nonzero imaginary components and therefore produce
`false`.

### 3. Scalar rank is preserved

```cpp
const auto scalar = pluss::make_series(4.5);
pluss::series::print(std::cout, scalar.isreal());
```

#### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

The result is still rank zero. `isreal()` never adds, removes, or flattens an
axis.

## Performance

The operation is linear in `numel()`. Non-complex inputs use an all-true fast
path; complex inputs inspect one imaginary component per lane. The result is a
new contiguous Boolean allocation, so retain it when the same mask will be
reused by several later operations.

## See Also

- [`real()`](real.md) extracts real components as numeric values.
- [`imag()`](imag.md) extracts the component tested by this predicate.
- [`is_complex()`](is_complex.md) queries storage capability rather than lane
  values.
- [`isfinite()`](isfinite.md) classifies finite numeric values.
