# Complex numbers

PlussQit supports complex dtypes:

- `ScalarType::Complex64`  (complex<float>)
- `ScalarType::Complex128` (complex<double>)

## Arithmetic

Complex supports elementwise:
- add/sub/mul/div
- pow

## Unary math

Complex supports:
- exp/log/sqrt/sin/cos/tan (and other `UnaryMath` ops)

## Projections / helpers

- `x.conj()` : complex conjugate
- `x.real()` : real-part (float32/float64)
- `x.imag()` : imag-part (float32/float64)
- `x.angle()` : phase angle (float32/float64)

## Comparisons

Torch does not define ordered comparisons for complex.
PlussQit supports:
- `eq`, `ne`

and throws for `lt/le/gt/ge` on complex dtypes.

## Reductions

- `sum` and `mean` are supported for complex.
- `amax/amin` are not defined for complex and throw.
