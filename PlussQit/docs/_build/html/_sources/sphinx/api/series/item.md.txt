<!-- pluss-hand-authored -->

# Series::item

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
template<class T>
T Series::item() const

Scalar Series::item() const
```

## Crossing Into A C++ Scalar

`item()` extracts the value of a rank-zero `Series`. It is the deliberate
boundary between array computation and ordinary C++ control flow: use it when
a result must become a loop condition, log value, test assertion, configuration
number, or argument to an API that accepts a native scalar rather than a
`Series`.

The precondition is deliberately about rank, not merely element count. Shape
`[]` is a scalar and may be extracted. Shapes `[1]`, `[1,1]`, and higher-rank
singletons remain arrays and are rejected. Use [`pull()`](pull.md) for a 1-D
Series or [`host()`](host.md) for a rank-aware host representation.

The typed overload, `item<T>()`, converts the value to the requested C++ type.
The untyped overload preserves the Series dtype in a tagged `pluss::Scalar`,
which is useful when generic code does not know the dtype at compile time.
Complex extraction must be explicit through `item<pluss::Complex64>()` or
`item<pluss::Complex128>()`; the tagged scalar intentionally has no complex
storage alternative.

## Ownership, Platform, And Autodiff

The returned value owns its scalar bits and does not alias Series storage.
Extracting from an accelerator-backed Series performs the required host
synchronization and transfer, so repeated per-element `item()` calls are a poor
way to inspect a device result. Transfer or aggregate once, then extract.

`item()` is not differentiable because its return value is no longer a
`Series`. It does not mutate or detach the receiver, but no reverse-mode edge
can continue through the native C++ value. Keep calculations in Series form
until the final observation boundary.

## Examples

### 1. Extract native Boolean, integer, and floating values

```cpp
std::cout << std::boolalpha
          << "bool=" << pluss::make_series(true).item<bool>() << '\n'
          << "int=" << pluss::make_series(7).item<std::int64_t>() << '\n'
          << "double=" << pluss::make_series(3.5).item<double>() << '\n';
```

#### Output

```text
bool=true
int=7
double=3.5
```

Spell the destination type explicitly when downstream C++ code requires an
exact type. Numeric conversion follows the same representability constraints
as the corresponding host materialization.

### 2. Scalar rank, not `numel()`, controls the contract

```cpp
const auto singleton = pluss::make_series({42.0});
std::cout << "dim=" << singleton.dim()
          << " values=" << singleton.pull<double>().front() << '\n';

try {
  (void)singleton.item<double>();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
dim=1 values=42
Series::item<T>(): expected a rank-zero Series
```

The one-element vector is intentionally not collapsed into a scalar. This
keeps rank visible at the C++ boundary and prevents accidental acceptance of a
mis-shaped result.

### 3. Preserve runtime dtype with the tagged overload

```cpp
std::cout << pluss::make_series(7).item() << '\n'
          << pluss::make_series(2.25).item() << '\n';
```

#### Output

```text
Scalar(dtype=int64, value=7)
Scalar(dtype=float64, value=2.25)
```

`pluss::Scalar` carries both the value and its `ScalarType`. Use this overload
for dtype-generic inspection; use `item<T>()` when a native type is already
known.

### 4. Extract a complex scalar explicitly

```cpp
using Complex = std::complex<double>;

const auto z = pluss::make_series(
    std::vector<Complex>{{2.0, -3.0}}, {});
std::cout << z.item<Complex>() << '\n';
```

#### Output

```text
(2,-3)
```

The empty shape passed to the factory creates a rank-zero complex Series.
Calling the untyped `item()` overload on this value is rejected because the
tagged scalar does not erase a complex value into a misleading real scalar.

### 5. Extract a final reduction, not every source lane

```cpp
const auto values = pluss::make_series({1.5, 2.5, 3.5, 4.5});
const double total = values.sum().item<double>();
std::cout << total << '\n';
```

#### Output

```text
12
```

This performs one host extraction after the reduction. It is preferable to
calling `select(...).item<double>()` four times, especially on an accelerator.

### 6. Reject ambiguous extraction

```cpp
try {
  (void)pluss::make_series({1, 2}).item<std::int64_t>();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
Series::item<T>(): expected a rank-zero Series
```

Reduce or select explicitly before extraction. The failure protects code from
silently reading only the first lane of a larger result.

## See Also

- [`dim()`](dim.md) reports whether the receiver is rank zero.
- [`pull()`](pull.md) exports a rank-one Series as `std::vector<T>`.
- [`host()`](host.md) exports any rank without collapsing its structure.
- [`select()`](select.md) chooses one lane while remaining inside Series.
- [`tensor()`](tensor.md) provides the external tensor interoperability
  boundary in enabled builds.
